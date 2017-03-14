/* Copyright (c) 2016-2017 Wojciech Owczarek,
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
 * @file   clockdriver_unix.h
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  structure definitions for the Unix OS clock driver
 *
 */

#ifndef CCK_CLOCKDRIVER_UNIX_H_
#define CCK_CLOCKDRIVER_UNIX_H_

#include <libcck/clockdriver.h>

#define UNIX_MAX_FREQ 500000
/* to keep tick adjustment to 0.1s / s  - 2 * max_freq */
#define UNIX_TICKADJ_MULT ( (1E9 / 10) / UNIX_MAX_FREQ - 2 )

bool _setupClockDriver_unix(ClockDriver* clockDriver);

typedef struct {

} ClockDriverData_unix;

typedef struct {
    bool setRtc;
} ClockDriverConfig_unix;


#endif /* CCK_CLOCKDRIVER_UNIX_H_ */
