/*-
 * Copyright (c) 2015 Wojciech Owczarek
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
 * @file    alarms.c
 * @authors Wojciech Owczarek
 * @date   Wed Dec 9 19:13:10 2015
 * This souece file contains the implementations of functions
 * handling raising and clearing alarms.
 */

#include "../ptpd.h"

static const char* alarmStateToString(AlarmState state);

static void dispatchEvent(AlarmEntry *alarm);
static void dispatchAlarm(AlarmEntry *alarm);

static const char
*alarmStateToString(AlarmState state)
{

    switch(state) {

	case ALARM_UNSET:
	    return "NONE";
	case ALARM_SET:
	    return "SET";
	case ALARM_CLEARED:
	    return "CLEAR";
	default:
	    return "?";
    }

}

static void
dispatchEvent(AlarmEntry *alarm) {
	INFO("Event %s dispatched\n", alarm->name);
	alarm->data.condition = FALSE;
}

static void
dispatchAlarm(AlarmEntry *alarm) {

	if(alarm->data.state == ALARM_UNSET) {
	    INFO("Alarm %s cleared\n", alarm->name);
	}

	if(alarm->data.state == ALARM_SET) {
	    INFO("Alarm %s set\n", alarm->name);
	}

}


void
initAlarms(AlarmEntry* alarms, int count)
{

    static AlarmEntry alarmTemplate[] = {

    /* short */	/* name */		/* desc */						/* enabled? */ /* id */		/* eventOnly */
    { "STA", 	"PORT_STATE", 		"Port state different to expected value", 		FALSE, { ALRM_PORT_STATE, 	FALSE }},
    { "OFM", 	"OFM_THRESHOLD", 	"Offset From Master outside threshold", 		FALSE, { ALRM_OFM_THRESHOLD,	FALSE }},
    { "OFMS", 	"OFM_SECONDS", 		"Offset From Master above 1 second", 			FALSE, { ALRM_OFM_SECONDS,	FALSE }},
    { "STEP", 	"CLOCK_STEP", 		"Clock was stepped", 					FALSE, { ALRM_CLOCK_STEP,	TRUE }},
    { "SYN", 	"NO_SYNC", 		"Clock is not receiving Sync messages",			FALSE, { ALRM_NO_SYNC,	FALSE }},
    { "DLY", 	"NO_DELAY", 		"Clock is not receiving (p)Delay responses",		FALSE, { ALRM_NO_DELAY,	FALSE }},
    { "MSTC", 	"MASTER_CHANGE", 	"Best master has changed",				FALSE, { ALRM_MASTER_CHANGE,	TRUE }},
    { "NWFL", 	"NETWORK_FAULT", 	"A network fault has occurred",				FALSE, { ALRM_NETWORK_FLT,	FALSE }},
    { "FADJ", 	"FAST_ADJ", 		"Clock is being adjusted too fast", 			FALSE, { ALRM_FAST_ADJ,	TRUE }},
    { "TPR", 	"TIMEPROP_CHANGE", 	"Time properties have changed",				FALSE, { ALRM_TIMEPROP_CHANGE,TRUE }},
    { "DOM", 	"DOMAIN_MISMATCH", 	"Clock is receiving all messages from incorrect domain",FALSE, { ALRM_DOMAIN_MISMATCH,FALSE }}

    };

    if(count > ALRM_MAX) return;

    memset(alarms, 0, count * sizeof(AlarmEntry));
    memcpy(alarms, alarmTemplate, sizeof(alarmTemplate));

}

void
setAlarmCondition(AlarmEntry *alarm, Boolean condition, PtpClock *ptpClock)
{

	Boolean change = condition ^ alarm->data.condition;

	if(change) {
	    DBG("Alarm %s condition set to %s\n", alarm->name, condition ? "TRUE" : "FALSE");
	    alarm->data.age = 0;
	}

	if(condition && !alarm->data.condition) {
	    capturePtpEventData(&alarm->data.eventData, ptpClock, ptpClock->rtOpts);
	    getTime(&alarm->data.timeSet);
	} else if(alarm->data.condition) {
	    getTime(&alarm->data.timeCleared);
	}

	alarm->data.condition = condition;

}

void
capturePtpEventData(PtpEventData *data, PtpClock *ptpClock, RunTimeOpts *rtOpts)
{

    data->defaultDS = ptpClock->defaultDS;
    data->currentDS = ptpClock->currentDS;
    data->timePropertiesDS = ptpClock->timePropertiesDS;
    data->portDS = ptpClock->portDS;
    data->parentDS = ptpClock->parentDS;

    if(ptpClock->bestMaster != NULL) {
	data->bestMaster = *ptpClock->bestMaster;
    } else {
	memset(&data->bestMaster, 0, sizeof(ForeignMasterRecord));
    }

    data->ofmAlarmThreshold = rtOpts->ofmAlarmThreshold;
}

/*
 * this is our asynchronous event/alarm processor / state machine. ALARM_TIMEOUT_PERIOD protects us from alarms flapping,
 * and ensures that an alarm will last at least n seconds: notification is processed only when the alarm is set for the
 * first time (from UNSET state), and is not cancelled until the timeout passes (in the meantime, alarm condition can be
 * triggered again and timer is reset, so it can flap at will), and cancel notification will only be sent when the condition
 * has cleared and timeout has passed.
 */
void
updateAlarms(AlarmEntry *alarms, int count)
{

    DBG("updateAlarms()\n");

    AlarmEntry *alarm;
    AlarmData *data;
    AlarmState lastState;

    int i = 0;

    for(i = 0; i < count; i++) {

	alarm = &alarms[i];
	data = &alarm->data;
	lastState = data->state;
	/* this is a one-off event */
	if(data->eventOnly) {
	    if(data->condition) {
		dispatchEvent(alarm);
	    }
	    continue;
	}
	/* this is an alarm */

	if(!data->condition) {
		/* condition is false, alarm is cleared and aged out: unset */
		if(data->state == ALARM_CLEARED && data->age >= ALARM_TIMEOUT_PERIOD ) {
		    data->state = ALARM_UNSET;
		    /* inform, run handlers */
		    dispatchAlarm(alarm);
		/* condition is false and alarm was set - clear and wait for age out */
		} else if (data->state == ALARM_SET) {
		    data->state = ALARM_CLEARED;
		}
	/* condition is true and age is 0: condition just changed */
	} else if (data->age ==0) {
		data->state = ALARM_SET;
		/* react only if alarm was set from unset (don't inform multiple times until it fully clears */
		if(lastState == ALARM_UNSET) {
		    /* inform, run handlers */
		    dispatchAlarm(alarm);
		}
	}
	data->age+=ALARM_UPDATE_INTERVAL;
    }

}

void
displayAlarms(AlarmEntry *alarms, int count)
{

    int i = 0;
    INFO("----------------------------------------------\n");
    INFO("              Alarm status: \n");
    INFO("----------------------------------------------\n");
    INFO("ALARM NAME\t| STATE    | DESCRIPTION\n");
    INFO("----------------------------------------------\n");
    for(i=0; i < count; i++) {
	if(alarms[i].data.eventOnly) continue;
	INFO("%-15s\t| %-8s | %s\n",  alarms[i].name, 
				    alarmStateToString(alarms[i].data.state), 
				    alarms[i].description);
    }
    INFO("----------------------------------------------\n");

}

/* populate @output with a one-line alarm summary, consisting of multiple:
 *  ALCD[x]
 * where:
 * - ALCD is the short name(code)
 * - x is "!" when alarm is set
 * - x is "." when alarm is cleared
 * - when alarm is gone (unset) its code is not listed
 *
 * Returning the number of bytes written. When output is null, only the number of bytes needed is returned
 * (e.g. to allow us to allocate a buffer)
 */
int
getAlarmSummary(char * output, int size, AlarmEntry *alarms, int count)
{

    int len = 0;
    int i = 0;

    char item[9];

    AlarmEntry *alarm;
    AlarmData *data;

    if(output != NULL) {
	memset(output, 0, size);
    }

    for(i=0; i < count; i++) {
	memset(item, 0, 8);
	alarm = &alarms[i];
	data = &alarm->data;

	if(data->state == ALARM_UNSET) {
	    continue;
	}

	strncpy(item, alarm->shortName, 4);
	snprintf(item + strlen(item), 5, "[%s] ", data->state == ALARM_SET ? "!" : ".");

	if(output != NULL) {
	    strncat(output, item, size - len);
	}

	len += strlen(item) + 1;

    }

    return len + 1;

}