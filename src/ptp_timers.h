#ifndef PTP_TIMERS_H_
#define PTP_TIMERS_H_

#include "ptpd.h"
#include <stdbool.h>

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

/* safeguard: a week */
#define PTPTIMER_MAX_INTERVAL 604800

/* max name length */
#define PTP_TIMER_NAME_MAX 20

struct PtpClock;

/**
* \brief Structure used as a timer
 */
typedef struct {
	double interval;
	bool expired;
	bool running;
	/* hook for a generic timer object that can be assigned */
	void *data;
} IntervalTimer;

typedef struct {
	void *data;	/* timer implementation data */
	char name[PTP_TIMER_NAME_MAX];
	double interval;
	bool expired;
	bool running;
} PtpTimer;

enum {
	/* fundamental PTP timers */
	ANNOUNCE_RECEIPT_TIMER,	/* Announce receipt timeout timer */
	DELAYREQ_INTERVAL_TIMER,	/* Delay Request transmission interval */
	PDELAYREQ_INTERVAL_TIMER,	/* Peer Delay Request transmission interval */
	SYNC_INTERVAL_TIMER,		/* Sync transmission interval */
	ANNOUNCE_INTERVAL_TIMER,	/* Announce transmission interval */

	/* non-spec PTP-related timers */
	PORT_FAULT_TIMER,		/* PTP port fault timer - only for internal faults (external do not start or stop it) */
	SYNC_RECEIPT_TIMER,		/* Sync receipt timeout (alarm) timer */
	DELAY_RECEIPT_TIMER,		/* Delay response receipt timeout (alarm) timer */
	UNICAST_GRANT_TIMER,		/* Unicast grant ageing timer */
	CALIBRATION_DELAY_TIMER,	/* Offset calibration delay timer */
	PERIODIC_INFO_TIMER,		/* Periodic PTP status updates */
	STATISTICS_UPDATE_TIMER,	/* Online PTP statistics update interval (non-moving mean and dev) */
	CLOCK_UPDATE_TIMER,		/* Clock update failsafe timer (stuck slave) */

	/* PTPd daemon timers - to be moved away */
	OPERATOR_MESSAGES_TIMER, 	/* Limit repeated warning messages */
	LEAP_SECOND_PAUSE_TIMER,	/* Event message processing pause around leap second event */
	STATUSFILE_UPDATE_TIMER,	/* Status file update */
	ALARM_UPDATE_TIMER,		/* Alarm ageing */

	/* marker */
	PTP_MAX_TIMER
};

/* user-supplied interface */
extern void ptpTimerStop(PtpTimer *timer);
extern void ptpTimerStart(PtpTimer *timer, const double interval);
extern void ptpTimerLogStart(PtpTimer *timer, const int power2);
extern bool ptpTimerExpired(PtpTimer *timer);
extern bool ptpTimerInit(struct PtpClock *ptpClock);
extern void ptpTimerShutdown(struct PtpClock *ptpClock);

/* quickhand macros assuming that the @var variable contains a timers array indexed by @id */
#define tmrExpired(var, id)	ptpTimerExpired(&((var)->timers[id##_TIMER]))
#define tmrStart(var, id, val)	ptpTimerStart(&((var)->timers[id##_TIMER]), val)
#define tmrStop(var, id)	ptpTimerStop(&((var)->timers[id##_TIMER]))
#define tmrRunning(var, id)	((var)->timers[id##_TIMER].running)
#define tmrInterval(var, id)	((var)->timers[id##_TIMER].interval)

const char * getPtpTimerName(int id);

#endif /* PTP_TIMERS_H_ */
