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

/* PTP callbacks and helpers */
void ptpPortPostInit(PtpClock *ptpClock);
void ptpPortPreShutdown(PtpClock *ptpClock);
void setPtpClockIdentity(PtpClock *ptpClock);
void ptpPortStepNotify(void *clockdriver, void *owner);
void ptpPortFrequencyJump(void *clockdriver, void *owner);
bool ptpPortStateChange(PtpClock *ptpClock, const uint8_t from, const uint8_t to);
void ptpPortLocked(void *clockdriver, void *owner, bool locked);
void clockStateChange(void *clockdriver, void *owner, const int oldState, const int newState);

/* timer support */
bool setupPtpTimers(PtpClock *ptpClock, CckFdSet *fdSet);
void shutdownPtpTimers(struct PtpClock *ptpClock);

/* component configuration */

/* push clock driver configuration to a single clock driver */
bool configureClockDriver(ClockDriver *driver, const void *configData);

/* prepare clock drivers controlled by the given PTP clock */
bool prepareClockDrivers(PtpClock *ptpClock, const GlobalConfig *global);

/* apply common libcck configuration */
bool configureLibCck(const GlobalConfig *global);

#endif /* _PTPD_CCK_GLUE_H */

