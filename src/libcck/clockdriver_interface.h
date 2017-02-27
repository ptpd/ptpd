/* Copyright (c) 2016-2017 Wojciech Owczarek,
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


static int clockDriver_init(ClockDriver*, const void *);
static int clockDriver_shutdown(ClockDriver *);

static bool getTime (ClockDriver*, CckTimestamp *);
static bool getTimeMonotonic (ClockDriver*, CckTimestamp *);
static bool getUtcTime (ClockDriver*, CckTimestamp *);
static bool setTime (ClockDriver*, CckTimestamp *);
static bool setOffset (ClockDriver*, CckTimestamp *);

static bool setFrequency (ClockDriver *, double, double);
static double getFrequency (ClockDriver *);

static bool getStatus (ClockDriver *, ClockStatus *);
static bool processStatus (ClockDriver *, ClockStatus *current, ClockStatus *new);
static bool getSystemClockOffset (ClockDriver *, CckTimestamp *);
static bool getOffsetFrom (ClockDriver *, ClockDriver *, CckTimestamp *);

static bool privateHealthCheck(ClockDriver *driver);
static bool isThisMe(ClockDriver *, const char* search);

static void loadVendorExt(ClockDriver *);

#define INIT_INTERFACE(var) \
    var->init = clockDriver_init; \
    var->shutdown = clockDriver_shutdown; \
    var->getTime = getTime; \
    var->getTimeMonotonic = getTimeMonotonic; \
    var->getUtcTime = getUtcTime; \
    var->setTime = setTime; \
    var->setOffset = setOffset; \
    var->setFrequency = setFrequency; \
    var->getFrequency = getFrequency; \
    var->getStatus = getStatus; \
    var->processStatus = processStatus; \
    var->getSystemClockOffset = getSystemClockOffset; \
    var->getOffsetFrom = getOffsetFrom; \
    var->privateHealthCheck = privateHealthCheck; \
    var->isThisMe = isThisMe; \
    var->loadVendorExt = loadVendorExt;
