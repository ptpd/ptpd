/* Copyright (c) 2015 Wojciech Owczarek,
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
 * @file   globalconfig.h
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  Global daemon configuration structure: RunTimeOpts
 *
 */



#ifndef PTPD_GLOBALCONFIG_H_
#define PTPD_GLOBALCONFIG_H_


/**
 * \struct RunTimeOpts
 * \brief Program options set at run-time
 */
/* program options set at run-time */
typedef struct {
	Integer8 logAnnounceInterval;
	Integer8 announceReceiptTimeout;
	Integer8 logSyncInterval;
	Integer8 logMinDelayReqInterval;
	Integer8 logMinPdelayReqInterval;

	Boolean slaveOnly;

	ClockQuality clockQuality;
	TimePropertiesDS timeProperties;

	UInteger8 priority1;
	UInteger8 priority2;
	UInteger8 domainNumber;
	UInteger16 portNumber;


	/* Max intervals for unicast negotiation */
	Integer8 logMaxPdelayReqInterval;
	Integer8 logMaxDelayReqInterval;
	Integer8 logMaxSyncInterval;
	Integer8 logMaxAnnounceInterval;

	/* optional BMC extension: accept any domain, prefer configured domain, prefer lower domain */
	Boolean anyDomain;

	/*
	 * For slave state, grace period of n * announceReceiptTimeout
	 * before going into LISTENING again - we disqualify the best GM
	 * and keep waiting for BMC to act - if a new GM picks up,
	 * we don't lose the calculated offsets etc. Allows seamless GM
	 * failover with little time variation
	 */

	int announceTimeoutGracePeriod;
//	Integer16 currentUtcOffset;

	char ifaceName[IFACE_NAME_LENGTH + 1];
	char primaryIfaceName[IFACE_NAME_LENGTH+1];
	char backupIfaceName[IFACE_NAME_LENGTH+1];
	Boolean backupIfaceEnabled;

	Boolean	noResetClock; // don't step the clock if offset > 1s
	Boolean stepForce; // force clock step on first sync after startup
	Boolean stepOnce; // only step clock on first sync after startup
#ifdef linux
	Boolean setRtc;
#endif /* linux */

	Boolean clearCounters;

	Integer8 masterRefreshInterval;

	Integer32 maxOffset; /* Maximum number of nanoseconds of offset */
	Integer32 maxDelay; /* Maximum number of nanoseconds of delay */

	Boolean	noAdjust;

	Boolean displayPackets;
	Integer16 s;
	TimeInternal inboundLatency, outboundLatency, ofmCorrection;
	Integer16 max_foreign_records;
	Enumeration8 delayMechanism;

	Boolean portDisabled;

	int ttl;
	int dscpValue;
#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined (__QNXNTO__)
	int cpuNumber;
#endif /* linux && HAVE_SCHED_H || HAVE_SYS_CPUSET_H*/

	Boolean alwaysRespectUtcOffset;
	Boolean preferUtcValid;
	Boolean requireUtcValid;
	Boolean useSysLog;
	Boolean checkConfigOnly;
	Boolean printLockFile;

	char configFile[PATH_MAX+1];

	LogFileHandler statisticsLog;
	LogFileHandler recordLog;
	LogFileHandler eventLog;
	LogFileHandler statusLog;

	int leapSecondPausePeriod;
	Enumeration8 leapSecondHandling;
	Integer32 leapSecondSmearPeriod;
	int leapSecondNoticePeriod;

	Boolean periodicUpdates;
	Boolean logStatistics;
	Enumeration8 statisticsTimestamp;

	Enumeration8 logLevel;
	int statisticsLogInterval;

	int statusFileUpdateInterval;

	Boolean ignoreLock;
	Boolean refreshIgmp;
	Boolean  nonDaemon;

	int initial_delayreq;

	Boolean ignore_delayreq_interval_master;
	Boolean autoDelayReqInterval;

	Boolean autoLockFile; /* mode and interface specific lock files are used
				    * when set to TRUE */
	char lockDirectory[PATH_MAX+1]; /* Directory to store lock files
				       * When automatic lock files used */
	char lockFile[PATH_MAX+1]; /* lock file location */
	char driftFile[PATH_MAX+1]; /* drift file location */
	char leapFile[PATH_MAX+1]; /* leap seconds file location */
	char frequencyDir[PATH_MAX + 1]; /* frequency file directory */
	Enumeration8 drift_recovery_method; /* how the observed drift is managed
				      between restarts */

	Boolean storeToFile;

	LeapSecondInfo	leapInfo;

	Boolean snmpEnabled;		/* SNMP subsystem enabled / disabled even if compiled in */
	Boolean snmpTrapsEnabled; 	/* enable sending of SNMP traps (requires alarms enabled) */
	Boolean alarmsEnabled; 		/* enable support for alarms */
	int	alarmMinAge;		/* minimal alarm age in seconds (from set to clear notification) */
	int	alarmInitialDelay;	/* initial delay before we start processing alarms; example:  */
					/* we don't need a port state alarm just before the port starts to sync */

	Boolean pcap; /* Receive and send packets using libpcap, bypassing the
			 network stack. */
	Enumeration8 transport; /* transport type */
	Enumeration8 ipMode; /* IP transmission mode */
	Boolean dot1AS; /* 801.2AS support -> transportSpecific field */
	Boolean bindToInterface; /* always bind to interface */

	Boolean disableUdpChecksums; /* disable UDP checksum validation where supported */

	/* list of unicast destinations for use with unicast with or without signaling */
	char unicastDestinations[MAXHOSTNAMELEN * UNICAST_MAX_DESTINATIONS];
	char unicastDomains[MAXHOSTNAMELEN * UNICAST_MAX_DESTINATIONS];
	char unicastLocalPreference[MAXHOSTNAMELEN * UNICAST_MAX_DESTINATIONS];

	/* list of extra clocks to sync */
	char extraClocks[PATH_MAX];
	char masterClock[PATH_MAX];
	char readOnlyClocks[PATH_MAX];
	char disabledClocks[PATH_MAX];
	char excludedClocks[PATH_MAX];

	int clockFailureDelay;
	Boolean lockClockDevice;

	char productDescription[65];
	char portDescription[65];

	Boolean		unicastDestinationsSet;
	char unicastPeerDestination[MAXHOSTNAMELEN];
	Boolean		unicastPeerDestinationSet;

	UInteger32	unicastGrantDuration;

	Boolean unicastNegotiation; /* Enable unicast negotiation support */
	Boolean	unicastNegotiationListening; /* Master: Reply to signaling messages when in LISTENING */
	Boolean disableBMCA; /* used to achieve master-only for unicast */
	Boolean unicastAcceptAny; /* Slave: accept messages from all GMs, regardless of grants */
	/*
	 * port mask to apply to portNumber when using negotiation:
	 * treats different port numbers as the same port ID for clocks which
	 * transmit signaling using one port ID, and rest of messages with another
	 */
	UInteger16  unicastPortMask; /* port mask to apply to portNumber when using negotiation */

#ifdef RUNTIME_DEBUG
	int debug_level;
#endif
	Boolean pidAsClockId;

	/**
	 * This field holds the flags denoting which subsystems
	 * have to be re-initialised as a result of config reload.
	 * Flags are defined in daemonconfig.h
	 * 0 = no change
	 */
	UInteger32 restartSubsystems;
	/* config dictionary containers - current, candidate, and from CLI */
	dictionary *currentConfig, *candidateConfig, *cliConfig;

	Enumeration8 selectedPreset;

	int servoMaxPpb;
	int servoMaxPpb_hw;

	double servoKP;
	double servoKI;

	double servoKI_hw;
	double servoKP_hw;

	double stableAdev;
	double stableAdev_hw;

	double unstableAdev;
	double unstableAdev_hw;

	int lockedAge;
	int lockedAge_hw;

	int holdoverAge;
	int holdoverAge_hw;

	int adevPeriod;

	Boolean negativeStep;

	Boolean negativeStep_hw;

	int clockSyncRate;
	int clockUpdateInterval;

	Enumeration8 servoDtMethod;
	double servoMaxdT;

	/* inter-clock sync filter options */

	Boolean clockStatFilterEnable;
	int clockStatFilterWindowSize;
	uint8_t clockStatFilterWindowType;
	uint8_t clockStatFilterType;

	Boolean clockOutlierFilterEnable;
	int clockOutlierFilterWindowSize;
	int clockOutlierFilterDelay;
	double clockOutlierFilterCutoff;
	int clockOutlierFilterBlockTimeout;

	Boolean clockStrictSync;
	int clockMinStep;

	/**
	 *  When enabled, ptpd ensures that Sync message sequence numbers
	 *  are increasing (consecutive sync is not lower than last).
	 *  This can prevent reordered sequences, but can also lock the slave
         *  up if, say, GM restarted and reset sequencing.
	 */
	Boolean syncSequenceChecking;

	/**
	  * (seconds) - if set to non-zero, slave will reset if no clock updates
	  * after this amount of time. Used to "unclog" slave stuck on offset filter
	  * threshold or after GM reset the Sync sequence number
          */
	int clockUpdateTimeout;

	OutlierFilterConfig oFilterMSConfig;
	OutlierFilterConfig oFilterSMConfig;

	StatFilterOptions filterMSOpts;
	StatFilterOptions filterSMOpts;

	Boolean maxDelayStableOnly;

	/* also used by the periodic message ticker */
	int statsUpdateInterval;

	int calibrationDelay;
	Boolean enablePanicMode;
	Boolean panicModeReleaseClock;
	int panicModeDuration;
	UInteger32 panicModeExitThreshold;
	int idleTimeout;

	UInteger32 ofmAlarmThreshold;

	NTPoptions ntpOptions;
	Boolean preferNTP;

	int electionDelay; /* timing domain election delay to prevent flapping */

	int maxDelayMaxRejected;

	/* max reset cycles in LISTENING before full network restart */
	int maxListen;

	Boolean managementEnabled;
	Boolean managementSetEnable;

	/* Access list settings */
	Boolean timingAclEnabled;
	Boolean managementAclEnabled;
	char timingAclPermitText[PATH_MAX+1];
	char timingAclDenyText[PATH_MAX+1];
	char managementAclPermitText[PATH_MAX+1];
	char managementAclDenyText[PATH_MAX+1];
	Enumeration8 timingAclOrder;
	Enumeration8 managementAclOrder;

	Boolean hwTimestamping;

	/* PTP monitoring extensions */
	Boolean ptpMonEnabled;
	Boolean ptpMonAnyDomain;
	UInteger8 ptpMonDomainNumber;

} RunTimeOpts;

#endif /*PTPD_GLOBALCONFIG_H_*/
