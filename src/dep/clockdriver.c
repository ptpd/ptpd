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
 * @file   clockdriver.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  Initialisation code for the clock driver
 *
 */

#include "clockdriver.h"

#define THIS_COMPONENT "clock: "

#define REGISTER_CLOCKDRIVER(type, suffix) \
	if(driverType==type) { \
	    setup = _setupClockDriver_##suffix(clockDriver);\
	    found = TRUE;\
	}

/* linked list - so that we can control all registered objects centrally */
LINKED_LIST_HOOK(ClockDriver);

static ClockDriver* _systemClock = NULL;
static ClockDriver* _bestClock = NULL;
static int _updateInterval = CLOCKDRIVER_UPDATE_INTERVAL;
static int _syncInterval = 1.0 / (CLOCK_SYNC_RATE + 0.0);


static const char *clockDriverNames[] = {
    [CLOCKDRIVER_UNIX] = "unix",
    [CLOCKDRIVER_LINUXPHC] = "linuxphc"
};

/* inherited methods */

static void setState(ClockDriver *, ClockState);
static void processUpdate(ClockDriver *);
static void touchClock(ClockDriver *driver);
static Boolean pushConfig(ClockDriver *, RunTimeOpts *);
static Boolean healthCheck(ClockDriver *);
static void setReference(ClockDriver *, ClockDriver *);
static void setExternalReference(ClockDriver *, const char*, int);
static void restoreFrequency (ClockDriver *);
static void storeFrequency (ClockDriver *);
static Boolean adjustFrequency (ClockDriver *, double, double);
static Boolean syncClock (ClockDriver*, double);
static Boolean syncClockExternal (ClockDriver*, TimeInternal, double);
static void putInfoLine(ClockDriver*, char*, int);
static void putStatsLine(ClockDriver*, char*, int);


/* inherited methods end */

static ClockDriver* compareClockDriver(ClockDriver *, ClockDriver *);
static void findBestClock();
static Boolean disciplineClock(ClockDriver *, TimeInternal, double);

ClockDriver *
createClockDriver(int driverType, const char *name)
{

    ClockDriver *clockDriver = NULL;

    if(getClockDriverByName(name) != NULL) {
	ERROR(THIS_COMPONENT"Cannot create clock driver %s: clock driver with this name already exists\n", name);
	return NULL;
    }

    XCALLOC(clockDriver, sizeof(ClockDriver));

    if(!setupClockDriver(clockDriver, driverType, name)) {
	if(clockDriver != NULL) {
	    free(clockDriver);
	}
	return NULL;
    } else {
	/* maintain the linked list */
	LINKED_LIST_INSERT(clockDriver);
    }

    clockDriver->refClass = -1;

    StatFilterOptions opts;
    memset(&opts, 0, sizeof(StatFilterOptions));

    /* for now only, hard-coded filter */
    opts.enabled = TRUE;
    opts.filterType = FILTER_MEDIAN;
    opts.windowSize = CLOCK_SYNC_RATE;
    opts.windowType = WINDOW_SLIDING;

    clockDriver->distance = 255;
    clockDriver->_filter = createDoubleMovingStatFilter(&opts, name);

    return clockDriver;

}

Boolean
setupClockDriver(ClockDriver* clockDriver, int driverType, const char *name)
{

    Boolean found = FALSE;
    Boolean setup = FALSE;

    clockDriver->type = driverType;
    strncpy(clockDriver->name, name, CLOCKDRIVER_NAME_MAX);

    /* inherited methods - implementation may wish to override them,
     * or even preserve these pointers in its private data and call both
     */

    clockDriver->setState = setState;
    clockDriver->processUpdate = processUpdate;
    clockDriver->pushConfig = pushConfig;
    clockDriver->healthCheck = healthCheck;

    clockDriver->setReference = setReference;
    clockDriver->setExternalReference = setExternalReference;

    clockDriver->storeFrequency = storeFrequency;
    clockDriver->restoreFrequency = restoreFrequency;
    clockDriver->adjustFrequency = adjustFrequency;

    clockDriver->syncClock = syncClock;
    clockDriver->syncClockExternal = syncClockExternal;

    clockDriver->touchClock = touchClock;

    clockDriver->putStatsLine = putStatsLine;
    clockDriver->putInfoLine = putInfoLine;

    /* inherited methods end */

    /* these macros call the setup functions for existing clock drivers */
    REGISTER_CLOCKDRIVER(CLOCKDRIVER_UNIX, unix);
    REGISTER_CLOCKDRIVER(CLOCKDRIVER_LINUXPHC, linuxphc);

    if(!found) {
	ERROR(THIS_COMPONENT"Setup requested for unknown clock driver type: %d\n", driverType);
    } else if(!setup) {
	return FALSE;
    } else {
	clockDriver->getTimeMonotonic(clockDriver, &clockDriver->_initTime);
	clockDriver->getTimeMonotonic(clockDriver, &clockDriver->_lastUpdate);
	DBG(THIS_COMPONENT"Created new clock driver type %d name %s serial %d\n", driverType, name, clockDriver->_serial);
    }

    clockDriver->state = CS_INIT;
    setupPIservo(&clockDriver->servo);
    clockDriver->servo.controller = clockDriver;

    return found;

}

void
freeClockDriver(ClockDriver** clockDriver)
{

    ClockDriver *pdriver = *clockDriver;
    ClockDriver *cd;

    if(*clockDriver == NULL) {
	return;
    }

    LINKED_LIST_FOREACH(cd) {
	if(cd->refClock == pdriver) {
	    pdriver->setReference(pdriver, NULL);
	}
    }

    if(pdriver->_init) {
	pdriver->shutdown(pdriver);
    }

    /* maintain the linked list */
    LINKED_LIST_REMOVE(pdriver);


    if(pdriver->_privateData != NULL) {
	free(pdriver->_privateData);
    }

    if(pdriver->_privateConfig != NULL) {
	free(pdriver->_privateConfig);
    }

    DBG(THIS_COMPONENT"Deleted clock driver type %d name %s serial %d\n", pdriver->type, pdriver->name, pdriver->_serial);

    if(pdriver == _systemClock) {
	_systemClock = NULL;
    }

    freeDoubleMovingStatFilter(&pdriver->_filter);

    free(*clockDriver);

    *clockDriver = NULL;

};

ClockDriver*
getSystemClock() {

    if(_systemClock != NULL) {
	return _systemClock;
    }

    _systemClock = createClockDriver(CLOCKDRIVER_UNIX, SYSTEM_CLOCK_NAME);

    if(_systemClock == NULL) {
	CRITICAL(THIS_COMPONENT"Could not start system clock driver, cannot continue\n");
	exit(1);
    }

    _systemClock->init(_systemClock, NULL);

    return _systemClock;

}

void
shutdownClockDrivers() {

	ClockDriver *cd;
	LINKED_LIST_DESTROYALL(cd, freeClockDriver);
	_systemClock = NULL;

}

Boolean
createClockDriversFromString(const char* list, RunTimeOpts *rtOpts, Boolean quiet) {

	ClockDriverSpec spec;
	ClockDriver *cd = NULL;
	int namelen = 0;
	int pathlen = 0;
	memset(&spec, 0, sizeof(spec));

	foreach_token_begin(clockspecs, list, specLine, DEFAULT_TOKEN_DELIM);

	    if(!parseClockDriverSpec(specLine, &spec)) {
		counter_clockspecs--;
		continue;
	    }

	    pathlen = strlen(spec.path);
	    namelen = strlen(spec.name);

	    if( (namelen <= 0) && (pathlen > 0) ) {
		strncpy(spec.name, spec.path + ( pathlen <= 5 ? 0 : pathlen - 5), CLOCKDRIVER_NAME_MAX);
	    }

	    namelen = strlen(spec.name);

	    if( (pathlen <= 0) && (namelen > 0) ) {
		strncpy(spec.path, spec.name, CLOCKDRIVER_NAME_MAX);
	    }

	    if(!strlen(spec.name) && !strlen(spec.path)) {
		ERROR(THIS_COMPONENT"Clock driver string: \"%s\": no name or path given\n", specLine);
		counter_clockspecs--;
		continue;
	    }

	    if((cd = getClockDriverByName(spec.name))) {
		if(!quiet) {
		    WARNING(THIS_COMPONENT"Clock driver string: \"%s\" : clock driver %s already exists\n", specLine, spec.name);
		}
	    } else if((cd = findClockDriver(spec.path))) {
		if(!quiet) {
		    WARNING(THIS_COMPONENT"Clock driver string: \"%s\" : cannot create another clock driver of this type\n", specLine, spec.name);
		}
	    } else {
		cd = createClockDriver(spec.type, spec.name);
	    }

	    if(cd != NULL) {
		if(!cd->_init) {
		    INFO(THIS_COMPONENT"Extra clock starting: name %s type %s path %s\n", spec.name, getClockDriverName(spec.type), spec.path);
		    cd->init(cd, spec.path);
		}
		cd->inUse = TRUE;
		cd->pushConfig(cd, rtOpts);
	    }

	foreach_token_end(clockspecs);

	return TRUE;

}


static void
setState(ClockDriver *driver, ClockState newState) {

	if(driver == NULL) {
	    return;
	}

	if((newState > CS_FREERUN) && driver->config.disabled) {
	    return;
	}

	/* todo: switch/case FSM leaving+entering as we get more conditions */

	if(driver->state != newState) {
	    NOTICE(THIS_COMPONENT"Clock %s changed state from %s to %s\n",
		    driver->name, getClockStateName(driver->state),
		    getClockStateName(newState));

	    getSystemClock()->getTimeMonotonic(getSystemClock(), &driver->_lastUpdate);
	    clearTime(&driver->age);

	    /* if we're going into FREERUN, but not from a "good" state, restore last good frequency */
	    if( (newState == CS_FREERUN) && !(driver->state == CS_LOCKED || driver->state == CS_HOLDOVER)) {
		driver->restoreFrequency(driver);
	    }

	    if( (newState == CS_LOCKED) && !driver->_locked) {
		driver->_locked = TRUE;
		driver->minAdev = driver->adev;
		driver->maxAdev = driver->adev;
	    }

	    /* entering or leaving locked state */
	    if((newState == CS_LOCKED) || (driver->state == CS_LOCKED)) {
		findBestClock();
		INFO(THIS_COMPONENT"Clock %s adev %.03f minAdev %.03f maxAdev %.03f minAdevTotal %.03f maxAdevTotal %.03f totalAdev %.03f\n",
		driver->name, driver->adev, driver->minAdev, driver->maxAdev, driver->minAdevTotal, driver->maxAdevTotal, driver->totalAdev);
	    }

	    if(newState == CS_HWFAULT) {
		driver->setReference(driver, NULL);
	    }

	    driver->lastState = driver->state;
	    driver->state = newState;

	}

}

void
updateClockDrivers() {

	ClockDriver *cd;

	TimeInternal now;

	LINKED_LIST_FOREACH(cd) {

	    if(cd->config.disabled) {
		continue;
	    }

	    if(cd->_warningTimeout > 0) {
		cd->_warningTimeout -= _updateInterval;
	    }

	    getSystemClock()->getTimeMonotonic(getSystemClock(), &now);
	    subTime(&cd->age, &now, &cd->_lastUpdate);

	    DBGV(THIS_COMPONENT"Clock %s age %d.%d\n", cd->name, cd->age.seconds, cd->age.nanoseconds);

	    switch(cd->state) {

		case CS_HWFAULT:
		    if(cd->age.seconds >= cd->config.failureDelay) {
			if(cd->healthCheck(cd)) {
			    cd->setState(cd, CS_FREERUN);
			} else {
			    cd->touchClock(cd);
			}
		    }
		    break;
		case CS_INIT:
		    break;
		case CS_STEP:
		    if(cd->age.seconds >= cd->config.stepTimeout) {
			WARNING(THIS_COMPONENT"Clock %s suspension delay timeout, resuming clock updates\n", cd->name);
			cd->setState(cd, CS_FREERUN);
			cd->_canResume = TRUE;
		    }
		    break;
		case CS_NEGSTEP:
		    break;
		case CS_FREERUN:
		    if((cd->refClock == NULL) && (!cd->externalReference)) {
			if(_bestClock != NULL) {
			    cd->setReference(cd, _bestClock);
			}
		    }
		    break;
		case CS_LOCKED:
			if((cd->refClock == NULL) && (!cd->externalReference)) {
			    cd->setState(cd, CS_HOLDOVER);
			    resetIntPermanentAdev(&cd->_adev);
			    break;
			} else if(cd->refClock != NULL) {
			    if((cd->refClock->state != CS_LOCKED) && (cd->refClock->state != CS_HOLDOVER)) {
				cd->setState(cd, CS_HOLDOVER);
				resetIntPermanentAdev(&cd->_adev);
				cd->setReference(cd, NULL);
				break;
			    }
			}
			if(cd->age.seconds > cd->config.lockedAge) {
			    resetIntPermanentAdev(&cd->_adev);
			    cd->setState(cd, CS_HOLDOVER);
			}
		    break;
		case CS_TRACKING:
			if((cd->refClock == NULL) && (!cd->externalReference)) {
			    resetIntPermanentAdev(&cd->_adev);
			    cd->setState(cd, CS_FREERUN);
			    break;
			}
		    break;
		case CS_HOLDOVER:
			if(cd->age.seconds > cd->config.holdoverAge) {
			    cd->setState(cd, CS_FREERUN);
			}
		    break;
		default:
		    DBG(THIS_COMPONENT"Clock driver %s in unknown state %02d\n", cd->name, cd->state);

	    }

	}

	findBestClock();

}

void
syncClocks(double tau) {

    _syncInterval = tau;

    ClockDriver *cd;

    /* sync locked clocks first, in case if they are to unlock */
    LINKED_LIST_FOREACH(cd) {

	    if(cd->config.disabled) {
		continue;
	    }

	    if(cd->state == CS_LOCKED) {
		cd->syncClock(cd, tau);
		cd->_skipSync = TRUE;
	    }
    }
    /* sync the whole rest */
    LINKED_LIST_FOREACH(cd) {

	    if((cd->config.disabled) || (cd->state == CS_HWFAULT)) {
		continue;
	    }

	    if(!cd->_skipSync) {
		cd->syncClock(cd, tau);
	    }
	    cd->_skipSync = FALSE;
    }


}

void
stepClocks(Boolean force) {

    ClockDriver *cd;
    LINKED_LIST_FOREACH(cd) {

	if((cd->config.disabled) || (cd->state == CS_HWFAULT)) {
	    continue;
	}

	if((cd != _bestClock) && ((cd->refClock != NULL) || cd->externalReference)) {
	DBG("%s STEP ref %d.%d\n", cd->name, cd->refOffset.seconds, cd->refOffset.nanoseconds);
	    cd->stepTime(cd, &cd->refOffset, force);
	}
    }

    /* step the best clock last so that other clocks don't lose its reference beforehand */
    if(_bestClock != NULL) {
	    _bestClock->stepTime(_bestClock, &_bestClock->refOffset, force);
    }

}

void
reconfigureClockDrivers(RunTimeOpts *rtOpts) {

    ClockDriver *cd;

    LINKED_LIST_FOREACH(cd) {
	cd->pushConfig(cd, rtOpts);
    }

	/* system clock cannot be disabled */
	getSystemClock()->config.disabled = FALSE;

    findBestClock();

}

static void
processUpdate(ClockDriver *driver) {

	Boolean update = FALSE;

	if(driver->config.disabled) {
	    return;
	}

	if(driver->state == CS_HWFAULT) {
	    return;
	}

	feedIntPermanentAdev(&driver->_adev, driver->lastFrequency);
	driver->totalAdev = feedIntPermanentAdev(&driver->_totalAdev, driver->lastFrequency);

	if(driver->servo.runningMaxOutput) {
	    /*resetIntPermanentAdev(&driver->_adev);*/
	}
	/* we have enough allan dev samples to represent adev period */
	if( (driver->_tau > ZEROF) && ((driver->_adev.count * driver->_tau) > driver->config.adevPeriod) ) {

	    driver->adev = driver->_adev.adev;

	    if(driver->adev > ZEROF) {
		if(!driver->adevValid) {
		    driver->minAdevTotal = driver->adev;
		    driver->maxAdevTotal = driver->adev;
		    }

		if(driver->adev > driver->maxAdevTotal) {
		    driver->maxAdevTotal = driver->adev;
		}

		if(driver->adev < driver->minAdevTotal) {
		    driver->minAdevTotal = driver->adev;
		}
	    }

	    DBG(THIS_COMPONENT"clock %s  ADEV %.09f\n", driver->name, driver->adev);

	    if(driver->state == CS_LOCKED) {
		if(driver->adev > driver->maxAdev) {
		    driver->maxAdev = driver->adev;
		}
		if(driver->adev < driver->minAdev) {
		    driver->minAdev = driver->adev;
		}
	    }

	    if((driver->state == CS_STEP) || (driver->state == CS_NEGSTEP)) {
		update = FALSE;
	    } else {
		if(driver->servo.runningMaxOutput) {
		    driver->setState(driver, CS_TRACKING);
		} else if(driver->adev <= driver->config.stableAdev) {
		    driver->storeFrequency(driver);
		    driver->setState(driver, CS_LOCKED);
		} else if((driver->adev >= driver->config.unstableAdev) && (driver->state == CS_LOCKED)) {
		    driver->setState(driver, CS_TRACKING);
		}
		update = TRUE;
	    }
	    driver->adevValid = TRUE;

	    resetIntPermanentAdev(&driver->_adev);
	}

	if(driver->state == CS_FREERUN) {
	    driver->setState(driver, CS_TRACKING);
	    update = TRUE;
	}

	if((driver->state == CS_HOLDOVER) && (driver->age.seconds >= driver->config.holdoverAge)) {
	    driver->setState(driver, CS_TRACKING);
	    update = TRUE;
	}

	if((driver->state == CS_LOCKED) && driver->servo.runningMaxOutput) {
	    driver->setState(driver, CS_TRACKING);
	    update = TRUE;
	}

	if((driver->state == CS_NEGSTEP) && !isTimeNegative(&driver->refOffset)) {
	    driver->lockedUp = FALSE;
	    driver->setState(driver, CS_FREERUN);
	    update = TRUE;
	}

	DBG(THIS_COMPONENT"clock %s  total ADEV %.09f\n", driver->name, driver->totalAdev);

	getSystemClock()->getTimeMonotonic(getSystemClock(), &driver->_lastSync);

	driver->touchClock(driver);

	if(update) {
	    updateClockDrivers();
	}

}

static void
setReference(ClockDriver *a, ClockDriver *b) {

    if(a == NULL) {
	return;
    }

    if(a->config.disabled) {
	return;
    }


    if( (b != NULL) && a->config.externalOnly) {
	DBG("Clock %s only accepts external reference clocks\n", a->name);
	return;
    }

    if(a == b) {
	ERROR(THIS_COMPONENT"Cannot set reference of clock %s to self\n", a->name);
	return;
    }

    /* loop detection - do not allow a reference to use
     * a reference which uses itself, directly or indirectly
     */
    if(b != NULL) {
	ClockDriver *cd;
	int hops = 0;
	for( cd = b->refClock; cd != NULL; cd = cd->refClock) {
	    hops++;
	    if(cd == a) {
		NOTICE(THIS_COMPONENT"Cannot set reference of clock %s to %s: %s already references %s (%d hops)\n",
		    a->name, b->name, a->name, b->name, hops);
		return;
	    }
	}
    }

    /* no change */
    if((a->refClock != NULL) && (a->refClock == b)) {
	return;
    }

    if(b == NULL && a->refClock != NULL) {
	NOTICE(THIS_COMPONENT"Clock %s lost reference %s\n", a->name, a->refClock->name);
	a->refClock = NULL;
	memset(a->refName, 0, CLOCKDRIVER_NAME_MAX);
	if(a->state == CS_LOCKED) {
	    a->setState(a, CS_HOLDOVER);
	} else {
	    a->distance = 255;
	}
	a->refClass = -1;
	return;
	
    }

    if(b == NULL && a->externalReference) {
	NOTICE(THIS_COMPONENT"Clock %s lost external reference %s\n", a->name, a->refName);
	a->externalReference = FALSE;
	memset(a->refName, 0, CLOCKDRIVER_NAME_MAX);
	a->refClock = NULL;
	if(a->state == CS_LOCKED) {
	    a->setState(a, CS_HOLDOVER);
	} else {
	    a->distance = 255;
	}
	a->refClass = -1;
	return;
    } else if (b != NULL) {
	NOTICE(THIS_COMPONENT"Clock %s changing reference to %s\n", a->name, b->name);
	a->externalReference = FALSE;
	a->refClock = b;
	a->distance = b->distance + 1;
	strncpy(a->refName, b->name, CLOCKDRIVER_NAME_MAX);
	a->setState(a, CS_FREERUN);
    }


// WOJ:CHECK

/*



    if(a->systemClock && (b!= NULL) && !b->systemClock) {
	a->servo.kI = b->servo.kI;
	a->servo.kP = b->servo.kP;
    }
*/

}

static void setExternalReference(ClockDriver *a, const char* refName, int refClass) {

	if(a == NULL) {
	    return;
	}

	if(a->config.disabled) {
	    return;
	}

	if( a->config.internalOnly) {
	    DBG("Clock %s only accepts internal reference clocks\n", a->name);
	    return;
	}

	if(!a->externalReference || strncmp(a->refName, refName, CLOCKDRIVER_NAME_MAX)) {
	    NOTICE(THIS_COMPONENT"Clock %s changing to external reference %s\n", a->name, refName);
	    a->setState(a, CS_FREERUN);
	}

	strncpy(a->refName, refName, CLOCKDRIVER_NAME_MAX);
	a->externalReference = TRUE;
	a->refClass = refClass;
	a->refClock = NULL;
	a->distance = 1;

}

static void
restoreFrequency (ClockDriver *driver) {

    double frequency = 0;
    char frequencyPath [PATH_MAX + 1];
    memset(frequencyPath, 0, PATH_MAX + 1);

    if(driver->config.disabled) {
	return;
    }

    if(driver->config.storeToFile) {
	snprintf(frequencyPath, PATH_MAX, "%s/%s", driver->config.frequencyDir, driver->config.frequencyFile);
	if(!doubleFromFile(frequencyPath, &frequency)) {
	    frequency = driver->getFrequency(driver);
	}
    }

    /* goddamn floats! */
    if(fabs(frequency) <= ZEROF) {
	frequency = driver->getFrequency(driver);
    }

    frequency = clampDouble(frequency, driver->maxFrequency);
    driver->servo.prime(&driver->servo, frequency);
    driver->storedFrequency = driver->servo.output;
    driver->setFrequency(driver, driver->storedFrequency, 1.0);

}

static void
storeFrequency (ClockDriver *driver) {

    char frequencyPath [PATH_MAX + 1];
    memset(frequencyPath, 0, PATH_MAX + 1);

    if(driver->config.disabled) {
	return;
    }

    if(driver->config.storeToFile) {
	snprintf(frequencyPath, PATH_MAX, "%s/%s", driver->config.frequencyDir, driver->config.frequencyFile);
	doubleToFile(frequencyPath, driver->lastFrequency);
    }

    driver->storedFrequency = driver->lastFrequency;

}

static Boolean
adjustFrequency (ClockDriver *driver, double adj, double tau) {

	if(driver->config.disabled) {
	    return FALSE;
	}

	Boolean ret = driver->setFrequency(driver, adj, tau);
	driver->processUpdate(driver);
	return ret;

}

/* mark an update */
static void
touchClock(ClockDriver *driver) {

	if(driver->config.disabled) {
	    return;
	}

	TimeInternal now;
	getSystemClock()->getTimeMonotonic(getSystemClock(), &now);
	subTime(&driver->age, &now, &driver->_lastUpdate);
	getSystemClock()->getTimeMonotonic(getSystemClock(), &driver->_lastUpdate);
	driver->_updated = TRUE;

}

static Boolean disciplineClock(ClockDriver *driver, TimeInternal offset, double tau) {

    if(driver->config.disabled) {
	return FALSE;
    }

    driver->refOffset = offset;

    char buf[100];
    memset(buf, 0, sizeof(buf));
    snprint_TimeInternal(buf, sizeof(buf), &driver->refOffset);

    /* do nothing if offset is zero - prevents from linked clocks being dragged around,
     * and it's not obvious that two clocks can be linked, as we can see with Intel,
     * where multiple NIC ports can present themselves as different clock IDs, but
     * in fact all show the same frequency (or so it seems...)
     */

    if(isTimeZero(&driver->refOffset)) {
	driver->lastFrequency = driver->getFrequency(driver);
	driver->processUpdate(driver);
	return TRUE;
    }

    if(!driver->config.readOnly) {

	/* forced step on first update */
	if((driver->config.stepType == CSTEP_STARTUP_FORCE) && !driver->_updated && !driver->_stepped && !driver->lockedUp) {
	    return driver->stepTime(driver, &offset, FALSE);
	}

	if(offset.seconds) {

		int sign = (offset.seconds < 0) ? -1 : 1;

		/* step on first update */
		if((driver->config.stepType == CSTEP_STARTUP) && !driver->_updated && !driver->_stepped && !driver->lockedUp) {
		    return driver->stepTime(driver, &offset, TRUE);
		}

		/* panic mode */
		if(driver->state == CS_STEP) {
			return FALSE;
		}

		/* we refused to step backwards and offset is still negative */
		if((sign == -1) && driver->state == CS_NEGSTEP) {
			return FALSE;
		}

		/* going into panic mode */
		if(driver->config.stepTimeout) {
			/* ...or in fact exiting it... */
			if(driver->_canResume) {
/* WOJ:CHECK */
/* we're assuming that we can actually step! */

			} else {
			    WARNING(THIS_COMPONENT"Clock %s offset above 1 second (%s s), suspending clock control for %d seconds (panic mode)\n",
				    driver->name, buf, driver->config.stepTimeout);
			    driver->setState(driver, CS_STEP);
			    return FALSE;
			}
		}

		/* we're not allowed to step the clock */
		if(driver->config.noStep) {
			if(!driver->_warningTimeout) {
			    driver->_warningTimeout = CLOCKDRIVER_WARNING_TIMEOUT;
			    WARNING(THIS_COMPONENT"Clock %s offset above 1 second (%s s) and cannot step clock, slewing clock at maximum rate (%d us/s)",
				driver->name, buf, (sign * driver->servo.maxOutput) / 1000);
			}
			driver->servo.prime(&driver->servo, sign * driver->servo.maxOutput);
			driver->_canResume = FALSE;
			return driver->adjustFrequency(driver, sign * driver->servo.maxOutput, tau);
		}

		WARNING(THIS_COMPONENT"Clock %s offset above 1 second (%s s), attempting to step the clock\n", driver->name, buf);
		if( driver->stepTime(driver, &offset, FALSE) ) {
		    driver->_canResume = FALSE;
		    clearTime(&driver->refOffset);
		    return TRUE;
		} else {
		    return FALSE;
		}

	} else {
		if(driver->state == CS_STEP) {
		    /* we are outside the exit threshold, clock is still suspended */
		    if(driver->config.stepExitThreshold && (labs(offset.nanoseconds) > driver->config.stepExitThreshold)) {
			return FALSE;
		    }
		    NOTICE(THIS_COMPONENT"Clock %s offset below 1 second, resuming clock control\n", driver->name);
		    driver->setState(driver, CS_FREERUN);
		}

		if(driver->state == CS_NEGSTEP) {
		    driver->lockedUp = FALSE;
		    driver->setState(driver, CS_FREERUN);
		}

		if(driver->externalReference) {
		    driver->servo.feed(&driver->servo, offset.nanoseconds, tau);
		    return driver->adjustFrequency(driver, driver->servo.output, tau);
		} else if(feedDoubleMovingStatFilter(driver->_filter, timeInternalToDouble(&offset))) {
		    offset = doubleToTimeInternal(driver->_filter->output);
		    driver->refOffset = offset;
		    driver->servo.feed(&driver->servo, offset.nanoseconds, tau);
		    return driver->adjustFrequency(driver, driver->servo.output, tau);
		}

	}

    }

    return FALSE;

}

static Boolean
syncClock(ClockDriver* driver, double tau) {

	if(driver->config.disabled) {
	    return FALSE;
	}

	TimeInternal delta;

	if(driver->externalReference || (driver->refClock == NULL)) {
	    return FALSE;
	}

	if(!driver->getOffsetFrom(driver, driver->refClock, &delta)) {
	    return FALSE;
	}

	driver->refOffset = delta;

	return disciplineClock(driver, delta, tau);

}

static Boolean
syncClockExternal(ClockDriver* driver, TimeInternal offset, double tau) {

    if(driver->config.disabled) {
	return FALSE;
    }

    if(!driver->externalReference) {
	return FALSE;
    }

    return disciplineClock(driver, offset, tau);

}

const char*
getClockDriverName(int type) {

    if ((type < 0) || (type >= CLOCKDRIVER_MAX)) {
	return NULL;
    }

    return clockDriverNames[type];

}

int
getClockDriverType(const char* name) {

    for(int i = 0; i < CLOCKDRIVER_MAX; i++) {

	if(!strcmp(name, clockDriverNames[i])) {
	    return i;
	}

    }

    return -1;

}

Boolean
parseClockDriverSpec(const char* line, ClockDriverSpec *spec) {

	memset(spec, 0, sizeof(ClockDriverSpec));
	spec->type = -1;

	foreach_token_begin(params, line, param, ":");
		switch(counter_params) {
		    case 0:
			spec->type = getClockDriverType(param);
			if(spec->type == -1) {
			    ERROR(THIS_COMPONENT"Clock driver string \"%s\" : unknown clock driver type: %s\n", line, param);
			}
			break;
		    case 1:
			strncpy(spec->path, param, PATH_MAX);
			break;
		    case 2:
			strncpy(spec->name, param, CLOCKDRIVER_NAME_MAX);
			break;
		    default:
			break;
		}
	foreach_token_end(params);

	if(counter_params < 1) {
	    ERROR(THIS_COMPONENT"Clock driver string: \"%s\": no parameters given\n", line);
	    return FALSE;
	}

	if(spec->type == -1) {
	    return FALSE;
	}

	return TRUE;

}

const char*
getClockStateName(ClockState state) {

    switch(state) {
	case CS_SUSPENDED:
	    return "SUSPENDED";
	case CS_HWFAULT:
	    return "HWFAULT";
	case CS_NEGSTEP:
	    return "NEGSTEP";
	case CS_STEP:
	    return "STEP";
	case CS_INIT:
	    return "INIT";
	case CS_FREERUN:
	    return "FREERUN";
	case CS_LOCKED:
	    return "LOCKED";
	case CS_TRACKING:
	    return "TRACKING";
	case CS_HOLDOVER:
	    return "HOLDOVER";
	default:
	    return "UNKNOWN";
    }

}

const char*
getClockStateShortName(ClockState state) {

    switch(state) {
	case CS_SUSPENDED:
	    return "SUSP";
	case CS_HWFAULT:
	    return "HWFL";
	case CS_NEGSTEP:
	    return "NSTP";
	case CS_STEP:
	    return "STEP";
	case CS_INIT:
	    return "INIT";
	case CS_FREERUN:
	    return "FREE";
	case CS_LOCKED:
	    return "LOCK";
	case CS_TRACKING:
	    return "TRCK";
	case CS_HOLDOVER:
	    return "HOLD";
	default:
	    return "UNKN";
    }

}

ClockDriver*
findClockDriver(const char * search) {

	ClockDriver *cd;
	LINKED_LIST_FOREACH(cd) {
	    if(cd->isThisMe(cd, search)) {
		return cd;
	    }
	}

	return NULL;

}

ClockDriver*
getClockDriverByName(const char * search) {

	ClockDriver *cd;

	LINKED_LIST_FOREACH(cd) {
	    if(!strncmp(cd->name, search, CLOCKDRIVER_NAME_MAX)) {
		return cd;
	    }
	}

	return NULL;

}

static Boolean
pushConfig(ClockDriver *driver, RunTimeOpts *global)
{

    Boolean ret = TRUE;

    _updateInterval = global->clockUpdateInterval;

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

    config->disabled = (token_in_list(global->disabledClocks, driver->name, DEFAULT_TOKEN_DELIM)) ? TRUE:FALSE;
    config->excluded = (token_in_list(global->excludedClocks, driver->name, DEFAULT_TOKEN_DELIM)) ? TRUE:FALSE;

    if(global->noAdjust) {
	config->readOnly = TRUE;
    } else {
	config->readOnly = (token_in_list(global->readOnlyClocks, driver->name, DEFAULT_TOKEN_DELIM)) ? TRUE:FALSE;
    }

    if(config->required && config->disabled) {
	if(!config->readOnly) {
	    WARNING(THIS_COMPONENT"clock %s cannot be disabled - set to read-only to exclude from sync\n", driver->name);
	} else {
	    WARNING(THIS_COMPONENT"clock %s cannot be disabled - already set to read-only\n", driver->name);
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
    config->stepExitThreshold = global->panicModeExitThreshold;

    driver->servo.kP = global->servoKP;
    driver->servo.kI = global->servoKI;

    driver->servo.maxOutput = global->servoMaxPpb;
    driver->servo.tauMethod = global->servoDtMethod;
    driver->servo.maxTau = global->servoMaxdT;

    strncpy(config->frequencyDir, global->frequencyDir, PATH_MAX);

    ret = ret && (driver->pushPrivateConfig(driver, global));

    DBG(THIS_COMPONENT"Clock driver %s configured\n", driver->name);

    return ret;

}

void
controlClockDrivers(int command) {

    ClockDriver *cd;
    Boolean found;

    switch(command) {

	case CD_NOTINUSE:
	    LINKED_LIST_FOREACH(cd) {
		if(!cd->systemClock) {
		    cd->inUse = FALSE;
		    cd->config.required = FALSE;
		}
	    }
	break;

	case CD_SHUTDOWN:
	    LINKED_LIST_DESTROYALL(cd, freeClockDriver);
	    _systemClock = NULL;
	break;

	case CD_CLEANUP:
	    do {
		found = FALSE;
		LINKED_LIST_FOREACH(cd) {
		    /* the system clock should always be kept */
		    if(!cd->inUse && !cd->systemClock) {
			freeClockDriver(&cd);
			found = TRUE;
			break;
		    }
		}
	    } while(found);
	break;

	case CD_DUMP:
	    LINKED_LIST_FOREACH(cd) {
		if(cd->config.disabled) {
		    continue;
		}
		INFO(THIS_COMPONENT"Clock driver %s state %s\n", cd->name, getClockStateName(cd->state));
	    }
	break;

	default:
	    ERROR(THIS_COMPONENT"Unnown clock driver command %02d\n", command);

    }

}

ClockDriver*
compareClockDriver(ClockDriver *a, ClockDriver *b) {

	/* if one is null, the other wins, if both are null, no winner */
	if((a != NULL) && (b == NULL) ) {
	    return a;
	}

	if((a == NULL) && (b != NULL) ) {
	    return b;
	}

	if((a == NULL) && (b == NULL)) {
	    return NULL;
	}

	/* if one is disabled, the other wins, if both are, no winner */
	if((!a->config.disabled) && (b->config.disabled) ) {
	    return a;
	}

	if((!b->config.disabled) && (a->config.disabled) ) {
	    return b;
	}

	if((a->config.disabled) && (b->config.disabled) ) {
	    return NULL;
	}

	/* if one is excluded, the other wins, if both are, no winner */
	if((!a->config.excluded) && (b->config.excluded) ) {
	    return a;
	}

	if((!b->config.excluded) && (a->config.excluded) ) {
	    return b;
	}

	if((a->config.excluded) && (b->config.excluded) ) {
	    return NULL;
	}

	/* better state than us - wins */
	if( (a->state < b->state) && (b->state > CS_FREERUN)) {
		/* however, do not prefer a locked with higher distance
		 * to a holdover with lower distance */
		if((a->state == CS_HOLDOVER) && (b->state == CS_LOCKED)) {
		    if(a->distance < b->distance) {
			return a;
		    }
		}
		    return b;
	}

	/* our state is better - we win */
	if(b->state < a->state && (a->state > CS_FREERUN) ) {
		/* however, do not prefer a locked with higher distance
		 * to a holdover with lower distance */
		if((b->state == CS_HOLDOVER) && (a->state == CS_LOCKED)) {
		    if(b->distance < a->distance) {
			return b;
		    }
		}
		return a;
	}

	/* same state */
	if(a->state == b->state) {

		if(a->state == CS_LOCKED || a->state == CS_HOLDOVER) {

		    /* external reference is better */
		    if(a->externalReference && !b->externalReference) {
			return a;
		    }

		    if(!a->externalReference && b->externalReference) {
			return b;
		    }

		    /* lower reference class is better */
		    if(a->externalReference && b->externalReference) {
			if(a->refClass < b-> refClass) {
			    return a;
			}
			if(b->refClass < a-> refClass) {
			    return b;
			}
		    }

		    /* clock referencing best clock is better than one with no reference or referencing non-best */
		    if((a->refClock && (a->refClock==_bestClock)) &&
			(!b->refClock || (b->refClock != _bestClock))) {
			return a;
		    }

		    if((b->refClock && (b->refClock==_bestClock)) &&
			(!a->refClock || (a->refClock != _bestClock))) {
			return b;
		    }

		    /* clock referencing the system clock is worse */
		    if(a->refClock && b->refClock) {
			if(!a->refClock->systemClock && b->refClock->systemClock) {
			    return a;
			}
			if(a->refClock->systemClock && !b->refClock->systemClock) {
			    return b;
			}
		    }

		    /* tiebreaker 1: lower reference chain hop count wins */
		    if(a->distance < b->distance) {
			return a;
		    }

		    if(a->distance > b->distance) {
			return b;
		    }

		    /* tiebreaker 2: system clock loses */
		    if(!a->systemClock && b->systemClock) {
			return a;
		    }
		    if(a->systemClock && !b->systemClock) {
			return b;
		    }

		    /* tiebreaker 3: lower allan dev wins */
		    if((a->adev > ZEROF) && (b->adev > ZEROF)) {

			if(a->adev < b->adev) {
			    return a;
			}

			if(a->adev > b->adev) {
			    return b;
			}
		    }

		    /* final tiebreaker - clock longer in state wins */
		    if(a->age.seconds > b->age.seconds) {
			return a;
		    }

		    if(a->age.seconds < b->age.seconds) {
			return b;
		    }

		}

	}
	/* erm... what would John Eidson do? */
	return a;
}

static void
findBestClock() {

    ClockDriver *newBest = NULL;
    ClockDriver *cd;

    LINKED_LIST_FOREACH(cd) {
	if((cd->config.disabled) || (cd->config.excluded)) {
	    continue;
	}
	if(cd->state == CS_LOCKED) {
	    newBest = cd;
	    break;
	}
    }

    if(newBest == NULL) {
	LINKED_LIST_FOREACH(cd) {
	    if((cd->config.disabled) || (cd->config.excluded)) {
		continue;
	    }
	    if(cd->state == CS_HOLDOVER) {
		newBest = cd;
		break;
	    }
	}
    }

    if(newBest != NULL) {
	LINKED_LIST_FOREACH(cd) {
	    if((cd->config.disabled) || (cd->state == CS_HWFAULT) || (cd->config.excluded)) {
		continue;
	    }
	    if(newBest != cd) {
		    newBest = compareClockDriver(newBest, cd);
	    }
    }

	if(newBest != _bestClock ) {
	    NOTICE(THIS_COMPONENT"New best clock selected: %s\n", newBest->name);
	}

    } else {

	if(newBest != _bestClock ) {
	    NOTICE(THIS_COMPONENT"No best clock available\n");
	}

    }

    if(newBest != _bestClock) {
	if(_bestClock != NULL) {
	    _bestClock->bestClock = FALSE;
	    /* we are changing best reference - drop the old one */
	    LINKED_LIST_FOREACH(cd) {
		if(cd->config.disabled) {
		    continue;
		}
		if(!cd->externalReference && (cd->refClock == _bestClock)
		    && (cd->state != CS_LOCKED) && (cd->state != CS_HOLDOVER)) {
		    cd->setReference(cd, NULL);
		}
	    }
	}
	_bestClock = newBest;
	if(_bestClock != NULL) {
	    _bestClock->bestClock = TRUE;
	}

	LINKED_LIST_FOREACH(cd) {
	    if(cd->config.disabled) {
		continue;
	    }
	    if(!cd->externalReference && (cd != _bestClock)) {
		cd->setReference(cd, NULL);
	    }
	}

	LINKED_LIST_FOREACH(cd) {
	    if(cd->config.disabled) {
		continue;
	    }
	    if(!cd->externalReference && (cd != _bestClock)) {
		cd->setReference(cd, _bestClock);
	    }
	}
    }

}

static void
putStatsLine(ClockDriver* driver, char* buf, int len) {

    char tmpBuf[100];
    memset(tmpBuf, 0, sizeof(tmpBuf));
    memset(buf, 0, len);
    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), &driver->refOffset);

    if(driver->config.disabled) {
	snprintf(buf, len - 1, "disabled");
    } else {
	snprintf(buf, len - 1, "%s%soffs: %-13s  adev: %-8.3f freq: %.03f", driver->config.readOnly ? "r" : " ",
	    driver->bestClock ? "*" : driver->state <= CS_INIT ? "!" : driver->config.excluded ? "-" : " ",
	    tmpBuf, driver->adev, driver->lastFrequency);
    }
}

static void
putInfoLine(ClockDriver* driver, char* buf, int len) {

    char tmpBuf[100];
    char tmpBuf2[30];
    memset(tmpBuf, 0, sizeof(tmpBuf2));
    memset(tmpBuf, 0, sizeof(tmpBuf));
    memset(buf, 0, len);
    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), &driver->refOffset);

    if((driver->state == CS_STEP) && driver->config.stepTimeout) {
	snprintf(tmpBuf2, sizeof(tmpBuf2), "%s %-4d", getClockStateShortName(driver->state), driver->config.stepTimeout - driver->age.seconds);
    } else if((driver->state == CS_HWFAULT) && driver->config.failureDelay) {
	snprintf(tmpBuf2, sizeof(tmpBuf2), "%s %-4d", getClockStateShortName(driver->state), driver->config.failureDelay - driver->age.seconds);
    } else {
	strncpy(tmpBuf2, getClockStateName(driver->state), CLOCKDRIVER_NAME_MAX);
    }

    if(driver->config.disabled) {
	snprintf(buf, len - 1, "disabled");
    } else {
	snprintf(buf, len - 1, "%s%sname:  %-12s state: %-9s ref: %-7s", driver->config.readOnly ? "r" : " ",
		driver->bestClock ? "*" : driver->state <= CS_INIT ? "!" : driver->config.excluded ? "-" : " ",
	    driver->name, tmpBuf2, strlen(driver->refName) ? driver->refName : "none");
    }
}

static Boolean healthCheck(ClockDriver *driver) {

    Boolean ret = TRUE;

    if(driver == NULL) {
	return FALSE;
    }

    DBG(THIS_COMPONENT"clock %s health check...\n", driver->name);

    ret &= driver->privateHealthCheck(driver);

    return ret;
}
