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
 * @file   timer.h
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  structure definitions for the event timer
 *
 */

#ifndef CCK_TIMER_H_
#define CCK_TIMER_H_

#include <libcck/cck.h>
#include <libcck/linked_list.h>
#include <libcck/cck_types.h>
#include <libcck/fd_set.h>


/* 50 us to prevent loops impossible to handle */
#define CCK_TIMER_MIN_INTERVAL 0.00005

/* timer implementation types - automagic */
enum {

    CCKTIMER_NONE = 0,
    /* "any" uses first ("best") timer implementaion available */
    CCKTIMER_ANY = 1,
#define CCK_ALL_IMPL /* invoke all implementations, even those we are not building */
#define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, textdesc) \
    typeenum,
#include "timer.def"

    CCKTIMER_MAX
};

/* commands that can be sent to all transports */
enum {
    TIMER_NOTINUSE,	/* mark all not in use */
    TIMER_SHUTDOWN,	/* shutdown all */
    TIMER_CLEANUP,	/* clean up timers not in use */
    TIMER_DUMP,		/* dump timer information  */
    TIMER_DISPATCH,	/* check expiry, dispatch handlers */
};

/* common timer configuration */
typedef struct {
    bool disabled;			/* timer is disabled */
    bool oneShot;			/* one-shot timer, no auto-rearm */
    double interval;			/* timer interval */
} CckTimerConfig;

typedef struct CckTimer CckTimer;

/* the timer */
struct CckTimer {

    int type;					/* timer implementation type */
    char name[CCK_COMPONENT_NAME_MAX +1];	/* instance name */
    void *owner;				/* pointer to user structure owning or controlling this component */

    /* timers support FDs in principle, but they don't have to */
    CckFdSet *fdSet;			/* file descriptor set watching this timer */
    CckFd myFd;				/* file descriptor wrapper */

    CckTimerConfig config;		/* config container */

    int numId;				/* numeric ID */
    double interval;			/* interval the timer is running at */

    /* user-supplied callbacks */
    struct {
	void (*onExpired) (void *timer, void *owner);
	void (*onStart) (void *timer, void *owner);
	void (*onStop) (void *timer, void *owner);
    } callbacks;

    /* flags */
    bool inUse;				/* timer is in active use, if not, can be removed */

    /* BEGIN "private" fields */

    int _serial;			/* object serial no */
    int	_init;				/* the driver was successfully initialised */
    int *_instanceCount;		/* instance counter for the whole component */

    bool _expired;			/* timer has expired */
    bool _running;			/* timer is running (started) */

    /* libCCK common fields - to be included in a general object header struct */
    void *_privateData;			/* implementation-specific data */

    /* END "private" fields */

    /* inherited methods */

    bool (*isExpired)	(CckTimer *);
    bool (*isRunning)	(CckTimer *);

    /* inherited methods end */

    /* public / private interface - implementations must implement all of those */

    /* init, shutdown */
    int (*init)		(CckTimer*, const bool oneShot, CckFdSet *fdSet);
    int (*shutdown) 	(CckTimer*);
    void (*start)	(CckTimer*, const double interval);
    void (*stop)	(CckTimer*);

    /* public / private interface end */

    /* attach the linked list */
    LL_MEMBER(CckTimer);

};

CckTimer*	createCckTimer(int type, const char* name);
bool 		setupCckTimer(CckTimer* timer, int type, const char* name);
void 		freeCckTimer(CckTimer** timer);

void		controlCckTimers(const int, const void*);
void 		shutdownCckTimers();
void		cckDispatchTimers();

CckTimer*	findCckTimer(const char *);
CckTimer*	getCckTimerByName(const char *);
CckTimer*	getCckTimerById(const int);

const char*	getCckTimerTypeName(int);
int		getCckTimerType(const char*);

/* invoking this without CCK_REGISTER_IMPL defined, includes the implementation headers - like here*/
#include "timer.def"

#endif /* CCK_TIMER_H_ */
