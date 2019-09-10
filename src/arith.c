/*-
 * Copyright (c) 2012-2015 Wojciech Owczarek,
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
 * @file   arith.c
 * @date   Tue Jul 20 16:12:51 2010
 *
 * @brief  Time format conversion routines and additional math functions.
 *
 *
 */

#include "ptpd.h"

void
internalTime_to_integer64(TimeInternal internal, Integer64 *bigint)
{
	int64_t scaledNanoseconds;

	scaledNanoseconds = internal.seconds;
	scaledNanoseconds *= 1000000000;
	scaledNanoseconds += internal.nanoseconds;
	scaledNanoseconds <<= 16;

	bigint->msb = (scaledNanoseconds >> 32) & 0x00000000ffffffff;
	bigint->lsb = scaledNanoseconds & 0x00000000ffffffff;
}

void
integer64_to_internalTime(Integer64 bigint, TimeInternal * internal)
{
	int sign;
	int64_t scaledNanoseconds;

	scaledNanoseconds = bigint.msb;
	scaledNanoseconds <<=32;
	scaledNanoseconds += bigint.lsb;

	/*determine sign of result big integer number*/

	if (scaledNanoseconds < 0) {
		scaledNanoseconds = -scaledNanoseconds;
		sign = -1;
	} else {
		sign = 1;
	}

	/*fractional nanoseconds are excluded (see 5.3.2)*/
	scaledNanoseconds >>= 16;
	internal->seconds = sign * (scaledNanoseconds / 1000000000);
	internal->nanoseconds = sign * (scaledNanoseconds % 1000000000);
}


void
fromInternalTime(const TimeInternal * internal, Timestamp * external)
{
	/*
	 * fromInternalTime is only used to convert time given by the system
	 * to a timestamp.  As a consequence, no negative value can normally
	 * be found in (internal)
	 *
	 * Note that offsets are also represented with TimeInternal structure,
	 * and can be negative, but offset are never convert into Timestamp
	 * so there is no problem here.
	 */

	if ((internal->seconds & ~INT_MAX) ||
	    (internal->nanoseconds & ~INT_MAX)) {
		DBG("Negative value canno't be converted into timestamp \n");
		return;
	} else {
		external->secondsField.lsb = internal->seconds;
		external->nanosecondsField = internal->nanoseconds;
		external->secondsField.msb = 0;
	}
}

void
toInternalTime(TimeInternal * internal, const Timestamp * external)
{
	/* Program will not run after 2038... */
	if (external->secondsField.lsb < INT_MAX) {
		internal->seconds = external->secondsField.lsb;
		internal->nanoseconds = external->nanosecondsField;
	} else {
		DBG("Clock servo canno't be executed : "
		    "seconds field is higher than signed integer (32bits) \n");
		return;
	}
}

void
ts_to_InternalTime(const struct timespec *a,  TimeInternal * b)
{
	b->seconds = a->tv_sec;
	b->nanoseconds = a->tv_nsec;
}

void
tv_to_InternalTime(const struct timeval *a,  TimeInternal * b)
{
	b->seconds = a->tv_sec;
	b->nanoseconds = a->tv_usec * 1000;
}


void
normalizeTime(TimeInternal * r)
{
	r->seconds += r->nanoseconds / 1000000000;
	r->nanoseconds -= (r->nanoseconds / 1000000000) * 1000000000;

	if (r->seconds > 0 && r->nanoseconds < 0) {
		r->seconds -= 1;
		r->nanoseconds += 1000000000;
	} else if (r->seconds < 0 && r->nanoseconds > 0) {
		r->seconds += 1;
		r->nanoseconds -= 1000000000;
	}
}

void
addTime(TimeInternal * r, const TimeInternal * x, const TimeInternal * y)
{
	r->seconds = x->seconds + y->seconds;
	r->nanoseconds = x->nanoseconds + y->nanoseconds;

	normalizeTime(r);
}

void
subTime(TimeInternal * r, const TimeInternal * x, const TimeInternal * y)
{
	r->seconds = x->seconds - y->seconds;
	r->nanoseconds = x->nanoseconds - y->nanoseconds;

	normalizeTime(r);
}

/// Divide an internal time value
///
/// @param r the time to convert
/// @param divisor
///

#if 0
/* TODO: this function could be simplified, as currently it is only called to halve the time */
void
divTime(TimeInternal *r, int divisor)
{
	uint64_t nanoseconds;

	if (divisor <= 0)
		return;

	nanoseconds = ((uint64_t)r->seconds * 1000000000) + r->nanoseconds;
	nanoseconds /= divisor;

	r->seconds = 0;
	r->nanoseconds = nanoseconds;
	normalizeTime(r);
}
#endif

void
div2Time(TimeInternal *r)
{
	r->nanoseconds += (r->seconds % 2) * 1000000000;
	r->seconds /= 2;
	r->nanoseconds /= 2;

	normalizeTime(r);
}



/* clear an internal time value */
void
clearTime(TimeInternal *r)
{
	r->seconds     = 0;
	r->nanoseconds = 0;
}


/* sets a time value to a certain nanoseconds */
void
nano_to_Time(TimeInternal *x, int nano)
{
	x->seconds     = 0;
	x->nanoseconds = nano;
	normalizeTime(x);
}

/* greater than operation */
int
gtTime(const TimeInternal *x, const TimeInternal *y)
{
	TimeInternal r;

	subTime(&r, x, y);
	return !isTimeInternalNegative(&r);
}

/* remove sign from variable */
void
absTime(TimeInternal *r)
{
	/* Make sure signs are the same */
	normalizeTime(r);
	r->seconds       = abs(r->seconds);
	r->nanoseconds   = abs(r->nanoseconds);
}


/* if 2 time values are close enough for X nanoseconds */
int
is_Time_close(const TimeInternal *x, const TimeInternal *y, int nanos)
{
	TimeInternal r1;
	TimeInternal r2;

	// first, subtract the 2 values. then call abs(),
	// then call gtTime for requested the number of nanoseconds
	subTime(&r1, x, y);
	absTime(&r1);

	nano_to_Time(&r2, nanos);

	return !gtTime(&r1, &r2);
}


int
check_timestamp_is_fresh2(const TimeInternal * timeA, const TimeInternal * timeB)
{
	int ret;

	// maximum 1 millisecond offset
	ret = is_Time_close(timeA, timeB, 1000000);
	DBG2("check_timestamp_is_fresh: %d\n ", ret);
	return ret;
}


int
check_timestamp_is_fresh(const TimeInternal * timeA)
{
	TimeInternal timeB;
	getTime(&timeB);

	return check_timestamp_is_fresh2(timeA, &timeB);
}


int
isTimeInternalNegative(const TimeInternal * p)
{
	return (p->seconds < 0) || (p->nanoseconds < 0);
}

double
secondsToMidnight(void)
{
	TimeInternal now;
	double stm, ret;
	getTime(&now);
	stm = 86400.0 - (now.seconds % 86400);
	ret =  (stm - now.nanoseconds / 1E9);
	return ret;
}

double
getPauseAfterMidnight(Integer8 announceInterval, int pausePeriod)
{
	double ai = pow(2,announceInterval);

	if (pausePeriod > 2.0 * ai)
		return (pausePeriod);
	else
		return (2.0 * ai);
}

double
timeInternalToDouble(const TimeInternal * p)
{
	double sign = (p->seconds < 0 || p->nanoseconds < 0 ) ? -1.0 : 1.0;
	return (sign * ( abs(p->seconds) + abs(p->nanoseconds) / 1E9 ));
}

TimeInternal
doubleToTimeInternal(const double d)
{
	TimeInternal t = {0, 0};

	t.seconds = trunc(d);
	t.nanoseconds = (d - (t.seconds + 0.0)) * 1E9;

	return t;
}

/* FNV-1 hash, 32-bit, optional modulo limiter */
uint32_t
fnvHash(void *input, size_t len, int modulo)
{
    int i = 0;

    static uint32_t prime = 16777619;
    static uint32_t basis = 2166136261;

    uint32_t hash = basis;
    uint8_t *buf = (uint8_t*)input;

    for(i = 0; i < len; i++)  {
        hash *= prime;
        hash ^= *(buf + i);
    }

    return (modulo > 0 ? hash % modulo : hash);
}
