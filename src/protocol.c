/*-
 * Copyright (c) 2012-2013 Wojciech Owczarek,
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
 * @file   protocol.c
 * @date   Wed Jun 23 09:40:39 2010
 * 
 * @brief  The code that handles the IEEE-1588 protocol and state machine
 * 
 * 
 */

#include "ptpd.h"

Boolean doInit(RunTimeOpts*,PtpClock*);
static void doState(RunTimeOpts*,PtpClock*);

void handle(RunTimeOpts*,PtpClock*);
static void handleAnnounce(MsgHeader*, ssize_t,Boolean, RunTimeOpts*,PtpClock*);
static void handleSync(const MsgHeader*, ssize_t,TimeInternal*,Boolean,RunTimeOpts*,PtpClock*);
static void handleFollowUp(const MsgHeader*, ssize_t,Boolean,RunTimeOpts*,PtpClock*);
static void handlePDelayReq(MsgHeader*, ssize_t,const TimeInternal*,Boolean,RunTimeOpts*,PtpClock*);
void handleDelayReq(const MsgHeader*, ssize_t, const TimeInternal*,Boolean,RunTimeOpts*,PtpClock*);
static void handlePDelayResp(const MsgHeader*, TimeInternal* ,ssize_t,Boolean,RunTimeOpts*,PtpClock*);
static void handleDelayResp(const MsgHeader*, ssize_t, RunTimeOpts*,PtpClock*);
static void handlePDelayRespFollowUp(const MsgHeader*, ssize_t, Boolean, RunTimeOpts*,PtpClock*);
static void handleManagement(MsgHeader*, Boolean,RunTimeOpts*,PtpClock*);
static void handleSignaling(PtpClock*);
static void updateDatasets(PtpClock* ptpClock, RunTimeOpts* rtOpts);

static void issueAnnounce(RunTimeOpts*,PtpClock*);
static void issueSync(RunTimeOpts*,PtpClock*);
static void issueFollowup(const TimeInternal*,RunTimeOpts*,PtpClock*, const UInteger16);
static void issuePDelayReq(RunTimeOpts*,PtpClock*);
static void issueDelayReq(RunTimeOpts*,PtpClock*);
static void issuePDelayResp(const TimeInternal*,MsgHeader*,RunTimeOpts*,PtpClock*);
static void issueDelayResp(const TimeInternal*,MsgHeader*,RunTimeOpts*,PtpClock*);
static void issuePDelayRespFollowUp(const TimeInternal*,MsgHeader*,RunTimeOpts*,PtpClock*, const UInteger16);
#if 0
static void issueManagement(MsgHeader*,MsgManagement*,RunTimeOpts*,PtpClock*);
#endif
static void issueManagementRespOrAck(MsgManagement*,RunTimeOpts*,PtpClock*);
static void issueManagementErrorStatus(MsgManagement*,RunTimeOpts*,PtpClock*);
static void processMessage(RunTimeOpts* rtOpts, PtpClock* ptpClock, TimeInternal* timeStamp, ssize_t length);
static void processSyncFromSelf(const TimeInternal * tint, RunTimeOpts * rtOpts, PtpClock * ptpClock, const UInteger16 sequenceId);
static void processDelayReqFromSelf(const TimeInternal * tint, RunTimeOpts * rtOpts, PtpClock * ptpClock);
static void processPDelayReqFromSelf(const TimeInternal * tint, RunTimeOpts * rtOpts, PtpClock * ptpClock);
static void processPDelayRespFromSelf(const TimeInternal * tint, RunTimeOpts * rtOpts, PtpClock * ptpClock, const UInteger16 sequenceId);

void addForeign(Octet*,MsgHeader*,PtpClock*);

void clearCounters(PtpClock *);

/* loop forever. doState() has a switch for the actions and events to be
   checked for 'port_state'. the actions and events may or may not change
   'port_state' by calling toState(), but once they are done we loop around
   again and perform the actions required for the new 'port_state'. */
void 
protocol(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBG("event POWERUP\n");

	toState(PTP_INITIALIZING, rtOpts, ptpClock);
	if(rtOpts->statusLog.logEnabled)
		writeStatusFile(ptpClock, rtOpts, TRUE);

#ifdef PTPD_NTPDC
	/* 
	 * The deamon is just starting - if NTP failover enabled, start the timers now,
	 * so that if we haven't found a master, we will fail over.
	 */
	if(rtOpts->ntpOptions.enableEngine) {
	    timerStart(NTPD_CHECK_TIMER,rtOpts->ntpOptions.checkInterval,ptpClock->itimer);

			  /* NTP is the desired clock controller - don't wait or failover, flip now */
                        if(rtOpts->ntpOptions.ntpInControl) {
                                ptpClock->ntpControl.isRequired = rtOpts->ntpOptions.ntpInControl;
                                        if(!ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, FALSE))
                                                DBG("STARTUP - NTP preferred - could not fail over\n");
                        } else if(rtOpts->ntpOptions.enableFailover) {
                                /* We have a timeout defined */
                                if(rtOpts->ntpOptions.failoverTimeout) {
                                        DBG("NTP failover timer started\n");
                                        timerStart(NTPD_FAILOVER_TIMER,
                                                    rtOpts->ntpOptions.failoverTimeout,
                                                    ptpClock->itimer);
                                /* Fail over to NTP straight away */
                                } else {
                                        DBG("Initiating NTP failover\n");
                                        ptpClock->ntpControl.isRequired = TRUE;
                                        ptpClock->ntpControl.isFailOver = TRUE;
                                        if(!ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, FALSE))
                                                DBG("STARTUP instant NTP failover - could not fail over\n");
                                }
                        }
	}
#endif /* PTPD_NTPDC */

	timerStart(STATUSFILE_UPDATE_TIMER,rtOpts->statusFileUpdateInterval,ptpClock->itimer);

	DBG("Debug Initializing...\n");

	if(rtOpts->statusLog.logEnabled)
		writeStatusFile(ptpClock, rtOpts, TRUE);

	for (;;)
	{
		/* 20110701: this main loop was rewritten to be more clear */

		if (ptpClock->portState == PTP_INITIALIZING) {
			if (!doInit(rtOpts, ptpClock)) {
				return;
			}
		} else {
			doState(rtOpts, ptpClock);
		}

		if (ptpClock->message_activity)
			DBGV("activity\n");

		/* Configuration has changed */
		if(rtOpts->restartSubsystems > 0) {
			DBG("RestartSubsystems: %d\n",rtOpts->restartSubsystems);
		    /* So far, PTP_INITIALIZING is required for both network and protocol restart */
		    if((rtOpts->restartSubsystems & PTPD_RESTART_PROTOCOL) ||
			(rtOpts->restartSubsystems & PTPD_RESTART_NETWORK)) {
			    if(rtOpts->restartSubsystems & PTPD_RESTART_PROTOCOL) {
				INFO("Applying protocol configuration: going into PTP_INITIALIZING\n");
			    }

			    if(rtOpts->restartSubsystems & PTPD_RESTART_NETWORK) {
				NOTIFY("Applying network configuration: going into PTPD_INITIALIZING\n");
			    }
			    toState(PTP_INITIALIZING, rtOpts, ptpClock);
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

#ifdef PTPD_STATISTICS
                    /* Reinitialising the outlier filter containers */
                    if(rtOpts->restartSubsystems & PTPD_RESTART_PEIRCE) {
                                NOTIFY("Applying outlier filter configuration: re-initialising filters\n");

                                if(ptpClock->delayMSRawStats != NULL )
                                        freeDoubleMovingStdDev(&ptpClock->delayMSRawStats);
                                if(ptpClock->delayMSFiltered != NULL )
                                        freeDoubleMovingMean(&ptpClock->delayMSFiltered);

                                if(ptpClock->delaySMRawStats != NULL )
                                        freeDoubleMovingStdDev(&ptpClock->delaySMRawStats);
                                if(ptpClock->delaySMFiltered != NULL )
                                        freeDoubleMovingMean(&ptpClock->delaySMFiltered);

                                if (rtOpts->delayMSOutlierFilterEnabled) {
                                        ptpClock->delayMSRawStats = createDoubleMovingStdDev(rtOpts->delayMSOutlierFilterCapacity);
                                        strncpy(ptpClock->delayMSRawStats->identifier, "delayMS", 10);
                                        ptpClock->delayMSFiltered = createDoubleMovingMean(rtOpts->delayMSOutlierFilterCapacity);
                                }

                                if (rtOpts->delaySMOutlierFilterEnabled) {
                                        ptpClock->delaySMRawStats = createDoubleMovingStdDev(rtOpts->delaySMOutlierFilterCapacity);
                                        strncpy(ptpClock->delaySMRawStats->identifier, "delaySM", 10);
                                        ptpClock->delaySMFiltered = createDoubleMovingMean(rtOpts->delaySMOutlierFilterCapacity);
                                }


                }
#endif /* PTPD_STATISTICS */

#ifdef PTPD_NTPDC

            if(rtOpts->restartSubsystems & PTPD_RESTART_NTPENGINE) {

		ntpShutdown(&rtOpts->ntpOptions, &ptpClock->ntpControl);
		if(rtOpts->ntpOptions.enableEngine) {
                        NOTIFY("Starting NTP control subsystem\n");
		    if(!(ptpClock->ntpControl.operational = 
		        ntpInit(&rtOpts->ntpOptions, &ptpClock->ntpControl))) {

			NOTICE("Could not start NTP control subsystem");
		        }
		} else {
                        NOTIFY("Shutting down NTP control subsystem\n");
		}

	    }

	    if((rtOpts->restartSubsystems & PTPD_RESTART_NTPENGINE) ||
		(rtOpts->restartSubsystems & PTPD_RESTART_NTPCONTROL) ) {
			NOTIFY("Applying NTP control configuration\n");
			/* This will display warnings once again if request keeps failing */
			ptpClock->ntpControl.requestFailed = FALSE;
			Boolean ntpFailover = FALSE;

	    if(rtOpts->ntpOptions.enableEngine && rtOpts->ntpOptions.enableFailover)
	        ntpFailover = timerRunning(NTPD_FAILOVER_TIMER,
				    ptpClock->itimer) ||
			     ptpClock->ntpControl.isFailOver;

	    if(rtOpts->ntpOptions.enableEngine) {
		timerStart(NTPD_CHECK_TIMER,rtOpts->ntpOptions.checkInterval,ptpClock->itimer);

	      /* NTP is the desired clock controller - don't wait or failover, flip now */
                        if(rtOpts->ntpOptions.ntpInControl) {
                                ptpClock->ntpControl.isRequired = rtOpts->ntpOptions.ntpInControl;
                                        if(!ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, FALSE))
                                                DBG("STARTUP - NTP preferred - could not fail over\n");
                        } else if(rtOpts->ntpOptions.enableFailover && ntpFailover) {
                                /* We have a timeout defined */
                                if(rtOpts->ntpOptions.failoverTimeout) {
                                        DBG("NTP failover timer started\n");
                                        timerStart(NTPD_FAILOVER_TIMER,
                                                    rtOpts->ntpOptions.failoverTimeout,
                                                    ptpClock->itimer);
                                /* Fail over to NTP straight away */
                                } else {
                                        DBG("Initiating NTP failover\n");
                                        ptpClock->ntpControl.isRequired = TRUE;
                                        ptpClock->ntpControl.isFailOver = TRUE;
                                        if(!ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, FALSE))
                                                DBG("STARTUP instant NTP failover - could not fail over\n");
                                }
                        } else {
                                ptpClock->ntpControl.isRequired = rtOpts->ntpOptions.ntpInControl;
				ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, TRUE);
			}
	    }


	    }

#endif /* PTPD_NTPDC */

		    /* Update PI servo parameters */
		    setupPIservo(&ptpClock->servo, rtOpts);
		    /* Config changes don't require subsystem restarts - acknowledge it */
		    if(rtOpts->restartSubsystems == PTPD_RESTART_NONE) {
				NOTIFY("Applying configuration\n");
		    }

		    if(rtOpts->restartSubsystems != -1)
			    rtOpts->restartSubsystems = 0;

		}
		/* Perform the heavy signal processing synchronously */
		checkSignals(rtOpts, ptpClock);
	}
}


/* perform actions required when leaving 'port_state' and entering 'state' */
void 
toState(UInteger8 state, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	ptpClock->message_activity = TRUE;
	
	/* leaving state tasks */
	switch (ptpClock->portState)
	{
	case PTP_MASTER:
		timerStop(SYNC_INTERVAL_TIMER, ptpClock->itimer);  
		timerStop(ANNOUNCE_INTERVAL_TIMER, ptpClock->itimer);
		timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer); 
		break;
		
	case PTP_SLAVE:
		timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
		
		if (ptpClock->delayMechanism == E2E)
			timerStop(DELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
		else if (ptpClock->delayMechanism == P2P)
			timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
/* If statistics are enabled, drift should have been saved already - otherwise save it*/
#ifndef PTPD_STATISTICS
#if !defined(__APPLE__)
		/* save observed drift value, don't inform user */
		saveDrift(ptpClock, rtOpts, TRUE);
#endif /* apple */
#endif /* PTPD_STATISTICS */

#ifdef PTPD_STATISTICS
		resetPtpEngineSlaveStats(&ptpClock->slaveStats);
		timerStop(STATISTICS_UPDATE_TIMER, ptpClock->itimer);
#endif /* PTPD_STATISTICS */
		ptpClock->panicMode = FALSE;
		ptpClock->panicOver = FALSE;
		timerStop(PANIC_MODE_TIMER, ptpClock->itimer);
		initClock(rtOpts, ptpClock); 
		break;
		
	case PTP_PASSIVE:
		timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
		timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
		break;
		
	case PTP_LISTENING:
		timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
		break;
		
	default:
		break;
	}
	
	/* entering state tasks */

	ptpClock->counters.stateTransitions++;

	DBG("state %s\n",portState_getName(state));

#ifdef PTPD_NTPDC
	/* 
	 * Keep the NTP control container informed. Depending on config,
	 * we may be going between SLAVE when NTP should not be running
	 * and MASTER when it should
	 */
	if(rtOpts->ntpOptions.enableEngine && !ptpClock->ntpControl.isFailOver) {
		ptpClock->ntpControl.isRequired = 
		rtOpts->ntpOptions.ntpInControl;
		if(!ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, TRUE)) {
			DBG("NTPdControl() fail during state change\n");
		}
	}
#endif /* PTPD_NTPDC */

	/*
	 * No need of PRE_MASTER state because of only ordinary clock
	 * implementation.
	 */
	
	switch (state)
	{
	case PTP_INITIALIZING:
		ptpClock->portState = PTP_INITIALIZING;
		break;
		
	case PTP_FAULTY:
		ptpClock->portState = PTP_FAULTY;
		break;
		
	case PTP_DISABLED:
		ptpClock->portState = PTP_DISABLED;
		break;
		
	case PTP_LISTENING:
		/* in Listening mode, make sure we don't send anything. Instead we just expect/wait for announces (started below) */
		timerStop(SYNC_INTERVAL_TIMER,      ptpClock->itimer);
		timerStop(ANNOUNCE_INTERVAL_TIMER,  ptpClock->itimer);
		timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
		timerStop(DELAYREQ_INTERVAL_TIMER,  ptpClock->itimer);
		
		/*
		 *  Count how many _unique_ timeouts happen to us.
		 *  If we were already in Listen mode, then do not count this as a seperate reset, but stil do a new IGMP refresh
		 */
		if (ptpClock->portState != PTP_LISTENING) {
			ptpClock->resetCount++;
		}

		/* Revert to the original DelayReq interval, and ignore the one for the last master */
		ptpClock->logMinDelayReqInterval = rtOpts->initial_delayreq;

		/* force a IGMP refresh per reset */
		if (rtOpts->ip_mode != IPMODE_UNICAST && rtOpts->do_IGMP_refresh && rtOpts->transport != IEEE_802_3) {
			netRefreshIGMP(&ptpClock->netPath, rtOpts, ptpClock);
		}
		

		timerStart(ANNOUNCE_RECEIPT_TIMER, 
			   (ptpClock->announceReceiptTimeout) * 
			   (pow(2,ptpClock->logAnnounceInterval)), 
			   ptpClock->itimer);
		ptpClock->announceTimeouts = 0;
/* We've entered LISTENING state from SLAVE - start NTP failover actions */
#ifdef PTPD_NTPDC
		if(ptpClock->portState == PTP_SLAVE) {
			/* NTP is the desired clock controller - don't wait or failover, flip now */
			if(rtOpts->ntpOptions.ntpInControl) {
				ptpClock->ntpControl.isRequired = rtOpts->ntpOptions.ntpInControl;
					if(!ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, FALSE))
						DBG("LISTENING - NTP preferred - could not fail over\n");
			} else if(rtOpts->ntpOptions.enableFailover) {
				/* We have a timeout defined */
				if(rtOpts->ntpOptions.failoverTimeout) {
					INFO("NTP failover timer started\n");
					timerStart(NTPD_FAILOVER_TIMER, 
						    rtOpts->ntpOptions.failoverTimeout, 
						    ptpClock->itimer);
				/* Fail over to NTP straight away */
				} else {
					DBG("Initiating NTP failover\n");
					ptpClock->ntpControl.isRequired = TRUE;
					ptpClock->ntpControl.isFailOver = TRUE;
					if(!ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, FALSE))
						DBG("LISTENING instant NTP failover - could not fail over\n");
				}
			}
		}
#endif /* PTPD_NTPDC */

		/* 
		 * Log status update only once - condition must be checked before we write the new state,
		 * but the new state must be eritten before the log message...
		 */
		if (ptpClock->portState != state) {
			ptpClock->portState = PTP_LISTENING;
			displayStatus(ptpClock, "Now in state: ");
		} else {
			ptpClock->portState = PTP_LISTENING;
		}

		break;

	case PTP_MASTER:
		timerStart(SYNC_INTERVAL_TIMER, 
			   pow(2,ptpClock->logSyncInterval), ptpClock->itimer);
		DBG("SYNC INTERVAL TIMER : %f \n",
		    pow(2,ptpClock->logSyncInterval));
		timerStart(ANNOUNCE_INTERVAL_TIMER, 
			   pow(2,ptpClock->logAnnounceInterval), 
			   ptpClock->itimer);
		timerStart(PDELAYREQ_INTERVAL_TIMER, 
			   pow(2,ptpClock->logMinPdelayReqInterval), 
			   ptpClock->itimer);
		ptpClock->portState = PTP_MASTER;
		displayStatus(ptpClock, "Now in state: ");
		break;

	case PTP_PASSIVE:
		timerStart(PDELAYREQ_INTERVAL_TIMER, 
			   pow(2,ptpClock->logMinPdelayReqInterval), 
			   ptpClock->itimer);
		timerStart(ANNOUNCE_RECEIPT_TIMER, 
			   (ptpClock->announceReceiptTimeout) * 
			   (pow(2,ptpClock->logAnnounceInterval)), 
			   ptpClock->itimer);
		ptpClock->portState = PTP_PASSIVE;
		p1(ptpClock, rtOpts);
		displayStatus(ptpClock, "Now in state: ");
		break;

	case PTP_UNCALIBRATED:
		ptpClock->portState = PTP_UNCALIBRATED;
		break;

	case PTP_SLAVE:

#ifdef PTPD_NTPDC
		/* about to go into SLAVE state from another state - make sure we check NTPd clock control (quietly)*/
		if (ptpClock->portState != PTP_SLAVE && rtOpts->ntpOptions.enableEngine) {
/* This does not get us out of panic mode */
if(!rtOpts->panicModeNtp || !ptpClock->panicMode)
 {
			INFO("About to enter SLAVE state - checking local NTPd\n");
			/*
			 * In case if we were in failover mode - disable it,
			 * but if we're controlling the clock, always disable NTPd
			 */
			if(!rtOpts->noAdjust) {
				ptpClock->ntpControl.isRequired = FALSE;
				ptpClock->ntpControl.isFailOver = FALSE;
			}
			ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, TRUE);
			timerStop(NTPD_FAILOVER_TIMER, ptpClock->itimer);
		}

		}
#endif /* PTPD_NTPDC */

		initClock(rtOpts, ptpClock);
#if !defined(__APPLE__)
		/*
		 * restore the observed drift value using the selected method,
		 * reset on failure or when -F 0 (default) is used, don't inform user
		 */
		restoreDrift(ptpClock, rtOpts, TRUE);
#endif /* apple */

		ptpClock->waitingForFollow = FALSE;
		ptpClock->waitingForDelayResp = FALSE;
#ifdef PTPD_STATISTICS
		ptpClock->statsUpdates = 0;
		ptpClock->isCalibrated = FALSE;
#endif /* PTPD_STATISTICS */
		// FIXME: clear these vars inside initclock
		clearTime(&ptpClock->delay_req_send_time);
		clearTime(&ptpClock->delay_req_receive_time);
		clearTime(&ptpClock->pdelay_req_send_time);
		clearTime(&ptpClock->pdelay_req_receive_time);
		clearTime(&ptpClock->pdelay_resp_send_time);
		clearTime(&ptpClock->pdelay_resp_receive_time);
		
		timerStart(OPERATOR_MESSAGES_TIMER,
			   OPERATOR_MESSAGES_INTERVAL,
			   ptpClock->itimer);
		
		timerStart(ANNOUNCE_RECEIPT_TIMER,
			   (ptpClock->announceReceiptTimeout) * 
			   (pow(2,ptpClock->logAnnounceInterval)), 
			   ptpClock->itimer);
		
		/*
		 * Previously, this state transition would start the
		 * delayreq timer immediately.  However, if this was
		 * faster than the first received sync, then the servo
		 * would drop the delayResp Now, we only start the
		 * timer after we receive the first sync (in
		 * handle_sync())
		 */
		ptpClock->waiting_for_first_sync = TRUE;
		ptpClock->waiting_for_first_delayresp = TRUE;
		ptpClock->announceTimeouts = 0;
		ptpClock->portState = PTP_SLAVE;
		displayStatus(ptpClock, "Now in state: ");

#ifdef PTPD_STATISTICS
		resetDoubleMovingStdDev(ptpClock->delayMSRawStats);
		resetDoubleMovingMean(ptpClock->delayMSFiltered);
		resetDoubleMovingStdDev(ptpClock->delaySMRawStats);
		resetDoubleMovingMean(ptpClock->delaySMFiltered);
		clearPtpEngineSlaveStats(&ptpClock->slaveStats);
		ptpClock->delayMSoutlier = FALSE;
		ptpClock->delaySMoutlier = FALSE;
		ptpClock->servo.driftMean = 0;
		ptpClock->servo.driftStdDev = 0;
		ptpClock->servo.isStable = FALSE;
		ptpClock->servo.statsCalculated = FALSE;
		resetDoublePermanentStdDev(&ptpClock->servo.driftStats);
		timerStart(STATISTICS_UPDATE_TIMER, rtOpts->statsUpdateInterval, ptpClock->itimer);
#endif /* PTPD_STATISTICS */

#if !defined(__APPLE__)

		/* 
		 * leap second pending in kernel but no leap second 
		 * info from GM - withdraw kernel leap second
		 * if the flags have disappeared but we're past 
		 * leap second event, do nothing - kernel flags 
		 * will be unset in handleAnnounce()
		 */
		if((!ptpClock->timePropertiesDS.leap59 && !ptpClock->timePropertiesDS.leap61) &&
		    !ptpClock->leapSecondInProgress &&
		   (checkTimexFlags(STA_INS) || checkTimexFlags(STA_DEL))) {
			WARNING("Leap second pending in kernel but not on "
				"GM: aborting kernel leap second\n");
			unsetTimexFlags(STA_INS | STA_DEL, TRUE);
		}
#endif /* apple */
		break;
	default:
		DBG("to unrecognized state\n");
		break;
	}

	if (rtOpts->logStatistics)
		logStatistics(rtOpts, ptpClock);
}


Boolean 
doInit(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBG("manufacturerIdentity: %s\n", MANUFACTURER_ID);
	DBG("manufacturerOUI: %02hhx:%02hhx:%02hhx \n",
		MANUFACTURER_ID_OUI0,
		MANUFACTURER_ID_OUI1,
		MANUFACTURER_ID_OUI2);
	/* initialize networking */
	netShutdown(&ptpClock->netPath);
	if (!netInit(&ptpClock->netPath, rtOpts, ptpClock)) {
		ERROR("failed to initialize network\n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return FALSE;
	}
	

#ifdef PTPD_NTPDC

	ntpShutdown(&rtOpts->ntpOptions, &ptpClock->ntpControl);
	if(rtOpts->ntpOptions.enableEngine) {
		if(!(ptpClock->ntpControl.operational = 
			ntpInit(&rtOpts->ntpOptions, &ptpClock->ntpControl))) {
			NOTICE("Could not start NTP control subsystem");
		}
	}

	if(rtOpts->ntpOptions.enableEngine) {
	    ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, TRUE);
	}

#endif /* PTPD_NTPDC */

	/* initialize other stuff */
	initData(rtOpts, ptpClock);
	initTimer();
	initClock(rtOpts, ptpClock);
	setupPIservo(&ptpClock->servo, rtOpts);
#if !defined(__APPLE__)
	/* restore observed drift and inform user */
	if(ptpClock->clockQuality.clockClass > 127)
		restoreDrift(ptpClock, rtOpts, FALSE);
#endif /* apple */
	m1(rtOpts, ptpClock );
	msgPackHeader(ptpClock->msgObuf, ptpClock);
	
	toState(PTP_LISTENING, rtOpts, ptpClock);

	if(rtOpts->statusLog.logEnabled)
		writeStatusFile(ptpClock, rtOpts, TRUE);


	return TRUE;
}

/* handle actions and events for 'port_state' */
static void 
doState(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	UInteger8 state;
	
	ptpClock->message_activity = FALSE;
	
	/* Process record_update (BMC algorithm) before everything else */
	switch (ptpClock->portState)
	{
	case PTP_LISTENING:
	case PTP_PASSIVE:
	case PTP_SLAVE:
	case PTP_MASTER:
		/*State decision Event*/

		/* If we received a valid Announce message
 		 * and can use it (record_update),
		 * or we received a SET management message that
		 * changed an attribute in ptpClock,
		 * then run the BMC algorithm
		 */ 
		if(ptpClock->record_update)
		{
			DBG2("event STATE_DECISION_EVENT\n");
			ptpClock->record_update = FALSE;
			state = bmc(ptpClock->foreign, rtOpts, ptpClock);
			if(state != ptpClock->portState)
				toState(state, rtOpts, ptpClock);
		}
		break;
		
	default:
		break;
	}
	
	
	switch (ptpClock->portState)
	{
	case PTP_FAULTY:
		/* imaginary troubleshooting */
		DBG("event FAULT_CLEARED\n");
		toState(PTP_INITIALIZING, rtOpts, ptpClock);
		return;
		
	case PTP_LISTENING:
	case PTP_UNCALIBRATED:
	case PTP_SLAVE:
	// passive mode behaves like the SLAVE state, in order to wait for the announce timeout of the current active master
	case PTP_PASSIVE:
		handle(rtOpts, ptpClock);
		
		/*
		 * handle SLAVE timers:
		 *   - No Announce message was received
		 *   - Time to send new delayReq  (miss of delayResp is not monitored explicitelly)
		 */
		if (timerExpired(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer))
		{
			DBG("event ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES\n");

			if(!ptpClock->slaveOnly && 
			   ptpClock->clockQuality.clockClass != SLAVE_ONLY_CLOCK_CLASS) {
				ptpClock->number_foreign_records = 0;
				ptpClock->foreign_record_i = 0;
				m1(rtOpts,ptpClock);
				toState(PTP_MASTER, rtOpts, ptpClock);

			} else if(ptpClock->portState != PTP_LISTENING) {
#ifdef PTPD_STATISTICS
				/* stop statistics updates */
				timerStop(STATISTICS_UPDATE_TIMER, ptpClock->itimer);
#endif /* PTPD_STATISTICS */

				if(ptpClock->announceTimeouts < rtOpts->announceTimeoutGracePeriod) {
				/*
				* Don't reset yet - just disqualify current GM.
				* If another live master exists, it will be selected,
				* otherwise, timer will cycle and we will reset.
				* Also don't clear the FMR just yet.
				*/
				if (ptpClock->grandmasterClockQuality.clockClass != 255 &&
				    ptpClock->grandmasterPriority1 != 255 &&
				    ptpClock->grandmasterPriority2 != 255) {
					ptpClock->grandmasterClockQuality.clockClass = 255;
					ptpClock->grandmasterPriority1 = 255;
					ptpClock->grandmasterPriority2 = 255;
					ptpClock->foreign[ptpClock->foreign_record_best].announce.grandmasterPriority1=255;
					ptpClock->foreign[ptpClock->foreign_record_best].announce.grandmasterPriority2=255;
					ptpClock->foreign[ptpClock->foreign_record_best].announce.grandmasterClockQuality.clockClass=255;
					WARNING("GM announce timeout, disqualified current best GM\n");
					ptpClock->counters.announceTimeouts++;
				}

				if (rtOpts->ip_mode != IPMODE_UNICAST && rtOpts->do_IGMP_refresh && rtOpts->transport != IEEE_802_3) {
					netRefreshIGMP(&ptpClock->netPath, rtOpts, ptpClock);
				}

/*
				timerStart(ANNOUNCE_RECEIPT_TIMER,
					(ptpClock->announceReceiptTimeout) *
					(pow(2,ptpClock->logAnnounceInterval)),
					ptpClock->itimer);
*/
				if (rtOpts->announceTimeoutGracePeriod > 0) ptpClock->announceTimeouts++;

					INFO("Waiting for new master, %d of %d attempts\n",ptpClock->announceTimeouts,rtOpts->announceTimeoutGracePeriod);
				} else {
					WARNING("No active masters present. Resetting port.\n");
					ptpClock->number_foreign_records = 0;
					ptpClock->foreign_record_i = 0;
					toState(PTP_LISTENING, rtOpts, ptpClock);
					}
			} else {
				/*
				 *  Force a reset when getting a timeout in state listening, that will lead to an IGMP reset
				 *  previously this was not the case when we were already in LISTENING mode
				 */
				    toState(PTP_LISTENING, rtOpts, ptpClock);
                                }
                }

#ifdef PTPD_NTPDC
		if(rtOpts->ntpOptions.enableEngine) {
			if( (rtOpts->ntpOptions.enableFailover || (rtOpts->panicModeNtp && ptpClock->panicMode ))&& 
				timerExpired(NTPD_FAILOVER_TIMER, ptpClock->itimer)) {
				NOTICE("NTP failover triggered\n");
				/* make sure we show errors if needed */
				ptpClock->ntpControl.requestFailed = FALSE;
				ptpClock->ntpControl.isRequired = TRUE;
				ptpClock->ntpControl.isFailOver = TRUE;
				if(ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, FALSE)) {
					/* successfully failed over to NTP - stop failover timer */
					timerStop(NTPD_FAILOVER_TIMER, ptpClock->itimer);
				} else {
					DBG("Could not check / request NTP failover\n");
				}
				/* (re)start the NTP check timer, so that you don't check just after a failover attempt */
				timerStart(NTPD_CHECK_TIMER,rtOpts->ntpOptions.checkInterval,ptpClock->itimer);
			}
		}
#endif /* PTPD_NTPDC */

		if (timerExpired(OPERATOR_MESSAGES_TIMER, ptpClock->itimer)) {
			reset_operator_messages(rtOpts, ptpClock);
		}


		if (ptpClock->delayMechanism == E2E) {
			if(timerExpired(DELAYREQ_INTERVAL_TIMER,
					ptpClock->itimer)) {
				DBG2("event DELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				issueDelayReq(rtOpts,ptpClock);
			}
		} else if (ptpClock->delayMechanism == P2P) {
			if (timerExpired(PDELAYREQ_INTERVAL_TIMER,
					ptpClock->itimer)) {
				DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				issuePDelayReq(rtOpts,ptpClock);
			}

			/* FIXME: Path delay should also rearm its timer with the value received from the Master */
		}

                if (ptpClock->timePropertiesDS.leap59 || ptpClock->timePropertiesDS.leap61) 
                        DBGV("seconds to midnight: %.3f\n",secondsToMidnight());

                /* leap second period is over */
                if(timerExpired(LEAP_SECOND_PAUSE_TIMER,ptpClock->itimer) &&
                    ptpClock->leapSecondInProgress) {
                            /* 
                             * do not unpause offset calculation just
                             * yet, just indicate and it will be
                             * unpaused in handleAnnounce()
                            */
                            ptpClock->leapSecondPending = FALSE;
                            timerStop(LEAP_SECOND_PAUSE_TIMER,ptpClock->itimer);
                    } 
		/* check if leap second is near and if we should pause updates */
		if( ptpClock->leapSecondPending &&
		    !ptpClock->leapSecondInProgress &&
		    (secondsToMidnight() <= 
		    getPauseAfterMidnight(ptpClock->logAnnounceInterval))) {
                            WARNING("Leap second event imminent - pausing "
				    "clock and offset updates\n");
                            ptpClock->leapSecondInProgress = TRUE;
#if !defined(__APPLE__)
                            if(!checkTimexFlags(ptpClock->timePropertiesDS.leap61 ? 
						STA_INS : STA_DEL)) {
                                    WARNING("Kernel leap second flags have "
					    "been unset - attempting to set "
					    "again");
                                    setTimexFlags(ptpClock->timePropertiesDS.leap61 ? 
						  STA_INS : STA_DEL, FALSE);
                            }
#endif /* apple */
			    /*
			     * start pause timer from now until [pause] after
			     * midnight, plus an extra second if inserting
			     * a leap second
			     */
			    timerStart(LEAP_SECOND_PAUSE_TIMER,
				       secondsToMidnight() + 
				       (int)ptpClock->timePropertiesDS.leap61 + 
				       getPauseAfterMidnight(ptpClock->logAnnounceInterval),
				       ptpClock->itimer);
		}

/* Update PTP slave statistics from online statistics containers */
#ifdef PTPD_STATISTICS
		if (timerExpired(STATISTICS_UPDATE_TIMER,ptpClock->itimer)) {
			if(!rtOpts->enablePanicMode || !ptpClock->panicMode)
				updatePtpEngineStats(ptpClock, rtOpts);
		}
#endif /* PTPD_STATISTICS */



		break;

	case PTP_MASTER:
		/*
		 * handle SLAVE timers:
		 *   - Time to send new Sync
		 *   - Time to send new Announce
		 *   - Time to send new PathDelay
		 *      (DelayResp has no timer - as these are sent and retransmitted by the slaves)
		 */
	
		if (timerExpired(SYNC_INTERVAL_TIMER, ptpClock->itimer)) {
			DBGV("event SYNC_INTERVAL_TIMEOUT_EXPIRES\n");
			issueSync(rtOpts, ptpClock);
		}
		
		if (timerExpired(ANNOUNCE_INTERVAL_TIMER, ptpClock->itimer)) {
			DBGV("event ANNOUNCE_INTERVAL_TIMEOUT_EXPIRES\n");
			issueAnnounce(rtOpts, ptpClock);
		}
		
		if (ptpClock->delayMechanism == P2P) {
			if (timerExpired(PDELAYREQ_INTERVAL_TIMER,
					ptpClock->itimer)) {
				DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				issuePDelayReq(rtOpts,ptpClock);
			}
		}
		
		// TODO: why is handle() below expiretimer, while in slave is the opposite
		handle(rtOpts, ptpClock);
		
		if (ptpClock->slaveOnly || ptpClock->clockQuality.clockClass == SLAVE_ONLY_CLOCK_CLASS)
			toState(PTP_LISTENING, rtOpts, ptpClock);
		
		break;

	case PTP_DISABLED:
		handle(rtOpts, ptpClock);
		break;
		
	default:
		DBG("(doState) do unrecognized state\n");
		break;
	}

#ifdef PTPD_NTPDC
/* Periodic NTP check */
		if(timerExpired(NTPD_CHECK_TIMER,ptpClock->itimer)) {
			/* check NTP status, make sure it is enabled / disabled based on NTPcontrol->required*/
			if (!ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl,TRUE)) {
				DBG("Could not perform NTP check\n");
			}
			/* Explicitly restart NTP check timer with the current interval value */
			timerStart(NTPD_CHECK_TIMER,rtOpts->ntpOptions.checkInterval,ptpClock->itimer);
		}
#endif /* PTPD_NTPDC */

        if(rtOpts->statusLog.logEnabled && timerExpired(STATUSFILE_UPDATE_TIMER,ptpClock->itimer)) {
                writeStatusFile(ptpClock,rtOpts,TRUE);
		/* ensures that the current updare interval is used */
		timerStart(STATUSFILE_UPDATE_TIMER,rtOpts->statusFileUpdateInterval,ptpClock->itimer);
        }

	if(rtOpts->enablePanicMode && timerExpired(PANIC_MODE_TIMER, ptpClock->itimer)) {

		DBG("Panic check\n");

		if(--ptpClock->panicModeTimeLeft <= 0) {
			ptpClock->panicMode = FALSE;
			ptpClock->panicOver = TRUE;
			DBG("panic over!\n");
		}
	}

}

static Boolean
isFromCurrentParent(const PtpClock *ptpClock, const MsgHeader* header)
{

	return(!memcmp(
		ptpClock->parentPortIdentity.clockIdentity,
		header->sourcePortIdentity.clockIdentity,
		CLOCK_IDENTITY_LENGTH)	&& 
		(ptpClock->parentPortIdentity.portNumber ==
		 header->sourcePortIdentity.portNumber));

}

void
processMessage(RunTimeOpts* rtOpts, PtpClock* ptpClock, TimeInternal* timeStamp, ssize_t length)
{

    Boolean isFromSelf;

    /*
     * make sure we use the TAI to UTC offset specified, if the
     * master is sending the UTC_VALID bit
     *
     * On the slave, all timestamps that we handle here have been
     * collected by our local clock (loopback+kernel-level
     * timestamp) This includes delayReq just send, and delayResp,
     * when it arrives.
     *
     * these are then adjusted to the same timebase of the Master
     * (+35 leap seconds, as of July 2012)
     *
     * wowczarek: added compatibility flag to always respect the
     * announced UTC offset, preventing clock jumps with some GMs
     */
    DBGV("__UTC_offset: %d %d \n", ptpClock->timePropertiesDS.currentUtcOffsetValid, ptpClock->timePropertiesDS.currentUtcOffset);
    if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
	timeStamp->seconds += ptpClock->timePropertiesDS.currentUtcOffset;
    }

    ptpClock->message_activity = TRUE;
    if (length < HEADER_LENGTH) {
	DBG("Error: message shorter than header length\n");
	ptpClock->counters.messageFormatErrors++;
	return;
    }
    msgUnpackHeader(ptpClock->msgIbuf, &ptpClock->msgTmpHeader);
    /* packet is not from self, and is from a non-zero source address - check ACLs */
    if(ptpClock->netPath.lastRecvAddr && 
	(ptpClock->netPath.lastRecvAddr != ptpClock->netPath.interfaceAddr.s_addr)) {
		struct in_addr in;
		in.s_addr=ptpClock->netPath.lastRecvAddr;
		if(ptpClock->msgTmpHeader.messageType == MANAGEMENT) {
			if(rtOpts->timingAclEnabled && 
				!matchIpv4AccessList(ptpClock->netPath.managementAcl, ntohl(ptpClock->netPath.lastRecvAddr))) {
					DBG("ACL dropped management message from %s\n", inet_ntoa(in));
					ptpClock->counters.aclTimingDiscardedMessages++;
					return;
			} else
					DBG("ACL Accepted management message from %s\n", inet_ntoa(in));
	        } else if(rtOpts->timingAclEnabled && 
		    !matchIpv4AccessList(ptpClock->netPath.timingAcl, ntohl(ptpClock->netPath.lastRecvAddr))) {
					DBG("ACL dropped timing message from %s\n", inet_ntoa(in));
					ptpClock->counters.aclManagementDiscardedMessages++;
					return;
		} else
					DBG("ACL accepted timing message from %s\n", inet_ntoa(in));
    }

    if (ptpClock->msgTmpHeader.versionPTP != ptpClock->versionNumber) {
	DBG("ignore version %d message\n", ptpClock->msgTmpHeader.versionPTP);
	ptpClock->counters.discardedMessages++;
	ptpClock->counters.versionMismatchErrors++;
	return;
    }

    if(ptpClock->msgTmpHeader.domainNumber != ptpClock->domainNumber) {
	DBG("ignore message from domainNumber %d\n", ptpClock->msgTmpHeader.domainNumber);
	ptpClock->counters.discardedMessages++;
	ptpClock->counters.domainMismatchErrors++;
	return;
    }

    /*Spec 9.5.2.2*/
    isFromSelf = (ptpClock->portIdentity.portNumber == ptpClock->msgTmpHeader.sourcePortIdentity.portNumber
	      && !memcmp(ptpClock->msgTmpHeader.sourcePortIdentity.clockIdentity, ptpClock->portIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH));

    /*
     * subtract the inbound latency adjustment if it is not a loop
     *  back and the time stamp seems reasonable 
     */
    if (!isFromSelf && timeStamp->seconds > 0)
	subTime(timeStamp, timeStamp, &rtOpts->inboundLatency);


#ifdef PTPD_DBG
    /* easy display of received messages */
    char *st;

    switch(ptpClock->msgTmpHeader.messageType)
    {
    case ANNOUNCE:
	st = "Announce";
	break;
    case SYNC:
	st = "Sync";
	break;
    case FOLLOW_UP:
	st = "FollowUp";
	break;
    case DELAY_REQ:
	st = "DelayReq";
	break;
    case DELAY_RESP:
	st = "DelayResp";
	break;
    case PDELAY_REQ:
	st = "PDelayReq";
	break;
    case PDELAY_RESP:
	st = "PDelayResp";
	break;
    case PDELAY_RESP_FOLLOW_UP:
	st = "PDelayRespFollowUp";
	break;	
    case MANAGEMENT:
	st = "Management";
	break;
    default:
	st = "Unk";
	break;
    }
    DBG("      ==> %s received\n", st);
#endif

    /*
     *  on the table below, note that only the event messsages are passed the local time,
     *  (collected by us by loopback+kernel TS, and adjusted with UTC seconds
     *
     *  (SYNC / DELAY_REQ / PDELAY_REQ / PDELAY_RESP)
     */
    switch(ptpClock->msgTmpHeader.messageType)
    {
    case ANNOUNCE:
	handleAnnounce(&ptpClock->msgTmpHeader,
	           length, isFromSelf, rtOpts, ptpClock);
	break;
    case SYNC:
	handleSync(&ptpClock->msgTmpHeader, 
	       length, timeStamp, isFromSelf, rtOpts, ptpClock);
	break;
    case FOLLOW_UP:
	handleFollowUp(&ptpClock->msgTmpHeader,
	           length, isFromSelf, rtOpts, ptpClock);
	break;
    case DELAY_REQ:
	handleDelayReq(&ptpClock->msgTmpHeader,
	           length, timeStamp, isFromSelf, rtOpts, ptpClock);
	break;
    case PDELAY_REQ:
	handlePDelayReq(&ptpClock->msgTmpHeader,
		length, timeStamp, isFromSelf, rtOpts, ptpClock);
	break;  
    case DELAY_RESP:
	handleDelayResp(&ptpClock->msgTmpHeader,
		length, rtOpts, ptpClock);
	break;
    case PDELAY_RESP:
	handlePDelayResp(&ptpClock->msgTmpHeader,
		 timeStamp, length, isFromSelf, rtOpts, ptpClock);
	break;
    case PDELAY_RESP_FOLLOW_UP:
	handlePDelayRespFollowUp(&ptpClock->msgTmpHeader, 
		     length, isFromSelf, rtOpts, ptpClock);
	break;
    case MANAGEMENT:
	handleManagement(&ptpClock->msgTmpHeader,
		 isFromSelf, rtOpts, ptpClock);
	break;
    case SIGNALING:
	handleSignaling(ptpClock);
	break;
    default:
	DBG("handle: unrecognized message\n");
	ptpClock->counters.discardedMessages++;
	ptpClock->counters.unknownMessages++;
	break;
    }

    if (rtOpts->displayPackets)
	msgDump(ptpClock);


}


/* check and handle received messages */
void
handle(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    int ret;
    ssize_t length = -1;

    TimeInternal timeStamp = { 0, 0 };
    fd_set readfds;

    FD_ZERO(&readfds);
    if (!ptpClock->message_activity) {
	ret = netSelect(NULL, &ptpClock->netPath, &readfds);
	if (ret < 0) {
	    PERROR("failed to poll sockets");
	    ptpClock->counters.messageRecvErrors++;
	    toState(PTP_FAULTY, rtOpts, ptpClock);
	    return;
	} else if (!ret) {
	    /* DBGV("handle: nothing\n"); */
	    return;
	}
	/* else length > 0 */
    }

    DBGV("handle: something\n");

    if (rtOpts->pcap == TRUE) {
	if (ptpClock->netPath.pcapEventSock >=0 && FD_ISSET(ptpClock->netPath.pcapEventSock, &readfds)) {
	    length = netRecvEvent(ptpClock->msgIbuf, &timeStamp, 
		          &ptpClock->netPath,0);
	    if (length == 0) /* timeout, return for now */
		return;
	    if (length < 0) {
		PERROR("failed to receive event on pcap");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		ptpClock->counters.messageRecvErrors++;
		return;
	    }
	    processMessage(rtOpts, ptpClock, &timeStamp, length);
	}
	if (ptpClock->netPath.pcapGeneralSock >=0 && FD_ISSET(ptpClock->netPath.pcapGeneralSock, &readfds)) {
	    length = netRecvGeneral(ptpClock->msgIbuf, &ptpClock->netPath);
	    if (length == 0) /* timeout, return for now */
		return;
	    if (length < 0) {
		PERROR("failed to receive general on pcap");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		ptpClock->counters.messageRecvErrors++;
		return;
	    }
	    processMessage(rtOpts, ptpClock, &timeStamp, length);
	}
    } else {
	if (FD_ISSET(ptpClock->netPath.eventSock, &readfds)) {
	    length = netRecvEvent(ptpClock->msgIbuf, &timeStamp, 
		          &ptpClock->netPath, 0);
	    if (length < 0) {
		PERROR("failed to receive on the event socket");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		ptpClock->counters.messageRecvErrors++;
		return;
	    }
	    processMessage(rtOpts, ptpClock, &timeStamp, length);
	}

	if (FD_ISSET(ptpClock->netPath.generalSock, &readfds)) {
	    length = netRecvGeneral(ptpClock->msgIbuf, &ptpClock->netPath);
	    if (length < 0) {
		PERROR("failed to receive on the general socket");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		ptpClock->counters.messageRecvErrors++;
		return;
	    }
	    processMessage(rtOpts, ptpClock, &timeStamp, length);
	}
    }

}

/*spec 9.5.3*/
static void 
handleAnnounce(MsgHeader *header, ssize_t length, 
	       Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

	DBGV("HandleAnnounce : Announce message received : \n");

	if(length < ANNOUNCE_LENGTH) {
		DBG("Error: Announce message too short\n");
		ptpClock->counters.messageFormatErrors++;
		return;
	}

	//DBGV("  >> HandleAnnounce : %d  \n", ptpClock->portState);

	switch (ptpClock->portState) {
	case PTP_INITIALIZING:
	case PTP_FAULTY:
	case PTP_DISABLED:
		DBG("Handleannounce : disregard\n");
		ptpClock->counters.discardedMessages++;
		return;
		
	case PTP_UNCALIBRATED:	
	case PTP_SLAVE:
		if (isFromSelf) {
			DBGV("HandleAnnounce : Ignore message from self \n");
			return;
		}
		if(rtOpts->requireUtcValid && !IS_SET(header->flagField1, UTCV)) {
			ptpClock->counters.ignoredAnnounce++;
			return;
		}
		
		/*
		 * Valid announce message is received : BMC algorithm
		 * will be executed 
		 */
		ptpClock->counters.announceMessagesReceived++;
		ptpClock->record_update = TRUE;

		switch (isFromCurrentParent(ptpClock, header)) {
		case TRUE:
	   		msgUnpackAnnounce(ptpClock->msgIbuf,
					  &ptpClock->msgTmp.announce);

			/* update datasets (file bmc.c) */
	   		s1(header,&ptpClock->msgTmp.announce,ptpClock, rtOpts);

			/* update current master in the fmr as well */
			memcpy(&ptpClock->foreign[ptpClock->foreign_record_best].header,
			       header,sizeof(MsgHeader));
			memcpy(&ptpClock->foreign[ptpClock->foreign_record_best].announce,
			       &ptpClock->msgTmp.announce,sizeof(MsgAnnounce));

			if(ptpClock->leapSecondInProgress) {
				/*
				 * if leap second period is over
				 * (pending == FALSE, inProgress ==
				 * TRUE), unpause offset calculation
				 * (received first announce after leap
				 * second)
				*/
				if (!ptpClock->leapSecondPending) {
					WARNING("Leap second event over - "
						"resuming clock and offset updates\n");
					ptpClock->leapSecondInProgress=FALSE;
					ptpClock->timePropertiesDS.leap59 = FALSE;
					ptpClock->timePropertiesDS.leap61 = FALSE;
#if !defined(__APPLE__)
					unsetTimexFlags(STA_INS | STA_DEL, TRUE);
#endif /* apple */
				}
			}
			DBG2("___ Announce: received Announce from current Master, so reset the Announce timer\n");
	   		/*Reset Timer handling Announce receipt timeout*/
	   		timerStart(ANNOUNCE_RECEIPT_TIMER,
				   (ptpClock->announceReceiptTimeout) * 
				   (pow(2,ptpClock->logAnnounceInterval)), 
				   ptpClock->itimer);

#ifdef PTPD_STATISTICS
		if(timerStopped(STATISTICS_UPDATE_TIMER, ptpClock->itimer)) {
			timerStart(STATISTICS_UPDATE_TIMER, rtOpts->statsUpdateInterval, ptpClock->itimer);
		}
#endif /* PTPD_STATISTICS */

			if (rtOpts->announceTimeoutGracePeriod &&
				ptpClock->announceTimeouts > 0) {
					NOTICE("Received Announce message from master - cancelling timeout\n");
					ptpClock->announceTimeouts = 0;
#ifdef PTPD_NTPDC
			/* Cancel NTP failover if in place */
			if (rtOpts->ntpOptions.enableEngine) {
				INFO("About to enter SLAVE state - checking local NTPd\n");
				/* In case if we were in failover mode - disable it */
				ptpClock->ntpControl.isRequired = FALSE;
				if(!ntpdControl(&rtOpts->ntpOptions, &ptpClock->ntpControl, TRUE)) {
					DBG("NTPDcontrol() failed during toState(SLAVE)\n");
				}
				timerStop(NTPD_FAILOVER_TIMER, ptpClock->itimer);
			}
#endif /* PTPD_NTPDC */

			}

			// remember IP address of our master for hybrid mode
			// todo: add this to bmc(), to cover the very first packet
			ptpClock->masterAddr = ptpClock->netPath.lastRecvAddr;

			break;

		case FALSE:
			/* addForeign takes care of AnnounceUnpacking */

			/* the actual decision to change masters is
			 * only done in doState() / record_update ==
			 * TRUE / bmc()
			 */

			/*
			 * wowczarek: do not restart timer here:
			 * the slave will  sit idle if current parent
			 * is not announcing, but another GM is
			 */
			addForeign(ptpClock->msgIbuf,header,ptpClock);
			break;

		default:
			DBG("HandleAnnounce : (isFromCurrentParent)"
			     "strange value ! \n");
	   		return;

		} /* switch on (isFromCurrentParrent) */
		break;

	/*
	 * Passive case: previously, this was handled in the default, just like the master case.
	 * This the announce would call addForeign(), but NOT reset the timer, so after 12s it would expire and we would come alive periodically 
	 *
	 * This code is now merged with the slave case to reset the timer, and call addForeign() if it's a third master
	 *
	 */
	case PTP_PASSIVE:
		if (isFromSelf) {
			DBGV("HandleAnnounce : Ignore message from self \n");
			return;
		}
		if(rtOpts->requireUtcValid && !IS_SET(header->flagField1, UTCV)) {
			ptpClock->counters.ignoredAnnounce++;
			return;
		}
		/*
		 * Valid announce message is received : BMC algorithm
		 * will be executed
		 */
		ptpClock->counters.announceMessagesReceived++;
		ptpClock->record_update = TRUE;

		if (isFromCurrentParent(ptpClock, header)) {
			msgUnpackAnnounce(ptpClock->msgIbuf,
					  &ptpClock->msgTmp.announce);

			/* TODO: not in spec
			 * datasets should not be updated by another master
			 * this is the reason why we are PASSIVE and not SLAVE
			 * this should be p1(ptpClock, rtOpts);
			 */
			/* update datasets (file bmc.c) */
			s1(header,&ptpClock->msgTmp.announce,ptpClock, rtOpts);

			ptpClock->masterAddr = ptpClock->netPath.lastRecvAddr;

			DBG("___ Announce: received Announce from current Master, so reset the Announce timer\n\n");
			/*Reset Timer handling Announce receipt timeout*/
			timerStart(ANNOUNCE_RECEIPT_TIMER,
				   (ptpClock->announceReceiptTimeout) *
				   (pow(2,ptpClock->logAnnounceInterval)),
				   ptpClock->itimer);
		} else {
			/*addForeign takes care of AnnounceUnpacking*/
			/* the actual decision to change masters is only done in  doState() / record_update == TRUE / bmc() */
			/* the original code always called: addforeign(new master) + timerstart(announce) */

			DBG("___ Announce: received Announce from another master, will add to the list, as it might be better\n\n");
			DBGV("this is to be decided immediatly by bmc())\n\n");
			addForeign(ptpClock->msgIbuf,header,ptpClock);
		}
		break;

		
	case PTP_MASTER:
	case PTP_LISTENING:           /* listening mode still causes timeouts in order to send IGMP refreshes */
	default :
		if (isFromSelf) {
			DBGV("HandleAnnounce : Ignore message from self \n");
			return;
		}
		if(rtOpts->requireUtcValid && !IS_SET(header->flagField1, UTCV)) {
			ptpClock->counters.ignoredAnnounce++;
			return;
		}
		ptpClock->counters.announceMessagesReceived++;
		DBGV("Announce message from another foreign master\n");
		addForeign(ptpClock->msgIbuf,header,ptpClock);
		ptpClock->record_update = TRUE;    /* run BMC() as soon as possible */
		break;

	} /* switch on (port_state) */
}


static void 
handleSync(const MsgHeader *header, ssize_t length, 
	   TimeInternal *tint, Boolean isFromSelf, 
	   RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	TimeInternal OriginTimestamp;
	TimeInternal correctionField;

	DBGV("Sync message received : \n");

	if (length < SYNC_LENGTH) {
		DBG("Error: Sync message too short\n");
		ptpClock->counters.messageFormatErrors++;
		return;
	}

	switch (ptpClock->portState) {
	case PTP_INITIALIZING:
	case PTP_FAULTY:
	case PTP_DISABLED:
		DBGV("HandleSync : disregard\n");
		ptpClock->counters.discardedMessages++;
		return;
		
	case PTP_UNCALIBRATED:
	case PTP_SLAVE:
		if (isFromSelf) {
			DBGV("HandleSync: Ignore message from self \n");
			return;
		}

		if (isFromCurrentParent(ptpClock, header)) {
			ptpClock->counters.syncMessagesReceived++;
			/* We only start our own delayReq timer after receiving the first sync */
			if (ptpClock->waiting_for_first_sync) {
				ptpClock->waiting_for_first_sync = FALSE;
				NOTICE("Received first Sync from Master\n");

				if (ptpClock->delayMechanism == E2E)
					timerStart(DELAYREQ_INTERVAL_TIMER,
						   pow(2,ptpClock->logMinDelayReqInterval),
						   ptpClock->itimer);
				else if (ptpClock->delayMechanism == P2P)
					timerStart(PDELAYREQ_INTERVAL_TIMER,
						   pow(2,ptpClock->logMinPdelayReqInterval),
						   ptpClock->itimer);
			}

			ptpClock->logSyncInterval = header->logMessageInterval;

			ptpClock->sync_receive_time.seconds = tint->seconds;
			ptpClock->sync_receive_time.nanoseconds = tint->nanoseconds;

			recordSync(rtOpts, header->sequenceId, tint);

			if ((header->flagField0 & PTP_TWO_STEP) == PTP_TWO_STEP) {
				DBG2("HandleSync: waiting for follow-up \n");
				ptpClock->twoStepFlag=TRUE;
				ptpClock->waitingForFollow = TRUE;
				ptpClock->recvSyncSequenceId = 
					header->sequenceId;
				/*Save correctionField of Sync message*/
				integer64_to_internalTime(
					header->correctionField,
					&correctionField);
				ptpClock->lastSyncCorrectionField.seconds = 
					correctionField.seconds;
				ptpClock->lastSyncCorrectionField.nanoseconds =
					correctionField.nanoseconds;
				break;
			} else {
				msgUnpackSync(ptpClock->msgIbuf,
					      &ptpClock->msgTmp.sync);
				integer64_to_internalTime(
					ptpClock->msgTmpHeader.correctionField,
					&correctionField);
				timeInternal_display(&correctionField);
				ptpClock->waitingForFollow = FALSE;
				toInternalTime(&OriginTimestamp,
					       &ptpClock->msgTmp.sync.originTimestamp);
				updateOffset(&OriginTimestamp,
					     &ptpClock->sync_receive_time,
					     &ptpClock->ofm_filt,rtOpts,
					     ptpClock,&correctionField);
				updateClock(rtOpts,ptpClock);
				ptpClock->twoStepFlag=FALSE;
				break;
			}
		} else {
			DBG("HandleSync: Sync message received from "
			     "another Master not our own \n");
			ptpClock->counters.discardedMessages++;
		}
		break;

	case PTP_MASTER:
	default :
		if (!isFromSelf) {
			DBGV("HandleSync: Sync message received from "
			     "another Master  \n");
			/* we are the master, but another is sending */
			ptpClock->counters.discardedMessages++;
			break;
		} if (ptpClock->twoStepFlag) {
			DBGV("HandleSync: going to send followup message\n ");
			processSyncFromSelf(tint, rtOpts, ptpClock, header->sequenceId);
			break;
		} else {
			DBGV("HandleSync: Sync message received from self\n ");
		}
	}
}

static void
processSyncFromSelf(const TimeInternal * tint, RunTimeOpts * rtOpts, PtpClock * ptpClock, const UInteger16 sequenceId) {
	TimeInternal timestamp;
	/*Add latency*/
	addTime(&timestamp, tint, &rtOpts->outboundLatency);
	/* Issue follow-up CORRESPONDING TO THIS SYNC */
	issueFollowup(&timestamp, rtOpts, ptpClock, sequenceId);
}


static void 
handleFollowUp(const MsgHeader *header, ssize_t length, 
	       Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBGV("Handlefollowup : Follow up message received \n");

	TimeInternal preciseOriginTimestamp;
	TimeInternal correctionField;

	if (length < FOLLOW_UP_LENGTH)
	{
		DBG("Error: Follow up message too short\n");
		ptpClock->counters.messageFormatErrors++;
		return;
	}

	if (isFromSelf)
	{
		DBGV("Handlefollowup : Ignore message from self \n");
		return;
	}

	switch (ptpClock->portState)
	{
	case PTP_INITIALIZING:
	case PTP_FAULTY:
	case PTP_DISABLED:
	case PTP_LISTENING:
		DBGV("Handfollowup : disregard\n");
		ptpClock->counters.discardedMessages++;
		return;
		
	case PTP_UNCALIBRATED:	
	case PTP_SLAVE:
		if (isFromCurrentParent(ptpClock, header)) {
			ptpClock->counters.followUpMessagesReceived++;
			ptpClock->logSyncInterval = header->logMessageInterval;
			if (ptpClock->waitingForFollow)	{
				if (ptpClock->recvSyncSequenceId == 
				     header->sequenceId) {
					msgUnpackFollowUp(ptpClock->msgIbuf,
							  &ptpClock->msgTmp.follow);
					ptpClock->waitingForFollow = FALSE;
					toInternalTime(&preciseOriginTimestamp,
						       &ptpClock->msgTmp.follow.preciseOriginTimestamp);
					integer64_to_internalTime(ptpClock->msgTmpHeader.correctionField,
								  &correctionField);
					addTime(&correctionField,&correctionField,
						&ptpClock->lastSyncCorrectionField);

					/*
					send_time = preciseOriginTimestamp (received inside followup)
					recv_time = sync_receive_time (received as CMSG in handleEvent)
					*/
					updateOffset(&preciseOriginTimestamp,
						     &ptpClock->sync_receive_time,&ptpClock->ofm_filt,
						     rtOpts,ptpClock,
						     &correctionField);
					updateClock(rtOpts,ptpClock);
					break;
				} else
					INFO("Ignored followup, SequenceID doesn't match with "
					     "last Sync message \n");
			} else {
				DBG2("Ignored followup, Slave was not waiting a follow up "
				     "message \n");
				ptpClock->counters.discardedMessages++;
				}
		} else {
			DBG2("Ignored, Follow up message is not from current parent \n");
			ptpClock->counters.discardedMessages++;
			}

	case PTP_MASTER:
	case PTP_PASSIVE:
		if(isFromCurrentParent(ptpClock, header))
			DBGV("Ignored, Follow up message received from current master \n");
		else {
		/* follow-ups and syncs are expected to be seen from parent, but not from others */
			DBGV("Follow up message received from foreign master!\n");
			ptpClock->counters.discardedMessages++;
		}
		break;

	default:
    		DBG("do unrecognized state1\n");
    		ptpClock->counters.discardedMessages++;
    		break;
	} /* Switch on (port_state) */

}

void
handleDelayReq(const MsgHeader *header, ssize_t length, 
	       const TimeInternal *tint, Boolean isFromSelf,
	       RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

	if (ptpClock->delayMechanism == E2E) {
		DBGV("delayReq message received : \n");
		
		if (length < DELAY_REQ_LENGTH) {
			DBG("Error: DelayReq message too short\n");
			ptpClock->counters.messageFormatErrors++;
			return;
		}

		switch (ptpClock->portState) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
		case PTP_PASSIVE:
			DBGV("HandledelayReq : disregard\n");
			ptpClock->counters.discardedMessages++;
			return;

		case PTP_SLAVE:
			if (isFromSelf)	{
				DBG("==> Handle DelayReq (%d)\n",
					 header->sequenceId);
				if ( ((UInteger16)(header->sequenceId + 1)) !=
					ptpClock->sentDelayReqSequenceId) {
					DBG("HandledelayReq : sequence mismatch - "
					    "last DelayReq sent: %d, received: %d\n",
					    ptpClock->sentDelayReqSequenceId,
					    header->sequenceId
					    );
					ptpClock->counters.discardedMessages++;
					ptpClock->counters.sequenceMismatchErrors++;
					break;
				}

				/*
				 * Get sending timestamp from IP stack
				 * with SO_TIMESTAMP
				 */

				/*
				 *  Make sure we process the REQ
				 *  _before_ the RESP. While we could
				 *  do this by any order, (because
				 *  it's implicitly indexed by
				 *  (ptpClock->sentDelayReqSequenceId
				 *  - 1), this is now made explicit
				 */
	
				processDelayReqFromSelf(tint, rtOpts, ptpClock);

				break;
			} else {
				DBG2("HandledelayReq : disregard delayreq from other client\n");
				ptpClock->counters.discardedMessages++;
			}
			break;

		case PTP_MASTER:
			msgUnpackHeader(ptpClock->msgIbuf,
					&ptpClock->delayReqHeader);
			ptpClock->counters.delayReqMessagesReceived++;

			// remember IP address of this client for hybrid mode
			ptpClock->LastSlaveAddr = ptpClock->netPath.lastRecvAddr;

					
			issueDelayResp(tint,&ptpClock->delayReqHeader,
				       rtOpts,ptpClock);
			break;

		default:
			DBG("do unrecognized state2\n");
			ptpClock->counters.discardedMessages++;
			break;
		}
	} else if (ptpClock->delayMechanism == P2P) {/* (Peer to Peer mode) */
		DBG("Delay messages are ignored in Peer to Peer mode\n");
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.delayModeMismatchErrors++;
	/* no delay mechanism */
	} else {
		DBG("DelayReq ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}


static void
processDelayReqFromSelf(const TimeInternal * tint, RunTimeOpts * rtOpts, PtpClock * ptpClock) {
	ptpClock->waitingForDelayResp = TRUE;

	ptpClock->delay_req_send_time.seconds = tint->seconds;
	ptpClock->delay_req_send_time.nanoseconds = tint->nanoseconds;

	/*Add latency*/
	addTime(&ptpClock->delay_req_send_time,
		&ptpClock->delay_req_send_time,
		&rtOpts->outboundLatency);
	
	DBGV("processDelayReqFromSelf: %s %d\n",
	    dump_TimeInternal(&ptpClock->delay_req_send_time),
	    rtOpts->outboundLatency);
	
}

static void
handleDelayResp(const MsgHeader *header, ssize_t length,
		RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	if (ptpClock->delayMechanism == E2E) {
		TimeInternal requestReceiptTimestamp;
		TimeInternal correctionField;

		DBGV("delayResp message received : \n");

		if(length < DELAY_RESP_LENGTH) {
			DBG("Error: DelayResp message too short\n");
			ptpClock->counters.messageFormatErrors++;
			return;
		}

		switch(ptpClock->portState) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
			DBGV("HandledelayResp : disregard\n");
			ptpClock->counters.discardedMessages++;
			return;

		case PTP_SLAVE:
			msgUnpackDelayResp(ptpClock->msgIbuf,
					   &ptpClock->msgTmp.resp);

			if ((memcmp(ptpClock->portIdentity.clockIdentity,
				    ptpClock->msgTmp.resp.requestingPortIdentity.clockIdentity,
				    CLOCK_IDENTITY_LENGTH) == 0) &&
			    (ptpClock->portIdentity.portNumber == 
			     ptpClock->msgTmp.resp.requestingPortIdentity.portNumber)
			    && isFromCurrentParent(ptpClock, header)) {
				DBG("==> Handle DelayResp (%d)\n",
					 header->sequenceId);

				if (!ptpClock->waitingForDelayResp) {
					DBGV("Ignored DelayResp - wasn't waiting for one\n");
					ptpClock->counters.discardedMessages++;
					break;
				}

				if (ptpClock->sentDelayReqSequenceId !=
				((UInteger16)(header->sequenceId + 1))) {
					DBG("HandledelayResp : sequence mismatch - "
					    "last DelayReq sent: %d, delayResp received: %d\n",
					    ptpClock->sentDelayReqSequenceId,
					    header->sequenceId
					    );
					ptpClock->counters.discardedMessages++;
					ptpClock->counters.sequenceMismatchErrors++;
					break;
				}

				ptpClock->counters.delayRespMessagesReceived++;
				ptpClock->waitingForDelayResp = FALSE;

				toInternalTime(&requestReceiptTimestamp,
					       &ptpClock->msgTmp.resp.receiveTimestamp);
				ptpClock->delay_req_receive_time.seconds = 
					requestReceiptTimestamp.seconds;
				ptpClock->delay_req_receive_time.nanoseconds = 
					requestReceiptTimestamp.nanoseconds;

				integer64_to_internalTime(
					header->correctionField,
					&correctionField);
				/*
					send_time = delay_req_send_time (received as CMSG in handleEvent)
					recv_time = requestReceiptTimestamp (received inside delayResp)
				*/

				updateDelay(&ptpClock->owd_filt,
					    rtOpts,ptpClock, &correctionField);
				if (ptpClock->waiting_for_first_delayresp) {
					ptpClock->waiting_for_first_delayresp = FALSE;
					NOTICE("Received first Delay Response from Master\n");
				}

				if (rtOpts->ignore_delayreq_interval_master == 0) {
					DBGV("current delay_req: %d  new delay req: %d \n",
						ptpClock->logMinDelayReqInterval,
						header->logMessageInterval);

					/* Accept new DelayReq value from the Master */
					if (ptpClock->logMinDelayReqInterval != header->logMessageInterval) {
						NOTICE("Received new Delay Request interval %d from Master (was: %d)\n",
							 header->logMessageInterval, ptpClock->logMinDelayReqInterval );
					}

					// collect new value indicated from the Master
					ptpClock->logMinDelayReqInterval = header->logMessageInterval;
					
					/* FIXME: the actual rearming of this timer with the new value only happens later in doState()/issueDelayReq() */
				} else {
					if (ptpClock->logMinDelayReqInterval != rtOpts->subsequent_delayreq) {
						INFO("New Delay Request interval applied: %d (was: %d)\n",
							rtOpts->subsequent_delayreq, ptpClock->logMinDelayReqInterval);
					}
					ptpClock->logMinDelayReqInterval = rtOpts->subsequent_delayreq;
				}
			} else {
				DBG("HandledelayResp : delayResp doesn't match with the delayReq. \n");
				ptpClock->counters.discardedMessages++;
				break;
			}
		}
	} else if (ptpClock->delayMechanism == P2P) { /* (Peer to Peer mode) */
		DBG("Delay messages are disregarded in Peer to Peer mode \n");
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.delayModeMismatchErrors++;
	/* no delay mechanism */
	} else {
		DBG("DelayResp ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}

}


static void
handlePDelayReq(MsgHeader *header, ssize_t length, 
		const TimeInternal *tint, Boolean isFromSelf, 
		RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	if (ptpClock->delayMechanism == P2P) {
		DBGV("PdelayReq message received : \n");

		if(length < PDELAY_REQ_LENGTH) {
			DBG("Error: PDelayReq message too short\n");
			ptpClock->counters.messageFormatErrors++;
			return;
		}

		switch (ptpClock->portState ) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
			DBGV("HandlePdelayReq : disregard\n");
			ptpClock->counters.discardedMessages++;
			return;

		case PTP_SLAVE:
		case PTP_MASTER:
		case PTP_PASSIVE:

			if (isFromSelf) {
				processPDelayReqFromSelf(tint, rtOpts, ptpClock);
				break;
			} else {
				ptpClock->counters.pdelayReqMessagesReceived++;
				msgUnpackHeader(ptpClock->msgIbuf,
						&ptpClock->PdelayReqHeader);
				issuePDelayResp(tint, header, rtOpts, 
						ptpClock);	
				break;
			}
		default:
			DBG("do unrecognized state3\n");
			ptpClock->counters.discardedMessages++;
			break;
		}
	} else if (ptpClock->delayMechanism == E2E){/* (End to End mode..) */
		DBG("Peer Delay messages are disregarded in End to End "
		      "mode \n");
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.delayModeMismatchErrors++;
	/* no delay mechanism */
	} else {
		DBG("PDelayReq ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}

static void
processPDelayReqFromSelf(const TimeInternal * tint, RunTimeOpts * rtOpts, PtpClock * ptpClock) {
	/*
	 * Get sending timestamp from IP stack
	 * with SO_TIMESTAMP
	 */
	ptpClock->pdelay_req_send_time.seconds = tint->seconds;
	ptpClock->pdelay_req_send_time.nanoseconds = tint->nanoseconds;
			
	/*Add latency*/
	addTime(&ptpClock->pdelay_req_send_time,
		&ptpClock->pdelay_req_send_time,
		&rtOpts->outboundLatency);
}

static void
handlePDelayResp(const MsgHeader *header, TimeInternal *tint,
		 ssize_t length, Boolean isFromSelf, 
		 RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	if (ptpClock->delayMechanism == P2P) {
		/* Boolean isFromCurrentParent = FALSE; NOTE: This is never used in this function */
		TimeInternal requestReceiptTimestamp;
		TimeInternal correctionField;
	
		DBG("PdelayResp message received : \n");

		if (length < PDELAY_RESP_LENGTH) {
			DBG("Error: PDelayResp message too short\n");
			ptpClock->counters.messageFormatErrors++;
			return;
		}

		switch (ptpClock->portState ) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
			DBGV("HandlePdelayResp : disregard\n");
			ptpClock->counters.discardedMessages++;
			return;

		case PTP_SLAVE:
		case PTP_MASTER:
			if (ptpClock->twoStepFlag && isFromSelf) {
				processPDelayRespFromSelf(tint, rtOpts, ptpClock, header->sequenceId);
				break;
			}
			msgUnpackPDelayResp(ptpClock->msgIbuf,
					    &ptpClock->msgTmp.presp);
		
#if 0  /* NOTE: This is never used in this function. Should it? */
/*
 * wowczarek: shouldn't, P2P vs. E2E. Parent can be somewhere else,
 * peer can sit  on an adjacent link - see next evaluation:
 * requestingPortIdentity vs. parentPortIdentity
 */
			isFromCurrentParent = !memcmp(ptpClock->parentPortIdentity.clockIdentity,
						      header->sourcePortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH) && 
				(ptpClock->parentPortIdentity.portNumber == 
				 header->sourcePortIdentity.portNumber);
#endif	
			if (ptpClock->sentPDelayReqSequenceId != 
			       ((UInteger16)(header->sequenceId + 1))) {
				    DBG("PDelayResp: sequence mismatch - sent: %d, received: %d\n",
					    ptpClock->sentPDelayReqSequenceId,
					    header->sequenceId);
				    ptpClock->counters.discardedMessages++;
				    ptpClock->counters.sequenceMismatchErrors++;
				    break;
			}
			if ((!memcmp(ptpClock->portIdentity.clockIdentity,ptpClock->msgTmp.presp.requestingPortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH))
				 && ( ptpClock->portIdentity.portNumber == ptpClock->msgTmp.presp.requestingPortIdentity.portNumber))	{
				ptpClock->counters.pdelayRespMessagesReceived++;
                                /* Two Step Clock */
				if ((header->flagField0 & PTP_TWO_STEP) == PTP_TWO_STEP) {
					/*Store t4 (Fig 35)*/
					ptpClock->pdelay_resp_receive_time.seconds = tint->seconds;
					ptpClock->pdelay_resp_receive_time.nanoseconds = tint->nanoseconds;
					/*store t2 (Fig 35)*/
					toInternalTime(&requestReceiptTimestamp,
						       &ptpClock->msgTmp.presp.requestReceiptTimestamp);
					ptpClock->pdelay_req_receive_time.seconds = requestReceiptTimestamp.seconds;
					ptpClock->pdelay_req_receive_time.nanoseconds = requestReceiptTimestamp.nanoseconds;
					
					integer64_to_internalTime(header->correctionField,&correctionField);
					ptpClock->lastPdelayRespCorrectionField.seconds = correctionField.seconds;
					ptpClock->lastPdelayRespCorrectionField.nanoseconds = correctionField.nanoseconds;
				} else {
				/* One step Clock */
					/*Store t4 (Fig 35)*/
					ptpClock->pdelay_resp_receive_time.seconds = tint->seconds;
					ptpClock->pdelay_resp_receive_time.nanoseconds = tint->nanoseconds;
					
					integer64_to_internalTime(header->correctionField,&correctionField);
					updatePeerDelay (&ptpClock->owd_filt,rtOpts,ptpClock,&correctionField,FALSE);
				}
				ptpClock->recvPDelayRespSequenceId = header->sequenceId;
				break;
			} else {
				DBG("HandlePdelayResp : Pdelayresp doesn't "
				     "match with the PdelayReq. \n");
				ptpClock->counters.discardedMessages++;
				break;
			}
			break; /* XXX added by gnn for safety */
		default:
			DBG("do unrecognized state4\n");
			ptpClock->counters.discardedMessages++;
			break;
		}
	} else if (ptpClock->delayMechanism == E2E) { /* (End to End mode..) */
		DBG("Peer Delay messages are disregarded in End to End "
		      "mode \n");
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.delayModeMismatchErrors++;
	/* no delay mechanism */
	} else {
		DBG("PDelayResp ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}

static void
processPDelayRespFromSelf(const TimeInternal * tint, RunTimeOpts * rtOpts, PtpClock * ptpClock, UInteger16 sequenceId) {
	TimeInternal timestamp;
	
	addTime(&timestamp, tint, &rtOpts->outboundLatency);

	issuePDelayRespFollowUp(&timestamp, &ptpClock->PdelayReqHeader,
		rtOpts, ptpClock, sequenceId);
}

static void 
handlePDelayRespFollowUp(const MsgHeader *header, ssize_t length, 
			 Boolean isFromSelf, RunTimeOpts *rtOpts, 
			 PtpClock *ptpClock)
{

	if (ptpClock->delayMechanism == P2P) {
		TimeInternal responseOriginTimestamp;
		TimeInternal correctionField;
	
		DBG("PdelayRespfollowup message received : \n");
	
		if(length < PDELAY_RESP_FOLLOW_UP_LENGTH) {
			DBG("Error: PDelayRespfollowup message too short\n");
			ptpClock->counters.messageFormatErrors++;
			return;
		}	
	
		switch(ptpClock->portState) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
			DBGV("HandlePdelayResp : disregard\n");
			ptpClock->counters.discardedMessages++;
			return;
		
		case PTP_SLAVE:
		case PTP_MASTER:
			if (isFromSelf) {
				DBGV("HandlePdelayRespFollowUp : Ignore message from self \n");
				return;
			}
			if (( ((UInteger16)(header->sequenceId + 1)) ==
			    ptpClock->sentPDelayReqSequenceId) && (header->sequenceId == ptpClock->recvPDelayRespSequenceId)) {
				msgUnpackPDelayRespFollowUp(
					ptpClock->msgIbuf,
					&ptpClock->msgTmp.prespfollow);
				ptpClock->counters.pdelayRespFollowUpMessagesReceived++;
				toInternalTime(
					&responseOriginTimestamp,
					&ptpClock->msgTmp.prespfollow.responseOriginTimestamp);
				ptpClock->pdelay_resp_send_time.seconds = 
					responseOriginTimestamp.seconds;
				ptpClock->pdelay_resp_send_time.nanoseconds = 
					responseOriginTimestamp.nanoseconds;
				integer64_to_internalTime(
					ptpClock->msgTmpHeader.correctionField,
					&correctionField);
				addTime(&correctionField,&correctionField,
					&ptpClock->lastPdelayRespCorrectionField);
				updatePeerDelay (&ptpClock->owd_filt,
						 rtOpts, ptpClock,
						 &correctionField,TRUE);
				break;
			} else {
				DBG("PdelayRespFollowup: sequence mismatch - Received: %d "
				"PdelayReq sent: %d, PdelayResp received: %d\n",
				header->sequenceId, ptpClock->sentPDelayReqSequenceId,
				ptpClock->recvPDelayRespSequenceId);
				ptpClock->counters.discardedMessages++;
				ptpClock->counters.sequenceMismatchErrors++;
				break;
			}
		default:
			DBGV("Disregard PdelayRespFollowUp message  \n");
			ptpClock->counters.discardedMessages++;
		}
	} else if (ptpClock->delayMechanism == E2E) { /* (End to End mode..) */
		DBG("Peer Delay messages are disregarded in End to End "
		      "mode \n");
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.delayModeMismatchErrors++;
	/* no delay mechanism */
	} else {
		DBG("PDelayRespFollowUp ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}

/* Only accept the management message if it satisfies 15.3.1 Table 36 */ 
int
acceptManagementMessage(PortIdentity thisPort, PortIdentity targetPort)
{
        ClockIdentity allOnesClkIdentity;
        UInteger16    allOnesPortNumber = 0xFFFF;
        memset(allOnesClkIdentity, 0xFF, sizeof(allOnesClkIdentity));

        return ((memcmp(targetPort.clockIdentity, thisPort.clockIdentity, CLOCK_IDENTITY_LENGTH) == 0) ||
                (memcmp(targetPort.clockIdentity, allOnesClkIdentity, CLOCK_IDENTITY_LENGTH) == 0))
                &&
                ((targetPort.portNumber == thisPort.portNumber) ||
                (targetPort.portNumber == allOnesPortNumber));
}


static void
handleManagement(MsgHeader *header,
		 Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBGV("Management message received : \n");

	if(!rtOpts->managementEnabled) {
		DBGV("Dropping management message - management message support disabled");
		ptpClock->counters.discardedMessages++;
		return;
	}

	if (isFromSelf) {
		DBGV("handleManagement: Ignore message from self \n");
		return;
	}

	msgUnpackManagement(ptpClock->msgIbuf,&ptpClock->msgTmp.manage, header, ptpClock);

	if(ptpClock->msgTmp.manage.tlv == NULL) {
		DBGV("handleManagement: TLV is empty\n");
		ptpClock->counters.messageFormatErrors++;
		return;
	}

        if(!acceptManagementMessage(ptpClock->portIdentity, ptpClock->msgTmp.manage.targetPortIdentity))
        {
                DBGV("handleManagement: The management message was not accepted");
		ptpClock->counters.discardedMessages++;
                return;
        }

	if(!rtOpts->managementSetEnable &&
	    (ptpClock->msgTmp.manage.actionField == SET ||
	    ptpClock->msgTmp.manage.actionField == COMMAND)) {
		DBGV("Dropping SET/COMMAND management message - read-only mode enabled");
		ptpClock->counters.discardedMessages++;
		return;
	}

	/* is this an error status management TLV? */
	if(ptpClock->msgTmp.manage.tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS) {
		DBGV("handleManagement: Error Status TLV\n");
		unpackMMErrorStatus(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		handleMMErrorStatus(&ptpClock->msgTmp.manage);
		ptpClock->counters.managementMessagesReceived++;
		/* cleanup msgTmp managementTLV */
		if(ptpClock->msgTmp.manage.tlv) {
			DBGV("cleanup ptpClock->msgTmp.manage message \n");
			if(ptpClock->msgTmp.manage.tlv->dataField) {
				freeMMErrorStatusTLV(ptpClock->msgTmp.manage.tlv);
				free(ptpClock->msgTmp.manage.tlv->dataField);
			}
			free(ptpClock->msgTmp.manage.tlv);
		}
		return;
	} else if (ptpClock->msgTmp.manage.tlv->tlvType != TLV_MANAGEMENT) {
		/* do nothing, implemention specific handling */
		DBGV("handleManagement: Currently unsupported management TLV type\n");
		ptpClock->counters.discardedMessages++;
		return;
	}

	switch(ptpClock->msgTmp.manage.tlv->managementId)
	{
	case MM_NULL_MANAGEMENT:
		DBGV("handleManagement: Null Management\n");
		handleMMNullManagement(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
		break;
	case MM_CLOCK_DESCRIPTION:
		DBGV("handleManagement: Clock Description\n");
		unpackMMClockDescription(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		handleMMClockDescription(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
		break;
	case MM_USER_DESCRIPTION:
		DBGV("handleManagement: User Description\n");
		unpackMMUserDescription(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		handleMMUserDescription(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
		break;
	case MM_SAVE_IN_NON_VOLATILE_STORAGE:
		DBGV("handleManagement: Save In Non-Volatile Storage\n");
		handleMMSaveInNonVolatileStorage(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
		break;
	case MM_RESET_NON_VOLATILE_STORAGE:
		DBGV("handleManagement: Reset Non-Volatile Storage\n");
		handleMMResetNonVolatileStorage(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
		break;
	case MM_INITIALIZE:
		DBGV("handleManagement: Initialize\n");
		unpackMMInitialize(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		handleMMInitialize(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
		break;
	case MM_DEFAULT_DATA_SET:
		DBGV("handleManagement: Default Data Set\n");
		unpackMMDefaultDataSet(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		handleMMDefaultDataSet(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
		break;
	case MM_CURRENT_DATA_SET:
		DBGV("handleManagement: Current Data Set\n");
		unpackMMCurrentDataSet(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		handleMMCurrentDataSet(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
		break;
        case MM_PARENT_DATA_SET:
                DBGV("handleManagement: Parent Data Set\n");
                unpackMMParentDataSet(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMParentDataSet(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_TIME_PROPERTIES_DATA_SET:
                DBGV("handleManagement: TimeProperties Data Set\n");
                unpackMMTimePropertiesDataSet(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMTimePropertiesDataSet(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_PORT_DATA_SET:
                DBGV("handleManagement: Port Data Set\n");
                unpackMMPortDataSet(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMPortDataSet(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_PRIORITY1:
                DBGV("handleManagement: Priority1\n");
                unpackMMPriority1(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMPriority1(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_PRIORITY2:
                DBGV("handleManagement: Priority2\n");
                unpackMMPriority2(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMPriority2(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_DOMAIN:
                DBGV("handleManagement: Domain\n");
                unpackMMDomain(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMDomain(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
	case MM_SLAVE_ONLY:
		DBGV("handleManagement: Slave Only\n");
		unpackMMSlaveOnly(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
		handleMMSlaveOnly(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
		break;
        case MM_LOG_ANNOUNCE_INTERVAL:
                DBGV("handleManagement: Log Announce Interval\n");
                unpackMMLogAnnounceInterval(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMLogAnnounceInterval(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_ANNOUNCE_RECEIPT_TIMEOUT:
                DBGV("handleManagement: Announce Receipt Timeout\n");
                unpackMMAnnounceReceiptTimeout(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMAnnounceReceiptTimeout(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_LOG_SYNC_INTERVAL:
                DBGV("handleManagement: Log Sync Interval\n");
                unpackMMLogSyncInterval(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMLogSyncInterval(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_VERSION_NUMBER:
                DBGV("handleManagement: Version Number\n");
                unpackMMVersionNumber(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMVersionNumber(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_ENABLE_PORT:
                DBGV("handleManagement: Enable Port\n");
                handleMMEnablePort(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_DISABLE_PORT:
                DBGV("handleManagement: Disable Port\n");
                handleMMDisablePort(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_TIME:
                DBGV("handleManagement: Time\n");
                unpackMMTime(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMTime(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock, rtOpts);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_CLOCK_ACCURACY:
                DBGV("handleManagement: Clock Accuracy\n");
                unpackMMClockAccuracy(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMClockAccuracy(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_UTC_PROPERTIES:
                DBGV("handleManagement: Utc Properties\n");
                unpackMMUtcProperties(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMUtcProperties(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_TRACEABILITY_PROPERTIES:
                DBGV("handleManagement: Traceability Properties\n");
                unpackMMTraceabilityProperties(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMTraceabilityProperties(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_DELAY_MECHANISM:
                DBGV("handleManagement: Delay Mechanism\n");
                unpackMMDelayMechanism(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMDelayMechanism(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
        case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
                DBGV("handleManagement: Log Min Pdelay Req Interval\n");
                unpackMMLogMinPdelayReqInterval(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
                handleMMLogMinPdelayReqInterval(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp, ptpClock);
		ptpClock->counters.managementMessagesReceived++;
                break;
	case MM_FAULT_LOG:
	case MM_FAULT_LOG_RESET:
	case MM_TIMESCALE_PROPERTIES:
	case MM_UNICAST_NEGOTIATION_ENABLE:
	case MM_PATH_TRACE_LIST:
	case MM_PATH_TRACE_ENABLE:
	case MM_GRANDMASTER_CLUSTER_TABLE:
	case MM_UNICAST_MASTER_TABLE:
	case MM_UNICAST_MASTER_MAX_TABLE_SIZE:
	case MM_ACCEPTABLE_MASTER_TABLE:
	case MM_ACCEPTABLE_MASTER_TABLE_ENABLED:
	case MM_ACCEPTABLE_MASTER_MAX_TABLE_SIZE:
	case MM_ALTERNATE_MASTER:
	case MM_ALTERNATE_TIME_OFFSET_ENABLE:
	case MM_ALTERNATE_TIME_OFFSET_NAME:
	case MM_ALTERNATE_TIME_OFFSET_MAX_KEY:
	case MM_ALTERNATE_TIME_OFFSET_PROPERTIES:
	case MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
	case MM_TRANSPARENT_CLOCK_PORT_DATA_SET:
	case MM_PRIMARY_DOMAIN:
		DBGV("handleManagement: Currently unsupported managementTLV %d\n",
				ptpClock->msgTmp.manage.tlv->managementId);
		handleErrorManagementMessage(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp,
			ptpClock, ptpClock->msgTmp.manage.tlv->managementId,
			NOT_SUPPORTED);
		ptpClock->counters.discardedMessages++;
		break;
	default:
		DBGV("handleManagement: Unknown managementTLV %d\n",
				ptpClock->msgTmp.manage.tlv->managementId);
		handleErrorManagementMessage(&ptpClock->msgTmp.manage, &ptpClock->outgoingManageTmp,
			ptpClock, ptpClock->msgTmp.manage.tlv->managementId,
			NO_SUCH_ID);
		ptpClock->counters.discardedMessages++;
	}

	/* send management message response or acknowledge */
	if(ptpClock->outgoingManageTmp.tlv->tlvType == TLV_MANAGEMENT) {
		if(ptpClock->outgoingManageTmp.actionField == RESPONSE ||
				ptpClock->outgoingManageTmp.actionField == ACKNOWLEDGE) {
			issueManagementRespOrAck(&ptpClock->outgoingManageTmp, rtOpts, ptpClock);
		}
	} else if(ptpClock->outgoingManageTmp.tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS) {
		issueManagementErrorStatus(&ptpClock->outgoingManageTmp, rtOpts, ptpClock);
	}

	/* cleanup msgTmp managementTLV */
	freeManagementTLV(&ptpClock->msgTmp.manage);
	/* cleanup outgoing managementTLV */
	freeManagementTLV(&ptpClock->outgoingManageTmp);

}

static void 
handleSignaling(PtpClock *ptpClock)
{

	ptpClock->counters.signalingMessagesReceived++;

}

/*Pack and send on general multicast ip adress an Announce message*/
static void 
issueAnnounce(RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	msgPackAnnounce(ptpClock->msgObuf,ptpClock);

	if (!netSendGeneral(ptpClock->msgObuf,ANNOUNCE_LENGTH,
			    &ptpClock->netPath, rtOpts, 0)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("Announce message can't be sent -> FAULTY state \n");
	} else {
		DBGV("Announce MSG sent ! \n");
		ptpClock->sentAnnounceSequenceId++;
		ptpClock->counters.announceMessagesSent++;
	}
}



/*Pack and send on event multicast ip adress a Sync message*/
static void
issueSync(RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Timestamp originTimestamp;
	TimeInternal internalTime;
	getTime(&internalTime);
	if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
		internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
	}
	fromInternalTime(&internalTime,&originTimestamp);

	msgPackSync(ptpClock->msgObuf,&originTimestamp,ptpClock);

	if (!netSendEvent(ptpClock->msgObuf,SYNC_LENGTH,&ptpClock->netPath,
		rtOpts, 0, &internalTime)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("Sync message can't be sent -> FAULTY state \n");
	} else {
		DBGV("Sync MSG sent ! \n");
#ifdef SO_TIMESTAMPING
		if(!ptpClock->netPath.txTimestampFailure) {
			if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
				internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			}
			
			processSyncFromSelf(&internalTime, rtOpts, ptpClock, ptpClock->sentSyncSequenceId);
		}
#endif

		ptpClock->sentSyncSequenceId++;
		ptpClock->counters.syncMessagesSent++;
	}
}


/*Pack and send on general multicast ip adress a FollowUp message*/
static void
issueFollowup(const TimeInternal *tint,RunTimeOpts *rtOpts,PtpClock *ptpClock, UInteger16 sequenceId)
{
	Timestamp preciseOriginTimestamp;
	fromInternalTime(tint,&preciseOriginTimestamp);
	
	msgPackFollowUp(ptpClock->msgObuf,&preciseOriginTimestamp,ptpClock,sequenceId);	

	if (!netSendGeneral(ptpClock->msgObuf,FOLLOW_UP_LENGTH,
			    &ptpClock->netPath, rtOpts, 0)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("FollowUp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("FollowUp MSG sent ! \n");
		ptpClock->counters.followUpMessagesSent++;
	}
}


/*Pack and send on event multicast ip adress a DelayReq message*/
static void
issueDelayReq(RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Timestamp originTimestamp;
	TimeInternal internalTime;
#if 0 /* PCAP ONLY */
	MsgHeader ourDelayReq;
#endif

	DBG("==> Issue DelayReq (%d)\n", ptpClock->sentDelayReqSequenceId );

	/*
	 * call GTOD. This time is later replaced in handleDelayReq,
	 * to get the actual send timestamp from the OS
	 */
	getTime(&internalTime);
	if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
		internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
	}
	fromInternalTime(&internalTime,&originTimestamp);

	// uses current sentDelayReqSequenceId
	msgPackDelayReq(ptpClock->msgObuf,&originTimestamp,ptpClock);

	Integer32 dst = 0;

	if (rtOpts->ip_mode == IPMODE_HYBRID) {
		dst = ptpClock->masterAddr;
	}

	if (!netSendEvent(ptpClock->msgObuf,DELAY_REQ_LENGTH,
			  &ptpClock->netPath, rtOpts, dst, &internalTime)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("delayReq message can't be sent -> FAULTY state \n");
	} else {
		DBGV("DelayReq MSG sent ! \n");
		
#ifdef SO_TIMESTAMPING
		if(!ptpClock->netPath.txTimestampFailure) {
			if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
				internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			}			
			
			processDelayReqFromSelf(&internalTime, rtOpts, ptpClock);
		}
#endif
		
		ptpClock->sentDelayReqSequenceId++;
		ptpClock->counters.delayReqMessagesSent++;

		/* From now on, we will only accept delayreq and
		 * delayresp of (sentDelayReqSequenceId - 1) */

		/* Explicitelly re-arm timer for sending the next delayReq */
		timerStart_random(DELAYREQ_INTERVAL_TIMER,
		   pow(2,ptpClock->logMinDelayReqInterval),
		   ptpClock->itimer);
#if 0 /* PCAP ONLY */
		msgUnpackHeader(ptpClock->msgObuf, &ourDelayReq);
		handleDelayReq(&ourDelayReq, DELAY_REQ_LENGTH, &internalTime,
			       TRUE, rtOpts, ptpClock);
#endif 
	}
}

/*Pack and send on event multicast ip adress a PDelayReq message*/
static void
issuePDelayReq(RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Timestamp originTimestamp;
	TimeInternal internalTime;
	getTime(&internalTime);
	if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
		internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
	}
	fromInternalTime(&internalTime,&originTimestamp);
	
	msgPackPDelayReq(ptpClock->msgObuf,&originTimestamp,ptpClock);
	if (!netSendPeerEvent(ptpClock->msgObuf,PDELAY_REQ_LENGTH,
			      &ptpClock->netPath, rtOpts, &internalTime)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("PdelayReq message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PDelayReq MSG sent ! \n");
		
#ifdef SO_TIMESTAMPING
		if(!ptpClock->netPath.txTimestampFailure) {
			if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
				internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			}			
			processPDelayReqFromSelf(&internalTime, rtOpts, ptpClock);
		}
#endif
		
		ptpClock->sentPDelayReqSequenceId++;
		ptpClock->counters.pdelayReqMessagesSent++;
	}
}

/*Pack and send on event multicast ip adress a PDelayResp message*/
static void
issuePDelayResp(const TimeInternal *tint,MsgHeader *header,RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{
	Timestamp requestReceiptTimestamp;
	TimeInternal internalTime;
	
	fromInternalTime(tint,&requestReceiptTimestamp);
	msgPackPDelayResp(ptpClock->msgObuf,header,
			  &requestReceiptTimestamp,ptpClock);

	if (!netSendPeerEvent(ptpClock->msgObuf,PDELAY_RESP_LENGTH,
			      &ptpClock->netPath, rtOpts, &internalTime)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("PdelayResp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PDelayResp MSG sent ! \n");
		
#ifdef SO_TIMESTAMPING
		if(!ptpClock->netPath.txTimestampFailure) {
			if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
				internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			}			
			processPDelayRespFromSelf(&internalTime, rtOpts, ptpClock, header->sequenceId);
		}
#endif
		
		ptpClock->counters.pdelayRespMessagesSent++;
	}
}


/*Pack and send on event multicast ip adress a DelayResp message*/
static void
issueDelayResp(const TimeInternal *tint,MsgHeader *header,RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Timestamp requestReceiptTimestamp;
	fromInternalTime(tint,&requestReceiptTimestamp);
	msgPackDelayResp(ptpClock->msgObuf,header,&requestReceiptTimestamp,
			 ptpClock);

	Integer32 dst = 0;

	if (rtOpts->ip_mode == IPMODE_HYBRID) {
		dst = ptpClock->LastSlaveAddr;
	}

	if (!netSendGeneral(ptpClock->msgObuf,PDELAY_RESP_LENGTH,
			    &ptpClock->netPath, rtOpts, dst)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("delayResp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PDelayResp MSG sent ! \n");
		ptpClock->counters.delayRespMessagesSent++;
	}
}


static void
issuePDelayRespFollowUp(const TimeInternal *tint, MsgHeader *header,
			     RunTimeOpts *rtOpts, PtpClock *ptpClock, const UInteger16 sequenceId)
{
	Timestamp responseOriginTimestamp;
	fromInternalTime(tint,&responseOriginTimestamp);

	msgPackPDelayRespFollowUp(ptpClock->msgObuf,header,
				  &responseOriginTimestamp,ptpClock, sequenceId);
	if (!netSendPeerGeneral(ptpClock->msgObuf,
				PDELAY_RESP_FOLLOW_UP_LENGTH,
				&ptpClock->netPath, rtOpts)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("PdelayRespFollowUp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PDelayRespFollowUp MSG sent ! \n");
		ptpClock->counters.pdelayRespFollowUpMessagesSent++;
	}
}

#if 0
static void 
issueManagement(MsgHeader *header,MsgManagement *manage,RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{

	ptpClock->counters.managementMessagesSent++;

}
#endif

static void 
issueManagementRespOrAck(MsgManagement *outgoing, RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{
	/* pack ManagementTLV */
	msgPackManagementTLV( ptpClock->msgObuf, outgoing, ptpClock);

	/* set header messageLength, the outgoing->tlv->lengthField is now valid */
	outgoing->header.messageLength = MANAGEMENT_LENGTH +
					TL_LENGTH +
					outgoing->tlv->lengthField;

	msgPackManagement( ptpClock->msgObuf, outgoing, ptpClock);

	if(!netSendGeneral(ptpClock->msgObuf, outgoing->header.messageLength,
			   &ptpClock->netPath, rtOpts, 0)) {
		DBGV("Management response/acknowledge can't be sent -> FAULTY state \n");
		ptpClock->counters.messageSendErrors++;
		toState(PTP_FAULTY, rtOpts, ptpClock);
	} else {
		DBGV("Management response/acknowledge msg sent \n");
		ptpClock->counters.managementMessagesSent++;
	}
}

static void
issueManagementErrorStatus(MsgManagement *outgoing, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	/* pack ManagementErrorStatusTLV */
	msgPackManagementErrorStatusTLV( ptpClock->msgObuf, outgoing, ptpClock);

	/* set header messageLength, the outgoing->tlv->lengthField is now valid */
	outgoing->header.messageLength = MANAGEMENT_LENGTH +
					TL_LENGTH +
					outgoing->tlv->lengthField;

	msgPackManagement( ptpClock->msgObuf, outgoing, ptpClock);

	if(!netSendGeneral(ptpClock->msgObuf, outgoing->header.messageLength,
			   &ptpClock->netPath, rtOpts, 0)) {
		DBGV("Management error status can't be sent -> FAULTY state \n");
		ptpClock->counters.messageSendErrors++;
		toState(PTP_FAULTY, rtOpts, ptpClock);
	} else {
		DBGV("Management error status msg sent \n");
		ptpClock->counters.managementMessagesSent++;
	}

}

void
addForeign(Octet *buf,MsgHeader *header,PtpClock *ptpClock)
{
	int i,j;
	Boolean found = FALSE;

	j = ptpClock->foreign_record_best;
	
	/*Check if Foreign master is already known*/
	for (i=0;i<ptpClock->number_foreign_records;i++) {
		if (!memcmp(header->sourcePortIdentity.clockIdentity,
			    ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity,
			    CLOCK_IDENTITY_LENGTH) && 
		    (header->sourcePortIdentity.portNumber == 
		     ptpClock->foreign[j].foreignMasterPortIdentity.portNumber))
		{
			/*Foreign Master is already in Foreignmaster data set*/
			ptpClock->foreign[j].foreignMasterAnnounceMessages++; 
			found = TRUE;
			DBGV("addForeign : AnnounceMessage incremented \n");
			msgUnpackHeader(buf,&ptpClock->foreign[j].header);
			msgUnpackAnnounce(buf,&ptpClock->foreign[j].announce);
			break;
		}
	
		j = (j+1)%ptpClock->number_foreign_records;
	}

	/*New Foreign Master*/
	if (!found) {
		if (ptpClock->number_foreign_records < 
		    ptpClock->max_foreign_records) {
			ptpClock->number_foreign_records++;
		}
		j = ptpClock->foreign_record_i;
		
		/*Copy new foreign master data set from Announce message*/
		copyClockIdentity(ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity,
		       header->sourcePortIdentity.clockIdentity);
		ptpClock->foreign[j].foreignMasterPortIdentity.portNumber = 
			header->sourcePortIdentity.portNumber;
		ptpClock->foreign[j].foreignMasterAnnounceMessages = 0;
		
		/*
		 * header and announce field of each Foreign Master are
		 * usefull to run Best Master Clock Algorithm
		 */
		msgUnpackHeader(buf,&ptpClock->foreign[j].header);
		msgUnpackAnnounce(buf,&ptpClock->foreign[j].announce);
		DBGV("New foreign Master added \n");
		
		ptpClock->foreign_record_i = 
			(ptpClock->foreign_record_i+1) % 
			ptpClock->max_foreign_records;	
	}
}

/* Update dataset fields which are safe to change without going into INITIALIZING */
static void
updateDatasets(PtpClock* ptpClock, RunTimeOpts* rtOpts)
{

	switch(ptpClock->portState) {

		/* We are master so update both the port and the parent dataset */
		case PTP_MASTER:

			ptpClock->numberPorts = NUMBER_PORTS;
			ptpClock->delayMechanism = rtOpts->delayMechanism;
			ptpClock->versionNumber = VERSION_PTP;

			ptpClock->clockQuality.clockAccuracy = 
				rtOpts->clockQuality.clockAccuracy;
			ptpClock->clockQuality.clockClass = rtOpts->clockQuality.clockClass;
			ptpClock->clockQuality.offsetScaledLogVariance = 
				rtOpts->clockQuality.offsetScaledLogVariance;
			ptpClock->priority1 = rtOpts->priority1;
			ptpClock->priority2 = rtOpts->priority2;

			ptpClock->grandmasterClockQuality.clockAccuracy = 
				ptpClock->clockQuality.clockAccuracy;
			ptpClock->grandmasterClockQuality.clockClass = 
				ptpClock->clockQuality.clockClass;
			ptpClock->grandmasterClockQuality.offsetScaledLogVariance = 
				ptpClock->clockQuality.offsetScaledLogVariance;
			ptpClock->clockQuality.clockAccuracy = 
			ptpClock->grandmasterPriority1 = ptpClock->priority1;
			ptpClock->grandmasterPriority2 = ptpClock->priority2;
			ptpClock->timePropertiesDS.currentUtcOffsetValid = rtOpts->timeProperties.currentUtcOffsetValid;
			ptpClock->timePropertiesDS.currentUtcOffset = rtOpts->timeProperties.currentUtcOffset;
			ptpClock->timePropertiesDS.timeTraceable = rtOpts->timeProperties.timeTraceable;
			ptpClock->timePropertiesDS.frequencyTraceable = rtOpts->timeProperties.frequencyTraceable;
			ptpClock->timePropertiesDS.ptpTimescale = rtOpts->timeProperties.ptpTimescale;
			ptpClock->timePropertiesDS.timeSource = rtOpts->timeProperties.timeSource;
			ptpClock->logAnnounceInterval = rtOpts->announceInterval;
			ptpClock->announceReceiptTimeout = rtOpts->announceReceiptTimeout;
			ptpClock->logSyncInterval = rtOpts->syncInterval;
			ptpClock->logMinPdelayReqInterval = rtOpts->logMinPdelayReqInterval;
			ptpClock->logMinDelayReqInterval = rtOpts->initial_delayreq;
			break;
		/*
		 * we are not master so update the port dataset only - parent will be updated
		 * by m1() if we go master - basically update the fields affecting BMC only
		 */
		case PTP_SLAVE:
		case PTP_PASSIVE:
			ptpClock->numberPorts = NUMBER_PORTS;
			ptpClock->delayMechanism = rtOpts->delayMechanism;
			ptpClock->versionNumber = VERSION_PTP;
			ptpClock->clockQuality.clockAccuracy = 
				rtOpts->clockQuality.clockAccuracy;
			ptpClock->clockQuality.clockClass = rtOpts->clockQuality.clockClass;
			ptpClock->clockQuality.offsetScaledLogVariance = 
				rtOpts->clockQuality.offsetScaledLogVariance;
			ptpClock->priority1 = rtOpts->priority1;
			ptpClock->priority2 = rtOpts->priority2;
			break;
		/* In all other states the datasets will be updated when going into an operational state */
		default:
		    break;
	}

}

void
clearCounters(PtpClock * ptpClock)
{

	/* TODO: print port info */
	DBG("Port counters cleared\n");
	memset(&ptpClock->counters, 0, sizeof(ptpClock->counters));

}

Boolean
respectUtcOffset(RunTimeOpts * rtOpts, PtpClock * ptpClock) {
	if (ptpClock->timePropertiesDS.currentUtcOffsetValid || rtOpts->alwaysRespectUtcOffset) {
		return TRUE;
	}
	return FALSE;
}
