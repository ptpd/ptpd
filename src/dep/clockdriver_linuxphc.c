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
 * @file   clockdriver_linuxphc.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  Linux PHC clock driver implementation
 *
 */

#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <linux/ptp_clock.h>
#include <sys/syscall.h>
#include <sys/timex.h>
#include "clockdriver.h"

#define GET_CONFIG() \
    ClockDriverConfig_linuxphc *myConfig = (ClockDriverConfig_linuxphc*)self->config;
#define GET_DATA() \
    ClockDriverData_linuxphc *myData = (ClockDriverData_linuxphc*)self->data;


static int clockdriver_init(ClockDriver*, const void *);
static int clockdriver_shutdown(ClockDriver *);

static Boolean getTime_linuxphc (ClockDriver*, TimeInternal *);
static Boolean getUtcTime (ClockDriver*, TimeInternal *);
static Boolean setTime_linuxphc (ClockDriver*, TimeInternal *);
static Boolean adjustFrequency (ClockDriver *, double, double);
static Boolean getStatus (ClockDriver *, ClockStatus *);
static Boolean setStatus (ClockDriver *, ClockStatus *);

static Boolean getClockCapabilities(ClockDriver *self, struct ptp_clock_caps *caps);
static Boolean getSystemClockOffset(ClockDriver *self, TimeInternal *delta);

#ifndef HAVE_CLOCK_ADJTIME
static inline int clock_adjtime(clockid_t clkid, struct timex *timex)
{
	return syscall(__NR_clock_adjtime, clkid, timex);
}
#endif


void
_setupClockDriver_linuxphc(ClockDriver* self)
{

    self->init = clockdriver_init;
    self->shutdown = clockdriver_shutdown;

    self->getTime = getTime_linuxphc;
    self->getUtcTime = getUtcTime;
    self->setTime = setTime_linuxphc;
    self->adjustFrequency = adjustFrequency;
    self->getStatus = getStatus;
    self->setStatus = setStatus;

    if(self->data == NULL) {
	XCALLOC(self->data, sizeof(ClockDriverData_linuxphc));
    }

    if(self->config == NULL) {
	XCALLOC(self->config, sizeof(ClockDriverConfig_linuxphc));
    }

    GET_DATA();

    myData->clockFd = -1;
    myData->clockId = -1;
}

static int
clockdriver_init(ClockDriver* self, const void *config) {

    GET_DATA();
    GET_CONFIG();

    struct ptp_clock_caps caps;

    *myConfig = *((ClockDriverConfig_linuxphc*) config);



    INFO("Linux PHC clock driver %s starting\n", myConfig->networkDevice);

    self->_init = FALSE;

    myData->clockFd = -1;
    myData->clockId = -1;

    if(strlen(myConfig->networkDevice)) {

	struct ethtool_ts_info info;
	if(!getTsInfo(myConfig->networkDevice, &info)) {
	    return -1;
	}
	myData->phcIndex = info.phc_index;
	snprintf(myConfig->characterDevice, PATH_MAX, "/dev/ptp%d", info.phc_index);
    }

    if(strlen(myConfig->characterDevice)) {
	myData->clockFd = open(myConfig->characterDevice, O_RDWR);
	if(myData->clockFd < 0) {
	    PERROR("Could not open clock device %s for writing", myConfig->characterDevice);
	    return -1;
	}
	myData->clockId = (~(clockid_t) (myData->clockFd) << 3) | 3;
    }

    if(!getClockCapabilities(self, &caps)) {
	return -1;
    }

    self->maxFreqAdj = caps.max_adj;

    INFO("Successfully started Linux PHC driver %s (%s) clock ID %06x\n", self->name, myConfig->characterDevice, myData->clockId);

    self->_init = TRUE;

    return 1;

}

static int
clockdriver_shutdown(ClockDriver *self) {

    GET_DATA();
    GET_CONFIG();

    INFO("Linux PHC clock driver %s (%s) shutting down\n", self->name, myConfig->characterDevice);
    if(myData->clockFd > -1) {
	close(myData->clockFd);
    }
    return 1;

}

static Boolean
getTime_linuxphc (ClockDriver *self, TimeInternal *time) {

    GET_DATA();

	struct timespec tp;
	if (clock_gettime(myData->clockId, &tp) < 0) {
		PERROR("Linux PHC clock_gettime() failed, exiting.");
		exit(0);
	}
	time->seconds = tp.tv_sec;
	time->nanoseconds = tp.tv_nsec;

    return TRUE;

}

static Boolean
getUtcTime (ClockDriver *self, TimeInternal *out) {
    return TRUE;
}

static Boolean
setTime_linuxphc (ClockDriver *self, TimeInternal *time) {

	GET_CONFIG();
	GET_DATA();

	struct timespec tp;
	tp.tv_sec = time->seconds;
	tp.tv_nsec = time->nanoseconds;

	if (clock_settime(myData->clockId, &tp) < 0) {
		PERROR("clock driver %s (%s): Could not set time ", self->name, myConfig->characterDevice);
		return FALSE;
	}

	struct timespec tmpTs = { time->seconds,0 };

	char timeStr[MAXTIMESTR];
	strftime(timeStr, MAXTIMESTR, "%x %X", localtime(&tmpTs.tv_sec));
	NOTICE("Linux PHC clock %s (%s): stepped clock to: %s.%d\n",
		self->name, myConfig->characterDevice,
	       timeStr, time->nanoseconds);

	return TRUE;

}
static Boolean
adjustFrequency (ClockDriver *self, double adj, double tau) {
	GET_CONFIG();
	GET_DATA();

	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = ADJ_FREQUENCY;

	CLAMP(adj,self->maxFreqAdj);

	tmx.freq = (long)(round(adj * 65.536));

	if(clock_adjtime(myData->clockId, &tmx) < 0) {
	    PERROR("Could not adjust frequency offset of clock %s (%s)", self->name, myConfig->characterDevice);
	    return FALSE;
	}

	return TRUE;

}
static Boolean
    getStatus (ClockDriver *self, ClockStatus *status) {
    return TRUE;
}

static Boolean
setStatus (ClockDriver *self, ClockStatus *status) {
    return TRUE;
}

static Boolean
getClockCapabilities(ClockDriver *self, struct ptp_clock_caps *caps) {

	GET_DATA();
	GET_CONFIG();

	if(ioctl(myData->clockFd, PTP_CLOCK_GETCAPS, caps) < 0) {
	    PERROR("Could not read clock capabilities for %s (%s): is the clock valid?", self->name, myConfig->characterDevice);
	    return FALSE;
	}

	getSystemClockOffset(self, NULL);

	return TRUE;

}

static Boolean
getSystemClockOffset(ClockDriver *self, TimeInternal *delta)
{

    GET_DATA();
    GET_CONFIG();

    TimeInternal t1, t2, tptp, tmpDelta;

    struct ptp_clock_time *samples;

    struct ptp_sys_offset sof;
    sof.n_samples = 15;

    if(ioctl(myData->clockFd, PTP_SYS_OFFSET, &sof) < 0) {
	return FALSE;
    }

    samples = sof.ts;

    for(int i = 0; i < sof.n_samples; i++) {

	t1.seconds = samples[2*i].sec;
	t1.nanoseconds = samples[2*i].nsec;

	tptp.seconds = samples[2*i+1].sec;
	tptp.nanoseconds = samples[2*i+1].nsec;

	t2.seconds = samples[2*i+2].sec;
	t2.nanoseconds = samples[2*i+2].nsec;

	addTime(&tmpDelta, &t1, &t2);
	div2Time(&tmpDelta);
	subTime(&tmpDelta, &tmpDelta, &tptp);

	INFO("Delta %d: %ld.%09ld ns\n", i, tmpDelta.seconds, tmpDelta.nanoseconds);

    }

    return TRUE;
}
