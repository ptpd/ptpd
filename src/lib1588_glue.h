/*-
 * Copyright (c) 2016      Wojciech Owczarek
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
 * @file   lib1588_glue.h
 * @date   Thu Nov 24 23:19:00 2016
 *
 * @brief  Functions (extern) required for lib1588 to operate
 *
 *
 */

#ifndef _LIB1588_CCK_GLUE_H
#define _LIB1588_CCK_GLUE_H

#include <stdio.h>
#include <stdbool.h>
#include "ptpd.h"

/* timer support - lib1588 externs */
void ptpTimerStop(PtpTimer *timer);
void ptpTimerStart(PtpTimer *timer, const double interval);
void ptpTimerLog2Start (PtpTimer *timer, const int power2);
bool ptpTimerExpired(PtpTimer *timer);

/* address management - lib1588 externs */
int		ptpAddrStrLen(void *input);
int		ptpAddrDataSize(void *input);
char*		ptpAddrToString(char *buf, const int len, void *input);
uint32_t	ptpAddrHash(void *addr, const int modulo);
int		ptpAddrCompare(void *a, void *b);
bool		ptpAddrIsEmpty(void *input);
bool		ptpAddrIsMulticast(void *input);
void		ptpAddrClear(void *input);
void		ptpAddrCopy(void *dst, void *src);
void*		ptpAddrDup(void *src);
void		ptpAddrFree(void **input);
void*		ptpAddrGetData(void *input);
int		ptpAddrSize();

/** lib1588 transport-related externs **/

/* run any network refresh code when PTP port is initialising */
void ptpNetworkRefresh(PtpClock *ptpClock);
/* retrieve packet counts from transport provider */
uint32_t getPtpPortRxPackets(PtpClock *ptpClock);
uint32_t getPtpPortPackets(PtpClock *ptpClock);
/* send data with optional transmit timestamp */
ssize_t sendPtpData(PtpClock *ptpClock, bool event, char *data, ssize_t len, void *dst, TimeInternal *timestamp);


/* lib1588 clock externs */
void getSystemTime(TimeInternal *);
void getPtpClockTime(TimeInternal *, PtpClock *);

/* lib1588 protocol-related externs */

/* set PTP clock identity (here provided by the underlying transport */
void setPtpClockIdentity(PtpClock *ptpClock);


#endif /* _LIB1588_CCK_GLUE_H */

