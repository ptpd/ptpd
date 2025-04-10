/*-
 * Copyright (c) 2015-2024 Ioannis Konstantelias,
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
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
 *
 * All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
#include "time_ops.h"

/**
 * @note Timestamp is fresh if it is at most 1 millisecond old.
 */
int
check_timestamp_is_fresh(const TimeInternal *timeA)
{
	TimeInternal timeB;
	g_impl.get_time(CLOCK_SYSTEM, &timeB);
	return ti_is_close(timeA, &timeB, 1000000);
}

double
secondsToMidnight(void)
{
	TimeInternal now;
	g_impl.get_time(CLOCK_SYSTEM, &now);

	const uint64_t nanoseconds_in_day = 86400UL * 1000000000ULL;
	const double nanoseconds_this_day = now.nanoseconds % nanoseconds_in_day;
	return nanoseconds_this_day / 1e9;
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
