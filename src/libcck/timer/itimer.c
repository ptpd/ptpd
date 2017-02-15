/* Copyright (c) 2017 Wojciech Owczarek,
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
 * @file   itimer.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  itimer timer implementation
 *
 */

#include <config.h>

#include <sys/time.h>
#include <signal.h>
#include <errno.h>

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/timer.h>
#include <libcck/timer_interface.h>

/* 64/sec fixed tick */
#define ITIMER_INTERVAL_US 15625
#define TIMER_SIGNAL SIGALRM

#define THIS_COMPONENT "timer.itimer: "

/* we need to iterate over the instances of this implementation */
LL_ROOT(CckTimer);

/* tracking the number of instances */
static int _instanceCount = 0;

static bool _initialised = false;
static bool _update = false;

/* common elapsed counter subtracted from each instance's interval - incremented by one globa itimer */
static volatile uint32_t elapsed;

/* this one we override */
static bool timerIsExpired(CckTimer *self);

static void itimerInit();
static void itimerShutdown();
static void itimerSigHandler(int signo);
static void itimerUpdate();

bool
_setupCckTimer_itimer(CckTimer *self)
{

    INIT_INTERFACE(self);

    self->isExpired = timerIsExpired;

    CCK_INIT_PDATA(CckTimer, itimer, self);

    self->_instanceCount = &_instanceCount;

    _instanceCount++;

    return true;

}

static int
timerInit (CckTimer *self, const bool oneShot, CckFdSet *fdSet) {

    /* first init: start up itimer ticker */
    if(!_initialised) {
	itimerInit();
    }

    self->config.oneShot = oneShot;

    LL_APPEND_STATIC(self);

    self->_init = true;

    CCK_DBG(THIS_COMPONENT"(%s): timer initialised\n", self->name);

    return 1;

}

static int
timerShutdown (CckTimer *self) {

    self->stop(self);

    LL_REMOVE_STATIC(self);

    CCK_DBG(THIS_COMPONENT"(%s): timer shutdown\n", self->name);

    /* last instance shutting down */
    if(!_first) {
	itimerShutdown();
    }

    self->_init = false;

    return 1;

}

static void
timerStart (CckTimer *self, const double interval) {

    double actual = max(interval, CCK_TIMER_MIN_INTERVAL);

    CCK_GET_PDATA(CckTimer, itimer, self, myData);

    self->_expired = false;
    self->_running = true;
    self->interval = actual;

    myData->left = (actual * 1E6) / ITIMER_INTERVAL_US;

    myData->left = max(myData->left, 1);
    myData->interval = myData->left;

    CCK_DBG(THIS_COMPONENT"(%s): Timer armed for %f s requested, %f s actual (%d * %d us ticks)%s\n",
			    self->name, interval, actual, myData->interval, ITIMER_INTERVAL_US,
			    self->config.oneShot ? ", one-shot" : "");

}

static void
timerStop (CckTimer *self) {

    CCK_GET_PDATA(CckTimer, itimer, self, myData);

    myData->interval = 0;
    self->_running = false;
    self->interval = 0;

    CCK_DBG(THIS_COMPONENT"(%s): timer stopped\n", self->name);

}

static bool
timerIsExpired(CckTimer *self) {

    bool ret;

    if(_update) {
	itimerUpdate();
    }

    ret = self->_expired;

    if(ret) {
	/* since we've already restarted - or finished if we're one shot */
	self->_expired = false; 
	if(self->config.oneShot) {
	    CCK_DBG(THIS_COMPONENT"(%s): one-shot timer stopping\n", self->name);
	    self->stop(self);
	}
    }

    return ret;

}

static void
itimerInit() {
    struct itimerval itimer;

    CCK_DBG(THIS_COMPONENT"initialising itimer ticker\n");

#ifdef __sun
	sigset(TIMER_SIGNAL, SIG_IGN);
#else
	signal(TIMER_SIGNAL, SIG_IGN);
#endif /* __sun */

	elapsed = 0;
	itimer.it_value.tv_sec = itimer.it_interval.tv_sec = 0;
	itimer.it_value.tv_usec = itimer.it_interval.tv_usec =
	    ITIMER_INTERVAL_US;

#ifdef __sun
	sigset(TIMER_SIGNAL, itimerSigHandler);
#else	
	signal(TIMER_SIGNAL, itimerSigHandler);
#endif /* __sun */
	setitimer(ITIMER_REAL, &itimer, 0);

	_initialised = true;

}

static void itimerShutdown() {

    CCK_DBG(THIS_COMPONENT"shutting down itimer ticker\n");

#ifdef __sun
	sigset(TIMER_SIGNAL, SIG_IGN);
#else
	signal(TIMER_SIGNAL, SIG_IGN);
#endif /* __sun */

	_initialised = false;

}

static void
itimerSigHandler(int signo) {

    /* this is a signal handler yo, so be light and pay due attention, the less the better */
    elapsed++;
    _update = true;

}

static void itimerUpdate() {


    CckTimer *t;

    _update = false;

    if(elapsed <= 0) {
	return;
    }

    LL_FOREACH_STATIC(t) {

	CCK_GET_PDATA(CckTimer, itimer, t, it);

	if(!t->_init || t->config.disabled || (it->interval <= 0)) {
	    continue;
	}

	it->left -= elapsed;

	if(it->left <= 0) {

	    it->left = it->interval;
	    t->_expired = true;
	    CCK_DBG(THIS_COMPONENT"(%s): timer expired, re-armed to %d ticks (%d us)%s\n",
				    t->name, it->interval, it->interval * ITIMER_INTERVAL_US,
				    t->config.oneShot ? ", one-shot, will stop on next expiry check" : "");

	}

    }

    elapsed = 0;

}

