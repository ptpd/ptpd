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

#define REGISTER_CLOCKDRIVER(type, suffix) \
	if(driverType==type) { \
	    _setupClockDriver_##suffix(clockDriver);\
	    found = TRUE;\
	}

/* linked list - so that we can control all registered objects centrally */
LINKED_LIST_HOOK(ClockDriver);


static ClockDriver* _systemClock = NULL;

/* inherited methods */

static void setState(ClockDriver *, ClockState);
static void processUpdate(ClockDriver *, double, double);
static Boolean pushConfig(ClockDriver *, RunTimeOpts *);

/* inherited methods end */

ClockDriver *
createClockDriver(int driverType, const char *name)
{

    ClockDriver *clockDriver = NULL;

    if(getClockDriverByName(name) != NULL) {
	ERROR("Cannot create clock driver %s: clock driver with this name already exists\n", name);
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
	ERROR("Setup requested for unknown clock driver type: %d\n", driverType);
    } else {
	clockDriver->getTimeMonotonic(clockDriver, &clockDriver->_initTime);
	DBG("Created new clock driver type %d name %s serial %d\n", driverType, name, clockDriver->_serial);
    }

    /* inherited methods */

    clockDriver->setState = setState;
    clockDriver->processUpdate = processUpdate;
    clockDriver->pushConfig = pushConfig;

    /* inherited methods end */

    clockDriver->state = CS_INIT;

    return found;

}

void
freeClockDriver(ClockDriver** clockDriver)
{

    if(*clockDriver == NULL) {
	return;
    }
    ClockDriver *pdriver = *clockDriver;

    pdriver->shutdown(pdriver);

	/* maintain the linked list */
	LINKED_LIST_REMOVE(pdriver);


    if(pdriver->privateData != NULL) {
	free(pdriver->privateData);
    }

    if(pdriver->privateConfig != NULL) {
	free(pdriver->privateConfig);
    }

    DBG("Deleted clock driver type %d name %s serial %d\n", pdriver->type, pdriver->name, pdriver->_serial);

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
	CRITICAL("Could not start system clock driver, cannot continue\n");
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

	if(driver->state != newState) {
	    NOTICE("Clock %s changed state from %s to %s\n",
		    driver->name, getClockStateName(driver->state),
		    getClockStateName(newState));
	    driver->state = newState;
	}

}

static void
processUpdate(ClockDriver *driver, double adj, double tau) {

	TimeInternal now;

	driver->_tau = tau;
	driver->adev = feedIntPermanentAdev(&driver->_adev, adj);
	driver->totalAdev = feedIntPermanentAdev(&driver->_totalAdev, adj);

	if((driver->_adev.count * tau) > 10) {
	    INFO("%s  ADEV %.09f\n", driver->name, driver->adev);
	    resetIntPermanentAdev(&driver->_adev);
	}

	driver->lastFrequency = adj;

	getSystemClock()->getTimeMonotonic(getSystemClock(), &now);
	if(driver->_updated) {
	    subTime(&driver->age, &now, &driver->_lastSync);
	} else {
	    subTime(&driver->age, &now, &driver->_initTime);
	}
	getSystemClock()->getTimeMonotonic(getSystemClock(), &driver->_lastSync);
	driver->_updated = TRUE;

}

const char*
getClockStateName(ClockState state) {

    switch(state) {
	case CS_INIT:
	    return "INIT";
	case CS_FREERUN:
	    return "FREERUN";
	case CS_LOCKED:
	    return "LOCKED";
	case CS_UNLOCKED:
	    return "UNLOCKED";
	case CS_HOLDOVER:
	    return "HOLDOVER";
	default:
	    return "UNKNOWN";
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

    ClockDriverConfig *config = &driver->config;

    config->readOnly = global->noAdjust;
    config->noStep   = global->noResetClock;
    config->negativeStep = global->negativeStep;
    config->saveFrequency = global->saveFrequency;
    config->adevPeriod = global->adevPeriod;
    config->stableAdev = global->stableAdev;
    config->unstableAdev = global->unstableAdev;
    config->holdoverAge = global->holdoverAge;
    config->freerunAge = global->freerunAge;

    strncpy(config->frequencyDir, global->frequencyDir, PATH_MAX);

    return( ret && (driver->pushPrivateConfig(driver, global)) );

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
		INFO("Clock driver %s state %s\n", cd->name, getClockStateName(cd->state));
	    }
	break;

	default:
	    ERROR("Unnown clock driver command %02d\n", command);

    }

}
