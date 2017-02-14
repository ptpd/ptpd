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
 * @file   clockdriver_linuxphc.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  Linux PHC clock driver implementation
 *
 */

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <sys/time.h>
#include <sys/timex.h>
#include <time.h>

#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <linux/ptp_clock.h>
#include <sys/syscall.h>

#include <libcck/cck.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_types.h>
#include <libcck/cck_logger.h>

#include <libcck/clockdriver.h>
#include <libcck/clockdriver_interface.h>
#include <libcck/net_utils.h>
#include <libcck/net_utils_linux.h>

#if defined(linux) && !defined(ADJ_SETOFFSET)
#define ADJ_SETOFFSET 0x0100
#endif

/*
 * Some Linux distributions come with kernels which support SYS_OFFSET,
 * but headers do not reflect this. If this fails in runtime,
 * the system is not fully usable for H/W PTP anyway.
 */
#if defined(HAVE_DECL_PTP_SYS_OFFSET) && !HAVE_DECL_PTP_SYS_OFFSET

#define PTP_MAX_SAMPLES 25
struct ptp_sys_offset {
    unsigned int n_samples;
    unsigned int rsv[3];
    struct ptp_clock_time ts[2 * PTP_MAX_SAMPLES + 1];
};
#define PTP_SYS_OFFSET     _IOW(PTP_CLK_MAGIC, 5, struct ptp_sys_offset)

#endif /* HAVE_DECL_PTP_SYS_OFFSET */

#define OSCLOCK_OFFSET_SAMPLES 9

#define THIS_COMPONENT "clock.linuxphc: "

/* tracking the number of instances */
static int _instanceCount = 0;

static bool getClockCapabilities(ClockDriver *self, struct ptp_clock_caps *caps);

#ifndef HAVE_CLOCK_ADJTIME
static inline int clock_adjtime(clockid_t clkid, struct timex *timex)
{
	return syscall(__NR_clock_adjtime, clkid, timex);
}
#endif


bool
_setupClockDriver_linuxphc(ClockDriver* self)
{

    INIT_INTERFACE(self);
    CCK_INIT_PDATA(ClockDriver, linuxphc, self);
    CCK_INIT_PCONFIG(ClockDriver, linuxphc, self);

    CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);

    myData->clockFd = -1;
    myData->clockId = -1;

    self->_instanceCount = &_instanceCount;

    _instanceCount++;

    return true;

}

static int
clockDriver_init(ClockDriver* self, const void *userData) {

    CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);
    CCK_GET_PCONFIG(ClockDriver, linuxphc, self, myConfig);

    struct ptp_clock_caps caps;

    char* initData = (char*)userData;

    if(strlen(initData) > 0) {

	if(interfaceExists(initData)) {
	    strncpy(myConfig->networkDevice, initData, IFNAMSIZ);
	} else {
	    strncpy(myConfig->characterDevice, initData, IFNAMSIZ);
	}

    }

    CCK_INFO(THIS_COMPONENT"Linux PHC clock driver %s starting\n", myConfig->networkDevice);

    self->_init = false;

    myData->clockFd = -1;
    myData->clockId = -1;

    if(strlen(myConfig->networkDevice)) {

	struct ethtool_ts_info info;
	if(!getEthtoolTsInfo(&info, myConfig->networkDevice)) {
	    return -1;
	}
	myData->phcIndex = info.phc_index;
	if(myData->phcIndex == -1) {
	    CCK_ERROR(THIS_COMPONENT"Device %s has no PTP clock\n", myConfig->networkDevice);
	    return -1;
	}
	snprintf(myConfig->characterDevice, PATH_MAX, "/dev/ptp%d", info.phc_index);

	self->loadVendorExt(self);

    }

    if(strlen(myConfig->characterDevice)) {
	myData->clockFd = open(myConfig->characterDevice, O_RDWR);
	if(myData->clockFd < 0) {
	    CCK_PERROR(THIS_COMPONENT"Could not open clock device %s for writing", myConfig->characterDevice);
	    return -1;
	}
	myData->clockId = (~(clockid_t) (myData->clockFd) << 3) | 3;
    }

    if(!getClockCapabilities(self, &caps)) {
	return -1;
    }

    if(caps.pps) {
	CCK_INFO(THIS_COMPONENT"Linux PHC clock '%s' supports 1PPS output\n", self->name);
	if (ioctl(myData->clockFd, PTP_ENABLE_PPS, 1) < 0) {
		CCK_ERROR(THIS_COMPONENT"Could not enable 1PPS output for Linux PHC clock '%s'\n", self->name);
	} else {
		CCK_INFO(THIS_COMPONENT"Successfully enabled 1PPS output for Linux PHC clock '%s'\n", self->name);
	}
    }

    /* run any vendor-specific initialisation code */
    self->_vendorInit(self);

    self->maxFrequency = caps.max_adj;

    self->servo.maxOutput = self->maxFrequency;

    CCK_INFO(THIS_COMPONENT"Linux PHC clock driver '%s' (%s, ID 0x%06x) started successfully\n", self->name, myConfig->characterDevice, myData->clockId);

    self->_init = true;

    self->setState(self, CS_FREERUN);

    memset(self->config.frequencyFile, 0, PATH_MAX);
    snprintf(self->config.frequencyFile, PATH_MAX, CLOCKDRIVER_FREQFILE_PREFIX"_phc%d.frequency", myData->phcIndex);

    return 1;

}

static int
clockDriver_shutdown(ClockDriver *self) {

    CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);
    CCK_GET_PCONFIG(ClockDriver, linuxphc, self, myConfig);

    CCK_INFO(THIS_COMPONENT"Linux PHC clock driver '%s' (%s) shutting down\n", self->name, myConfig->characterDevice);

    /* run any vendor-specific shutdown code */
    self->_vendorShutdown(self);

    if(myData->clockFd > -1) {
	close(myData->clockFd);
    }

    if(_instanceCount > 0) {
	_instanceCount--;
    }

    return 1;

}

static bool
getTime (ClockDriver *self, CckTimestamp *time) {

    CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);

	if((!self->_init) || (self->state == CS_HWFAULT)) {
	    return false;
	}

	struct timespec tp;
	if (clock_gettime(myData->clockId, &tp) < 0) {
		CCK_PERROR(THIS_COMPONENT"Linux PHC clock_gettime() failed,");
		self->setState(self, CS_HWFAULT);
		return false;
	}
	time->seconds = tp.tv_sec;
	time->nanoseconds = tp.tv_nsec;

    return true;

}

bool
getTimeMonotonic(ClockDriver *self, CckTimestamp * time)
{
	return (getSystemClock()->getTimeMonotonic(getSystemClock(), time));
}


static bool
getUtcTime (ClockDriver *self, CckTimestamp *out) {
    return true;
}

static bool
setTime (ClockDriver *self, CckTimestamp *time) {

	CCK_GET_PCONFIG(ClockDriver, linuxphc, self, myConfig);
	CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);

	if((!self->_init) || (self->state == CS_HWFAULT)) {
	    return false;
	}

	if((self->config.readOnly) || (self->config.disabled)) {
		return true;
	}

	struct timespec tp;
	tp.tv_sec = time->seconds;
	tp.tv_nsec = time->nanoseconds;

	if (clock_settime(myData->clockId, &tp) < 0) {
		CCK_PERROR(THIS_COMPONENT"Linux PHC clock %s (%s): Could not set time ", self->name, myConfig->characterDevice);
		self->setState(self, CS_HWFAULT);
		return false;
	}

	return true;

}

static bool
setOffset (ClockDriver *self, CckTimestamp *delta) {

	CckTimestamp newTime;

	CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);

	if((!self->_init) || (self->state == CS_HWFAULT)) {
	    return false;
	}

	if((self->config.readOnly) || (self->config.disabled)) {
		return true;
	}

	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = ADJ_SETOFFSET | ADJ_NANO;

	tmx.time.tv_sec = delta->seconds;
	tmx.time.tv_usec = delta->nanoseconds;

	int ret = clock_adjtime(myData->clockId, &tmx);

	if( ret < 0) {
#ifdef CCK_DEBUG
	    CCK_GET_PCONFIG(ClockDriver, linuxphc, self, myConfig);
	    CCK_DBG("Could set clock offset of Linux PHC clock %s (%s): %d\n", self->name, myConfig->characterDevice, ret);
#endif /* CCK_DEBUG */
	    getTime(self, &newTime);
	    tsOps()->add(&newTime, &newTime, delta);
	    return setTime(self, &newTime);
	}

	return true;

}


static bool
setFrequency (ClockDriver *self, double adj, double tau) {

	CCK_GET_PCONFIG(ClockDriver, linuxphc, self, myConfig);
	CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);

	self->_tau = tau;

	if((!self->_init) || (self->state == CS_HWFAULT)) {
	    return false;
	}

	if((self->config.readOnly) || (self->config.disabled)) {
		return false;
	}

	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = ADJ_FREQUENCY;

	adj = clamp(adj, self->maxFrequency);

	tmx.freq = (long)(round(adj * 65.536));

	if(clock_adjtime(myData->clockId, &tmx) < 0) {
	    CCK_PERROR(THIS_COMPONENT"Could not adjust frequency offset of clock %s (%s)", self->name, myConfig->characterDevice);
	    self->setState(self, CS_HWFAULT);
	    return false;
	}

	self->lastFrequency = adj;

	return true;

}

static double
getFrequency (ClockDriver *self) {

	CCK_GET_PCONFIG(ClockDriver, linuxphc, self, myConfig);
	CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);

	if((!self->_init) || (self->state == CS_HWFAULT)) {
	    return false;
	}

	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	if(clock_adjtime(myData->clockId, &tmx) < 0) {
	    CCK_PERROR(THIS_COMPONENT"Could not get frequency of clock %s (%s)", self->name, myConfig->characterDevice);
	    self->setState(self, CS_HWFAULT);
	    return 0;
	}
	return( (tmx.freq + 0.0) / 65.536);

}

static bool
    getStatus (ClockDriver *self, ClockStatus *status) {
    return true;
}

static bool
setStatus (ClockDriver *self, ClockStatus *status) {
    return true;
}

static bool
getClockCapabilities(ClockDriver *self, struct ptp_clock_caps *caps) {

	CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);
	CCK_GET_PCONFIG(ClockDriver, linuxphc, self, myConfig);

	if(ioctl(myData->clockFd, PTP_CLOCK_GETCAPS, caps) < 0) {
	    CCK_PERROR(THIS_COMPONENT"Could not read clock capabilities for %s (%s): is the clock valid?", self->name, myConfig->characterDevice);
	    return false;
	}

	return true;

}

static bool
getOffsetFrom (ClockDriver *self, ClockDriver *from, CckTimestamp *delta)
{

	CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);
	CCK_GET_PDATA(ClockDriver, linuxphc, from, hisData);

	CckTimestamp deltaA, deltaB;

	if((!self->_init) || (self->state == CS_HWFAULT)) {
	    return false;
	}

	if(from->type == self->type) {
	    if(myData->phcIndex == hisData->phcIndex) {
		delta->seconds = 0;
		delta->nanoseconds = 0;
		return true;
	    } else {
		if(!self->getSystemClockOffset(self, &deltaA)) {
		    return false;
		}
		if(!self->getSystemClockOffset(from, &deltaB)) {
		    return false;
		}
		tsOps()->sub(delta, &deltaA, &deltaB);
	    }
	    return true;
	}

	if((from->type == CLOCKDRIVER_UNIX) && from->systemClock) {
	    if(!self->getSystemClockOffset(self, delta)) {
		return false;
	    }
/*
	    delta->seconds = -delta->seconds;
	    delta->nanoseconds = -delta->nanoseconds;
*/
	    return true;
	}

	return false;

}

static bool
getSystemClockOffset(ClockDriver *self, CckTimestamp *output)
{

#if defined(CCK_DEBUG) || defined(PTPD_CLOCK_SYNC_PROFILING)
    char isMin;
#endif /* debug or clock sync profiling */

    CCK_GET_PDATA(ClockDriver, linuxphc, self, myData);
    CCK_GET_PCONFIG(ClockDriver, linuxphc, self, myConfig);

    CckTimestamp t1, t2, tptp, tmpDelta, duration, minDuration, delta;

    struct ptp_clock_time *samples;
    struct ptp_sys_offset sof;

    sof.n_samples = OSCLOCK_OFFSET_SAMPLES;

    if((!self->_init) || (self->state == CS_HWFAULT)) {
	return false;
    }


    if(ioctl(myData->clockFd, PTP_SYS_OFFSET, &sof) < 0) {
	CCK_PERROR(THIS_COMPONENT"Could not read OS clock offset for %s (%s)",
		self->name, myConfig->characterDevice);
	self->setState(self, CS_HWFAULT);
	return false;
    }

    samples = sof.ts;

    for(int i = 0; i < sof.n_samples; i++) {

	t1.seconds = samples[2*i].sec;
	t1.nanoseconds = samples[2*i].nsec;
	tptp.seconds = samples[2*i+1].sec;
	tptp.nanoseconds = samples[2*i+1].nsec;
	t2.seconds = samples[2*i+2].sec;
	t2.nanoseconds = samples[2*i+2].nsec;

	tsOps()->sub(&duration, &t2, &t1);

	tsOps()->rtt(&tmpDelta, &t1, &tptp, &t2);
#if defined(CCK_DEBUG) || defined(PTPD_CLOCK_SYNC_PROFILING)
	isMin = ' ';
#endif /* debug or clock sync profiling */

	if((i == 0) || (tsOps()->cmp(&duration, &minDuration) < 0) ) {
	    minDuration = duration;
	    delta = tmpDelta;
#if defined(CCK_DEBUG) || defined(PTPD_CLOCK_SYNC_PROFILING)
	    isMin = '+';
#endif /* debug or clock sync profiling */
	}
#ifdef PTPD_CLOCK_SYNC_PROFILING
	CCK_INFO(THIS_COMPONENT"prof ref meas %c\t clock %s\t seq %d\t dur %d.%09d\t delta %d.%09d\n",
	isMin, self->name, i, duration.seconds, duration.nanoseconds,
	tmpDelta.seconds, tmpDelta.nanoseconds);
#else
	CCK_DBG(THIS_COMPONENT"prof ref meas %c\t clock %s\t seq %d\t dur %d.%09d\t delta %d.%09d\n",
	isMin, self->name, i, duration.seconds, duration.nanoseconds,
	tmpDelta.seconds, tmpDelta.nanoseconds);
#endif
    }

    if(output != NULL) {
	*output = delta;
    }

    return true;
}

static bool
isThisMe(ClockDriver *self, const char* search)
{
	CCK_GET_PCONFIG(ClockDriver, linuxphc, self, myConfig);

	if(!strncmp(search, myConfig->characterDevice, PATH_MAX)) {
		return true;
	}

	if(!strncmp(search, myConfig->networkDevice, IFNAMSIZ)) {
		return true;
	}

	return false;

}

static void
loadVendorExt(ClockDriver *self) {

	bool found = false;
	char vendorName[100];
	uint32_t oui;
	int (*loader)(ClockDriver*, const char*);

	CCK_GET_PCONFIG(ClockDriver, linuxphc, self, myConfig);

	int ret = getHwAddrData((unsigned char*)&oui, myConfig->networkDevice, 4);

	oui = ntohl(oui) >> 8;

	if(ret != 1) {
	    CCK_WARNING(THIS_COMPONENT"%s: could not retrieve hardware address for vendor extension check\n",
		    self->name);
	} else {

	    memset(vendorName, 0, 100);
	    CCK_INFO(THIS_COMPONENT"%s: probing for vendor extensions (OUI %06x)\n", self->name,
				oui);

	    #define LOAD_VENDOR_EXT(voui,vendor,name) \
		case voui: \
		    found = true; \
		    strncpy(vendorName, name, 100); \
		    loader = loadCdVendorExt_##vendor; \
		    break;

	    switch(oui) {

		#include "vext/linuxphc_vext.def"

		default:
		    CCK_INFO(THIS_COMPONENT"%s: no vendor extensions available\n", self->name);
	    }

	    #undef LOAD_VENDOR_EXT

	    if(found) {
		if(loader(self, myConfig->networkDevice) < 0) {
		    CCK_ERROR(THIS_COMPONENT"%s: vendor: %s, failed loading extensions, using Linux PHC\n", self->name, vendorName);
		} else {
		    CCK_INFO(THIS_COMPONENT"%s: vendor: %s, extensions loaded successfully\n", self->name, vendorName);
		}
	    }

	}

}


static bool
privateHealthCheck(ClockDriver *driver)
{

    bool ret = true;

    if(driver == NULL) {
	return false;
    }

    CCK_DBG(THIS_COMPONENT"clock %s private health check...\n", driver->name);

    return ret;

}
