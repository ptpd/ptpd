#ifndef EVENTTIMER_H_
#define EVENTTIMER_H_

#include "../ptpd.h"

/*-
 * Copyright (c) 2015 Wojciech Owczarek,
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

#define EVENTTIMER_MAX_DESC		20
#define EVENTTIMER_MIN_INTERVAL_US	250 /* 4000/sec */

typedef struct EventTimer EventTimer;

struct EventTimer {

	/* data */
	char id[EVENTTIMER_MAX_DESC + 1];
	Boolean expired;
	Boolean running;

	/* "methods" */
	void (*start) (EventTimer* timer, double interval);
	void (*stop) (EventTimer* timer);
	void (*reset) (EventTimer* timer);
	void (*shutdown) (EventTimer* timer);
	Boolean (*isExpired) (EventTimer* timer);
	Boolean (*isRunning) (EventTimer* timer);	

	/* implementation data */
#ifdef PTPD_PTIMERS
	timer_t timerId;
#else
	int32_t itimerInterval;
	int32_t itimerLeft;
#endif /* PTPD_PTIMERS */

	/* linked list */
	EventTimer *_first;
	EventTimer *_next;
	EventTimer *_prev;

};

EventTimer *createEventTimer(const char *id);
void freeEventTimer(EventTimer **timer);
void setupEventTimer(EventTimer *timer);

void startEventTimers();
void shutdownEventTimers();


#endif /* EVENTTIMER_H_ */

