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
	    _setupClockDriver_##suffix(clockDriver);\
	    found = TRUE;\
	}

/* linked list - so that we can control all registered objects centrally */
LINKED_LIST_HOOK(ClockDriver);


static ClockDriver* _systemClock = NULL;
static ClockDriver* _bestClock = NULL;
static int _updateInterval = CLOCKDRIVER_UPDATE_INTERVAL;
static int _syncInterval = 1.0 / (CLOCK_SYNC_RATE + 0.0);

/* inherited methods */

static void setState(ClockDriver *, ClockState);
static void processUpdate(ClockDriver *);
static void touchClock(ClockDriver *driver);
static Boolean pushConfig(ClockDriver *, RunTimeOpts *);
static void setReference(ClockDriver *, ClockDriver *);
static void setExternalReference(ClockDriver *, const char*);
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
	freeClockDriver(&clockDriver);
    } else {
	/* maintain the linked list */
	LINKED_LIST_INSERT(clockDriver);
    }

    return clockDriver;

}

Boolean
setupClockDriver(ClockDriver* clockDriver, int driverType, const char *name)
{

    Boolean found = FALSE;

    clockDriver->type = driverType;
    strncpy(clockDriver->name, name, CLOCKDRIVER_NAME_MAX);

    REGISTER_CLOCKDRIVER(CLOCKDRIVER_UNIX, unix);
    REGISTER_CLOCKDRIVER(CLOCKDRIVER_LINUXPHC, linuxphc);

    if(!found) {
	ERROR(THIS_COMPONENT"Setup requested for unknown clock driver type: %d\n", driverType);
    } else {
	clockDriver->getTimeMonotonic(clockDriver, &clockDriver->_initTime);
	clockDriver->getTimeMonotonic(clockDriver, &clockDriver->_lastUpdate);
	DBG(THIS_COMPONENT"Created new clock driver type %d name %s serial %d\n", driverType, name, clockDriver->_serial);
    }

    /* inherited methods */

    clockDriver->setState = setState;
    clockDriver->processUpdate = processUpdate;
    clockDriver->pushConfig = pushConfig;

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

    pdriver->shutdown(pdriver);

	/* maintain the linked list */
	LINKED_LIST_REMOVE(pdriver);


    if(pdriver->privateData != NULL) {
	free(pdriver->privateData);
    }

    if(pdriver->privateConfig != NULL) {
	free(pdriver->privateConfig);
    }

    DBG(THIS_COMPONENT"Deleted clock driver type %d name %s serial %d\n", pdriver->type, pdriver->name, pdriver->_serial);

    if(pdriver == _systemClock) {
	_systemClock = NULL;
    }

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

    return _systemClock;

}

void
shutdownClockDrivers() {

	ClockDriver *cd;
	LINKED_LIST_DESTROYALL(cd, freeClockDriver);
	_systemClock = NULL;

}

static void
setState(ClockDriver *driver, ClockState newState) {

/* todo: switch/case FSM as we get more conditions */

	if(driver->state != newState) {
	    NOTICE(THIS_COMPONENT"Clock %s changed state from %s to %s\n",
		    driver->name, getClockStateName(driver->state),
		    getClockStateName(newState));
	    getSystemClock()->getTimeMonotonic(getSystemClock(), &driver->_lastUpdate);
	    clearTime(&driver->age);
	    /* if we're going into FREERUN, but not from a "good" state, restore last good frequency */
	    if( (newState == CS_FREERUN) && (driver->state == CS_LOCKED || driver->state == CS_HOLDOVER)) {
		driver->restoreFrequency(driver);
	    }

	    driver->lastState = driver->state;
	    driver->state = newState;

	findBestClock();
	syncClocks(_syncInterval);

	}

}

void
updateClockDrivers() {

	ClockDriver *cd;

	TimeInternal now;

	LINKED_LIST_FOREACH(cd) {

	    if(cd->_warningTimeout > 0) {
		cd->_warningTimeout -= _updateInterval;
	    }

	    getSystemClock()->getTimeMonotonic(getSystemClock(), &now);
	    subTime(&cd->age, &now, &cd->_lastUpdate);

	    DBGV(THIS_COMPONENT"Clock %s age %d.%d\n", cd->name, cd->age.seconds, cd->age.nanoseconds);

	    switch(cd->state) {

		case CS_INIT:
		    break;
		case CS_SUSPENDED:
		    if(cd->age.seconds > cd->config.suspendTimeout) {
			WARNING(THIS_COMPONENT"Clock %s suspension delay timeout, resuming clock updates\n", cd->name);
			cd->setState(cd, CS_FREERUN);
			cd->_canResume = TRUE;
		    }
		    break;
		case CS_NSTEP:
		    break;
		case CS_FREERUN:
		    break;
		case CS_LOCKED:
			if((cd->refClock == NULL) && (!cd->externalReference)) {
			    cd->setState(cd, CS_HOLDOVER);
			    resetIntPermanentAdev(&cd->_adev);
			    break;
			}
			if(cd->age.seconds > cd->config.lockedAge) {
			    cd->setState(cd, CS_HOLDOVER);
			}
			resetIntPermanentAdev(&cd->_adev);
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
    LINKED_LIST_FOREACH(cd) {
	cd->syncClock(cd, tau);
    }

}

void
stepClocks(Boolean force) {

    ClockDriver *cd;
    LINKED_LIST_FOREACH(cd) {
	cd->stepTime(cd, &cd->refOffset, force);
    }

}

void
reconfigureClockDrivers(RunTimeOpts *rtOpts) {

    ClockDriver *cd;
    LINKED_LIST_FOREACH(cd) {
	cd->pushConfig(cd, rtOpts);
    }

}

static void
processUpdate(ClockDriver *driver) {

	Boolean update = FALSE;

	feedIntPermanentAdev(&driver->_adev, driver->lastFrequency);
	driver->totalAdev = feedIntPermanentAdev(&driver->_totalAdev, driver->lastFrequency);

	if(driver->servo.runningMaxOutput) {
	    resetIntPermanentAdev(&driver->_adev);
	}

	/* we have enough allan dev samples to represent adev period */
	if( (driver->_adev.count * driver->_tau) > driver->config.adevPeriod ) {
	    driver->adev = driver->_adev.adev;
	    DBG(THIS_COMPONENT"clock %s  ADEV %.09f\n", driver->name, driver->adev);
	    if(driver->servo.runningMaxOutput) {
		driver->setState(driver, CS_TRACKING);
	    }
	    if(driver->adev <= driver->config.stableAdev) {
		driver->storeFrequency(driver);
		driver->setState(driver, CS_LOCKED);
	    }
	    if((driver->adev >= driver->config.unstableAdev) && (driver->state == CS_LOCKED)) {
		driver->setState(driver, CS_TRACKING);
	    }
	    update = TRUE;
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

	if((driver->state == CS_NSTEP) && !isTimeNegative(&driver->refOffset)) {
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

    if(a == b) {
	ERROR(THIS_COMPONENT"Cannot set reference of clock %s to self\n", a->name);
	return;
    }

    /* no change */
    if((a->refClock != NULL) && (a->refClock == b)) {
	return;
    }

    if(b == NULL && a->refClock != NULL) {
	NOTICE(THIS_COMPONENT"Clock %s lost reference %s\n", a->name, a->refClock->name);
	a->refClock = NULL;
	memset(a->refName, 0, CLOCKDRIVER_NAME_MAX);
	clearTime(&a->refOffset);
	return;
	
    }

    if(b == NULL && a->externalReference) {
	NOTICE(THIS_COMPONENT"Clock %s lost external reference %s\n", a->name, a->refName);
	a->externalReference = FALSE;
	memset(a->refName, 0, CLOCKDRIVER_NAME_MAX);
	a->refClock = NULL;
	return;
    } else if (b != NULL) {
	NOTICE(THIS_COMPONENT"Clock %s changing reference to %s\n", a->name, b->name);
	a->externalReference = FALSE;
	a->refClock = b;
	strncpy(a->refName, b->name, CLOCKDRIVER_NAME_MAX);
	a->setState(a, CS_FREERUN);
    }

    if(a->systemClock && !b->systemClock) {
	a->servo.kI = b->servo.kI;
	a->servo.kP = b->servo.kP;
    }

}

static void setExternalReference(ClockDriver *a, const char* refName) {

	if(!a->externalReference || strncmp(a->refName, refName, CLOCKDRIVER_NAME_MAX)) {
	    NOTICE(THIS_COMPONENT"Clock %s changing to external reference %s\n", a->name, refName);
	}

	strncpy(a->refName, refName, CLOCKDRIVER_NAME_MAX);
	a->externalReference = TRUE;
	a->refClock = NULL;
	a->setState(a, CS_FREERUN);
}

static void
restoreFrequency (ClockDriver *driver) {

    double frequency = 0;
    char frequencyPath [PATH_MAX + 1];
    memset(frequencyPath, 0, PATH_MAX + 1);

    if(driver->config.storeToFile) {
	snprintf(frequencyPath, PATH_MAX, "%s/%s", driver->config.frequencyDir, driver->config.frequencyFile);
	if(!doubleFromFile(frequencyPath, &frequency)) {
	    frequency = driver->getFrequency(driver);
	}
    }

    if(frequency == 0) {
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

    if(driver->config.storeToFile) {
	snprintf(frequencyPath, PATH_MAX, "%s/%s", driver->config.frequencyDir, driver->config.frequencyFile);
	doubleToFile(frequencyPath, driver->lastFrequency);
    }

    driver->storedFrequency = driver->lastFrequency;

}

static Boolean
adjustFrequency (ClockDriver *driver, double adj, double tau) {

	Boolean ret = driver->setFrequency(driver, adj, tau);
	driver->processUpdate(driver);
	return ret;

}

/* mark an update */
static void
touchClock(ClockDriver *driver) {

	TimeInternal now;
	getSystemClock()->getTimeMonotonic(getSystemClock(), &now);
	subTime(&driver->age, &now, &driver->_lastUpdate);
	getSystemClock()->getTimeMonotonic(getSystemClock(), &driver->_lastUpdate);
	driver->_updated = TRUE;

}

static Boolean disciplineClock(ClockDriver *driver, TimeInternal offset, double tau) {

    driver->refOffset = offset;

    char buf[100];
    memset(buf, 0, sizeof(buf));
    snprint_TimeInternal(buf, sizeof(buf), &driver->refOffset);

    /* do nothing if offset is zero - prevents from linked clocks being dragged around,
     * and it's not obvious that two clocks can be linked, as we can see with Intel,
     * where multiple NIC ports can present themselves as different clock IDs, but
     * in fact all drive the same clock.
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
		if(driver->state == CS_SUSPENDED) {
			return FALSE;
		}

		/* we refused to step backwards and offset is still negative */
		if((sign == -1) && driver->state == CS_NSTEP) {
			return FALSE;
		}

		/* going into panic mode */
		if(driver->config.suspendTimeout) {
			/* ...or in fact exiting it... */
			if(driver->_canResume) {
/* WOJ:CHECK */
/* we're assuming that we can actually step! */
			    driver->_canResume = FALSE;
			} else {
			    WARNING(THIS_COMPONENT"Clock %s offset above 1 second (%s s), suspending clock control for %d seconds (panic mode)\n",
				    driver->name, buf, driver->config.suspendTimeout);
			    driver->setState(driver, CS_SUSPENDED);
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
			return driver->adjustFrequency(driver, sign * driver->servo.maxOutput, tau);
		}

		WARNING(THIS_COMPONENT"Clock %s offset above 1 second (%s s), attempting to step the clock\n", driver->name, buf);
		return driver->stepTime(driver, &offset, FALSE);

	} else {
		if(driver->state == CS_SUSPENDED) {
		    /* we are outside the exit threshold, clock is still suspended */
		    if(driver->config.suspendExitThreshold && (labs(offset.nanoseconds) > driver->config.suspendExitThreshold)) {
			return FALSE;
		    }
		    NOTICE(THIS_COMPONENT"Clock %s offset below 1 second, resuming clock control\n", driver->name);
		    driver->setState(driver, CS_FREERUN);
		}

		if(driver->state == CS_NSTEP) {
		    driver->lockedUp = FALSE;
		    driver->setState(driver, CS_FREERUN);
		}

		driver->servo.feed(&driver->servo, offset.nanoseconds, tau);
		return driver->adjustFrequency(driver, driver->servo.output, tau);

	}

    }

    return FALSE;

}

static Boolean
syncClock(ClockDriver* driver, double tau) {

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

    if(!driver->externalReference) {
	return FALSE;
    }

    return disciplineClock(driver, offset, tau);

}

const char*
getClockStateName(ClockState state) {

    switch(state) {
	case CS_NSTEP:
	    return "FAULT";
	case CS_SUSPENDED:
	    return "SUSPENDED";
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
	case CS_NSTEP:
	    return "FLT";
	case CS_SUSPENDED:
	    return "SUSP";
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
findClockDriver(char * search) {

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

    config->readOnly = global->noAdjust;
    config->noStep = global->noResetClock;
    config->negativeStep = global->negativeStep;
    config->storeToFile = global->storeToFile;
    config->adevPeriod = global->adevPeriod;
    config->stableAdev = global->stableAdev;
    config->unstableAdev = global->unstableAdev;
    config->lockedAge = global->lockedAge;
    config->holdoverAge = global->holdoverAge;
    config->freerunAge = global->freerunAge;
    config->suspendTimeout = (global->enablePanicMode) ? 60 * global-> panicModeDuration : 0;
    config->suspendExitThreshold = global->panicModeExitThreshold;

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
		INFO(THIS_COMPONENT"Clock driver %s state %s\n", cd->name, getClockStateName(cd->state));
	    }
	break;

	default:
	    ERROR(THIS_COMPONENT"Unnown clock driver command %02d\n", command);

    }

}

ClockDriver*
compareClockDriver(ClockDriver *a, ClockDriver *b) {

	/* better state than us - wins */
	if( (b->state > a->state) && (b->state > CS_FREERUN)) {
		return b;
	}

	/* our state is better - we win */
	if(b->state < a->state) {
		return a;
	}

	/* same state */
	if(a->state == b->state) {

		if(a->state == CS_LOCKED || a->state == CS_HOLDOVER) {

		    if(a->externalReference && !b->externalReference) {
			return a;
		    }

		    if(!a->externalReference && b->externalReference) {
			return b;
		    }

		    if(a->adev < b->adev) {
			return a;
		    }

		    if(a->adev > b->adev) {
			return b;
		    }

		}

	}

	return a;
}

static void
findBestClock() {

    ClockDriver *newBest = NULL;
    ClockDriver *cd;

    LINKED_LIST_FOREACH(cd) {
	if(cd->state == CS_LOCKED) {
	    newBest = cd;
	    break;
	}
    }

    if(newBest == NULL) {
	LINKED_LIST_FOREACH(cd) {
	    if(cd->state == CS_HOLDOVER) {
		newBest = cd;
		break;
	    }
	}
    }

    if(newBest != NULL) {
	LINKED_LIST_FOREACH(cd) {
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
	}
	_bestClock = newBest;
	if(_bestClock != NULL) {
	    _bestClock->bestClock = TRUE;
	}
	LINKED_LIST_FOREACH(cd) {
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

    snprintf(buf, len - 1, " offs: %-13s  adev: %-8.3f freq: %.03f",
	    tmpBuf, driver->adev, driver->lastFrequency);

}

static void
putInfoLine(ClockDriver* driver, char* buf, int len) {

    char tmpBuf[100];
    char tmpBuf2[30];
    memset(tmpBuf, 0, sizeof(tmpBuf2));
    memset(tmpBuf, 0, sizeof(tmpBuf));
    memset(buf, 0, len);
    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), &driver->refOffset);

    if((driver->state == CS_SUSPENDED) && driver->config.suspendTimeout) {
	snprintf(tmpBuf2, sizeof(tmpBuf2), "SUSP %-4d", driver->config.suspendTimeout - driver->age.seconds);
    } else {
	strncpy(tmpBuf2, getClockStateName(driver->state), CLOCKDRIVER_NAME_MAX);
    }

    snprintf(buf, len - 1, "%sname:  %-12s state: %-9s ref: %-10s", driver->bestClock ? "*" : " ", driver->name,
	    tmpBuf2, strlen(driver->refName) ? driver->refName : "none");

}

void		reconfigureClockDrivers(RunTimeOpts *);