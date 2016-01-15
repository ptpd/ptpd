/* Copyright (c) 2015 Wojciech Owczarek,
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
 * @file   clockdriver_linuxphc.h
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  structure definitions for the Linux PHC clock driver
 *
 */

#ifndef PTPD_CLOCKDRIVER_LINUXPHC_H_
#define PTPD_CLOCKDRIVER_LINUXPHC_H_

#include "clockdriver.h"

#define OSCLOCK_OFFSET_SAMPLES 15

typedef struct {
    int clockFd;
    int phcIndex;
    clockid_t clockId;
} ClockDriverData_linuxphc;

typedef struct {
    char networkDevice[IFACE_NAME_LENGTH];
    char characterDevice[PATH_MAX];
} ClockDriverConfig_linuxphc;

void _setupClockDriver_linuxphc(ClockDriver* clockDriver);


#endif /* PTPD_CLOCKDRIVER_LINUXPHC_H_ */
