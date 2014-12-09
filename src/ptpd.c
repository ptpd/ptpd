/*-
 * Copyright (c) 2014      Rick Ratzel,
 *                         Eric Satterness,
 *                         National Instruments.
 * Copyright (c) 2012-2013 Wojciech Owczarek,
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments.
 * Copyright (c) 2009-2010 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen
 *
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
 *
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

/**
 * @file   ptpd.c
 * @date   Wed Jun 23 10:13:38 2010
 *
 * @brief  The main() function for the PTP daemon
 *
 * This file contains very little code, as should be obvious,
 * and only serves to tie together the rest of the daemon.
 * All of the default options are set here, but command line
 * arguments and configuration file is processed in the
 * ptpdStartup() routine called
 * below.
 */

#include <stdint.h>

#include "ptp_api.h"


int main( int argc, char **argv ) {

   PtpSession* ptpSess;
   int shouldRun = 0;
   int retVal = 0;

   ptp_initializeSession( &ptpSess );

   /*
    * TODO: Refactor processing the command line settings to exclude
    * applying side effects of those processed settings inside the same
    * function. The side effects of the settings can be applied separately.
    * This would simplify the logic required to determine whether the daemon
    * should run, exit early, or provide information to the user.
    */
   retVal = ptp_setOptsFromCommandLine( ptpSess, argc, argv, &shouldRun );

   /*
    * The only time we want to continue running ptp is if shouldRun is true and
    * ret is 0 (meaning there were no errors while setting the options). If that
    * isn't the case, we should stop.
    */
   if( !shouldRun || ( retVal != 0 ) ) {
      if( ( retVal != 0 ) && !ptp_checkConfigOnly( ptpSess ) ) {
         ptp_logError( "startup failed\n" );
      }
      return retVal;
   }

   retVal = ptp_testNetworkConfig( ptpSess );

   if( retVal != 0 ) {
      ptp_logError( "startup failed due to a network config error\n" );
      return retVal;
   }

   retVal = ptp_run( ptpSess );

   if( retVal != 0 ) {
      ptp_logError( "Shutdown due to an error\n" );
   }

   ptp_deleteSession( ptpSess );

   return retVal;
}
