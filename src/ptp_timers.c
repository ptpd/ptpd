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
 * @file   timer.c
 * @date   Wed Oct 1 00:41:26 2014
 *
 * @brief  PTP timer handler code
 *
 * Glue code providing PTPd with event timers.
 * This code can be re-written to make use of any timer
 * implementation. Provided is an EventTimer which uses
 * a fixed tick interval timer, or POSIX timers, depending
 * on what is available. So the options are:
 * - write another EventTimer implementation,
 *   using the existing framework,
 * - write something different
 */

#include "ptpd.h"

void
timerStop(IntervalTimer * itimer)
{
	if (itimer == NULL)
		return;
	EventTimer *timer = (EventTimer *)(itimer->data);

	timer->stop(timer);
}

void
timerStart(IntervalTimer * itimer, double interval)
{
	if (itimer == NULL)
		return;

        if(interval > PTPTIMER_MAX_INTERVAL) {
            interval = PTPTIMER_MAX_INTERVAL;
        }

	itimer->interval = interval;
	EventTimer* timer = (EventTimer *)(itimer->data);

	timer->start(timer, interval);
}

Boolean
timerExpired(IntervalTimer * itimer)
{

	if (itimer == NULL)
		return FALSE;

	EventTimer *timer = (EventTimer *)(itimer->data);

	return timer->isExpired(timer);
}

Boolean
timerRunning(IntervalTimer * itimer)
{

	if (itimer==NULL)
		return FALSE;

	EventTimer *timer = (EventTimer *)(itimer->data);

	return timer->isRunning(timer);
}

Boolean timerSetup(IntervalTimer *itimers)
{

    Boolean ret = TRUE;

/* WARNING: these descriptions MUST be in the same order,
 * and in the same number as the enum in ptp_timers.h
 */

    static const char* timerDesc[PTP_MAX_TIMER] = {
  "PDELAYREQ_INTERVAL",
  "DELAYREQ_INTERVAL",
  "SYNC_INTERVAL",
  "ANNOUNCE_RECEIPT",
  "ANNOUNCE_INTERVAL",
  "SYNC_RECEIPT",
  "DELAY_RECEIPT",
  "UNICAST_GRANT",
  "OPERATOR_MESSAGES",
  "LEAP_SECOND_PAUSE",
  "STATUSFILE_UPDATE",
  "PANIC_MODE",
  "PERIODIC_INFO_TIMER",
#ifdef PTPD_STATISTICS
  "STATISTICS_UPDATE",
#endif /* PTPD_STATISTICS */
  "ALARM_UPDATE",
  "MASTER_NETREFRESH",
  "CALIBRATION_DELAY",
  "CLOCK_UPDATE",
  "TIMINGDOMAIN_UPDATE"
    };

    int i = 0;

    startEventTimers();

    for(i=0; i<PTP_MAX_TIMER; i++) {

	itimers[i].data = NULL;
	itimers[i].data = (void *)(createEventTimer(timerDesc[i]));
	if(itimers[i].data == NULL) {
	    ret = FALSE;
	}
    }

    return ret;

}

void timerShutdown(IntervalTimer *itimers)
{

    int i = 0;
    EventTimer *timer = NULL;


    for(i=0; i<PTP_MAX_TIMER; i++) {
	    timer = (EventTimer*)(itimers[i].data);
	    freeEventTimer(&timer);
    }

    shutdownEventTimers();

}
