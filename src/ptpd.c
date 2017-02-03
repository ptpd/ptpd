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

#include <libcck/net_utils.h>
#include <libcck/transport_address.h>
#include <libcck/cck_utils.h>
#include <libcck/timer.h>

RunTimeOpts rtOpts;			/* statically allocated run-time
					 * configuration data */

Boolean startupInProgress;

/*
 * Global variable with the main PTP port. This is used to show the current state in DBG()/message()
 * without having to pass the pointer everytime.
 *
 * if ptpd is extended to handle multiple ports (eg, to instantiate a Boundary Clock),
 * then DBG()/message() needs a per-port pointer argument
 */
PtpClock *G_ptpClock = NULL;

int
main(int argc, char **argv)
{

	PtpClock *ptpClock;
	Integer16 ret;

	PtpClockUserData userData;

	memset(&userData, 0, sizeof(userData));

	userData.fdSet = getCckFdSet();

	startupInProgress = TRUE;

	/* Initialize run time options with command line arguments */
	if (!(ptpClock = ptpdStartup(argc, argv, &ret, &rtOpts))) {
		if (ret != 0 && !rtOpts.checkConfigOnly)
			ERROR(USER_DESCRIPTION" startup failed\n");
		return ret;
	}

	startupInProgress = FALSE;

	cckInit(getCckFdSet());

	/* global variable for message(), please see comment on top of this file */
	G_ptpClock = ptpClock;

	DBG("event POWERUP\n");

	if(!setupPtpTimers(ptpClock, getCckFdSet())) {
	    CRITICAL("Failed to initialise event timers. PTPd is inoperable.\n");
	    return 1;
	}

	tStart(ptpClock, ALARM_UPDATE ,ALARM_UPDATE_INTERVAL);
	/* run the status file update every 1 .. 1.2 seconds */
	tStart(ptpClock, STATUSFILE_UPDATE, rtOpts.statusFileUpdateInterval * (1.0 + 0.2 * getRand()));
	tStart(ptpClock, PERIODIC_INFO, rtOpts.statsUpdateInterval);

	if (!netInit(ptpClock, getCckFdSet())) {
	    CRITICAL("Failed to start network transports!\n");
	    return 1;
	}

	toState(PTP_INITIALIZING, &rtOpts, ptpClock);
	if(rtOpts.statusLog.logEnabled)
		writeStatusFile(ptpClock, &rtOpts, TRUE);


	/* look, we have an event loop now... */
	while(true) {
	    cckPollData(getCckFdSet(), NULL);
	    cckDispatchTimers();
	    ptpRun(&rtOpts, ptpClock);
	    checkSignals(&rtOpts, ptpClock);
	    /* Configuration has changed */
	    if(rtOpts.restartSubsystems > 0) {
		restartSubsystems(&rtOpts, ptpClock);
	    }
	}

        ptpdShutdown(ptpClock);

	NOTIFY("Self shutdown\n");

	return 1;
}
