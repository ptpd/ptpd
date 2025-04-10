/**
 * @file impl.c
 *
 * Copyright (c) 2025 Ioannis Konstantelias
 *
 * All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "ptpd_config.h"
#include "impl.h"
#include "sys.h"

#ifdef HAVE_POSIX_TIMER
#include "timer_posix.h"
#else
#include "timer_itimer.h"
#endif /* HAVE_POSIX_TIMERS */

const struct impl g_impl = {
#ifdef HAVE_POSIX_TIMER
	.tmr_create = tmr_posix_create,
#else
	.tmr_create = tmr_itimer_create,
#endif /* HAVE_POSIX_TIMERS */
	.sleep_for = sleep_for,
	.set_cpu_affinity = set_cpu_affinity,
	.get_time = get_time,
	.set_time = set_time,
};
