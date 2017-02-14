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
	{"ptpengine:transport_mode", "unicast"},
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
	{"ptpengine:transport_mode", "unicast"},
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
	{"global:status_file", "/var/run/ptpd.status"},
	{"global:statistics_log_interval", "1"},
	{"global:lock_file", "/var/run/ptpd.pid"},
	{"global:log_statistics", "y"},
	{"global:statistics_file", "/var/log/ptpd.statistics"},
//	{"global:statistics_file_truncate", "n"},
	{"global:log_file", "/var/log/ptpd.log"},
	{"global:statistics_timestamp_format", "both"},
	{NULL}}
    },

    { "full-logging-instance", "Logging for all facilities using 'instance' variable which the user should provide", {
	{"global:log_status", "y"},
	{"global:status_file", "@rundir@/ptpd.@instance@.status"},
	{"global:statistics_log_interval", "1"},
	{"global:lock_file", "@rundir@/ptpd.@instance@.pid"},
	{"global:log_statistics", "y"},
	{"global:statistics_file", "@logdir@/ptpd.@instance@.statistics"},
//	{"global:statistics_file_truncate", "n"},
	{"global:log_file", "@logdir@/ptpd.@instance@.log"},
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

/* Load all global defaults */
void
loadDefaultSettings( GlobalConfig* global )
{

	/* Wipe the memory first to avoid unconsistent behaviour - no need to set Boolean to FALSE, int to 0 etc. */
	memset(global, 0, sizeof(GlobalConfig));

	global->logAnnounceInterval = DEFAULT_ANNOUNCE_INTERVAL;
	global->logSyncInterval = DEFAULT_SYNC_INTERVAL;
	global->logMinPdelayReqInterval = DEFAULT_PDELAYREQ_INTERVAL;
	global->clockQuality.clockAccuracy = DEFAULT_CLOCK_ACCURACY;
	global->clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
	global->clockQuality.offsetScaledLogVariance = DEFAULT_CLOCK_VARIANCE;
	global->priority1 = DEFAULT_PRIORITY1;
	global->priority2 = DEFAULT_PRIORITY2;
	global->domainNumber = DEFAULT_DOMAIN_NUMBER;
	global->portNumber = NUMBER_PORTS;

	global->anyDomain = FALSE;

	global->networkProtocol = TT_FAMILY_IPV4;

	/* timePropertiesDS */
	global->timeProperties.currentUtcOffsetValid = DEFAULT_UTC_VALID;
	global->timeProperties.currentUtcOffset = DEFAULT_UTC_OFFSET;
	global->timeProperties.timeSource = INTERNAL_OSCILLATOR;
	global->timeProperties.timeTraceable = FALSE;
	global->timeProperties.frequencyTraceable = FALSE;
	global->timeProperties.ptpTimescale = TRUE;

	global->transportMode = TMODE_MC;
	global->dot1AS = FALSE;
	global->bindToInterface = FALSE;

	global->disableUdpChecksums = TRUE;

	global->unicastNegotiation = FALSE;
	global->unicastNegotiationListening = FALSE;
	global->disableBMCA = FALSE;
	global->unicastGrantDuration = 300;
	global->unicastAcceptAny = FALSE;
	global->unicastPortMask = 0;

	global->noAdjust = NO_ADJUST;  // false
	global->logStatistics = TRUE;
	global->statisticsTimestamp = TIMESTAMP_DATETIME;

	global->periodicUpdates = FALSE; /* periodically log a status update */

	/* Deep display of all packets seen by the daemon */
	global->displayPackets = FALSE;

	global->s = DEFAULT_DELAY_S;
	global->inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
	global->outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
	global->fmrCapacity = DEFAULT_MAX_FOREIGN_RECORDS;
	global->nonDaemon = FALSE;

	/*
	 * defaults for new options
	 */
	global->ignore_delayreq_interval_master = FALSE;
	global->refreshMulticast = TRUE;
	global->useSysLog       = FALSE;
	global->announceReceiptTimeout  = DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
#ifdef RUNTIME_DEBUG
	global->debug_level = LOG_INFO;			/* by default debug messages as disabled, but INFO messages and below are printed */
#endif
	global->ttl = 64;
	global->ipv6Scope = IPV6_SCOPE_GLOBAL;

	global->delayMechanism   = DEFAULT_DELAY_MECHANISM;
	global->noResetClock     = DEFAULT_NO_RESET_CLOCK;
	global->portDisabled	 = FALSE;
	global->stepOnce	 = FALSE;
	global->stepForce	 = FALSE;
	global->setRtc		 = FALSE;

	global->clearCounters = FALSE;
	global->statisticsLogInterval = 0;

	global->initial_delayreq = DEFAULT_DELAYREQ_INTERVAL;
	global->logMinDelayReqInterval = DEFAULT_DELAYREQ_INTERVAL;
	global->autoDelayReqInterval = TRUE;
	global->masterRefreshInterval = 60;

	/* maximum values for unicast negotiation */
    	global->logMaxPdelayReqInterval = 5;
	global->logMaxDelayReqInterval = 5;
	global->logMaxSyncInterval = 5;
	global->logMaxAnnounceInterval = 5;

	strncpy(global->lockDirectory, DEFAULT_LOCKDIR, PATH_MAX);
	strncpy(global->driftFile, DEFAULT_DRIFTFILE, PATH_MAX);
	strncpy(global->frequencyDir, "/etc", PATH_MAX);
/*	strncpy(global->lockFile, DEFAULT_LOCKFILE, PATH_MAX); */
	global->autoLockFile = FALSE;
	global->snmpEnabled = FALSE;
	global->snmpTrapsEnabled = FALSE;
	global->alarmsEnabled = FALSE;
	global->alarmInitialDelay = 0;
	global->alarmMinAge = 30;
	/* This will only be used if the "none" preset is configured */
#ifndef PTPD_SLAVE_ONLY
	global->slaveOnly = FALSE;
#else
	global->slaveOnly = TRUE;
#endif /* PTPD_SLAVE_ONLY */
	/* Otherwise default to slave only via the preset */
	global->selectedPreset = PTP_PRESET_SLAVEONLY;
	global->pidAsClockId = FALSE;

	strncpy(global->portDescription,"ptpd", sizeof(global->portDescription));

	/* highest possible */
	global->logLevel = LOG_ALL;

	/* ADJ_FREQ_MAX by default */
	global->servoMaxPpb = ADJ_FREQ_MAX / 1000;
	global->servoMaxPpb_hw = 2000000 / 1000;
	/* kP and kI are scaled to 10000 and are gains now - values same as originally */
	global->servoKP = 0.1;
	global->servoKI = 0.001;

	global->servoKI_hw = 0.3;
	global->servoKP_hw = 0.7;

	global->stableAdev = 200;
	global->stableAdev_hw = 50;

	global->unstableAdev = 2000;
	global->unstableAdev_hw = 500;

	global->lockedAge = 60;
	global->lockedAge_hw = 180;

	global->holdoverAge = 600;
	global->holdoverAge_hw = 1800;

	global->adevPeriod = 10;

	global->negativeStep = FALSE;
	global->negativeStep_hw = FALSE;

	global->servoDtMethod = DT_CONSTANT;

	global->storeToFile = TRUE;

	global->clockUpdateInterval = CLOCKDRIVER_UPDATE_INTERVAL;
	global->clockSyncRate = CLOCKDRIVER_SYNC_RATE;
	global->clockFailureDelay = 10;
	global->clockStrictSync = TRUE;
	global->clockMinStep = 500;
	global->clockCalibrationTime = 10;

	/* when measuring dT, use a maximum of 5 sync intervals (would correspond to avg 20% discard rate) */
	global->servoMaxdT = 5.0;

	/* inter-clock sync filter options */

	global->clockStatFilterEnable = TRUE;
	global->clockStatFilterWindowSize = 0; /* this will revert to clock sync rate */
	global->clockStatFilterWindowType = WINDOW_SLIDING;
	global->clockStatFilterType = FILTER_MEDIAN;

	global->clockOutlierFilterEnable = FALSE;
	global->clockOutlierFilterWindowSize = 200;
	global->clockOutlierFilterDelay = 100;
	global->clockOutlierFilterCutoff = 5.0;
	global->clockOutlierFilterBlockTimeout = 15; /* maximum filter blocking time before it is reset */

	/* disabled by default */
	global->announceTimeoutGracePeriod = 0;

	/* currentUtcOffsetValid compatibility flags */
	global->alwaysRespectUtcOffset = TRUE;
	global->preferUtcValid = FALSE;
	global->requireUtcValid = FALSE;

	/* Try 46 for expedited forwarding */
	global->dscpValue = 0;

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined (__QNXNTO__)
	global-> cpuNumber = -1;
#endif /* (linux && HAVE_SCHED_H) || HAVE_SYS_CPUSET_H*/

	global->oFilterMSConfig.enabled = FALSE;
	global->oFilterMSConfig.discard = TRUE;
	global->oFilterMSConfig.autoTune = TRUE;
	global->oFilterMSConfig.stepDelay = FALSE;
	global->oFilterMSConfig.alwaysFilter = FALSE;
	global->oFilterMSConfig.stepThreshold = 1000000;
	global->oFilterMSConfig.stepLevel = 500000;
	global->oFilterMSConfig.capacity = 20;
	global->oFilterMSConfig.threshold = 2.0;
	global->oFilterMSConfig.weight = 1;
	global->oFilterMSConfig.minPercent = 20;
	global->oFilterMSConfig.maxPercent = 95;
	global->oFilterMSConfig.thresholdStep = 0.1;
	global->oFilterMSConfig.minThreshold = 1.5;
	global->oFilterMSConfig.maxThreshold = 5.0;
	global->oFilterMSConfig.delayCredit = 200;
	global->oFilterMSConfig.creditIncrement = 10;
	global->oFilterMSConfig.maxDelay = 1500;

	global->oFilterSMConfig.enabled = FALSE;
	global->oFilterSMConfig.discard = TRUE;
	global->oFilterSMConfig.autoTune = TRUE;
	global->oFilterSMConfig.stepDelay = FALSE;
	global->oFilterSMConfig.alwaysFilter = FALSE;
	global->oFilterSMConfig.stepThreshold = 1000000;
	global->oFilterSMConfig.stepLevel = 500000;
	global->oFilterSMConfig.capacity = 20;
	global->oFilterSMConfig.threshold = 2.0;
	global->oFilterSMConfig.weight = 1;
	global->oFilterSMConfig.minPercent = 20;
	global->oFilterSMConfig.maxPercent = 95;
	global->oFilterSMConfig.thresholdStep = 0.1;
	global->oFilterSMConfig.minThreshold = 1.5;
	global->oFilterSMConfig.maxThreshold = 5.0;
	global->oFilterSMConfig.delayCredit = 200;
	global->oFilterSMConfig.creditIncrement = 10;
	global->oFilterSMConfig.maxDelay = 1500;

	global->filterMSOpts.enabled = FALSE;
	global->filterMSOpts.filterType = FILTER_MEDIAN;
	global->filterMSOpts.windowSize = 5;
	global->filterMSOpts.windowType = WINDOW_SLIDING;
	global->filterMSOpts.samplingInterval = 0;

	global->filterSMOpts.enabled = FALSE;
	global->filterSMOpts.filterType = FILTER_MEDIAN;
	global->filterSMOpts.windowSize = 5;
	global->filterSMOpts.windowType = WINDOW_SLIDING;
	global->filterSMOpts.samplingInterval = 0;

	/* How often refresh statistics (seconds) */
	global->statsUpdateInterval = 30;

	/* How long to wait for one-way delay prefiltering */
	global->calibrationDelay = 0;
	/* if set to TRUE and maxDelay is defined, only check against threshold if servo is stable */
	global->maxDelayStableOnly = FALSE;
	/* if set to non-zero, reset slave if more than this amount of consecutive delay measurements was above maxDelay */
	global->maxDelayMaxRejected = 0;

	/* status file options */
	global->statusFileUpdateInterval = 1;

	global->ofmAlarmThreshold = 0;

	/* panic mode options */
	global->enablePanicMode = FALSE;
	global->panicModeDuration = 30;
	global->panicModeExitThreshold = 0;

	global->faultTimeout = DEFAULT_FAULT_TIMEOUT;

	global->panicModeReleaseClock = FALSE;

	global->ntpOptions.enableEngine = FALSE;
	global->ntpOptions.enableControl = FALSE;
	global->ntpOptions.enableFailover = FALSE;
	global->ntpOptions.failoverTimeout = 120;
	global->ntpOptions.checkInterval = 15;
	global->ntpOptions.keyId = 0;
	strncpy(global->ntpOptions.hostAddress,"localhost",MAXHOSTNAMELEN);

	global->preferNTP = FALSE;


	global->leapSecondPausePeriod = 5;
	/* by default, announce the leap second 12 hours before the event:
	 * Clause 9.4 paragraph 5 */
	global->leapSecondNoticePeriod = 43200;
	global->leapSecondHandling = LEAP_ACCEPT;
	global->leapSecondSmearPeriod = 86400;

/* timing domain */
	global->idleTimeout = 120; /* idle timeout */
	global->electionDelay = 15; /* anti-flapping delay */

/* Log file settings */

	global->statisticsLog.logID = "statistics";
	global->statisticsLog.openMode = "a+";
	global->statisticsLog.logFP = NULL;
	global->statisticsLog.truncateOnReopen = FALSE;
	global->statisticsLog.unlinkOnClose = FALSE;
	global->statisticsLog.maxSize = 0;

	global->recordLog.logID = "record";
	global->recordLog.openMode = "a+";
	global->recordLog.logFP = NULL;
	global->recordLog.truncateOnReopen = FALSE;
	global->recordLog.unlinkOnClose = FALSE;
	global->recordLog.maxSize = 0;

	global->eventLog.logID = "log";
	global->eventLog.openMode = "a+";
	global->eventLog.logFP = NULL;
	global->eventLog.truncateOnReopen = FALSE;
	global->eventLog.unlinkOnClose = FALSE;
	global->eventLog.maxSize = 0;

	global->statusLog.logID = "status";
	global->statusLog.openMode = "w";
	strncpy(global->statusLog.logPath, DEFAULT_STATUSFILE, PATH_MAX);
	global->statusLog.logFP = NULL;
	global->statusLog.truncateOnReopen = FALSE;
	global->statusLog.unlinkOnClose = TRUE;

	global->deduplicateLog = TRUE;

/* Management message support settings */
	global->managementEnabled = TRUE;
	global->managementSetEnable = FALSE;

/* IP ACL settings */

	global->timingAclEnabled = FALSE;
	global->managementAclEnabled = FALSE;
	global->timingAclOrder = CCK_ACL_DENY_PERMIT;
	global->managementAclOrder = CCK_ACL_DENY_PERMIT;

	// by default we don't check Sync message sequence continuity
	global->syncSequenceChecking = FALSE;
	global->clockUpdateTimeout = 0;

	global->hwTimestamping = TRUE;

	global->ptpMonEnabled = FALSE;
	global->ptpMonDomainNumber = global->domainNumber;
	global->ptpMonAnyDomain = FALSE;

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
getPtpPreset(int presetNumber, GlobalConfig* global)
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
		ret.noAdjust = FALSE;
		ret.clockClass.minValue = 0;
		ret.clockClass.maxValue = 127;
		ret.clockClass.defaultValue = DEFAULT_CLOCK_CLASS__APPLICATION_SPECIFIC_TIME_SOURCE;
		break;
	default:
		ret.presetName = "none";
		ret.slaveOnly = global->slaveOnly;
		ret.noAdjust = global->noAdjust;
		ret.clockClass.minValue = 0;
		ret.clockClass.maxValue = 255;
		ret.clockClass.defaultValue = global->clockQuality.clockClass;
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
	filename=strtok_r(text__,DEFAULT_TOKEN_DELIM,&stash);
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
	templateName=strtok_r(text__,DEFAULT_TOKEN_DELIM,&stash);
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
