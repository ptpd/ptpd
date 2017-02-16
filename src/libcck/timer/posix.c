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
 * @file   posix.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  posix timer implementation
 *
 */

#include <config.h>

#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/timer.h>
#include <libcck/timer_interface.h>

#define TIMER_SIGNAL SIGALRM

/* timers should be based on CLOCK_MONOTONIC,
 *   but if it's not implemented, use CLOCK_REALTIME
 */

#ifdef _POSIX_MONOTONIC_CLOCK
#define CLK_TYPE CLOCK_MONOTONIC
#else
#define CLK_TYPE CLOCK_REALTIME
#endif /* CLOCK_MONOTONIC */

#define THIS_COMPONENT "timer.posix: "

/* tracking the number of instances */
static int _instanceCount = 0;

/* we need to iterate over the instances of this implementation */
LL_ROOT(CckTimer);

static bool _initialised = false;

static void posixInit();
static void posixShutdown();

static void posixSigHandler(int signo, siginfo_t *info, void *usercontext);


bool
_setupCckTimer_posix(CckTimer *self)
{

    INIT_INTERFACE(self);

    CCK_INIT_PDATA(CckTimer, posix, self);

    self->_instanceCount = &_instanceCount;

    _instanceCount++;

    return true;

}

static int
timerInit (CckTimer *self, const bool oneShot, CckFdSet *fdSet) {

    struct sigevent sev;
    CCK_GET_PDATA(CckTimer, posix, self, myData);

    /* first init: start up posix ticker */
    if(!_initialised) {
	posixInit();
    }

    memset(&sev, 0, sizeof(sev));

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = TIMER_SIGNAL;
    sev.sigev_value.sival_ptr = self;

	if(timer_create(CLK_TYPE, &sev, &myData->timerId) == -1) {
	    CCK_PERROR(THIS_COMPONENT"(%s): Could not create posix timer\n", self->name);
	    return 0;
	} else {
	    CCK_DBG(THIS_COMPONENT"(%s): Created posix timer id %d\n", self->name, myData->timerId);
	}

    self->config.oneShot = oneShot;

    LL_APPEND_STATIC(self);

    self->_init = true;

    CCK_DBG(THIS_COMPONENT"(%s): Timer initialised\n", self->name);

    return 1;

}

static int
timerShutdown (CckTimer *self) {

    self->stop(self);

    CCK_GET_PDATA(CckTimer, posix, self, myData);

    if(timer_delete(myData->timerId) == -1) {
	CCK_PERROR(THIS_COMPONENT"(%s): Could not delete posix timer %s!", self->name);
    }

    LL_REMOVE_STATIC(self);

    CCK_DBG(THIS_COMPONENT"(%s): timer shutdown\n", self->name);

    /* last instance shutting down */
    if(!_first) {
	posixShutdown();
    }

    self->_init = false;

    return 1;

}

static void
timerStart (CckTimer *self, const double interval) {

    double initial = 0.0;
    double actual = max(interval, CCK_TIMER_MIN_INTERVAL);
    struct timespec ts;
    struct itimerspec its;
    CCK_GET_PDATA(CckTimer, posix, self, myData);

    ts.tv_sec = actual;
    ts.tv_nsec = (actual - ts.tv_sec) * 1E9;

    its.it_interval = ts;

    if(self->config.randomDelay) {
	initial =  actual * cckRand() * CCK_TIMER_RANDDELAY;
	CCK_DBG(THIS_COMPONENT"(%s): Random delay %.05f s\n",
			self->name, initial);
	initial += actual;
    } else if(self->config.delay) {
	initial = self->config.delay;
	CCK_DBG(THIS_COMPONENT"(%s): Fixed delay %.05f s\n",
		self->name, initial);
	initial += actual;
    }

    ts.tv_sec = initial;
    ts.tv_nsec = (initial - ts.tv_sec) * 1E9;

    its.it_value = ts;

    if (timer_settime(myData->timerId, 0, &its, NULL) < 0) {
	CCK_PERROR(THIS_COMPONENT"(%s): Could not arm posix timer (%f s requested)", self->name, actual);
	return;
    }

    self->_expired = false;
    self->_running = true;
    self->interval = actual;

    CCK_DBG(THIS_COMPONENT"(%s): Timer armed for %f s requested, %f s actual%s\n",
			    self->name, interval, actual,
			    self->config.oneShot ? ", one-shot" : "");

}

static void
timerStop (CckTimer *self) {

    struct itimerspec its;
    CCK_GET_PDATA(CckTimer, posix, self, myData);

    memset(&its, 0, sizeof(its));

    if (timer_settime(myData->timerId, 0, &its, NULL) < 0) {
	CCK_PERROR(THIS_COMPONENT"(%s): Could not stop posix timer", self->name);
	return;
    }

    self->_running = false;
    self->interval = 0;

    CCK_DBG(THIS_COMPONENT"(%s): timer stopped\n", self->name);

}

static void
posixInit() {

    CCK_DBG(THIS_COMPONENT"initialising POSIX timer support\n");

    struct sigaction sa;

#ifdef __sun
	sigset(TIMER_SIGNAL, SIG_IGN);
#else
	signal(TIMER_SIGNAL, SIG_IGN);
#endif /* __sun */

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = posixSigHandler;
    sigemptyset(&sa.sa_mask);
    if(sigaction(TIMER_SIGNAL, &sa, NULL) == -1) {
	CCK_PERROR(THIS_COMPONENT"Could not initialise timer signal handler!");
    }

    _initialised = true;

}

static void posixShutdown() {

    CCK_DBG(THIS_COMPONENT"shutting down POSIX timer support\n");

#ifdef __sun
    sigset(TIMER_SIGNAL, SIG_IGN);
#else
    signal(TIMER_SIGNAL, SIG_IGN);
#endif /* __sun */

    _initialised = false;

}

/* called only on timer expiry (we have multiple timers unlike with itimer) */
static void
posixSigHandler(int signo, siginfo_t *info, void *usercontext) {

    /* this is a signal handler yo, so be light and pay due attention, the less the better */

    if(signo != TIMER_SIGNAL) {
	return;
    }

    /* Ignore if the signal wasn't sent by a timer */
    if(info->si_code != SI_TIMER) {
	return;
    }

    /* get user data */
    CckTimer *timer = (CckTimer*)info->si_value.sival_ptr;

    /* Hopkirk (deceased) */
    if(timer) {
        timer->_expired = true;
    }

}
