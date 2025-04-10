/**
 * @file impl.h
 * Defines the timer API.
 *
 * Copyright (c) 2025 Ioannis Konstantelias
 *
 * All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef IMPL_H_
#define IMPL_H_

#include "ptp_datatypes.h"
#include "unix/sys.h"

struct impl
{
	/**
	 * @{
	 * Implementation of the timer function
	 */
	struct tmr *(*tmr_create)(void);
	/** @ */

	void (*sleep_for)(double duration);
	int (*set_cpu_affinity)(int cpu);
	void (*get_time)(enum clock_type type, TimeInternal *time);
	void (*set_time)(const TimeInternal *time);
};

/**
 * @brief Global pointer to the implementation of system functions
 * Should be defined by the user
 */
extern const struct impl g_impl;

#endif /* IMPL_H */
