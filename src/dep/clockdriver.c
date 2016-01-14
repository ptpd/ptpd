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
static ClockDriver *_first = NULL;
static ClockDriver *_last = NULL;
static uint32_t _serial = 0;

static ClockDriver* _systemClock = NULL;

/* inherited methods */

static void setState(ClockDriver *, ClockState);
static void processUpdate(ClockDriver *, double, double);

/* inherited methods end */

ClockDriver *
createClockDriver(int driverType, const char *name)
{

    ClockDriver *clockDriver;

    XCALLOC(clockDriver, sizeof(ClockDriver));

    if(!setupClockDriver(clockDriver, driverType, name)) {
	freeClockDriver(&clockDriver);
    } else {

	/* maintain the linked list */

	if(_first == NULL) {
		_first = clockDriver;
	}

	if(_last != NULL) {
	    clockDriver->_prev = _last;
	    clockDriver->_prev->_next = clockDriver;
	}

	_last = clockDriver;
	clockDriver->_first = _first;
    }

    return clockDriver;

}

Boolean
setupClockDriver(ClockDriver* clockDriver, int driverType, const char *name)
{

    Boolean found = FALSE;

    clockDriver->type = driverType;
    strncpy(clockDriver->name, name, CLOCKDRIVER_NAME_MAX);

    clockDriver->_serial = _serial;
    _serial++;

    REGISTER_CLOCKDRIVER(CLOCKDRIVER_UNIX, unix);
    REGISTER_CLOCKDRIVER(CLOCKDRIVER_LINUXPHC, linuxphc);


    if(!found) {
	ERROR("Setup requested for unknown clock driver type: %d\n", driverType);
    } else {
	DBG("Created new clock driver type %d name %s serial %d\n", driverType, name, clockDriver->_serial);
    }

    /* inherited methods */

    clockDriver->setState = setState;
    clockDriver->processUpdate = processUpdate;

    /* inherited methods end */


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

	if(pdriver == _last) {
	    _serial = pdriver->_serial;
	}

	if(pdriver->_prev != NULL) {

		if(pdriver == _last) {
			_last = pdriver->_prev;
		}

		if(pdriver->_next != NULL) {
			pdriver->_prev->_next = pdriver->_next;
		} else {
			pdriver->_prev->_next = NULL;
		}
	/* last one */
	} else if (pdriver->_next == NULL) {
		_first = NULL;
		_last = NULL;
	}

	if(pdriver->_next != NULL) {

		if(pdriver == _first) {
			_first = pdriver->_next;
		}

		if(pdriver->_prev != NULL) {
			pdriver->_next->_prev = pdriver->_prev;
		} else {
			pdriver->_next->_prev = NULL;
		}

	}

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

    _systemClock = createClockDriver(CLOCKDRIVER_UNIX, "SYSTEM_CLOCK");

    if(_systemClock == NULL) {
	CRITICAL("Could not start system clock driver, cannot continue\n");
	exit(1);
    }

    return _systemClock;

}

void
shutdownClockDrivers() {
	ClockDriver *cd;
	while(_first != NULL) {
	    cd = _last;
	    freeClockDriver(&cd);
	}
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
