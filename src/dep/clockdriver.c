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


static ClockDriver* _osClock = NULL;

static uint32_t _serial = 0;

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

    if(pdriver->data != NULL) {
	free(pdriver->data);
    }

    if(pdriver->config != NULL) {
	free(pdriver->config);
    }

    DBG("Deleted clock driver type %d name %s serial %d\n", pdriver->type, pdriver->name, pdriver->_serial);

    free(*clockDriver);

    *clockDriver = NULL;

};

ClockDriver*
getOsClock() {

    if(_osClock != NULL) {
	return _osClock;
    }

    _osClock = createClockDriver(CLOCKDRIVER_UNIX, "SYSTEM_CLOCK");

    if(_osClock == NULL) {
	CRITICAL("Could not start system clock driver, cannot continue\n");
	exit(1);
    }

    return _osClock;

}

void
shutdownClockDrivers() {
	ClockDriver *cd;
	while(_first != NULL) {
	    cd = _last;
	    freeClockDriver(&cd);
	}
	_osClock = NULL;
}

