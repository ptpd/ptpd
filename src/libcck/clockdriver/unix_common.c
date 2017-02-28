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
 * @file   unix_common.c
 * @date   Sat Jan 9 16:14:10 2017
 *
 * @brief  Common Unix / Linux clock driver utilities
 *
 */

#include <config.h>
#include "unix_common.h"

#include <string.h>

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>

#include <libcck/clockdriver.h>

#include <sys/time.h>
#include <time.h>

#ifdef HAVE_SYS_TIMEX_H
#include <sys/timex.h>
#endif /* HAVE_SYS_TIMEX_H */

#ifndef HAVE_ADJTIMEX
#ifdef HAVE_NTP_ADJTIME
#define adjtimex ntp_adjtime
#endif /* HAVE_NTP_ADJTIME */
#endif /* HAVE_ADJTIMEX */

#ifdef HAVE_SYS_TIMEX_H

int
getTimexFlags(void)
{

	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = 0;

	ret = adjtimex(&tmx);

	if (ret < 0) {
		CCK_PERROR("getTimexFlags(): Could not read adjtimex flags");
		return -1;

	}

	return( tmx.status );

}

bool
checkTimexFlags(const int flags)
{

	int tflags = getTimexFlags();

	if (tflags == -1) {
		return false;
	}
	return ((tflags & flags) == flags);

}

bool
setTimexFlags(const int flags, const bool quiet)
{

	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_STATUS;

	tmx.status = getTimexFlags();
	if(tmx.status == -1) {
		CCK_PERROR("setTimexFlags(): Could not read current adjtimex flags");
		return false;
	};
	/* unset all read-only flags */
	tmx.status &= ~STA_RONLY;
	tmx.status |= flags;

	ret = adjtimex(&tmx);

	if (ret < 0) {
		CCK_PERROR("setTimexFlags(): Could not set adjtimex flags");
		return false;
	}

	if(!quiet && ret > 2) {
		switch (ret) {
		case TIME_OOP:
			CCK_WARNING("setTimexFlags(): Leap second already in progress\n");
			break;
		case TIME_WAIT:
			CCK_WARNING("setTimexFlags(): Leap second already occurred\n");
			break;
#if !defined(TIME_BAD)
		case TIME_ERROR:
#else
		case TIME_BAD:
#endif /* TIME_BAD */
		default:
			CCK_DBGV("setTimexFlags(): adjtimex() returned TIME_BAD\n");
			break;
		}
	}

    return true;

}

bool
clearTimexFlags(const int flags, const bool quiet)
{

	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_STATUS;

	tmx.status = getTimexFlags();
	if(tmx.status == -1) {
		CCK_PERROR("clearTimexFlags(): Could not read current adjtimex flags");
		return false;
	};
	/* unset all read-only flags */
	tmx.status &= ~STA_RONLY;
	tmx.status &= ~flags;

	ret = adjtimex(&tmx);

	if (ret < 0) {
		CCK_PERROR("clearTimexFlags(): Could not set adjtimex flags");
		return false;
	}

	if(!quiet && ret > 2) {
		switch (ret) {
		case TIME_OOP:
			CCK_WARNING("clearTimexFlags(): Leap second already in progress\n");
			break;
		case TIME_WAIT:
			CCK_WARNING("clearTimexFlags(): Leap second already occurred\n");
			break;
#if !defined(TIME_BAD)
		case TIME_ERROR:
#else
		case TIME_BAD:
#endif /* TIME_BAD */
		default:
			CCK_DBGV("clearTimexFlags(): adjtimex() returned TIME_BAD\n");
			break;
		}
	}

    return true;

}

#if defined(MOD_TAI) &&  NTP_API == 4
bool
setKernelUtcOffset(const int offset)
{

	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_TAI;
	tmx.constant = offset;

	CCK_DBG("setKernelUtcOffset(): Kernel NTP API supports TAI offset. "
	     "Setting TAI offset to %d\n", offset);

	ret = adjtimex(&tmx);

	if (ret < 0) {
		CCK_PERROR("setKernelUtcOffset(): Could not set kernel TAI offset");
		return false;
	}

	return true;

}

int
getKernelUtcOffset(void)
{

	int ret;

#if defined(HAVE_NTP_GETTIME)
	struct ntptimeval ntpv;
	memset(&ntpv, 0, sizeof(ntpv));
	ret = ntp_gettime(&ntpv);
#else
	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = 0;
	ret = adjtimex(&tmx);
#endif /* HAVE_NTP_GETTIME */

	if (ret < 0) {
		return -1;
	}
#if !defined(HAVE_NTP_GETTIME) && defined(HAVE_STRUCT_TIMEX_TAI)
	return tmx.tai;
#elif defined(HAVE_NTP_GETTIME) && defined(HAVE_STRUCT_NTPTIMEVAL_TAI)
	return (ntpv.tai);
#endif /* HAVE_STRUCT_TIMEX_TAI */
	return 0;
}

#endif /* MOD_TAI */

int
setOffsetInfo(CckTimestamp *offset)
{

	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = MOD_MAXERROR | MOD_ESTERROR;

	tmx.maxerror = offset->seconds * 1E6 +
			offset->nanoseconds / 1000;
	tmx.esterror = tmx.maxerror;

	return adjtimex(&tmx);
}

#endif /* HAVE_SYS_TIMEX_H */
