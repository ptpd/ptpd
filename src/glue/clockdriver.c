/*-
 * Copyright (c) 2016      Wojciech Owczarek
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
 * @file   clockdriver.c
 * @date   Thu Nov 24 23:19:00 2016
 *
 * @brief  Clock driver wrappers between PTPd / lib1588 and libCCK
 *
 *
 */


#include "cck_glue.h"

#include <libcck/cck_utils.h>
#include <libcck/clockdriver.h>
#include <libcck/ttransport.h>

bool
configureClockDriver(ClockDriver *driver, const void *configData)
{

    const GlobalConfig *global = configData;

    bool ret = TRUE;

    ClockDriverConfig *config = &driver->config;

    config->stepType = CSTEP_ALWAYS;

    if(global->noResetClock) {
	config->stepType = CSTEP_NEVER;
    }

    if(global->stepOnce) {
	config->stepType = CSTEP_STARTUP;
    }

    if(global->stepForce) {
	config->stepType = CSTEP_STARTUP_FORCE;
    }

    config->disabled = (tokenInList(global->disabledClocks, driver->name, DEFAULT_TOKEN_DELIM)) ? TRUE:FALSE;
    config->excluded = (tokenInList(global->excludedClocks, driver->name, DEFAULT_TOKEN_DELIM)) ? TRUE:FALSE;

    if(global->noAdjust) {
	config->readOnly = TRUE;
    } else {
	config->readOnly = (tokenInList(global->readOnlyClocks, driver->name, DEFAULT_TOKEN_DELIM)) ? TRUE:FALSE;
    }

    if(config->required && config->disabled) {
	if(!config->readOnly) {
	    WARNING("clock: clock %s cannot be disabled - set to read-only to exclude from sync\n", driver->name);
	} else {
	    WARNING("clock: clock %s cannot be disabled - already set to read-only\n", driver->name);
	}
	config->disabled = FALSE;
    }

    config->noStep = global->noResetClock;
    config->negativeStep = global->negativeStep;
    config->storeToFile = global->storeToFile;
    config->adevPeriod = global->adevPeriod;
    config->stableAdev = global->stableAdev;
    config->unstableAdev = global->unstableAdev;
    config->lockedAge = global->lockedAge;
    config->holdoverAge = global->holdoverAge;
    config->stepTimeout = (global->enablePanicMode) ? global-> panicModeDuration : 0;
    config->calibrationTime = global->clockCalibrationTime;
    config->stepExitThreshold = global->panicModeExitThreshold;
    config->strictSync = global->clockStrictSync;
    config->minStep = global->clockMinStep;

    config->statFilter = global->clockStatFilterEnable;
    config->filterWindowSize = global->clockStatFilterWindowSize;

    if(config->filterWindowSize == 0) {
	config->filterWindowSize = global->clockSyncRate;
    }

    config->filterType = global->clockStatFilterType;
    config->filterWindowType = global->clockStatFilterWindowType;

    config->outlierFilter = global->clockOutlierFilterEnable;
    config->madWindowSize = global->clockOutlierFilterWindowSize;
    config->madDelay = global->clockOutlierFilterDelay;
    config->madMax = global->clockOutlierFilterCutoff;
    config->outlierFilterBlockTimeout = global->clockOutlierFilterBlockTimeout;

    driver->servo.kP = global->servoKP;
    driver->servo.kI = global->servoKI;

    driver->servo.maxOutput = global->servoMaxPpb;
    driver->servo.tauMethod = global->servoDtMethod;
    driver->servo.maxTau = global->servoMaxdT;

    strncpy(config->frequencyDir, global->frequencyDir, PATH_MAX);

    configureClockDriverFilters(driver);

    #define CCK_REGISTER_IMPL(drvtype, suffix, strname) \
	if(driver->type == drvtype) { \
	    ret = ret && (pushClockDriverPrivateConfig_##suffix(driver, global)); \
	}

    #include <libcck/clockdriver.def>

    DBG("clock: Clock driver %s configured\n", driver->name);

    return ret;

}

static bool
pushClockDriverPrivateConfig_linuxphc(ClockDriver *driver, const GlobalConfig *global)
{

    ClockDriverConfig *config = &driver->config;

    CCK_GET_PCONFIG(ClockDriver, linuxphc, driver, myConfig);

    config->stableAdev = global->stableAdev_hw;
    config->unstableAdev = global->unstableAdev_hw;
    config->lockedAge = global->lockedAge_hw;
    config->holdoverAge = global->holdoverAge_hw;
    config->negativeStep = global->negativeStep_hw;

    myConfig->lockDevice = global->lockClockDevice;

    driver->servo.kP = global->servoKP_hw;
    driver->servo.kI = global->servoKI_hw;
    driver->servo.maxOutput = global->servoMaxPpb_hw;

    return TRUE;

}

static bool
pushClockDriverPrivateConfig_unix(ClockDriver *driver, const GlobalConfig *global)
{

    CCK_GET_PCONFIG(ClockDriver, unix, driver, myConfig);

    myConfig->setRtc = global->setRtc;

    return TRUE;

}

/* wrapper */
void
getSystemTime(TimeInternal *time)
{
    getSystemClock()->getTime(getSystemClock(), time);
}

void
getPtpClockTime(TimeInternal *time, PtpClock *ptpClock)
{
    ClockDriver *cd = ptpClock->clockDriver;
    cd->getTime(cd, time);
}

void
myPtpClockStepNotify(void *owner)
{

    PtpClock *ptpClock = (PtpClock*)owner;

    DBG("PTP clock was notified of clock step - resetting outlier filters and re-calibrating offsets\n");

    recalibrateClock(ptpClock, true);
}

void
myPtpClockUpdate(void *owner)
{

    PtpClock *ptpClock = owner;

    if(ptpClock && ptpClock->masterClock) {

	ClockDriver *mc = ptpClock->masterClock;

	if((ptpClock->portDS.portState == PTP_MASTER) || (ptpClock->portDS.portState == PTP_PASSIVE)) {
	    mc->setState(mc, CS_LOCKED);
	}

	mc->touchClock(mc);

    }

}

void
myPtpClockLocked(void *owner, bool locked)
{

    PtpClock *ptpClock = owner;

    if(ptpClock) {
	ptpClock->clockLocked = locked;
    }

}

bool
prepareClockDrivers(PtpClock *ptpClock) {

	GlobalConfig *global = ptpClock->global;

	TTransport *ev = ptpClock->eventTransport;

	ClockDriver *cd = ev->getClockDriver(ev);

	configureClockDriver(getSystemClock(), global);

	ptpClock->clockDriver = cd;

	configureClockDriver(cd, global);
	cd->inUse = TRUE;
	cd->config.required = TRUE;

	if(ptpClock->portDS.portState == PTP_SLAVE) {
	    cd->setExternalReference(ptpClock->clockDriver, "PTP", RC_PTP);
	    cd->owner = ptpClock;
	    cd->callbacks.onStep = myPtpClockStepNotify;
	    cd->callbacks.onLock = myPtpClockLocked;
	}

	ClockDriver *lastMaster = ptpClock->masterClock;

	if(strlen(global->masterClock) > 0) {
	    ptpClock->masterClock = getClockDriverByName(global->masterClock);
	    if(ptpClock->masterClock == NULL) {
		WARNING("Could not find designated master clock: %s\n", global->masterClock);
	    }
	} else {
	    ptpClock->masterClock = NULL;
	}

	if((lastMaster) && (lastMaster != ptpClock->masterClock)) {
	    lastMaster->setReference(lastMaster, NULL);
	    lastMaster->setState(lastMaster, CS_FREERUN);
	}

	if((ptpClock->masterClock) && (ptpClock->masterClock != ptpClock->clockDriver)) {
	    if( (ptpClock->portDS.portState == PTP_MASTER) || (ptpClock->portDS.portState == PTP_PASSIVE)) {
		((ClockDriver*)ptpClock->masterClock)->setExternalReference(ptpClock->masterClock, "PREFMST", RC_EXTERNAL);
	    }
	}

	cd->callbacks.onUpdate = myPtpClockUpdate;

	return TRUE;


}

