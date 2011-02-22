/*-
 * Copyright (c) 2009-2011 George V. Neville-Neil, Steven Kreuzer, 
 *                         Martin Burnicki, Gael Mace, Alexandre Van Kempen
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
 * @file   timer.c
 * @date   Wed Jun 23 09:41:26 2010
 * 
 * @brief  The timers which run the state machine.
 * 
 * Timers in the PTP daemon are run off of the signal system.  
 */

#include "../ptpd.h"

#define TIMER_INTERVAL 1
unsigned int elapsed;

void 
catch_alarm(int sig)
{
	elapsed++;
	DBGV("catch_alarm: elapsed %d\n", elapsed);
}

void 
initTimer(void)
{
	struct itimerval itimer;

	DBG("initTimer\n");

	signal(SIGALRM, SIG_IGN);

	elapsed = 0;
	itimer.it_value.tv_sec = itimer.it_interval.tv_sec = TIMER_INTERVAL;
	itimer.it_value.tv_usec = itimer.it_interval.tv_usec = 0;

	signal(SIGALRM, catch_alarm);
	setitimer(ITIMER_REAL, &itimer, 0);
}

void 
timerUpdate(IntervalTimer * itimer)
{

	int i, delta;

	delta = elapsed;
	elapsed = 0;

	if (delta <= 0)
		return;

	for (i = 0; i < TIMER_ARRAY_SIZE; ++i) {
		if ((itimer[i].interval) > 0 && ((itimer[i].left) -= delta) 
		    <= 0) {
			itimer[i].left = itimer[i].interval;
			itimer[i].expire = TRUE;
			DBGV("timerUpdate: timer %u expired\n", i);
		}
	}

}

void 
timerStop(UInteger16 index, IntervalTimer * itimer)
{
	if (index >= TIMER_ARRAY_SIZE)
		return;

	itimer[index].interval = 0;
}

void 
timerStart(UInteger16 index, UInteger16 interval, IntervalTimer * itimer)
{
	if (index >= TIMER_ARRAY_SIZE)
		return;

	itimer[index].expire = FALSE;
	itimer[index].left = interval;
	itimer[index].interval = itimer[index].left;

	DBGV("timerStart: set timer %d to %d\n", index, interval);
}

Boolean 
timerExpired(UInteger16 index, IntervalTimer * itimer)
{
	timerUpdate(itimer);

	if (index >= TIMER_ARRAY_SIZE)
		return FALSE;

	if (!itimer[index].expire)
		return FALSE;

	itimer[index].expire = FALSE;

	return TRUE;
}
