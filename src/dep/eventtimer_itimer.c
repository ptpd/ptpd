/*-
 * Copyright (c) 2012-2015 Wojciech Owczarek,
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments.
 * Copyright (c) 2009-2010 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen
 *
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
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
 * @file   eventtimer_itimer.c
 * @date   Wed Oct 1 00:41:26 2014
 *
 * @brief  EventTimer implementation using interval timers
 *
 * The original interval timer timer implementation.
 */

#include "../ptpd.h"

#define US_TIMER_INTERVAL (31250)

static volatile unsigned int elapsed;

static void eventTimerStart_itimer(EventTimer *timer, double interval);
static void eventTimerStop_itimer(EventTimer *timer);
static void eventTimerReset_itimer(EventTimer *timer);
static void eventTimerShutdown_itimer(EventTimer *timer);
static Boolean eventTimerIsRunning_itimer(EventTimer *timer);
static Boolean eventTimerIsExpired_itimer(EventTimer *timer);

static void itimerUpdate(EventTimer *et);
static void timerSignalHandler(int sig);

void
setupEventTimer(EventTimer *timer)
{

	if(timer == NULL) {
	    return;
	}

	memset(timer, 0, sizeof(EventTimer));

	timer->start = eventTimerStart_itimer;
	timer->stop = eventTimerStop_itimer;
	timer->reset = eventTimerReset_itimer;
	timer->shutdown = eventTimerShutdown_itimer;
	timer->isExpired = eventTimerIsExpired_itimer;
	timer->isRunning = eventTimerIsRunning_itimer;
}

static void
eventTimerStart_itimer(EventTimer *timer, double interval)
{

	timer->expired = FALSE;
	timer->running = TRUE;

	/*
	 *  US_TIMER_INTERVAL defines the minimum interval between sigalarms.
	 *  timerStart has a float parameter for the interval, which is casted to integer.
	 *  very small amounts are forced to expire ASAP by setting the interval to 1
	 */
	timer->itimerLeft = (interval * 1E6) / US_TIMER_INTERVAL;
	if(timer->itimerLeft == 0){
		/*
		 * the interval is too small, raise it to 1 to make sure it expires ASAP
		 */
		timer->itimerLeft = 1;
	}
	
	timer->itimerInterval = timer->itimerLeft;

	DBG2("timerStart:     Set timer %s to %f  New interval: %d; new left: %d\n", timer->id, interval, timer->itimerLeft , timer->itimerInterval);
}

static void
eventTimerStop_itimer(EventTimer *timer)
{

	timer->itimerInterval = 0;
	timer->running = FALSE;
	DBG2("timerStop:      Stopping timer %s\n", timer->id);

}

static void
itimerUpdate(EventTimer *et)
{

	EventTimer *timer = NULL;

	if (elapsed <= 0)
		return;

	/*
	 * if time actually passed, then decrease every timer left
	 * the one(s) that went to zero or negative are:
	 *  a) rearmed at the original time (ignoring the time that may have passed ahead)
	 *  b) have their expiration latched until timerExpired() is called
	 */

	for(timer = et->_first; timer != NULL; timer = timer->_next) {
	    if ( (timer->itimerInterval > 0) && ((timer->itimerLeft -= elapsed) <= 0)) {
			timer->itimerLeft = timer->itimerInterval;
			timer->expired = TRUE;
		DBG("TimerUpdate:    Timer %s has now expired.  Re-armed with interval %d\n", timer->id, timer->itimerInterval);
	    }
	}

	elapsed = 0;

}



static void
eventTimerReset_itimer(EventTimer *timer)
{
}

static void
eventTimerShutdown_itimer(EventTimer *timer)
{
}


static Boolean
eventTimerIsRunning_itimer(EventTimer *timer)
{

	itimerUpdate(timer);

	DBG2("timerIsRunning:   Timer %s %s running\n", timer->id,
		timer->running ? "is" : "is not");

	return timer->running;
}

static Boolean
eventTimerIsExpired_itimer(EventTimer *timer)
{

	Boolean ret;

	itimerUpdate(timer);

	ret = timer->expired;

	DBG2("timerIsExpired:   Timer %s %s expired\n", timer->id,
		timer->expired ? "is" : "is not");

	if(ret) {
	    timer->expired = FALSE;
	}

	return ret;

}

void
startEventTimers(void)
{
	struct itimerval itimer;

	DBG("initTimer\n");

#ifdef __sun
	sigset(SIGALRM, SIG_IGN);
#else
	signal(SIGALRM, SIG_IGN);
#endif /* __sun */

	elapsed = 0;
	itimer.it_value.tv_sec = itimer.it_interval.tv_sec = 0;
	itimer.it_value.tv_usec = itimer.it_interval.tv_usec =
	    US_TIMER_INTERVAL;

#ifdef __sun
	sigset(SIGALRM, timerSignalHandler);
#else	
	signal(SIGALRM, timerSignalHandler);
#endif /* __sun */
	setitimer(ITIMER_REAL, &itimer, 0);
}

void
shutdownEventTimers(void)
{

#ifdef __sun
	sigset(SIGALRM, SIG_IGN);
#else
	signal(SIGALRM, SIG_IGN);
#endif /* __sun */
}

static void
timerSignalHandler(int sig)
{
	elapsed++;
	/* be sure to NOT call DBG in asynchronous handlers! */
}
