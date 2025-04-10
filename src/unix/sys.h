/**
 * @file sys.h
 * Defines the timer API.
 *
 * Copyright (c) 2025 Ioannis Konstantelias
 *
 * All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SYS_H_
#define SYS_H_

#include "ptp_datatypes.h"

void sleep_for(double duration);

int set_cpu_affinity(int cpu);

enum clock_type
{
	CLOCK_STEADY,
	CLOCK_SYSTEM
};
void get_time(enum clock_type type, TimeInternal *time);
void set_time(const TimeInternal *time);

#endif /* SYS_H_ */
