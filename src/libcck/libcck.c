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

#define tmrctl(id, command, ... ) timers[id]->command(timers[id], __VA_ARGS__)

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

static CckTimer* timers[TMR_MAX];
static CckConfig cckConfig;
static CckFdSet fdSet;

static void cckDefaults(CckConfig *config);
static bool initCckTimers(CckFdSet *set);
static void cckTimerHandler(void *timer, void *owner);


bool
cckInit(CckFdSet *set)
{

    bool ret = true;

    cckDefaults(&cckConfig);
    clearCckFdSet(set);

    ret &= initCckTimers(set);

    if(!ret) {
	CCK_CRITICAL("LibCCK startup error\n");
	cckShutdown();
    }

    tmrctl(TMR_CLOCKUPDATE, start, cckConfig.clockUpdateInterval);
    tmrctl(TMR_NETMONITOR, start, cckConfig.netMonitorInterval);
    tmrctl(TMR_CLOCKSYNC, start, 1 / ( 0.0 + cckConfig.clockSyncRate));

    CCK_NOTICE("LibCCK version "CCK_API_VER_STR" initialised\n");

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
    return &cckConfig;
}

CckFdSet*
getCckFdSet()
{
    return &fdSet;
}


static void
cckDefaults(CckConfig *config)
{

    config->clockSyncRate	= CLOCKDRIVER_SYNC_RATE;
    config->netMonitorInterval	= TT_MONITOR_INTERVAL;
    config->clockUpdateInterval	= CLOCKDRIVER_UPDATE_INTERVAL;

}

static void
cckTimerHandler (void *timer, void *owner)
{

    CckTimer *myTimer = timer;

    CCK_INFO("internal timer %s action!\n", myTimer->name);

}

static bool
initCckTimers(CckFdSet *set)
{

    for(int i = 0; i < TMR_MAX; i++) {

	timers[i] = createCckTimer(CCKTIMER_ANY, timerDesc[i]);
	timers[i]->numId = i;

	int res = timers[i]->init(timers[i], false, set);

	if(res != 1) {
	    return false;
	}

	timers[i]->callbacks.onExpired = cckTimerHandler;

    }

    return true;

}
