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
 * @file   timer.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  Initialisation and control of the Timestamping Transport
 *
 */

#include <config.h>

#include <string.h>
#include <sys/types.h>

#include <libcck/cck_utils.h>
#include <libcck/timer.h>
#include <libcck/cck_logger.h>

#define THIS_COMPONENT "timer: "

/* linked list - so that we can control all registered objects centrally */
LL_ROOT(CckTimer);

static const char *timerTypeNames[] = {

    [CCKTIMER_NONE] = "none",
    [CCKTIMER_ANY] = "any",
#define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, textdesc) \
    [typeenum] = textname,

#include "timer.def"

    [CCKTIMER_MAX] = "nosuchtype"

};

/* inherited method declarations */

static bool isTimerExpired(CckTimer *timer);
static bool isTimerRunning(CckTimer *timer);

/* inherited method declarations end */

/* timer management code */

CckTimer*
createCckTimer(const int type, const char* name) {

    tmpstr(aclName, CCK_COMPONENT_NAME_MAX);

    CckTimer *timer = NULL;

    if(getCckTimerByName(name) != NULL) {
	CCK_ERROR(THIS_COMPONENT"Cannot create timer %s: timer with this name already exists\n", name);
	return NULL;
    }

    CCKCALLOC(timer, sizeof(CckTimer));

    if(!setupCckTimer(timer, type, name)) {
	if(timer != NULL) {
	    free(timer);
	}
	return NULL;
    } else {
	/* maintain the linked list */
	LL_APPEND_STATIC(timer);
    }

    return timer;

}

bool
setupCckTimer(CckTimer* timer, const int type, const char* name) {

    bool found = false;
    bool setup = false;

    int mytype = type;

    strncpy(timer->name, name, CCK_COMPONENT_NAME_MAX);

    /* inherited methods - implementation may wish to override them,
     * or even preserve these pointers in its private data and call both
     */

    timer->isExpired = isTimerExpired;
    timer->isRunning = isTimerRunning;

    /* inherited methods end */


    /* these macros call the setup functions for existing timer implementations */

    #define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, textdesc) \
	if(!found && (type == CCKTIMER_ANY || mytype==typeenum)) {\
	    mytype = type; \
	    found = true;\
	    setup = _setupCckTimer_##typesuffix(timer);\
	}
    #include "timer.def"

    if(type == CCKTIMER_ANY && !found) {
	CCK_CRITICAL(THIS_COMPONENT"No available timer implementations found. LibCCK is inoperable\n");
	exit(1);
    }

    if(!found) {

	CCK_ERROR(THIS_COMPONENT"Setup requested for unknown clock timer type: %d\n", type);

    } else if(!setup) {
	return false;
    } else {

	CCK_DBG(THIS_COMPONENT"Created new timer type %d name %s serial %d\n", type, name, timer->_serial);

    }

    timer->type = mytype;

    return found;

}

void
freeCckTimer(CckTimer** timer) {

    CckTimer *ptimer = *timer;

    if(!timer || !*timer) {
	return;
    }

    if(ptimer->_init) {
	ptimer->shutdown(ptimer);
    }

    /* maintain the linked list */
    LL_REMOVE_STATIC(ptimer);

    if(ptimer->_privateData != NULL) {
	free(ptimer->_privateData);
    }

    CCK_DBG(THIS_COMPONENT"Deleted timer type %d name %s serial %d\n",
			    ptimer->type, ptimer->name, ptimer->_serial);

    free(*timer);

    *timer = NULL;

}


void
controlCckTimers(const int command, const void *arg) {

    CckTimer *timer;
    bool found;

    switch(command) {

	case TIMER_NOTINUSE:
	    LL_FOREACH_STATIC(timer) {
		timer->inUse = false;
	    }
	break;

	case TIMER_SHUTDOWN:
	    LL_DESTROYALL(timer, freeCckTimer);
	break;

	case TIMER_CLEANUP:
	    do {
		found = false;
		LL_FOREACH_STATIC(timer) {
		    if(!timer->inUse) {
			freeCckTimer(&timer);
			found = true;
			break;
		    }
		}
	    } while(found);
	break;

	case TIMER_DUMP:
	    LL_FOREACH_STATIC(timer) {
		if(!timer->config.disabled) {
		/* TODO: dump timer information */
//		timer->dumpCounters(timer);
		}

	    }
	break;

	case TIMER_DISPATCH:
		cckDispatchTimers();
	break;

	default:
	    CCK_ERROR(THIS_COMPONENT"controlCckTimers(): Unnown timer command %02d\n", command);

    }

}

void
shutdownCckTimers() {

	CckTimer *timer;
	LL_DESTROYALL(timer, freeCckTimer);

}

CckTimer*
findCckTimer(const char *search) {

    return getCckTimerByName(search);

}

CckTimer*
getCckTimerByName(const char *name) {

	CckTimer *timer;

	LL_FOREACH_STATIC(timer) {
	    if(!strncmp(timer->name, name, CCK_COMPONENT_NAME_MAX)) {
		return timer;
	    }
	}

	return NULL;

}

CckTimer*
getCckTimerById(const int numId) {

	CckTimer *timer;

	LL_FOREACH_STATIC(timer) {
	    if(timer->numId == numId) {
		return timer;
	    }
	}

	return NULL;


}

const char*
getCckTimerTypeName(const int type) {

    if ((type < 0) || (type >= CCKTIMER_MAX)) {
	return NULL;
    }

    return timerTypeNames[type];

}

int
getCckTimerType(const char* name) {

    for(int i = 0; i < CCKTIMER_MAX; i++) {

	if(!strcmp(name, timerTypeNames[i])) {
	    return i;
	}

    }

    return -1;

}

void cckDispatchTimers() {

    CckTimer * timer;
    LL_FOREACH_STATIC(timer) {
	if(!timer->config.disabled) {
	    if(timer->isExpired(timer)) {
		SAFE_CALLBACK(timer->callbacks.onExpired, timer, timer->owner);
	    }
	}
    }
}

static bool
isTimerExpired(CckTimer *timer) {

    bool ret;

    ret = timer->_expired;

    if(ret) {
	CCK_DBG(THIS_COMPONENT"(%s) Timer expired\n", timer->name);
	/* since we've already restarted - or finished if we're one shot */
	timer->_expired = false;
	if(timer->config.oneShot) {
	    CCK_DBG(THIS_COMPONENT"(%s): one-shot timer stopping\n", timer->name);
	    timer->stop(timer);
	}
    }

    return ret;

}

static bool
isTimerRunning(CckTimer *timer) {

    return timer->_running;

}
