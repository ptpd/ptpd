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

#include "../datatypes.h"

#define DOMAIN_MISMATCH_MIN 10	/* trigger domain mismatch alarm after at least 10 mismatches */
#define ALARM_UPDATE_INTERVAL 1 /* how often we process alarms */
#define ALARM_TIMEOUT_PERIOD 30	/* minimal alarm age to clear */
#define ALARM_HANDLERS_MAX 3	/* max number of alarm handlers */
#define ALARM_MESSAGE_LENGTH 100/* length of an alarm-specific message string */

/* explicitly numbered because these are referenced in the MIB as textual convention */
typedef enum {
	ALRM_PORT_STATE = 0,		/*x done*/
	ALRM_OFM_THRESHOLD = 1,		/*x done */
	ALRM_OFM_SECONDS = 2, 		/*x done*/
	ALRM_CLOCK_STEP = 3,		/*x done*/
	ALRM_NO_SYNC = 4,		/*x done*/
	ALRM_NO_DELAY = 5, 			/*x done*/
	ALRM_MASTER_CHANGE = 6, 		/*x done*/
	ALRM_NETWORK_FLT = 7,		/*x done*/
	ALRM_FAST_ADJ = 8,			/*+/- currently only at maxppb */
	ALRM_TIMEPROP_CHANGE = 9,		/*x done*/
	ALRM_DOMAIN_MISMATCH = 10, 		/*+/- currently only when all packets come from an incorrect domain */
	ALRM_MAX
} AlarmType;

/* explicitly numbered because these are referenced in the MIB as textual convention */
typedef enum {
	ALARM_UNSET = 0,		/* idle */
	ALARM_SET = 1,			/* condition has been triggerd */
	ALARM_CLEARED = 2		/* condition has cleared */
} AlarmState;

struct _alarmEntry {
	char shortName[5];		/* short code i.e. OFS, DLY, SYN, FLT etc. */
	char name[31];			/* full name i.e. OFFSET_THRESHOLD, NO_DELAY, NO_SYNC etc. */
	char description[101];		/* text description */
	Boolean enabled;		/* is the alarm operational ? */
	uint8_t id; 			/* alarm ID */
	Boolean eventOnly;		/* this is only an event - don't manage state, just dispatch/inform when condition is met */
	void (*handlers[ALARM_HANDLERS_MAX])(struct _alarmEntry *); /* alarm handlers */
	void * userData;		/* user data pointer */
	Boolean unhandled;		/* this event is pending pick-up - prevets from clearing condition until it's handled */
	uint32_t age;			/* age of alarm in current state (seconds) */
	uint32_t minAge;			/* minimum age of alarm (time between set and clear notification - condition can go away earlier */
	AlarmState state;		/* state of the alarm */
	Boolean condition;		/* is the alarm condition met? (so we can check conditions and set alarms separately */
	TimeInternal timeSet;		/* time when set */
	TimeInternal timeCleared;	/* time when cleared */
	Boolean internalOnly;		/* do not display in status file / indicate that the alarm is internal only */
	PtpEventData eventData;		/* the event data union - so we can capture any data we need without the need to capture a whole PtpClock */
};

typedef struct _alarmEntry AlarmEntry;

void initAlarms(AlarmEntry* alarms, int count, void* userData); 			/* fill an array with initial alarm data */
void configureAlarms(AlarmEntry* alarms, int count, void* userData); 			/* fill an array with initial alarm data */
void enableAlarms(AlarmEntry* alarms, int count, Boolean enabled); 			/* enable/disable all alarms */
void updateAlarms(AlarmEntry *alarms, int count);					/* dispatch alarms: age, set, clear alarms etc. */
void displayAlarms(AlarmEntry *alarms, int count);					/* display a formatted alarm summary table */
int  getAlarmSummary(char * output, int size, AlarmEntry *alarms, int count);		/* produce a one-line alarm summary string */
void handleAlarm(AlarmEntry *alarms, void *userData);

#endif /*PTPDALARMS_H_*/
