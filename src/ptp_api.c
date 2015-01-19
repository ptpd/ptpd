/*-
 * Copyright (c) 2014      Rick Ratzel,
 *                         National Instruments.
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "ptpd.h"
#include "ptp_api.h"


#define USEC_PER_MSEC 1000
#define STOP_TIMEOUT_MSEC 10000  /* 10 seconds */

/*
 * PtpManagedSettings consists of copies of all PtpClock members that are
 * updated via an external management mechanism (1588 "native" management for
 * now, but possibly also SNMP in the future).
 *
 * These copies mean that the PtpClock instance can be deleted and re-created
 * without losing state.  This allows an application to recover from a PTP
 * faulty state without losing changes made by management.
 */
struct PtpManagedSettings {
   Boolean slaveOnly;
   UInteger8 priority1;
   UInteger8 priority2;
   UInteger8 domainNumber;
   Integer8 logAnnounceInterval;
   UInteger8 announceReceiptTimeout;
   Integer8 logSyncInterval;
   Enumeration8 delayMechanism;
   Integer8 logMinPdelayReqInterval;
   Octet user_description[USER_DESCRIPTION_MAX + 1];
   UInteger4 versionNumber;
   ClockQuality clockQuality;
   TimePropertiesDS timePropertiesDS;
};


/*
 * A PtpSession consists of a PtpClock and the corresponding RunTimeOpts.
 *
 * NOTE: the need for globals for instances of both of these structs (as well as
 * startupInProgress) still limits this to 1 PtpSession per process - this will
 * hopefully change soon.
*/
struct PtpSession {
   PtpClock* ptpClock;
   RunTimeOpts* rtOpts;
   Boolean protocolIsRunning;
};


/*
 * Global variable with the main PTP port. This is used to show the current
 * state in DBG()/message() without having to pass the pointer everytime.
 *
 * if ptpd is extended to handle multiple ports (eg, to instantiate a Boundary Clock),
 * then DBG()/message() needs a per-port pointer argument
 */
PtpClock *G_ptpClock = NULL;
RunTimeOpts *G_rtOpts = NULL;
Boolean startupInProgress;


static void
_deleteConfigDictionaries( PtpSession* ptpSess )
{
   if( ptpSess == NULL ) { return; }
   if( ptpSess->rtOpts == NULL ) { return; }

   dictionary_del( &(ptpSess->rtOpts->currentConfig) );
   dictionary_del( &(ptpSess->rtOpts->candidateConfig) );
   dictionary_del( &(ptpSess->rtOpts->cliConfig) );
}


static void
_createConfigDictionaries( PtpSession* ptpSess )
{
   /* currentConfig is created during setRTOptsFromCommandLine() */
   ptpSess->rtOpts->candidateConfig = dictionary_new( 0 );
   ptpSess->rtOpts->cliConfig = dictionary_new( 0 );
}


static void
_deletePtpRTOpts( PtpSession* ptpSess )
{
   if( ptpSess == NULL ) { return; }

   _deleteConfigDictionaries( ptpSess );
   free( ptpSess->rtOpts ); ptpSess->rtOpts = NULL;
}


static void
_saveManagedSettingsFromPtpClock( PtpManagedSettings* settings,
                                  PtpClock* ptpClock )
{
   assert( settings != NULL );
   assert( ptpClock != NULL );

   settings->slaveOnly = ptpClock->slaveOnly;
   settings->priority1 = ptpClock->priority1;
   settings->priority2 = ptpClock->priority2;
   settings->domainNumber = ptpClock->domainNumber;
   settings->logAnnounceInterval = ptpClock->logAnnounceInterval;
   settings->announceReceiptTimeout = ptpClock->announceReceiptTimeout;
   settings->logSyncInterval = ptpClock->logSyncInterval;
   settings->delayMechanism = ptpClock->delayMechanism;
   settings->logMinPdelayReqInterval = ptpClock->logMinPdelayReqInterval;
   settings->versionNumber = ptpClock->versionNumber;

   strncpy( settings->user_description, ptpClock->user_description,
            USER_DESCRIPTION_MAX+1 );

   settings->clockQuality.clockClass = ptpClock->clockQuality.clockClass;
   settings->clockQuality.clockAccuracy = ptpClock->clockQuality.clockAccuracy;
   settings->clockQuality.offsetScaledLogVariance = ptpClock->clockQuality.offsetScaledLogVariance;

   settings->timePropertiesDS.currentUtcOffset = ptpClock->timePropertiesDS.currentUtcOffset;
   settings->timePropertiesDS.currentUtcOffsetValid = ptpClock->timePropertiesDS.currentUtcOffsetValid;
   settings->timePropertiesDS.leap59 = ptpClock->timePropertiesDS.leap59;
   settings->timePropertiesDS.leap61 = ptpClock->timePropertiesDS.leap61;
   settings->timePropertiesDS.timeTraceable = ptpClock->timePropertiesDS.timeTraceable;
   settings->timePropertiesDS.frequencyTraceable = ptpClock->timePropertiesDS.frequencyTraceable;
   settings->timePropertiesDS.ptpTimescale = ptpClock->timePropertiesDS.ptpTimescale;
   settings->timePropertiesDS.timeSource = ptpClock->timePropertiesDS.timeSource;
}


static void
_restoreManagedSettingsOnPtpClock( PtpManagedSettings* settings,
                                   PtpClock* ptpClock, RunTimeOpts* rtOpts )
{
   assert( settings != NULL );
   assert( ptpClock != NULL );
   assert( rtOpts != NULL );

   /*
    * TODO: write automated test to ensure each variable is handled correctly.
    */
   ptpClock->slaveOnly = settings->slaveOnly;
   ptpClock->priority1 = settings->priority1;
   ptpClock->priority2 = settings->priority2;
   ptpClock->domainNumber = settings->domainNumber;
   ptpClock->logAnnounceInterval = settings->logAnnounceInterval;
   ptpClock->announceReceiptTimeout = settings->announceReceiptTimeout;
   ptpClock->logSyncInterval = settings->logSyncInterval;
   ptpClock->delayMechanism = settings->delayMechanism;
   ptpClock->logMinPdelayReqInterval = settings->logMinPdelayReqInterval;
   ptpClock->versionNumber = settings->versionNumber;

   strncpy( ptpClock->user_description, settings->user_description,
            USER_DESCRIPTION_MAX+1 );

   ptpClock->clockQuality.clockClass = settings->clockQuality.clockClass;
   ptpClock->clockQuality.clockAccuracy = settings->clockQuality.clockAccuracy;
   ptpClock->clockQuality.offsetScaledLogVariance = settings->clockQuality.offsetScaledLogVariance;

   ptpClock->timePropertiesDS.currentUtcOffset = settings->timePropertiesDS.currentUtcOffset;
   ptpClock->timePropertiesDS.currentUtcOffsetValid = settings->timePropertiesDS.currentUtcOffsetValid;
   ptpClock->timePropertiesDS.leap59 = settings->timePropertiesDS.leap59;
   ptpClock->timePropertiesDS.leap61 = settings->timePropertiesDS.leap61;
   ptpClock->timePropertiesDS.timeTraceable = settings->timePropertiesDS.timeTraceable;
   ptpClock->timePropertiesDS.frequencyTraceable = settings->timePropertiesDS.frequencyTraceable;
   ptpClock->timePropertiesDS.ptpTimescale = settings->timePropertiesDS.ptpTimescale;
   ptpClock->timePropertiesDS.timeSource = settings->timePropertiesDS.timeSource;

   /*
    * TODO: Make this better - ptpd calls initData() when the clock goes into
    * the INITIALIZING state, which resets various values on ptpClock from
    * rtOpts.  Set the same values on rtOpts here, so the clock settings will
    * persist after transitioning to INITIALIZING.  See initData() in bmc.c
    */
   rtOpts->clockQuality.clockClass = settings->clockQuality.clockClass;
   rtOpts->clockQuality.clockAccuracy = settings->clockQuality.clockAccuracy;
   rtOpts->clockQuality.offsetScaledLogVariance = settings->clockQuality.offsetScaledLogVariance;

   rtOpts->slaveOnly = settings->slaveOnly;
   rtOpts->priority1 = settings->priority1;
   rtOpts->priority2 = settings->priority2;
   rtOpts->domainNumber = settings->domainNumber;

   rtOpts->announceInterval = settings->logAnnounceInterval;
   rtOpts->announceReceiptTimeout = settings->announceReceiptTimeout;
   rtOpts->syncInterval = settings->logSyncInterval;
   rtOpts->delayMechanism = settings->delayMechanism;
   rtOpts->logMinPdelayReqInterval = settings->logMinPdelayReqInterval;
   /*
    * TODO: need to figure out how to restore version number - it is very
    * unlikely someone would want to change this though.
    */
}


/*
 *******************************************************************************
 * Public API
 *******************************************************************************
 */
int
ptp_initializeSession( PtpSession** ptpSess )
{
   PtpSession* newPtpSess = NULL;

   assert( ptpSess != NULL );

   newPtpSess = (PtpSession*) malloc( sizeof( PtpSession ) );
   if( newPtpSess == NULL ) {
      return ENOMEM;
   }

   newPtpSess->rtOpts = (RunTimeOpts*) malloc( sizeof( RunTimeOpts ) );
   if( newPtpSess->rtOpts == NULL ) {
      free( newPtpSess );
      return ENOMEM;
   }

   newPtpSess->ptpClock = NULL;
   newPtpSess->protocolIsRunning = FALSE;

   loadDefaultSettings( newPtpSess->rtOpts );

   _createConfigDictionaries( newPtpSess );

   *ptpSess = newPtpSess;

   return 0;
}


int
ptp_deleteSession( PtpSession* ptpSess )
{
   if( ptpSess == NULL ) {
      return EINVAL;
   }
   /* TODO: stop clock, etc. */

   _deletePtpRTOpts( ptpSess );

   /*
    * ptpSess->ptpClock should have already been free'd and set to NULL by
    * ptpdShutdown().
    */
   free( ptpSess );

   return 0;
}


int
ptp_setOptsFromCommandLine( PtpSession* ptpSess,
                            int argc, char** argv,
                            int* shouldRun )
{
   Integer16 retCode;

   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );

   /*
    * Remove the config dictionaries if they exist from a prior call in order to
    * allow additional config settings to be applied to rtOpts.  The most recent
    * rtOpts settings will then be reflected in rtOpts->currentConfig once this
    * call completes.
    */
   if( ptpSess->rtOpts->currentConfig != NULL ) {
      _deleteConfigDictionaries( ptpSess );
      _createConfigDictionaries( ptpSess );
   }

   /*
    * TODO: setRTOptsFromCommandLine() calls functions that require G_rtOpts to
    * be set - that needs to be fixed.
    */
   G_rtOpts = ptpSess->rtOpts;

   *shouldRun =
      (setRTOptsFromCommandLine( argc, argv, &retCode, ptpSess->rtOpts ) == TRUE) ? 1 : 0;

   return (int) retCode;
}


int
ptp_testNetworkConfig( PtpSession* ptpSess )
{
   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );

   if( testNetworkConfig( ptpSess->rtOpts ) ) {
      return 0;
   } else {
      return -1;
   }
}


int
ptp_checkConfigOnly( PtpSession* ptpSess )
{
   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );

   return (ptpSess->rtOpts->checkConfigOnly == TRUE);
}


int
ptp_dumpConfig( PtpSession* ptpSess, FILE* out )
{
   assert( ptpSess != NULL );
   assert( out != NULL );
   assert( ptpSess->rtOpts != NULL );

   if( ptpSess->rtOpts->currentConfig != NULL ) {
      dictionary_dump( ptpSess->rtOpts->currentConfig, out );
      return 0;
   }

   return EOPNOTSUPP;
}


int
ptp_duplicateConfig( PtpSession* ptpSess, PtpDictionary** configDict )
{
   assert( ptpSess != NULL );
   assert( configDict != NULL );
   assert( ptpSess->rtOpts != NULL );

   if( ptpSess->rtOpts->currentConfig == NULL ) {
      return ENODATA;
   }

   dictionary_del( configDict );
   if( (*configDict = dictionary_new( 0 )) == NULL ) {
      return ENOMEM;
   }

   /*
    * Pass 0 to indicate that no warning messages should be generated about vars
    * being clobbered and pass NULL as the warning string.
    */
   if( dictionary_merge( ptpSess->rtOpts->currentConfig, *configDict,
                         0, (const char*) NULL ) != 0 ) {
      /*
       * TODO: dictionary_merge() returns non-zero for internal errors (invalid
       * pointers, etc.) as well as "no memory" errors.
       */
      return ENOMEM;
   }

   return 0;
}


int
ptp_deleteConfigObject( PtpDictionary* configDict )
{
   if( configDict != NULL ) {
      dictionary_del( &configDict );
      return 0;
   }

   return EINVAL;
}


int
ptp_applyConfigObject( PtpSession* ptpSess, PtpDictionary* configDict )
{
   assert( ptpSess != NULL );
   assert( configDict != NULL );

   if( ptpSess->protocolIsRunning ) {
      return EBUSY;
   }

   /*
    * Cleanup any existing dictionaries since they are no longer needed: of the
    * 3 dictionaries, cliConfig and candidateConfig are only needed for
    * command-line parsing, and currentConfig is re-created in parseConfig()
    */
   _deletePtpRTOpts( ptpSess );

   /*
    * Recreate rtOpts with all default values before applying the new config.
    *
    * TODO: Look into using only loadDefaultSettings() instead - delete/malloc
    * will guarantee the RT opts are in a known state, but may not be necessary.
    */
   ptpSess->rtOpts = (RunTimeOpts*) malloc( sizeof( RunTimeOpts ) );
   if( ptpSess->rtOpts == NULL ) {
      return ENOMEM;
   }
   loadDefaultSettings( ptpSess->rtOpts );

   /*
    * Map all options from the config dictionary to corresponding rtOpts fields,
    * using existing rtOpts fields as defaults.  This returns a pointer to a new
    * set of resulting options in rtOpts, which should be assigned to
    * currentConfig.
    */
   ptpSess->rtOpts->currentConfig = parseConfig( configDict, ptpSess->rtOpts );

   return 0;
}



int
ptp_run( PtpSession* ptpSess, PtpManagedSettings** settingsObj )
{
   Integer16 ptpdStartupReturnVal = 0;
   Boolean cckInitSucceeded = FALSE;
   Boolean protocolSucceeded = FALSE;

   /*
    * TODO: Refactor internal ptpd functions to use the same return code scheme
    * as the ptp_api. The ptp_api returns an int (0 on success and non-zero on
    * error). Some internal ptpd functions like ptpdStartup return an int (0 on
    * success and either 2 or 3 on error). While others, like protocol(),
    * return a boolean (TRUE on success and FALSE on failure). So we're forced
    * to convert error codes. The next logical error codes to return are 4 and
    * 5 when cckInit() or protocol() returns false.
    */
   #define CCK_INIT_FAILED 4
   #define PROTOCOL_FAILED 5

   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );
   assert( ptpSess->ptpClock == NULL );

   cckInitSucceeded = cckInit();

   /*
    * Convert cckInit's error to an unused error code.
    */
   if( !cckInitSucceeded ) {
      return CCK_INIT_FAILED;
   }

   /*
    * TODO: move this global into the PtpClock struct.
    */
   startupInProgress = TRUE;

   if( !(ptpSess->ptpClock = ptpdStartup( &ptpdStartupReturnVal, ptpSess->rtOpts )) ) {
      cckShutdown();
      return ptpdStartupReturnVal;
   }

   startupInProgress = FALSE;
   ptpSess->rtOpts->run = TRUE;

   G_ptpClock = ptpSess->ptpClock;
   G_rtOpts = ptpSess->rtOpts;

   /*
    * Re-apply saved settings made during a previous run only if a valid
    * settings object was passed in.
    */
   if( (settingsObj != NULL) && (*settingsObj != NULL) ) {
      _restoreManagedSettingsOnPtpClock( *settingsObj,
                                         ptpSess->ptpClock,
                                         ptpSess->rtOpts );
   }

   /*
    * protocol() does not return until the "rtOpts->run" flag is set FALSE
    */
   ptpSess->protocolIsRunning = TRUE;
   protocolSucceeded = protocol( ptpSess->rtOpts, ptpSess->ptpClock );
   ptpSess->protocolIsRunning = FALSE;

   /*
    * Only save the managed runtime settings if an object was passed in.
    * Re-use an existing object if one was allocated in the past.
    */
   if( settingsObj != NULL ) {
      if( *settingsObj == NULL ) {
         *settingsObj = \
            (PtpManagedSettings*) malloc( sizeof( PtpManagedSettings ) );
         if( *settingsObj == NULL ) { return ENOMEM; }
      }

      _saveManagedSettingsFromPtpClock( *settingsObj,
                                        ptpSess->ptpClock );
   }

   /*
    * This frees ptpSess->ptpClock and sets G_ptpClock to NULL
    */
   ptpdShutdown( ptpSess->ptpClock, ptpSess->rtOpts );
   ptpSess->ptpClock = NULL;

   cckShutdown();

   /*
    * Convert protocol's error to an unused error code.
    */
   if ( !protocolSucceeded ) { return PROTOCOL_FAILED; }

   return 0;
}


int
ptp_deleteManagedSettingsObject( PtpManagedSettings* settingsObj )
{
   if( settingsObj != NULL ) {
      free( settingsObj );
      return 0;
   }

   return EINVAL;
}


int
ptp_stop( PtpSession* ptpSess )
{
   int timeoutCount = 0;

   if( ptpSess == NULL ) { return EINVAL; }
   if( ptpSess->rtOpts == NULL ) { return EINVAL; }

   /*
    * This function only applies when the thread-based timer is used and when
    * the protocol is currently running.  When the signal-based timer is used,
    * ptpd is stopped by sending a signal.
    */
   if( !ptpSess->rtOpts->useTimerThread ) { return EOPNOTSUPP; }
   if( !ptpSess->rtOpts->run ) { return EAGAIN; }

   ptpSess->rtOpts->run = FALSE;

   while( ptpSess->protocolIsRunning && (timeoutCount < STOP_TIMEOUT_MSEC) ) {
      usleep( USEC_PER_MSEC ); /* 1ms */
      timeoutCount++;
   }

   if( timeoutCount >= STOP_TIMEOUT_MSEC ) {
      return EBUSY;
   }

   /*
    * Stop the timer thread started in timer.c
    */
   return stopThreadedTimer();
}


int
ptp_enableTimerThread( PtpSession* ptpSess )
{
   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );

   ptpSess->rtOpts->useTimerThread = TRUE;

   return 0;
}


int
ptp_setClockAdjustFunc( PtpSession* ptpSess,
                        ClockAdjustFunc clientClockAdjustFunc, void* clientData )
{
   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );

   if( ptpSess->protocolIsRunning ) {
      return EBUSY;
   }
   ptpSess->rtOpts->clientClockAdjustFunc = clientClockAdjustFunc;
   ptpSess->rtOpts->clientClockAdjustData = clientData;

   return 0;
}


int
ptp_setClockQuality( PtpSession* ptpSess,
                     ClockQuality* newClockQuality )
{
   /*
    * TODO: implement this.
    */
   return 0;
}


int
ptp_getGrandmasterClockQuality( PtpSession* ptpSess,
                                ClockQuality* gmClockQuality )
{
   assert( ptpSess != NULL );

   if( ptpSess->ptpClock != NULL ) {
      gmClockQuality->clockClass = ptpSess->ptpClock->grandmasterClockQuality.clockClass;
      gmClockQuality->clockAccuracy = ptpSess->ptpClock->grandmasterClockQuality.clockAccuracy;
      gmClockQuality->offsetScaledLogVariance = ptpSess->ptpClock->grandmasterClockQuality.offsetScaledLogVariance;

      return 0;
   }

   return ENOPROTOOPT;
}


int
ptp_getPortState( PtpSession* ptpSess,
                  Enumeration8* portState )
{
   assert( ptpSess != NULL );

   if( ptpSess->ptpClock != NULL ) {
      *portState = ptpSess->ptpClock->portState;

      return 0;
   }

   return ENOPROTOOPT;
}


int
ptp_getInterfaceName( PtpSession* ptpSess,
                      char** interface )
{
   assert( ptpSess != NULL );

   if( ptpSess->rtOpts != NULL && ptpSess->rtOpts->ifaceName != NULL ) {
      *interface = ptpSess->rtOpts->ifaceName;

      return 0;
   }

   return ENODATA;
}

void
ptp_logError( const char* msg )
{
   ERROR("%s %s", USER_DESCRIPTION, msg);
}


void
ptp_logNotify( const char* msg )
{
   NOTIFY( msg );
}
