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

#include "clockdriver.h"
#include "clockdriver_interface.h"

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

    INIT_INTERFACE(self);
    INIT_DATA(self, linuxphc);
    INIT_CONFIG(self, linuxphc);

    GET_DATA(self, myData, linuxphc);

    myData->clockFd = -1;
    myData->clockId = -1;

}

static int
clockdriver_init(ClockDriver* self, const void *config) {

    GET_DATA(self, myData, linuxphc);
    GET_CONFIG(self, myConfig, linuxphc);

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
	if(myData->phcIndex == -1) {
	    ERROR("Device %s has no PTP clock\n", myConfig->networkDevice);
	    return -1;
	}
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

    self->maxFrequency = caps.max_adj;

    INFO("Successfully started Linux PHC clock driver %s (%s) clock ID %06x\n", self->name, myConfig->characterDevice, myData->clockId);

    self->_init = TRUE;

    self->setState(self, CS_FREERUN);

    snprintf(self->config.frequencyFile, PATH_MAX, PTPD_PROGNAME"_phc%d.frequency", myData->phcIndex);

    return 1;

}

static int
clockdriver_shutdown(ClockDriver *self) {

    GET_DATA(self, myData, linuxphc);
    GET_CONFIG(self, myConfig, linuxphc);

    INFO("Linux PHC clock driver %s (%s) shutting down\n", self->name, myConfig->characterDevice);
    if(myData->clockFd > -1) {
	close(myData->clockFd);
    }
    return 1;

}

static Boolean
getTime (ClockDriver *self, TimeInternal *time) {

    GET_DATA(self, myData, linuxphc);

	struct timespec tp;
	if (clock_gettime(myData->clockId, &tp) < 0) {
		PERROR("Linux PHC clock_gettime() failed, exiting.");
		exit(0);
	}
	time->seconds = tp.tv_sec;
	time->nanoseconds = tp.tv_nsec;

    return TRUE;

}

Boolean
getTimeMonotonic(ClockDriver *self, TimeInternal * time)
{
	return (getSystemClock()->getTimeMonotonic(getSystemClock(), time));
}


static Boolean
getUtcTime (ClockDriver *self, TimeInternal *out) {
    return TRUE;
}

static Boolean
setTime (ClockDriver *self, TimeInternal *time, Boolean force) {

	GET_CONFIG(self, myConfig, linuxphc);
	GET_DATA(self, myData, linuxphc);

	TimeInternal now;
	getTime(self, &now);

	if(self->config.readOnly) {
		return TRUE;
	}

	if(!force && !self->config.negativeStep && !gtTime(time, &now)) {
		WARNING("Cannot step Linux PHC  clock %s (%s) backwards\n", self->name, myConfig->characterDevice);
		return FALSE;
	}

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
stepTime (ClockDriver *self, TimeInternal *delta, Boolean force) {

	GET_CONFIG(self, myConfig, linuxphc);
	GET_DATA(self, myData, linuxphc);

	TimeInternal newTime;

	if(self->config.readOnly) {
		return TRUE;
	}

	if(!force && !self->config.negativeStep && isTimeNegative(delta)) {
		WARNING("Cannot step Linux PHC clock %s (%s) backwards\n", self->name, myConfig->characterDevice);
		return FALSE;
	}

	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = ADJ_SETOFFSET | ADJ_NANO;

	tmx.time.tv_sec = delta->seconds;
	tmx.time.tv_usec = delta->nanoseconds;

	if(clock_adjtime(myData->clockId, &tmx) < 0) {
	    DBG("Could set clock offset of Linux PHC clock %s (%s)", self->name, myConfig->characterDevice);
	    getTime(self, &newTime);
	    addTime(&newTime, &newTime, delta);
	    return setTime(self, &newTime, force);
	}

	NOTICE("Linux PHC clock %s (%s): stepped clock by %s%d.%09d seconds\n", self->name, myConfig->characterDevice,
		    (delta->seconds <0 || delta->nanoseconds <0) ? "-":"", delta->seconds, delta->nanoseconds);

	return TRUE;

}

static Boolean
setFrequency (ClockDriver *self, double adj, double tau) {

	GET_CONFIG(self, myConfig, linuxphc);
	GET_DATA(self, myData, linuxphc);

	if(self->config.readOnly) {
		return TRUE;
	}

	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = ADJ_FREQUENCY;

	CLAMP(adj,self->maxFrequency);

	tmx.freq = (long)(round(adj * 65.536));

	if(clock_adjtime(myData->clockId, &tmx) < 0) {
	    PERROR("Could not adjust frequency offset of clock %s (%s)", self->name, myConfig->characterDevice);
	    return FALSE;
	}

	self->processUpdate(self, adj, tau);

	return TRUE;

}

static double
getFrequency (ClockDriver *self) {

	GET_CONFIG(self, myConfig, linuxphc);
	GET_DATA(self, myData, linuxphc);

	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	if(clock_adjtime(myData->clockId, &tmx) < 0) {
	    PERROR("Could not get frequency of clock %s (%s)", self->name, myConfig->characterDevice);
	    return 0;
	}

	return(tmx.freq / 65.536);

}

static Boolean
restoreFrequency (ClockDriver *self) {
	return TRUE;
}

static Boolean
saveFrequency (ClockDriver *self) {
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

	GET_DATA(self, myData, linuxphc);
	GET_CONFIG(self, myConfig, linuxphc);

	if(ioctl(myData->clockFd, PTP_CLOCK_GETCAPS, caps) < 0) {
	    PERROR("Could not read clock capabilities for %s (%s): is the clock valid?", self->name, myConfig->characterDevice);
	    return FALSE;
	}

	return TRUE;

}

static Boolean
getOffsetFrom (ClockDriver *self, ClockDriver *from, TimeInternal *delta)
{

	GET_DATA(self, myData, linuxphc);
	GET_DATA(from, hisData, linuxphc);

	if(from->type == self->type) {
	    if(myData->phcIndex == hisData->phcIndex) {
		delta->seconds = 0;
		delta->nanoseconds = 0;
		return TRUE;
	    }
	}

	if(from->type == CLOCKDRIVER_UNIX && from->systemClock) {
	    if(!getSystemClockOffset(self, delta)) {
		return FALSE;
	    }
	    delta->seconds = -delta->seconds;
	    delta->nanoseconds = -delta->nanoseconds;
	    return TRUE;
	}

	return FALSE;

}

static Boolean
getSystemClockOffset(ClockDriver *self, TimeInternal *output)
{

    GET_DATA(self, myData, linuxphc);
    GET_CONFIG(self, myConfig, linuxphc);

    TimeInternal t1, t2, tptp, tmpDelta, duration, minDuration, delta;

    struct ptp_clock_time *samples;
    struct ptp_sys_offset sof;

    sof.n_samples = OSCLOCK_OFFSET_SAMPLES;

    if(ioctl(myData->clockFd, PTP_SYS_OFFSET, &sof) < 0) {
	PERROR("Could not read OS clock offset for %s (%s)",
		self->name, myConfig->characterDevice);
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

	subTime(&duration, &t2, &t1);

	if(i==0) {
	    minDuration = duration;
	}

	timeDelta(&t1, &tptp, &t2, &tmpDelta);

	if(!gtTime(&duration, &minDuration)) {
	    minDuration = duration;
	    delta = tmpDelta;
	}

    }

    if(output != NULL) {
	*output = delta;
    }

    return TRUE;
}

static Boolean
pushPrivateConfig(ClockDriver *self, RunTimeOpts *globalConfig)
{

    return TRUE;

}

static Boolean
isThisMe(ClockDriver *self, const char* search)
{
	GET_CONFIG(self, myConfig, linuxphc);

	if(!strncmp(search, myConfig->characterDevice, PATH_MAX)) {
		return TRUE;
	}

	if(!strncmp(search, myConfig->networkDevice, IFACE_NAME_LENGTH)) {
		return TRUE;
	}

	return FALSE;

}
