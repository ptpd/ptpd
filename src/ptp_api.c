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
 * A PtpSession consists of a PtpClock and the corresponding RunTimeOpts.
 *
 * NOTE: the need for globals for instances of both of these structs (as well as
 * startupInProgress) still limits this to 1 PtpSession per process - this will
 * hopefully change soon.
*/
typedef struct {
   PtpClock* ptpClock;
   RunTimeOpts* rtOpts;
   Boolean protocolIsRunning;
} PtpSession;


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


int
ptp_initializeSession( void** session )
{
   PtpSession* ptpSess = NULL;

   assert( session != NULL );

   ptpSess = (PtpSession*) malloc( sizeof( PtpSession ) );
   if( ptpSess == NULL ) {
      return ENOMEM;
   }

   ptpSess->rtOpts = (RunTimeOpts*) malloc( sizeof( RunTimeOpts ) );
   if( ptpSess->rtOpts == NULL ) {
      free( ptpSess );
      return ENOMEM;
   }

   ptpSess->ptpClock = NULL;
   ptpSess->protocolIsRunning = FALSE;

   loadDefaultSettings( ptpSess->rtOpts );

   _createConfigDictionaries( ptpSess );

   *session = ptpSess;

   return 0;
}


int
ptp_deleteSession( void* session )
{
   PtpSession* ptpSess = (PtpSession*) session;

   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );

   /* TODO: stop clock, etc. */
   _deleteConfigDictionaries( ptpSess );

   free( ptpSess->rtOpts ); ptpSess->rtOpts = NULL;
   /*
    * ptpSess->ptpClock should have already been free'd and set to NULL by
    * ptpdShutdown().
    */
   free( ptpSess );

   return 0;
}


int
ptp_setOptsFromCommandLine( void* session,
                            int argc, char** argv,
                            int* shouldRun )
{
   PtpSession* ptpSess = (PtpSession*) session;
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
ptp_testNetworkConfig( void* session )
{
   PtpSession* ptpSess = (PtpSession*) session;

   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );

   if( testNetworkConfig( ptpSess->rtOpts ) ) {
      return 0;
   } else {
      return -1;
   }
}


int
ptp_checkConfigOnly( void* session )
{
   PtpSession* ptpSess = (PtpSession*) session;

   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );

   return (ptpSess->rtOpts->checkConfigOnly == TRUE);
}


int
ptp_dumpConfig( void* session, FILE* out )
{
   PtpSession* ptpSess = (PtpSession*) session;

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
ptp_run( void* session )
{
   PtpSession* ptpSess = (PtpSession*) session;
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
    * protocol() does not return until the "rtOpts->run" flag is set FALSE
    */
   ptpSess->protocolIsRunning = TRUE;
   protocolSucceeded = protocol( ptpSess->rtOpts, ptpSess->ptpClock );
   ptpSess->protocolIsRunning = FALSE;

   /*
    * This frees ptpSess->ptpClock
    */
   ptpdShutdown( ptpSess->ptpClock, ptpSess->rtOpts );
   cckShutdown();

   /*
    * Convert protocol's error to an unused error code.
    */
   if ( !protocolSucceeded ) {
      return PROTOCOL_FAILED;
   }

   return 0;
}


int
ptp_stop( void* session )
{
   PtpSession* ptpSess = (PtpSession*) session;
   int timeoutCount = 0;

   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );

   /*
    * This function only applies when the thread-based timer is used.  When the
    * signal-based timer is used, ptpd is stopped by sending a signal.
    */
   if( ptpSess->rtOpts->useTimerThread ) {

      ptpSess->rtOpts->run = FALSE;

      while( ptpSess->protocolIsRunning && (timeoutCount < STOP_TIMEOUT_MSEC) ) {
         usleep( USEC_PER_MSEC ); /* 1ms */
         timeoutCount++;
      }

      /*
       * Stop the timer thread started in timer.c
       */
      stopThreadedTimer();

      return (timeoutCount < STOP_TIMEOUT_MSEC) ? 0 : EBUSY;
   }

   return EOPNOTSUPP;
}


int
ptp_enableTimerThread( void* session )
{
   PtpSession* ptpSess = (PtpSession*) session;

   assert( ptpSess != NULL );
   assert( ptpSess->rtOpts != NULL );

   ptpSess->rtOpts->useTimerThread = TRUE;

   return 0;
}


int
ptp_setClockAdjustFunc( void* session,
                        ClockAdjustFunc clientClockAdjustFunc, void* clientData )
{
   PtpSession* ptpSess = (PtpSession*) session;

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
ptp_setClockQuality( void* session,
                     ClockQuality* newClockQuality )
{
   /*
    * TODO: implement this.
    */
   return 0;
}


int
ptp_getGrandmasterClockQuality( void* session,
                                ClockQuality* gmClockQuality )
{
   PtpSession* ptpSess = (PtpSession*) session;

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
ptp_getPortState( void* session,
                  Enumeration8* portState )
{
   PtpSession* ptpSess = (PtpSession*) session;

   assert( ptpSess != NULL );

   if( ptpSess->ptpClock != NULL ) {
      *portState = ptpSess->ptpClock->portState;

      return 0;
   }

   return ENOPROTOOPT;
}


int
ptp_getInterfaceName( void* session,
                      char** interface )
{
   PtpSession* ptpSess = (PtpSession*) session;

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
