#ifndef PTPDALARMS_H_
#define PTPDALARMS_H_

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
 * @file    alarms.h
 * @authors Wojciech Owczarek
 * @date   Wed Dec 9 19:13:10 2015
 * Data type and function definitions related to
 * handling raising and clearing alarms.
 */

typedef struct {
} PtpEventData;

typedef enum {
	ALRM_PORT_STATE,
	ALRM_OFM_THRESHOLD,
	ALRM_OFM_SECONDS,
	ALRM_CLOCK_STEP,
	ALRM_NO_SYNC,
	ALRM_NO_DELAY,
	ALRM_MASTER_CHANGE,
	ALRM_NETWORK_FLT,
	ALRM_FAST_ADJ,
	ALRM_TIMEPROP_CHANGE,
	ALRM_DOMAIN_MISMATCH,
	ALRM_MAX
} AlarmType;


typedef enum {
	ALARM_UNSET = 0,		/* idle */
	ALARM_SET,		/* condition has been triggerd */
	ALARM_CLEARED		/* condition has cleared */
} AlarmState;

typedef struct {
	uint8_t id; 			/* alarm ID */
	uint32_t age;			/* age of alarm in current state (seconds) */
	AlarmState state;		/* state of the alarm */
	Boolean condition;		/* is the alarm condition met? (so we can check conditions and set alarms separately */
	TimeInternal timeSet;		/* time when set */
	TimeInternal timeCleared;	/* time when cleared */
	Boolean eventOnly;		/* this is only an event - don't manage state, just dispatch/inform when condition is met */
	Boolean internalOnly;		/* do not display in status file / indicate that the alarm is internal only */
	PtpEventData eventData;		/* the event data union - so we can capture any data we need without the need to capture a whole PtpClock */
} AlarmData;

typedef struct {
	char shortName[5];		/* short code i.e. OFS, DLY, SYN, FLT etc. */
	char name[31];			/* full name i.e. OFFSET_THRESHOLD, NO_DELAY, NO_SYNC etc. */
	char description[101];		/* text description */

/*	char *userHandle;		*/ /* pointer to user data associated with the alarm */
/*	char userData[200];		*/ /* or maybe some space to contain the data taken there and then */

	AlarmData data;			/* alarm data container (so it's easier to do a static initialisation */
} AlarmEntry;

AlarmEntry * getAlarms();

#endif /*PTPDALARMS_H_*/
