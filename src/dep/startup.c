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
#include "cck_glue.h"
#include <libcck/cck_logger.h>

#ifndef HAVE_DAEMON
extern int daemon(int nochdir, int noclose);
#endif

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

static void exitHandler(PtpClock * ptpClock);

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

void
applyConfig(dictionary *baseConfig, GlobalConfig *global, PtpClock *ptpClock)
{

	Boolean reloadSuccessful = TRUE;


	/* Load default config to fill in the blanks in the config file */
	GlobalConfig tmpOpts;
	loadDefaultSettings(&tmpOpts);

	/* Check the new configuration for errors, fill in the blanks from defaults */
	if( ( global->candidateConfig = parseConfig(CFGOP_PARSE, NULL, baseConfig, &tmpOpts)) == NULL ) {
	    WARNING("Configuration has errors, reload aborted\n");
	    return;
	}

	/* Check for changes between old and new configuration */
	if(compareConfig(global->candidateConfig,global->currentConfig)) {
	    INFO("Configuration unchanged\n");
	    goto cleanup;
	}

	/*
	 * Mark which subsystems have to be restarted. Most of this will be picked up by doState()
	 * If there are errors past config correctness (such as non-existent NIC,
	 * or lock file clashes if automatic lock files used - abort the mission
	 */

	global->restartSubsystems =
	    checkSubsystemRestart(global->candidateConfig, global->currentConfig, global);

	/* If we're told to re-check lock files, do it: tmpOpts already has what global should */
	if( (global->restartSubsystems & PTPD_CHECK_LOCKS) &&
	    tmpOpts.autoLockFile && !checkOtherLocks(&tmpOpts)) {
		reloadSuccessful = FALSE;
	}

	/* If the network configuration has changed, check if the interface is OK */
	if(global->restartSubsystems & PTPD_RESTART_NETWORK) {
		INFO("Network configuration changed - checking interface(s)\n");
		int family = getConfiguredFamily(&tmpOpts);
		if(!testInterface(tmpOpts.ifName, family, tmpOpts.sourceAddress)) {
		    reloadSuccessful = FALSE;
		    ERROR("Error: Cannot use %s interface\n",tmpOpts.ifName);
		}
	}
#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined(__QNXNTO__)
        /* Changing the CPU affinity mask */
        if(global->restartSubsystems & PTPD_CHANGE_CPUAFFINITY) {
                NOTIFY("Applying CPU binding configuration: changing selected CPU core\n");

                if(setCpuAffinity(tmpOpts.cpuNumber) < 0) {
                        if(tmpOpts.cpuNumber == -1) {
                                ERROR("Could not unbind from CPU core %d\n", global->cpuNumber);
                        } else {
                                ERROR("Could bind to CPU core %d\n", tmpOpts.cpuNumber);
                        }
			reloadSuccessful = FALSE;
                } else {
                        if(tmpOpts.cpuNumber > -1)
                                INFO("Successfully bound "PTPD_PROGNAME" to CPU core %d\n", tmpOpts.cpuNumber);
                        else
                                INFO("Successfully unbound "PTPD_PROGNAME" from cpu core CPU core %d\n", global->cpuNumber);
                }
         }
#endif

	if(!reloadSuccessful) {
		ERROR("New configuration cannot be applied - aborting reload\n");
		global->restartSubsystems = 0;
		goto cleanup;
	}


		/**
		 * Commit changes to global and currentConfig
		 * (this should never fail as the config has already been checked if we're here)
		 * However if this DOES fail, some default has been specified out of range -
		 * this is the only situation where parse will succeed but commit not:
		 * disable quiet mode to show what went wrong, then die.
		 */
		if (global->currentConfig) {
			dictionary_del(&global->currentConfig);
		}
		if ( (global->currentConfig = parseConfig(CFGOP_PARSE_QUIET, NULL, global->candidateConfig,global)) == NULL) {
			CRITICAL("************ "PTPD_PROGNAME": parseConfig returned NULL during config commit"
				 "  - this is a BUG - report the following: \n");

			if ((global->currentConfig = parseConfig(CFGOP_PARSE, NULL, global->candidateConfig,global)) == NULL)
			    CRITICAL("*****************" PTPD_PROGNAME" shutting down **********************\n");
			/*
			 * Could be assert(), but this should be done any time this happens regardless of
			 * compile options. Anyhow, if we're here, the daemon will no doubt segfault soon anyway
			 */
			abort();
		}

	/* clean up */
	cleanup:

		dictionary_del(&global->candidateConfig);
}


/**
 * Signal handler for HUP which tells us to swap the log file
 * and reload configuration file if specified
 *
 * @param sig
 */
void
sighupHandler(GlobalConfig * global, PtpClock * ptpClock)
{



	NOTIFY("SIGHUP received\n");

	if(global->transportMode != TMODE_UC) {
		DBG("SIGHUP - running multicast, re-joining multicast on all transports\n");
		ptpNetworkRefresh(ptpClock);
	}

	/* if we don't have a config file specified, we're done - just reopen log files*/
	if(strlen(global->configFile) !=  0) {

	    dictionary* tmpConfig = dictionary_new(0);

	    /* Try reloading the config file */
	    NOTIFY("Reloading configuration file: %s\n",global->configFile);

            if(!loadConfigFile(&tmpConfig, global)) {

		    dictionary_del(&tmpConfig);

	    } else {
		    dictionary_merge(global->cliConfig, tmpConfig, 1, 1, "from command line");
		    applyConfig(tmpConfig, global, ptpClock);
		    dictionary_del(&tmpConfig);

	    }

	}

	if(global->recordLog.logEnabled ||
	    global->eventLog.logEnabled ||
	    (global->statisticsLog.logEnabled))
		INFO("Reopening log files\n");

	restartLogging(global);

	if(global->statisticsLog.logEnabled)
		ptpClock->resetStatisticsLog = TRUE;

}

void
restartSubsystems(GlobalConfig *global, PtpClock *ptpClock)
{

		DBG("RestartSubsystems: %d\n",global->restartSubsystems);
	    /* So far, PTP_INITIALIZING is required for both network and protocol restart */
	    if((global->restartSubsystems & PTPD_RESTART_PROTOCOL) ||
		(global->restartSubsystems & PTPD_RESTART_NETWORK)) {
			    if(global->restartSubsystems & PTPD_RESTART_NETWORK) {
			NOTIFY("Applying network configuration: going into PTP_INITIALIZING\n");
		    }
		    /* These parameters have to be passed to ptpClock before re-init */
		    ptpClock->defaultDS.clockQuality.clockClass = global->clockQuality.clockClass;
		    ptpClock->defaultDS.slaveOnly = global->slaveOnly;
		    ptpClock->disabled = global->portDisabled;
			    if(global->restartSubsystems & PTPD_RESTART_PROTOCOL) {
			INFO("Applying protocol configuration: going into %s\n",
			ptpClock->disabled ? "PTP_DISABLED" : "PTP_INITIALIZING");
		    }
		    toState(ptpClock->disabled ? PTP_DISABLED : PTP_INITIALIZING, global, ptpClock);
	    } else {

	    if(global->restartSubsystems & PTPD_UPDATE_DATASETS) {
			NOTIFY("Applying PTP engine configuration: updating datasets\n");
			updateDatasets(ptpClock, global);
	    }}
	    /* Nothing happens here for now - SIGHUP handler does this anyway */
	    if(global->restartSubsystems & PTPD_RESTART_LOGGING) {
			NOTIFY("Applying logging configuration: restarting logging\n");
	    }
		if(global->restartSubsystems & PTPD_RESTART_ACLS) {
        		NOTIFY("Applying access control list configuration\n");
		configureAcls(ptpClock, global);
		}
    		if(global->restartSubsystems & PTPD_RESTART_ALARMS) {
	    NOTIFY("Applying alarm configuration\n");
	    configureAlarms(ptpClock->alarms, ALRM_MAX, (void*)ptpClock);
	}
                    /* Reinitialising the outlier filter containers */
                if(global->restartSubsystems & PTPD_RESTART_FILTERS) {
                        NOTIFY("Applying filter configuration: re-initialising filters\n");
			freeDoubleMovingStatFilter(&ptpClock->filterMS);
			freeDoubleMovingStatFilter(&ptpClock->filterSM);
    			ptpClock->oFilterMS.shutdown(&ptpClock->oFilterMS);
			ptpClock->oFilterSM.shutdown(&ptpClock->oFilterSM);

			outlierFilterSetup(&ptpClock->oFilterMS);
			outlierFilterSetup(&ptpClock->oFilterSM);

			ptpClock->oFilterMS.init(&ptpClock->oFilterMS,&global->oFilterMSConfig, "delayMS");
			ptpClock->oFilterSM.init(&ptpClock->oFilterSM,&global->oFilterSMConfig, "delaySM");


			if(global->filterMSOpts.enabled) {
				ptpClock->filterMS = createDoubleMovingStatFilter(&global->filterMSOpts,"delayMS");
			}

			if(global->filterSMOpts.enabled) {
				ptpClock->filterSM = createDoubleMovingStatFilter(&global->filterSMOpts, "delaySM");
			}

		}

		configureLibCck(global);

		controlClockDrivers(CD_NOTINUSE, NULL);
		prepareClockDrivers(ptpClock, global);
		createClockDriversFromString(global->extraClocks, configureClockDriver, global, TRUE);
		/* check if we want a master clock */
		ClockDriver *masterClock = NULL;
		if(strlen(global->masterClock) > 0) {
		    masterClock = getClockDriverByName(global->masterClock);
		    if(masterClock == NULL) {
			WARNING("Could not find designated master clock: %s\n", global->masterClock);
		    }
		}
		setCckMasterClock(masterClock);

		/* clean up unused clock drivers */
		controlClockDrivers(CD_CLEANUP, NULL);
		reconfigureClockDrivers(configureClockDriver, global);

		    /* Config changes don't require subsystem restarts - acknowledge it */
		    if(global->restartSubsystems == PTPD_RESTART_NONE) {
				NOTIFY("Applying configuration\n");
		    }

		    if(global->restartSubsystems != -1)
			    global->restartSubsystems = 0;

}


/*
 * Synchronous signal processing:
 * This function should be called regularly from the main loop
 */
void
checkSignals(GlobalConfig * global, PtpClock * ptpClock)
{

	TTransport *event = ptpClock->eventTransport;
	TTransport *general = ptpClock->generalTransport;

	/*
	 * note:
	 * alarm signals are handled in a similar way in dep/timer.c
	 */

	if(sigint_received || sigterm_received){
		exitHandler(ptpClock);
	}

	if(sighup_received){
		sighupHandler(global, ptpClock);
		sighup_received=0;
	}

	if(sigusr1_received){
		WARNING("SIGUSR1 received, stepping all clocks to current known offset\n");
		stepClocks(TRUE);
		sigusr1_received = 0;
	}

	if(sigusr2_received){

/* testing only: testing step detection */
#if 0
		{
		ptpClock->addOffset ^= 1;
		INFO("a: %d\n", ptpClock->addOffset);
		sigusr2_received = 0;
		}
#endif
		displayCounters(ptpClock);
		displayAlarms(ptpClock->alarms, ALRM_MAX);

    		if(event) {
		    event->dataAcl->dump(event->dataAcl);
		    event->controlAcl->dump(event->controlAcl);
		    if(global->clearCounters) {
			event->clearCounters(event);
		    }
		}

		if(general && (general != event)) {
		    general->dataAcl->dump(general->dataAcl);
		    general->controlAcl->dump(general->controlAcl);
		    if(global->clearCounters) {
			general->clearCounters(general);
		    }
		}

		controlTTransports(TT_DUMP, NULL);

		if(global->oFilterSMConfig.enabled) {
			ptpClock->oFilterSM.display(&ptpClock->oFilterSM);
		}
		if(global->oFilterMSConfig.enabled) {
			ptpClock->oFilterMS.display(&ptpClock->oFilterMS);
		}
		sigusr2_received = 0;

		INFO("Inter-clock offset table:\n");
		compareAllClocks();

		if(global->clearCounters) {
			clearCounters(ptpClock);
			NOTIFY("* PTP engine counters cleared*\n");
		}

	}

}

#ifdef RUNTIME_DEBUG
/* These functions are useful to temporarily enable Debug around parts of code, similar to bash's "set -x" */
void enable_runtime_debug(void )
{
	GlobalConfig *global = getGlobalConfig();
	global->debug_level = max(LOG_DEBUGV, global->debug_level);
}

void disable_runtime_debug(void )
{
	GlobalConfig *global = getGlobalConfig();
	global->debug_level = LOG_INFO;
}
#endif

int
writeLockFile(GlobalConfig * global)
{

	int lockPid = 0;

	DBGV("Checking lock file: %s\n", global->lockFile);

	if ( (G_lockFilePointer=fopen(global->lockFile, "w+")) == NULL) {
		PERROR("Could not open lock file %s for writing", global->lockFile);
		return(0);
	}
	if (lockFile(fileno(G_lockFilePointer)) < 0) {
		if ( checkLockStatus(fileno(G_lockFilePointer),
			DEFAULT_LOCKMODE, &lockPid) == 0) {
			     ERROR("Another "PTPD_PROGNAME" instance is running: %s locked by PID %d\n",
				    global->lockFile, lockPid);
		} else {
			PERROR("Could not acquire lock on %s:", global->lockFile);
		}
		goto failure;
	}
	if(ftruncate(fileno(G_lockFilePointer), 0) == -1) {
		PERROR("Could not truncate %s: %s",
			global->lockFile, strerror(errno));
		goto failure;
	}
	if ( fprintf(G_lockFilePointer, "%ld\n", (long)getpid()) == -1) {
		PERROR("Could not write to lock file %s: %s",
			global->lockFile, strerror(errno));
		goto failure;
	}
	INFO("Successfully acquired lock on %s\n", global->lockFile);
	fflush(G_lockFilePointer);
	return(1);
	failure:
	fclose(G_lockFilePointer);
	return(0);

}

/*
 * exit the program cleanly
 */
static void
exitHandler(PtpClock * ptpClock)
{
	NOTIFY(PTPD_PROGNAME" shutting down on close signal\n");

	shutdownPtpPort(ptpClock, ptpClock->global);
	ptpdShutdown(ptpClock);

	NOTIFY(PTPD_PROGNAME" exiting\n");
	exit(0);
}

void
ptpdShutdown(PtpClock * ptpClock)
{

	GlobalConfig *global = getGlobalConfig();
	
	cckShutdown();

	if (global->currentConfig != NULL)
		dictionary_del(&global->currentConfig);
	if(global->cliConfig != NULL)
		dictionary_del(&global->cliConfig);

	/* properly clean lockfile (eventough new deaemons can acquire the lock after we die) */
	if(!global->ignoreLock && G_lockFilePointer != NULL) {
	    fclose(G_lockFilePointer);
	    G_lockFilePointer = NULL;
	}

	unlink(global->lockFile);

	if(global->statusLog.logEnabled) {
		/* close and remove the status file */
		if(global->statusLog.logFP != NULL) {
			fclose(global->statusLog.logFP);
			global->statusLog.logFP = NULL;
		}
		unlink(global->statusLog.logPath);
	}

	stopLogging(global);

}

void argvDump(int argc, char **argv)
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

bool
ptpdStartup(int argc, char **argv, GlobalConfig * global, int *ret)
{

	TimeInternal tmpTime;
	int i = 0;

	/* pass our log message writer function to libCCK */
	cckSetLoggerFun(logMessageWrapper);

	/*
	 * Set the default mode for all newly created files - previously
	 * this was not the case for log files. This adds consistency
	 * and allows to use FILE* vs. fds everywhere
	 */
	umask(~DEFAULT_FILE_PERMS);

	/* get some entropy in... */
	getSystemTime(&tmpTime);
	srand(tmpTime.seconds ^ tmpTime.nanoseconds);

	/**
	 * If a required setting, such as interface name, or a setting
	 * requiring a range check is to be set via getopts_long,
	 * the respective currentConfig dictionary entry should be set,
	 * instead of just setting the global field.
	 *
	 * Config parameter evaluation priority order:
	 * 	1. Any dictionary keys set in the getopt_long loop
	 * 	2. CLI long section:key type options
	 * 	3. Any built-in config templates
	 *	4. Any templates loaded from template file
	 * 	5. Config file (parsed last), merged with 2. and 3 - will be overwritten by CLI options
	 * 	6. Defaults and any global fields set in the getopt_long loop
	**/

	/**
	 * Load defaults. Any options set here and further inside loadCommandLineOptions()
	 * by setting global fields, will be considered the defaults
	 * for config file and section:key long options.
	 */
	loadDefaultSettings(global);
	/* initialise the config dictionary */
	global->candidateConfig = dictionary_new(0);
	global->cliConfig = dictionary_new(0);

	/* parse all long section:key options and clean up argv for getopt */
	loadCommandLineKeys(global->cliConfig,argc,argv);
	/* parse the normal short and long options, exit on error */
	if (!loadCommandLineOptions(global, global->cliConfig, argc, argv, ret)) {
	    goto fail;
	}

	/* Display startup info and argv if not called with -? or -H */
		NOTIFY("%s version %s starting\n",USER_DESCRIPTION, USER_VERSION);
		argvDump(argc, argv);
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
	if(strlen(global->configFile) > 0) {
		/* config file settings overwrite all others, except for empty strings */
		INFO("Loading configuration file: %s\n",global->configFile);
		if(loadConfigFile(&global->candidateConfig, global)) {
			dictionary_merge(global->cliConfig, global->candidateConfig, 1, 1, "from command line");
		} else {
		    *ret = 1;
		    dictionary_merge(global->cliConfig, global->candidateConfig, 1, 1, "from command line");
		    goto configcheck;
		}
	} else {
		dictionary_merge(global->cliConfig, global->candidateConfig, 1, 1, "from command line");
	}
	/**
	 * This is where the final checking  of the candidate settings container happens.
	 * A dictionary is returned with only the known options, explicitly set to defaults
	 * if not present. NULL is returned on any config error - parameters missing, out of range,
	 * etc. The getopt loop in loadCommandLineOptions() only sets keys verified here.
	 */
	if( ( global->currentConfig = parseConfig(CFGOP_PARSE, NULL, global->candidateConfig,global)) == NULL ) {
	    *ret = 1;
	    dictionary_del(&global->candidateConfig);
	    goto configcheck;
	}

	/* we've been told to print the lock file and exit cleanly */
	if(global->printLockFile) {
	    printf("%s\n", global->lockFile);
	    *ret = 0;
	    goto fail;
	}

	/* we don't need the candidate config any more */
	dictionary_del(&global->candidateConfig);

	/* Check network before going into background */
	INFO("Checking if network interface(s) are usable\n");

	if(!testTransportConfig(global)) {
	    *ret = 1;
	    goto configcheck;
	}

	INFO("Interfaces OK\n");

configcheck:
	/*
	 * We've been told to check config only - clean exit before checking locks
	 */
	if(global->checkConfigOnly) {
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
	if(!global->ignoreLock) {
		if(!writeLockFile(global)){
			/* check and create Lock */
			ERROR("Error: file lock failed (use -L or global:ignore_lock to ignore lock file)\n");
			*ret = 3;
			goto fail;
		}
		/* check for potential conflicts when automatic lock files are used */
		if(!checkOtherLocks(global)) {
			*ret = 3;
			goto fail;
		}
	}

	/* Manage log files: stats, log, status and quality file */
	restartLogging(global);

	/*  DAEMON */
#ifdef PTPD_NO_DAEMON
	if(!global->nonDaemon){
		global->nonDaemon = TRUE;
	}
#endif

	if(!global->nonDaemon){
		/*
		 * fork to daemon - nochdir non-zero to preserve the working directory:
		 * allows relative paths to be used for log files, config files etc.
		 * Always redirect stdout/err to /dev/null
		 */
		if (daemon(1,0) == -1) {
			PERROR("Failed to fork to background");
			*ret = 3;
			goto fail;
		}
		INFO("Now running in the background\n");
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
	if(!global->nonDaemon && !global->ignoreLock){
		/* check and create Lock */
		if(!writeLockFile(global)){
			ERROR("Error: file lock failed (use -L or global:ignore_lock to ignore lock file)\n");
			*ret = 3;
			goto fail;
		}
	}

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined(__QNXNTO__)
	/* Try binding to a single CPU core if configured to do so */
	if(global->cpuNumber > -1) {
    		if(setCpuAffinity(global->cpuNumber) < 0) {
		    ERROR("Could not bind to CPU core %d\n", global->cpuNumber);
		} else {
		    INFO("Successfully bound "PTPD_PROGNAME" to CPU core %d\n", global->cpuNumber);
		}
	}
#endif
	/* establish signal handlers */
	signal(SIGINT,  catchSignals);
	signal(SIGTERM, catchSignals);
	signal(SIGHUP,  catchSignals);

	signal(SIGUSR1, catchSignals);
	signal(SIGUSR2, catchSignals);
	*ret = 0;
	return true;
	
fail:
	dictionary_del(&global->cliConfig);
	dictionary_del(&global->candidateConfig);
	dictionary_del(&global->currentConfig);
	return false;
}
