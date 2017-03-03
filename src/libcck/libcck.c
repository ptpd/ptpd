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
 * @file   libcck.h
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  libCCK initialisation and housekeeping code
 *
 */

#include <libcck/cck.h>
#include <libcck/libcck.h>

#include <libcck/cck_logger.h>
#include <libcck/cck_utils.h>
#include <libcck/clockdriver.h>
#include <libcck/ttransport.h>
#include <libcck/timer.h>
#include <libcck/acl.h>

#define THIS_COMPONENT "libCCK: "

#define tmrctl(id, command, ... ) _timers[id]->command(_timers[id], __VA_ARGS__)

enum {
    TMR_CLOCKSYNC,
    TMR_CLOCKUPDATE,
    TMR_NETMONITOR,
    TMR_MAX
};

static const char * timerDesc[] = {
    [TMR_CLOCKSYNC]	= "__CCK_CLOCK_SYNC",
    [TMR_CLOCKUPDATE]	= "__CCK_CLOCK_UPDATE",
    [TMR_NETMONITOR]	= "__CCK_NET_MONITOR"
};

static const CckConfig defaults = {

    .clockSyncRate		= CLOCKDRIVER_SYNC_RATE,
    .netMonitorInterval		= TT_MONITOR_INTERVAL,
    .clockUpdateInterval	= CLOCKDRIVER_UPDATE_INTERVAL,
    .transportFaultTimeout	= TT_FAULT_TIMEOUT,

};

static CckTimer* _timers[TMR_MAX];
static CckConfig config;
static CckFdSet fdSet;

static bool initCckTimers(CckFdSet *set);
static void cckTimerHandler(void *timer, void *owner);
static void cckApplyConfig();

bool
cckInit(CckFdSet *set)
{

    bool ret = true;

    clearCckFdSet(set);

    /* set defaults */
    memcpy(&config, &defaults, sizeof(config));

    ret &= initCckTimers(set);

    if(!ret) {
	CCK_CRITICAL(THIS_COMPONENT"LibCCK startup error\n");
	cckShutdown();
    }

    cckApplyConfig();

    CCK_NOTICE(THIS_COMPONENT"LibCCK version "CCK_API_VER_STR" initialised\n");

    return ret;

}

void cckShutdown()
{

    shutdownTTransports();
    shutdownClockDrivers();
    shutdownCckTimers();
    shutdownCckAcls();

}

CckConfig*
getCckConfig()
{
    return &config;
}

CckFdSet*
getCckFdSet()
{
    return &fdSet;
}

const CckConfig*
cckDefaults()
{
    return &defaults;
}

static void
cckApplyConfig()
{
    tmrctl(TMR_CLOCKUPDATE, start, config.clockUpdateInterval);
    tmrctl(TMR_NETMONITOR, start, config.netMonitorInterval);
    tmrctl(TMR_CLOCKSYNC, start, 1 / ( 0.0 + config.clockSyncRate));
}

static void
cckTimerHandler (void *timer, void *owner)
{

    CckTimer *myTimer = timer;
    int iinterval = myTimer->interval;


    switch(myTimer->numId) {

	case TMR_CLOCKSYNC:
		syncClocks(myTimer->interval);
	    break;
	case TMR_CLOCKUPDATE:
		updateClockDrivers(myTimer->interval);
	    break;
	case TMR_NETMONITOR:
		controlTTransports(TT_UPDATECOUNTERS, &iinterval);
		controlTTransports(TT_MONITOR, &iinterval);
	    break;

    }

    CCK_DBG(THIS_COMPONENT"internal timer %s handled\n", myTimer->name);

}

static bool
initCckTimers(CckFdSet *set)
{

    for(int i = 0; i < TMR_MAX; i++) {

	_timers[i] = createCckTimer(CCKTIMER_ANY, timerDesc[i]);
	_timers[i]->numId = i;
	_timers[i]->config.randomDelay = true;

	int res = _timers[i]->init(_timers[i], false, set);

	if(res != 1) {
	    return false;
	}

	_timers[i]->callbacks.onExpired = cckTimerHandler;

    }

    return true;

}
