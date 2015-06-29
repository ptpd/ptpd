/*-
 * Copyright (c) 2012-2015 Wojciech Owczarek,
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
static void doState(const RunTimeOpts*,PtpClock*);

void handle(const RunTimeOpts*,PtpClock*);

static void handleAnnounce(MsgHeader*, ssize_t,Boolean, const RunTimeOpts*,PtpClock*);
static void handleSync(const MsgHeader*, ssize_t,TimeInternal*,Boolean,Integer32, Integer32, const RunTimeOpts*,PtpClock*);
static void handleFollowUp(const MsgHeader*, ssize_t,Boolean,const RunTimeOpts*,PtpClock*);
static void handlePdelayReq(MsgHeader*, ssize_t,const TimeInternal*, Integer32, Boolean,const RunTimeOpts*,PtpClock*);
static void handleDelayReq(const MsgHeader*, ssize_t, const TimeInternal*,Integer32, Boolean,const RunTimeOpts*,PtpClock*);
static void handlePdelayResp(const MsgHeader*, TimeInternal* ,ssize_t,Boolean, Integer32, Integer32, const RunTimeOpts*,PtpClock*);
static void handleDelayResp(const MsgHeader*, ssize_t, const RunTimeOpts*,PtpClock*);
static void handlePdelayRespFollowUp(const MsgHeader*, ssize_t, Boolean, const RunTimeOpts*,PtpClock*);
static void handleManagement(MsgHeader*, Boolean,const RunTimeOpts*,PtpClock*);

/* handleSignaling in signaling.c */

#ifndef PTPD_SLAVE_ONLY /* does not get compiled when building slave only */
static void issueAnnounce(const RunTimeOpts*,PtpClock*);
static void issueAnnounceSingle(Integer32, UInteger16*, const RunTimeOpts*,PtpClock*);
static void issueSync(const RunTimeOpts*,PtpClock*);
static TimeInternal issueSyncSingle(Integer32, UInteger16*, const RunTimeOpts*,PtpClock*);
static void issueFollowup(const TimeInternal*,const RunTimeOpts*,PtpClock*, Integer32, const UInteger16);
#endif /* PTPD_SLAVE_ONLY */
static void issuePdelayReq(const RunTimeOpts*,PtpClock*);
static void issueDelayReq(const RunTimeOpts*,PtpClock*);
static void issuePdelayResp(const TimeInternal*,MsgHeader*,Integer32,const RunTimeOpts*,PtpClock*);
static void issueDelayResp(const TimeInternal*,MsgHeader*,Integer32,const RunTimeOpts*,PtpClock*);
static void issuePdelayRespFollowUp(const TimeInternal*,MsgHeader*, Integer32, const RunTimeOpts*,PtpClock*, const UInteger16);
#if 0
static void issueManagement(MsgHeader*,MsgManagement*,const RunTimeOpts*,PtpClock*);
#endif
static void issueManagementRespOrAck(MsgManagement*,const RunTimeOpts*,PtpClock*);
static void issueManagementErrorStatus(MsgManagement*,const RunTimeOpts*,PtpClock*);

static void processMessage(const RunTimeOpts* rtOpts, PtpClock* ptpClock, TimeInternal* timeStamp, ssize_t length);

#ifndef PTPD_SLAVE_ONLY /* does not get compiled when building slave only */
static void processSyncFromSelf(const TimeInternal * tint, const RunTimeOpts * rtOpts, PtpClock * ptpClock, Integer32 dst, const UInteger16 sequenceId);
static void indexSync(TimeInternal *timeStamp, UInteger16 sequenceId, Integer32 transportAddress, SyncDestEntry *index);
#endif /* PTPD_SLAVE_ONLY */

static void processDelayReqFromSelf(const TimeInternal * tint, const RunTimeOpts * rtOpts, PtpClock * ptpClock);
static void processPdelayReqFromSelf(const TimeInternal * tint, const RunTimeOpts * rtOpts, PtpClock * ptpClock);
static void processPdelayRespFromSelf(const TimeInternal * tint, const RunTimeOpts * rtOpts, PtpClock * ptpClock, Integer32 dst, const UInteger16 sequenceId);

/* this shouldn't really be in protocol.c, it will be moved later */
static void timestampCorrection(const RunTimeOpts * rtOpts, PtpClock *ptpClock, TimeInternal *timeStamp);

static Integer32 lookupSyncIndex(TimeInternal *timeStamp, UInteger16 sequenceId, SyncDestEntry *index);
static Integer32 findSyncDestination(TimeInternal *timeStamp, const RunTimeOpts *rtOpts, PtpClock *ptpClock);


#ifndef PTPD_SLAVE_ONLY

/* store transportAddress in an index table */
static void
indexSync(TimeInternal *timeStamp, UInteger16 sequenceId, Integer32 transportAddress, SyncDestEntry *index)
{

    uint32_t hash = 0;

#ifdef RUNTIME_DEBUG
	struct in_addr tmpAddr;
	tmpAddr.s_addr = transportAddress;
#endif /* RUNTIME_DEBUG */

    if(timeStamp == NULL || index == NULL) {
	return;
    }

    hash = fnvHash(timeStamp, sizeof(TimeInternal), UNICAST_MAX_DESTINATIONS);

    if(index[hash].transportAddress) {
	DBG("indexSync: hash collision - clearing entry %s:%04x\n", inet_ntoa(tmpAddr), hash);
	index[hash].transportAddress = 0;
    } else {
	DBG("indexSync: indexed successfully %s:%04x\n", inet_ntoa(tmpAddr), hash);
	index[hash].transportAddress = transportAddress;
    }

}

#endif /* PTPD_SLAVE_ONLY */

/* sync destination index lookup */
static Integer32
lookupSyncIndex(TimeInternal *timeStamp, UInteger16 sequenceId, SyncDestEntry *index)
{

    uint32_t hash = 0;
    Integer32 previousAddress;

    if(timeStamp == NULL || index == NULL) {
	return 0;
    }

    hash = fnvHash(timeStamp, sizeof(TimeInternal), UNICAST_MAX_DESTINATIONS);

    if(index[hash].transportAddress == 0) {
	DBG("lookupSyncIndex: cache miss\n");
	return 0;
    } else {
	DBG("lookupSyncIndex: cache hit - clearing old entry\n");
	previousAddress =  index[hash].transportAddress;
	index[hash].transportAddress = 0;
	return previousAddress;
    }

}

/* iterative search for Sync destination for the given cached timestamp */
static Integer32
findSyncDestination(TimeInternal *timeStamp, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    int i = 0;

    for(i = 0; i < UNICAST_MAX_DESTINATIONS; i++) {

	if(rtOpts->unicastNegotiation) {
		if( (timeStamp->seconds == ptpClock->unicastGrants[i].lastSyncTimestamp.seconds) &&
		    (timeStamp->nanoseconds == ptpClock->unicastGrants[i].lastSyncTimestamp.nanoseconds)) {
			clearTime(&ptpClock->unicastGrants[i].lastSyncTimestamp);
			return ptpClock->unicastGrants[i].transportAddress;
		    }
	} else {
		if( (timeStamp->seconds == ptpClock->unicastDestinations[i].lastSyncTimestamp.seconds) &&
		    (timeStamp->nanoseconds == ptpClock->unicastDestinations[i].lastSyncTimestamp.nanoseconds)) {
			clearTime(&ptpClock->unicastDestinations[i].lastSyncTimestamp);
			return ptpClock->unicastDestinations[i].transportAddress;
		    }
	}
    }

    return 0;

}

void addForeign(Octet*,MsgHeader*,PtpClock*, UInteger8);

/* loop forever. doState() has a switch for the actions and events to be
   checked for 'port_state'. the actions and events may or may not change
   'port_state' by calling toState(), but once they are done we loop around
   again and perform the actions required for the new 'port_state'. */
void 
protocol(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBG("event POWERUP\n");

	timerStart(&ptpClock->timers[TIMINGDOMAIN_UPDATE_TIMER],timingDomain.updateInterval);

	toState(PTP_INITIALIZING, rtOpts, ptpClock);
	if(rtOpts->statusLog.logEnabled)
		writeStatusFile(ptpClock, rtOpts, TRUE);

	/* run the status file update every 1 .. 1.2 seconds */
	timerStart(&ptpClock->timers[STATUSFILE_UPDATE_TIMER],rtOpts->statusFileUpdateInterval * (1.0 + 0.2 * getRand()));
	timerStart(&ptpClock->timers[PERIODIC_INFO_TIMER],rtOpts->statsUpdateInterval);

	DBG("Debug Initializing...\n");

	if(rtOpts->statusLog.logEnabled)
		writeStatusFile(ptpClock, rtOpts, TRUE);

	for (;;)
	{
		/* 20110701: this main loop was rewritten to be more clear */

		if (ptpClock->portState == PTP_INITIALIZING) {

			    /* 
			     * DO NOT shut down once started. We have to "wait intelligently",
			     * that is keep processing signals. If init failed, wait for n seconds
			     * until next retry, do not exit. Wait in chunks so SIGALRM can interrupt.
			     */
			    if(ptpClock->initFailure) {
				    usleep(10000);
				    ptpClock->initFailureTimeout--;
			    }

			    if(!ptpClock->initFailure || ptpClock->initFailureTimeout <= 0) {
				if(!doInit(rtOpts, ptpClock)) {
					ERROR("PTPd init failed - will retry in %d seconds\n", DEFAULT_FAILURE_WAITTIME);
					writeStatusFile(ptpClock, rtOpts, TRUE);
					ptpClock->initFailure = TRUE;
					ptpClock->initFailureTimeout = 100 * DEFAULT_FAILURE_WAITTIME;
				} else {
					ptpClock->initFailure = FALSE;
					ptpClock->initFailureTimeout = 0;
				}

			   }
			
		} else {
			doState(rtOpts, ptpClock);
		}

		if (ptpClock->message_activity)
			DBGV("activity\n");

		/* Configuration has changed */
		if(rtOpts->restartSubsystems > 0) {
			restartSubsystems(rtOpts, ptpClock);
		}

		if (timerExpired(&ptpClock->timers[TIMINGDOMAIN_UPDATE_TIMER])) {
		    timingDomain.update(&timingDomain);
		}

		if (timerExpired(&ptpClock->timers[UNICAST_GRANT_TIMER])) {
			if(rtOpts->unicastDestinationsSet) {
			    refreshUnicastGrants(ptpClock->unicastGrants, 
				ptpClock->unicastDestinationCount, rtOpts, ptpClock);
			} else {
			    refreshUnicastGrants(ptpClock->unicastGrants, 
				UNICAST_MAX_DESTINATIONS, rtOpts, ptpClock);
			}
			if(ptpClock->unicastPeerDestination.transportAddress) {
			    refreshUnicastGrants(&ptpClock->peerGrants, 
				1, rtOpts, ptpClock);

			}
		}

		/* Perform the heavy signal processing synchronously */
		checkSignals(rtOpts, ptpClock);
	}
}


/* perform actions required when leaving 'port_state' and entering 'state' */
void 
toState(UInteger8 state, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	ptpClock->message_activity = TRUE;
	
	/* leaving state tasks */
	switch (ptpClock->portState)
	{
	case PTP_MASTER:

		timerStop(&ptpClock->timers[SYNC_INTERVAL_TIMER]);  
		timerStop(&ptpClock->timers[ANNOUNCE_INTERVAL_TIMER]);
		timerStop(&ptpClock->timers[PDELAYREQ_INTERVAL_TIMER]); 
		timerStop(&ptpClock->timers[MASTER_NETREFRESH_TIMER]); 

		if(rtOpts->unicastNegotiation && rtOpts->ipMode==IPMODE_UNICAST) {
		    cancelAllGrants(ptpClock->unicastGrants, UNICAST_MAX_DESTINATIONS,
				rtOpts, ptpClock);
		    if(ptpClock->delayMechanism == P2P) {
			    cancelAllGrants(&ptpClock->peerGrants, 1,
				rtOpts, ptpClock);
		    }
		}

		break;
	case PTP_SLAVE:
		timerStop(&ptpClock->timers[ANNOUNCE_RECEIPT_TIMER]);
		
		if(rtOpts->unicastNegotiation && rtOpts->ipMode==IPMODE_UNICAST && ptpClock->parentGrants != NULL) {
			/* do not cancel, just start re-requesting so we can still send a cancel on exit */
			ptpClock->parentGrants->grantData[ANNOUNCE].timeLeft = 0;
			ptpClock->parentGrants->grantData[SYNC].timeLeft = 0;
			if(ptpClock->delayMechanism == E2E) {
			    ptpClock->parentGrants->grantData[DELAY_RESP].timeLeft = 0;
			}

			ptpClock->parentGrants = NULL;

			if(ptpClock->delayMechanism == P2P) {
			    cancelUnicastTransmission(&ptpClock->peerGrants.grantData[PDELAY_RESP], rtOpts, ptpClock);
			    cancelAllGrants(&ptpClock->peerGrants, 1, rtOpts, ptpClock);
			}
		}

		if (ptpClock->delayMechanism == E2E)
			timerStop(&ptpClock->timers[DELAYREQ_INTERVAL_TIMER]);
		else if (ptpClock->delayMechanism == P2P)
			timerStop(&ptpClock->timers[PDELAYREQ_INTERVAL_TIMER]);
/* If statistics are enabled, drift should have been saved already - otherwise save it*/
#ifndef PTPD_STATISTICS
		/* save observed drift value, don't inform user */
		saveDrift(ptpClock, rtOpts, TRUE);
#endif /* PTPD_STATISTICS */

#ifdef PTPD_STATISTICS
		resetPtpEngineSlaveStats(&ptpClock->slaveStats);
		timerStop(&ptpClock->timers[STATISTICS_UPDATE_TIMER]);
#endif /* PTPD_STATISTICS */
		ptpClock->panicMode = FALSE;
		ptpClock->panicOver = FALSE;
		timerStop(&ptpClock->timers[PANIC_MODE_TIMER]);
		initClock(rtOpts, ptpClock);
		
	case PTP_PASSIVE:
		timerStop(&ptpClock->timers[PDELAYREQ_INTERVAL_TIMER]);
		timerStop(&ptpClock->timers[ANNOUNCE_RECEIPT_TIMER]);
		break;
		
	case PTP_LISTENING:
		timerStop(&ptpClock->timers[ANNOUNCE_RECEIPT_TIMER]);
		/* we're leaving LISTENING - reset counter */
                if(state != PTP_LISTENING) {
                    ptpClock->listenCount = 0;
                }
		break;
	case PTP_INITIALIZING:
		if(rtOpts->unicastNegotiation) {
		    if(!ptpClock->slaveOnly) {

			initUnicastGrantTable(ptpClock->unicastGrants, 
				ptpClock->delayMechanism, 
				UNICAST_MAX_DESTINATIONS, NULL,
				rtOpts, ptpClock);

			if(rtOpts->unicastDestinationsSet) {
			    initUnicastGrantTable(ptpClock->unicastGrants, 
					ptpClock->delayMechanism, 
					ptpClock->unicastDestinationCount, ptpClock->unicastDestinations,
					rtOpts, ptpClock);
			}
			
		    } else {
			initUnicastGrantTable(ptpClock->unicastGrants, 
					ptpClock->delayMechanism, 
					ptpClock->unicastDestinationCount, ptpClock->unicastDestinations,
					rtOpts, ptpClock);
		    }
		    if(ptpClock->unicastPeerDestination.transportAddress) {
			initUnicastGrantTable(&ptpClock->peerGrants, 
					ptpClock->delayMechanism, 
					1, &ptpClock->unicastPeerDestination,
					rtOpts, ptpClock);
		    }
			/* this must be set regardless of delay mechanism,
			 * so functions can see this is a peer table
			 */
			ptpClock->peerGrants.isPeer = TRUE;
		}
		break;
	default:
		break;
	}
	
	/* entering state tasks */

	ptpClock->counters.stateTransitions++;

	DBG("state %s\n",portState_getName(state));

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

		/* well, theoretically we're still in the previous state, so we're not in breach of standard */
		if(rtOpts->unicastNegotiation && rtOpts->ipMode==IPMODE_UNICAST) {
		    cancelAllGrants(ptpClock->unicastGrants, ptpClock->unicastDestinationCount,
				rtOpts, ptpClock);
		}
		ptpClock->masterAddr = 0;
		/* see? NOW we're in disabled state */
		ptpClock->portState = PTP_DISABLED;

		break;
		
	case PTP_LISTENING:

		if(rtOpts->unicastNegotiation) {
		    timerStart(&ptpClock->timers[UNICAST_GRANT_TIMER], UNICAST_GRANT_REFRESH_INTERVAL);
		}

		/* in Listening state, make sure we don't send anything. Instead we just expect/wait for announces (started below) */
		timerStop(&ptpClock->timers[SYNC_INTERVAL_TIMER]);
		timerStop(&ptpClock->timers[ANNOUNCE_INTERVAL_TIMER]);
		timerStop(&ptpClock->timers[PDELAYREQ_INTERVAL_TIMER]);
		timerStop(&ptpClock->timers[DELAYREQ_INTERVAL_TIMER]);

		/* This is (re) started on clock updates only */
                timerStop(&ptpClock->timers[CLOCK_UPDATE_TIMER]);

		/* if we're ignoring announces (disable_bmca), go straight to master */
		if(ptpClock->clockQuality.clockClass <= 127 && rtOpts->disableBMCA) {
			DBG("unicast master only and ignoreAnnounce: going into MASTER state\n");
			ptpClock->number_foreign_records = 0;
			ptpClock->foreign_record_i = 0;
			m1(rtOpts,ptpClock);
			toState(PTP_MASTER, rtOpts, ptpClock);
			break;
		}

		/*
		 *  Count how many _unique_ timeouts happen to us.
		 *  If we were already in Listen mode, then do not count this as a seperate reset, but stil do a new IGMP refresh
		 */
		if (ptpClock->portState != PTP_LISTENING) {
			ptpClock->resetCount++;
		} else {
                        ptpClock->listenCount++;
                        if( ptpClock->listenCount >= rtOpts->maxListen ) {
                            WARNING("Still in LISTENING after %d restarts - restarting transports\n",
				    rtOpts->maxListen);
                            toState(PTP_FAULTY, rtOpts, ptpClock);
                            ptpClock->listenCount = 0;
                            break;
                        }
                }

		/* Revert to the original DelayReq interval, and ignore the one for the last master */
		ptpClock->logMinDelayReqInterval = rtOpts->initial_delayreq;

		/* force a IGMP refresh per reset */
		if (rtOpts->ipMode != IPMODE_UNICAST && rtOpts->do_IGMP_refresh && rtOpts->transport != IEEE_802_3) {
		    /* if multicast refresh failed, restart network - helps recover after driver reloads and such */
                    if(!netRefreshIGMP(&ptpClock->netPath, rtOpts, ptpClock)) {
                            WARNING("Error while refreshing multicast - restarting transports\n");
                            toState(PTP_FAULTY, rtOpts, ptpClock);
                            break;
                    }
		}
		
			timerStart(&ptpClock->timers[ANNOUNCE_RECEIPT_TIMER], 
				(ptpClock->announceReceiptTimeout) * 
				(pow(2,ptpClock->logAnnounceInterval)));
				ptpClock->announceTimeouts = 0;

		ptpClock->masterAddr = 0;

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
#ifndef PTPD_SLAVE_ONLY
	case PTP_MASTER:
		if(rtOpts->unicastNegotiation) {
		    timerStart(&ptpClock->timers[UNICAST_GRANT_TIMER], 1);
		}
		timerStart(&ptpClock->timers[SYNC_INTERVAL_TIMER], 
			   pow(2,ptpClock->logSyncInterval));
		DBG("SYNC INTERVAL TIMER : %f \n",
		    pow(2,ptpClock->logSyncInterval));
		timerStart(&ptpClock->timers[ANNOUNCE_INTERVAL_TIMER], 
			   pow(2,ptpClock->logAnnounceInterval));
		timerStart(&ptpClock->timers[PDELAYREQ_INTERVAL_TIMER], 
			   pow(2,ptpClock->logMinPdelayReqInterval));
		if( rtOpts->do_IGMP_refresh &&
		    rtOpts->transport == UDP_IPV4 &&
		    rtOpts->ipMode != IPMODE_UNICAST &&
		    rtOpts->masterRefreshInterval > 9 )
			timerStart(&ptpClock->timers[MASTER_NETREFRESH_TIMER], 
			   rtOpts->masterRefreshInterval);
		ptpClock->portState = PTP_MASTER;
		displayStatus(ptpClock, "Now in state: ");

		ptpClock->masterAddr = 0;

		break;
#endif /* PTPD_SLAVE_ONLY */
	case PTP_PASSIVE:
		timerStart(&ptpClock->timers[PDELAYREQ_INTERVAL_TIMER], 
			   pow(2,ptpClock->logMinPdelayReqInterval));
		timerStart(&ptpClock->timers[ANNOUNCE_RECEIPT_TIMER], 
			   (ptpClock->announceReceiptTimeout) * 
			   (pow(2,ptpClock->logAnnounceInterval)));
		ptpClock->portState = PTP_PASSIVE;
		p1(ptpClock, rtOpts);
		displayStatus(ptpClock, "Now in state: ");
		break;

	case PTP_UNCALIBRATED:
		ptpClock->portState = PTP_UNCALIBRATED;
		break;

	case PTP_SLAVE:
		if(rtOpts->unicastNegotiation) {
		    timerStart(&ptpClock->timers[UNICAST_GRANT_TIMER], 1);
		}
		if(rtOpts->clockUpdateTimeout > 0) {
			timerStart(&ptpClock->timers[CLOCK_UPDATE_TIMER], rtOpts->clockUpdateTimeout);
		}
		initClock(rtOpts, ptpClock);
		/*
		 * restore the observed drift value using the selected method,
		 * reset on failure or when -F 0 (default) is used, don't inform user
		 */
		restoreDrift(ptpClock, rtOpts, TRUE);

		ptpClock->waitingForFollow = FALSE;
		ptpClock->waitingForDelayResp = FALSE;

		if(rtOpts->calibrationDelay) {
			ptpClock->isCalibrated = FALSE;
		}

		// FIXME: clear these vars inside initclock
		clearTime(&ptpClock->delay_req_send_time);
		clearTime(&ptpClock->delay_req_receive_time);
		clearTime(&ptpClock->pdelay_req_send_time);
		clearTime(&ptpClock->pdelay_req_receive_time);
		clearTime(&ptpClock->pdelay_resp_send_time);
		clearTime(&ptpClock->pdelay_resp_receive_time);
		
		timerStart(&ptpClock->timers[OPERATOR_MESSAGES_TIMER],
			   OPERATOR_MESSAGES_INTERVAL);
		
		timerStart(&ptpClock->timers[ANNOUNCE_RECEIPT_TIMER],
			   (ptpClock->announceReceiptTimeout) * 
			   (pow(2,ptpClock->logAnnounceInterval)));
		
		/*
		 * Previously, this state transition would start the
		 * delayreq timer immediately.  However, if this was
		 * faster than the first received sync, then the servo
		 * would drop the delayResp Now, we only start the
		 * timer after we receive the first sync (in
		 * handle_sync())
		 */
		ptpClock->syncWaiting = TRUE;
		ptpClock->delayRespWaiting = TRUE;
		ptpClock->announceTimeouts = 0;
		ptpClock->portState = PTP_SLAVE;
		displayStatus(ptpClock, "Now in state: ");
		ptpClock->followUpGap = 0;

#ifdef PTPD_STATISTICS
		if(rtOpts->oFilterMSConfig.enabled) {
			ptpClock->oFilterMS.reset(&ptpClock->oFilterMS);
		}
		if(rtOpts->oFilterSMConfig.enabled) {
			ptpClock->oFilterSM.reset(&ptpClock->oFilterSM);
		}
		if(rtOpts->filterMSOpts.enabled) {
			resetDoubleMovingStatFilter(ptpClock->filterMS);
		}
		if(rtOpts->filterSMOpts.enabled) {
			resetDoubleMovingStatFilter(ptpClock->filterSM);
		}
		clearPtpEngineSlaveStats(&ptpClock->slaveStats);
		ptpClock->servo.driftMean = 0;
		ptpClock->servo.driftStdDev = 0;
		ptpClock->servo.isStable = FALSE;
		ptpClock->servo.stableCount = 0;
		ptpClock->servo.updateCount = 0;
		ptpClock->servo.statsCalculated = FALSE;
		resetDoublePermanentStdDev(&ptpClock->servo.driftStats);
		timerStart(&ptpClock->timers[STATISTICS_UPDATE_TIMER], rtOpts->statsUpdateInterval);
#endif /* PTPD_STATISTICS */
		break;
	default:
		DBG("to unrecognized state\n");
		break;
	}

	if (rtOpts->logStatistics)
		logStatistics(ptpClock);
}


Boolean 
doInit(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	char filterMask[200];

	DBG("manufacturerIdentity: %s\n", MANUFACTURER_ID);
	DBG("manufacturerOUI: %02hhx:%02hhx:%02hhx \n",
		MANUFACTURER_ID_OUI0,
		MANUFACTURER_ID_OUI1,
		MANUFACTURER_ID_OUI2);
	/* initialize networking */
	netShutdown(&ptpClock->netPath);

	if(rtOpts->backupIfaceEnabled &&
		ptpClock->runningBackupInterface) {
		rtOpts->ifaceName = rtOpts->backupIfaceName;
	} else {
		rtOpts->ifaceName = rtOpts->primaryIfaceName;
	}

	if (!netInit(&ptpClock->netPath, rtOpts, ptpClock)) {
		ERROR("Failed to initialize network\n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return FALSE;
	}

	strncpy(filterMask,FILTER_MASK,199);

	/* initialize other stuff */
	initData(rtOpts, ptpClock);
	initClock(rtOpts, ptpClock);
	setupPIservo(&ptpClock->servo, rtOpts);
	/* restore observed drift and inform user */
	if(ptpClock->clockQuality.clockClass > 127)
		restoreDrift(ptpClock, rtOpts, FALSE);
	m1(rtOpts, ptpClock );
	msgPackHeader(ptpClock->msgObuf, ptpClock);
	
	toState(PTP_LISTENING, rtOpts, ptpClock);

	if(rtOpts->statusLog.logEnabled)
		writeStatusFile(ptpClock, rtOpts, TRUE);


	return TRUE;
}

/* handle actions and events for 'port_state' */
static void 
doState(const RunTimeOpts *rtOpts, PtpClock *ptpClock)
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
		if (timerExpired(&ptpClock->timers[ANNOUNCE_RECEIPT_TIMER]))
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
				timerStop(&ptpClock->timers[STATISTICS_UPDATE_TIMER]);
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

				if (rtOpts->ipMode != IPMODE_UNICAST && rtOpts->do_IGMP_refresh && rtOpts->transport != IEEE_802_3) {
				/* if multicast refresh failed, restart network - helps recover after driver reloads and such */
                		    if(!netRefreshIGMP(&ptpClock->netPath, rtOpts, ptpClock)) {
                        		WARNING("Error while refreshing multicast - restarting transports\n");
                        		toState(PTP_FAULTY, rtOpts, ptpClock);
                        		break;
                		    }
				}

				if (rtOpts->announceTimeoutGracePeriod > 0) ptpClock->announceTimeouts++;

					INFO("Waiting for new master, %d of %d attempts\n",ptpClock->announceTimeouts,rtOpts->announceTimeoutGracePeriod);
				} else {
					WARNING("No active masters present. Resetting port.\n");
					ptpClock->number_foreign_records = 0;
					ptpClock->foreign_record_i = 0;

					/* if flipping between primary and backup interface, a full nework re-init is required */
					if(rtOpts->backupIfaceEnabled) {
						ptpClock->runningBackupInterface = !ptpClock->runningBackupInterface;
						toState(PTP_INITIALIZING, rtOpts, ptpClock);
						NOTICE("Now switching to %s interface\n", ptpClock->runningBackupInterface ? 
							    "backup":"primary");
					    } else {

						toState(PTP_LISTENING, rtOpts, ptpClock);
					    }

					}
			} else {

				    /* if flipping between primary and backup interface, a full nework re-init is required */
				    if(rtOpts->backupIfaceEnabled) {
					ptpClock->runningBackupInterface = !ptpClock->runningBackupInterface;
					toState(PTP_INITIALIZING, rtOpts, ptpClock);
					NOTICE("Now switching to %s interface\n", ptpClock->runningBackupInterface ? 
						"backup":"primary");

				    } else {
				/*
				 *  Force a reset when getting a timeout in state listening, that will lead to an IGMP reset
				 *  previously this was not the case when we were already in LISTENING mode
				 */
				    toState(PTP_LISTENING, rtOpts, ptpClock);
				    }
                                }


                }

		/* Reset the slave if clock update timeout configured */
		if ( ptpClock->portState == PTP_SLAVE && (rtOpts->clockUpdateTimeout > 0) &&
		    timerExpired(&ptpClock->timers[CLOCK_UPDATE_TIMER])) {
			if(ptpClock->panicMode || rtOpts->noAdjust) {
				timerStart(&ptpClock->timers[CLOCK_UPDATE_TIMER], rtOpts->clockUpdateTimeout);
			} else {
			    WARNING("No offset updates in %d seconds - resetting slave\n",
				rtOpts->clockUpdateTimeout);
			    toState(PTP_LISTENING, rtOpts, ptpClock);
			}
		}

		if (timerExpired(&ptpClock->timers[OPERATOR_MESSAGES_TIMER])) {
			resetWarnings(rtOpts, ptpClock);
		}

		if(ptpClock->portState==PTP_SLAVE && rtOpts->calibrationDelay && !ptpClock->isCalibrated) {
			if(timerExpired(&ptpClock->timers[CALIBRATION_DELAY_TIMER])) {
				ptpClock->isCalibrated = TRUE;
				if(ptpClock->clockControl.granted) {
					NOTICE("Offset computation now calibrated, enabled clock control\n");
				} else {
					NOTICE("Offset computation now calibrated\n");
				}
			} else if(!timerRunning(&ptpClock->timers[CALIBRATION_DELAY_TIMER])) {
			    timerStart(&ptpClock->timers[CALIBRATION_DELAY_TIMER], rtOpts->calibrationDelay);
			}
		}

		if (ptpClock->delayMechanism == E2E) {
			if(timerExpired(&ptpClock->timers[DELAYREQ_INTERVAL_TIMER])) {
				DBG2("event DELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				/* if unicast negotiation is enabled, only request if granted */
				if(!rtOpts->unicastNegotiation || 
					(ptpClock->parentGrants && 
					    ptpClock->parentGrants->grantData[DELAY_RESP].granted)) {
						issueDelayReq(rtOpts,ptpClock);
				}
			}
		} else if (ptpClock->delayMechanism == P2P) {
			if (timerExpired(&ptpClock->timers[PDELAYREQ_INTERVAL_TIMER])) {
				DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				/* if unicast negotiation is enabled, only request if granted */
				if(!rtOpts->unicastNegotiation || 
					( ptpClock->peerGrants.grantData[PDELAY_RESP].granted)) {
					    issuePdelayReq(rtOpts,ptpClock);
				}
			}

			/* FIXME: Path delay should also rearm its timer with the value received from the Master */
		}

                if (ptpClock->timePropertiesDS.leap59 || ptpClock->timePropertiesDS.leap61) 
                        DBGV("seconds to midnight: %.3f\n",secondsToMidnight());

                /* leap second period is over */
                if(timerExpired(&ptpClock->timers[LEAP_SECOND_PAUSE_TIMER]) &&
                    ptpClock->leapSecondInProgress) {
                            /* 
                             * do not unpause offset calculation just
                             * yet, just indicate and it will be
                             * unpaused in handleAnnounce()
                            */
                            ptpClock->leapSecondPending = FALSE;
                            timerStop(&ptpClock->timers[LEAP_SECOND_PAUSE_TIMER]);
                    } 
		/* check if leap second is near and if we should pause updates */
		if( ptpClock->leapSecondPending &&
		    !ptpClock->leapSecondInProgress &&
		    (secondsToMidnight() <= 
		    getPauseAfterMidnight(ptpClock->logAnnounceInterval, 
			rtOpts->leapSecondPausePeriod))) {
                            WARNING("Leap second event imminent - pausing "
				    "clock and offset updates\n");
                            ptpClock->leapSecondInProgress = TRUE;
			    /*
			     * start pause timer from now until [pause] after
			     * midnight, plus an extra second if inserting
			     * a leap second
			     */
			    timerStart(&ptpClock->timers[LEAP_SECOND_PAUSE_TIMER],
				       ((secondsToMidnight() + 
				       (int)ptpClock->timePropertiesDS.leap61) + 
				       getPauseAfterMidnight(ptpClock->logAnnounceInterval,
					    rtOpts->leapSecondPausePeriod)));
		}

/* Update PTP slave statistics from online statistics containers */
#ifdef PTPD_STATISTICS
		if (timerExpired(&ptpClock->timers[STATISTICS_UPDATE_TIMER])) {
			if(!rtOpts->enablePanicMode || !ptpClock->panicMode)
				updatePtpEngineStats(ptpClock, rtOpts);
		}
#endif /* PTPD_STATISTICS */

		break;
#ifndef PTPD_SLAVE_ONLY
	case PTP_MASTER:
		/*
		 * handle MASTER timers:
		 *   - Time to send new Announce(s)
		 *   - Time to send new PathDelay
		 *   - Time to send new Sync(s) (last order - so that follow-up always follows sync
		 *     in two-step mode: improves interoperability
		 *      (DelayResp has no timer - as these are sent and retransmitted by the slaves)
		 */

		/* master leap second triggers */

		/* if we have an offset from some source, we assume it's valid */
		if(ptpClock->clockStatus.utcOffset != 0) {
			ptpClock->timePropertiesDS.currentUtcOffset = ptpClock->clockStatus.utcOffset;
			ptpClock->timePropertiesDS.currentUtcOffsetValid = TRUE;
	
		}

		/* update the tpDS with clockStatus leap flags - only if running PTP timescale */
		if(ptpClock->timePropertiesDS.ptpTimescale &&
		    (secondsToMidnight() < rtOpts->leapSecondNoticePeriod)) {
			ptpClock->timePropertiesDS.leap59 = ptpClock->clockStatus.leapDelete;
			ptpClock->timePropertiesDS.leap61 = ptpClock->clockStatus.leapInsert;
		} else {
		    ptpClock->timePropertiesDS.leap59 = FALSE;
		    ptpClock->timePropertiesDS.leap61 = FALSE;
		}

		if(ptpClock->timePropertiesDS.leap59 ||
		    ptpClock->timePropertiesDS.leap61 ) {
		    if(!ptpClock->leapSecondInProgress) {
			ptpClock->leapSecondPending = TRUE;
		    }
                    DBGV("seconds to midnight: %.3f\n",secondsToMidnight());
		}

		/* check if leap second is near and if we should pause updates */
		if( ptpClock->leapSecondPending &&
		    !ptpClock->leapSecondInProgress &&
		    (secondsToMidnight() <= 
		    getPauseAfterMidnight(ptpClock->logAnnounceInterval,
			rtOpts->leapSecondPausePeriod))) {
                            WARNING("Leap second event imminent - pausing "
				    "event message processing\n");
                            ptpClock->leapSecondInProgress = TRUE;
			    /*
			     * start pause timer from now until [pause] after
			     * midnight, plus an extra second if inserting
			     * a leap second
			     */
			    timerStart(&ptpClock->timers[LEAP_SECOND_PAUSE_TIMER],
				       ((secondsToMidnight() + 
				       (int)ptpClock->timePropertiesDS.leap61) + 
				       getPauseAfterMidnight(ptpClock->logAnnounceInterval,
					    rtOpts->leapSecondPausePeriod)));
		}

                /* leap second period is over */
                if(timerExpired(&ptpClock->timers[LEAP_SECOND_PAUSE_TIMER]) &&
                    ptpClock->leapSecondInProgress) {
                            ptpClock->leapSecondPending = FALSE;
                            timerStop(&ptpClock->timers[LEAP_SECOND_PAUSE_TIMER]);
			    ptpClock->leapSecondInProgress = FALSE;
                            NOTICE("Leap second event over - resuming "
				    "event message processing\n");
                } 


		if (timerExpired(&ptpClock->timers[ANNOUNCE_INTERVAL_TIMER])) {
			DBGV("event ANNOUNCE_INTERVAL_TIMEOUT_EXPIRES\n");
			/* restart the timer with current interval in case if it changed */
			if(pow(2,ptpClock->logAnnounceInterval) != ptpClock->timers[ANNOUNCE_INTERVAL_TIMER].interval) {
				timerStart(&ptpClock->timers[ANNOUNCE_INTERVAL_TIMER], 
					pow(2,ptpClock->logAnnounceInterval));
			}
			issueAnnounce(rtOpts, ptpClock);

		}

		if (ptpClock->delayMechanism == P2P) {
			if (timerExpired(&ptpClock->timers[PDELAYREQ_INTERVAL_TIMER])) {
				DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				/* if unicast negotiation is enabled, only request if granted */
				if(!rtOpts->unicastNegotiation || 
					( ptpClock->peerGrants.grantData[PDELAY_RESP].granted)) {
					    issuePdelayReq(rtOpts,ptpClock);
				}
			}
		}

		if(rtOpts->do_IGMP_refresh &&
		    rtOpts->transport == UDP_IPV4 &&
		    rtOpts->ipMode != IPMODE_UNICAST &&
		    rtOpts->masterRefreshInterval > 9 &&
		    timerExpired(&ptpClock->timers[MASTER_NETREFRESH_TIMER])) {
				DBGV("Master state periodic IGMP refresh - next in %d seconds...\n",
				rtOpts->masterRefreshInterval);
				/* if multicast refresh failed, restart network - helps recover after driver reloads and such */
                		    if(!netRefreshIGMP(&ptpClock->netPath, rtOpts, ptpClock)) {
                        		WARNING("Error while refreshing multicast - restarting transports\n");
                        		toState(PTP_FAULTY, rtOpts, ptpClock);
                        		break;
                		    }
		}

		if (timerExpired(&ptpClock->timers[SYNC_INTERVAL_TIMER])) {
			DBGV("event SYNC_INTERVAL_TIMEOUT_EXPIRES\n");
			/* re-arm timer if changed */
			if(pow(2,ptpClock->logSyncInterval) != ptpClock->timers[SYNC_INTERVAL_TIMER].interval) {
				timerStart(&ptpClock->timers[SYNC_INTERVAL_TIMER], 
					pow(2,ptpClock->logSyncInterval));
			}

			issueSync(rtOpts, ptpClock);
		}
		if(!ptpClock->warnedUnicastCapacity) {
		    if(ptpClock->slaveCount >= UNICAST_MAX_DESTINATIONS || 
			ptpClock->unicastDestinationCount >= UNICAST_MAX_DESTINATIONS) {
			    if(rtOpts->ipMode == IPMODE_UNICAST) {
				WARNING("Maximum unicast slave capacity reached: %d\n",UNICAST_MAX_DESTINATIONS);
				ptpClock->warnedUnicastCapacity = TRUE;
			    }
		    }
		}

		if (timerExpired(&ptpClock->timers[OPERATOR_MESSAGES_TIMER])) {
			resetWarnings(rtOpts, ptpClock);
		}

		// TODO: why is handle() below expiretimer, while in slave is the opposite
		handle(rtOpts, ptpClock);

		if (ptpClock->slaveOnly || ptpClock->clockQuality.clockClass == SLAVE_ONLY_CLOCK_CLASS)
			toState(PTP_LISTENING, rtOpts, ptpClock);

		break;
#endif /* PTPD_SLAVE_ONLY */

	case PTP_DISABLED:
		handle(rtOpts, ptpClock);
		break;
		
	default:
		DBG("(doState) do unrecognized state\n");
		break;
	}

	if(rtOpts->periodicUpdates && timerExpired(&ptpClock->timers[PERIODIC_INFO_TIMER])) {
		periodicUpdate(rtOpts, ptpClock);
	}

        if(rtOpts->statusLog.logEnabled && timerExpired(&ptpClock->timers[STATUSFILE_UPDATE_TIMER])) {
                writeStatusFile(ptpClock,rtOpts,TRUE);
		/* ensures that the current updare interval is used */
		timerStart(&ptpClock->timers[STATUSFILE_UPDATE_TIMER],rtOpts->statusFileUpdateInterval);
        }

	if(rtOpts->enablePanicMode && timerExpired(&ptpClock->timers[PANIC_MODE_TIMER])) {

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

/* apply any corrections and manglings required to the timestamp */
static void
timestampCorrection(const RunTimeOpts * rtOpts, PtpClock *ptpClock, TimeInternal *timeStamp)
{

	TimeInternal fudge = {0,0};
	if(rtOpts->leapSecondHandling == LEAP_SMEAR && (ptpClock->leapSecondPending)) {
	DBG("Leap second smear: correction %.09f ns, seconds to midnight %f, leap smear period %d\n", ptpClock->leapSmearFudge,
		secondsToMidnight(), rtOpts->leapSecondSmearPeriod);
	    if(secondsToMidnight() <= rtOpts->leapSecondSmearPeriod) {
		ptpClock->leapSmearFudge = 1.0 - (secondsToMidnight() + 0.0) / (rtOpts->leapSecondSmearPeriod+0.0);
		if(ptpClock->timePropertiesDS.leap59) {
		    ptpClock->leapSmearFudge *= -1;
		}
		fudge = doubleToTimeInternal(ptpClock->leapSmearFudge);
	    }
	} 

	if(ptpClock->portState == PTP_SLAVE && ptpClock->leapSecondPending && !ptpClock->leapSecondInProgress) {
	    addTime(timeStamp, timeStamp, &fudge);
	}

}


void
processMessage(const RunTimeOpts* rtOpts, PtpClock* ptpClock, TimeInternal* timeStamp, ssize_t length)
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
    if(ptpClock->netPath.lastSourceAddr && 
	(ptpClock->netPath.lastSourceAddr != ptpClock->netPath.interfaceAddr.s_addr)) {
#ifdef RUNTIME_DEBUG
		struct in_addr tmpAddr;
		tmpAddr.s_addr = ptpClock->netPath.lastSourceAddr;
#endif /* RUNTIME_DEBUG */
		if(ptpClock->msgTmpHeader.messageType == MANAGEMENT) {
			if(rtOpts->managementAclEnabled) {
			    if (!matchIpv4AccessList(
				ptpClock->netPath.managementAcl, 
				ntohl(ptpClock->netPath.lastSourceAddr))) {
					DBG("ACL dropped management message from %s\n", inet_ntoa(tmpAddr));
					ptpClock->counters.aclManagementDiscardedMessages++;
					return;
				} else
					DBG("ACL Accepted management message from %s\n", inet_ntoa(tmpAddr));
			}
	        } else if(rtOpts->timingAclEnabled) {
			if(!matchIpv4AccessList(ptpClock->netPath.timingAcl, 
			    ntohl(ptpClock->netPath.lastSourceAddr))) {
				DBG("ACL dropped timing message from %s\n", inet_ntoa(tmpAddr));
				ptpClock->counters.aclTimingDiscardedMessages++;
				return;
			} else
				DBG("ACL accepted timing message from %s\n", inet_ntoa(tmpAddr));
		}
    }

    if (ptpClock->msgTmpHeader.versionPTP != ptpClock->versionNumber) {
	DBG("ignore version %d message\n", ptpClock->msgTmpHeader.versionPTP);
	ptpClock->counters.discardedMessages++;
	ptpClock->counters.versionMismatchErrors++;
	return;
    }

    if(ptpClock->msgTmpHeader.domainNumber != ptpClock->domainNumber) {
	Boolean domainOK = FALSE;
	int i = 0;
	if (rtOpts->unicastNegotiation && ptpClock->unicastDestinationCount) {
	    for (i = 0; i < ptpClock->unicastDestinationCount; i++) {
		if(ptpClock->msgTmpHeader.domainNumber == ptpClock->unicastGrants[i].domainNumber) {
		    domainOK = TRUE;
		    DBG("Accepted message type %s from domain %d (unicast neg)\n", 
			getMessageTypeName(ptpClock->msgTmpHeader.messageType),ptpClock->msgTmpHeader.domainNumber);
		    break;
		}
	    }
	}
	if(ptpClock->slaveOnly && rtOpts->anyDomain) {
		DBG("anyDomain enabled: accepting announce from domain %d (we are %d)\n", 
			ptpClock->msgTmpHeader.domainNumber,
			ptpClock->domainNumber
			);
	} else if(!domainOK) {
		DBG("Ignored message %s received from %d domain\n", 
			getMessageTypeName(ptpClock->msgTmpHeader.messageType),
			ptpClock->msgTmpHeader.domainNumber);
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.domainMismatchErrors++;
		return;
	}
    }


    if(rtOpts->transport != IEEE_802_3) {

	/* received a UNICAST message */
        if((ptpClock->msgTmpHeader.flagField0 & PTP_UNICAST) == PTP_UNICAST) {
    	/* in multicast mode, accept only management unicast messages, in hybrid mode accept only unicast delay messages */
    	    if((rtOpts->ipMode == IPMODE_MULTICAST && ptpClock->msgTmpHeader.messageType != MANAGEMENT) ||
    		(rtOpts->ipMode == IPMODE_HYBRID && ptpClock->msgTmpHeader.messageType != DELAY_REQ && 
		    ptpClock->msgTmpHeader.messageType != DELAY_RESP)) {
			DBG("ignord unicast message in multicast mode%d\n");
			ptpClock->counters.discardedMessages++;
			return;
	    }
	    /* received a MULTICAST message */
	} else {
	/* in unicast mode, accept only management multicast messages */
		if(rtOpts->ipMode == IPMODE_UNICAST && ptpClock->msgTmpHeader.messageType != MANAGEMENT) {
		    DBG("ignord multicast message in unicast mode%d\n");
		    ptpClock->counters.discardedMessages++;
		    return;
		}
	}

    }

    /* what shall we do with the drunken sailor? */
    timestampCorrection(rtOpts, ptpClock, timeStamp);

    /*Spec 9.5.2.2*/
    isFromSelf = !cmpPortIdentity(&ptpClock->portIdentity, &ptpClock->msgTmpHeader.sourcePortIdentity);

    /*
     * subtract the inbound latency adjustment if it is not a loop
     *  back and the time stamp seems reasonable 
     */
    if (!isFromSelf && timeStamp->seconds > 0)
	subTime(timeStamp, timeStamp, &rtOpts->inboundLatency);

    DBG("      ==> %s message received, sequence %d\n", getMessageTypeName(ptpClock->msgTmpHeader.messageType),
							ptpClock->msgTmpHeader.sequenceId);

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
	       length, timeStamp, isFromSelf, ptpClock->netPath.lastSourceAddr, ptpClock->netPath.lastDestAddr, rtOpts, ptpClock);
	break;
    case FOLLOW_UP:
	handleFollowUp(&ptpClock->msgTmpHeader,
	           length, isFromSelf, rtOpts, ptpClock);
	break;
    case DELAY_REQ:
	handleDelayReq(&ptpClock->msgTmpHeader,
	           length, timeStamp, ptpClock->netPath.lastSourceAddr, isFromSelf, rtOpts, ptpClock);
	break;
    case PDELAY_REQ:
	handlePdelayReq(&ptpClock->msgTmpHeader,
		length, timeStamp, ptpClock->netPath.lastSourceAddr, isFromSelf, rtOpts, ptpClock);
	break;  
    case DELAY_RESP:
	handleDelayResp(&ptpClock->msgTmpHeader,
		length, rtOpts, ptpClock);
	break;
    case PDELAY_RESP:
	handlePdelayResp(&ptpClock->msgTmpHeader,
		 timeStamp, length, isFromSelf, ptpClock->netPath.lastSourceAddr, ptpClock->netPath.lastDestAddr, rtOpts, ptpClock);
	break;
    case PDELAY_RESP_FOLLOW_UP:
	handlePdelayRespFollowUp(&ptpClock->msgTmpHeader, 
		     length, isFromSelf, rtOpts, ptpClock);
	break;
    case MANAGEMENT:
	handleManagement(&ptpClock->msgTmpHeader,
		 isFromSelf, rtOpts, ptpClock);
	break;
    case SIGNALING:
       handleSignaling(&ptpClock->msgTmpHeader, isFromSelf,
                ptpClock->netPath.lastSourceAddr, rtOpts, ptpClock);
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
handle(const RunTimeOpts *rtOpts, PtpClock *ptpClock)
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

#ifdef PTPD_PCAP
    if (rtOpts->pcap == TRUE) {
	if (ptpClock->netPath.pcapEventSock >=0 && FD_ISSET(ptpClock->netPath.pcapEventSock, &readfds)) {
	    length = netRecvEvent(ptpClock->msgIbuf, &timeStamp, 
		          &ptpClock->netPath,0);
	    if (length == 0){ /* timeout, return for now */
		return;
		}


	    if (length < 0) {
		PERROR("failed to receive event on pcap");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		ptpClock->counters.messageRecvErrors++;

		return;
	    }
	    if(ptpClock->leapSecondInProgress) {
		DBG("Leap second in progress - will not process event message\n");
	    } else {

		processMessage(rtOpts, ptpClock, &timeStamp, length);
	    }
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
#endif
	if (FD_ISSET(ptpClock->netPath.eventSock, &readfds)) {
	    length = netRecvEvent(ptpClock->msgIbuf, &timeStamp, 
		          &ptpClock->netPath, 0);
	    if (length < 0) {
		PERROR("failed to receive on the event socket");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		ptpClock->counters.messageRecvErrors++;
		return;
	    }
	    if(ptpClock->leapSecondInProgress) {
		DBG("Leap second in progress - will not process event message\n");
	    } else {
		processMessage(rtOpts, ptpClock, &timeStamp, length);
	    }
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
#ifdef PTPD_PCAP
    }
#endif

}

/*spec 9.5.3*/
static void 
handleAnnounce(MsgHeader *header, ssize_t length, 
	       Boolean isFromSelf, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

	UnicastGrantTable *nodeTable = NULL;
	UInteger8 localPreference = 0;

	DBGV("HandleAnnounce : Announce message received : \n");

	if(length < ANNOUNCE_LENGTH) {
		DBG("Error: Announce message too short\n");
		ptpClock->counters.messageFormatErrors++;
		return;
	}

	/* if we're ignoring announces (telecom) */
	if(ptpClock->clockQuality.clockClass <= 127 && rtOpts->disableBMCA) {
		DBG("unicast master only and disableBMCA: dropping Announce\n");
		ptpClock->counters.discardedMessages++;
		return;
	}

	if(rtOpts->unicastNegotiation && rtOpts->ipMode == IPMODE_UNICAST) {

		nodeTable = findUnicastGrants(&header->sourcePortIdentity, 0, 
							ptpClock->unicastGrants, ptpClock->unicastGrantIndex, UNICAST_MAX_DESTINATIONS,
							FALSE);
		if(nodeTable == NULL || !(nodeTable->grantData[ANNOUNCE].granted)) {
			DBG("Ignoring announce from master: unicast transmission not granted\n");
			ptpClock->counters.discardedMessages++;
			return;
		} else {
		    localPreference = nodeTable->localPreference;
		    nodeTable->grantData[ANNOUNCE].receiving = header->sequenceId;
		}
	}

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
				 * second) - make sure the upstream knows about this.
				*/
				if (!ptpClock->leapSecondPending) {
					WARNING("Leap second event over - "
						"resuming clock and offset updates\n");
					ptpClock->leapSecondInProgress = FALSE;
					if(rtOpts->leapSecondHandling==LEAP_STEP) {
						ptpClock->clockControl.stepRequired = TRUE;
					}
					/* reverse frequency shift */
					if(rtOpts->leapSecondHandling==LEAP_SMEAR){
					    /* the flags are probably off by now, using the shift sign to detect leap second type */
					    if(ptpClock->leapSmearFudge < 0) {
						DBG("Reversed LEAP59 smear frequency offset\n");
						ptpClock->servo.observedDrift += 1E9 / rtOpts->leapSecondSmearPeriod;
					    }
					    if(ptpClock->leapSmearFudge > 0) {
						DBG("Reversed LEAP61 smear frequency offset\n");
						ptpClock->servo.observedDrift -= 1E9 / rtOpts->leapSecondSmearPeriod;
					    }
					    ptpClock->leapSmearFudge = 0;
					}
					ptpClock->timePropertiesDS.leap59 = FALSE;
					ptpClock->timePropertiesDS.leap61 = FALSE;
					ptpClock->clockStatus.leapInsert = FALSE;
					ptpClock->clockStatus.leapDelete = FALSE;
					ptpClock->clockStatus.update = TRUE;
					
				}
			}

			DBG2("___ Announce: received Announce from current Master, so reset the Announce timer\n");

	   		/* Reset Timer handling Announce receipt timeout - in anyDomain, time out from current domain first */
			if(header->domainNumber == ptpClock->domainNumber) {
	   			timerStart(&ptpClock->timers[ANNOUNCE_RECEIPT_TIMER],
				   (ptpClock->announceReceiptTimeout) * 
				   (pow(2,ptpClock->logAnnounceInterval)));
			}
#ifdef PTPD_STATISTICS
		if(!timerRunning(&ptpClock->timers[STATISTICS_UPDATE_TIMER])) {
			timerStart(&ptpClock->timers[STATISTICS_UPDATE_TIMER], rtOpts->statsUpdateInterval);
		}
#endif /* PTPD_STATISTICS */

			if (rtOpts->announceTimeoutGracePeriod &&
				ptpClock->announceTimeouts > 0) {
					NOTICE("Received Announce message from master - cancelling timeout\n");
					ptpClock->announceTimeouts = 0;
					/* we are available for clock control again */
					ptpClock->clockControl.available = TRUE;
			}

			/* save the master address for display purposes or hybrid mode */
			ptpClock->masterAddr = ptpClock->netPath.lastSourceAddr;

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
			addForeign(ptpClock->msgIbuf,header,ptpClock,localPreference);
			break;

		default:
			DBG("HandleAnnounce : (isFromCurrentParent)"
			     "strange value ! \n");
	   		return;

		} /* switch on (isFromCurrentParent) */
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

			ptpClock->masterAddr = ptpClock->netPath.lastSourceAddr;

			DBG("___ Announce: received Announce from current Master, so reset the Announce timer\n\n");

			/* Reset Timer handling Announce receipt timeout: different domain my get here
			  when using anyDomain.*/
			timerStart(&ptpClock->timers[ANNOUNCE_RECEIPT_TIMER],
			   (ptpClock->announceReceiptTimeout) *
			   (pow(2,ptpClock->logAnnounceInterval)));

		} else {
			/*addForeign takes care of AnnounceUnpacking*/
			/* the actual decision to change masters is only done in  doState() / record_update == TRUE / bmc() */
			/* the original code always called: addforeign(new master) + timerstart(announce) */

			DBG("___ Announce: received Announce from another master, will add to the list, as it might be better\n\n");
			DBGV("this is to be decided immediatly by bmc())\n\n");
			addForeign(ptpClock->msgIbuf,header,ptpClock,localPreference);
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
		addForeign(ptpClock->msgIbuf,header,ptpClock, localPreference);
		ptpClock->record_update = TRUE;    /* run BMC() as soon as possible */
		break;

	} /* switch on (port_state) */
}


static void 
handleSync(const MsgHeader *header, ssize_t length, 
	   TimeInternal *tint, Boolean isFromSelf, Integer32 sourceAddress, Integer32 destinationAddress,
	   const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	TimeInternal OriginTimestamp;
	TimeInternal correctionField;

	Integer32 dst = 0;

	if(destinationAddress) {
	    dst = destinationAddress;
	} else {
	    dst = 0;
	}

	DBGV("Sync message received : \n");

	if (length < SYNC_LENGTH) {
		DBG("Error: Sync message too short\n");
		ptpClock->counters.messageFormatErrors++;
		return;
	}

	if(!isFromSelf && rtOpts->unicastNegotiation && rtOpts->ipMode == IPMODE_UNICAST) {
	    UnicastGrantTable *nodeTable = NULL;
	    nodeTable = findUnicastGrants(&header->sourcePortIdentity, 0, 
			ptpClock->unicastGrants, ptpClock->unicastGrantIndex, UNICAST_MAX_DESTINATIONS,
			FALSE);
	    if(nodeTable != NULL) {
		nodeTable->grantData[SYNC].receiving = header->sequenceId;
	    }
	}

	switch (ptpClock->portState) {
	case PTP_INITIALIZING:
	case PTP_FAULTY:
	case PTP_DISABLED:
	case PTP_PASSIVE:
	case PTP_LISTENING:
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
			if (ptpClock->syncWaiting) {
				ptpClock->syncWaiting = FALSE;


				NOTICE("Received first Sync from Master\n");

				if (ptpClock->delayMechanism == E2E)
					timerStart(&ptpClock->timers[DELAYREQ_INTERVAL_TIMER],
						   pow(2,ptpClock->logMinDelayReqInterval));
				else if (ptpClock->delayMechanism == P2P)
					timerStart(&ptpClock->timers[PDELAYREQ_INTERVAL_TIMER],
						   pow(2,ptpClock->logMinPdelayReqInterval));
			} else if ( rtOpts->syncSequenceChecking ) {
				if(  (ptpClock->counters.consecutiveSequenceErrors < MAX_SEQ_ERRORS) &&
					((((UInteger16)(ptpClock->recvSyncSequenceId + 32768)) >
					(header->sequenceId + 32767)) ||
				    (((UInteger16)(ptpClock->recvSyncSequenceId + 1)) >
					(header->sequenceId)))  )  {
					ptpClock->counters.discardedMessages++;
					ptpClock->counters.sequenceMismatchErrors++;
					ptpClock->counters.consecutiveSequenceErrors++;
					DBG("HandleSync : sequence mismatch - "
					    "received: %d, expected %d or greater, consecutive errors %d\n",
					    header->sequenceId,
					    (UInteger16)(ptpClock->recvSyncSequenceId + 1),
					    ptpClock->counters.consecutiveSequenceErrors
					    );
					break;
			    } else {
				    if(ptpClock->counters.consecutiveSequenceErrors >= MAX_SEQ_ERRORS) {
					    DBG("HandleSync: rejected %d out of order sequences, accepting current sync\n",
						    MAX_SEQ_ERRORS);
				    }
				    ptpClock->counters.consecutiveSequenceErrors = 0;
			    }
			}

			ptpClock->logSyncInterval = header->logMessageInterval;

			/* this will be 0x7F for unicast so if we have a grant, use the granted value */
			if(rtOpts->unicastNegotiation && ptpClock->parentGrants 
				&& ptpClock->parentGrants->grantData[SYNC].granted) {
				ptpClock->logSyncInterval =  ptpClock->parentGrants->grantData[SYNC].logInterval;
			}
			

			recordSync(header->sequenceId, tint);

			if ((header->flagField0 & PTP_TWO_STEP) == PTP_TWO_STEP) {
				DBG2("HandleSync: waiting for follow-up \n");
				ptpClock->twoStepFlag=TRUE;
				if(ptpClock->waitingForFollow) {
				    ptpClock->followUpGap++;
				    if(ptpClock->followUpGap < MAX_FOLLOWUP_GAP) {
					DBG("Received Sync while waiting for follow-up - "
					    "will wait for another %d messages\n",
					    MAX_FOLLOWUP_GAP - ptpClock->followUpGap);
					    break;		    
				    }
					DBG("No follow-up for %d sync - waiting for new follow-up\n",
					    ptpClock->followUpGap);
				}

				ptpClock->sync_receive_time.seconds = tint->seconds;
				ptpClock->sync_receive_time.nanoseconds = tint->nanoseconds;

				ptpClock->waitingForFollow = TRUE;
				/*Save correctionField of Sync message*/
				integer64_to_internalTime(
					header->correctionField,
					&correctionField);
				ptpClock->lastSyncCorrectionField.seconds = 
					correctionField.seconds;
				ptpClock->lastSyncCorrectionField.nanoseconds =
					correctionField.nanoseconds;
				ptpClock->recvSyncSequenceId = 
					header->sequenceId;
				break;
			} else {

				ptpClock->sync_receive_time.seconds = tint->seconds;
				ptpClock->sync_receive_time.nanoseconds = tint->nanoseconds;

				ptpClock->recvSyncSequenceId = 
					header->sequenceId;
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
				checkOffset(rtOpts,ptpClock);
				if (ptpClock->clockControl.updateOK) {
					ptpClock->acceptedUpdates++;
					updateClock(rtOpts,ptpClock);
				}
				ptpClock->offsetUpdates++;
				
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
			DBGV("HandleSync: going to send followup message\n");

			/* who do we send the followUp to? no destination given - try looking up index */
			if((rtOpts->ipMode == IPMODE_UNICAST) && !dst) {
				msgUnpackSync(ptpClock->msgIbuf,
					      &ptpClock->msgTmp.sync);
				toInternalTime(&OriginTimestamp, &ptpClock->msgTmp.sync.originTimestamp);
			    dst = lookupSyncIndex(&OriginTimestamp, header->sequenceId, ptpClock->syncDestIndex);

#ifdef RUNTIME_DEBUG
			    {
				struct in_addr tmpAddr;
				tmpAddr.s_addr = dst;
				DBG("handleSync: master sync dest cache hit: %s\n", inet_ntoa(tmpAddr));
			    }
#endif /* RUNTIME_DEBUG */

			    if(!dst) {
				DBG("handleSync: master sync dest cache miss - searching\n");
				dst = findSyncDestination(&OriginTimestamp, rtOpts, ptpClock);
				/* give up. Better than sending FollowUp to random destinations*/
				if(!dst) {
				    DBG("handleSync: master sync dest not found for followUp. Giving up.\n");
				    return;
				} else {
				    DBG("unicast destination found.\n");
				}
			    }

			}


#ifndef PTPD_SLAVE_ONLY /* does not get compiled when building slave only */
			processSyncFromSelf(tint, rtOpts, ptpClock, dst, header->sequenceId);
#endif /* PTPD_SLAVE_ONLY */
			break;
		} else {
			DBGV("HandleSync: Sync message received from self\n");
		}
	}
}

#ifndef PTPD_SLAVE_ONLY /* does not get compiled when building slave only */
static void
processSyncFromSelf(const TimeInternal * tint, const RunTimeOpts * rtOpts, PtpClock * ptpClock, Integer32 dst, const UInteger16 sequenceId) {
	TimeInternal timestamp;
	/*Add latency*/
	addTime(&timestamp, tint, &rtOpts->outboundLatency);
	/* Issue follow-up CORRESPONDING TO THIS SYNC */
	issueFollowup(&timestamp, rtOpts, ptpClock, dst, sequenceId);
}
#endif /* PTPD_SLAVE_ONLY */

static void 
handleFollowUp(const MsgHeader *header, ssize_t length, 
	       Boolean isFromSelf, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	TimeInternal preciseOriginTimestamp;
	TimeInternal correctionField;

	DBGV("Handlefollowup : Follow up message received \n");

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
			/* this will be 0x7F for unicast so if we have a grant, use the granted value */
			if(rtOpts->unicastNegotiation && ptpClock->parentGrants 
				&& ptpClock->parentGrants->grantData[SYNC].granted) {
				ptpClock->logSyncInterval =  ptpClock->parentGrants->grantData[SYNC].logInterval;
			}

			if (ptpClock->waitingForFollow)	{
				if (ptpClock->recvSyncSequenceId == 
				     header->sequenceId) {
					ptpClock->followUpGap = 0;
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
					checkOffset(rtOpts,ptpClock);
					if (ptpClock->clockControl.updateOK) {
						ptpClock->acceptedUpdates++;
						updateClock(rtOpts,ptpClock);
					}
					ptpClock->offsetUpdates++;

					break;
				} else {
					    DBG("HandleFollowUp : sequence mismatch - "
					    "last Sync: %d, this FollowUp: %d\n",
					    ptpClock->recvSyncSequenceId,
					    header->sequenceId);
					    ptpClock->counters.sequenceMismatchErrors++;
					    ptpClock->counters.discardedMessages++;
					}
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
	       const TimeInternal *tint, Integer32 sourceAddress, Boolean isFromSelf,
	       const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

	UnicastGrantTable *nodeTable = NULL;

	if (ptpClock->delayMechanism == E2E) {

		if(!isFromSelf && rtOpts->unicastNegotiation && rtOpts->ipMode == IPMODE_UNICAST) {
		    nodeTable = findUnicastGrants(&header->sourcePortIdentity, 0,
				ptpClock->unicastGrants, ptpClock->unicastGrantIndex, UNICAST_MAX_DESTINATIONS,
				FALSE);
		    if(nodeTable == NULL || !(nodeTable->grantData[DELAY_RESP].granted)) {
			DBG("Ignoring Delay Request from slave: unicast transmission not granted\n");
			ptpClock->counters.discardedMessages++;
			    return;
		    } else {
			nodeTable->grantData[DELAY_REQ].receiving = header->sequenceId;
		    }

		}
	

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
			ptpClock->LastSlaveAddr = ptpClock->netPath.lastSourceAddr;

			issueDelayResp(tint,&ptpClock->delayReqHeader, sourceAddress,
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
processDelayReqFromSelf(const TimeInternal * tint, const RunTimeOpts * rtOpts, PtpClock * ptpClock) {

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
		const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

	if (ptpClock->delayMechanism == E2E) {


		TimeInternal requestReceiptTimestamp;
		TimeInternal correctionField;

		if(rtOpts->unicastNegotiation && rtOpts->ipMode == IPMODE_UNICAST) {
		    UnicastGrantTable *nodeTable = NULL;
		    nodeTable = findUnicastGrants(&header->sourcePortIdentity, 0, 
				ptpClock->unicastGrants, ptpClock->unicastGrantIndex, UNICAST_MAX_DESTINATIONS,
				FALSE);
		    if(nodeTable != NULL) {
			nodeTable->grantData[DELAY_RESP].receiving = header->sequenceId;
		    }
		}

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
					DBG("Ignored DelayResp sequence %d - wasn't waiting for one\n",
						header->sequenceId);
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
				if (ptpClock->delayRespWaiting) {

					NOTICE("Received first Delay Response from Master\n");
				}

				if (rtOpts->ignore_delayreq_interval_master == 0) {
					DBGV("current delay_req: %d  new delay req: %d \n",
						ptpClock->logMinDelayReqInterval,
					header->logMessageInterval);
                        	    if (ptpClock->logMinDelayReqInterval != header->logMessageInterval) {
			
                    			if(header->logMessageInterval == UNICAST_MESSAGEINTERVAL &&
						rtOpts->autoDelayReqInterval) {

						if(rtOpts->unicastNegotiation && ptpClock->parentGrants && ptpClock->parentGrants->grantData[DELAY_RESP].granted) {
						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Delay Interval %d from master\n",
							    ptpClock->parentGrants->grantData[DELAY_RESP].logInterval);
						    }
						    ptpClock->logMinDelayReqInterval = ptpClock->parentGrants->grantData[DELAY_RESP].logInterval;
						} else {

						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Delay Interval %d from master (unicast-unknown) - overriding with %d\n",
							    header->logMessageInterval, rtOpts->logMinDelayReqInterval);
						    }
						    ptpClock->logMinDelayReqInterval = rtOpts->logMinDelayReqInterval;

						}	
					} else {
						/* Accept new DelayReq value from the Master */

						    NOTICE("Received new Delay Request interval %d from Master (was: %d)\n",
							     header->logMessageInterval, ptpClock->logMinDelayReqInterval );

						    // collect new value indicated by the Master
						    ptpClock->logMinDelayReqInterval = header->logMessageInterval;
						}
					/* FIXME: the actual rearming of this timer with the new value only happens later in doState()/issueDelayReq() */
				    }
				} else {

					if (ptpClock->logMinDelayReqInterval != rtOpts->logMinDelayReqInterval) {
						INFO("New Delay Request interval applied: %d (was: %d)\n",
							rtOpts->logMinDelayReqInterval, ptpClock->logMinDelayReqInterval);
					}
					ptpClock->logMinDelayReqInterval = rtOpts->logMinDelayReqInterval;
				}
			} else {

				DBG("HandledelayResp : delayResp doesn't match with the delayReq. \n");
				ptpClock->counters.discardedMessages++;
				break;
			}
			
			ptpClock->delayRespWaiting = FALSE;
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
handlePdelayReq(MsgHeader *header, ssize_t length, 
		const TimeInternal *tint, Integer32 sourceAddress, Boolean isFromSelf, 
		const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

	UnicastGrantTable *nodeTable = NULL;

	if (ptpClock->delayMechanism == P2P) {

		if(!isFromSelf && rtOpts->unicastNegotiation && rtOpts->ipMode == IPMODE_UNICAST) {
		    nodeTable = findUnicastGrants(&header->sourcePortIdentity, 0, 
				ptpClock->unicastGrants, ptpClock->unicastGrantIndex, UNICAST_MAX_DESTINATIONS,
				FALSE);
		    if(nodeTable == NULL || !(nodeTable->grantData[PDELAY_RESP].granted)) {
			DBG("Ignoring Peer Delay Request from peer: unicast transmission not granted\n");
			ptpClock->counters.discardedMessages++;
			    return;
		    } else {
			nodeTable->grantData[PDELAY_RESP].receiving = header->sequenceId;
		    }

		}

		DBGV("PdelayReq message received : \n");

		if(length < PDELAY_REQ_LENGTH) {
			DBG("Error: PdelayReq message too short\n");
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
				processPdelayReqFromSelf(tint, rtOpts, ptpClock);
				break;
			} else {
				ptpClock->counters.pdelayReqMessagesReceived++;
				msgUnpackHeader(ptpClock->msgIbuf,
						&ptpClock->PdelayReqHeader);
				issuePdelayResp(tint, header, sourceAddress, rtOpts,
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
		DBG("PdelayReq ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}

static void
processPdelayReqFromSelf(const TimeInternal * tint, const RunTimeOpts * rtOpts, PtpClock * ptpClock) {
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
handlePdelayResp(const MsgHeader *header, TimeInternal *tint,
		 ssize_t length, Boolean isFromSelf, Integer32 sourceAddress, Integer32 destinationAddress,
		 const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	if (ptpClock->delayMechanism == P2P) {

		Integer32 dst = 0;

		if(destinationAddress) {
			dst = destinationAddress;
		    } else {
			/* last resort: may cause PdelayRespFollowUp to be sent to incorrect destinations */
			dst = ptpClock->lastPdelayRespDst;
		    }


		/* Boolean isFromCurrentParent = FALSE; NOTE: This is never used in this function */
		TimeInternal requestReceiptTimestamp;
		TimeInternal correctionField;
	
		DBG("PdelayResp message received : \n");

		if (length < PDELAY_RESP_LENGTH) {
			DBG("Error: PdelayResp message too short\n");
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
				processPdelayRespFromSelf(tint, rtOpts, ptpClock, dst, header->sequenceId);
				break;
			}
			msgUnpackPdelayResp(ptpClock->msgIbuf,
					    &ptpClock->msgTmp.presp);

			if (ptpClock->sentPdelayReqSequenceId != 
			       ((UInteger16)(header->sequenceId + 1))) {
				    DBG("PdelayResp: sequence mismatch - sent: %d, received: %d\n",
					    ptpClock->sentPdelayReqSequenceId,
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
				if (rtOpts->ignore_delayreq_interval_master == 0) {
					DBGV("current pdelay_req: %d  new pdelay req: %d \n",
						ptpClock->logMinPdelayReqInterval,
					header->logMessageInterval);
                        	    if (ptpClock->logMinPdelayReqInterval != header->logMessageInterval) {
			
                    			if(header->logMessageInterval == UNICAST_MESSAGEINTERVAL &&
						rtOpts->autoDelayReqInterval) {

						if(rtOpts->unicastNegotiation && ptpClock->peerGrants.grantData[PDELAY_RESP].granted) {
						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Peer Delay Interval %d from peer\n",
							    ptpClock->peerGrants.grantData[PDELAY_RESP].logInterval);
						    }
						    ptpClock->logMinPdelayReqInterval = ptpClock->peerGrants.grantData[PDELAY_RESP].logInterval;
						} else {

						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Peer Delay Interval %d from peer (unicast-unknown) - overriding with %d\n",
							    header->logMessageInterval, rtOpts->logMinPdelayReqInterval);
						    }
						    ptpClock->logMinPdelayReqInterval = rtOpts->logMinPdelayReqInterval;

						}	
					} else {
						/* Accept new DelayReq value from the Master */

						    NOTICE("Received new Peer Delay Request interval %d from Master (was: %d)\n",
							     header->logMessageInterval, ptpClock->logMinPdelayReqInterval );

						    // collect new value indicated by the Master
						    ptpClock->logMinPdelayReqInterval = header->logMessageInterval;
				    }
				} else {

					if (ptpClock->logMinPdelayReqInterval != rtOpts->logMinPdelayReqInterval) {
						INFO("New Peer Delay Request interval applied: %d (was: %d)\n",
							rtOpts->logMinPdelayReqInterval, ptpClock->logMinPdelayReqInterval);
					}
					ptpClock->logMinPdelayReqInterval = rtOpts->logMinPdelayReqInterval;
				}
				
				}
	                    		ptpClock->delayRespWaiting = FALSE;
				}

				ptpClock->recvPdelayRespSequenceId = header->sequenceId;
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
		DBG("PdelayResp ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}

static void 
processPdelayRespFromSelf(const TimeInternal * tint, const RunTimeOpts * rtOpts, PtpClock * ptpClock, Integer32 dst, const UInteger16 sequenceId)
{
	TimeInternal timestamp;
	
	addTime(&timestamp, tint, &rtOpts->outboundLatency);

	issuePdelayRespFollowUp(&timestamp, &ptpClock->PdelayReqHeader, dst,
		rtOpts, ptpClock, sequenceId);
}

static void 
handlePdelayRespFollowUp(const MsgHeader *header, ssize_t length, 
			 Boolean isFromSelf, const RunTimeOpts *rtOpts, 
			 PtpClock *ptpClock)
{

	if (ptpClock->delayMechanism == P2P) {
		TimeInternal responseOriginTimestamp;
		TimeInternal correctionField;
	
		DBG("PdelayRespfollowup message received : \n");
	
		if(length < PDELAY_RESP_FOLLOW_UP_LENGTH) {
			DBG("Error: PdelayRespfollowup message too short\n");
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
			    ptpClock->sentPdelayReqSequenceId) && (header->sequenceId == ptpClock->recvPdelayRespSequenceId)) {
				msgUnpackPdelayRespFollowUp(
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

/* pdelay interval handling begin */
				if (rtOpts->ignore_delayreq_interval_master == 0) {
					DBGV("current delay_req: %d  new delay req: %d \n",
						ptpClock->logMinPdelayReqInterval,
					header->logMessageInterval);
                        	    if (ptpClock->logMinPdelayReqInterval != header->logMessageInterval) {
			
                    			if(header->logMessageInterval == UNICAST_MESSAGEINTERVAL &&
						rtOpts->autoDelayReqInterval) {

						if(rtOpts->unicastNegotiation && ptpClock->peerGrants.grantData[PDELAY_RESP].granted) {
						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Peer Delay Interval %d from peer\n",
							    ptpClock->peerGrants.grantData[PDELAY_RESP].logInterval);
						    }
						    ptpClock->logMinPdelayReqInterval = ptpClock->peerGrants.grantData[PDELAY_RESP].logInterval;
						} else {

						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Peer Delay Interval %d from peer (unicast-unknown) - overriding with %d\n",
							    header->logMessageInterval, rtOpts->logMinPdelayReqInterval);
						    }
						    ptpClock->logMinPdelayReqInterval = rtOpts->logMinPdelayReqInterval;

						}	
					} else {
						/* Accept new DelayReq value from the Master */

						    NOTICE("Received new Peer Delay Request interval %d from Master (was: %d)\n",
							     header->logMessageInterval, ptpClock->logMinPdelayReqInterval );

						    // collect new value indicated by the Master
						    ptpClock->logMinPdelayReqInterval = header->logMessageInterval;
				    }
				} else {

					if (ptpClock->logMinPdelayReqInterval != rtOpts->logMinPdelayReqInterval) {
						INFO("New Peer Delay Request interval applied: %d (was: %d)\n",
							rtOpts->logMinPdelayReqInterval, ptpClock->logMinPdelayReqInterval);
					}
					ptpClock->logMinPdelayReqInterval = rtOpts->logMinPdelayReqInterval;
				}

/* pdelay interval handling end */
			}
                    		ptpClock->delayRespWaiting = FALSE;	
				break;
			} else {
				DBG("PdelayRespFollowup: sequence mismatch - Received: %d "
				"PdelayReq sent: %d, PdelayResp received: %d\n",
				header->sequenceId, ptpClock->sentPdelayReqSequenceId,
				ptpClock->recvPdelayRespSequenceId);
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
		DBG("PdelayRespFollowUp ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}

/* Only accept the management / signaling message if it satisfies 15.3.1 Table 36 */ 
Boolean
acceptPortIdentity(PortIdentity thisPort, PortIdentity targetPort)
{
        ClockIdentity allOnesClkIdentity;
        UInteger16    allOnesPortNumber = 0xFFFF;
        memset(allOnesClkIdentity, 0xFF, sizeof(allOnesClkIdentity));

	if(!memcmp(targetPort.clockIdentity, thisPort.clockIdentity, CLOCK_IDENTITY_LENGTH) &&
	    (targetPort.portNumber == thisPort.portNumber)) {
	    return TRUE;
	}

        if(!memcmp(targetPort.clockIdentity, allOnesClkIdentity, CLOCK_IDENTITY_LENGTH) &&
            (targetPort.portNumber == allOnesPortNumber)) {
	    return TRUE;
	}

	return FALSE;
}


static void
handleManagement(MsgHeader *header,
		 Boolean isFromSelf, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
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

        if(!acceptPortIdentity(ptpClock->portIdentity, ptpClock->msgTmp.manage.targetPortIdentity))
        {
                DBGV("handleManagement: The management message was not accepted");
		ptpClock->counters.discardedMessages++;
                /* cleanup msgTmp managementTLV */
                freeManagementTLV(&ptpClock->msgTmp.manage);
                return;
        }

	if(!rtOpts->managementSetEnable &&
	    (ptpClock->msgTmp.manage.actionField == SET ||
	    ptpClock->msgTmp.manage.actionField == COMMAND)) {
		DBGV("Dropping SET/COMMAND management message - read-only mode enabled");
		ptpClock->counters.discardedMessages++;
                /* cleanup msgTmp managementTLV */
                freeManagementTLV(&ptpClock->msgTmp.manage);
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
                /* cleanup msgTmp managementTLV */
                freeManagementTLV(&ptpClock->msgTmp.manage);
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

        /* If the management message we received was unicast, we also reply with unicast */
        if((header->flagField0 & PTP_UNICAST) == PTP_UNICAST)
                ptpClock->LastSlaveAddr = ptpClock->netPath.lastSourceAddr;
        else ptpClock->LastSlaveAddr = 0;

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

/* This code does not get built in the slave-only build */
#ifndef PTPD_SLAVE_ONLY

/* send Announce to all destinations */
static void 
issueAnnounce(const RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Integer32 dst = 0;
	int i = 0;
	UnicastGrantData *grant = NULL;
	Boolean okToSend = TRUE;

	/* send Announce to Ethernet or multicast */
	if(rtOpts->transport == IEEE_802_3 || (rtOpts->ipMode != IPMODE_UNICAST)) {
		issueAnnounceSingle(dst, &ptpClock->sentAnnounceSequenceId, rtOpts, ptpClock);
	/* send Announce to unicast destination(s) */
	} else {
	    /* send to granted only */
	    if(rtOpts->unicastNegotiation) {
		for(i = 0; i < UNICAST_MAX_DESTINATIONS; i++) {
		    grant = &(ptpClock->unicastGrants[i].grantData[ANNOUNCE]);
		    okToSend = TRUE;
		    if(grant->logInterval > ptpClock->logAnnounceInterval ) {
			grant->intervalCounter %= (UInteger32)(pow(2,grant->logInterval - ptpClock->logAnnounceInterval));
			if(grant->intervalCounter != 0) {
			    okToSend = FALSE;
			}
			grant->intervalCounter++;
		    }
		    if(grant->granted) {
			if(okToSend) {
			    issueAnnounceSingle(ptpClock->unicastGrants[i].transportAddress,
			    &grant->sentSeqId,rtOpts, ptpClock);
			}
		    }
		}
	    /* send to fixed unicast destinations */
	    } else {
		for(i = 0; i < ptpClock->unicastDestinationCount; i++) {
			issueAnnounceSingle(ptpClock->unicastDestinations[i].transportAddress,
			&(ptpClock->unicastGrants[i].grantData[ANNOUNCE].sentSeqId),
						rtOpts, ptpClock);
		    }
		}
	}

}

/* send single announce to a single destination */
static void 
issueAnnounceSingle(Integer32 dst, UInteger16 *sequenceId, const RunTimeOpts *rtOpts,PtpClock *ptpClock)
{

	msgPackAnnounce(ptpClock->msgObuf, *sequenceId,ptpClock);

	if (!netSendGeneral(ptpClock->msgObuf,ANNOUNCE_LENGTH,
			    &ptpClock->netPath, rtOpts, dst)) {
		    toState(PTP_FAULTY,rtOpts,ptpClock);
		    ptpClock->counters.messageSendErrors++;
		    DBGV("Announce message can't be sent -> FAULTY state \n");
	} else {
		    DBGV("Announce MSG sent ! \n");
		    (*sequenceId)++;
		    ptpClock->counters.announceMessagesSent++;
	}
}

/* send Sync to all destinations */
static void 
issueSync(const RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Integer32 dst = 0;
	int i = 0;
	UnicastGrantData *grant = NULL;
	Boolean okToSend = TRUE;

	/* send Sync to Ethernet or multicast */
	if(rtOpts->transport == IEEE_802_3 || (rtOpts->ipMode != IPMODE_UNICAST)) {
		(void)issueSyncSingle(dst, &ptpClock->sentSyncSequenceId, rtOpts, ptpClock);

	/* send Sync to unicast destination(s) */
	} else {
	    for(i = 0; i < UNICAST_MAX_DESTINATIONS; i++) {
		ptpClock->syncDestIndex[i].transportAddress = 0;
		clearTime(&ptpClock->unicastGrants[i].lastSyncTimestamp);
		clearTime(&ptpClock->unicastDestinations[i].lastSyncTimestamp);
	    }
	    /* send to granted only */
	    if(rtOpts->unicastNegotiation) {
		for(i = 0; i < UNICAST_MAX_DESTINATIONS; i++) {
		    grant = &(ptpClock->unicastGrants[i].grantData[SYNC]);
		    okToSend = TRUE;
		    /* handle different intervals */
		    if(grant->logInterval > ptpClock->logSyncInterval ) {
			grant->intervalCounter %= (UInteger32)(pow(2,grant->logInterval - ptpClock->logSyncInterval));
			if(grant->intervalCounter != 0) {
			    okToSend = FALSE;
			}
			DBG("mixed interval to %d counter: %d\n", grant->parent->transportAddress,grant->intervalCounter);
			grant->intervalCounter++;
		    }

		    if(grant->granted) {
			if(okToSend) {
			    ptpClock->unicastGrants[i].lastSyncTimestamp =
				issueSyncSingle(ptpClock->unicastGrants[i].transportAddress,
				&grant->sentSeqId,rtOpts, ptpClock);
			}
		    }
		}
	    /* send to fixed unicast destinations */
	    } else {
		for(i = 0; i < ptpClock->unicastDestinationCount; i++) {
			ptpClock->unicastDestinations[i].lastSyncTimestamp =
			    issueSyncSingle(ptpClock->unicastDestinations[i].transportAddress,
			    &(ptpClock->unicastGrants[i].grantData[SYNC].sentSeqId),
						rtOpts, ptpClock);
		    }
		}
	}

}

/*Pack and send a single Sync message, return the embedded timestamp*/
static TimeInternal
issueSyncSingle(Integer32 dst, UInteger16 *sequenceId, const RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Timestamp originTimestamp;
	TimeInternal internalTime, now;

	getTime(&internalTime);

	if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
		internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
	}

	/* 
         * LEAPNOTE01#
         * This is done here rather than in netSendEvent because we must also
         * prevent sequence IDs from incrementing so that there's no discontinuity.
         * Ideally netSendEvent should be incrementing the sequence numbers,
         * then this could be centrally blocked, but then again this makes
         * netSendEvent less generic - on the other end in 2.4 this will be 
         * a glue function for libcck transports, so this will be fine.
	 * In other words, ptpClock->netSendEvent() will be a function pointer.
	 * What was I talking about? Yes, right, sequence numbers can be 
         * incremented in netSendEvent.
	 */

	if(ptpClock->leapSecondInProgress) {
		DBG("Leap second in progress - will not send SYNC\n");
		clearTime(&internalTime);
		return internalTime;
	}

	fromInternalTime(&internalTime,&originTimestamp);

	now = internalTime;

	msgPackSync(ptpClock->msgObuf,*sequenceId,&originTimestamp,ptpClock);

	if (!netSendEvent(ptpClock->msgObuf,SYNC_LENGTH,&ptpClock->netPath,
		rtOpts, dst, &internalTime)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("Sync message can't be sent -> FAULTY state \n");
	} else {

		DBGV("Sync MSG sent ! \n");

#ifdef SO_TIMESTAMPING

#ifdef PTPD_PCAP
		if((ptpClock->netPath.pcapEvent == NULL) && !ptpClock->netPath.txTimestampFailure) {
#else
		if(!ptpClock->netPath.txTimestampFailure) {
#endif /* PTPD_PCAP */
			if(internalTime.seconds && internalTime.nanoseconds) {

			    if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
				    internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			    }
			    processSyncFromSelf(&internalTime, rtOpts, ptpClock, dst, *sequenceId);
			}
		}
#endif

		ptpClock->lastSyncDst = dst;

		if(!internalTime.seconds && !internalTime.nanoseconds) {
		    internalTime = now;
		}

		/* index the Sync destination */
		indexSync(&internalTime, *sequenceId, dst, ptpClock->syncDestIndex);

		(*sequenceId)++;
		ptpClock->counters.syncMessagesSent++;

	}

    return internalTime;
}



/*Pack and send on general multicast ip adress a FollowUp message*/
static void
issueFollowup(const TimeInternal *tint,const RunTimeOpts *rtOpts,PtpClock *ptpClock, Integer32 dst, UInteger16 sequenceId)
{
	Timestamp preciseOriginTimestamp;
	fromInternalTime(tint,&preciseOriginTimestamp);
	
	msgPackFollowUp(ptpClock->msgObuf,&preciseOriginTimestamp,ptpClock,sequenceId);	

	if (!netSendGeneral(ptpClock->msgObuf,FOLLOW_UP_LENGTH,
			    &ptpClock->netPath, rtOpts, dst)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("FollowUp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("FollowUp MSG sent ! \n");
		ptpClock->counters.followUpMessagesSent++;
	}
}

#endif /* PTPD_SLAVE_ONLY */

/*Pack and send on event multicast ip adress a DelayReq message*/
static void
issueDelayReq(const RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Timestamp originTimestamp;
	TimeInternal internalTime;
#if 0 /* PCAP ONLY */
	MsgHeader ourDelayReq;
#endif
	/* see LEAPNOTE01# in this file */
	if(ptpClock->leapSecondInProgress) {
	    DBG("Leap second in progress - will not send DELAY_REQ\n");
	    return;
	}

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

	  /* in hybrid mode  or unicast mode, send delayReq to current master */
        if (rtOpts->ipMode == IPMODE_HYBRID || rtOpts->ipMode == IPMODE_UNICAST) {
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

#ifdef PTPD_PCAP
		if((ptpClock->netPath.pcapEvent == NULL) && !ptpClock->netPath.txTimestampFailure) {
#else
		if(!ptpClock->netPath.txTimestampFailure) {
#endif /* PTPD_PCAP */
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

		/* Explicitly re-arm timer for sending the next delayReq
		 * 9.5.11.2: arm the timer with a uniform range from 0 to 2 x interval
		 * this is only ever used here, so removed the timerStart_random function
		 */ 

		timerStart(&ptpClock->timers[DELAYREQ_INTERVAL_TIMER],
		   pow(2,ptpClock->logMinDelayReqInterval) * getRand() * 2.0);
#if 0 /* PCAP ONLY */
		msgUnpackHeader(ptpClock->msgObuf, &ourDelayReq);
		handleDelayReq(&ourDelayReq, DELAY_REQ_LENGTH, &internalTime,
			       TRUE, rtOpts, ptpClock);
#endif 
	}
}

/*Pack and send on event multicast ip adress a PdelayReq message*/
static void
issuePdelayReq(const RunTimeOpts *rtOpts,PtpClock *ptpClock)
{
	Integer32 dst = 0;
	Timestamp originTimestamp;
	TimeInternal internalTime;

	/* see LEAPNOTE01# in this file */
	if(ptpClock->leapSecondInProgress) {
	    DBG("Leap secon in progress - will not send PDELAY_REQ\n");
	    return;
	}

	getTime(&internalTime);
	if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
		internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
	}
	fromInternalTime(&internalTime,&originTimestamp);

	if(rtOpts->ipMode == IPMODE_UNICAST && ptpClock->unicastPeerDestination.transportAddress) {
	    dst = ptpClock->unicastPeerDestination.transportAddress;
	}
	
	msgPackPdelayReq(ptpClock->msgObuf,&originTimestamp,ptpClock);
	if (!netSendPeerEvent(ptpClock->msgObuf,PDELAY_REQ_LENGTH,
			      &ptpClock->netPath, rtOpts, dst, &internalTime)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("PdelayReq message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PdelayReq MSG sent ! \n");
		
#ifdef SO_TIMESTAMPING

#ifdef PTPD_PCAP
		if((ptpClock->netPath.pcapEvent == NULL) && !ptpClock->netPath.txTimestampFailure) {
#else
		if(!ptpClock->netPath.txTimestampFailure) {
#endif /* PTPD_PCAP */
			if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
				internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			}			
			processPdelayReqFromSelf(&internalTime, rtOpts, ptpClock);
		}
#endif
		
		ptpClock->sentPdelayReqSequenceId++;
		ptpClock->counters.pdelayReqMessagesSent++;
	}
}

/*Pack and send on event multicast ip adress a PdelayResp message*/
static void
issuePdelayResp(const TimeInternal *tint,MsgHeader *header, Integer32 sourceAddress, const RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{
	
	Timestamp requestReceiptTimestamp;
	TimeInternal internalTime;

	Integer32 dst = 0;

	/* see LEAPNOTE01# in this file */
	if(ptpClock->leapSecondInProgress) {
		DBG("Leap second in progress - will not send PDELAY_RESP\n");
		return;
	}

	/* if request was unicast and we're running unicast, reply to source */
	if ( (rtOpts->ipMode != IPMODE_MULTICAST) &&
	     (header->flagField0 & PTP_UNICAST) == PTP_UNICAST) {
		dst = sourceAddress;
	}
	
	fromInternalTime(tint,&requestReceiptTimestamp);
	msgPackPdelayResp(ptpClock->msgObuf,header,
			  &requestReceiptTimestamp,ptpClock);

	if (!netSendPeerEvent(ptpClock->msgObuf,PDELAY_RESP_LENGTH,
			      &ptpClock->netPath, rtOpts, dst, &internalTime)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("PdelayResp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PdelayResp MSG sent ! \n");
		
#ifdef SO_TIMESTAMPING

#ifdef PTPD_PCAP
		if((ptpClock->netPath.pcapEvent == NULL) && !ptpClock->netPath.txTimestampFailure) {
#else
		if(!ptpClock->netPath.txTimestampFailure) {
#endif /* PTPD_PCAP */
			if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
				internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			}			
			processPdelayRespFromSelf(&internalTime, rtOpts, ptpClock, dst, header->sequenceId);
		}
#endif
		
		ptpClock->counters.pdelayRespMessagesSent++;
		ptpClock->lastPdelayRespDst = dst;
	}
}


/*Pack and send a DelayResp message on event socket*/
static void
issueDelayResp(const TimeInternal *tint,MsgHeader *header,Integer32 sourceAddress, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Timestamp requestReceiptTimestamp;
	fromInternalTime(tint,&requestReceiptTimestamp);
	msgPackDelayResp(ptpClock->msgObuf,header,&requestReceiptTimestamp,
			 ptpClock);

	Integer32 dst = 0;

	/* if request was unicast and we're running unicast, reply to source */
	if ( (rtOpts->ipMode != IPMODE_MULTICAST) &&
	     (header->flagField0 & PTP_UNICAST) == PTP_UNICAST) {
		dst = sourceAddress;
	}

	if (!netSendGeneral(ptpClock->msgObuf, DELAY_RESP_LENGTH,
			    &ptpClock->netPath, rtOpts, dst)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("delayResp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PdelayResp MSG sent ! \n");
		ptpClock->counters.delayRespMessagesSent++;
	}
}

static void
issuePdelayRespFollowUp(const TimeInternal *tint, MsgHeader *header, Integer32 dst,
			     const RunTimeOpts *rtOpts, PtpClock *ptpClock, const UInteger16 sequenceId)
{
	Timestamp responseOriginTimestamp;
	fromInternalTime(tint,&responseOriginTimestamp);

	msgPackPdelayRespFollowUp(ptpClock->msgObuf,header,
				  &responseOriginTimestamp,ptpClock, sequenceId);
	if (!netSendPeerGeneral(ptpClock->msgObuf,
				PDELAY_RESP_FOLLOW_UP_LENGTH,
				&ptpClock->netPath, rtOpts, dst)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		ptpClock->counters.messageSendErrors++;
		DBGV("PdelayRespFollowUp message can't be sent -> FAULTY state \n");
	} else {
		DBGV("PdelayRespFollowUp MSG sent ! \n");
		ptpClock->counters.pdelayRespFollowUpMessagesSent++;
	}
}

#if 0
static void 
issueManagement(MsgHeader *header,MsgManagement *manage,const RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{

	ptpClock->counters.managementMessagesSent++;

}
#endif

static void 
issueManagementRespOrAck(MsgManagement *outgoing, const RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{
	Integer32 dst = 0;

	/* pack ManagementTLV */
	msgPackManagementTLV( ptpClock->msgObuf, outgoing, ptpClock);

	/* set header messageLength, the outgoing->tlv->lengthField is now valid */
	outgoing->header.messageLength = MANAGEMENT_LENGTH +
					TL_LENGTH +
					outgoing->tlv->lengthField;

	msgPackManagement( ptpClock->msgObuf, outgoing, ptpClock);

	/* If LastSlaveAddr was populated, we reply via Unicast */
	if (ptpClock->LastSlaveAddr > 0)
		dst = ptpClock->LastSlaveAddr;

	if(!netSendGeneral(ptpClock->msgObuf, outgoing->header.messageLength,
			   &ptpClock->netPath, rtOpts, dst)) {
		DBGV("Management response/acknowledge can't be sent -> FAULTY state \n");
		ptpClock->counters.messageSendErrors++;
		toState(PTP_FAULTY, rtOpts, ptpClock);
	} else {
		DBGV("Management response/acknowledge msg sent \n");
		ptpClock->counters.managementMessagesSent++;
	}
}

static void
issueManagementErrorStatus(MsgManagement *outgoing, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Integer32 dst = 0;

	/* pack ManagementErrorStatusTLV */
	msgPackManagementErrorStatusTLV( ptpClock->msgObuf, outgoing, ptpClock);

	/* set header messageLength, the outgoing->tlv->lengthField is now valid */
	outgoing->header.messageLength = MANAGEMENT_LENGTH +
					TL_LENGTH +
					outgoing->tlv->lengthField;

	msgPackManagement( ptpClock->msgObuf, outgoing, ptpClock);

	if (ptpClock->LastSlaveAddr > 0)
		dst = ptpClock->LastSlaveAddr;

	if(!netSendGeneral(ptpClock->msgObuf, outgoing->header.messageLength,
			   &ptpClock->netPath, rtOpts, dst)) {
		DBGV("Management error status can't be sent -> FAULTY state \n");
		ptpClock->counters.messageSendErrors++;
		toState(PTP_FAULTY, rtOpts, ptpClock);
	} else {
		DBGV("Management error status msg sent \n");
		ptpClock->counters.managementMessagesSent++;
	}

}

void
addForeign(Octet *buf,MsgHeader *header,PtpClock *ptpClock, UInteger8 localPreference)
{
	int i,j;
	Boolean found = FALSE;

	DBGV("addForeign localPref: %d\n", localPreference);

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
			ptpClock->foreign[j].localPreference = localPreference;
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
		
		/* Preserve best master record from overwriting (sf FR #22) - use next slot*/
		if (ptpClock->foreign_record_i == ptpClock->foreign_record_best) {
			ptpClock->foreign_record_i++;
			ptpClock->foreign_record_i %= ptpClock->number_foreign_records;
		}
		
		j = ptpClock->foreign_record_i;
		
		/*Copy new foreign master data set from Announce message*/
		copyClockIdentity(ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity,
		       header->sourcePortIdentity.clockIdentity);
		ptpClock->foreign[j].foreignMasterPortIdentity.portNumber = 
			header->sourcePortIdentity.portNumber;
		ptpClock->foreign[j].foreignMasterAnnounceMessages = 0;
		ptpClock->foreign[j].localPreference = localPreference;
		
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
void
updateDatasets(PtpClock* ptpClock, const RunTimeOpts* rtOpts)
{

	if(rtOpts->unicastNegotiation) {
	    	updateUnicastGrantTable(ptpClock->unicastGrants, 
			    ptpClock->unicastDestinationCount, rtOpts);
		if(rtOpts->unicastPeerDestinationSet) {
	    	    updateUnicastGrantTable(&ptpClock->peerGrants, 
			    1, rtOpts);

		}
	}

	switch(ptpClock->portState) {

		/* We are master so update both the port and the parent dataset */
		case PTP_MASTER:

			if(rtOpts->dot2AS) {
			    ptpClock->transportSpecific = TSP_ETHERNET_AVB;
			} else {
			    ptpClock->transportSpecific = TSP_DEFAULT;
			}

			ptpClock->numberPorts = NUMBER_PORTS;
			ptpClock->portIdentity.portNumber = rtOpts->portNumber;

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
			ptpClock->logAnnounceInterval = rtOpts->logAnnounceInterval;
			ptpClock->announceReceiptTimeout = rtOpts->announceReceiptTimeout;
			ptpClock->logSyncInterval = rtOpts->logSyncInterval;
			ptpClock->logMinPdelayReqInterval = rtOpts->logMinPdelayReqInterval;
			ptpClock->logMinDelayReqInterval = rtOpts->initial_delayreq;
			break;
		/*
		 * we are not master so update the port dataset only - parent will be updated
		 * by m1() if we go master - basically update the fields affecting BMC only
		 */
		case PTP_SLAVE:
                    	if(ptpClock->logMinDelayReqInterval == UNICAST_MESSAGEINTERVAL &&
				rtOpts->autoDelayReqInterval) {
					NOTICE("Running at %d Delay Interval (unicast) - overriding with %d\n",
					ptpClock->logMinDelayReqInterval, rtOpts->logMinDelayReqInterval);
					ptpClock->logMinDelayReqInterval = rtOpts->logMinDelayReqInterval;
				}
		case PTP_PASSIVE:
			ptpClock->numberPorts = NUMBER_PORTS;
			ptpClock->portIdentity.portNumber = rtOpts->portNumber;

			if(rtOpts->dot2AS) {
			    ptpClock->transportSpecific = TSP_ETHERNET_AVB;
			} else {
			    ptpClock->transportSpecific = TSP_DEFAULT;
			}

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
respectUtcOffset(const RunTimeOpts * rtOpts, PtpClock * ptpClock) {
	if (ptpClock->timePropertiesDS.currentUtcOffsetValid || rtOpts->alwaysRespectUtcOffset) {
		return TRUE;
	}
	return FALSE;
}

