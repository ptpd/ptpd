/*-
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
	ptpdShutdown(ptpClock);

	NOTIFY("Shutdown on close signal\n");
	exit(0);
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

	/* if we don't have a config file specified, we're done - just reopen log files*/
	if(strlen(rtOpts->configFile) ==  0)
	    goto end;

	dictionary* tmpConfig = dictionary_new(0);
	/* Try reloading the config file */
	NOTIFY("Reloading configuration file: %s\n",rtOpts->configFile);
            if(!loadConfigFile(&tmpConfig, rtOpts)) {
		dictionary_del(tmpConfig);
		goto end;
        }
		dictionary_merge(rtOpts->cliConfig, tmpConfig, 1, "from command line");
	/* Load default config to fill in the blanks in the config file */
	RunTimeOpts tmpOpts;
	loadDefaultSettings(&tmpOpts);

	/* Check the new configuration for errors, fill in the blanks from defaults */
	if( ( rtOpts->candidateConfig = parseConfig(tmpConfig,&tmpOpts)) == NULL ) {
	    WARNING("Configuration file has errors, reload aborted\n");
	    dictionary_del(tmpConfig);
	    goto end;
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
	    checkSubsystemRestart(rtOpts->candidateConfig, rtOpts->currentConfig);

	/* If we're told to re-check lock files, do it: tmpOpts already has what rtOpts should */
	if( (rtOpts->restartSubsystems & PTPD_CHECK_LOCKS) &&
	    tmpOpts.autoLockFile && !checkOtherLocks(&tmpOpts)) {
		rtOpts->restartSubsystems = -1;
	}

	/* If the network configuration has changed, check if the interface is OK */
	if(rtOpts->restartSubsystems & PTPD_RESTART_NETWORK) {
		INFO("Network configuration changed - checking interface\n");
		if(!testInterface(tmpOpts.ifaceName)) {
		    rtOpts->restartSubsystems = -1;
		    ERROR("Error: Cannot use %s interface\n",tmpOpts.ifaceName);
		}

	}

#ifdef linux
#ifdef HAVE_SCHED_H
                    /* Changing the CPU affinity mask */
                    if(rtOpts->restartSubsystems & PTPD_CHANGE_CPUAFFINITY) {
                        NOTIFY("Applying CPU binding configuration: changing selected CPU core\n");
                        cpu_set_t mask;
                        CPU_ZERO(&mask);
                        if(tmpOpts.cpuNumber > -1) {
                                CPU_SET(tmpOpts.cpuNumber,&mask);
                        } else {
				int i;
				for(i = 0;  i < CPU_SETSIZE; i++) {
				    CPU_SET(i, &mask);
				}
			}
                        if(sched_setaffinity(0, sizeof(mask), &mask) < 0) {
                                if(tmpOpts.cpuNumber == -1) {
                                        PERROR("Could not unbind from CPU core %d", rtOpts->cpuNumber);
                                } else {
                                        PERROR("Could bind to CPU core %d", tmpOpts.cpuNumber);
                                }
                                rtOpts->restartSubsystems = -1;
                        } else {
                                if(tmpOpts.cpuNumber > -1)
                                        INFO("Successfully bound "PTPD_PROGNAME" to CPU core %d\n", tmpOpts.cpuNumber);
                                else
                                        INFO("Successfully unbound "PTPD_PROGNAME" from cpu core CPU core %d\n", rtOpts->cpuNumber);
                        }
                    }
#endif /* HAVE_SCHED_H */
#endif /* linux */

	if(rtOpts->restartSubsystems == -1) {
		ERROR("New configuration cannot be applied - aborting reload\n");
		rtOpts->restartSubsystems = 0;
		goto cleanup;
	}


		/* Tell parseConfig to shut up - it's had its chance already */
		dictionary_set(rtOpts->candidateConfig,"%quiet%:%quiet%","Y");

		/**
		 * Commit changes to rtOpts and currentConfig
		 * (this should never fail as the config has already been checked if we're here)
		 * However if this DOES fail, some default has been specified out of range -
		 * this is the only situation where parse will succeed but commit not:
		 * disable quiet mode to show what went wrong, then die.
		 */
		if (rtOpts->currentConfig) {
			dictionary_del(rtOpts->currentConfig);
		}
		if ( (rtOpts->currentConfig = parseConfig(rtOpts->candidateConfig,rtOpts)) == NULL) {
			CRITICAL("************ "PTPD_PROGNAME": parseConfig returned NULL during config commit"
				 "  - this is a BUG - report the following: \n");

			dictionary_unset(rtOpts->candidateConfig,"%quiet%:%quiet%");

			if ((rtOpts->currentConfig = parseConfig(rtOpts->candidateConfig,rtOpts)) == NULL)
			    CRITICAL("*****************" PTPD_PROGNAME" shutting down **********************\n");
			/*
			 * Could be assert(), but this should be done any time this happens regardless of
			 * compile options. Anyhow, if we're here, the daemon will no doubt segfault soon anyway
			 */
			abort();
		}

	/* clean up */
	cleanup:

		dictionary_del(tmpConfig);
		dictionary_del(rtOpts->candidateConfig);

	end:

	if(rtOpts->recordLog.logEnabled ||
	    rtOpts->eventLog.logEnabled ||
	    (rtOpts->statisticsLog.logEnabled))
		INFO("Reopening log files\n");

	restartLogging(rtOpts);

	if(rtOpts->statisticsLog.logEnabled)
		ptpClock->resetStatisticsLog = TRUE;


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
		WARNING("SIGUSR1 received, stepping clock to current known OFM\n");
		servo_perform_clock_step(rtOpts, ptpClock);
	sigusr1_received = 0;
	}


#ifdef DBG_SIGUSR2_CHANGE_DOMAIN
	if(sigusr2_received){
		/* swap domain numbers */
		static int prev_domain;
		static int first_time = 1;
		if(first_time){
			first_time = 0;
			prev_domain = ptpClock->domainNumber + 1;
		}

		int  temp = ptpClock->domainNumber;
		ptpClock->domainNumber = prev_domain;
		prev_domain = temp;

		
		// propagate new choice as the run-time option
		rtOpts->domainNumber = ptpClock->domainNumber;
		
		WARNING("SigUSR2 received. PTP_Domain is now %d  (saved: %d)\n",
			ptpClock->domainNumber,
			prev_domain
		);
		sigusr2_received = 0;
	}
#endif

#ifdef DBG_SIGUSR2_DUMP_COUNTERS
	if(sigusr2_received){
		displayCounters(ptpClock);
		sigusr2_received = 0;
	}
#endif

		
#ifdef DBG_SIGUSR2_CHANGE_DEBUG
#ifdef RUNTIME_DEBUG
	if(sigusr2_received){
		/* cycle debug levels, from INFO (=no debug) to Verbose */
		INFO("Current debug level: %d\n", rtOpts->debug_level);
		
		(rtOpts->debug_level)++;
		if(rtOpts->debug_level > LOG_DEBUGV ){
			rtOpts->debug_level = LOG_INFO;
	}

		INFO("New debug level: %d\n", rtOpts->debug_level);
	sigusr2_received = 0;
	}
#endif
#endif
	
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

//	INFO("Checking lock file: %s\n", rtOpts->lockFile);

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
	return(1);
	failure:
	fclose(G_lockFilePointer);
	return(0);

}

void 
ptpdShutdown(PtpClock * ptpClock)
{

	extern RunTimeOpts rtOpts;

	netShutdown(&ptpClock->netPath);
#ifdef PTPD_NTPDC
	ntpShutdown(&rtOpts.ntpOptions, &ptpClock->ntpControl);
#endif /* PTPD_NTPDC */
	free(ptpClock->foreign);

	/* free management messages, they can have dynamic memory allocated */
	if(ptpClock->msgTmpHeader.messageType == MANAGEMENT)
		freeManagementTLV(&ptpClock->msgTmp.manage);
	freeManagementTLV(&ptpClock->outgoingManageTmp);

#if !defined(__APPLE__)
#ifndef PTPD_STATISTICS
	/* Not running statistics code - write observed drift to driftfile if enabled, inform user */
	if(ptpClock->slaveOnly && !ptpClock->servo.runningMaxOutput)
		saveDrift(ptpClock, &rtOpts, FALSE);
#else
	/* We are running statistics code - save drift on exit only if we're nor monitoring servo stability */
	if(!rtOpts.servoStabilityDetection && !ptpClock->servo.runningMaxOutput)
		saveDrift(ptpClock, &rtOpts, FALSE);
#endif /* PTPD_STATISTICS */
#endif /* apple */

	if (rtOpts.currentConfig != NULL)
		dictionary_del(rtOpts.currentConfig);
	if(rtOpts.cliConfig != NULL)
		dictionary_del(rtOpts.cliConfig);

	free(ptpClock);
	ptpClock = NULL;

	extern PtpClock* G_ptpClock;
	G_ptpClock = NULL;

	/* properly clean lockfile (eventough new deaemons can acquire the lock after we die) */
	fclose(G_lockFilePointer);
	unlink(rtOpts.lockFile);

	if(rtOpts.statusLog.logEnabled) {
		/* close and remove the status file */
		if(rtOpts.statusLog.logFP != NULL)
			fclose(rtOpts.statusLog.logFP);
		unlink(rtOpts.statusLog.logPath);
	}

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
	int i = 0;

	/* 
	 * Set the default mode for all newly created files - previously
	 * this was not the case for log files. This adds consistency
	 * and allows to use FILE* vs. fds everywhere
	 */
	umask(DEFAULT_UMASK);
	/** 
	 * If a required setting, such as interface name, or a setting
	 * requiring a range check is to be set via getopts_long,
	 * the respective currentConfig dictionary entry should be set,
	 * instead of just setting the rtOpts field.
	 *
	 * Config parameter evaluation priority order:
	 * 	1. Any dictionary keys set in the getopt_long loop
	 * 	2. CLI long section:key type options
	 * 	3. Config file (parsed last), merged with 2. and 3 - will be overwritten by CLO options
	 * 	4. Defaults and any rtOpts fields set in the getopt_long loop
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
	/* parse the normal short and long option, exit on error */
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
			dictionary_merge(rtOpts->cliConfig, rtOpts->candidateConfig, 1, "from command line");
		} else {
		    *ret = 1;
			dictionary_merge(rtOpts->cliConfig, rtOpts->candidateConfig, 1, "from command line");
		    goto configcheck;
		}
	} else {
		dictionary_merge(rtOpts->cliConfig, rtOpts->candidateConfig, 1, "from command line");
	}
	/**
	 * This is where the final checking  of the candidate settings container happens.
	 * A dictionary is returned with only the known options, explicitly set to defaults
	 * if not present. NULL is returned on any config error - parameters missing, out of range,
	 * etc. The getopt loop in loadCommandLineOptions() only sets keys verified here.
	 */
	if( ( rtOpts->currentConfig = parseConfig(rtOpts->candidateConfig,rtOpts)) == NULL ) {
	    *ret = 1;
	    dictionary_del(rtOpts->candidateConfig);
	    goto configcheck;
	}

	/* we've been told to print the lock file and exit cleanly */
	if(rtOpts->printLockFile) {
	    printf("%s\n", rtOpts->lockFile);
	    *ret = 0;
	    goto fail;
	}

	/* we don't need the candidate config any more */
	dictionary_del(rtOpts->candidateConfig);

	/* Check network before going into background */
	if(!testInterface(rtOpts->ifaceName)) {
	    ERROR("Error: Cannot use %s interface\n",rtOpts->ifaceName);
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
	    return 0;
	}

	/* Previous errors - exit */
	if(*ret !=0)
		return 0;

	/* First lock check, just to be user-friendly to the operator */
	if(!rtOpts->ignore_daemon_lock) {
		if(!writeLockFile(rtOpts)){
			/* check and create Lock */
			ERROR("Error: file lock failed (use -L or global:ignore_lock to ignore lock file)\n");
			*ret = 3;
			return 0;
		}
		/* check for potential conflicts when automatic lock files are used */
		if(!checkOtherLocks(rtOpts)) {
			*ret = 3;
			return 0;
		}
	}

	/* Manage log files: stats, log, status and quality file */
	restartLogging(rtOpts);

	/* Allocate memory after we're done with other checks but before going into daemon */
	ptpClock = (PtpClock *) calloc(1, sizeof(PtpClock));
	if (!ptpClock) {
		PERROR("Error: Failed to allocate memory for protocol engine data");
		*ret = 2;
		return 0;
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
			return 0;
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

	/* Init user_description */
	memset(ptpClock->user_description, 0, sizeof(ptpClock->user_description));
	memcpy(ptpClock->user_description, &USER_DESCRIPTION, sizeof(USER_DESCRIPTION));
	
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
			return 0;
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
			return 0;
		}
	}

#ifdef linux
#ifdef HAVE_SCHED_H

	/* Try binding to a single CPU core if configured to do so */

	if(rtOpts->cpuNumber > -1) {

		cpu_set_t mask;
    		CPU_ZERO(&mask);
    		CPU_SET(rtOpts->cpuNumber,&mask);
    		if(sched_setaffinity(0, sizeof(mask), &mask) < 0) {
		    PERROR("Could not bind to CPU core %d", rtOpts->cpuNumber);
		} else {
		    INFO("Successfully bound "PTPD_PROGNAME" to CPU core %d\n", rtOpts->cpuNumber);
		}
	}
#endif /* HAVE_SCHED_H */
#endif /* linux */

	/* use new synchronous signal handlers */
	signal(SIGINT,  catchSignals);
	signal(SIGTERM, catchSignals);
	signal(SIGHUP,  catchSignals);

	signal(SIGUSR1, catchSignals);
	signal(SIGUSR2, catchSignals);

#if defined PTPD_SNMP
	/* Start SNMP subsystem */
	if (rtOpts->snmp_enabled)
		snmpInit(ptpClock);
#endif



	NOTICE(USER_DESCRIPTION" started successfully on %s using \"%s\" preset (PID %d)\n",
			    rtOpts->ifaceName,
			    (getPtpPreset(rtOpts->selectedPreset,rtOpts)).presetName,
			    getpid());
	ptpClock->resetStatisticsLog = TRUE;

#ifdef PTPD_STATISTICS
	if (rtOpts->delayMSOutlierFilterEnabled) {
		ptpClock->delayMSRawStats = createDoubleMovingStdDev(rtOpts->delayMSOutlierFilterCapacity);
		strncpy(ptpClock->delayMSRawStats->identifier, "delayMS", 10);
		ptpClock->delayMSFiltered = createDoubleMovingMean(rtOpts->delayMSOutlierFilterCapacity);
	} else {
		ptpClock->delayMSRawStats = NULL;
		ptpClock->delayMSFiltered = NULL;
	}

	if (rtOpts->delaySMOutlierFilterEnabled) {
		ptpClock->delaySMRawStats = createDoubleMovingStdDev(rtOpts->delaySMOutlierFilterCapacity);
		strncpy(ptpClock->delaySMRawStats->identifier, "delaySM", 10);
		ptpClock->delaySMFiltered = createDoubleMovingMean(rtOpts->delaySMOutlierFilterCapacity);
	} else {
		ptpClock->delaySMRawStats = NULL;
		ptpClock->delaySMFiltered = NULL;
	}
#endif

	*ret = 0;
	return ptpClock;
	
fail:
	dictionary_del(rtOpts->candidateConfig);
	return 0;
}
