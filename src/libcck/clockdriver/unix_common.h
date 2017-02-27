/* Copyright (c) 2017 Wojciech Owczarek,
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
 * @file   unix_common.h
 * @date   Sat Jan 9 16:14:10 2017
 *
 * @brief  Declaration for common Unix / Linux clock driver utilities
 *
 */

#ifndef CCK_CLOCKDRIVER_UNIX_COMMON_H_
#define CCK_CLOCKDRIVER_UNIX_COMMON_H_

#include <config.h>

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>

#include <libcck/clockdriver.h>

/* Helper function to manage ntpadjtime / adjtimex flags and UTC/TAI offset */
int	getTimexFlags(void);
bool	checkTimexFlags(const int flags);
bool	setTimexFlags(const int flags, const bool quiet);
bool	clearTimexFlags(const int flags, const bool quiet);

bool	setKernelUtcOffset(const int offset);
int	getKernelUtcOffset(void);

/* MOD_ESTERROR + MOD_MAXERROR */
int	setOffsetInfo(CckTimestamp *offset);

#endif /* CCK_CLOCKDRIVER_UNIX_COMMON_H_ */
