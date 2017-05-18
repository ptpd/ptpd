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
 * @file   protocol.c
 * @date   Wed Jun 23 09:40:39 2010
 *
 * @brief  The code that handles the IEEE-1588 protocol and state machine
 *
 *
 */

#include "ptpd.h"

#include "lib1588/ptp_message.h"


static void doState(GlobalConfig*,PtpClock*);

static void handleAnnounce(MsgHeader*, ssize_t,Boolean, void *, const GlobalConfig*, PtpClock*);
static void handleSync(const MsgHeader *header, ssize_t length,TimeInternal *tint, Boolean isFromSelf, void *sourceAddress, void* destinationAddress, const GlobalConfig *global, PtpClock *ptpClock);
static void handleFollowUp(const MsgHeader*, ssize_t,Boolean,const GlobalConfig*,PtpClock*);
static void handlePdelayReq(MsgHeader*, ssize_t,const TimeInternal*, void*, Boolean,const GlobalConfig*,PtpClock*);
static void handleDelayReq(const MsgHeader*, ssize_t, const TimeInternal*, void *src, Boolean,const GlobalConfig*,PtpClock*);
static void handlePdelayResp(const MsgHeader*, TimeInternal* ,ssize_t,Boolean, void *src, void *dst, const GlobalConfig*,PtpClock*);
static void handleDelayResp(const MsgHeader*, ssize_t, const GlobalConfig*,PtpClock*);
static void handlePdelayRespFollowUp(const MsgHeader*, ssize_t, Boolean, const GlobalConfig*,PtpClock*);

static void addForeign(Octet *buf,MsgHeader *header,PtpClock *ptpClock, UInteger8 localPreference, void *src);

/* handleManagement in management.c */
/* handleSignaling in signaling.c */

#ifndef PTPD_SLAVE_ONLY /* does not get compiled when building slave only */
static void issueAnnounce(const GlobalConfig*,PtpClock*);
static void issueAnnounceSingle(void *dst, UInteger16*, Timestamp*, const GlobalConfig*,PtpClock*);
static void issueSync(const GlobalConfig*,PtpClock*);
#endif /* PTPD_SLAVE_ONLY */

/* enabled for slave-only to support PTPMon */
static TimeInternal issueSyncSingle(void *dst, UInteger16*, const GlobalConfig*,PtpClock*);
static void issueFollowup(const TimeInternal*,const GlobalConfig*,PtpClock*, void *dst, const UInteger16);

static void issuePdelayReq(const GlobalConfig*,PtpClock*);
static void issueDelayReq(const GlobalConfig*,PtpClock*);
static void issuePdelayResp(const TimeInternal*,MsgHeader*, void *dst,const GlobalConfig*,PtpClock*);
static void issueDelayResp(const TimeInternal*,MsgHeader*, void *dst,const GlobalConfig*,PtpClock*);
static void issuePdelayRespFollowUp(const TimeInternal*,MsgHeader*, void *dst, const GlobalConfig*,PtpClock*, const UInteger16);

static int populatePtpMon(char *buf, MsgHeader *header, PtpClock *ptpClock, const GlobalConfig *global);

static void processSyncFromSelf(const TimeInternal * tint, const GlobalConfig * global, PtpClock * ptpClock, void *dst, const UInteger16 sequenceId);
static void indexSync(TimeInternal *timeStamp, UInteger16 sequenceId, void* transportAddress, SyncDestEntry *index);

static void processDelayReqFromSelf(const TimeInternal * tint, const GlobalConfig * global, PtpClock * ptpClock);
static void processPdelayReqFromSelf(const TimeInternal * tint, const GlobalConfig * global, PtpClock * ptpClock);
static void processPdelayRespFromSelf(const TimeInternal * tint, const GlobalConfig * global, PtpClock * ptpClock, void *dst, const UInteger16 sequenceId);

/* this shouldn't really be in protocol.c, it will be moved later */
static void timestampCorrection(const GlobalConfig * global, PtpClock *ptpClock, TimeInternal *timeStamp);

static void* lookupSyncIndex(TimeInternal *timeStamp, UInteger16 sequenceId, SyncDestEntry *index);
static void* findSyncDestination(TimeInternal *timeStamp, const GlobalConfig *global, PtpClock *ptpClock);

static int populatePtpMon(char *buf, MsgHeader *header, PtpClock *ptpClock, const GlobalConfig *global);


/* single FSM step execution */
void
ptpRun(GlobalConfig *global, PtpClock *ptpClock)
{

	doState(global, ptpClock);

	if (ptpClock->message_activity)
			DBGV("activity\n");

	if (tmrExpired(ptpClock, ALARM_UPDATE)) {
		if(global->alarmInitialDelay && (ptpClock->alarmDelay > 0)) {
		    ptpClock->alarmDelay -= ALARM_UPDATE_INTERVAL;
		    if(ptpClock->alarmDelay <= 0 && global->alarmsEnabled) {
			INFO("Alarm delay expired - starting alarm processing\n");
			    enableAlarms(ptpClock->alarms, ALRM_MAX, TRUE);
			}
		}

		updateAlarms(ptpClock->alarms, ALRM_MAX);

	}

	if (tmrExpired(ptpClock, UNICAST_GRANT)) {
		refreshUnicastGrants(ptpClock->unicastGrants,
			global->unicastDestinationsSet ?
				    ptpClock->unicastDestinationCount : UNICAST_MAX_DESTINATIONS,
			global, ptpClock);

		if(!ptpAddrIsEmpty(ptpClock->unicastPeerDestination.protocolAddress)) {
		    refreshUnicastGrants(&ptpClock->peerGrants,
			1, global, ptpClock);
		}
	}

	if (tmrExpired(ptpClock, OPERATOR_MESSAGES)) {
		resetWarnings(global, ptpClock);
	}


}

/* state change wrapper to allow for event generation / control when changing state */
void setPortState(PtpClock *ptpClock, Enumeration8 state)
{

    if(ptpClock->portDS.portState != state) {

	DBG("State change request from %s to %s\n", portState_getName(ptpClock->portDS.lastPortState), portState_getName(state));

	if(ptpClock->callbacks.onStateChange) {
	    if(!ptpClock->callbacks.onStateChange(ptpClock, ptpClock->portDS.portState, state)) {
		DBG("onStateChange callback denied state change request\n");
		return;
	    }

	}

	ptpClock->portDS.lastPortState = ptpClock->portDS.portState;

	ptpClock->portDS.portState = state;
	displayStatus(ptpClock, "Now in state: ");

    }

    /* "expected state" checks */

    if(ptpClock->defaultDS.slaveOnly) {
	    SET_ALARM(ALRM_PORT_STATE, state != PTP_SLAVE);
    } else if(ptpClock->defaultDS.clockQuality.clockClass < 128) {
	    SET_ALARM(ALRM_PORT_STATE, state != PTP_MASTER && state != PTP_PASSIVE );
    }


}

/* perform actions required when leaving 'port_state' and entering 'state' */
void
toState(UInteger8 state, const GlobalConfig *global, PtpClock *ptpClock)
{
	ptpClock->message_activity = TRUE;
	
	/* leaving state tasks */
	switch (ptpClock->portDS.portState)
	{
	case PTP_FAULTY:
		tmrStop(ptpClock, PORT_FAULT);
		break;
	case PTP_MASTER:

		tmrStop(ptpClock, SYNC_INTERVAL);
		tmrStop(ptpClock, ANNOUNCE_INTERVAL);
		tmrStop(ptpClock, PDELAYREQ_INTERVAL);
		tmrStop(ptpClock, DELAY_RECEIPT);

		if(global->unicastNegotiation && global->transportMode==TMODE_UC) {
		    cancelAllGrants(ptpClock->unicastGrants, UNICAST_MAX_DESTINATIONS,
				global, ptpClock);
		    if(ptpClock->portDS.delayMechanism == P2P) {
			    cancelAllGrants(&ptpClock->peerGrants, 1,
				global, ptpClock);
		    }
		}

		break;
	case PTP_SLAVE:
		tmrStop(ptpClock, ANNOUNCE_RECEIPT);
		tmrStop(ptpClock, SYNC_RECEIPT);
		tmrStop(ptpClock, DELAY_RECEIPT);

		if(global->unicastNegotiation && global->transportMode==TMODE_UC && ptpClock->parentGrants != NULL) {
			/* do not cancel, just start re-requesting so we can still send a cancel on exit */
			ptpClock->parentGrants->grantData[ANNOUNCE_INDEXED].timeLeft = 0;
			ptpClock->parentGrants->grantData[SYNC_INDEXED].timeLeft = 0;
			if(ptpClock->portDS.delayMechanism == E2E) {
			    ptpClock->parentGrants->grantData[DELAY_RESP_INDEXED].timeLeft = 0;
			}

			ptpClock->parentGrants = NULL;

			if(ptpClock->portDS.delayMechanism == P2P) {
			    cancelUnicastTransmission(&ptpClock->peerGrants.grantData[PDELAY_RESP_INDEXED], global, ptpClock);
			    cancelAllGrants(&ptpClock->peerGrants, 1, global, ptpClock);
			}
		}

		if (ptpClock->portDS.delayMechanism == E2E)
			tmrStop(ptpClock, DELAYREQ_INTERVAL);
		else if (ptpClock->portDS.delayMechanism == P2P)
			tmrStop(ptpClock, PDELAYREQ_INTERVAL);

		resetPtpEngineSlaveStats(&ptpClock->slaveStats);
		tmrStop(ptpClock, STATISTICS_UPDATE);

	case PTP_PASSIVE:
		tmrStop(ptpClock, PDELAYREQ_INTERVAL);
		tmrStop(ptpClock, DELAY_RECEIPT);
		tmrStop(ptpClock, ANNOUNCE_RECEIPT);
		break;
		
	case PTP_LISTENING:
		tmrStop(ptpClock, ANNOUNCE_RECEIPT);
		tmrStop(ptpClock, SYNC_RECEIPT);
		tmrStop(ptpClock, DELAY_RECEIPT);

		/* we're leaving LISTENING - reset counter */
                if(state != PTP_LISTENING) {
                    ptpClock->listenCount = 0;
                }
		break;
	case PTP_INITIALIZING:
		if(global->unicastNegotiation) {
		    if(!ptpClock->defaultDS.slaveOnly) {

			initUnicastGrantTable(ptpClock->unicastGrants,
				ptpClock->portDS.delayMechanism,
				UNICAST_MAX_DESTINATIONS, NULL,
				global, ptpClock);

			if(global->unicastDestinationsSet) {
			    initUnicastGrantTable(ptpClock->unicastGrants,
					ptpClock->portDS.delayMechanism,
					ptpClock->unicastDestinationCount, ptpClock->unicastDestinations,
					global, ptpClock);
			}
			
		    } else {
			initUnicastGrantTable(ptpClock->unicastGrants,
					ptpClock->portDS.delayMechanism,
					ptpClock->unicastDestinationCount, ptpClock->unicastDestinations,
					global, ptpClock);
		    }
		    if(!ptpAddrIsEmpty(ptpClock->unicastPeerDestination.protocolAddress)) {
			initUnicastGrantTable(&ptpClock->peerGrants,
					ptpClock->portDS.delayMechanism,
					1, &ptpClock->unicastPeerDestination,
					global, ptpClock);
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
		DBG("manufacturerOUI: %02hhx:%02hhx:%02hhx \n",
			MANUFACTURER_ID_OUI0,
			MANUFACTURER_ID_OUI1,
			MANUFACTURER_ID_OUI2);
		{
		    char filterMask[208];
		    strncpy(filterMask,FILTER_MASK,208);
		}

		initData(global, ptpClock);
		initClock(global, ptpClock);

		m1(global, ptpClock );
		msgPackHeader(ptpClock->msgObuf, ptpClock);

		if(global->statusLog.logEnabled) {
		    writeStatusFile(ptpClock, global, TRUE);
		}

		setPortState(ptpClock, PTP_INITIALIZING);
		break;
		
	case PTP_FAULTY:
		setPortState(ptpClock, PTP_FAULTY);
		break;
		
	case PTP_DISABLED:
		initData(global, ptpClock);
		updateDatasets(ptpClock, global);
		/* well, theoretically we're still in the previous state, so we're not in breach of standard */
		if(global->unicastNegotiation && global->transportMode==TMODE_UC) {
		    cancelAllGrants(ptpClock->unicastGrants, ptpClock->unicastDestinationCount,
				global, ptpClock);
		}
		ptpClock->bestMaster = NULL;
		/* see? NOW we're in disabled state */
		setPortState(ptpClock, PTP_DISABLED);
		if(global->portDisabled) {
		    WARNING("PTP port running in DISABLED state. Awaiting config change or management message\n");
		}
		break;
		
	case PTP_LISTENING:

		if(global->unicastNegotiation) {
		    tmrStart(ptpClock, UNICAST_GRANT, UNICAST_GRANT_REFRESH_INTERVAL);
		}

		/* in Listening state, make sure we don't send anything. Instead we just expect/wait for announces (started below) */
		tmrStop(ptpClock, SYNC_INTERVAL);
		tmrStop(ptpClock, ANNOUNCE_INTERVAL);
		tmrStop(ptpClock, SYNC_RECEIPT);
		tmrStop(ptpClock, DELAY_RECEIPT);
		tmrStop(ptpClock, PDELAYREQ_INTERVAL);
		tmrStop(ptpClock, DELAYREQ_INTERVAL);

		/* This is (re) started on clock updates only */
                tmrStop(ptpClock, CLOCK_UPDATE);

		/* if we're ignoring announces (disable_bmca), go straight to master */
		if(ptpClock->defaultDS.clockQuality.clockClass <= 127 && global->disableBMCA) {
			DBG("unicast master only and ignoreAnnounce: going into MASTER state\n");
			ptpClock->number_foreign_records = 0;
			ptpClock->foreign_record_i = 0;
			m1(global,ptpClock);
			toState(PTP_MASTER, global, ptpClock);
			/* in case if we were not allowed to go into master state */
			if(ptpClock->portDS.portState != PTP_MASTER) {
			    toState(PTP_PASSIVE, global, ptpClock);
			}
			break;
		}

		/*
		 *  Count how many _unique_ timeouts happen to us.
		 *  If we were already in Listen mode, then do not count this as a seperate reset, but stil do a new IGMP refresh
		 */
		if (ptpClock->portDS.portState != PTP_LISTENING) {
			ptpClock->resetCount++;
		} else {
                        ptpClock->listenCount++;
                }

		/* Revert to the original DelayReq interval, and ignore the one for the last master */
		ptpClock->portDS.logMinDelayReqInterval = global->initial_delayreq;

		/* force a IGMP refresh per reset */
		ptpNetworkRefresh(ptpClock);
		
		tmrStart(ptpClock, ANNOUNCE_RECEIPT,
				(ptpClock->portDS.announceReceiptTimeout) *
				(pow(2,ptpClock->portDS.logAnnounceInterval)));

		ptpClock->announceTimeouts = 0;

		ptpClock->bestMaster = NULL;

		setPortState(ptpClock, PTP_LISTENING);

		break;
#ifndef PTPD_SLAVE_ONLY
	case PTP_MASTER:
		if(global->unicastNegotiation) {
		    tmrStart(ptpClock, UNICAST_GRANT, 1);
		}
		tmrStart(ptpClock, SYNC_INTERVAL,
			   pow(2,ptpClock->portDS.logSyncInterval));
		DBG("SYNC INTERVAL TIMER : %f \n",
		    pow(2,ptpClock->portDS.logSyncInterval));
		tmrStart(ptpClock, ANNOUNCE_INTERVAL,
			   pow(2,ptpClock->portDS.logAnnounceInterval));
		tmrStart(ptpClock, PDELAYREQ_INTERVAL,
			   pow(2,ptpClock->portDS.logMinPdelayReqInterval));
		if(ptpClock->portDS.delayMechanism == P2P) {
		    tmrStart(ptpClock, DELAY_RECEIPT, max(
		    (ptpClock->portDS.announceReceiptTimeout) * (pow(2,ptpClock->portDS.logAnnounceInterval)),
			MISSED_MESSAGES_MAX * (pow(2,ptpClock->portDS.logMinPdelayReqInterval))));
		}

		setPortState(ptpClock, PTP_MASTER);
		/* in case if we were not allowed to go into master state */
		if(ptpClock->portDS.portState != PTP_MASTER) {
			toState(PTP_PASSIVE, global, ptpClock);
			break;
		}

		ptpClock->bestMaster = NULL;

		break;
#endif /* PTPD_SLAVE_ONLY */
	case PTP_PASSIVE:
		tmrStart(ptpClock, PDELAYREQ_INTERVAL,
			   pow(2,ptpClock->portDS.logMinPdelayReqInterval));
		tmrStart(ptpClock, ANNOUNCE_RECEIPT,
			   (ptpClock->portDS.announceReceiptTimeout) *
			   (pow(2,ptpClock->portDS.logAnnounceInterval)));
		if(ptpClock->portDS.delayMechanism == P2P) {
		    tmrStart(ptpClock, DELAY_RECEIPT, max(
		    (ptpClock->portDS.announceReceiptTimeout) * (pow(2,ptpClock->portDS.logAnnounceInterval)),
			MISSED_MESSAGES_MAX * (pow(2,ptpClock->portDS.logMinPdelayReqInterval))));
		}
		setPortState(ptpClock, PTP_PASSIVE);
		p1(ptpClock, global);
		break;

	case PTP_UNCALIBRATED:
		setPortState(ptpClock, PTP_UNCALIBRATED);
		break;

	case PTP_SLAVE:

		if(global->unicastNegotiation) {
		    tmrStart(ptpClock, UNICAST_GRANT, 1);
		}
		if(global->clockUpdateTimeout > 0) {
			tmrStart(ptpClock, CLOCK_UPDATE, global->clockUpdateTimeout);
		}
		initClock(global, ptpClock);

		ptpClock->waitingForFollow = FALSE;
		ptpClock->waitingForDelayResp = FALSE;

		if(global->calibrationDelay) {
			ptpClock->isCalibrated = FALSE;
		}

		// FIXME: clear these vars inside initclock
		clearTime(&ptpClock->delay_req_send_time);
		clearTime(&ptpClock->delay_req_receive_time);
		clearTime(&ptpClock->pdelay_req_send_time);
		clearTime(&ptpClock->pdelay_req_receive_time);
		clearTime(&ptpClock->pdelay_resp_send_time);
		clearTime(&ptpClock->pdelay_resp_receive_time);
		
		tmrStart(ptpClock, OPERATOR_MESSAGES,
			   OPERATOR_MESSAGES_INTERVAL);
		
		tmrStart(ptpClock, ANNOUNCE_RECEIPT,
			   (ptpClock->portDS.announceReceiptTimeout) *
			   (pow(2,ptpClock->portDS.logAnnounceInterval)));

		tmrStart(ptpClock, SYNC_RECEIPT, max(
		   (ptpClock->portDS.announceReceiptTimeout) * (pow(2,ptpClock->portDS.logAnnounceInterval)),
		   MISSED_MESSAGES_MAX * (pow(2,ptpClock->portDS.logSyncInterval))
		));

		if(ptpClock->portDS.delayMechanism == E2E) {
		    tmrStart(ptpClock, DELAY_RECEIPT, max(
		    (ptpClock->portDS.announceReceiptTimeout) * (pow(2,ptpClock->portDS.logAnnounceInterval)),
			MISSED_MESSAGES_MAX * (pow(2,ptpClock->portDS.logMinDelayReqInterval))));
		}
		if(ptpClock->portDS.delayMechanism == P2P) {
		    tmrStart(ptpClock, DELAY_RECEIPT, max(
		    (ptpClock->portDS.announceReceiptTimeout) * (pow(2,ptpClock->portDS.logAnnounceInterval)),
			MISSED_MESSAGES_MAX * (pow(2,ptpClock->portDS.logMinPdelayReqInterval))));
		}

		/*
		 * Previously, this state transition would start the
		 * delayreq timer immediately.  However, if this was
		 * faster than the first received sync, then the servo
		 * would drop the delayResp. Now, we only start the
		 * timer after we receive the first sync (in
		 * handle_sync())
		 */
		ptpClock->syncWaiting = TRUE;
		ptpClock->delayRespWaiting = TRUE;
		ptpClock->announceTimeouts = 0;
		setPortState(ptpClock, PTP_SLAVE);
		ptpClock->followUpGap = 0;

		if(global->oFilterMSConfig.enabled) {
			ptpClock->oFilterMS.reset(&ptpClock->oFilterMS);
		}
		if(global->oFilterSMConfig.enabled) {
			ptpClock->oFilterSM.reset(&ptpClock->oFilterSM);
		}
		if(global->filterMSOpts.enabled) {
			resetDoubleMovingStatFilter(ptpClock->filterMS);
		}
		if(global->filterSMOpts.enabled) {
			resetDoubleMovingStatFilter(ptpClock->filterSM);
		}
		clearPtpEngineSlaveStats(&ptpClock->slaveStats);

		tmrStart(ptpClock, STATISTICS_UPDATE, global->statsUpdateInterval);

		break;
	default:
		DBG("to unrecognized state\n");
		break;
	}

	if (global->logStatistics)
		logStatistics(ptpClock);
}

/* handle actions and events for 'port_state' */
static void
doState(GlobalConfig *global, PtpClock *ptpClock)
{
	UInteger8 state;
	
	ptpClock->message_activity = FALSE;
	ptpClock->disabled = global->portDisabled;

	if (ptpClock->disabled && (ptpClock->portDS.portState != PTP_DISABLED)) {
	    toState(PTP_DISABLED, global, ptpClock);
	    return;
	}

	/* Process record_update (BMC algorithm) before everything else */
	switch (ptpClock->portDS.portState)
	{
	case PTP_INITIALIZING:
	    if (ptpClock->disabled) {
		toState(PTP_DISABLED, global, ptpClock);
	    } else {
		toState(PTP_LISTENING, global, ptpClock);
	    }
	    if(global->statusLog.logEnabled)
		writeStatusFile(ptpClock, global, TRUE);
	    return;
	case PTP_DISABLED:
		if(!ptpClock->disabled) {
		    toState(PTP_LISTENING, global, ptpClock);
		}
		break;
		
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
			state = bmc(ptpClock->foreign, global, ptpClock);
			if(state != ptpClock->portDS.portState)
				toState(state, global, ptpClock);
		}
		break;
		
	default:
		break;
	}
	
	switch (ptpClock->portDS.portState)
	{
	case PTP_FAULTY:
		/* wait for timeout (only for internal faults, reported via ptpInternalFault() ) */

		if(tmrExpired(ptpClock, PORT_FAULT)) {
		    DBG("event FAULT_CLEARED\n");
		    toState(PTP_INITIALIZING, global, ptpClock);
		}

		break;
		
	case PTP_LISTENING:
	case PTP_UNCALIBRATED:
	// passive mode behaves like the SLAVE state, in order to wait for the announce timeout of the current active master
	case PTP_PASSIVE:
	case PTP_SLAVE:
		
		/*
		 * handle SLAVE timers:
		 *   - No Announce message was received
		 *   - Time to send new delayReq  (miss of delayResp is not monitored explicitelly)
		 */
		if (tmrExpired(ptpClock, ANNOUNCE_RECEIPT)) {
			DBG("event ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES\n");

			if(!ptpClock->defaultDS.slaveOnly &&
			   ptpClock->defaultDS.clockQuality.clockClass != SLAVE_ONLY_CLOCK_CLASS) {
				ptpClock->number_foreign_records = 0;
				ptpClock->foreign_record_i = 0;
				ptpClock->bestMaster = NULL;
				m1(global,ptpClock);
				toState(PTP_MASTER, global, ptpClock);
				/* in case if we were not allowed to go into master state */
				if(ptpClock->portDS.portState != PTP_MASTER) {
				    toState(PTP_PASSIVE, global, ptpClock);
				}

			} else if(ptpClock->portDS.portState != PTP_LISTENING) {
				/* stop statistics updates */
				tmrStop(ptpClock, STATISTICS_UPDATE);

				if(ptpClock->announceTimeouts < global->announceTimeoutGracePeriod) {
				/*
				* Don't reset yet - just disqualify current GM.
				* If another live master exists, it will be selected,
				* otherwise, timer will cycle and we will reset.
				* Also don't clear the FMR just yet.
				*/
				if (!ptpClock->bestMaster->disqualified) {
					ptpClock->bestMaster->disqualified = TRUE;
					WARNING("GM announce timeout, disqualified current best GM\n");
					ptpClock->counters.announceTimeouts++;
				}

				ptpNetworkRefresh(ptpClock);

				if (global->announceTimeoutGracePeriod > 0) ptpClock->announceTimeouts++;

					INFO("Waiting for new master, %d of %d attempts\n",ptpClock->announceTimeouts,global->announceTimeoutGracePeriod);
				} else {
					WARNING("No active masters present. Resetting port.\n");
					ptpClock->number_foreign_records = 0;
					ptpClock->foreign_record_i = 0;
					ptpClock->bestMaster = NULL;
					toState(PTP_LISTENING, global, ptpClock);
				}
			} else {
				/*
				 * Cycle into LISTENING
				 */
				    toState(PTP_LISTENING, global, ptpClock);
                                }

                }

		/* Reset the slave if clock update timeout configured */
		if ( ptpClock->portDS.portState == PTP_SLAVE && (global->clockUpdateTimeout > 0) &&
		    tmrExpired(ptpClock, CLOCK_UPDATE)) {
			if(global->noAdjust) {
				tmrStart(ptpClock, CLOCK_UPDATE, global->clockUpdateTimeout);
			} else {
			    WARNING("No offset updates in %d seconds - resetting slave\n",
				global->clockUpdateTimeout);
			    toState(PTP_LISTENING, global, ptpClock);
			}
		}

		if(ptpClock->portDS.portState==PTP_SLAVE && global->calibrationDelay && !ptpClock->isCalibrated) {
			if(tmrExpired(ptpClock, CALIBRATION_DELAY)) {
				ptpClock->isCalibrated = TRUE;
				NOTICE("PTP offset computation calibrated\n");
			} else if(!tmrRunning(ptpClock, CALIBRATION_DELAY)) {
			    tmrStart(ptpClock, CALIBRATION_DELAY, global->calibrationDelay);
			}
		}

		if (ptpClock->portDS.delayMechanism == E2E) {
			if(tmrExpired(ptpClock, DELAYREQ_INTERVAL)) {
				DBG("event DELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				/* if unicast negotiation is enabled, only request if granted */
				if(!global->unicastNegotiation ||
					(ptpClock->parentGrants &&
					    ptpClock->parentGrants->grantData[DELAY_RESP_INDEXED].granted)) {
						issueDelayReq(global,ptpClock);
				}
			}
		} else if (ptpClock->portDS.delayMechanism == P2P) {
			if (tmrExpired(ptpClock, PDELAYREQ_INTERVAL)) {
				DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				/* if unicast negotiation is enabled, only request if granted */
				if(!global->unicastNegotiation ||
					( ptpClock->peerGrants.grantData[PDELAY_RESP_INDEXED].granted)) {
					    issuePdelayReq(global,ptpClock);
				}
			}

			/* FIXME: Path delay should also rearm its timer with the value received from the Master */
		}

                if (ptpClock->timePropertiesDS.leap59 || ptpClock->timePropertiesDS.leap61)
                        DBGV("seconds to midnight: %.3f\n",secondsToMidnight());

                /* leap second period is over */
                if(tmrExpired(ptpClock, LEAP_SECOND_PAUSE) &&
                    ptpClock->leapSecondInProgress) {
                            /*
                             * do not unpause offset calculation just
                             * yet, just indicate and it will be
                             * unpaused in handleAnnounce()
                            */
                            ptpClock->leapSecondPending = FALSE;
                            tmrStop(ptpClock, LEAP_SECOND_PAUSE);
                    }
		/* check if leap second is near and if we should pause updates */
		if( ptpClock->leapSecondPending &&
		    !ptpClock->leapSecondInProgress &&
		    (secondsToMidnight() <=
		    getPauseAfterMidnight(ptpClock->portDS.logAnnounceInterval,
			global->leapSecondPausePeriod))) {
                            WARNING("Leap second event imminent - pausing "
				    "clock and offset updates\n");
                            ptpClock->leapSecondInProgress = TRUE;
			    /*
			     * start pause timer from now until [pause] after
			     * midnight, plus an extra second if inserting
			     * a leap second
			     */
			    tmrStart(ptpClock, LEAP_SECOND_PAUSE,
				       ((secondsToMidnight() +
				       (int)ptpClock->timePropertiesDS.leap61) +
				       getPauseAfterMidnight(ptpClock->portDS.logAnnounceInterval,
					    global->leapSecondPausePeriod)));
		}

/* Update PTP slave statistics from online statistics containers */
		if (tmrExpired(ptpClock, STATISTICS_UPDATE)) {
/* WOJ:CHECK */
			updatePtpEngineStats(ptpClock, global);
		}

		SET_ALARM(ALRM_NO_SYNC, tmrExpired(ptpClock, SYNC_RECEIPT));
		SET_ALARM(ALRM_NO_DELAY, tmrExpired(ptpClock, DELAY_RECEIPT));

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
		    (secondsToMidnight() < global->leapSecondNoticePeriod)) {
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
		    getPauseAfterMidnight(ptpClock->portDS.logAnnounceInterval,
			global->leapSecondPausePeriod))) {
                            WARNING("Leap second event imminent - pausing "
				    "event message processing\n");
                            ptpClock->leapSecondInProgress = TRUE;
			    /*
			     * start pause timer from now until [pause] after
			     * midnight, plus an extra second if inserting
			     * a leap second
			     */
			    tmrStart(ptpClock, LEAP_SECOND_PAUSE,
				       ((secondsToMidnight() +
				       (int)ptpClock->timePropertiesDS.leap61) +
				       getPauseAfterMidnight(ptpClock->portDS.logAnnounceInterval,
					    global->leapSecondPausePeriod)));
		}

                /* leap second period is over */
                if(tmrExpired(ptpClock, LEAP_SECOND_PAUSE) &&
                    ptpClock->leapSecondInProgress) {
                            ptpClock->leapSecondPending = FALSE;
                            tmrStop(ptpClock, LEAP_SECOND_PAUSE);
			    ptpClock->leapSecondInProgress = FALSE;
                            NOTICE("Leap second event over - resuming "
				    "event message processing\n");
                }

		if (tmrExpired(ptpClock, ANNOUNCE_INTERVAL)) {
			DBGV("event ANNOUNCE_INTERVAL_TIMEOUT_EXPIRES\n");
			/* restart the timer with current interval in case if it changed */
			if(pow(2,ptpClock->portDS.logAnnounceInterval) != tmrInterval(ptpClock, ANNOUNCE_INTERVAL)) {
				tmrStart(ptpClock, ANNOUNCE_INTERVAL,
					pow(2,ptpClock->portDS.logAnnounceInterval));
			}
			issueAnnounce(global, ptpClock);
		}

		if (ptpClock->portDS.delayMechanism == P2P) {
			if (tmrExpired(ptpClock, PDELAYREQ_INTERVAL)) {
				DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				/* if unicast negotiation is enabled, only request if granted */
				if(!global->unicastNegotiation ||
					( ptpClock->peerGrants.grantData[PDELAY_RESP_INDEXED].granted)) {
					    issuePdelayReq(global,ptpClock);
				}
			}
		}

		if (tmrExpired(ptpClock, SYNC_INTERVAL)) {
			DBGV("event SYNC_INTERVAL_TIMEOUT_EXPIRES\n");
			/* re-arm timer if changed */
			if(pow(2,ptpClock->portDS.logSyncInterval) != tmrInterval(ptpClock, SYNC_INTERVAL)) {
				tmrStart(ptpClock, SYNC_INTERVAL,
					pow(2,ptpClock->portDS.logSyncInterval));
			}
			issueSync(global, ptpClock);
		}

		if(!ptpClock->warnedUnicastCapacity) {
		    if(ptpClock->slaveCount >= UNICAST_MAX_DESTINATIONS ||
			ptpClock->unicastDestinationCount >= UNICAST_MAX_DESTINATIONS) {
			    if(global->transportMode == TMODE_UC) {
				WARNING("Maximum unicast slave capacity reached: %d\n",UNICAST_MAX_DESTINATIONS);
				ptpClock->warnedUnicastCapacity = TRUE;
			    }
		    }
		}

		if (ptpClock->defaultDS.slaveOnly || ptpClock->defaultDS.clockQuality.clockClass == SLAVE_ONLY_CLOCK_CLASS)
			toState(PTP_LISTENING, global, ptpClock);

		break;
#endif /* PTPD_SLAVE_ONLY */

	default:
		DBG("(doState) do unrecognized state\n");
		break;
	}

	if(global->periodicUpdates && tmrExpired(ptpClock, PERIODIC_INFO)) {
		periodicUpdate(global, ptpClock);
	}

        if(global->statusLog.logEnabled && tmrExpired(ptpClock, STATUSFILE_UPDATE)) {
                writeStatusFile(ptpClock,global,TRUE);
		/* ensures that the current updare interval is used */
		tmrStart(ptpClock, STATUSFILE_UPDATE,global->statusFileUpdateInterval);
        }
}

static Boolean
isFromCurrentParent(const PtpClock *ptpClock, const MsgHeader* header)
{

	return(!memcmp(
		ptpClock->parentDS.parentPortIdentity.clockIdentity,
		header->sourcePortIdentity.clockIdentity,
		CLOCK_IDENTITY_LENGTH)	&&
		(ptpClock->parentDS.parentPortIdentity.portNumber ==
		 header->sourcePortIdentity.portNumber));

}

/* apply any corrections and manglings required to the timestamp */
static void
timestampCorrection(const GlobalConfig * global, PtpClock *ptpClock, TimeInternal *timeStamp)
{

	TimeInternal fudge = {0,0};
	if(global->leapSecondHandling == LEAP_SMEAR && (ptpClock->leapSecondPending)) {
	DBG("Leap second smear: correction %.09f ns, seconds to midnight %f, leap smear period %d\n", ptpClock->leapSmearFudge,
		secondsToMidnight(), global->leapSecondSmearPeriod);
	    if(secondsToMidnight() <= global->leapSecondSmearPeriod) {
		ptpClock->leapSmearFudge = 1.0 - (secondsToMidnight() + 0.0) / (global->leapSecondSmearPeriod+0.0);
		if(ptpClock->timePropertiesDS.leap59) {
		    ptpClock->leapSmearFudge *= -1;
		}
		fudge = doubleToTimeInternal(ptpClock->leapSmearFudge);
	    }
	}

	if(ptpClock->portDS.portState == PTP_SLAVE && ptpClock->leapSecondPending && !ptpClock->leapSecondInProgress) {
	    addTime(timeStamp, timeStamp, &fudge);
	}

}

void
processPtpData(PtpClock *ptpClock, TimeInternal* timeStamp, ssize_t length, void *src, void *dst)
{

    Boolean isFromSelf;
    MsgHeader *header;

    PtpMessage m;
    PtpTlv *tlv;
    int ret;

    if(length == 0) {
	DBG("Received an empty PTP message (this should not happen) - ignoring\n");
	    return;
    }

    if (length < 0) {
	DBG("Error while receiving message");
	ptpInternalFault(ptpClock);
	ptpClock->counters.messageRecvErrors++;
	return;
    }

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
    if (respectUtcOffset(ptpClock->global, ptpClock)) {
	timeStamp->seconds += ptpClock->timePropertiesDS.currentUtcOffset;
    }

    ptpClock->message_activity = TRUE;

    if (length < HEADER_LENGTH) {
	DBG("Error: message shorter than header length (%d < %d)\n", length, HEADER_LENGTH);
	ptpClock->counters.messageFormatErrors++;
	return;
    }

    msgUnpackHeader(ptpClock->msgIbuf, &ptpClock->msgTmpHeader);
    header = &ptpClock->msgTmpHeader;

    if((header->messageType < PTP_GENERAL_MSGCLASS) && ptpClock->leapSecondInProgress) {
	DBG("Leap second in progress - will not process event message\n");
	return;
    }

    /*Spec 9.5.2.2*/
    isFromSelf = !cmpPortIdentity(&ptpClock->portDS.portIdentity, &header->sourcePortIdentity);

    memset(&m, 0, sizeof(PtpMessage));

    header->ptpmon = FALSE;
    header->mtie = FALSE;

    if(ptpClock->global->ptpMonEnabled && !isFromSelf && (header->messageType == DELAY_REQ) &&
    (header->messageLength > DELAY_REQ_LENGTH)) {

	ret  = unpackPtpMessage(&m, ptpClock->msgIbuf, ptpClock->msgIbuf + length);

	if (ret > 0) {
		header->ptpmon = FALSE;
		header->mtie = FALSE;
		for(tlv = m.firstTlv; tlv != NULL; tlv = tlv->next) {
		    if(tlv->tlvType == PTP_TLVTYPE_PTPMON_REQUEST) {
			DBG("PTPMON REQUEST TLV RECEIVED in DLYREQ\n");
			header->ptpmon = TRUE;
			ptpClock->counters.ptpMonReqReceived++;
		    }
		    if(tlv->tlvType == PTP_TLVTYPE_PTPMON_MTIE_REQUEST) {
			DBG("PTPMON MTIE REQUEST TLV RECEIVED in DLYREQ\n");
			header->mtie = TRUE;
			ptpClock->counters.ptpMonMtieReqReceived++;
		    }
		}
	    freePtpMessage(&m);
	}

    }

#if 0
    /* lib1588 testing */


    printf("**** BEGIN lib1588 unpack test\n");


    printf("==== Unpacking received data using lib1588\n");
    int bob = unpackPtpMessage(&m, ptpClock->msgIbuf, ptpClock->msgIbuf + length);
    printf("==== RETURNED: %d\n", bob);

    if((m.header.messageType == PTP_MSGTYPE_SIGNALING) ||
	(m.header.messageType == PTP_MSGTYPE_MANAGEMENT)
	) {
	printf("==== Done unpacking. Message:\n");
	displayPtpOctetBuf(ptpClock->msgIbuf, "ibuf", length);
	displayPtpMessage(&m);
    }


    printf("**** END lib1588 unpack test\n");

    freePtpMessage(&m);

#endif /* PTPD_EXPERIMENTAL */

    if (header->versionPTP != ptpClock->portDS.versionNumber) {
	DBG("ignore version %d message\n", header->versionPTP);
	ptpClock->counters.discardedMessages++;
	ptpClock->counters.versionMismatchErrors++;
	return;
    }

    if(header->domainNumber != ptpClock->defaultDS.domainNumber) {
	Boolean domainOK = FALSE;
	int i = 0;
	if (ptpClock->global->unicastNegotiation && ptpClock->unicastDestinationCount) {
	    for (i = 0; i < ptpClock->unicastDestinationCount; i++) {
		if(header->domainNumber == ptpClock->unicastGrants[i].domainNumber) {
		    domainOK = TRUE;
		    DBG("Accepted message type %s from domain %d (unicast neg)\n",
			getMessageTypeName(header->messageType),header->domainNumber);
		    break;
		}
	    }
	}

	if(ptpClock->defaultDS.slaveOnly && ptpClock->global->anyDomain) {
		DBG("anyDomain enabled: accepting announce from domain %d (we are %d)\n",
			header->domainNumber,
			ptpClock->defaultDS.domainNumber);
		domainOK = TRUE;
	}

	if(header->ptpmon || header->mtie) {
		if(ptpClock->global->ptpMonAnyDomain || (header->domainNumber == ptpClock->global->ptpMonDomainNumber)) {
		    DBG("accepted ptpMon message from domain %d (we are %d)\n",
			header->domainNumber,
			ptpClock->defaultDS.domainNumber);
		    domainOK = TRUE;
		}
	}

	if(!domainOK) {
		DBG("Ignored message %s received from %d domain\n",
			getMessageTypeName(header->messageType),
			header->domainNumber);
		ptpClock->portDS.lastMismatchedDomain = header->domainNumber;
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.domainMismatchErrors++;

		SET_ALARM(ALRM_DOMAIN_MISMATCH, ((ptpClock->counters.domainMismatchErrors >= DOMAIN_MISMATCH_MIN) &&
		ptpClock->counters.domainMismatchErrors == getPtpPortRxPackets(ptpClock)));

		return;
	}
    } else {
	SET_ALARM(ALRM_DOMAIN_MISMATCH, FALSE);
    }

    /* received a UNICAST message */
    if((header->flagField0 & PTP_UNICAST) == PTP_UNICAST) {
    /* in multicast mode, accept only management unicast messages, in hybrid mode accept only unicast delay messages */
        if((ptpClock->global->transportMode == TMODE_MC && header->messageType != MANAGEMENT) ||
    	(ptpClock->global->transportMode == TMODE_MIXED && header->messageType != DELAY_REQ &&
	    header->messageType != DELAY_RESP)) {
		if(!header->ptpmon && !header->mtie) {
		    DBG("ignored unicast message in multicast mode%d\n");
		    ptpClock->counters.discardedMessages++;
		    return;
		}
	    }
    /* received a MULTICAST message */
    } else {
	/* in unicast mode, accept only management multicast messages */
	if(ptpClock->global->transportMode == TMODE_UC && header->messageType != MANAGEMENT) {
	    DBG("ignored multicast message in unicast mode%d\n");
	    ptpClock->counters.discardedMessages++;
	    return;
	}
    }

    /* what shall we do with the drunken sailor? */
    timestampCorrection(ptpClock->global, ptpClock, timeStamp);

    /*
     * subtract the inbound latency adjustment if it is not a loop
     *  back and the time stamp seems reasonable
     */
    if (!isFromSelf && timeStamp->seconds > 0)
	subTime(timeStamp, timeStamp, &ptpClock->global->inboundLatency);

    DBG("      ==> %s message received, sequence %d\n", getMessageTypeName(header->messageType),
							header->sequenceId);

    /*
     *  on the table below, note that only the event messsages are passed the local time,
     *  (collected by us by loopback+kernel TS, and adjusted with UTC seconds
     *
     *  (SYNC / DELAY_REQ / PDELAY_REQ / PDELAY_RESP)
     */
    switch(header->messageType)
    {
    case ANNOUNCE:
	handleAnnounce(&ptpClock->msgTmpHeader,
	           length, isFromSelf, src, ptpClock->global, ptpClock);
	break;
    case SYNC:
	handleSync(&ptpClock->msgTmpHeader,
	       length, timeStamp, isFromSelf, src, dst, ptpClock->global, ptpClock);
	break;
    case FOLLOW_UP:
	handleFollowUp(&ptpClock->msgTmpHeader,
	           length, isFromSelf, ptpClock->global, ptpClock);
	break;
    case DELAY_REQ:
	handleDelayReq(&ptpClock->msgTmpHeader,
	           length, timeStamp, src, isFromSelf, ptpClock->global, ptpClock);
	break;
    case PDELAY_REQ:
	handlePdelayReq(&ptpClock->msgTmpHeader,
		length, timeStamp, src, isFromSelf, ptpClock->global, ptpClock);
	break;
    case DELAY_RESP:
	handleDelayResp(&ptpClock->msgTmpHeader,
		length, ptpClock->global, ptpClock);
	break;
    case PDELAY_RESP:
	handlePdelayResp(&ptpClock->msgTmpHeader,
		 timeStamp, length, isFromSelf, src, dst, ptpClock->global, ptpClock);
	break;
    case PDELAY_RESP_FOLLOW_UP:
	handlePdelayRespFollowUp(&ptpClock->msgTmpHeader,
		     length, isFromSelf, ptpClock->global, ptpClock);
	break;
    case MANAGEMENT:
	handleManagement(&ptpClock->msgTmpHeader,
		 isFromSelf, src, ptpClock->global, ptpClock);
	break;
    case SIGNALING:
       handleSignaling(&ptpClock->msgTmpHeader, isFromSelf,
                src, ptpClock->global, ptpClock);
	break;
    default:
	DBG("handle: unrecognized message\n");
	ptpClock->counters.discardedMessages++;
	ptpClock->counters.unknownMessages++;
	break;
    }

    if (ptpClock->global->displayPackets)
	msgDump(ptpClock);


}

/*spec 9.5.3*/
static void
handleAnnounce(MsgHeader *header, ssize_t length,
	       Boolean isFromSelf, void *src, const GlobalConfig *global, PtpClock *ptpClock)
{

	UnicastGrantTable *nodeTable = NULL;
	UInteger8 localPreference = LOWEST_LOCALPREFERENCE;

	DBGV("HandleAnnounce : Announce message received : \n");

	if(length < ANNOUNCE_LENGTH) {
		DBG("Error: Announce message too short\n");
		ptpClock->counters.messageFormatErrors++;
		return;
	}

	/* if we're ignoring announces (telecom) */
	if(ptpClock->defaultDS.clockQuality.clockClass <= 127 && global->disableBMCA) {
		DBG("unicast master only and disableBMCA: dropping Announce\n");
		ptpClock->counters.discardedMessages++;
		return;
	}

	if(global->unicastNegotiation && global->transportMode == TMODE_UC) {

		nodeTable = findUnicastGrants(&header->sourcePortIdentity, NULL,
							ptpClock->unicastGrants, &ptpClock->grantIndex, UNICAST_MAX_DESTINATIONS,
							FALSE);
		if(nodeTable == NULL || !(nodeTable->grantData[ANNOUNCE_INDEXED].granted)) {
			if(!global->unicastAcceptAny) {
			    DBG("Ignoring announce from master: unicast transmission not granted\n");
			    ptpClock->counters.discardedMessages++;
			    return;
			}
		} else {
		    localPreference = nodeTable->localPreference;
		    nodeTable->grantData[ANNOUNCE_INDEXED].receiving = header->sequenceId;
		}
	}

	switch (ptpClock->portDS.portState) {
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
		if(global->requireUtcValid && !IS_SET(header->flagField1, UTCV)) {
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
	   		s1(header,&ptpClock->msgTmp.announce,ptpClock, global);

			/* update current master in the fmr as well */
			memcpy(&ptpClock->bestMaster->header,
			       header,sizeof(MsgHeader));
			memcpy(&ptpClock->bestMaster->announce,
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
					if(global->leapSecondHandling==LEAP_STEP) {
						ptpClock->clockControl.stepRequired = TRUE;
					}
					/* reverse frequency shift */
					if(global->leapSecondHandling==LEAP_SMEAR){
					    /* the flags are probably off by now, using the shift sign to detect leap second type */
					    if(ptpClock->leapSmearFudge < 0) {
						DBG("Reversed LEAP59 smear frequency offset\n");
						((ClockDriver*)ptpClock->clockDriver)->servo.integral += 1E9 / global->leapSecondSmearPeriod;
					    }
					    if(ptpClock->leapSmearFudge > 0) {
						DBG("Reversed LEAP61 smear frequency offset\n");
						((ClockDriver*)ptpClock->clockDriver)->servo.integral -= 1E9 / global->leapSecondSmearPeriod;
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
			if(header->domainNumber == ptpClock->defaultDS.domainNumber) {
	   			tmrStart(ptpClock, ANNOUNCE_RECEIPT,
				   (ptpClock->portDS.announceReceiptTimeout) *
				   (pow(2,ptpClock->portDS.logAnnounceInterval)));
			}

		if(!tmrRunning(ptpClock, STATISTICS_UPDATE)) {
			tmrStart(ptpClock, STATISTICS_UPDATE, global->statsUpdateInterval);
		}

			if (global->announceTimeoutGracePeriod &&
				ptpClock->announceTimeouts > 0) {
					NOTICE("Received Announce message from current master - cancelling timeout\n");
					ptpClock->announceTimeouts = 0;
					/* we are available for clock control again */
					ptpClock->clockControl.available = TRUE;
			}

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
			addForeign(ptpClock->msgIbuf,header,ptpClock,localPreference,src);
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
		if(global->requireUtcValid && !IS_SET(header->flagField1, UTCV)) {
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
			 * this should be p1(ptpClock, global);
			 */
			/* update datasets (file bmc.c) */
			s1(header,&ptpClock->msgTmp.announce,ptpClock, global);

			/* update current best master in the fmr as well */
			memcpy(&ptpClock->bestMaster->header,
			       header,sizeof(MsgHeader));
			memcpy(&ptpClock->bestMaster->announce,
			       &ptpClock->msgTmp.announce,sizeof(MsgAnnounce));

			DBG("___ Announce: received Announce from current Master, so reset the Announce timer\n\n");

			/* Reset Timer handling Announce receipt timeout: different domain my get here
			  when using anyDomain.*/
			tmrStart(ptpClock, ANNOUNCE_RECEIPT,
			   (ptpClock->portDS.announceReceiptTimeout) *
			   (pow(2,ptpClock->portDS.logAnnounceInterval)));

		} else {
			/*addForeign takes care of AnnounceUnpacking*/
			/* the actual decision to change masters is only done in  doState() / record_update == TRUE / bmc() */
			/* the original code always called: addforeign(new master) + ptpTimerStart(announce) */

			DBG("___ Announce: received Announce from another master, will add to the list, as it might be better\n\n");
			DBGV("this is to be decided immediatly by bmc())\n\n");
			addForeign(ptpClock->msgIbuf,header,ptpClock,localPreference,src);
		}
		break;

		
	case PTP_MASTER:
	case PTP_LISTENING:           /* listening mode still causes timeouts in order to send IGMP refreshes */
	default :
		if (isFromSelf) {
			DBGV("HandleAnnounce : Ignore message from self \n");
			return;
		}
		if(global->requireUtcValid && !IS_SET(header->flagField1, UTCV)) {
			ptpClock->counters.ignoredAnnounce++;
			return;
		}
		ptpClock->counters.announceMessagesReceived++;
		DBGV("Announce message from another foreign master\n");
		addForeign(ptpClock->msgIbuf,header,ptpClock, localPreference,src);
		ptpClock->record_update = TRUE;    /* run BMC() as soon as possible */
		break;

	} /* switch on (port_state) */
}

/* store transportAddress in an index table */
static void
indexSync(TimeInternal *timeStamp, UInteger16 sequenceId, void *protocolAddress, SyncDestEntry *index)
{

    uint32_t hash = 0;

#ifdef RUNTIME_DEBUG
	tmpstr(strAddr, ptpAddrStrLen(protocolAddress));
#endif /* RUNTIME_DEBUG */

    if(timeStamp == NULL || index == NULL) {
	return;
    }

    hash = fnvHash(timeStamp, sizeof(TimeInternal), UNICAST_MAX_DESTINATIONS);

    if(!ptpAddrIsEmpty(index[hash].protocolAddress)) {
	DBG("indexSync: hash collision - clearing entry %s:%04x\n", ptpAddrToString(strAddr, strAddr_len, protocolAddress), hash);
	ptpAddrClear(index[hash].protocolAddress);
    } else {
	DBG("indexSync: indexed successfully %s:%04x\n", ptpAddrToString(strAddr, strAddr_len, protocolAddress), hash);
	ptpAddrCopy(index[hash].protocolAddress, protocolAddress);
    }

}

/* sync destination index lookup */
static void*
lookupSyncIndex(TimeInternal *timeStamp, UInteger16 sequenceId, SyncDestEntry *index)
{

    uint32_t hash = 0;

    if(timeStamp == NULL || index == NULL) {
	return NULL;
    }

    hash = fnvHash(timeStamp, sizeof(TimeInternal), UNICAST_MAX_DESTINATIONS);

    if(ptpAddrIsEmpty(index[hash].protocolAddress)) {
	DBG("lookupSyncIndex: cache miss\n");
	return NULL;
    } else {
	DBG("lookupSyncIndex: cache hit - old entry should be cleared\n");
	return index[hash].protocolAddress;
    }

}

/* iterative search for Sync destination for the given cached timestamp */
static void*
findSyncDestination(TimeInternal *timeStamp, const GlobalConfig *global, PtpClock *ptpClock)
{

    int i = 0;

    for(i = 0; i < UNICAST_MAX_DESTINATIONS; i++) {

	if(global->unicastNegotiation) {
		if( (timeStamp->seconds == ptpClock->unicastGrants[i].lastSyncTimestamp.seconds) &&
		    (timeStamp->nanoseconds == ptpClock->unicastGrants[i].lastSyncTimestamp.nanoseconds)) {
			clearTime(&ptpClock->unicastGrants[i].lastSyncTimestamp);
			return ptpClock->unicastGrants[i].protocolAddress;
		    }
	} else {
		if( (timeStamp->seconds == ptpClock->unicastDestinations[i].lastSyncTimestamp.seconds) &&
		    (timeStamp->nanoseconds == ptpClock->unicastDestinations[i].lastSyncTimestamp.nanoseconds)) {
			clearTime(&ptpClock->unicastDestinations[i].lastSyncTimestamp);
			return ptpClock->unicastDestinations[i].protocolAddress;
		    }
	}
    }

    return 0;

}

static void
handleSync(const MsgHeader *header, ssize_t length,
	   TimeInternal *tint, Boolean isFromSelf, void *src, void* dst,
	   const GlobalConfig *global, PtpClock *ptpClock)
{

	TimeInternal OriginTimestamp;
	TimeInternal correctionField;

	Boolean clearIndexEntry = FALSE;
	void* myDst = NULL;

	if((global->transportMode == TMODE_UC) && dst) {
	    myDst = dst;
	} else {
	    dst = NULL;
	}

	DBGV("Sync message received : \n");

	if (length < SYNC_LENGTH) {
		DBG("Error: Sync message too short\n");
		ptpClock->counters.messageFormatErrors++;
		return;
	}

	if(!isFromSelf && global->unicastNegotiation && global->transportMode == TMODE_UC) {
	    UnicastGrantTable *nodeTable = NULL;
	    nodeTable = findUnicastGrants(&header->sourcePortIdentity, NULL,
			ptpClock->unicastGrants, &ptpClock->grantIndex, UNICAST_MAX_DESTINATIONS,
			FALSE);
	    if(nodeTable != NULL) {
		nodeTable->grantData[SYNC_INDEXED].receiving = header->sequenceId;
	    }
	}

	switch (ptpClock->portDS.portState) {
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
			tmrStart(ptpClock, SYNC_RECEIPT, max(
			    (ptpClock->portDS.announceReceiptTimeout) * (pow(2,ptpClock->portDS.logAnnounceInterval)),
			    MISSED_MESSAGES_MAX * (pow(2,ptpClock->portDS.logSyncInterval))
			));

			/* We only start our own delayReq timer after receiving the first sync */
			if (ptpClock->syncWaiting) {
				ptpClock->syncWaiting = FALSE;

				NOTICE("Received first Sync from Master\n");

				if (ptpClock->portDS.delayMechanism == E2E)
					tmrStart(ptpClock, DELAYREQ_INTERVAL,
						   pow(2,ptpClock->portDS.logMinDelayReqInterval));
				else if (ptpClock->portDS.delayMechanism == P2P)
					tmrStart(ptpClock, PDELAYREQ_INTERVAL,
						   pow(2,ptpClock->portDS.logMinPdelayReqInterval));
			} else if ( global->syncSequenceChecking ) {
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

			ptpClock->portDS.logSyncInterval = header->logMessageInterval;

			/* this will be 0x7F for unicast so if we have a grant, use the granted value */
			if(global->unicastNegotiation && ptpClock->parentGrants
				&& ptpClock->parentGrants->grantData[SYNC_INDEXED].granted) {
				ptpClock->portDS.logSyncInterval =  ptpClock->parentGrants->grantData[SYNC_INDEXED].logInterval;
			}
			

			recordSync(header->sequenceId, tint);

			if ((header->flagField0 & PTP_TWO_STEP) == PTP_TWO_STEP) {
				DBG2("HandleSync: waiting for follow-up \n");
				ptpClock->defaultDS.twoStepFlag=TRUE;
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
				fromOriginTimestamp(&OriginTimestamp,
					       &ptpClock->msgTmp.sync.originTimestamp);
				ptpClock->lastOriginTimestamp = OriginTimestamp;
				updateOffset(&OriginTimestamp,
					     &ptpClock->sync_receive_time,
					     &ptpClock->ofm_filt,global,
					     ptpClock,&correctionField);
				checkOffset(global,ptpClock);
				if (ptpClock->clockControl.updateOK) {
					ptpClock->acceptedUpdates++;
					updateClock(global,ptpClock);
				}
				ptpClock->offsetUpdates++;
				
				ptpClock->defaultDS.twoStepFlag=FALSE;
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
		} if (ptpClock->defaultDS.twoStepFlag) {
			DBGV("HandleSync: going to send followup message\n");

			/* who do we send the followUp to? no destination given - try looking up index */
			if((global->transportMode == TMODE_UC) && !dst) {
				msgUnpackSync(ptpClock->msgIbuf,
					      &ptpClock->msgTmp.sync);
				fromOriginTimestamp(&OriginTimestamp, &ptpClock->msgTmp.sync.originTimestamp);
			    myDst = lookupSyncIndex(&OriginTimestamp, header->sequenceId, ptpClock->syncDestIndex);

#ifdef RUNTIME_DEBUG
			    {
				tmpstr(strAddr, ptpAddrStrLen(dst));
				DBG("handleSync: master sync dest cache hit: %s\n", 
				    ptpAddrToString(strAddr, strAddr_len, dst));
			    }
#endif /* RUNTIME_DEBUG */

			    if(myDst) {
				clearIndexEntry = TRUE;
			    } else {
				DBG("handleSync: master sync dest cache miss - searching\n");
				myDst = findSyncDestination(&OriginTimestamp, global, ptpClock);
				/* give up. Better than sending FollowUp to random destinations*/
				if(!myDst) {
				    DBG("handleSync: master sync dest not found for followUp. Giving up.\n");
				    return;
				} else {
				    DBG("unicast destination found.\n");
				}
			    }

			}

#ifndef PTPD_SLAVE_ONLY /* does not get compiled when building slave only */
			processSyncFromSelf(tint, global, ptpClock, myDst, header->sequenceId);
			/*
			 * clear sync index entry - has to be done here,
			 * otherwise lookupSyncIndex would have to clear it
			 * before returning it...
			 */
			if(clearIndexEntry) {
			    ptpAddrClear(myDst);
			}
#endif /* PTPD_SLAVE_ONLY */
			break;
		} else {
			DBGV("HandleSync: Sync message received from self\n");
		}
	}
}

static void
processSyncFromSelf(const TimeInternal * tint, const GlobalConfig * global, PtpClock * ptpClock, void *dst, const UInteger16 sequenceId) {

	TimeInternal timestamp;
	/*Add latency*/

	addTime(&timestamp, tint, &global->outboundLatency);
	/* Issue follow-up CORRESPONDING TO THIS SYNC */
	issueFollowup(&timestamp, global, ptpClock, dst, sequenceId);
}

static void
handleFollowUp(const MsgHeader *header, ssize_t length,
	       Boolean isFromSelf, const GlobalConfig *global, PtpClock *ptpClock)
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

	switch (ptpClock->portDS.portState)
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
			ptpClock->portDS.logSyncInterval = header->logMessageInterval;
			/* this will be 0x7F for unicast so if we have a grant, use the granted value */
			if(global->unicastNegotiation && ptpClock->parentGrants
				&& ptpClock->parentGrants->grantData[SYNC_INDEXED].granted) {
				ptpClock->portDS.logSyncInterval =  ptpClock->parentGrants->grantData[SYNC_INDEXED].logInterval;
			}

			if (ptpClock->waitingForFollow)	{
				if (ptpClock->recvSyncSequenceId ==
				     header->sequenceId) {
					ptpClock->followUpGap = 0;
					msgUnpackFollowUp(ptpClock->msgIbuf,
							  &ptpClock->msgTmp.follow);
					ptpClock->waitingForFollow = FALSE;
					fromOriginTimestamp(&preciseOriginTimestamp,
						       &ptpClock->msgTmp.follow.preciseOriginTimestamp);
					ptpClock->lastOriginTimestamp = preciseOriginTimestamp;
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
						     global,ptpClock,
						     &correctionField);
					checkOffset(global,ptpClock);
					if (ptpClock->clockControl.updateOK) {
						ptpClock->acceptedUpdates++;
						updateClock(global,ptpClock);
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
	       const TimeInternal *tint, void *src, Boolean isFromSelf,
	       const GlobalConfig *global, PtpClock *ptpClock)
{

	UnicastGrantTable *nodeTable = NULL;

	if(header->ptpmon || header->mtie) {
			msgUnpackHeader(ptpClock->msgIbuf,
					&ptpClock->delayReqHeader);
			ptpClock->delayReqHeader.ptpmon = header->ptpmon;
			ptpClock->delayReqHeader.mtie = header->mtie;
			ptpClock->counters.delayReqMessagesReceived++;

			issueDelayResp(tint,&ptpClock->delayReqHeader, src,
				       global,ptpClock);
			DBG("HANDLED %s%sDLYRESP\n", header->ptpmon ? "PTPMON " : "",
					    header->mtie ? "MTIE " : "" );
			return;
	}

	if (ptpClock->portDS.delayMechanism == E2E) {

		if(!isFromSelf && global->unicastNegotiation && global->transportMode == TMODE_UC) {
		    nodeTable = findUnicastGrants(&header->sourcePortIdentity, NULL,
				ptpClock->unicastGrants, &ptpClock->grantIndex, UNICAST_MAX_DESTINATIONS,
				FALSE);
		    if(nodeTable == NULL || !(nodeTable->grantData[DELAY_RESP_INDEXED].granted)) {
			DBG("Ignoring Delay Request from slave: unicast transmission not granted\n");
			ptpClock->counters.discardedMessages++;
			    return;
		    } else {
			nodeTable->grantData[DELAY_REQ].receiving = header->sequenceId;
		    }

		}
	

		DBG("delayReq message received : \n");
		
		if (length < DELAY_REQ_LENGTH) {
			DBG("Error: DelayReq message too short\n");
			ptpClock->counters.messageFormatErrors++;
			return;
		}

		switch (ptpClock->portDS.portState) {
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

				processDelayReqFromSelf(tint, global, ptpClock);

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

			issueDelayResp(tint,&ptpClock->delayReqHeader, src,
				       global,ptpClock);
			break;

		default:
			DBG("do unrecognized state2\n");
			ptpClock->counters.discardedMessages++;
			break;
		}
	} else if (ptpClock->portDS.delayMechanism == P2P) {/* (Peer to Peer mode) */
		DBG("Delay messages are ignored in Peer to Peer mode\n");
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.delayMechanismMismatchErrors++;
	/* no delay mechanism */
	} else {
		DBG("DelayReq ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}


static void
processDelayReqFromSelf(const TimeInternal * tint, const GlobalConfig * global, PtpClock * ptpClock) {

	ptpClock->waitingForDelayResp = TRUE;
	ptpClock->delay_req_send_time.seconds = tint->seconds;
	ptpClock->delay_req_send_time.nanoseconds = tint->nanoseconds;

	/*Add latency*/
	addTime(&ptpClock->delay_req_send_time,
		&ptpClock->delay_req_send_time,
		&global->outboundLatency);
	
	DBGV("processDelayReqFromSelf: %s %d\n",
	    dump_TimeInternal(&ptpClock->delay_req_send_time),
	    global->outboundLatency);
	
}

static void
handleDelayResp(const MsgHeader *header, ssize_t length,
		const GlobalConfig *global, PtpClock *ptpClock)
{

	if (ptpClock->portDS.delayMechanism == E2E) {

		TimeInternal requestReceiptTimestamp;
		TimeInternal correctionField;

		if(global->unicastNegotiation && global->transportMode == TMODE_UC) {
		    UnicastGrantTable *nodeTable = NULL;
		    nodeTable = findUnicastGrants(&header->sourcePortIdentity, NULL,
				ptpClock->unicastGrants, &ptpClock->grantIndex, UNICAST_MAX_DESTINATIONS,
				FALSE);
		    if(nodeTable != NULL) {
			nodeTable->grantData[DELAY_RESP_INDEXED].receiving = header->sequenceId;
		    } else {
			DBG("delayResp - unicast master not identified\n");
		    }
		}

		DBGV("delayResp message received : \n");

		if(length < DELAY_RESP_LENGTH) {
			DBG("Error: DelayResp message too short\n");
			ptpClock->counters.messageFormatErrors++;
			return;
		}

		switch(ptpClock->portDS.portState) {
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

			if ((memcmp(ptpClock->portDS.portIdentity.clockIdentity,
				    ptpClock->msgTmp.resp.requestingPortIdentity.clockIdentity,
				    CLOCK_IDENTITY_LENGTH) == 0) &&
			    (ptpClock->portDS.portIdentity.portNumber ==
			     ptpClock->msgTmp.resp.requestingPortIdentity.portNumber)
			    && isFromCurrentParent(ptpClock, header)) {
				DBG("==> Handle DelayResp (%d)\n",
					 header->sequenceId);

				tmrStart(ptpClock, DELAY_RECEIPT, max(
				    (ptpClock->portDS.announceReceiptTimeout) * (pow(2,ptpClock->portDS.logAnnounceInterval)),
					MISSED_MESSAGES_MAX * (pow(2,ptpClock->portDS.logMinDelayReqInterval))));

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

				fromOriginTimestamp(&requestReceiptTimestamp,
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

				updateDelay(&ptpClock->mpdIirFilter,
					    global,ptpClock, &correctionField);
				if (ptpClock->delayRespWaiting) {

					NOTICE("Received first Delay Response from Master\n");
				}

				if (global->logDelayReqOverride == 0) {
					DBGV("current delay_req: %d  new delay req: %d \n",
						ptpClock->portDS.logMinDelayReqInterval,
					header->logMessageInterval);
                        	    if (ptpClock->portDS.logMinDelayReqInterval != header->logMessageInterval) {
			
                    			if(header->logMessageInterval == UNICAST_MESSAGEINTERVAL &&
						global->autoDelayReqInterval) {

						if(global->unicastNegotiation && ptpClock->parentGrants && ptpClock->parentGrants->grantData[DELAY_RESP_INDEXED].granted) {
						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Delay Interval %d from master\n",
							    ptpClock->parentGrants->grantData[DELAY_RESP_INDEXED].logInterval);
						    }
						    ptpClock->portDS.logMinDelayReqInterval = ptpClock->parentGrants->grantData[DELAY_RESP_INDEXED].logInterval;
						} else {

						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Delay Interval %d from master (unicast-unknown) - overriding with %d\n",
							    header->logMessageInterval, global->logMinDelayReqInterval);
						    }
						    ptpClock->portDS.logMinDelayReqInterval = global->logMinDelayReqInterval;

						}	
					} else {
						/* Accept new DelayReq value from the Master */

						    NOTICE("Received new Delay Request interval %d from Master (was: %d)\n",
							     header->logMessageInterval, ptpClock->portDS.logMinDelayReqInterval );

						    // collect new value indicated by the Master
						    ptpClock->portDS.logMinDelayReqInterval = header->logMessageInterval;
						}
					/* FIXME: the actual rearming of this timer with the new value only happens later in doState()/issueDelayReq() */
				    }
				} else {

					if (ptpClock->portDS.logMinDelayReqInterval != global->logMinDelayReqInterval) {
						INFO("New Delay Request interval applied: %d (was: %d)\n",
							global->logMinDelayReqInterval, ptpClock->portDS.logMinDelayReqInterval);
					}
					ptpClock->portDS.logMinDelayReqInterval = global->logMinDelayReqInterval;
				}
				/* arm the timer again now that we have the correct delayreq interval */
				tmrStart(ptpClock, DELAY_RECEIPT, max(
				    (ptpClock->portDS.announceReceiptTimeout) * (pow(2,ptpClock->portDS.logAnnounceInterval)),
					MISSED_MESSAGES_MAX * (pow(2,ptpClock->portDS.logMinDelayReqInterval))));
			} else {

				DBG("HandledelayResp : delayResp doesn't match with the delayReq. \n");
				ptpClock->counters.discardedMessages++;
				break;
			}
			
			ptpClock->delayRespWaiting = FALSE;
		}
	} else if (ptpClock->portDS.delayMechanism == P2P) { /* (Peer to Peer mode) */
		DBG("Delay messages are disregarded in Peer to Peer mode \n");
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.delayMechanismMismatchErrors++;
	/* no delay mechanism */
	} else {
		DBG("DelayResp ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}

}


static void
handlePdelayReq(MsgHeader *header, ssize_t length,
		const TimeInternal *tint, void *src, Boolean isFromSelf,
		const GlobalConfig *global, PtpClock *ptpClock)
{

	UnicastGrantTable *nodeTable = NULL;

	if (ptpClock->portDS.delayMechanism == P2P) {

		if(!isFromSelf && global->unicastNegotiation && global->transportMode == TMODE_UC) {
		    nodeTable = findUnicastGrants(&header->sourcePortIdentity, NULL,
				ptpClock->unicastGrants, &ptpClock->grantIndex, UNICAST_MAX_DESTINATIONS,
				FALSE);
		    if(nodeTable == NULL || !(nodeTable->grantData[PDELAY_RESP_INDEXED].granted)) {
			DBG("Ignoring Peer Delay Request from peer: unicast transmission not granted\n");
			ptpClock->counters.discardedMessages++;
			    return;
		    } else {
			nodeTable->grantData[PDELAY_RESP_INDEXED].receiving = header->sequenceId;
		    }

		}

		DBGV("PdelayReq message received : \n");

		if(length < PDELAY_REQ_LENGTH) {
			DBG("Error: PdelayReq message too short\n");
			ptpClock->counters.messageFormatErrors++;
			return;
		}

		switch (ptpClock->portDS.portState ) {
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
				processPdelayReqFromSelf(tint, global, ptpClock);
				break;
			} else {
				ptpClock->counters.pdelayReqMessagesReceived++;
				msgUnpackHeader(ptpClock->msgIbuf,
						&ptpClock->PdelayReqHeader);
				issuePdelayResp(tint, header, src, global,
						ptpClock);	
				break;
			}
		default:
			DBG("do unrecognized state3\n");
			ptpClock->counters.discardedMessages++;
			break;
		}
	} else if (ptpClock->portDS.delayMechanism == E2E){/* (End to End mode..) */
		DBG("Peer Delay messages are disregarded in End to End "
		      "mode \n");
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.delayMechanismMismatchErrors++;
	/* no delay mechanism */
	} else {
		DBG("PdelayReq ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}

static void
processPdelayReqFromSelf(const TimeInternal * tint, const GlobalConfig * global, PtpClock * ptpClock) {
	/*
	 * Get sending timestamp from IP stack
	 * with SO_TIMESTAMP
	 */
	ptpClock->pdelay_req_send_time.seconds = tint->seconds;
	ptpClock->pdelay_req_send_time.nanoseconds = tint->nanoseconds;
			
	/*Add latency*/
	addTime(&ptpClock->pdelay_req_send_time,
		&ptpClock->pdelay_req_send_time,
		&global->outboundLatency);
}

static void
handlePdelayResp(const MsgHeader *header, TimeInternal *tint,
		 ssize_t length, Boolean isFromSelf, void *src, void *dst,
		 const GlobalConfig *global, PtpClock *ptpClock)
{
	if (ptpClock->portDS.delayMechanism == P2P) {

		void *myDst = NULL;

		if(dst) {
			myDst = dst;
		    } else {
			/* last resort: may cause PdelayRespFollowUp to be sent to incorrect destinations */
			myDst = ptpClock->lastPdelayRespDst;
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

		switch (ptpClock->portDS.portState ) {
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
			if (ptpClock->defaultDS.twoStepFlag && isFromSelf) {
				processPdelayRespFromSelf(tint, global, ptpClock, myDst, header->sequenceId);
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
			if ((!memcmp(ptpClock->portDS.portIdentity.clockIdentity,ptpClock->msgTmp.presp.requestingPortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH))
				 && ( ptpClock->portDS.portIdentity.portNumber == ptpClock->msgTmp.presp.requestingPortIdentity.portNumber))	{
				ptpClock->counters.pdelayRespMessagesReceived++;
                                /* Two Step Clock */
				if ((header->flagField0 & PTP_TWO_STEP) == PTP_TWO_STEP) {
					/*Store t4 (Fig 35)*/
					ptpClock->pdelay_resp_receive_time.seconds = tint->seconds;
					ptpClock->pdelay_resp_receive_time.nanoseconds = tint->nanoseconds;
					/*store t2 (Fig 35)*/
					fromOriginTimestamp(&requestReceiptTimestamp,
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
					updatePeerDelay (&ptpClock->mpdIirFilter,global,ptpClock,&correctionField,FALSE);
				if (global->logDelayReqOverride == 0) {
					DBGV("current pdelay_req: %d  new pdelay req: %d \n",
						ptpClock->portDS.logMinPdelayReqInterval,
					header->logMessageInterval);
                        	    if (ptpClock->portDS.logMinPdelayReqInterval != header->logMessageInterval) {
			
                    			if(header->logMessageInterval == UNICAST_MESSAGEINTERVAL &&
						global->autoDelayReqInterval) {

						if(global->unicastNegotiation && ptpClock->peerGrants.grantData[PDELAY_RESP_INDEXED].granted) {
						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Peer Delay Interval %d from peer\n",
							    ptpClock->peerGrants.grantData[PDELAY_RESP_INDEXED].logInterval);
						    }
						    ptpClock->portDS.logMinPdelayReqInterval = ptpClock->peerGrants.grantData[PDELAY_RESP_INDEXED].logInterval;
						} else {

						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Peer Delay Interval %d from peer (unicast-unknown) - overriding with %d\n",
							    header->logMessageInterval, global->logMinPdelayReqInterval);
						    }
						    ptpClock->portDS.logMinPdelayReqInterval = global->logMinPdelayReqInterval;

						}	
					} else {
						/* Accept new DelayReq value from the Master */

						    NOTICE("Received new Peer Delay Request interval %d from Master (was: %d)\n",
							     header->logMessageInterval, ptpClock->portDS.logMinPdelayReqInterval );

						    // collect new value indicated by the Master
						    ptpClock->portDS.logMinPdelayReqInterval = header->logMessageInterval;
				    }
				} else {

					if (ptpClock->portDS.logMinPdelayReqInterval != global->logMinPdelayReqInterval) {
						INFO("New Peer Delay Request interval applied: %d (was: %d)\n",
							global->logMinPdelayReqInterval, ptpClock->portDS.logMinPdelayReqInterval);
					}
					ptpClock->portDS.logMinPdelayReqInterval = global->logMinPdelayReqInterval;
				}
				
				}
	                    		ptpClock->delayRespWaiting = FALSE;
					tmrStart(ptpClock, DELAY_RECEIPT, max(
					    (ptpClock->portDS.announceReceiptTimeout) * (pow(2,ptpClock->portDS.logAnnounceInterval)),
						MISSED_MESSAGES_MAX * (pow(2,ptpClock->portDS.logMinPdelayReqInterval))));
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
	} else if (ptpClock->portDS.delayMechanism == E2E) { /* (End to End mode..) */
		DBG("Peer Delay messages are disregarded in End to End "
		      "mode \n");
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.delayMechanismMismatchErrors++;
	/* no delay mechanism */
	} else {
		DBG("PdelayResp ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}

static void
processPdelayRespFromSelf(const TimeInternal * tint, const GlobalConfig * global, PtpClock * ptpClock, void *dst, const UInteger16 sequenceId)
{
	TimeInternal timestamp;
	
	addTime(&timestamp, tint, &global->outboundLatency);

	issuePdelayRespFollowUp(&timestamp, &ptpClock->PdelayReqHeader, dst,
		global, ptpClock, sequenceId);
}

static void
handlePdelayRespFollowUp(const MsgHeader *header, ssize_t length,
			 Boolean isFromSelf, const GlobalConfig *global,
			 PtpClock *ptpClock)
{

	if (ptpClock->portDS.delayMechanism == P2P) {
		TimeInternal responseOriginTimestamp;
		TimeInternal correctionField;
	
		DBG("PdelayRespfollowup message received : \n");
	
		if(length < PDELAY_RESP_FOLLOW_UP_LENGTH) {
			DBG("Error: PdelayRespfollowup message too short\n");
			ptpClock->counters.messageFormatErrors++;
			return;
		}	
	
		switch(ptpClock->portDS.portState) {
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
				fromOriginTimestamp(
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
				updatePeerDelay (&ptpClock->mpdIirFilter,
						 global, ptpClock,
						 &correctionField,TRUE);

/* pdelay interval handling begin */
				if (global->logDelayReqOverride == 0) {
					DBGV("current delay_req: %d  new delay req: %d \n",
						ptpClock->portDS.logMinPdelayReqInterval,
					header->logMessageInterval);
                        	    if (ptpClock->portDS.logMinPdelayReqInterval != header->logMessageInterval) {
			
                    			if(header->logMessageInterval == UNICAST_MESSAGEINTERVAL &&
						global->autoDelayReqInterval) {

						if(global->unicastNegotiation && ptpClock->peerGrants.grantData[PDELAY_RESP_INDEXED].granted) {
						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Peer Delay Interval %d from peer\n",
							    ptpClock->peerGrants.grantData[PDELAY_RESP_INDEXED].logInterval);
						    }
						    ptpClock->portDS.logMinPdelayReqInterval = ptpClock->peerGrants.grantData[PDELAY_RESP_INDEXED].logInterval;
						} else {

						    if(ptpClock->delayRespWaiting) {
							    NOTICE("Received Peer Delay Interval %d from peer (unicast-unknown) - overriding with %d\n",
							    header->logMessageInterval, global->logMinPdelayReqInterval);
						    }
						    ptpClock->portDS.logMinPdelayReqInterval = global->logMinPdelayReqInterval;

						}	
					} else {
						/* Accept new DelayReq value from the Master */

						    NOTICE("Received new Peer Delay Request interval %d from Master (was: %d)\n",
							     header->logMessageInterval, ptpClock->portDS.logMinPdelayReqInterval );

						    // collect new value indicated by the Master
						    ptpClock->portDS.logMinPdelayReqInterval = header->logMessageInterval;
				    }
				} else {

					if (ptpClock->portDS.logMinPdelayReqInterval != global->logMinPdelayReqInterval) {
						INFO("New Peer Delay Request interval applied: %d (was: %d)\n",
							global->logMinPdelayReqInterval, ptpClock->portDS.logMinPdelayReqInterval);
					}
					ptpClock->portDS.logMinPdelayReqInterval = global->logMinPdelayReqInterval;
				}

/* pdelay interval handling end */
			}
                    		ptpClock->delayRespWaiting = FALSE;	
				tmrStart(ptpClock, DELAY_RECEIPT, max(
					(ptpClock->portDS.announceReceiptTimeout) * (pow(2,ptpClock->portDS.logAnnounceInterval)),
					MISSED_MESSAGES_MAX * (pow(2,ptpClock->portDS.logMinPdelayReqInterval))));

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
	} else if (ptpClock->portDS.delayMechanism == E2E) { /* (End to End mode..) */
		DBG("Peer Delay messages are disregarded in End to End "
		      "mode \n");
		ptpClock->counters.discardedMessages++;
		ptpClock->counters.delayMechanismMismatchErrors++;
	/* no delay mechanism */
	} else {
		DBG("PdelayRespFollowUp ignored - we are in DELAY_DISABLED mode");
		ptpClock->counters.discardedMessages++;
	}
}

/* Only accept the management / signaling message if it satisfies 15.3.1 Table 36 */
/* Also 13.12.1 table 32 */
Boolean
acceptPortIdentity(PortIdentity thisPort, PortIdentity targetPort)
{
        ClockIdentity allOnesClkIdentity;
        UInteger16    allOnesPortNumber = 0xFFFF;
        memset(allOnesClkIdentity, 0xFF, sizeof(allOnesClkIdentity));

	/* equal port IDs: equal clock ID and equal port ID */
	if(!memcmp(targetPort.clockIdentity, thisPort.clockIdentity, CLOCK_IDENTITY_LENGTH) &&
	    (targetPort.portNumber == thisPort.portNumber)) {
	    return TRUE;
	}

	/* equal clockIDs and target port number is wildcard */
	if(!memcmp(targetPort.clockIdentity, thisPort.clockIdentity, CLOCK_IDENTITY_LENGTH) &&
	    (targetPort.portNumber == allOnesPortNumber)) {
	    return TRUE;
	}

	/* target clock ID is wildcard, port number matches */
        if(!memcmp(targetPort.clockIdentity, allOnesClkIdentity, CLOCK_IDENTITY_LENGTH) &&
            (targetPort.portNumber == thisPort.portNumber)) {
	    return TRUE;
	}

	/* target port and clock IDs are both wildcard */
        if(!memcmp(targetPort.clockIdentity, allOnesClkIdentity, CLOCK_IDENTITY_LENGTH) &&
            (targetPort.portNumber == allOnesPortNumber)) {
	    return TRUE;
	}

	return FALSE;
}

/* This code does not get built in the slave-only build */
#ifndef PTPD_SLAVE_ONLY

/* send Announce to all destinations */
static void
issueAnnounce(const GlobalConfig *global,PtpClock *ptpClock)
{
	void *dst = ptpClock->generalDestination;
	int i = 0;
	UnicastGrantData *grant = NULL;
	Boolean okToSend = TRUE;

	Timestamp originTimestamp;
	TimeInternal internalTime;

	/* grab a single timestamp once for all Announce - doing it for each individual message can be costly */
	getPtpClockTime(&internalTime, ptpClock);
	toOriginTimestamp(&originTimestamp, &internalTime);

	/* send Announce to Ethernet or multicast */
	if(global->transportMode != TMODE_UC) {
		issueAnnounceSingle(dst, &ptpClock->sentAnnounceSequenceId, &originTimestamp, global, ptpClock);
	/* send Announce to unicast destination(s) */
	} else {
	    /* send to granted only */
	    if(global->unicastNegotiation) {
		for(i = 0; i < UNICAST_MAX_DESTINATIONS; i++) {
		    grant = &(ptpClock->unicastGrants[i].grantData[ANNOUNCE_INDEXED]);
		    okToSend = TRUE;
		    if(grant->logInterval > ptpClock->portDS.logAnnounceInterval ) {
			grant->intervalCounter %= (UInteger32)(pow(2,grant->logInterval - ptpClock->portDS.logAnnounceInterval));
			if(grant->intervalCounter != 0) {
			    okToSend = FALSE;
			}
			grant->intervalCounter++;
		    }
		    if(grant->granted) {
			if(okToSend) {
			    issueAnnounceSingle(ptpClock->unicastGrants[i].protocolAddress,
			    &grant->sentSeqId, &originTimestamp, global, ptpClock);
			}
		    }
		}
	    /* send to fixed unicast destinations */
	    } else {
		for(i = 0; i < ptpClock->unicastDestinationCount; i++) {
			issueAnnounceSingle(ptpClock->unicastDestinations[i].protocolAddress,
			&(ptpClock->unicastGrants[i].grantData[ANNOUNCE_INDEXED].sentSeqId), &originTimestamp,
						global, ptpClock);
		    }
		}
	}

}

/* send single announce to a single destination */
static void
issueAnnounceSingle(void *dst, UInteger16 *sequenceId, Timestamp *originTimestamp, const GlobalConfig *global,PtpClock *ptpClock)
{

	msgPackAnnounce(ptpClock->msgObuf, *sequenceId, originTimestamp, ptpClock);
	if (sendPtpData(ptpClock, PTPMSG_GENERAL, ptpClock->msgObuf, ANNOUNCE_LENGTH, dst, NULL)) {
		    DBGV("Announce MSG sent ! \n");
		    (*sequenceId)++;
		    ptpClock->counters.announceMessagesSent++;
	}
}

/* send Sync to all destinations */
static void
issueSync(const GlobalConfig *global,PtpClock *ptpClock)
{
	void *dst = ptpClock->eventDestination;
	int i = 0;
	UnicastGrantData *grant = NULL;
	Boolean okToSend = TRUE;

	/* send Sync to multicast */
	if(global->transportMode != TMODE_UC) {
		issueSyncSingle(dst, &ptpClock->sentSyncSequenceId, global, ptpClock);
	/* send Sync to unicast destination(s) */
	} else {
	    for(i = 0; i < UNICAST_MAX_DESTINATIONS; i++) {
		ptpAddrClear(ptpClock->syncDestIndex[i].protocolAddress);
		clearTime(&ptpClock->unicastGrants[i].lastSyncTimestamp);
		clearTime(&ptpClock->unicastDestinations[i].lastSyncTimestamp);
	    }
	    /* send to granted only */
	    if(global->unicastNegotiation) {
		for(i = 0; i < UNICAST_MAX_DESTINATIONS; i++) {
		    grant = &(ptpClock->unicastGrants[i].grantData[SYNC_INDEXED]);
		    okToSend = TRUE;
		    /* handle different intervals */
		    if(grant->logInterval > ptpClock->portDS.logSyncInterval ) {
			grant->intervalCounter %= (UInteger32)(pow(2,grant->logInterval - ptpClock->portDS.logSyncInterval));
			if(grant->intervalCounter != 0) {
			    okToSend = FALSE;
			}
#ifdef RUNTIME_DEBUG
			{
			    tmpstr(strAddr, ptpAddrStrLen(grant->parent->protocolAddress));
			    DBG("mixed interval to %d counter: %d\n",
			        ptpAddrToString(strAddr, strAddr_len, grant->parent->protocolAddress), grant->intervalCounter);

			}
#endif /* RUNTIME_DEBUG */
			grant->intervalCounter++;
		    }

		    if(grant->granted) {
			if(okToSend) {
			    ptpClock->unicastGrants[i].lastSyncTimestamp =
				issueSyncSingle(ptpClock->unicastGrants[i].protocolAddress,
				&grant->sentSeqId,global, ptpClock);
			}
		    }
		}
	    /* send to fixed unicast destinations */
	    } else {
		for(i = 0; i < ptpClock->unicastDestinationCount; i++) {
			ptpClock->unicastDestinations[i].lastSyncTimestamp =
			    issueSyncSingle(ptpClock->unicastDestinations[i].protocolAddress,
			    &(ptpClock->unicastGrants[i].grantData[SYNC_INDEXED].sentSeqId),
						global, ptpClock);
		    }
		}
	}

}

#endif /* PTPD_SLAVE_ONLY */

/*Pack and send a single Sync message, return the embedded timestamp*/
static TimeInternal
issueSyncSingle(void *dst, UInteger16 *sequenceId, const GlobalConfig *global,PtpClock *ptpClock)
{
	Timestamp originTimestamp;
	TimeInternal internalTime, now;

	clearTime(&internalTime);
	clearTime(&now);

	getPtpClockTime(&internalTime, ptpClock);

	if (respectUtcOffset(global, ptpClock) == TRUE) {
		internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
	}

	if(ptpClock->leapSecondInProgress) {
		DBG("Leap second in progress - will not send SYNC\n");
		clearTime(&internalTime);
		return internalTime;
	}

	toOriginTimestamp(&originTimestamp, &internalTime);

	now = internalTime;

	msgPackSync(ptpClock->msgObuf,*sequenceId,&originTimestamp,ptpClock);
	clearTime(&internalTime);
	if (sendPtpData(ptpClock, PTPMSG_EVENT, ptpClock->msgObuf, SYNC_LENGTH, dst, &internalTime)) {
		DBGV("Sync MSG sent ! \n");

		if(!isTimeZero(&internalTime)) {
			    if (respectUtcOffset(global, ptpClock) == TRUE) {
				    internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			    }
			    processSyncFromSelf(&internalTime, global, ptpClock, dst, *sequenceId);
		}

		ptpAddrCopy(ptpClock->lastSyncDst, dst);

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
issueFollowup(const TimeInternal *tint,const GlobalConfig *global,PtpClock *ptpClock, void *dst, UInteger16 sequenceId)
{
	Timestamp preciseOriginTimestamp;
	toOriginTimestamp(&preciseOriginTimestamp, tint);

	msgPackFollowUp(ptpClock->msgObuf,&preciseOriginTimestamp,ptpClock,sequenceId);

	if (sendPtpData(ptpClock, PTPMSG_GENERAL, ptpClock->msgObuf, FOLLOW_UP_LENGTH, dst, NULL)) {
		DBGV("FollowUp MSG sent ! \n");
		ptpClock->counters.followUpMessagesSent++;
	}

}


/*Pack and send on event multicast ip adress a DelayReq message*/
static void
issueDelayReq(const GlobalConfig *global,PtpClock *ptpClock)
{
	Timestamp originTimestamp;
	TimeInternal internalTime;

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
	getPtpClockTime(&internalTime, ptpClock);

	if (respectUtcOffset(global, ptpClock) == TRUE) {
		internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
	}

	toOriginTimestamp(&originTimestamp, &internalTime);

	// uses current sentDelayReqSequenceId
	msgPackDelayReq(ptpClock->msgObuf,&originTimestamp,ptpClock);

	void *dst = ptpClock->eventDestination;

	  /* in hybrid mode  or unicast mode, send delayReq to current master */
        if (global->transportMode == TMODE_MIXED || global->transportMode == TMODE_UC) {
		if(ptpClock->bestMaster) {
    		    dst = ptpClock->bestMaster->protocolAddress;
		}
        }
	if (sendPtpData(ptpClock, PTPMSG_EVENT, ptpClock->msgObuf, DELAY_REQ_LENGTH, dst, &internalTime)) {
		DBGV("DelayReq MSG sent ! \n");
		if(!isTimeZero(&internalTime)) {
			if (respectUtcOffset(global, ptpClock) == TRUE) {
				internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			}			
			
			processDelayReqFromSelf(&internalTime, global, ptpClock);
		}

		ptpClock->sentDelayReqSequenceId++;
		ptpClock->counters.delayReqMessagesSent++;

		/* From now on, we will only accept delayreq and
		 * delayresp of (sentDelayReqSequenceId - 1) */

		/* Explicitly re-arm timer for sending the next delayReq
		 * 9.5.11.2: arm the timer with a uniform range from 0 to 2 x interval
		 * this is only ever used here, so removed the ptpTimerStart_random function
		 */

		tmrStart(ptpClock, DELAYREQ_INTERVAL,
		   pow(2,ptpClock->portDS.logMinDelayReqInterval) * getRand() * 2.0);
	}
}

/*Pack and send on event multicast ip adress a PdelayReq message*/
static void
issuePdelayReq(const GlobalConfig *global,PtpClock *ptpClock)
{
	void *dst = ptpClock->peerEventDestination;
	Timestamp originTimestamp;
	TimeInternal internalTime;

	/* see LEAPNOTE01# in this file */
	if(ptpClock->leapSecondInProgress) {
	    DBG("Leap secon in progress - will not send PDELAY_REQ\n");
	    return;
	}

	getPtpClockTime(&internalTime, ptpClock);

	if (respectUtcOffset(global, ptpClock) == TRUE) {
		internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
	}
	toOriginTimestamp(&originTimestamp, &internalTime);

	if(global->transportMode == TMODE_UC && ptpClock->unicastPeerDestination.protocolAddress) {
	    dst = ptpClock->unicastPeerDestination.protocolAddress;
	}
	
	msgPackPdelayReq(ptpClock->msgObuf,&originTimestamp,ptpClock);
	if (sendPtpData(ptpClock, PTPMSG_EVENT, ptpClock->msgObuf, PDELAY_REQ_LENGTH, dst, &internalTime)) {
		DBGV("PdelayReq MSG sent ! \n");
		

		if(!isTimeZero(&internalTime)) {
			if (respectUtcOffset(global, ptpClock) == TRUE) {
				internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			}			
			processPdelayReqFromSelf(&internalTime, global, ptpClock);
		}

		ptpClock->sentPdelayReqSequenceId++;
		ptpClock->counters.pdelayReqMessagesSent++;
	}
}

/*Pack and send on event multicast ip adress a PdelayResp message*/
static void
issuePdelayResp(const TimeInternal *tint,MsgHeader *header, void *src, const GlobalConfig *global,
		PtpClock *ptpClock)
{
	
	Timestamp requestReceiptTimestamp;
	TimeInternal internalTime;

	void *dst = ptpClock->peerEventDestination;

	/* see LEAPNOTE01# in this file */
	if(ptpClock->leapSecondInProgress) {
		DBG("Leap second in progress - will not send PDELAY_RESP\n");
		return;
	}

	/* if request was unicast and we're running unicast, reply to source */
	if ( (global->transportMode != TMODE_MC) &&
	     (header->flagField0 & PTP_UNICAST) == PTP_UNICAST) {
		dst = src;
	}
	
	toOriginTimestamp(&requestReceiptTimestamp, tint);
	msgPackPdelayResp(ptpClock->msgObuf,header,
			  &requestReceiptTimestamp,ptpClock);
	if (sendPtpData(ptpClock, PTPMSG_EVENT, ptpClock->msgObuf, PDELAY_RESP_LENGTH, dst, &internalTime)) {
		DBGV("PdelayResp MSG sent ! \n");
		
		if(!isTimeZero(&internalTime)) {
			if (respectUtcOffset(global, ptpClock) == TRUE) {
				internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
			}			
			processPdelayRespFromSelf(&internalTime, global, ptpClock, dst, header->sequenceId);
		}

		
		ptpClock->counters.pdelayRespMessagesSent++;
		ptpAddrCopy(ptpClock->lastPdelayRespDst, dst);
	}
}


/*Pack and send a DelayResp message on event socket*/
static void
issueDelayResp(const TimeInternal *tint,MsgHeader *header, void *src, const GlobalConfig *global, PtpClock *ptpClock)
{

	int len = DELAY_RESP_LENGTH;
	Timestamp requestReceiptTimestamp;
	void *dst = ptpClock->generalDestination;

	toOriginTimestamp(&requestReceiptTimestamp, tint);
	/* rewrite domain number with our own */
	header->domainNumber = ptpClock->defaultDS.domainNumber;
	msgPackDelayResp(ptpClock->msgObuf,header,&requestReceiptTimestamp,
			 ptpClock);

	/* if request was unicast and we're running unicast, reply to source */
	if(header->ptpmon || header->mtie) {
		DBG("Populating PTPMON response\n");
		dst = src;
		len = populatePtpMon(ptpClock->msgObuf, header, ptpClock, global);
		if(len == 0) {
		    DBG("Could not populate PTPMON response!\n");
		    return;
		}
	} else if ( (global->transportMode != TMODE_MC) &&
	     (header->flagField0 & PTP_UNICAST) == PTP_UNICAST) {
		dst = src;
	}

	if (sendPtpData(ptpClock, PTPMSG_GENERAL, ptpClock->msgObuf, len, dst, NULL)) {
		DBGV("PdelayResp MSG sent ! \n");
		ptpClock->counters.delayRespMessagesSent++;
	}

	if(header->ptpmon) {
	    DBG("Issuing SYNC for PTPMON\n");
	    (void)issueSyncSingle(dst, &header->sequenceId, global, ptpClock);
	}

}

static void
issuePdelayRespFollowUp(const TimeInternal *tint, MsgHeader *header, void *dst,
			     const GlobalConfig *global, PtpClock *ptpClock, const UInteger16 sequenceId)
{
	Timestamp responseOriginTimestamp;
	toOriginTimestamp(&responseOriginTimestamp, tint);

	msgPackPdelayRespFollowUp(ptpClock->msgObuf,header,
				  &responseOriginTimestamp,ptpClock, sequenceId);
	if (sendPtpData(ptpClock, PTPMSG_GENERAL, ptpClock->msgObuf, PDELAY_RESP_FOLLOW_UP_LENGTH, dst, NULL)) {
		DBGV("PdelayRespFollowUp MSG sent ! \n");
		ptpClock->counters.pdelayRespFollowUpMessagesSent++;
	}

}

static void
addForeign(Octet *buf,MsgHeader *header,PtpClock *ptpClock, UInteger8 localPreference, void *src)
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
			ptpClock->foreign[j].disqualified = FALSE;
			ptpClock->foreign[j].localPreference = localPreference;
			break;
		}
	
		j = (j+1)%ptpClock->number_foreign_records;
	}

	/*New Foreign Master*/
	if (!found) {
		if (ptpClock->number_foreign_records <
		    ptpClock->fmrCapacity) {
			ptpClock->number_foreign_records++;
		}
		
		/* Preserve best master record from overwriting (sf FR #22) - use next slot */
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

		ptpAddrCopy(ptpClock->foreign[j].protocolAddress, src);
		ptpClock->foreign[j].disqualified = FALSE;
		/*
		 * header and announce field of each Foreign Master are
		 * usefull to run Best Master Clock Algorithm
		 */
		msgUnpackHeader(buf,&ptpClock->foreign[j].header);
		msgUnpackAnnounce(buf,&ptpClock->foreign[j].announce);
		DBGV("New foreign Master added \n");
		
		ptpClock->foreign_record_i =
			(ptpClock->foreign_record_i+1) %
			ptpClock->fmrCapacity;
	}
}

/* Update dataset fields which are safe to change without going into INITIALIZING */
void
updateDatasets(PtpClock* ptpClock, const GlobalConfig* global)
{

	if(global->unicastNegotiation) {
	    	updateUnicastGrantTable(ptpClock->unicastGrants,
			    ptpClock->unicastDestinationCount, global);
		if(global->unicastPeerDestinationSet) {
	    	    updateUnicastGrantTable(&ptpClock->peerGrants,
			    1, global);

		}
	}

	memset(ptpClock->userDescription, 0, sizeof(ptpClock->userDescription));
	memcpy(ptpClock->userDescription, global->portDescription, strlen(global->portDescription));

	switch(ptpClock->portDS.portState) {

		/* We are master so update both the port and the parent dataset */
		case PTP_MASTER:

			if(global->dot1AS) {
			    ptpClock->portDS.transportSpecific = TSP_ETHERNET_AVB;
			} else {
			    ptpClock->portDS.transportSpecific = TSP_DEFAULT;
			}

			ptpClock->defaultDS.numberPorts = NUMBER_PORTS;
			ptpClock->portDS.portIdentity.portNumber = global->portNumber;

			ptpClock->portDS.delayMechanism = global->delayMechanism;
			ptpClock->portDS.versionNumber = VERSION_PTP;

			ptpClock->defaultDS.clockQuality.clockAccuracy =
				global->clockQuality.clockAccuracy;
			ptpClock->defaultDS.clockQuality.clockClass = global->clockQuality.clockClass;
			ptpClock->defaultDS.clockQuality.offsetScaledLogVariance =
				global->clockQuality.offsetScaledLogVariance;
			ptpClock->defaultDS.priority1 = global->priority1;
			ptpClock->defaultDS.priority2 = global->priority2;

			ptpClock->parentDS.grandmasterClockQuality.clockAccuracy =
				ptpClock->defaultDS.clockQuality.clockAccuracy;
			ptpClock->parentDS.grandmasterClockQuality.clockClass =
				ptpClock->defaultDS.clockQuality.clockClass;
			ptpClock->parentDS.grandmasterClockQuality.offsetScaledLogVariance =
				ptpClock->defaultDS.clockQuality.offsetScaledLogVariance;
			ptpClock->defaultDS.clockQuality.clockAccuracy =
			ptpClock->parentDS.grandmasterPriority1 = ptpClock->defaultDS.priority1;
			ptpClock->parentDS.grandmasterPriority2 = ptpClock->defaultDS.priority2;
			ptpClock->timePropertiesDS.currentUtcOffsetValid = global->timeProperties.currentUtcOffsetValid;
			ptpClock->timePropertiesDS.currentUtcOffset = global->timeProperties.currentUtcOffset;
			ptpClock->timePropertiesDS.timeTraceable = global->timeProperties.timeTraceable;
			ptpClock->timePropertiesDS.frequencyTraceable = global->timeProperties.frequencyTraceable;
			ptpClock->timePropertiesDS.ptpTimescale = global->timeProperties.ptpTimescale;
			ptpClock->timePropertiesDS.timeSource = global->timeProperties.timeSource;
			ptpClock->portDS.logAnnounceInterval = global->logAnnounceInterval;
			ptpClock->portDS.announceReceiptTimeout = global->announceReceiptTimeout;
			ptpClock->portDS.logSyncInterval = global->logSyncInterval;
			ptpClock->portDS.logMinPdelayReqInterval = global->logMinPdelayReqInterval;
			ptpClock->portDS.logMinDelayReqInterval = global->initial_delayreq;
			break;
		/*
		 * we are not master so update the port dataset only - parent will be updated
		 * by m1() if we go master - basically update the fields affecting BMC only
		 */
		case PTP_SLAVE:
                    	if(ptpClock->portDS.logMinDelayReqInterval == UNICAST_MESSAGEINTERVAL &&
				global->autoDelayReqInterval) {
					NOTICE("Running at %d Delay Interval (unicast) - overriding with %d\n",
					ptpClock->portDS.logMinDelayReqInterval, global->logMinDelayReqInterval);
					ptpClock->portDS.logMinDelayReqInterval = global->logMinDelayReqInterval;
				}
		case PTP_PASSIVE:
			ptpClock->defaultDS.numberPorts = NUMBER_PORTS;
			ptpClock->portDS.portIdentity.portNumber = global->portNumber;

			if(global->dot1AS) {
			    ptpClock->portDS.transportSpecific = TSP_ETHERNET_AVB;
			} else {
			    ptpClock->portDS.transportSpecific = TSP_DEFAULT;
			}

			ptpClock->portDS.delayMechanism = global->delayMechanism;
			ptpClock->portDS.versionNumber = VERSION_PTP;
			ptpClock->defaultDS.clockQuality.clockAccuracy =
				global->clockQuality.clockAccuracy;
			ptpClock->defaultDS.clockQuality.clockClass = global->clockQuality.clockClass;
			ptpClock->defaultDS.clockQuality.offsetScaledLogVariance =
				global->clockQuality.offsetScaledLogVariance;
			ptpClock->defaultDS.priority1 = global->priority1;
			ptpClock->defaultDS.priority2 = global->priority2;
			break;
		/* in DISABLED state we still listen for management messages */
		case PTP_DISABLED:
			ptpClock->portDS.versionNumber = VERSION_PTP;
		default:
		/* In all other states the datasets will be updated when going into an operational state */
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
respectUtcOffset(const GlobalConfig * global, PtpClock * ptpClock) {
	if (ptpClock->timePropertiesDS.currentUtcOffsetValid || global->alwaysRespectUtcOffset) {
		return TRUE;
	}
	return FALSE;
}

static int populatePtpMon(char *buf, MsgHeader *header, PtpClock *ptpClock, const GlobalConfig *global) {

    PtpTlv *tlv;
    PtpMessage message;
    PtpTlvPtpMonResponse *monresp;
    PtpTlvPtpMonMtieResponse *mtieresp;
    TimeInternal tmpTime;
    int ret = 0;

    memset(&message, 0, sizeof(PtpMessage));

    ret = unpackPtpMessage(&message, buf, buf + DELAY_RESP_LENGTH);

    if(ret < DELAY_RESP_LENGTH) {
	return 0;
    }

    if(header->ptpmon) {

	tlv = createPtpTlv();

	if(tlv == NULL) {
	    freePtpMessage(&message);
	    return 0;
	}

	tlv->tlvType = PTP_TLVTYPE_PTPMON_RESPONSE;
	attachPtpTlv(&message, tlv);
	monresp = &tlv->body.ptpMonResponse;

		/* port state and parentPortaddress */
		monresp->portState = ptpClock->portDS.portState;
                monresp->parentPortAddress.addressLength = ptpClock->portDS.addressLength;
                monresp->parentPortAddress.networkProtocol = ptpClock->portDS.networkProtocol;
                XMALLOC(monresp->parentPortAddress.addressField,
                        monresp->parentPortAddress.addressLength);

		if(ptpClock->portDS.portState > PTP_MASTER &&
		    ptpClock->bestMaster && ptpClock->bestMaster->protocolAddress) {
			memcpy(monresp->parentPortAddress.addressField,
                        ptpAddrGetData(ptpClock->bestMaster->protocolAddress),
                        monresp->parentPortAddress.addressLength);
		}

		/* parentDS */
		copyPortIdentity((PortIdentity*)&monresp->parentPortIdentity, &ptpClock->parentDS.parentPortIdentity);
		monresp->PS = ptpClock->parentDS.parentStats;
		monresp->reserved = 0;
		monresp->observedParentOffsetScaledLogVariance =
				ptpClock->parentDS.observedParentOffsetScaledLogVariance;
		monresp->observedParentClockPhaseChangeRate =
				ptpClock->parentDS.observedParentClockPhaseChangeRate;
		monresp->grandmasterPriority1 = ptpClock->parentDS.grandmasterPriority1;
		monresp->grandmasterClockQuality.clockAccuracy =
				ptpClock->parentDS.grandmasterClockQuality.clockAccuracy;
		monresp->grandmasterClockQuality.clockClass =
				ptpClock->parentDS.grandmasterClockQuality.clockClass;
		monresp->grandmasterClockQuality.offsetScaledLogVariance =
				ptpClock->parentDS.grandmasterClockQuality.offsetScaledLogVariance;
		monresp->grandmasterPriority2 = ptpClock->parentDS.grandmasterPriority2;
		copyClockIdentity(monresp->grandmasterIdentity, ptpClock->parentDS.grandmasterIdentity);

		/* currentDS */
		monresp->stepsRemoved = ptpClock->currentDS.stepsRemoved;
		if(monresp->portState == PTP_SLAVE) {
		    monresp->offsetFromMaster.internalTime.seconds  = ptpClock->currentDS.offsetFromMaster.seconds;
		    monresp->offsetFromMaster.internalTime.nanoseconds  = ptpClock->currentDS.offsetFromMaster.nanoseconds;
		    monresp->meanPathDelay.internalTime.seconds = ptpClock->currentDS.meanPathDelay.seconds;
		    monresp->meanPathDelay.internalTime.nanoseconds = ptpClock->currentDS.meanPathDelay.nanoseconds;
		}

		/* timePropertiesDS */
		monresp->currentUtcOffset = ptpClock->timePropertiesDS.currentUtcOffset;
		Octet ftra = SET_FIELD(ptpClock->timePropertiesDS.frequencyTraceable, FTRA);
		Octet ttra = SET_FIELD(ptpClock->timePropertiesDS.timeTraceable, TTRA);
		Octet ptp = SET_FIELD(ptpClock->timePropertiesDS.ptpTimescale, PTPT);
		Octet utcv = SET_FIELD(ptpClock->timePropertiesDS.currentUtcOffsetValid, UTCV);
		Octet li59 = SET_FIELD(ptpClock->timePropertiesDS.leap59, LI59);
		Octet li61 = SET_FIELD(ptpClock->timePropertiesDS.leap61, LI61);
		monresp->ftra_ttra_ptp_utcv_li59_li61 = ftra | ttra | ptp | utcv | li59 | li61;
		monresp->timeSource = ptpClock->timePropertiesDS.timeSource;

		if(monresp->portState == PTP_SLAVE) {
		    monresp->lastSyncTimestamp.internalTime.seconds = ptpClock->lastOriginTimestamp.seconds;
		    monresp->lastSyncTimestamp.internalTime.nanoseconds = ptpClock->lastOriginTimestamp.nanoseconds;
		}
    }

    if(header->mtie) {

	tlv = createPtpTlv();

	if(tlv == NULL) {
	    freePtpMessage(&message);
	    return 0;
	}

	tlv->tlvType = PTP_TLVTYPE_PTPMON_MTIE_RESPONSE;
	attachPtpTlv(&message, tlv);
	mtieresp = &tlv->body.ptpMonMtieResponse;

	if(ptpClock->portDS.portState == PTP_SLAVE) {
	    mtieresp->mtieValid = ptpClock->slaveStats.statsCalculated;
	    if(mtieresp->mtieValid) {
		mtieresp->windowNumber = ptpClock->slaveStats.windowNumber;
		mtieresp->windowDuration = global->statsUpdateInterval;
		tmpTime = doubleToTimeInternal(ptpClock->slaveStats.ofmMinFinal);
		mtieresp->minOffsetFromMaster.internalTime.seconds = tmpTime.seconds;
		mtieresp->minOffsetFromMaster.internalTime.nanoseconds = tmpTime.nanoseconds;
		tmpTime = doubleToTimeInternal(ptpClock->slaveStats.ofmMaxFinal);
		mtieresp->maxOffsetFromMaster.internalTime.seconds = tmpTime.seconds;
		mtieresp->maxOffsetFromMaster.internalTime.nanoseconds = tmpTime.nanoseconds;
	    }
	}

    }

    ret = packPtpMessage(buf, &message, buf + PACKET_SIZE);

    freePtpMessage(&message);

    if(ret < DELAY_RESP_LENGTH) {
	return 0;
    }

    return ret;

}

void
ptpInternalFault(PtpClock *ptpClock)
{

    tmrStart(ptpClock, PORT_FAULT, ptpClock->global->faultTimeout);
    toState(PTP_FAULTY, ptpClock->global, ptpClock);

}
