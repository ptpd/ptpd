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
 * @file   timerfd.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  timerfd timer implementation
 *
 */

#include <config.h>

#include <string.h>
#include <time.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <errno.h>


#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/timer.h>
#include <libcck/timer_interface.h>

/* timers should be based on CLOCK_MONOTONIC,
 *   but if it's not implemented, use CLOCK_REALTIME
 */

#ifdef _TIMERFD_MONOTONIC_CLOCK
#define CLK_TYPE CLOCK_MONOTONIC
#else
#define CLK_TYPE CLOCK_REALTIME
#endif /* CLOCK_MONOTONIC */

#define THIS_COMPONENT "timer.timerfd: "

/* tracking the number of instances */
static int _instanceCount = 0;

static void timerFdExpired(void *myFd, void *timer);

bool
_setupCckTimer_timerfd(CckTimer *self)
{

    INIT_INTERFACE(self);

    self->_instanceCount = &_instanceCount;

    _instanceCount++;

    return true;

}

static int
timerInit (CckTimer *self, const bool oneShot, CckFdSet *fdSet) {


    self->myFd.fd = timerfd_create(CLK_TYPE, TFD_NONBLOCK);

	if(self->myFd.fd == -1) {
	    CCK_PERROR(THIS_COMPONENT"(%s): Could not create timerfd timer\n", self->name);
	    return 0;
	} else {
	    CCK_DBG(THIS_COMPONENT"(%s): Created timerfd timer id %d\n", self->name, self->numId);
	}

    self->config.oneShot = oneShot;

    self->myFd.owner = self;

    self->myFd.callbacks.onData = timerFdExpired;

    self->fdSet = fdSet;

    /* add ourselves to the FD set */
    if(self->fdSet != NULL) {
	cckAddFd(self->fdSet, &self->myFd);
    }

    self->_init = true;

    CCK_DBG(THIS_COMPONENT"(%s): Timer initialised\n", self->name);

    return 1;

}

static int
timerShutdown (CckTimer *self) {

    self->stop(self);

    CCK_DBG(THIS_COMPONENT"(%s): timer shutdown\n", self->name);

    if(self->fdSet) {
	cckRemoveFd(self->fdSet, &self->myFd);
    }

    close(self->myFd.fd);
    self->myFd.fd = -1;

    self->_init = false;

    return 1;

}

static void
timerStart (CckTimer *self, const double interval) {

    double initial = 0.0;
    double actual = max(interval, CCK_TIMER_MIN_INTERVAL);
    struct timespec ts;
    struct itimerspec its;

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

    if (timerfd_settime(self->myFd.fd, 0, &its, NULL) < 0) {
	CCK_PERROR(THIS_COMPONENT"(%s): Could not arm timerfd timer (%f s requested)", self->name, actual);
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

    memset(&its, 0, sizeof(its));

    if (timerfd_settime(self->myFd.fd, 0, &its, NULL) < 0) {
	CCK_PERROR(THIS_COMPONENT"(%s): Could not stop timerfd timer", self->name);
	return;
    }

    self->_running = false;
    self->interval = 0;

    CCK_DBG(THIS_COMPONENT"(%s): timer stopped\n", self->name);

}


static void
timerFdExpired(void *myFd, void *timer) {

    /* data is an uint64_t expiration counter */
    uint64_t dummy;
    CckTimer *self = timer;

    if(read(self->myFd.fd, &dummy, sizeof(dummy))) {
	self->_expired = true;
    }

}
