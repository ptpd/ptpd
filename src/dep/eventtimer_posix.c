/*-
 * Copyright (c) 2015      Wojciech Owczarek,
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
 * @file   eventtimer_posix.c
 * @date   Wed Oct 1 00:41:26 2014
 *
 * @brief  EventTimer implementation using POSIX timers
 *
 * The less signal intensive timer implementation,
 * allowing for high(er) message intervals.
 */

#include "../ptpd.h"

#define TIMER_SIGNAL SIGALRM

/* timers should be based on CLOCK_MONOTONIC,
 *   but if it's not implemented, use CLOCK_REALTIME
 */

#ifdef _POSIX_MONOTONIC_CLOCK
#define CLK_TYPE CLOCK_MONOTONIC
#else
#define CLK_TYPE CLOCK_REALTIME
#endif /* CLOCK_MONOTONIC */

static void eventTimerStart_posix(EventTimer *timer, double interval);
static void eventTimerStop_posix(EventTimer *timer);
static void eventTimerReset_posix(EventTimer *timer);
static void eventTimerShutdown_posix(EventTimer *timer);
static Boolean eventTimerIsRunning_posix(EventTimer *timer);
static Boolean eventTimerIsExpired_posix(EventTimer *timer);
static void timerSignalHandler(int sig, siginfo_t *info, void *usercontext);

void
setupEventTimer(EventTimer *timer)
{

	struct sigevent sev;

	if(timer == NULL) {
	    return;
	}

	memset(&sev, 0, sizeof(sev));
	memset(timer, 0, sizeof(EventTimer));

	timer->start = eventTimerStart_posix;
	timer->stop = eventTimerStop_posix;
	timer->reset = eventTimerReset_posix;
	timer->shutdown = eventTimerShutdown_posix;
	timer->isExpired = eventTimerIsExpired_posix;
	timer->isRunning = eventTimerIsRunning_posix;

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = TIMER_SIGNAL;
	sev.sigev_value.sival_ptr = timer;

	if(timer_create(CLK_TYPE, &sev, &timer->timerId) == -1) {
	    PERROR("Could not create posix timer %s", timer->id);
	} else {
	    DBGV("Created posix timer %s ",timer->id);
	}
}

static void
eventTimerStart_posix(EventTimer *timer, double interval)
{

	struct timespec ts;
	struct itimerspec its;
   
	memset(&its, 0, sizeof(its));

	ts.tv_sec = interval;
	ts.tv_nsec = (interval - ts.tv_sec) * 1E9;

	if(!ts.tv_sec && ts.tv_nsec < EVENTTIMER_MIN_INTERVAL_US * 1000) {
	    ts.tv_nsec = EVENTTIMER_MIN_INTERVAL_US * 1000;
	}

	DBGV("Timer %s start requested at %d.%4d sec interval\n", timer->id, ts.tv_sec, ts.tv_nsec);

	its.it_interval = ts;
	its.it_value = ts;

	if (timer_settime(timer->timerId, 0, &its, NULL) < 0) {
		PERROR("could not arm posix timer %s", timer->id);
		return;
	}

	DBG2("timerStart:     Set timer %s to %f\n", timer->id, interval);

	timer->expired = FALSE;
	timer->running = TRUE;

}

static void
eventTimerStop_posix(EventTimer *timer)
{

	struct itimerspec its;

	DBGV("Timer %s stop requested\n", timer->id);

	memset(&its, 0, sizeof(its));

	if (timer_settime(timer->timerId, 0, &its, NULL) < 0) {
		PERROR("could not stop posix timer %s", timer->id);
		return;
	}

	timer->running = FALSE;

	DBG2("timerStop: stopped timer %s\n", timer->id);

}

static void
eventTimerReset_posix(EventTimer *timer)
{
}

static void
eventTimerShutdown_posix(EventTimer *timer)
{
	if(timer_delete(timer->timerId) == -1) {
	    PERROR("Could not delete timer %s!", timer->id);
	}

}

static Boolean
eventTimerIsRunning_posix(EventTimer *timer)
{

	DBG2("timerIsRunning:   Timer %s %s running\n", timer->id,
		timer->running ? "is" : "is not");

	return timer->running;
}

static Boolean
eventTimerIsExpired_posix(EventTimer *timer)
{

	Boolean ret;

	ret = timer->expired;

	DBG2("timerIsExpired:   Timer %s %s expired\n", timer->id,
		timer->expired ? "is" : "is not");

	/* the five monkeys experiment */
	if(ret) {
	    timer->expired = FALSE;
	}

	return ret;

}

void
startEventTimers(void)
{
	struct sigaction sa;

	DBG("initTimer\n");

#ifdef __sun
	sigset(SIGALRM, SIG_IGN);
#else
	signal(SIGALRM, SIG_IGN);
#endif /* __sun */

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = timerSignalHandler;
	sigemptyset(&sa.sa_mask);
	if(sigaction(TIMER_SIGNAL, &sa, NULL) == -1) {
	    PERROR("Could not initialise timer handler");
	}

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

/* this only ever gets called when a timer expires */
static void
timerSignalHandler(int sig, siginfo_t *info, void *usercontext)
{

	/* retrieve the user data structure */
	EventTimer * timer = (EventTimer*)info->si_value.sival_ptr;

	/* Ignore if the signal wasn't sent by a timer */
	if(info->si_code != SI_TIMER)
		return;

	/* Hopkirk (deceased) */
	if(timer != NULL) {
	    timer->expired = TRUE;
	}

}
