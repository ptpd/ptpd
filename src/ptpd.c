/*-
 * Copyright (c) 2012-2017 Wojciech Owczarek,
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
 * @file   ptpd.c
 * @date   Wed Jun 23 10:13:38 2010
 *
 * @brief  The main() function for the PTP daemon
 *
 * This file contains very little code, as should be obvious,
 * and only serves to tie together the rest of the daemon.
 * All of the default options are set here, but command line
 * arguments and configuration file is processed in the
 * ptpdStartup() routine called
 * below.
 */

#include "ptpd.h"

#include <stdbool.h>

#include <libcck/fd_set.h>
#include <libcck/net_utils.h>
#include <libcck/transport_address.h>
#include <libcck/cck_utils.h>
#include <libcck/libcck.h>
#include <libcck/timer.h>

static GlobalConfig global;			/* statically allocated run-time
					 * configuration data */

bool startupInProgress;

int
main(int argc, char **argv)
{

	PtpClock ptpClock;
	PtpClockUserData userData;

	int ret;

	memset(&userData, 0, sizeof(userData));
	memset(&ptpClock, 0, sizeof(ptpClock));

	userData.fdSet = getCckFdSet();

	ptpClock.userData = &userData;

	startupInProgress = true;

	/* Parse and check config, go into background */
	/* ret == 0: success. ret > 0: failure. ret < 0: no options given. */
	if(!ptpdStartup(argc, argv, &global, &ret)) {
	    if((ret > 0) && !global.checkConfigOnly) {
		    ERROR(USER_DESCRIPTION" startup failed\n");
	    }
	    /* no options given - return 1, but it's not a config failure */
	    if(ret < 0) {
		return 1;
	    }
	    return ret;
	}

	cckInit(getCckFdSet());
	configureLibCck(&global);

	if(!initPtpPort(&ptpClock, &global)) {
	    return 1;
	}

	ptpPortPostInit(&ptpClock);

	NOTICE(USER_DESCRIPTION" started successfully on %s using \"%s\" preset (PID %d)\n",
			    global.ifName,
			    (getPtpPreset(global.selectedPreset, &global)).presetName,
			    getpid());

	startupInProgress = false;

	tmrStart(&ptpClock, ALARM_UPDATE, ALARM_UPDATE_INTERVAL);
	/* run the status file update every 1 .. 1.2 seconds */
	tmrStart(&ptpClock, STATUSFILE_UPDATE, global.statusFileUpdateInterval * (1.0 + 0.2 * getRand()));
	tmrStart(&ptpClock, PERIODIC_INFO, global.statsUpdateInterval);

	if (!initPtpTransports(&ptpClock, getCckFdSet(), &global)) {
	    CRITICAL("Failed to start network transports!\n");
	    return 1;
	}

	toState(PTP_INITIALIZING, &global, &ptpClock);
	if(global.statusLog.logEnabled)
		writeStatusFile(&ptpClock, &global, TRUE);

	/* look, we have an event loop now... */
	while(true) {
	    cckPollData(getCckFdSet(), NULL);
	    cckDispatchTimers();
	    ptpRun(&global, &ptpClock);
	    checkSignals(&global, &ptpClock);
	    /* Configuration has changed */
	    if(global.restartSubsystems > 0) {
		restartSubsystems(&global, &ptpClock);
	    }
	}

	shutdownPtpPort(&ptpClock, &global);

        ptpdShutdown(&ptpClock);

	NOTIFY("Self shutdown\n");

	return 1;
}


bool initPtpPort(PtpClock *port, GlobalConfig *global)
{

	port->global = global;
	port->foreign = calloc(global->fmrCapacity, sizeof(ForeignMasterRecord));

	if (port->foreign == NULL) {
			PERROR("failed to allocate memory for foreign "
			       "master data");
			return false;
	}

	if(global->statisticsLog.logEnabled) {
		port->resetStatisticsLog = true;
	}

	if(!setupPtpTimers(port, getCckFdSet())) {
	    CRITICAL("Failed to initialise event timers. PTPd is inoperable.\n");
	    return false;
	}

	/* init alarms */
	initAlarms(port->alarms, ALRM_MAX, port);
	configureAlarms(port->alarms, ALRM_MAX, port);
	port->alarmDelay = global->alarmInitialDelay;

	/* we're delaying alarm processing - disable alarms for now */
	if(port->alarmDelay) {
	    enableAlarms(port->alarms, ALRM_MAX, FALSE);
	}

#if defined PTPD_SNMP
	/* Start SNMP subsystem */
	if (global->snmpEnabled)
		snmpInit(global, port);
#endif

	port->resetStatisticsLog = true;

	outlierFilterSetup(&port->oFilterMS);
	outlierFilterSetup(&port->oFilterSM);

	port->oFilterMS.init(&port->oFilterMS,&global->oFilterMSConfig, "delayMS");
	port->oFilterSM.init(&port->oFilterSM,&global->oFilterSMConfig, "delaySM");

	if(global->filterMSOpts.enabled) {
		port->filterMS = createDoubleMovingStatFilter(&global->filterMSOpts,"delayMS");
	}

	if(global->filterSMOpts.enabled) {
		port->filterSM = createDoubleMovingStatFilter(&global->filterSMOpts, "delaySM");
	}

	return true;

}

void shutdownPtpPort(PtpClock *port, GlobalConfig *global)
{

	/*
         * go into DISABLED state so the FSM can call any PTP-specific shutdown actions,
	 * such as canceling unicast transmission
         */
	toState(PTP_DISABLED, global, port);
	/* process any outstanding events before exit */
	updateAlarms(port->alarms, ALRM_MAX);

	shutdownPtpTransports(port);
	shutdownPtpTimers(port);

	ptpPortPreShutdown(port);

	free(port->foreign);

	/* free management and signaling messages, they can have dynamic memory allocated */
	if(port->msgTmpHeader.messageType == MANAGEMENT)
		freeManagementTLV(&port->msgTmp.manage);
	freeManagementTLV(&port->outgoingManageTmp);
	if(port->msgTmpHeader.messageType == SIGNALING)
		freeSignalingTLV(&port->msgTmp.signaling);
	freeSignalingTLV(&port->outgoingSignalingTmp);

#ifdef PTPD_SNMP
	snmpShutdown();
#endif /* PTPD_SNMP */

	port->oFilterMS.shutdown(&port->oFilterMS);
	port->oFilterSM.shutdown(&port->oFilterSM);
        freeDoubleMovingStatFilter(&port->filterMS);
        freeDoubleMovingStatFilter(&port->filterSM);

}


/* temporary */
GlobalConfig*
getGlobalConfig()
{
    return &global;
}
