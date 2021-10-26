/*-
 * Copyright (c) 2020      Chris Johns,
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
 * @file   eventtimer_kqueue.c
 * @date
 *
 * @brief  EventTimer implementation using kqueue timers
 *
 * Needs support in the kqueue to pass timer events to here.
 */

#include "../ptpd.h"

static void eventTimerStart_kqueue(EventTimer *timer, double interval);
static void eventTimerStop_kqueue(EventTimer *timer);
static void eventTimerReset_kqueue(EventTimer *timer);
static void eventTimerShutdown_kqueue(EventTimer *timer);
static Boolean eventTimerIsRunning_kqueue(EventTimer *timer);
static Boolean eventTimerIsExpired_kqueue(EventTimer *timer);

static int timerInstance;

void
setupEventTimer(EventTimer *timer)
{

  struct kevent evset;

	if(timer == NULL) {
	    return;
	}

	memset(timer, 0, sizeof(EventTimer));

	timer->start = eventTimerStart_kqueue;
	timer->stop = eventTimerStop_kqueue;
	timer->reset = eventTimerReset_kqueue;
	timer->shutdown = eventTimerShutdown_kqueue;
	timer->isExpired = eventTimerIsExpired_kqueue;
	timer->isRunning = eventTimerIsRunning_kqueue;

  timer->timerId = ++timerInstance;

  EV_SET(&evset, timer->timerId, EVFILT_TIMER, EV_ADD | EV_DISABLE, 0, 0, timer);

  if (kevent(ptpKqueueGet(), &evset, 1, NULL, 0, NULL) < 0)
      PERROR("kevent timer add | disable");

  DBGV("Created kqueue timer %s (%d)", timer->id, timer->timerId);
}

static void
eventTimerStart_kqueue(EventTimer *timer, double interval)
{
  int kq = ptpKqueueGet();
  if (kq >= 0) {
    struct kevent evset;
    intptr_t data = interval * 1000000;

    EV_SET(&evset, timer->timerId,
           EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR,
           NOTE_USECONDS, data, timer);

    DBGV("Timer %s start requested at %ld us interval\n", timer->id, data);

    if (kevent(kq, &evset, 1, NULL, 0, NULL) < 0) {
      PERROR("kevent timer add | enable");
      return;
    }

    DBG2("timerStart:     Set timer %s to %f\n", timer->id, interval);

    timer->expired = FALSE;
    timer->running = TRUE;
  }
}

static void
eventTimerStop_kqueue(EventTimer *timer)
{
  int kq = ptpKqueueGet();
  if (kq >= 0) {
    struct kevent evset;

    EV_SET(&evset, timer->timerId, EVFILT_TIMER, EV_ADD | EV_DISABLE, 0, 0, timer);

    DBGV("Timer %s stop requested\n", timer->id);

    if (kevent(kq, &evset, 1, NULL, 0, NULL) < 0)
      PERROR("kevent timer add | disable");

    timer->running = FALSE;

    DBG2("timerStop: stopped timer %s\n", timer->id);
  }
}

static void
eventTimerReset_kqueue(EventTimer *timer)
{
}

static void
eventTimerShutdown_kqueue(EventTimer *timer)
{
  int kq = ptpKqueueGet();
  if (kq >= 0) {
    struct kevent evset;

    EV_SET(&evset, timer->timerId, EVFILT_TIMER, EV_DELETE, 0, 0, timer);

    if (kevent(kq, &evset, 1, NULL, 0, NULL) < 0)
      PERROR("kevent timer disable");

    timer->running = FALSE;

  DBG2("timerShutdown: %s\n", timer->id);
  }
}

static Boolean
eventTimerIsRunning_kqueue(EventTimer *timer)
{
	DBG2("timerIsRunning:   Timer %s %s running\n", timer->id,
		timer->running ? "is" : "is not");

	return timer->running;
}

static Boolean
eventTimerIsExpired_kqueue(EventTimer *timer)
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
	DBG("initTimer\n");
}

void
shutdownEventTimers(void)
{
}
