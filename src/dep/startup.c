/*-
 * Copyright (c) 2014-2015 Wojciech Owczarek,
 * Copyright (c) 2012-2013 George V. Neville-Neil,
 *                         Wojciech Owczarek
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments.
 * Copyright (c) 2009-2010 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen
 *
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
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
 * @file   startup.c
 * @date   Wed Jun 23 09:33:27 2010
 *
 * @brief  Code to handle daemon startup, including command line args
 *
 * The function in this file are called when the daemon starts up
 * and include the getopt() command line argument parsing.
 */

#include "../ptpd.h"

/*
 * valgrind 3.5.0 currently reports no errors (last check: 20110512)
 * valgrind 3.4.1 lacks an adjtimex handler
 *
 * to run:   sudo valgrind --show-reachable=yes --leak-check=full --track-origins=yes -- ./ptpd2 -c ...
 */

/*
  to test daemon locking and startup sequence itself, try:

  function s()  { set -o pipefail ;  eval "$@" |  sed 's/^/\t/' ; echo $?;  }
  sudo killall ptpd2
  s ./ptpd2
  s sudo ./ptpd2
  s sudo ./ptpd2 -t -g
  s sudo ./ptpd2 -t -g -b eth0
  s sudo ./ptpd2 -t -g -b eth0
  ps -ef | grep ptpd2
*/

/*
 * Synchronous signal processing:
 * original idea: http://www.openbsd.org/cgi-bin/cvsweb/src/usr.sbin/ntpd/ntpd.c?rev=1.68;content-type=text%2Fplain
 */
volatile sig_atomic_t	 sigint_received  = 0;
volatile sig_atomic_t	 sigterm_received = 0;
volatile sig_atomic_t	 sighup_received  = 0;
volatile sig_atomic_t	 sigusr1_received = 0;
volatile sig_atomic_t	 sigusr2_received = 0;

/* Pointer to the current lock file */
FILE* G_lockFilePointer;

/*
 * Function to catch signals asynchronously.
 * Assuming that the daemon periodically calls checkSignals(), then all operations are safely done synchrously at a later opportunity.
 *
 * Please do NOT call any functions inside this handler - especially DBG() and its friends, or any glibc.
 */
void catchSignals(int sig)
{
	switch (sig) {
	case SIGINT:
		sigint_received = 1;
		break;
	case SIGTERM:
		sigterm_received = 1;
		break;
	case SIGHUP:
		sighup_received = 1;
		break;
	case SIGUSR1:
		sigusr1_received = 1;
		break;
	case SIGUSR2:
		sigusr2_received = 1;
		break;
	default:
		/*
		 * TODO: should all other signals be catched, and handled as SIGINT?
		 *
		 * Reason: currently, all other signals are just uncatched, and the OS kills us.
		 * The difference is that we could then close the open files properly.
		 */
		break;
	}
}

/*
 * exit the program cleanly
 */
void
do_signal_close(PtpClock * ptpClock)
{

	timingDomain.shutdown(&timingDomain);

	NOTIFY("Shutdown on close signal\n");
	exit(0);
}

void
applyConfig(dictionary *baseConfig, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

	Boolean reloadSuccessful = TRUE;


	/* Load default config to fill in the blanks in the config file */
	RunTimeOpts tmpOpts;
	loadDefaultSettings(&tmpOpts);

	/* Check the new configuration for errors, fill in the blanks from defaults */
	if( ( rtOpts->candidateConfig = parseConfig(CFGOP_PARSE, NULL, baseConfig, &tmpOpts)) == NULL ) {
	    WARNING("Configuration has errors, reload aborted\n");
	    return;
	}

	/* Check for changes between old and new configuration */
	if(compareConfig(rtOpts->candidateConfig,rtOpts->currentConfig)) {
	    INFO("Configuration unchanged\n");
	    goto cleanup;
	}

	/*
	 * Mark which subsystems have to be restarted. Most of this will be picked up by doState()
	 * If there are errors past config correctness (such as non-existent NIC,
	 * or lock file clashes if automatic lock files used - abort the mission
	 */

	rtOpts->restartSubsystems =
	    checkSubsystemRestart(rtOpts->candidateConfig, rtOpts->currentConfig, rtOpts);

	/* If we're told to re-check lock files, do it: tmpOpts already has what rtOpts should */
	if( (rtOpts->restartSubsystems & PTPD_CHECK_LOCKS) &&
	    tmpOpts.autoLockFile && !checkOtherLocks(&tmpOpts)) {
		reloadSuccessful = FALSE;
	}

	/* If the network configuration has changed, check if the interface is OK */
	if(rtOpts->restartSubsystems & PTPD_RESTART_NETWORK) {
		INFO("Network configuration changed - checking interface(s)\n");
		if(!testInterface(tmpOpts.primaryIfaceName, &tmpOpts)) {
		    reloadSuccessful = FALSE;
		    ERROR("Error: Cannot use %s interface\n",tmpOpts.primaryIfaceName);
		}
		if(rtOpts->backupIfaceEnabled && !testInterface(tmpOpts.backupIfaceName, &tmpOpts)) {
		    rtOpts->restartSubsystems = -1;
		    ERROR("Error: Cannot use %s interface as backup\n",tmpOpts.backupIfaceName);
		}
	}
#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined(__QNXNTO__)
        /* Changing the CPU affinity mask */
        if(rtOpts->restartSubsystems & PTPD_CHANGE_CPUAFFINITY) {
                NOTIFY("Applying CPU binding configuration: changing selected CPU core\n");

                if(setCpuAffinity(tmpOpts.cpuNumber) < 0) {
                        if(tmpOpts.cpuNumber == -1) {
                                ERROR("Could not unbind from CPU core %d\n", rtOpts->cpuNumber);
                        } else {
                                ERROR("Could bind to CPU core %d\n", tmpOpts.cpuNumber);
                        }
			reloadSuccessful = FALSE;
                } else {
                        if(tmpOpts.cpuNumber > -1)
                                INFO("Successfully bound "PTPD_PROGNAME" to CPU core %d\n", tmpOpts.cpuNumber);
                        else
                                INFO("Successfully unbound "PTPD_PROGNAME" from cpu core CPU core %d\n", rtOpts->cpuNumber);
                }
         }
#endif

	if(!reloadSuccessful) {
		ERROR("New configuration cannot be applied - aborting reload\n");
		rtOpts->restartSubsystems = 0;
		goto cleanup;
	}


		/**
		 * Commit changes to rtOpts and currentConfig
		 * (this should never fail as the config has already been checked if we're here)
		 * However if this DOES fail, some default has been specified out of range -
		 * this is the only situation where parse will succeed but commit not:
		 * disable quiet mode to show what went wrong, then die.
		 */
		if (rtOpts->currentConfig) {
			dictionary_del(&rtOpts->currentConfig);
		}
		if ( (rtOpts->currentConfig = parseConfig(CFGOP_PARSE_QUIET, NULL, rtOpts->candidateConfig,rtOpts)) == NULL) {
			CRITICAL("************ "PTPD_PROGNAME": parseConfig returned NULL during config commit"
				 "  - this is a BUG - report the following: \n");

			if ((rtOpts->currentConfig = parseConfig(CFGOP_PARSE, NULL, rtOpts->candidateConfig,rtOpts)) == NULL)
			    CRITICAL("*****************" PTPD_PROGNAME" shutting down **********************\n");
			/*
			 * Could be assert(), but this should be done any time this happens regardless of
			 * compile options. Anyhow, if we're here, the daemon will no doubt segfault soon anyway
			 */
			abort();
		}

	/* clean up */
	cleanup:

		dictionary_del(&rtOpts->candidateConfig);
}


/**
 * Signal handler for HUP which tells us to swap the log file
 * and reload configuration file if specified
 *
 * @param sig
 */
void
do_signal_sighup(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{



	NOTIFY("SIGHUP received\n");

#ifdef RUNTIME_DEBUG
	if(rtOpts->transport == UDP_IPV4 && rtOpts->ipMode != IPMODE_UNICAST) {
		DBG("SIGHUP - running an ipv4 multicast based mode, re-sending IGMP joins\n");
		netRefreshIGMP(&ptpClock->netPath, rtOpts, ptpClock);
	}
#endif /* RUNTIME_DEBUG */


	/* if we don't have a config file specified, we're done - just reopen log files*/
	if(strlen(rtOpts->configFile) !=  0) {

	    dictionary* tmpConfig = dictionary_new(0);

	    /* Try reloading the config file */
	    NOTIFY("Reloading configuration file: %s\n",rtOpts->configFile);

            if(!loadConfigFile(&tmpConfig, rtOpts)) {

		    dictionary_del(&tmpConfig);

	    } else {
		    dictionary_merge(rtOpts->cliConfig, tmpConfig, 1, 1, "from command line");
		    applyConfig(tmpConfig, rtOpts, ptpClock);
		    dictionary_del(&tmpConfig);

	    }

	}

	/* tell the service it can perform any HUP-triggered actions */
	ptpClock->timingService.reloadRequested = TRUE;

	if(rtOpts->recordLog.logEnabled ||
	    rtOpts->eventLog.logEnabled ||
	    (rtOpts->statisticsLog.logEnabled))
		INFO("Reopening log files\n");

	restartLogging(rtOpts);

	if(rtOpts->statisticsLog.logEnabled)
		ptpClock->resetStatisticsLog = TRUE;


}

void
restartSubsystems(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
			DBG("RestartSubsystems: %d\n",rtOpts->restartSubsystems);
		    /* So far, PTP_INITIALIZING is required for both network and protocol restart */
		    if((rtOpts->restartSubsystems & PTPD_RESTART_PROTOCOL) ||
			(rtOpts->restartSubsystems & PTPD_RESTART_NETWORK)) {

			    if(rtOpts->restartSubsystems & PTPD_RESTART_NETWORK) {
				NOTIFY("Applying network configuration: going into PTP_INITIALIZING\n");
			    }

			    /* These parameters have to be passed to ptpClock before re-init */
			    ptpClock->defaultDS.clockQuality.clockClass = rtOpts->clockQuality.clockClass;
			    ptpClock->defaultDS.slaveOnly = rtOpts->slaveOnly;
			    ptpClock->disabled = rtOpts->portDisabled;

			    if(rtOpts->restartSubsystems & PTPD_RESTART_PROTOCOL) {
				INFO("Applying protocol configuration: going into %s\n",
				ptpClock->disabled ? "PTP_DISABLED" : "PTP_INITIALIZING");
			    }

			    /* Move back to primary interface only during configuration changes. */
			    ptpClock->runningBackupInterface = FALSE;
			    toState(ptpClock->disabled ? PTP_DISABLED : PTP_INITIALIZING, rtOpts, ptpClock);

		    } else {
		    /* Nothing happens here for now - SIGHUP handler does this anyway */
		    if(rtOpts->restartSubsystems & PTPD_UPDATE_DATASETS) {
				NOTIFY("Applying PTP engine configuration: updating datasets\n");
				updateDatasets(ptpClock, rtOpts);
		    }}
		    /* Nothing happens here for now - SIGHUP handler does this anyway */
		    if(rtOpts->restartSubsystems & PTPD_RESTART_LOGGING) {
				NOTIFY("Applying logging configuration: restarting logging\n");
		    }


    		if(rtOpts->restartSubsystems & PTPD_RESTART_ACLS) {
            		NOTIFY("Applying access control list configuration\n");
            		/* re-compile ACLs */
            		freeIpv4AccessList(&ptpClock->netPath.timingAcl);
            		freeIpv4AccessList(&ptpClock->netPath.managementAcl);
            		if(rtOpts->timingAclEnabled) {
                    	    ptpClock->netPath.timingAcl=createIpv4AccessList(rtOpts->timingAclPermitText,
                                rtOpts->timingAclDenyText, rtOpts->timingAclOrder);
            		}
            		if(rtOpts->managementAclEnabled) {
                    	    ptpClock->netPath.managementAcl=createIpv4AccessList(rtOpts->managementAclPermitText,
                                rtOpts->managementAclDenyText, rtOpts->managementAclOrder);
            		}
    		}

    		if(rtOpts->restartSubsystems & PTPD_RESTART_ALARMS) {
		    NOTIFY("Applying alarm configuration\n");
		    configureAlarms(ptpClock->alarms, ALRM_MAX, (void*)ptpClock);
		}

#ifdef PTPD_STATISTICS
                    /* Reinitialising the outlier filter containers */
                    if(rtOpts->restartSubsystems & PTPD_RESTART_FILTERS) {

                                NOTIFY("Applying filter configuration: re-initialising filters\n");

				freeDoubleMovingStatFilter(&ptpClock->filterMS);
				freeDoubleMovingStatFilter(&ptpClock->filterSM);

				ptpClock->oFilterMS.shutdown(&ptpClock->oFilterMS);
				ptpClock->oFilterSM.shutdown(&ptpClock->oFilterSM);

				outlierFilterSetup(&ptpClock->oFilterMS);
				outlierFilterSetup(&ptpClock->oFilterSM);

				ptpClock->oFilterMS.init(&ptpClock->oFilterMS,&rtOpts->oFilterMSConfig, "delayMS");
				ptpClock->oFilterSM.init(&ptpClock->oFilterSM,&rtOpts->oFilterSMConfig, "delaySM");


				if(rtOpts->filterMSOpts.enabled) {
					ptpClock->filterMS = createDoubleMovingStatFilter(&rtOpts->filterMSOpts,"delayMS");
				}

				if(rtOpts->filterSMOpts.enabled) {
					ptpClock->filterSM = createDoubleMovingStatFilter(&rtOpts->filterSMOpts, "delaySM");
				}

		    }
#endif /* PTPD_STATISTICS */


	    ptpClock->timingService.reloadRequested = TRUE;

            if(rtOpts->restartSubsystems & PTPD_RESTART_NTPENGINE && timingDomain.serviceCount > 1) {
		ptpClock->ntpControl.timingService.shutdown(&ptpClock->ntpControl.timingService);
	    }

	    if((rtOpts->restartSubsystems & PTPD_RESTART_NTPENGINE) ||
        	(rtOpts->restartSubsystems & PTPD_RESTART_NTPCONFIG)) {
        	ntpSetup(rtOpts, ptpClock);
    	    }
		if((rtOpts->restartSubsystems & PTPD_RESTART_NTPENGINE) && rtOpts->ntpOptions.enableEngine) {
		    timingServiceSetup(&ptpClock->ntpControl.timingService);
		    ptpClock->ntpControl.timingService.init(&ptpClock->ntpControl.timingService);
		}

		ptpClock->timingService.dataSet.priority1 = rtOpts->preferNTP;

		timingDomain.electionDelay = rtOpts->electionDelay;
		if(timingDomain.electionLeft > timingDomain.electionDelay) {
			timingDomain.electionLeft = timingDomain.electionDelay;
		}

		timingDomain.services[0]->holdTime = rtOpts->ntpOptions.failoverTimeout;

		if(timingDomain.services[0]->holdTimeLeft >
			timingDomain.services[0]->holdTime) {
			timingDomain.services[0]->holdTimeLeft =
			rtOpts->ntpOptions.failoverTimeout;
		}

		ptpClock->timingService.timeout = rtOpts->idleTimeout;

		    /* Update PI servo parameters */
		    setupPIservo(&ptpClock->servo, rtOpts);
		    /* Config changes don't require subsystem restarts - acknowledge it */
		    if(rtOpts->restartSubsystems == PTPD_RESTART_NONE) {
				NOTIFY("Applying configuration\n");
		    }

		    if(rtOpts->restartSubsystems != -1)
			    rtOpts->restartSubsystems = 0;

}


/*
 * Synchronous signal processing:
 * This function should be called regularly from the main loop
 */
void
checkSignals(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	/*
	 * note:
	 * alarm signals are handled in a similar way in dep/timer.c
	 */

	if(sigint_received || sigterm_received){
		do_signal_close(ptpClock);
	}

	if(sighup_received){
		do_signal_sighup(rtOpts, ptpClock);
	sighup_received=0;
	}

	if(sigusr1_received){
	    if(ptpClock->portDS.portState == PTP_SLAVE){
		    WARNING("SIGUSR1 received, stepping clock to current known OFM\n");
                    stepClock(rtOpts, ptpClock);                                                                                                        
//		    ptpClock->clockControl.stepRequired = TRUE;
	    } else {
		    ERROR("SIGUSR1 received - will not step clock, not in PTP_SLAVE state\n");
	    }
	sigusr1_received = 0;
	}

	if(sigusr2_received){

/* testing only: testing step detection */
#if 0
		{
		ptpClock->addOffset ^= 1;
		INFO("a: %d\n", ptpClock->addOffset);
		sigusr2_received = 0;
		return;
		}
#endif
		displayCounters(ptpClock);
		displayAlarms(ptpClock->alarms, ALRM_MAX);
		if(rtOpts->timingAclEnabled) {
			INFO("\n\n");
			INFO("** Timing message ACL:\n");
			dumpIpv4AccessList(ptpClock->netPath.timingAcl);
		}
		if(rtOpts->managementAclEnabled) {
			INFO("\n\n");
			INFO("** Management message ACL:\n");
			dumpIpv4AccessList(ptpClock->netPath.managementAcl);
		}
		if(rtOpts->clearCounters) {
			clearCounters(ptpClock);
			NOTIFY("PTP engine counters cleared\n");
		}
#ifdef PTPD_STATISTICS
		if(rtOpts->oFilterSMConfig.enabled) {
			ptpClock->oFilterSM.display(&ptpClock->oFilterSM);
		}
		if(rtOpts->oFilterMSConfig.enabled) {
			ptpClock->oFilterMS.display(&ptpClock->oFilterMS);
		}
#endif /* PTPD_STATISTICS */
		sigusr2_received = 0;
	}

}

#ifdef RUNTIME_DEBUG
/* These functions are useful to temporarily enable Debug around parts of code, similar to bash's "set -x" */
void enable_runtime_debug(void )
{
	extern RunTimeOpts rtOpts;
	
	rtOpts.debug_level = max(LOG_DEBUGV, rtOpts.debug_level);
}

void disable_runtime_debug(void )
{
	extern RunTimeOpts rtOpts;
	
	rtOpts.debug_level = LOG_INFO;
}
#endif

int
writeLockFile(RunTimeOpts * rtOpts)
{

	int lockPid = 0;

	DBGV("Checking lock file: %s\n", rtOpts->lockFile);

	if ( (G_lockFilePointer=fopen(rtOpts->lockFile, "w+")) == NULL) {
		PERROR("Could not open lock file %s for writing", rtOpts->lockFile);
		return(0);
	}
	if (lockFile(fileno(G_lockFilePointer)) < 0) {
		if ( checkLockStatus(fileno(G_lockFilePointer),
			DEFAULT_LOCKMODE, &lockPid) == 0) {
			     ERROR("Another "PTPD_PROGNAME" instance is running: %s locked by PID %d\n",
				    rtOpts->lockFile, lockPid);
		} else {
			PERROR("Could not acquire lock on %s:", rtOpts->lockFile);
		}
		goto failure;
	}
	if(ftruncate(fileno(G_lockFilePointer), 0) == -1) {
		PERROR("Could not truncate %s: %s",
			rtOpts->lockFile, strerror(errno));
		goto failure;
	}
	if ( fprintf(G_lockFilePointer, "%ld\n", (long)getpid()) == -1) {
		PERROR("Could not write to lock file %s: %s",
			rtOpts->lockFile, strerror(errno));
		goto failure;
	}
	INFO("Successfully acquired lock on %s\n", rtOpts->lockFile);
	fflush(G_lockFilePointer);
	return(1);
	failure:
	fclose(G_lockFilePointer);
	return(0);

}

void
ptpdShutdown(PtpClock * ptpClock)
{

	extern RunTimeOpts rtOpts;
	
	/*
         * go into DISABLED state so the FSM can call any PTP-specific shutdown actions,
	 * such as canceling unicast transmission
         */
	toState(PTP_DISABLED, &rtOpts, ptpClock);
	/* process any outstanding events before exit */
	updateAlarms(ptpClock->alarms, ALRM_MAX);
	netShutdown(&ptpClock->netPath);
	free(ptpClock->foreign);

	/* free management and signaling messages, they can have dynamic memory allocated */
	if(ptpClock->msgTmpHeader.messageType == MANAGEMENT)
		freeManagementTLV(&ptpClock->msgTmp.manage);
	freeManagementTLV(&ptpClock->outgoingManageTmp);
	if(ptpClock->msgTmpHeader.messageType == SIGNALING)
		freeSignalingTLV(&ptpClock->msgTmp.signaling);
	freeSignalingTLV(&ptpClock->outgoingSignalingTmp);

#ifdef PTPD_SNMP
	snmpShutdown();
#endif /* PTPD_SNMP */

#ifndef PTPD_STATISTICS
	/* Not running statistics code - write observed drift to driftfile if enabled, inform user */
	if(ptpClock->defaultDS.slaveOnly && !ptpClock->servo.runningMaxOutput)
		saveDrift(ptpClock, &rtOpts, FALSE);
#else
	ptpClock->oFilterMS.shutdown(&ptpClock->oFilterMS);
	ptpClock->oFilterSM.shutdown(&ptpClock->oFilterSM);
        freeDoubleMovingStatFilter(&ptpClock->filterMS);
        freeDoubleMovingStatFilter(&ptpClock->filterSM);

	/* We are running statistics code - save drift on exit only if we're not monitoring servo stability */
	if(!rtOpts.servoStabilityDetection && !ptpClock->servo.runningMaxOutput)
		saveDrift(ptpClock, &rtOpts, FALSE);
#endif /* PTPD_STATISTICS */

	if (rtOpts.currentConfig != NULL)
		dictionary_del(&rtOpts.currentConfig);
	if(rtOpts.cliConfig != NULL)
		dictionary_del(&rtOpts.cliConfig);

	timerShutdown(ptpClock->timers);

	free(ptpClock);
	ptpClock = NULL;

	extern PtpClock* G_ptpClock;
	G_ptpClock = NULL;



	/* properly clean lockfile (eventough new deaemons can acquire the lock after we die) */
	if(!rtOpts.ignore_daemon_lock && G_lockFilePointer != NULL) {
	    fclose(G_lockFilePointer);
	    G_lockFilePointer = NULL;
	}
	unlink(rtOpts.lockFile);

	if(rtOpts.statusLog.logEnabled) {
		/* close and remove the status file */
		if(rtOpts.statusLog.logFP != NULL) {
			fclose(rtOpts.statusLog.logFP);
			rtOpts.statusLog.logFP = NULL;
		}
		unlink(rtOpts.statusLog.logPath);
	}

	stopLogging(&rtOpts);

}

void dump_command_line_parameters(int argc, char **argv)
{

	int i = 0;
	char sbuf[1000];
	char *st = sbuf;
	int len = 0;
	
	*st = '\0';
	for(i=0; i < argc; i++){
		if(strcmp(argv[i],"") == 0)
		    continue;
		len += snprintf(sbuf + len,
					     sizeof(sbuf) - len,
					     "%s ", argv[i]);
	}
	INFO("Starting %s daemon with parameters:      %s\n", PTPD_PROGNAME, sbuf);
}



PtpClock *
ptpdStartup(int argc, char **argv, Integer16 * ret, RunTimeOpts * rtOpts)
{
	PtpClock * ptpClock;
	TimeInternal tmpTime;
	int i = 0;

	/*
	 * Set the default mode for all newly created files - previously
	 * this was not the case for log files. This adds consistency
	 * and allows to use FILE* vs. fds everywhere
	 */
	umask(~DEFAULT_FILE_PERMS);

	/* get some entropy in... */
	getTime(&tmpTime);
	srand(tmpTime.seconds ^ tmpTime.nanoseconds);

	/**
	 * If a required setting, such as interface name, or a setting
	 * requiring a range check is to be set via getopts_long,
	 * the respective currentConfig dictionary entry should be set,
	 * instead of just setting the rtOpts field.
	 *
	 * Config parameter evaluation priority order:
	 * 	1. Any dictionary keys set in the getopt_long loop
	 * 	2. CLI long section:key type options
	 * 	3. Any built-in config templates
	 *	4. Any templates loaded from template file
	 * 	5. Config file (parsed last), merged with 2. and 3 - will be overwritten by CLI options
	 * 	6. Defaults and any rtOpts fields set in the getopt_long loop
	**/

	/**
	 * Load defaults. Any options set here and further inside loadCommandLineOptions()
	 * by setting rtOpts fields, will be considered the defaults
	 * for config file and section:key long options.
	 */
	loadDefaultSettings(rtOpts);
	/* initialise the config dictionary */
	rtOpts->candidateConfig = dictionary_new(0);
	rtOpts->cliConfig = dictionary_new(0);

	/* parse all long section:key options and clean up argv for getopt */
	loadCommandLineKeys(rtOpts->cliConfig,argc,argv);
	/* parse the normal short and long options, exit on error */
	if (!loadCommandLineOptions(rtOpts, rtOpts->cliConfig, argc, argv, ret)) {
	    goto fail;
	}

	/* Display startup info and argv if not called with -? or -H */
		NOTIFY("%s version %s starting\n",USER_DESCRIPTION, USER_VERSION);
		dump_command_line_parameters(argc, argv);
	/*
	 * we try to catch as many error conditions as possible, but before we call daemon().
	 * the exception is the lock file, as we get a new pid when we call daemon(),
	 * so this is checked twice: once to read, second to read/write
	 */
	if(geteuid() != 0)
	{
		printf("Error: "PTPD_PROGNAME" daemon can only be run as root\n");
			*ret = 1;
			goto fail;
		}

	/* Have we got a config file? */
	if(strlen(rtOpts->configFile) > 0) {
		/* config file settings overwrite all others, except for empty strings */
		INFO("Loading configuration file: %s\n",rtOpts->configFile);
		if(loadConfigFile(&rtOpts->candidateConfig, rtOpts)) {
			dictionary_merge(rtOpts->cliConfig, rtOpts->candidateConfig, 1, 1, "from command line");
		} else {
		    *ret = 1;
			dictionary_merge(rtOpts->cliConfig, rtOpts->candidateConfig, 1, 1, "from command line");
		    goto configcheck;
		}
	} else {
		dictionary_merge(rtOpts->cliConfig, rtOpts->candidateConfig, 1, 1, "from command line");
	}
	/**
	 * This is where the final checking  of the candidate settings container happens.
	 * A dictionary is returned with only the known options, explicitly set to defaults
	 * if not present. NULL is returned on any config error - parameters missing, out of range,
	 * etc. The getopt loop in loadCommandLineOptions() only sets keys verified here.
	 */
	if( ( rtOpts->currentConfig = parseConfig(CFGOP_PARSE, NULL, rtOpts->candidateConfig,rtOpts)) == NULL ) {
	    *ret = 1;
	    dictionary_del(&rtOpts->candidateConfig);
	    goto configcheck;
	}

	/* we've been told to print the lock file and exit cleanly */
	if(rtOpts->printLockFile) {
	    printf("%s\n", rtOpts->lockFile);
	    *ret = 0;
	    goto fail;
	}

	/* we don't need the candidate config any more */
	dictionary_del(&rtOpts->candidateConfig);

	/* Check network before going into background */
	if(!testInterface(rtOpts->primaryIfaceName, rtOpts)) {
	    ERROR("Error: Cannot use %s interface\n",rtOpts->primaryIfaceName);
	    *ret = 1;
	    goto configcheck;
	}
	if(rtOpts->backupIfaceEnabled && !testInterface(rtOpts->backupIfaceName, rtOpts)) {
	    ERROR("Error: Cannot use %s interface as backup\n",rtOpts->backupIfaceName);
	    *ret = 1;
	    goto configcheck;
	}


configcheck:
	/*
	 * We've been told to check config only - clean exit before checking locks
	 */
	if(rtOpts->checkConfigOnly) {
	    if(*ret != 0) {
		printf("Configuration has errors\n");
		*ret = 1;
		}
	    else
		printf("Configuration OK\n");
	    goto fail;
	}

	/* Previous errors - exit */
	if(*ret !=0)
		goto fail;

	/* First lock check, just to be user-friendly to the operator */
	if(!rtOpts->ignore_daemon_lock) {
		if(!writeLockFile(rtOpts)){
			/* check and create Lock */
			ERROR("Error: file lock failed (use -L or global:ignore_lock to ignore lock file)\n");
			*ret = 3;
			goto fail;
		}
		/* check for potential conflicts when automatic lock files are used */
		if(!checkOtherLocks(rtOpts)) {
			*ret = 3;
			goto fail;
		}
	}

	/* Manage log files: stats, log, status and quality file */
	restartLogging(rtOpts);

	/* Allocate memory after we're done with other checks but before going into daemon */
	ptpClock = (PtpClock *) calloc(1, sizeof(PtpClock));
	if (!ptpClock) {
		PERROR("Error: Failed to allocate memory for protocol engine data");
		*ret = 2;
		goto fail;
	} else {
		DBG("allocated %d bytes for protocol engine data\n",
		    (int)sizeof(PtpClock));


		ptpClock->foreign = (ForeignMasterRecord *)
			calloc(rtOpts->max_foreign_records,
			       sizeof(ForeignMasterRecord));
		if (!ptpClock->foreign) {
			PERROR("failed to allocate memory for foreign "
			       "master data");
			*ret = 2;
			free(ptpClock);
			goto fail;
		} else {
			DBG("allocated %d bytes for foreign master data\n",
			    (int)(rtOpts->max_foreign_records *
				  sizeof(ForeignMasterRecord)));
		}
	}

	if(rtOpts->statisticsLog.logEnabled)
		ptpClock->resetStatisticsLog = TRUE;

	/* Init to 0 net buffer */
	memset(ptpClock->msgIbuf, 0, PACKET_SIZE);
	memset(ptpClock->msgObuf, 0, PACKET_SIZE);
	
	/* Init outgoing management message */
	ptpClock->outgoingManageTmp.tlv = NULL;
	

	/*  DAEMON */
#ifdef PTPD_NO_DAEMON
	if(!rtOpts->nonDaemon){
		rtOpts->nonDaemon=TRUE;
	}
#endif

	if(!rtOpts->nonDaemon){
		/*
		 * fork to daemon - nochdir non-zero to preserve the working directory:
		 * allows relative paths to be used for log files, config files etc.
		 * Always redirect stdout/err to /dev/null
		 */
		if (daemon(1,0) == -1) {
			PERROR("Failed to start as daemon");
			*ret = 3;
			goto fail;
		}
		INFO("  Info:    Now running as a daemon\n");
		/*
		 * Wait for the parent process to terminate, but not forever.
		 * On some systems this happened after we tried re-acquiring
		 * the lock, so the lock would fail. Hence, we wait.
		 */
		for (i = 0; i < 1000000; i++) {
			/* Once we've been reaped by init, parent PID will be 1 */
			if(getppid() == 1)
				break;
			usleep(1);
		}
	}

	/* Second lock check, to replace the contents with our own new PID and re-acquire the advisory lock */
	if(!rtOpts->nonDaemon && !rtOpts->ignore_daemon_lock){
		/* check and create Lock */
		if(!writeLockFile(rtOpts)){
			ERROR("Error: file lock failed (use -L or global:ignore_lock to ignore lock file)\n");
			*ret = 3;
			goto fail;
		}
	}

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined(__QNXNTO__)
	/* Try binding to a single CPU core if configured to do so */
	if(rtOpts->cpuNumber > -1) {
    		if(setCpuAffinity(rtOpts->cpuNumber) < 0) {
		    ERROR("Could not bind to CPU core %d\n", rtOpts->cpuNumber);
		} else {
		    INFO("Successfully bound "PTPD_PROGNAME" to CPU core %d\n", rtOpts->cpuNumber);
		}
	}
#endif

	/* set up timers */
	if(!timerSetup(ptpClock->timers)) {
		PERROR("failed to set up event timers");
		*ret = 2;
		free(ptpClock);
		goto fail;
	}

	ptpClock->rtOpts = rtOpts;

	/* init alarms */
	initAlarms(ptpClock->alarms, ALRM_MAX, (void*)ptpClock);
	configureAlarms(ptpClock->alarms, ALRM_MAX, (void*)ptpClock);
	ptpClock->alarmDelay = rtOpts->alarmInitialDelay;
	/* we're delaying alarm processing - disable alarms for now */
	if(ptpClock->alarmDelay) {
	    enableAlarms(ptpClock->alarms, ALRM_MAX, FALSE);
	}


	/* establish signal handlers */
	signal(SIGINT,  catchSignals);
	signal(SIGTERM, catchSignals);
	signal(SIGHUP,  catchSignals);

	signal(SIGUSR1, catchSignals);
	signal(SIGUSR2, catchSignals);

#if defined PTPD_SNMP
	/* Start SNMP subsystem */
	if (rtOpts->snmpEnabled)
		snmpInit(rtOpts, ptpClock);
#endif



	NOTICE(USER_DESCRIPTION" started successfully on %s using \"%s\" preset (PID %d)\n",
			    rtOpts->ifaceName,
			    (getPtpPreset(rtOpts->selectedPreset,rtOpts)).presetName,
			    getpid());
	ptpClock->resetStatisticsLog = TRUE;

#ifdef PTPD_STATISTICS

	outlierFilterSetup(&ptpClock->oFilterMS);
	outlierFilterSetup(&ptpClock->oFilterSM);

	ptpClock->oFilterMS.init(&ptpClock->oFilterMS,&rtOpts->oFilterMSConfig, "delayMS");
	ptpClock->oFilterSM.init(&ptpClock->oFilterSM,&rtOpts->oFilterSMConfig, "delaySM");


	if(rtOpts->filterMSOpts.enabled) {
		ptpClock->filterMS = createDoubleMovingStatFilter(&rtOpts->filterMSOpts,"delayMS");
	}

	if(rtOpts->filterSMOpts.enabled) {
		ptpClock->filterSM = createDoubleMovingStatFilter(&rtOpts->filterSMOpts, "delaySM");
	}

#endif

#ifdef PTPD_PCAP
		ptpClock->netPath.pcapEventSock = -1;
		ptpClock->netPath.pcapGeneralSock = -1;
#endif /* PTPD_PCAP */

		ptpClock->netPath.generalSock = -1;
		ptpClock->netPath.eventSock = -1;

	*ret = 0;

	return ptpClock;
	
fail:
	dictionary_del(&rtOpts->cliConfig);
	dictionary_del(&rtOpts->candidateConfig);
	dictionary_del(&rtOpts->currentConfig);
	return 0;
}

void
ntpSetup (RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	TimingService *ts = &ptpClock->ntpControl.timingService;



	if (rtOpts->ntpOptions.enableEngine) {
	    timingDomain.services[1] = ts;
	    strncpy(ts->id, "NTP0", TIMINGSERVICE_MAX_DESC);
	    ts->dataSet.priority1 = 0;
	    ts->dataSet.type = TIMINGSERVICE_NTP;
	    ts->config = &rtOpts->ntpOptions;
	    ts->controller = &ptpClock->ntpControl;
	    /* for now, NTP is considered always active, so will never go idle */
	    ts->timeout = 60;
	    ts->updateInterval = rtOpts->ntpOptions.checkInterval;
	    timingDomain.serviceCount = 2;
	} else {
	    timingDomain.serviceCount = 1;
	    timingDomain.services[1] = NULL;
	    if(timingDomain.best == ts || timingDomain.current == ts || timingDomain.preferred == ts) {
		timingDomain.best = timingDomain.current = timingDomain.preferred = NULL;
	    }
	}
}




