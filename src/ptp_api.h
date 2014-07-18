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

#ifndef PTPAPI_H_
#define PTPAPI_H_

/*
 * Include guard for ptpd types - if ptpd.h was included before, do not
 * redeclare these types.
 *
 * TODO: move these to a shared header.
 */
#ifndef PTPD_H_

typedef int32_t Integer32;
typedef uint8_t UInteger8;
typedef uint16_t UInteger16;
typedef unsigned char Enumeration8;

enum {
   PTP_INITIALIZING=1,  PTP_FAULTY,  PTP_DISABLED,
   PTP_LISTENING,  PTP_PRE_MASTER,  PTP_MASTER,
   PTP_PASSIVE,  PTP_UNCALIBRATED,  PTP_SLAVE
};

typedef struct {
   UInteger8 clockClass;
   Enumeration8 clockAccuracy;
   UInteger16 offsetScaledLogVariance;
} ClockQuality;

typedef void (*ClockAdjustFunc)( Integer32 oldSeconds, Integer32 oldNanoseconds,
                                 Integer32 newSeconds, Integer32 newNanoseconds,
                                 void* clientData );

#endif /* PTPD_H_ */


int ptp_initializeSession( void** session );
int ptp_deleteSession( void* session );

int ptp_setOptsFromCommandLine( void* session,
                                int argc, char** argv,
                                int* shouldRun );

int ptp_checkConfigOnly( void* session );

int ptp_run( void* session );
int ptp_stop( void* session );

int ptp_enableTimerThread( void* session );
int ptp_setClockAdjustFunc( void* session,
                            ClockAdjustFunc clientClockAdjustFunc, void* clientData );

int ptp_setClockQuality( void* session,
                         ClockQuality* newClockQuality );

int ptp_getGrandmasterClockQuality( void* session,
                                    ClockQuality* gmClockQuality );

int ptp_getPortState( void* session,
                      Enumeration8* portState );

void ptp_logError( const char* msg );
void ptp_logNotify( const char* msg );


#endif /* PTPAPI_H_ */
