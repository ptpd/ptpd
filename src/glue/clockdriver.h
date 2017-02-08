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
 * @file   clockdriver.h
 * @date   Thu Nov 24 23:19:00 2016
 *
 * @brief  Clock driver wrappers between PTPd / lib1588 and libCCK
 *
 *
 */

#ifndef _PTPD_CCK_GLUE_CLOCKDRIVER_H
#define _PTPD_CCK_GLUE_CLOCKDRIVER_H

#include "../ptpd.h"
#include "../globalconfig.h"

#include <libcck/clockdriver.h>

void getSystemTime(TimeInternal *);
void getPtpClockTime(TimeInternal *, PtpClock *);
void myPtpClockUpdate(void *owner);

/* push clock driver configuration to a single clock driver */
bool configureClockDriver(ClockDriver *driver, const void *configData);
/* prepare all clock drivers */
bool prepareClockDrivers(PtpClock *ptpClock);


#endif /* _PTPD_CCK_GLUE_CLOCKDRIVER_H */

