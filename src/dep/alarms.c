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
 * This source file contains the implementations of functions
 * handling raising and clearing of alarms.
 */

#include "../ptpd.h"

static const char* alarmStateToString(AlarmState state);

static void dispatchEvent(AlarmEntry *alarm);
static void dispatchAlarm(AlarmEntry *alarm);

static void getAlarmMessage(char *out, int count, AlarmEntry *alarm);

static void alarmHandler_log(AlarmEntry *alarm);
static void eventHandler_log(AlarmEntry *alarm);

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
getAlarmMessage(char *out, int count, AlarmEntry *alarm)
{

    memset(out, 0, count);

    switch(alarm->id) {
	
	case ALRM_PORT_STATE:
	    if(alarm->state == ALARM_UNSET) {
		return;
	    }
	    snprintf(out, count, ": Port state is %s", portState_getName(alarm->eventData.portDS.portState));
	    return;
	case ALRM_OFM_THRESHOLD:
	    if(alarm->state == ALARM_UNSET) {
		snprintf(out, count, ": Offset from master is now %.09f s, threshold is %d ns",
			timeInternalToDouble(&alarm->eventData.currentDS.offsetFromMaster),
			alarm->eventData.ofmAlarmThreshold);
		return;
	    }
	    snprintf(out, count, ": Offset from master is %.09f s, threshold is %d ns",
			timeInternalToDouble(&alarm->eventData.currentDS.offsetFromMaster),
			alarm->eventData.ofmAlarmThreshold);
	    return;
	case ALRM_OFM_SECONDS:
	    if(alarm->state == ALARM_UNSET) {
		    snprintf(out, count, ": Offset from master is now %.09f s, below 1 second",
			timeInternalToDouble(&alarm->eventData.currentDS.offsetFromMaster));
		return;
	    }
	    snprintf(out, count, ": Offset from master is %.09f s, above 1 second", 
			timeInternalToDouble(&alarm->eventData.currentDS.offsetFromMaster));
	    return;
	case ALRM_CLOCK_STEP:
	    snprintf(out, count, ": Clock stepped by %.09f s",
			timeInternalToDouble(&alarm->eventData.currentDS.offsetFromMaster));
	    return;
	case ALRM_NO_SYNC:
	    if(alarm->state == ALARM_UNSET) {
		return;
	    }
	    snprintf(out, count, ": Not receiving Sync");
	    return;
	case ALRM_NO_DELAY:
	    if(alarm->state == ALARM_UNSET) {
		return;
	    }
	    snprintf(out, count, ": Not receiving Delay Response");
	    return;
	case ALRM_MASTER_CHANGE:
	    return;
	case ALRM_NETWORK_FLT:
	    return;
	case ALRM_FAST_ADJ:
	    return;
	case ALRM_TIMEPROP_CHANGE:
	    return;
	case ALRM_DOMAIN_MISMATCH:
	    snprintf(out, count, ": Configured domain is %d, last seen %d", alarm->eventData.defaultDS.domainNumber,
			alarm->eventData.portDS.lastMismatchedDomain);
	    return;
	default:
	    return;
    }

}

static void
alarmHandler_log(AlarmEntry *alarm)
{

	char message[ALARM_MESSAGE_LENGTH+1];
	message[ALARM_MESSAGE_LENGTH] = '\0';
	getAlarmMessage(message, ALARM_MESSAGE_LENGTH, alarm);

	if(alarm->state == ALARM_SET) {
	    NOTICE("Alarm %s set%s\n", alarm->name, message);
	}

	if(alarm->state == ALARM_UNSET) {
	    NOTICE("Alarm %s cleared%s\n", alarm->name, message);
	}
}

static void
eventHandler_log(AlarmEntry *alarm)
{
	char message[ALARM_MESSAGE_LENGTH+1];
	message[ALARM_MESSAGE_LENGTH] = '\0';
	getAlarmMessage(message, ALARM_MESSAGE_LENGTH, alarm);

	NOTICE("Event %s triggered%s\n", alarm->name, message);
}

static void
dispatchEvent(AlarmEntry *alarm) {

	for(int i=0; alarm->handlers[i] != NULL; i++) {
	    alarm->handlers[i](alarm);
	}
	alarm->condition = FALSE;
}

static void
dispatchAlarm(AlarmEntry *alarm)
{
	for(int i=0; alarm->handlers[i] != NULL; i++) {
	    alarm->handlers[i](alarm);
	}
}


void
initAlarms(AlarmEntry* alarms, int count, void *userData)
{

    static AlarmEntry alarmTemplate[] = {

    /* short */	/* name */		/* desc */						/* enabled? */ /* id */		/* eventOnly */	/*handlers */
    { "STA", 	"PORT_STATE", 		"Port state different to expected value", 		FALSE, ALRM_PORT_STATE, 	FALSE, 		{alarmHandler_log}},
    { "OFM", 	"OFM_THRESHOLD", 	"Offset From Master outside threshold", 		FALSE, ALRM_OFM_THRESHOLD,	FALSE, 		{alarmHandler_log}},
    { "OFMS", 	"OFM_SECONDS", 		"Offset From Master above 1 second", 			FALSE, ALRM_OFM_SECONDS,	FALSE, 		{alarmHandler_log}},
    { "STEP", 	"CLOCK_STEP", 		"Clock was stepped", 					FALSE, ALRM_CLOCK_STEP,		TRUE, 		{eventHandler_log}},
    { "SYN", 	"NO_SYNC", 		"Clock is not receiving Sync messages",			FALSE, ALRM_NO_SYNC,		FALSE, 		{alarmHandler_log}},
    { "DLY", 	"NO_DELAY", 		"Clock is not receiving (p)Delay responses",		FALSE, ALRM_NO_DELAY,		FALSE, 		{alarmHandler_log}},
    { "MSTC", 	"MASTER_CHANGE", 	"Best master has changed",				FALSE, ALRM_MASTER_CHANGE,	TRUE, 		{eventHandler_log}},
    { "NWFL", 	"NETWORK_FAULT", 	"A network fault has occurred",				FALSE, ALRM_NETWORK_FLT,	FALSE, 		{alarmHandler_log}},
    { "FADJ", 	"FAST_ADJ", 		"Clock is being adjusted too fast", 			FALSE, ALRM_FAST_ADJ,		FALSE, 		{alarmHandler_log}},
    { "TPR", 	"TIMEPROP_CHANGE", 	"Time properties have changed",				FALSE, ALRM_TIMEPROP_CHANGE,	TRUE, 		{eventHandler_log}},
    { "DOM", 	"DOMAIN_MISMATCH", 	"Clock is receiving all messages from incorrect domain",FALSE, ALRM_DOMAIN_MISMATCH,	FALSE, 		{alarmHandler_log}}

    };

    if(count > ALRM_MAX) return;

    memset(alarms, 0, count * sizeof(AlarmEntry));
    memcpy(alarms, alarmTemplate, sizeof(alarmTemplate));

    for(int i = 0; i < count; i++) {
	alarms[i].userData = userData;
    }

}

/*
 * we are passing a generic pointer because eventually the alarm will have a "source" field,
 * that being PTP, NTP or any other subsystem, which may have separate routines to handle this
 */
void
configureAlarms(AlarmEntry *alarms, int count, void * userData)
{

    PtpClock *ptpClock = (PtpClock*) userData;

    for(int i = 0; i < count; i++) {
	alarms[i].minAge = ptpClock->rtOpts->alarmMinAge;
	alarms[i].enabled = ptpClock->rtOpts->alarmsEnabled;
#ifdef PTPD_SNMP
	if(ptpClock->rtOpts->snmpEnabled && ptpClock->rtOpts->snmpTrapsEnabled) {
	    DBG("SNMP alarm handler attached for %s\n", alarms[i].name);
	    alarms[i].handlers[1] = alarmHandler_snmp;
	} else {
	    alarms[i].handlers[1] = NULL;
	}
#endif
    }

}

void
enableAlarms(AlarmEntry* alarms, int count, Boolean enabled)
{
    for(int i = 0; i < count; i++) {
	alarms[i].enabled = enabled;
    }

}

void
setAlarmCondition(AlarmEntry *alarm, Boolean condition, PtpClock *ptpClock)
{
	if(!alarm->enabled) {
	    return;
	}

	Boolean change = condition ^ alarm->condition;

	/* if there is no change, exit */
	if(!change) {
	    return;
	}

	/* condition has cleared but event has not yet been processed - don't touch it */
	if(!condition && alarm->unhandled) {
	    return;
	}

	/* capture event data and time if condition is met */

	capturePtpEventData(&alarm->eventData, ptpClock, ptpClock->rtOpts);

	if(condition) {
	    getTime(&alarm->timeSet);
	} else {
	    getTime(&alarm->timeCleared);
	}

	DBG("Alarm %s condition set to %s\n", alarm->name, condition ? "TRUE" : "FALSE");

	alarm->age = 0;
	alarm->condition = condition;
	alarm->unhandled = TRUE;

}

void
capturePtpEventData(PtpEventData *eventData, PtpClock *ptpClock, RunTimeOpts *rtOpts)
{

    eventData->defaultDS = ptpClock->defaultDS;
    eventData->currentDS = ptpClock->currentDS;
    eventData->timePropertiesDS = ptpClock->timePropertiesDS;
    eventData->portDS = ptpClock->portDS;
    eventData->parentDS = ptpClock->parentDS;

    if(ptpClock->bestMaster != NULL) {
	eventData->bestMaster = *ptpClock->bestMaster;
    } else {
	memset(&eventData->bestMaster, 0, sizeof(ForeignMasterRecord));
    }

    eventData->ofmAlarmThreshold = rtOpts->ofmAlarmThreshold;
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
    AlarmState lastState;

    for(int i = 0; i < count; i++) {

	alarm = &alarms[i];

	if(!alarm->enabled) {
	    continue;
	}

	lastState = alarm->state;
	/* this is a one-off event */
	if(alarm->eventOnly) {
	    if(alarm->condition) {
		dispatchEvent(alarm);
	    }
	} else {
	/* this is an alarm */
	    if(!alarm->condition) {
		/* condition is false, alarm is cleared and aged out: unset */
		if(alarm->state == ALARM_CLEARED && alarm->age >= alarm->minAge ) {
		    alarm->state = ALARM_UNSET;
		    /* inform, run handlers */
		    dispatchAlarm(alarm);
		    clearTime(&alarm->timeSet);
		    clearTime(&alarm->timeCleared);
		/* condition is false and alarm was set - clear and wait for age out */
		} else if (alarm->state == ALARM_SET) {
		    alarm->state = ALARM_CLEARED;
		}
	    /* condition is true and age is 0: condition just changed */
	    } else if (alarm->age == 0) {
		alarm->state = ALARM_SET;
		/* react only if alarm was set from unset (don't inform multiple times until it fully clears */
		if(lastState == ALARM_UNSET) {
		    /* inform, run handlers */
		    dispatchAlarm(alarm);
		}
	    }
	    alarm->age+=ALARM_UPDATE_INTERVAL;
	}
	alarm->unhandled = FALSE;
    }

}

void
displayAlarms(AlarmEntry *alarms, int count)
{

    INFO("----------------------------------------------\n");
    INFO("              Alarm status: \n");
    INFO("----------------------------------------------\n");
    INFO("ALARM NAME\t| STATE    | DESCRIPTION\n");
    INFO("----------------------------------------------\n");
    for(int i=0; i < count; i++) {
	if(alarms[i].eventOnly) continue;
	INFO("%-15s\t| %-8s | %s\n",  alarms[i].name, 
				    alarmStateToString(alarms[i].state), 
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

    if(output != NULL) {
	memset(output, 0, size);
    }

    for(i=0; i < count; i++) {
	memset(item, 0, 8);
	alarm = &alarms[i];

	if(alarm->state == ALARM_UNSET) {
	    continue;
	}

	strncpy(item, alarm->shortName, 4);
	snprintf(item + strlen(item), 5, "[%s] ", alarm->state == ALARM_SET ? "!" : ".");

	if(output != NULL) {
	    strncat(output, item, size - len);
	}

	len += strlen(item) + 1;

    }

    return len + 1;

}
