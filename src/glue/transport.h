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
 * @file   cck_configure.h
 * @date   Thu Nov 24 23:19:00 2016
 *
 * @brief  Wrappers and callbacks binding libCCK with PTPd code
 *
 *
 */

#ifndef _PTPD_CCK_GLUE_H
#define _PTPD_CCK_GLUE_H

#include "../ptpd.h"

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/fd_set.h>
#include <libcck/timer.h>
#include <libcck/clockdriver.h>
#include <libcck/ttransport.h>
#include <libcck/libcck.h>

#include "../globalconfig.h"

typedef struct {
	CckFdSet *fdSet;
} PtpClockUserData;

void getSystemTime(TimeInternal *);
void getPtpClockTime(TimeInternal *, PtpClock *);

/* callbacks */
void ptpNetworkRefresh(PtpClock *ptpClock);
void myNetworkMonitor(PtpClock *ptpClock, const int interval);
void getPtpNetworkInfo(PtpClock *ptpClock);

void myPtpClockPreInit(PtpClock *ptpClock);
void myPtpClockPostShutdown(PtpClock *ptpClock);
void myPtpClockSetClockIdentity(PtpClock *ptpClock);
void myPtpClockStepNotify(void *owner);
void myPtpClockStateChange(PtpClock *ptpClock, const uint8_t from, const uint8_t to);
void myPtpClockLocked(void *owner, bool locked);



void myPtpClockUpdate(void *owner);

int ptpPreSend (void *transport, void *owner, char *data, const size_t len, bool isMulticast);
int ptpMatchData (void *owner, void *transport, char *a, const size_t alen, char *b, const size_t blen);
int ptpIsRegularData (void *transport, void *owner, char *data, const size_t len, bool isMulticast);

/* address management */
int		myAddrStrLen(void *input);
int		myAddrDataSize(void *input);
char*		myAddrToString(char *buf, const int len, void *input);
uint32_t	myAddrHash(void *addr, const int modulo);
int		myAddrCompare(void *a, void *b);
bool		myAddrIsEmpty(void *input);
bool		myAddrIsMulticast(void *input);
void		myAddrClear(void *input);
void		myAddrCopy(void *dst, void *src);
void*		myAddrDup(void *src);
void		myAddrFree(void **input);
void*		myAddrGetData(void *input);
int		myAddrSize();

/* timer support */
void ptpTimerStop(PtpTimer *timer);
void ptpTimerStart(PtpTimer *timer, const double interval);
void ptpTimerLogStart(PtpTimer *timer, const int power2);
bool ptpTimerExpired(PtpTimer *timer);
bool setupPtpTimers(PtpClock *ptpClock, CckFdSet *fdSet);
void shutdownPtpTimers(struct PtpClock *ptpClock);

/* data extraction - required for management message and misconfigured domain alarm check */
uint32_t myPtpClockGetRxPackets(PtpClock *ptpClock);
uint32_t myPtpClockGetTxPackets(PtpClock *ptpClock);

/* receive and process data when available */
void ptpDataCallback(void *fd, void *transport);
/* send data with optional transmit timestamp */
ssize_t sendPtpData(PtpClock *ptpClock, bool event, char *data, ssize_t len, void *dst, TimeInternal *timestamp);

/* component configuration */

/* configure ACLs */
void configureAcls(PtpClock *ptpClock);
/* push clock driver configuration to a single clock driver */
bool configureClockDriver(ClockDriver *driver, const void *configData);
/* configure unicast destinations */
bool configureUnicast(PtpClock *ptpClock);
/* set PTP groups / destination addresses */
void configurePtpAddrs(PtpClock *ptpClock, const int family);
/* get event transport configuration flags */
int getEventTransportFlags(const GlobalConfig *global);
/* get general transport configuration flags */
int getGeneralTransportFlags(const GlobalConfig *global);
/* generate transport configuration */
TTransportConfig *getEventTransportConfig(const int type, const PtpClock *ptpClock);
TTransportConfig *getGeneralTransportConfig(const int type, const PtpClock *ptpClock);

bool netInit(PtpClock *ptpClock, CckFdSet *fdSet);
void netShutdown(PtpClock *ptpClock);


#endif /* _PTPD_CCK_GLUE_H */

