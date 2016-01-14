/* Copyright (c) 2015 Wojciech Owczarek,
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
 * @file   clockdriver_interface.h
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  Clock driver API function declarations and macros for inclusion by
 *         clock driver implementations
 *
 */


static int clockdriver_init(ClockDriver*, const void *);
static int clockdriver_shutdown(ClockDriver *);

static Boolean getTime (ClockDriver*, TimeInternal *);
static Boolean getTimeMonotonic (ClockDriver*, TimeInternal *);
static Boolean getUtcTime (ClockDriver*, TimeInternal *);
static Boolean setTime (ClockDriver*, TimeInternal *, Boolean);
static Boolean stepTime (ClockDriver*, TimeInternal *, Boolean);

static Boolean setFrequency (ClockDriver *, double, double);
static double getFrequency (ClockDriver *);
static Boolean restoreFrequency (ClockDriver *);
static Boolean saveFrequency (ClockDriver *);

static Boolean getStatus (ClockDriver *, ClockStatus *);
static Boolean setStatus (ClockDriver *, ClockStatus *);
static Boolean getOffsetFrom (ClockDriver *, ClockDriver *, TimeInternal *);

#define INIT_INTERFACE(var) \
    var->init = clockdriver_init; \
    var->shutdown = clockdriver_shutdown; \
    var->getTime = getTime; \
    var->getTimeMonotonic = getTimeMonotonic; \
    var->getUtcTime = getUtcTime; \
    var->setTime = setTime; \
    var->stepTime = stepTime; \
    var->setFrequency = setFrequency; \
    var->getFrequency = getFrequency; \
    var->saveFrequency = saveFrequency; \
    var->restoreFrequency = restoreFrequency; \
    var->getStatus = getStatus; \
    var->setStatus = setStatus; \
    var->getOffsetFrom = getOffsetFrom;

#define INIT_DATA(var, type) \
    if(var->privateData == NULL) { \
	XCALLOC(var->privateData, sizeof(ClockDriverData_##type)); \
    }
#define INIT_CONFIG(var, type) \
    if(var->privateConfig == NULL) { \
	XCALLOC(var->privateConfig, sizeof(ClockDriverConfig_##type)); \
    }

#define GET_CONFIG(from, to, type) \
    ClockDriverConfig_##type *to = (ClockDriverConfig_##type*)from->privateConfig;
#define GET_DATA(from, to, type) \
    ClockDriverData_##type *to = (ClockDriverData_##type*)from->privateData;


