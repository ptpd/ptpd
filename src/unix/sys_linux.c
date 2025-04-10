/**
 * @file sys_linux.c
 * Defines the timer API.
 *
 * Copyright (c) 2025 Ioannis Konstantelias
 *
 * All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "sys.h"
#include "ptpd_config.h"

#define _GNU_SOURCE // For sched_setaffinity
#define __USE_GNU   // For sched_setaffinity

#include <sched.h>

int
set_cpu_affinity(int cpu)
{
#ifdef HAVE_SCHED_H
	cpu_set_t mask;
	CPU_ZERO(&mask);
	if (cpu >= 0) {
		CPU_SET(cpu, &mask);
	} else {
		int i;
		for (i = 0; i < CPU_SETSIZE; i++) {
			CPU_SET(i, &mask);
		}
	}
	return sched_setaffinity(0, sizeof(mask), &mask);
#else
#pragma message("[unimplemented] __func__")
#endif /* HAVE_SCHED_H */

	return -1;
}
