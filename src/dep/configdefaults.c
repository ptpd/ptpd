/* Copyright (c) 2013-2015 Wojciech Owczarek,
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
 * @file   configtemplates.c
 * @date   Mon Oct 12 03:25:32 2015
 *
 * @brief  Definitions of configuration templates
 *
 */

#include "../ptpd.h"

#define CONFIG_ISTRUE(key) \
	(iniparser_getboolean(dict,key,FALSE)==TRUE)

#define CONFIG_ISSET(key) \
	(strcmp(iniparser_getstring(dict, key, ""),"") != 0)

#define IS_QUIET()\
	( CONFIG_ISTRUE("%quiet%:%quiet%") )

/*
 * static definition of configuration templates - maximum 100 settings per template,
 * as defined in configtemplates.h. Template for a template below.
 */

/* =============== configuration templates begin ===============*/

static const ConfigTemplate configTemplates[] = {

    { "g8265-e2e-master", "ITU-T G.8265.1 (Telecom profile) unicast master", {
	{"ptpengine:preset", "masteronly"},
	{"ptpengine:ip_mode", "unicast"},
	{"ptpengine:unicast_negotiation", "y"},
	{"ptpengine:domain", "4"},
	{"ptpengine:disable_bmca", "y"},
	{"ptpengine:delay_mechanism", "E2E"},
	{"ptpengine:log_sync_interval", "-6"},
	{"ptpengine:log_delayreq_interval", "-6"},
	{"ptpengine:announce_receipt_timeout", "3"},
	{"ptpengine:priority1", "128"},
	{"ptpengine:priority2", "128"},
	{NULL}}
    },

    { "g8265-e2e-slave", "ITU-T G.8265.1 (Telecom profile) unicast slave", {
	{"ptpengine:preset", "slaveonly"},
	{"ptpengine:ip_mode", "unicast"},
	{"ptpengine:unicast_negotiation", "y"},
	{"ptpengine:domain", "4"},
	{"ptpengine:delay_mechanism", "E2E"},
	{"ptpengine:announce_receipt_timeout", "3"},
	{NULL}}
    },

    { "layer2-p2p-master", "Layer 2 master using Peer to Peer", {
	{"ptpengine:transport","ethernet"},
	{"ptpengine:delay_mechanism","P2P"},
	{"ptpengine:preset","masteronly"},
//	{"",""},
	{NULL}}
    },

    { "layer2-p2p-slave", "Layer 2 slave running End to End", {
	{"ptpengine:transport","ethernet"},
	{"ptpengine:delay_mechanism","E2E"},
	{"ptpengine:preset","slaveonly"},
//	{"",""},
	{NULL}}
    },

    { "valid-utc-properties", "Basic property set to announce valid UTC, UTC offset and leap seconds", {
	{"ptpengine:ptp_timescale","PTP"},
	{"ptpengine:time_traceable","y"},
	{"ptpengine:frequency_traceable","y"},
	{"clock:leap_seconds_file", DATADIR"/"PACKAGE_NAME"/leap-seconds.list"},
	/*
	 * UTC offset value and UTC offset valid flag
	 * will be announced if leap file is parsed and up to date
	 */
	{NULL}}
    },

    { "full-logging", "Enable logging for all facilities (statistics, status, log file)", {
	{"global:log_status", "y"},
	{"global:status_file", "/var/run/ptpd2.status"},
	{"global:statistics_log_interval", "1"},
	{"global:lock_file", "/var/run/ptpd2.pid"},
	{"global:log_statistics", "y"},
	{"global:statistics_file", "/var/log/ptpd2.statistics"},
//	{"global:statistics_file_truncate", "n"},
	{"global:log_file", "/var/log/ptpd2.log"},
	{"global:statistics_timestamp_format", "both"},
	{NULL}}
    },

    { "full-logging-instance", "Logging for all facilities using 'instance' variable which the user should provide", {
	{"global:log_status", "y"},
	{"global:status_file", "@rundir@/ptpd2.@instance@.status"},
	{"global:statistics_log_interval", "1"},
	{"global:lock_file", "@rundir@/ptpd2.@instance@.pid"},
	{"global:log_statistics", "y"},
	{"global:statistics_file", "@logdir@/ptpd2.@instance@.statistics"},
//	{"global:statistics_file_truncate", "n"},
	{"global:log_file", "@logdir@/ptpd2.@instance@.log"},
	{"global:statistics_timestamp_format", "both"},
	{NULL}}
    },

    {NULL}
};

/* =============== configuration templates end ===============*/

/* template of a template looks like this... */

/*
    { "template-name", "template description", {
	{"section:key","value"},
	{"",""},
	{NULL}}
    },

    { "", "", {
	{"",""},
	{"",""},
	{NULL}}
    },


*/

/* Load all rtOpts defaults */
void
loadDefaultSettings( RunTimeOpts* rtOpts )
{

	/* Wipe the memory first to avoid unconsistent behaviour - no need to set Boolean to FALSE, int to 0 etc. */
	memset(rtOpts, 0, sizeof(RunTimeOpts));

	rtOpts->logAnnounceInterval = DEFAULT_ANNOUNCE_INTERVAL;
	rtOpts->logSyncInterval = DEFAULT_SYNC_INTERVAL;
	rtOpts->logMinPdelayReqInterval = DEFAULT_PDELAYREQ_INTERVAL;
	rtOpts->clockQuality.clockAccuracy = DEFAULT_CLOCK_ACCURACY;
	rtOpts->clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
	rtOpts->clockQuality.offsetScaledLogVariance = DEFAULT_CLOCK_VARIANCE;
	rtOpts->priority1 = DEFAULT_PRIORITY1;
	rtOpts->priority2 = DEFAULT_PRIORITY2;
	rtOpts->domainNumber = DEFAULT_DOMAIN_NUMBER;
	rtOpts->portNumber = NUMBER_PORTS;

	rtOpts->anyDomain = FALSE;

	rtOpts->transport = UDP_IPV4;

	/* timePropertiesDS */
	rtOpts->timeProperties.currentUtcOffsetValid = DEFAULT_UTC_VALID;
	rtOpts->timeProperties.currentUtcOffset = DEFAULT_UTC_OFFSET;
	rtOpts->timeProperties.timeSource = INTERNAL_OSCILLATOR;
	rtOpts->timeProperties.timeTraceable = FALSE;
	rtOpts->timeProperties.frequencyTraceable = FALSE;
	rtOpts->timeProperties.ptpTimescale = TRUE;

	rtOpts->ipMode = IPMODE_MULTICAST;
	rtOpts->dot1AS = FALSE;

	rtOpts->disableUdpChecksums = TRUE;

	rtOpts->unicastNegotiation = FALSE;
	rtOpts->unicastNegotiationListening = FALSE;
	rtOpts->disableBMCA = FALSE;
	rtOpts->unicastGrantDuration = 300;
	rtOpts->unicastAcceptAny = FALSE;
	rtOpts->unicastPortMask = 0;

	rtOpts->noAdjust = NO_ADJUST;  // false
	rtOpts->logStatistics = TRUE;
	rtOpts->statisticsTimestamp = TIMESTAMP_DATETIME;

	rtOpts->periodicUpdates = FALSE; /* periodically log a status update */

	/* Deep display of all packets seen by the daemon */
	rtOpts->displayPackets = FALSE;

	rtOpts->s = DEFAULT_DELAY_S;
	rtOpts->inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
	rtOpts->outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
	rtOpts->max_foreign_records = DEFAULT_MAX_FOREIGN_RECORDS;
	rtOpts->nonDaemon = FALSE;

	/*
	 * defaults for new options
	 */
	rtOpts->ignore_delayreq_interval_master = FALSE;
	rtOpts->do_IGMP_refresh = TRUE;
	rtOpts->useSysLog       = FALSE;
	rtOpts->announceReceiptTimeout  = DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
#ifdef RUNTIME_DEBUG
	rtOpts->debug_level = LOG_INFO;			/* by default debug messages as disabled, but INFO messages and below are printed */
#endif
	rtOpts->ttl = 64;
	rtOpts->delayMechanism   = DEFAULT_DELAY_MECHANISM;
	rtOpts->noResetClock     = DEFAULT_NO_RESET_CLOCK;
	rtOpts->portDisabled	 = FALSE;
	rtOpts->stepOnce	 = FALSE;
	rtOpts->stepForce	 = FALSE;
#ifdef HAVE_LINUX_RTC_H
	rtOpts->setRtc		 = FALSE;
#endif /* HAVE_LINUX_RTC_H */

	rtOpts->clearCounters = FALSE;
	rtOpts->statisticsLogInterval = 0;

	rtOpts->initial_delayreq = DEFAULT_DELAYREQ_INTERVAL;
	rtOpts->logMinDelayReqInterval = DEFAULT_DELAYREQ_INTERVAL;
	rtOpts->autoDelayReqInterval = TRUE;
	rtOpts->masterRefreshInterval = 60;

	/* maximum values for unicast negotiation */
    	rtOpts->logMaxPdelayReqInterval = 5;
	rtOpts->logMaxDelayReqInterval = 5;
	rtOpts->logMaxSyncInterval = 5;
	rtOpts->logMaxAnnounceInterval = 5;

	rtOpts->drift_recovery_method = DRIFT_KERNEL;
	strncpy(rtOpts->lockDirectory, DEFAULT_LOCKDIR, PATH_MAX);
	strncpy(rtOpts->driftFile, DEFAULT_DRIFTFILE, PATH_MAX);
/*	strncpy(rtOpts->lockFile, DEFAULT_LOCKFILE, PATH_MAX); */
	rtOpts->autoLockFile = FALSE;
	rtOpts->snmpEnabled = FALSE;
	rtOpts->snmpTrapsEnabled = FALSE;
	rtOpts->alarmsEnabled = FALSE;
	rtOpts->alarmInitialDelay = 0;
	rtOpts->alarmMinAge = 30;
	/* This will only be used if the "none" preset is configured */
#ifndef PTPD_SLAVE_ONLY
	rtOpts->slaveOnly = FALSE;
#else
	rtOpts->slaveOnly = TRUE;
#endif /* PTPD_SLAVE_ONLY */
	/* Otherwise default to slave only via the preset */
	rtOpts->selectedPreset = PTP_PRESET_SLAVEONLY;
	rtOpts->pidAsClockId = FALSE;

	strncpy(rtOpts->portDescription,"ptpd", sizeof(rtOpts->portDescription));

	/* highest possible */
	rtOpts->logLevel = LOG_ALL;

	/* ADJ_FREQ_MAX by default */
	rtOpts->servoMaxPpb = ADJ_FREQ_MAX / 1000;
	/* kP and kI are scaled to 10000 and are gains now - values same as originally */
	rtOpts->servoKP = 0.1;
	rtOpts->servoKI = 0.001;

	rtOpts->servoDtMethod = DT_CONSTANT;
	/* when measuring dT, use a maximum of 5 sync intervals (would correspond to avg 20% discard rate) */
	rtOpts->servoMaxdT = 5.0;

	/* disabled by default */
	rtOpts->announceTimeoutGracePeriod = 0;

	/* currentUtcOffsetValid compatibility flags */
	rtOpts->alwaysRespectUtcOffset = TRUE;
	rtOpts->preferUtcValid = FALSE;
	rtOpts->requireUtcValid = FALSE;

	/* Try 46 for expedited forwarding */
	rtOpts->dscpValue = 0;

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined (__QNXNTO__)
	rtOpts-> cpuNumber = -1;
#endif /* (linux && HAVE_SCHED_H) || HAVE_SYS_CPUSET_H*/

#ifdef PTPD_STATISTICS

	rtOpts->oFilterMSConfig.enabled = FALSE;
	rtOpts->oFilterMSConfig.discard = TRUE;
	rtOpts->oFilterMSConfig.autoTune = TRUE;
	rtOpts->oFilterMSConfig.stepDelay = FALSE;
	rtOpts->oFilterMSConfig.alwaysFilter = FALSE;
	rtOpts->oFilterMSConfig.stepThreshold = 1000000;
	rtOpts->oFilterMSConfig.stepLevel = 500000;
	rtOpts->oFilterMSConfig.capacity = 20;
	rtOpts->oFilterMSConfig.threshold = 1.0;
	rtOpts->oFilterMSConfig.weight = 1;
	rtOpts->oFilterMSConfig.minPercent = 20;
	rtOpts->oFilterMSConfig.maxPercent = 95;
	rtOpts->oFilterMSConfig.thresholdStep = 0.1;
	rtOpts->oFilterMSConfig.minThreshold = 0.1;
	rtOpts->oFilterMSConfig.maxThreshold = 5.0;
	rtOpts->oFilterMSConfig.delayCredit = 200;
	rtOpts->oFilterMSConfig.creditIncrement = 10;
	rtOpts->oFilterMSConfig.maxDelay = 1500;

	rtOpts->oFilterSMConfig.enabled = FALSE;
	rtOpts->oFilterSMConfig.discard = TRUE;
	rtOpts->oFilterSMConfig.autoTune = TRUE;
	rtOpts->oFilterSMConfig.stepDelay = FALSE;
	rtOpts->oFilterSMConfig.alwaysFilter = FALSE;
	rtOpts->oFilterSMConfig.stepThreshold = 1000000;
	rtOpts->oFilterSMConfig.stepLevel = 500000;
	rtOpts->oFilterSMConfig.capacity = 20;
	rtOpts->oFilterSMConfig.threshold = 1.0;
	rtOpts->oFilterSMConfig.weight = 1;
	rtOpts->oFilterSMConfig.minPercent = 20;
	rtOpts->oFilterSMConfig.maxPercent = 95;
	rtOpts->oFilterSMConfig.thresholdStep = 0.1;
	rtOpts->oFilterSMConfig.minThreshold = 0.1;
	rtOpts->oFilterSMConfig.maxThreshold = 5.0;
	rtOpts->oFilterSMConfig.delayCredit = 200;
	rtOpts->oFilterSMConfig.creditIncrement = 10;
	rtOpts->oFilterSMConfig.maxDelay = 1500;

	rtOpts->filterMSOpts.enabled = FALSE;
	rtOpts->filterMSOpts.filterType = FILTER_MIN;
	rtOpts->filterMSOpts.windowSize = 4;
	rtOpts->filterMSOpts.windowType = WINDOW_SLIDING;

	rtOpts->filterSMOpts.enabled = FALSE;
	rtOpts->filterSMOpts.filterType = FILTER_MIN;
	rtOpts->filterSMOpts.windowSize = 4;
	rtOpts->filterSMOpts.windowType = WINDOW_SLIDING;

	/* How often refresh statistics (seconds) */
	rtOpts->statsUpdateInterval = 30;
	/* Servo stability detection settings follow */
	rtOpts->servoStabilityDetection = FALSE;
	/* Stability threshold (ppb) - observed drift std dev value considered stable */
	rtOpts->servoStabilityThreshold = 10;
	/* How many consecutive statsUpdateInterval periods of observed drift std dev within threshold  means stable servo */
	rtOpts->servoStabilityPeriod = 1;
	/* How many minutes without servo stabilisation means servo has not stabilised */
	rtOpts->servoStabilityTimeout = 10;
	/* How long to wait for one-way delay prefiltering */
	rtOpts->calibrationDelay = 0;
	/* if set to TRUE and maxDelay is defined, only check against threshold if servo is stable */
	rtOpts->maxDelayStableOnly = FALSE;
	/* if set to non-zero, reset slave if more than this amount of consecutive delay measurements was above maxDelay */
	rtOpts->maxDelayMaxRejected = 0;
#endif

	/* status file options */
	rtOpts->statusFileUpdateInterval = 1;

	rtOpts->ofmAlarmThreshold = 0;

	/* panic mode options */
	rtOpts->enablePanicMode = FALSE;
	rtOpts->panicModeDuration = 2;
	rtOpts->panicModeExitThreshold = 0;

	/* full network reset after 5 times in listening */
	rtOpts->maxListen = 5;

	rtOpts->panicModeReleaseClock = FALSE;
	rtOpts->ntpOptions.enableEngine = FALSE;
	rtOpts->ntpOptions.enableControl = FALSE;
	rtOpts->ntpOptions.enableFailover = FALSE;
	rtOpts->ntpOptions.failoverTimeout = 120;
	rtOpts->ntpOptions.checkInterval = 15;
	rtOpts->ntpOptions.keyId = 0;
	strncpy(rtOpts->ntpOptions.hostAddress,"localhost",MAXHOSTNAMELEN); 	/* not configurable, but could be */
	rtOpts->preferNTP = FALSE;

	rtOpts->leapSecondPausePeriod = 5;
	/* by default, announce the leap second 12 hours before the event:
	 * Clause 9.4 paragraph 5 */
	rtOpts->leapSecondNoticePeriod = 43200;
	rtOpts->leapSecondHandling = LEAP_ACCEPT;
	rtOpts->leapSecondSmearPeriod = 86400;

/* timing domain */
	rtOpts->idleTimeout = 120; /* idle timeout */
	rtOpts->electionDelay = 15; /* anti-flapping delay */

/* Log file settings */

	rtOpts->statisticsLog.logID = "statistics";
	rtOpts->statisticsLog.openMode = "a+";
	rtOpts->statisticsLog.logFP = NULL;
	rtOpts->statisticsLog.truncateOnReopen = FALSE;
	rtOpts->statisticsLog.unlinkOnClose = FALSE;
	rtOpts->statisticsLog.maxSize = 0;

	rtOpts->recordLog.logID = "record";
	rtOpts->recordLog.openMode = "a+";
	rtOpts->recordLog.logFP = NULL;
	rtOpts->recordLog.truncateOnReopen = FALSE;
	rtOpts->recordLog.unlinkOnClose = FALSE;
	rtOpts->recordLog.maxSize = 0;

	rtOpts->eventLog.logID = "log";
	rtOpts->eventLog.openMode = "a+";
	rtOpts->eventLog.logFP = NULL;
	rtOpts->eventLog.truncateOnReopen = FALSE;
	rtOpts->eventLog.unlinkOnClose = FALSE;
	rtOpts->eventLog.maxSize = 0;

	rtOpts->statusLog.logID = "status";
	rtOpts->statusLog.openMode = "w";
	strncpy(rtOpts->statusLog.logPath, DEFAULT_STATUSFILE, PATH_MAX);
	rtOpts->statusLog.logFP = NULL;
	rtOpts->statusLog.truncateOnReopen = FALSE;
	rtOpts->statusLog.unlinkOnClose = TRUE;

/* Management message support settings */
	rtOpts->managementEnabled = TRUE;
	rtOpts->managementSetEnable = FALSE;

/* IP ACL settings */

	rtOpts->timingAclEnabled = FALSE;
	rtOpts->managementAclEnabled = FALSE;
	rtOpts->timingAclOrder = ACL_DENY_PERMIT;
	rtOpts->managementAclOrder = ACL_DENY_PERMIT;

	// by default we don't check Sync message sequence continuity
	rtOpts->syncSequenceChecking = FALSE;
	rtOpts->clockUpdateTimeout = 0;

}

/* The PtpEnginePreset structure for reference:

typedef struct {

    char* presetName;
    Boolean slaveOnly;
    Boolean noAdjust;
    UInteger8_option clockClass;

} PtpEnginePreset;
*/

PtpEnginePreset
getPtpPreset(int presetNumber, RunTimeOpts* rtOpts)
{

	PtpEnginePreset ret;

	memset(&ret,0,sizeof(ret));

	switch(presetNumber) {

	case PTP_PRESET_SLAVEONLY:
		ret.presetName="slaveonly";
		ret.slaveOnly = TRUE;
		ret.noAdjust = FALSE;
		ret.clockClass.minValue = SLAVE_ONLY_CLOCK_CLASS;
		ret.clockClass.maxValue = SLAVE_ONLY_CLOCK_CLASS;
		ret.clockClass.defaultValue = SLAVE_ONLY_CLOCK_CLASS;
		break;
	case PTP_PRESET_MASTERSLAVE:
		ret.presetName = "masterslave";
		ret.slaveOnly = FALSE;
		ret.noAdjust = FALSE;
		ret.clockClass.minValue = 128;
		ret.clockClass.maxValue = 254;
		ret.clockClass.defaultValue = DEFAULT_CLOCK_CLASS;
		break;
	case PTP_PRESET_MASTERONLY:
		ret.presetName = "masteronly";
		ret.slaveOnly = FALSE;
		ret.noAdjust = TRUE;
		ret.clockClass.minValue = 0;
		ret.clockClass.maxValue = 127;
		ret.clockClass.defaultValue = DEFAULT_CLOCK_CLASS__APPLICATION_SPECIFIC_TIME_SOURCE;
		break;
	default:
		ret.presetName = "none";
		ret.slaveOnly = rtOpts->slaveOnly;
		ret.noAdjust = rtOpts->noAdjust;
		ret.clockClass.minValue = 0;
		ret.clockClass.maxValue = 255;
		ret.clockClass.defaultValue = rtOpts->clockQuality.clockClass;
	}

	return ret;
}

/* Apply template search from dictionary src to dictionary dst */
static int
applyTemplateFromDictionary(dictionary *dict, dictionary *src, char *search, int overwrite) {

	char *key, *value, *pos;
	int i = 0;
	int found;

	/* -1 on not found, 0+ on actual applies */
	found = -1;

	for(i=0; i<src->n; i++) {
	    key = src->key[i];
	    value = src->val[i];
	    pos = strstr(key, ":");
	    if(value != NULL  && pos != NULL) {
		*pos =  '\0';
		pos++;
		if(!strcmp(search, key) && (strstr(pos,":") != NULL)) {
		    if(found < 0) found = 0;
		    if( overwrite || !CONFIG_ISSET(pos)) {
			DBG("template %s setting %s to %s\n", search, pos, value);
			dictionary_set(dict, pos, value);
			found++;
		    }
		}
		/* reverse the damage - no need for strdup() */
		pos--;
		*pos = ':';
	    }
	}

	return found;

}

static void loadFileList(dictionary *dict, char *list) {

    char* stash;
    char* text_;
    char* text__;
    char *filename;

    if(dict == NULL) return;
    if(!strlen(list)) return;

    text_=strdup(list);

    for(text__=text_;; text__=NULL) {
	filename=strtok_r(text__,", ;\t",&stash);
	if(filename==NULL) break;
	if(!iniparser_merge_file(dict, filename,1)) {
	    ERROR("Could not load template file %s\n", filename);
	} else {
	    INFO("Loaded template file %s\n",filename);
	}
    }

    if(text_ != NULL) {
	free(text_);
    }

}

/* split list to tokens, look for each template and apply it if found */
int
applyConfigTemplates(dictionary *target, char *templateNames, char *files) {

    ConfigTemplate *template = NULL;
    TemplateOption *option = NULL;
    char *templateName = NULL;
    int iFound = -1;
    int fFound = -1;

    dictionary *fileDict;
    dictionary *dict;

    char* stash;
    char* text_;
    char* text__;

    if(target == NULL) {
	return 0;
    }

    if(!strlen(templateNames)) return 0;

    fileDict = dictionary_new(0);
    dict = dictionary_new(0);

    loadFileList(fileDict, DEFAULT_TEMPLATE_FILE);
    loadFileList(fileDict, files);

    text_=strdup(templateNames);

    for(text__=text_;; text__=NULL) {

	iFound = -1;
	fFound = -1;

	/* apply from built-in templates */
	templateName=strtok_r(text__,", ;\t",&stash);
	if(templateName==NULL) break;

	template = (ConfigTemplate*)configTemplates;
	while(template->name != NULL) {
	    if(!strcmp(template->name, templateName)) {
		if(iFound < 0) iFound = 0;
		DBG("Loading built-in config template %s\n", template->name);
		option = (TemplateOption*)template->options;
		    while(option->name != NULL) {
			dictionary_set(dict, option->name, option->value);
			DBG("----> %s = %s",option->name, option->value);
			option++;
			iFound++;
		    }
		break;
	    }
	    template++;
	}

	/* apply from previously loaded files */
	fFound = applyTemplateFromDictionary(dict, fileDict, templateName, 1);

	if (!IS_QUIET()) {
	    if((iFound < 0) && (fFound < 0)) {
		WARNING("Configuration template \"%s\" not found\n", templateName);
	    } else {
		if(iFound < 0) iFound = 0;
		if(fFound < 0) fFound = 0;
		NOTICE("Applied configuration template \"%s\" (%d matches)\n", templateName,
			iFound + fFound);
	    }
	}

    }

    if(text_ != NULL) {
	free(text_);
    }

    /* preserve existing settings */
    dictionary_merge(dict, target, 0 , !IS_QUIET(), NULL);
    dictionary_del(&fileDict);
    dictionary_del(&dict);
    return 0;

}

/* dump the list of templates provided */
void
dumpConfigTemplates() {

    ConfigTemplate *template = NULL;
    TemplateOption *option = NULL;
	printf("\n");
	template = (ConfigTemplate*)configTemplates;
	while(template->name != NULL) {
		printf("   Template: %s\n", template->name);
		printf("Description: %s\n\n", template->description);
		option = (TemplateOption*)template->options;
		    while(option->name != NULL) {
			printf("\t\t%s=\"%s\"\n", option->name, option->value);
			option++;
		    }
	    template++;
	    printf("\n-\n\n");
	}

}
