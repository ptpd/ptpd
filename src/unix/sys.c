/**
 * @file sys.c
 * Defines the timer API.
 *
 * Copyright (c) 2025 Ioannis Konstantelias
 *
 * All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "sys.h"
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include "ptpd_config.h"
#include "time_ops.h"

void
sleep_for(double duration)
{
	const TimeInternal duration_ti = ti_from_double(duration);
	struct timespec remaining;
	ti_to_timespec(&duration_ti, &remaining);

	while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR) {
	}
}

void
get_time(enum clock_type type, TimeInternal *time)
{
#ifdef HAVE_CLOCK_GETTIME
	const clockid_t clk_id = type == CLOCK_SYSTEM ? CLOCK_REALTIME : CLOCK_MONOTONIC;
	struct timespec ts;
	if (clock_gettime(clk_id, &ts) == -1) {
		perror("clock_gettime");
		return;
	}
	ti_from_timespec(&ts, time);
#else
#pragma error("[unimplemented] " __func__)
#endif /* _POSIX_TIMERS */
}

void
set_time(const TimeInternal *time)
{
#ifdef HAVE_CLOCK_GETTIME
	struct timespec ts;
	ti_to_timespec(time, &ts);
	if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
		perror("clock_settime");
	}
#else
#pragma error("[unimplemented] " __func__)
#endif /* _POSIX_TIMERS */
}
