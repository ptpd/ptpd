/*-
 * libCCK - Clock Construction Kit
 *
 * Copyright (c) 2014 Wojciech Owczarek,
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
 * @file   cck_clockdriver.h
 *
 * @brief  libCCK clockdriver component definitions
 *
 */

#ifndef CCK_CLOCKDRIVER_H_
#define CCK_CLOCKDRIVER_H_

#ifndef CCK_H_INSIDE_
#error libCCK component headers should not be uncluded directly - please include cck.h only.
#endif

typedef struct CckClockDriver CckClockDriver;

#include "cck.h"

#include <config.h>

/* Example data type provided by the component */
typedef struct {

    int param1;

} CckClockDriverStruct;

/*
  Configuration data for a clockdriver - some parameters are generic,
  so it's up to the clockdriver implementation how they are used.
  This is easier to use than per-implementation config, although
  this is still possible using configData.
*/
typedef struct {
	/* we probably don't need this, this is passed after create() */
	int clockDriverType;
	/* generic int type clockdriver specific parameter #1 - ttl for IP clockdrivers */
	int param1;
	/* generic pointer - implementations can make use of it */
	void* configData;
} CckClockDriverConfig;


struct CckClockDriver {
	/* mandatory - this has to be the first member */
	CckComponent header;

	int clockDriverType;
	CckClockDriverConfig config;

	/* clock status field - leap second information, UTC offset, etc. */
//	CckClockStatus status;

	/* integer clock ID / type - may be useful for some implementations */
	int clockId;
	int clockType;

	/* file descriptor, socket or another handle for clocks that need one */
	int fd;

	/* if true, the clock can be set / controlled */
	CckBool readOnly;

	/* if true, control of this clock is already "taken" - can be read, but not set anymore */
	CckBool busy;

	/* if clock is in use by another libCCK component, this pointer has the component tag */
	CckComponent* parent;

	/* container for implementation-specific data */
	void* clockDriverData;

	/* Example callback that can be assigned by the user */
	void (*clockDriverCallback) (CckBool, int*);

	/* ClockDriver interface */

	/* === libCCK interface mandatory methods begin ==== */

	/* shut the component down */
	int (*shutdown) (void*);

	/* === libCCK interface mandatory methods end ==== */

	/* start up the clock driver, perform whatever initialisation */
	int (*init) (CckClockDriver*, const CckClockDriverConfig*);

	/* read time */
	CckBool (*getTime) (CckClockDriver*, CckTimestamp*);
	/* step time */
	CckBool (*setTime) (CckClockDriver*, CckTimestamp*);
	/* apply positive / negative frequency adjustment in ppm */
	CckBool (*adjustFrequency) (CckClockDriver*, float);
	/* get maximum adjustment supported */
	float (*getMaxAdj) (void);
	/* refresh clock information (status field) - leap second status, UTC offset, etc */
	void (*updateStatus) (CckClockDriver*);
	/* set clock status data and update status field accordingly */
//	CckBool (*setStatus) (CckClockDriver*, CckClockStatus*);

};

/* libCCk core functions */
CckClockDriver*  createCckClockDriver(int clockDriverType, const char* instanceName);
void       setupCckClockDriver(CckClockDriver* clockDriver, int clockDriverType, const char* instanceName);
void       freeCckClockDriver(CckClockDriver** clockDriver);

/* any additional utility functions provided by the component */
void clearClockDriverStruct(CckClockDriverStruct* clockDriverStruct);

/* ============ CLOCKDRIVER IMPLEMENTATIONS BEGIN ============ */

/* clockdriver type enum */
enum {
	CCK_CLOCKDRIVER_NULL = 0,
	CCK_CLOCKDRIVER_UNIX
};

/* clockdriver implementation headers */

#include "clockdriver/cck_clockdriver_null.h"
#include "clockdriver/cck_clockdriver_unix.h"

/* ============ CLOCKDRIVER IMPLEMENTATIONS END ============== */

#endif /* CCK_CLOCKDRIVER_H_ */
