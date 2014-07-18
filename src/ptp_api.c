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

#include "ptpd.h"
#include "ptp_api.h"

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


int
ptp_initializeSession( void** session )
{
   /* FIXME: assert session not NULL */
   PtpSession* ptpSess = (PtpSession*) malloc( sizeof( PtpSession ) );
   /* FIXME: check malloc */
   ptpSess->rtOpts = (RunTimeOpts*) malloc( sizeof( RunTimeOpts ) );
   /* FIXME: check malloc */
   ptpSess->ptpClock = NULL;

   loadDefaultSettings( ptpSess->rtOpts );

   ptpSess->rtOpts->candidateConfig = dictionary_new(0);
   ptpSess->rtOpts->cliConfig = dictionary_new(0);

   *session = ptpSess;
   return 0;
}


int
ptp_deleteSession( void* session )
{
   /* FIXME: assert session not NULL */
   /* FIXME: assert rtOpts not NULL */
   PtpSession* ptpSess = (PtpSession*) session;

   /* FIXME: stop clock, etc. */
   dictionary_del( &(ptpSess->rtOpts->currentConfig) );
   dictionary_del( &(ptpSess->rtOpts->candidateConfig) );
   dictionary_del( &(ptpSess->rtOpts->cliConfig) );

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
   /* FIXME: assert session not NULL */
   /* FIXME: assert rtOpts not NULL */
   PtpSession* ptpSess = (PtpSession*) session;
   Integer16 retCode;

   /*
    * FIXME: setRTOptsFromCommandLine() calls functions that require G_rtOpts to
    * be set - that needs to be fixed.
    */
   G_rtOpts = ptpSess->rtOpts;

   *shouldRun =
      (setRTOptsFromCommandLine( argc, argv, &retCode, ptpSess->rtOpts ) == TRUE) ? 1 : 0;

   return (int) retCode;
}


int
ptp_checkConfigOnly( void* session )
{
   /* FIXME: assert session not NULL */
   /* FIXME: assert rtOpts not NULL */
   PtpSession* ptpSess = (PtpSession*) session;

   return (ptpSess->rtOpts->checkConfigOnly == TRUE);
}


int
ptp_run( void* session )
{
   /* FIXME: assert session not NULL */
   /* FIXME: assert rtOpts not NULL */
   PtpSession* ptpSess = (PtpSession*) session;

   /* FIXME: assert ptpClock *is* NULL */

   Integer16 returnVal = 0;

   cckInit();

   /*
    * TODO: move this global into the PtpClock struct.
    */
   startupInProgress = TRUE;

   if( !(ptpSess->ptpClock = ptpdStartup( &returnVal, ptpSess->rtOpts )) ) {
      cckShutdown();
      return returnVal;
   }

   startupInProgress = FALSE;

   /* global variable for message(), please see comment on top of this file */
   G_ptpClock = ptpSess->ptpClock;
   G_rtOpts = ptpSess->rtOpts;

   /* do the protocol engine */
   protocol( ptpSess->rtOpts, ptpSess->ptpClock );
   /* forever loop.. */

   /*
    * This frees ptpSess->ptpClock
    */
   ptpdShutdown( ptpSess->ptpClock, ptpSess->rtOpts );
   cckShutdown();

   return (int) returnVal;
}


int
ptp_stop( void* session )
{
   /* FIXME: assert session not NULL */
   /* FIXME: assert rtOpts not NULL */
   PtpSession* ptpSess = (PtpSession*) session;

   /*
    * This function only applies when the thread-based timer is used.  When the
    * signal-based timer is used, ptpd is stopped by sending a signal.
    */
   if( ptpSess->rtOpts->useTimerThread ) {
      ptpSess->rtOpts->run = FALSE;
      /*
       * TODO: join the thread
       */
      return 0;
   }

   return 1;
}


int
ptp_enableTimerThread( void* session )
{
   /* FIXME: assert session not NULL */
   /* FIXME: assert rtOpts not NULL */
   PtpSession* ptpSess = (PtpSession*) session;

   ptpSess->rtOpts->useTimerThread = TRUE;
   ptpSess->rtOpts->run = TRUE;

   return 0;
}


int
ptp_setClockAdjustFunc( void* session,
                        ClockAdjustFunc clientClockAdjustFunc, void* clientData )
{
   /* FIXME: assert session not NULL */
   /* FIXME: assert rtOpts not NULL */
   PtpSession* ptpSess = (PtpSession*) session;

   /*
    * Only allow the function to be changed once after rtOpts init to prevent
    * this from being changed (possibly in a separate thread) while ptpd is
    * running.
    */
   if( ptpSess->rtOpts->clientClockAdjustFunc != NULL ) {
      return 1;
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
   /* FIXME: assert session not NULL */
   PtpSession* ptpSess = (PtpSession*) session;

   if( ptpSess->ptpClock != NULL ) {
      gmClockQuality->clockClass = ptpSess->ptpClock->grandmasterClockQuality.clockClass;
      gmClockQuality->clockAccuracy = ptpSess->ptpClock->grandmasterClockQuality.clockAccuracy;
      gmClockQuality->offsetScaledLogVariance = ptpSess->ptpClock->grandmasterClockQuality.offsetScaledLogVariance;

      return 0;
   }
   return 1;
}


int
ptp_getPortState( void* session,
                  Enumeration8* portState )
{
   /* FIXME: assert session not NULL */
   PtpSession* ptpSess = (PtpSession*) session;

   if( ptpSess->ptpClock != NULL ) {
      *portState = ptpSess->ptpClock->portState;

      return 0;
   }
   return 1;
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
