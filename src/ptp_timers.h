#ifndef PTP_TIMERS_H_
#define PTP_TIMERS_H_

#include "ptpd.h"

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


#ifdef PTPD_PTIMERS
#define LOG_MIN_INTERVAL -7
#else
/* 62.5ms tick for interval timers = 16/sec max */
#define LOG_MIN_INTERVAL -4
#endif /* PTPD_PTIMERS */

/* safeguard: a week */
#define PTPTIMER_MAX_INTERVAL 604800

/**
* \brief Structure used as a timer
 */
typedef struct {
	double interval;
	Boolean expired;
	Boolean running;
	/* hook for a generic timer object that can be assigned */
	void *data;
} IntervalTimer;

/* WARNING: when updating these timers,
 * you MUST update timerSetup() in ptp_timers.c accordingly!
 * otherwise expect a segfault if there are more timers here
 * than descriptions in ptp_timers.c
 */

enum {
  PDELAYREQ_INTERVAL_TIMER=0,/**<\brief Timer handling the PdelayReq Interval*/
  DELAYREQ_INTERVAL_TIMER,/**<\brief Timer handling the delayReq Interva*/
  SYNC_INTERVAL_TIMER,/**<\brief Timer handling Interval between master sends two Syncs messages */
  ANNOUNCE_RECEIPT_TIMER,/**<\brief Timer handling announce receipt timeout*/
  ANNOUNCE_INTERVAL_TIMER, /**<\brief Timer handling interval before master sends two announce messages*/
  /* non-spec timers */
  SYNC_RECEIPT_TIMER,
  DELAY_RECEIPT_TIMER,
  UNICAST_GRANT_TIMER, /* used to age out unicast grants (sent, received) */
  OPERATOR_MESSAGES_TIMER,  /* used to limit the operator messages */
  LEAP_SECOND_PAUSE_TIMER, /* timer used for pausing updates when leap second is imminent */
  STATUSFILE_UPDATE_TIMER, /* timer used for refreshing the status file */
  PANIC_MODE_TIMER,	   /* timer used for the duration of "panic mode" */
  PERIODIC_INFO_TIMER,	   /* timer used for dumping periodic status updates */
#ifdef PTPD_STATISTICS
  STATISTICS_UPDATE_TIMER, /* online mean / std dev updare interval (non-moving statistics) */
#endif /* PTPD_STATISTICS */
  ALARM_UPDATE_TIMER,
  MASTER_NETREFRESH_TIMER,
  CALIBRATION_DELAY_TIMER,
  CLOCK_UPDATE_TIMER,
  TIMINGDOMAIN_UPDATE_TIMER,
  PTP_MAX_TIMER
};

/* functions used by 1588 only */
void timerStop(IntervalTimer *itimer);
void timerStart(IntervalTimer * itimer, double interval);
Boolean timerExpired(IntervalTimer * itimer);
Boolean timerRunning(IntervalTimer * itimer);
Boolean timerSetup(IntervalTimer *itimers);
void timerShutdown(IntervalTimer *itimers);

#endif /* PTP_TIMERS_H_ */
