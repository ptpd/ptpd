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

static const char* timerDesc[PTP_MAX_TIMER] = {

	/* fundamental PTP timers */
	[ANNOUNCE_RECEIPT_TIMER]	= "ANNOUNCE_RECEIPT",
	[DELAYREQ_INTERVAL_TIMER]	= "DELAYREQ_INTERVAL",
	[PDELAYREQ_INTERVAL_TIMER]	= "PDELAYREQ_INTERVAL",
	[SYNC_INTERVAL_TIMER]		= "SYNC_INTERVAL",
	[ANNOUNCE_INTERVAL_TIMER]	= "ANNOUNCE_INTERVAL",

	/* non-spec PTP-related timers */
	[PORT_FAULT_TIMER]		= "PORT_FAULT",
	[SYNC_RECEIPT_TIMER]		= "SYNC_RECEIPT",
	[DELAY_RECEIPT_TIMER]		= "DELAY_RECEIPT",
	[UNICAST_GRANT_TIMER]		= "UNICAST_GRANT",
	[CALIBRATION_DELAY_TIMER]	= "CALIBRATION_DELAY",
	[PERIODIC_INFO_TIMER]		= "PERIODIC_INFO",
	[STATISTICS_UPDATE_TIMER]	= "STATISTICS_UPDATE",
	[CLOCK_UPDATE_TIMER]		= "CLOCK_UPDATE",

	/* PTPd daemon timers - to be moved away */
	[OPERATOR_MESSAGES_TIMER]	= "OPERATOR_MESSAGES",
	[LEAP_SECOND_PAUSE_TIMER]	= "LEAP_SECOND_PAUSE",
	[STATUSFILE_UPDATE_TIMER]	= "STATUSFILE_UPDATE",
	[ALARM_UPDATE_TIMER]		= "ALARM_UPDATE",

};

const char *
getPtpTimerName(int id)
{

    if(id >= 0 && id < PTP_MAX_TIMER) {
	return timerDesc[id];
    }

    return "UNKNOWN_TIMER";

}
