/*-
 * Copyright (c) 2015-2024 Ioannis Konstantelias,
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
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * @file   servo.c
 * @date   Tue Jul 20 16:19:19 2010
 *
 * @brief  Code which implements the clock servo in software.
 *
 *
 */

#include "../ptpd.h"
#include "timer_collection.h"

#define CLAMP(var,bound) {\
    if(var < -bound) {\
	var = -bound;\
    }\
    if(var > bound) {\
	var = bound;\
    }\
}

#ifdef PTPD_STATISTICS
static void checkServoStable(PtpClock *ptpClock, const RunTimeOpts *rtOpts);
#endif

void
resetWarnings(const RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	ptpClock->warned_operator_slow_slewing = 0;
	ptpClock->warned_operator_fast_slewing = 0;
	ptpClock->warnedUnicastCapacity = FALSE;
	//ptpClock->seen_servo_stable_first_time = FALSE;
}

void
initClock(const RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	DBG("initClock\n");

	/* If we've been suppressing ntpdc error messages, show them once again */
	ptpClock->ntpControl.requestFailed = FALSE;
	ptpClock->disabled = rtOpts->portDisabled;

/* do not reset frequency here - restoreDrift will do it if necessary */
/* 2.3.1: restoreDrift now always compiled - this is no longer needed */
#if 0
	ptpClock->servo.observedDrift = 0;
#endif
	/* clear vars */

	/* clean more original filter variables */
	ti_clear(&ptpClock->currentDS.offsetFromMaster);
	ti_clear(&ptpClock->currentDS.meanPathDelay);
	ti_clear(&ptpClock->delaySM);
	ti_clear(&ptpClock->delayMS);

	ptpClock->ofm_filt.y           = 0;
	ptpClock->ofm_filt.nsec_prev   = 0;

	ptpClock->mpd_filt.s_exp       = 0;  /* clears one-way delay filter */
	ptpClock->offsetFirstUpdated   = FALSE;

	ptpClock->char_last_msg='I';

	resetWarnings(rtOpts, ptpClock);

	/* For Hybrid mode */
//	ptpClock->masterAddr = 0;

	ptpClock->maxDelayRejected = 0;

}

void
updateDelay(one_way_delay_filter * mpd_filt, const RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField)
{

	/* updates paused, leap second pending - do nothing */
	if(ptpClock->leapSecondInProgress)
		return;

	DBGV("updateDelay\n");

	/* todo: do all intermediate calculations on temp vars */
	TimeInternal prev_meanPathDelay = ptpClock->currentDS.meanPathDelay;

	ptpClock->char_last_msg = 'D';

	Boolean maxDelayHit = FALSE;

	{

#ifdef PTPD_STATISTICS
		/* if maxDelayStableOnly configured, only check once servo is stable */
		Boolean checkThreshold = rtOpts-> maxDelayStableOnly ?
		    (ptpClock->servo.isStable && rtOpts->maxDelay) :
		    (rtOpts->maxDelay);
#else
		Boolean checkThreshold = rtOpts->maxDelay;
#endif
		//perform basic checks, using local variables only
		TimeInternal slave_to_master_delay;


		/* calc 'slave_to_master_delay' */
		ti_sub(&slave_to_master_delay, &ptpClock->delay_req_receive_time,
		       &ptpClock->delay_req_send_time);

		if (checkThreshold && /* If maxDelay is 0 then it's OFF */
		    ptpClock->offsetFirstUpdated) {

			if (ti_is_negative(&slave_to_master_delay) &&
			    (llabs(slave_to_master_delay.nanoseconds) > rtOpts->maxDelay)) {
				INFO("updateDelay aborted, "
				     "delay (%lf sec) is negative\n",
				     ti_to_double(&slave_to_master_delay));
				INFO("send (%lf sec)\n",
				     ti_to_double(&ptpClock->delay_req_send_time));
				INFO("recv (%lf sec)\n",
				     ti_to_double(&ptpClock->delay_req_receive_time));
				goto finish;
			}

			if (ti_seconds(&slave_to_master_delay) && checkThreshold) {
				INFO("updateDelay aborted, slave to master delay %lf greater than "
				     "1 second\n",
				     ti_to_double(&slave_to_master_delay));
				if (rtOpts->displayPackets)
					msgDump(ptpClock);
				goto finish;
			}

			if (slave_to_master_delay.nanoseconds > rtOpts->maxDelay) {
				ptpClock->counters.maxDelayDrops++;
				DBG("updateDelay aborted, slave to master delay %d greater than "
				     "administratively set maximum %d\n",
				     slave_to_master_delay.nanoseconds,
				     rtOpts->maxDelay);
				if(rtOpts->maxDelayMaxRejected) {
                                    maxDelayHit = TRUE;
				    /* if we blocked maxDelayMaxRejected samples, reset the slave to unblock the filter */
				    if(++ptpClock->maxDelayRejected > rtOpts->maxDelayMaxRejected) {
					    WARNING("%d consecutive measurements above %d threshold - resetting slave\n",
							rtOpts->maxDelayMaxRejected, slave_to_master_delay.nanoseconds);
					    toState(PTP_LISTENING, rtOpts, ptpClock);
				    }
				}

				if (rtOpts->displayPackets)
					msgDump(ptpClock);
				goto finish;
			} else {
				ptpClock->maxDelayRejected=0;
			}
		}
	}

	/*
	 * The packet has passed basic checks, so we'll:
	 *   - update the global delaySM variable
	 *   - calculate a new filtered MPD
	 */
	if (ptpClock->offsetFirstUpdated) {
		Integer16 s;

		/*
		 * calc 'slave_to_master_delay' (Master to Slave delay is
		 * already computed in updateOffset )
		 */

		DBG("==> UpdateDelay():   %s\n",
			dump_TimeInternal2("Req_RECV:", &ptpClock->delay_req_receive_time,
			"Req_SENT:", &ptpClock->delay_req_send_time));

	/* raw value before filtering */
		ti_sub(&ptpClock->rawDelaySM, &ptpClock->delay_req_receive_time,
		       &ptpClock->delay_req_send_time);

#ifdef PTPD_STATISTICS

/* testing only: step detection */
#if 0
	TimeInternal bob;
	bob.nanoseconds = -1000000;
	bob.seconds = 0;
	if(ptpClock->addOffset) {
	    	ti_add(&ptpClock->rawDelaySM, &ptpClock->rawDelaySM, &bob);
	}
#endif

	/* run the delayMS stats filter */
	if(rtOpts->filterSMOpts.enabled) {
		if (!feedDoubleMovingStatFilter(ptpClock->filterSM,
						ti_to_double(&ptpClock->rawDelaySM))) {
			return;
		}
		ptpClock->rawDelaySM = ti_from_double(ptpClock->filterSM->output);
	}

	/* run the delaySM outlier filter */
	if(!rtOpts->noAdjust && ptpClock->oFilterSM.config.enabled && (ptpClock->oFilterSM.config.alwaysFilter || !ptpClock->servo.runningMaxOutput) ) {
		if (ptpClock->oFilterSM.filter(&ptpClock->oFilterSM,
					       ti_to_double(&ptpClock->rawDelaySM))) {
			ptpClock->delaySM = ti_from_double(ptpClock->oFilterSM.output);
		} else {
			ptpClock->counters.delaySMOutliersFound++;
			/* If the outlier filter has blocked the sample, "reverse" the last maxDelay action */
			if (maxDelayHit) {
				    ptpClock->maxDelayRejected--;
			}
			goto finish;
		}
	} else {
		ptpClock->delaySM = ptpClock->rawDelaySM;
	}



#else
		ti_sub(&ptpClock->delaySM, &ptpClock->delay_req_receive_time,
		       &ptpClock->delay_req_send_time);
#endif

		/* update MeanPathDelay */
	ti_add(&ptpClock->currentDS.meanPathDelay, &ptpClock->delaySM, &ptpClock->delayMS);

	/* Subtract correctionField */
	ti_sub(&ptpClock->currentDS.meanPathDelay, &ptpClock->currentDS.meanPathDelay,
	       correctionField);

	/* Compute one-way delay */
	ti_div2(&ptpClock->currentDS.meanPathDelay);

	if (ti_seconds(&ptpClock->currentDS.meanPathDelay)) {
		DBG("update delay: cannot filter with large OFM, "
		    "clearing filter\n");
		INFO("Servo: Ignoring delayResp because of large OFM\n");

		mpd_filt->s_exp = mpd_filt->nsec_prev = 0;
		/* revert back to previous value */
		ptpClock->currentDS.meanPathDelay = prev_meanPathDelay;
		goto finish;
	}

	if (ti_is_negative(&ptpClock->currentDS.meanPathDelay)) {
		DBG("update delay: found negative value for OWD, "
		    "so ignoring this value: %d\n",
		    ptpClock->currentDS.meanPathDelay.nanoseconds);
		/* revert back to previous value */
		ptpClock->currentDS.meanPathDelay = prev_meanPathDelay;
		goto finish;
	}

		/* avoid overflowing filter */
		s = rtOpts->s;
		while (abs(mpd_filt->y) >> (31 - s))
			--s;

		/* crank down filter cutoff by increasing 's_exp' */
		if (mpd_filt->s_exp < 1)
			mpd_filt->s_exp = 1;
		else if (mpd_filt->s_exp < 1 << s)
			++mpd_filt->s_exp;
		else if (mpd_filt->s_exp > 1 << s)
			mpd_filt->s_exp = 1 << s;

		/* filter 'meanPathDelay' */
		double fy =
			(double)((mpd_filt->s_exp - 1.0) *
			mpd_filt->y / (mpd_filt->s_exp + 0.0) +
			(ptpClock->currentDS.meanPathDelay.nanoseconds / 2.0 +
			 mpd_filt->nsec_prev / 2.0) / (mpd_filt->s_exp + 0.0));

		mpd_filt->nsec_prev = ptpClock->currentDS.meanPathDelay.nanoseconds;

		mpd_filt->y = round(fy);

		ptpClock->currentDS.meanPathDelay.nanoseconds = mpd_filt->y;

		DBGV("delay filter %d, %d\n", mpd_filt->y, mpd_filt->s_exp);
	} else {
		DBG("Ignoring delayResp because we didn't receive any sync yet\n");
		ptpClock->counters.discardedMessages++;
	}

finish:

DBG("UpdateDelay: Max delay hit: %d\n", maxDelayHit);

#ifdef PTPD_STATISTICS
	/* don't churn on stats containers with the old value if we've discarded an outlier */
	if(!(ptpClock->oFilterSM.config.enabled && ptpClock->oFilterSM.config.discard && ptpClock->oFilterSM.lastOutlier)) {
		feedDoublePermanentStdDev(&ptpClock->slaveStats.mpdStats,
					  ti_to_double(&ptpClock->currentDS.meanPathDelay));
		feedDoublePermanentMedian(&ptpClock->slaveStats.mpdMedianContainer,
					  ti_to_double(&ptpClock->currentDS.meanPathDelay));
		if(!ptpClock->slaveStats.mpdStatsUpdated) {
			if (ti_to_double(&ptpClock->currentDS.meanPathDelay) != 0.0) {
				ptpClock->slaveStats.mpdMax =
					ti_to_double(&ptpClock->currentDS.meanPathDelay);
				ptpClock->slaveStats.mpdMin =
					ti_to_double(&ptpClock->currentDS.meanPathDelay);
				ptpClock->slaveStats.mpdStatsUpdated = TRUE;
			}
		} else {
			ptpClock->slaveStats.mpdMax =
				max(ptpClock->slaveStats.mpdMax,
				    ti_to_double(&ptpClock->currentDS.meanPathDelay));
			ptpClock->slaveStats.mpdMin =
				min(ptpClock->slaveStats.mpdMin,
				    ti_to_double(&ptpClock->currentDS.meanPathDelay));
		}
	}
#endif /* PTPD_STATISTICS */

	logStatistics(ptpClock);

}

void
updatePeerDelay(one_way_delay_filter * mpd_filt, const RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField, Boolean twoStep)
{
	Integer16 s;

	/* updates paused, leap second pending - do nothing */
	if(ptpClock->leapSecondInProgress)
		return;


	DBGV("updatePeerDelay\n");

	ptpClock->char_last_msg = 'P';	

	if (twoStep) {
		/* calc 'slave_to_master_delay' */
		ti_sub(&ptpClock->pdelayMS, &ptpClock->pdelay_resp_receive_time,
		       &ptpClock->pdelay_resp_send_time);
		ti_sub(&ptpClock->pdelaySM, &ptpClock->pdelay_req_receive_time,
		       &ptpClock->pdelay_req_send_time);

		/* update 'one_way_delay' */
		ti_add(&ptpClock->portDS.peerMeanPathDelay, &ptpClock->pdelayMS,
		       &ptpClock->pdelaySM);

		/* Subtract correctionField */
		ti_sub(&ptpClock->portDS.peerMeanPathDelay, &ptpClock->portDS.peerMeanPathDelay,
		       correctionField);

		/* Compute one-way delay */
		ti_div2(&ptpClock->portDS.peerMeanPathDelay);
	} else {
		/* One step clock */

		ti_sub(&ptpClock->portDS.peerMeanPathDelay, &ptpClock->pdelay_resp_receive_time,
		       &ptpClock->pdelay_req_send_time);

		/* Subtract correctionField */
		ti_sub(&ptpClock->portDS.peerMeanPathDelay, &ptpClock->portDS.peerMeanPathDelay,
		       correctionField);

		/* Compute one-way delay */
		ti_div2(&ptpClock->portDS.peerMeanPathDelay);
	}

	if (ti_seconds(&ptpClock->portDS.peerMeanPathDelay)) {
		/* cannot filter with secs, clear filter */
		mpd_filt->s_exp = mpd_filt->nsec_prev = 0;
		return;
	}
	/* avoid overflowing filter */
	s = rtOpts->s;
	while (abs(mpd_filt->y) >> (31 - s))
		--s;

	/* crank down filter cutoff by increasing 's_exp' */
	if (mpd_filt->s_exp < 1)
		mpd_filt->s_exp = 1;
	else if (mpd_filt->s_exp < 1 << s)
		++mpd_filt->s_exp;
	else if (mpd_filt->s_exp > 1 << s)
		mpd_filt->s_exp = 1 << s;

	/* filter 'meanPathDelay' */
	mpd_filt->y = (mpd_filt->s_exp - 1) *
		mpd_filt->y / mpd_filt->s_exp +
		(ptpClock->portDS.peerMeanPathDelay.nanoseconds / 2 +
		 mpd_filt->nsec_prev / 2) / mpd_filt->s_exp;

	mpd_filt->nsec_prev = ptpClock->portDS.peerMeanPathDelay.nanoseconds;
	ptpClock->portDS.peerMeanPathDelay.nanoseconds = mpd_filt->y;

	DBGV("delay filter %d, %d\n", mpd_filt->y, mpd_filt->s_exp);


	if(ptpClock->portDS.portState == PTP_SLAVE)
	logStatistics(ptpClock);
}

void
updateOffset(TimeInternal * send_time, TimeInternal * recv_time,
    offset_from_master_filter * ofm_filt, const RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField)
{

	ptpClock->clockControl.offsetOK = FALSE;

	Boolean maxDelayHit = FALSE;

	DBGV("UTCOffset: %d | leap 59: %d |  leap61: %d\n",
	     ptpClock->timePropertiesDS.currentUtcOffset,ptpClock->timePropertiesDS.leap59,ptpClock->timePropertiesDS.leap61);

	/* prepare time constant for servo*/
	ptpClock->servo.maxdT = rtOpts->servoMaxdT;
	if(ptpClock->portDS.logSyncInterval == UNICAST_MESSAGEINTERVAL) {
		ptpClock->servo.dT = 1;

		if(rtOpts->unicastNegotiation && ptpClock->parentGrants && ptpClock->parentGrants->grantData[SYNC].granted) {
			ptpClock->servo.dT = pow(2,ptpClock->parentGrants->grantData[SYNC].logInterval);
		}

	} else {
		ptpClock->servo.dT = pow(2, ptpClock->portDS.logSyncInterval);
	}

#ifdef PTPD_STATISTICS
	/* multiply interval if interval filter used on delayMS */
	if(rtOpts->filterMSOpts.enabled && rtOpts->filterMSOpts.windowType != WINDOW_SLIDING) {
	    ptpClock->servo.dT *= rtOpts->filterMSOpts.windowSize;
	}
#endif
        /* updates paused, leap second pending - do nothing */
        if(ptpClock->leapSecondInProgress)
		return;

	DBGV("==> updateOffset\n");

	{

#ifdef PTPD_STATISTICS
		/* if maxDelayStableOnly configured, only check once servo is stable */
		Boolean checkThreshold = (rtOpts->maxDelayStableOnly ?
		    (ptpClock->servo.isStable && rtOpts->maxDelay) :
		    (rtOpts->maxDelay));
#else
		Boolean checkThreshold = rtOpts->maxDelay;
#endif

	//perform basic checks, using only local variables
	TimeInternal master_to_slave_delay;


	/* calc 'master_to_slave_delay' */
	ti_sub(&master_to_slave_delay, recv_time, send_time);

	if (checkThreshold) { /* If maxDelay is 0 then it's OFF */
		if (ti_seconds(&master_to_slave_delay) && checkThreshold) {
			INFO("updateOffset aborted, master to slave delay greater than 1"
			     " second.\n");
			/* msgDump(ptpClock); */
			return;
		}

		if (llabs(master_to_slave_delay.nanoseconds) > rtOpts->maxDelay) {
			ptpClock->counters.maxDelayDrops++;
			DBG("updateOffset aborted, master to slave delay %d greater than "
			     "administratively set maximum %d\n",
			     master_to_slave_delay.nanoseconds,
			     rtOpts->maxDelay);
				if(rtOpts->maxDelayMaxRejected) {
				    maxDelayHit = TRUE;
				    /* if we blocked maxDelayMaxRejected samples, reset the slave to unblock the filter */
				    if(++ptpClock->maxDelayRejected > rtOpts->maxDelayMaxRejected) {
					    WARNING("%d consecutive delay measurements above %d threshold - resetting slave\n",
							rtOpts->maxDelayMaxRejected, master_to_slave_delay.nanoseconds);
					    toState(PTP_LISTENING, rtOpts, ptpClock);
				    }
			    } else {
				ptpClock->maxDelayRejected=0;
			    }
				if (rtOpts->displayPackets)
					msgDump(ptpClock);
			return;
		}
	}}

	// used for stats feedback
	ptpClock->char_last_msg='S';

	/*
	 * The packet has passed basic checks, so we'll:
	 *   - run the filters
	 *   - update the global delayMS variable
	 *   - calculate the new filtered OFM
	 */

	/* raw value before filtering */
	ti_sub(&ptpClock->rawDelayMS, recv_time, send_time);

	DBG("UpdateOffset: max delay hit: %d\n", maxDelayHit);

#ifdef PTPD_STATISTICS

/* testing only: step detection */
	/*
		TimeInternal bob;
		bob.nanoseconds = 1000000;
		bob.seconds = 0;
		if(ptpClock->addOffset) {
			ti_add(&ptpClock->rawDelayMS, &ptpClock->rawDelayMS, &bob);
		}
	*/
	/* run the delayMS stats filter */
	if (rtOpts->filterMSOpts.enabled) {
		/* FALSE if filter wants to skip the update */
		if (!feedDoubleMovingStatFilter(ptpClock->filterMS,
						ti_to_double(&ptpClock->rawDelayMS))) {
			goto finish;
		}
		ptpClock->rawDelayMS = ti_from_double(ptpClock->filterMS->output);
	}

	/* run the delayMS outlier filter */
	if(!rtOpts->noAdjust && ptpClock->oFilterMS.config.enabled && (ptpClock->oFilterMS.config.alwaysFilter || !ptpClock->servo.runningMaxOutput)) {
		if (ptpClock->oFilterMS.filter(&ptpClock->oFilterMS,
					       ti_to_double(&ptpClock->rawDelayMS))) {
			ptpClock->delayMS = ti_from_double(ptpClock->oFilterMS.output);
		} else {
			ptpClock->counters.delayMSOutliersFound++;
			/* If the outlier filter has blocked the sample, "reverse" the last maxDelay action */
			if (maxDelayHit) {
				    ptpClock->maxDelayRejected--;
			}
			goto finish;
		}
	} else {
		ptpClock->delayMS = ptpClock->rawDelayMS;
	}
#else
	/* Used just for End to End mode. */
	ti_sub(&ptpClock->delayMS, recv_time, send_time);
#endif

	/* Take care of correctionField */
	ti_sub(&ptpClock->delayMS, &ptpClock->delayMS, correctionField);

	/* update 'offsetFromMaster' */
	if (ptpClock->portDS.delayMechanism == P2P) {
		ti_sub(&ptpClock->currentDS.offsetFromMaster, &ptpClock->delayMS,
		       &ptpClock->portDS.peerMeanPathDelay);
		/* (End to End mode or disabled - if disabled, meanpath delay is zero) */
	} else if (ptpClock->portDS.delayMechanism == E2E ||
	    ptpClock->portDS.delayMechanism == DELAY_DISABLED ) {

		ti_sub(&ptpClock->currentDS.offsetFromMaster, &ptpClock->delayMS,
		       &ptpClock->currentDS.meanPathDelay);
	}

	if (ti_seconds(&ptpClock->currentDS.offsetFromMaster)) {
		/* cannot filter with secs, clear filter */
		ofm_filt->nsec_prev = 0;
		ptpClock->offsetFirstUpdated = TRUE;
		ptpClock->clockControl.offsetOK = TRUE;
		SET_ALARM(ALRM_OFM_SECONDS, TRUE);
		goto finish;
	} else {
		SET_ALARM(ALRM_OFM_SECONDS, FALSE);
		if(rtOpts->ofmAlarmThreshold) {
			if (llabs(ptpClock->currentDS.offsetFromMaster.nanoseconds) >
			    rtOpts->ofmAlarmThreshold) {
				SET_ALARM(ALRM_OFM_THRESHOLD, TRUE);
			} else {
				SET_ALARM(ALRM_OFM_THRESHOLD, FALSE);
			}
		}
	}

	/* filter 'offsetFromMaster' */
	ofm_filt->y = ptpClock->currentDS.offsetFromMaster.nanoseconds / 2 +
		ofm_filt->nsec_prev / 2;
	ofm_filt->nsec_prev = ptpClock->currentDS.offsetFromMaster.nanoseconds;
	ptpClock->currentDS.offsetFromMaster.nanoseconds = ofm_filt->y;

	/* Apply the offset shift */
	ti_sub(&ptpClock->currentDS.offsetFromMaster, &ptpClock->currentDS.offsetFromMaster,
	       &rtOpts->ofmShift);

	DBGV("offset filter %d\n", ofm_filt->y);

	/*
	 * Offset must have been computed at least one time before
	 * computing end to end delay
	 */
	ptpClock->offsetFirstUpdated = TRUE;
	ptpClock->clockControl.offsetOK = TRUE;

#ifdef PTPD_STATISTICS
	if(!ptpClock->oFilterMS.lastOutlier) {
		feedDoublePermanentStdDev(&ptpClock->slaveStats.ofmStats,
					  ti_to_double(&ptpClock->currentDS.offsetFromMaster));
		feedDoublePermanentMedian(&ptpClock->slaveStats.ofmMedianContainer,
					  ti_to_double(&ptpClock->currentDS.offsetFromMaster));
		if(!ptpClock->slaveStats.ofmStatsUpdated) {
			if (ti_to_double(&ptpClock->currentDS.offsetFromMaster) != 0.0) {
				ptpClock->slaveStats.ofmMax =
					ti_to_double(&ptpClock->currentDS.offsetFromMaster);
				ptpClock->slaveStats.ofmMin =
					ti_to_double(&ptpClock->currentDS.offsetFromMaster);
				ptpClock->slaveStats.ofmStatsUpdated = TRUE;
			}
		} else {
			ptpClock->slaveStats.ofmMax =
				max(ptpClock->slaveStats.ofmMax,
				    ti_to_double(&ptpClock->currentDS.offsetFromMaster));
			ptpClock->slaveStats.ofmMin =
				min(ptpClock->slaveStats.ofmMin,
				    ti_to_double(&ptpClock->currentDS.offsetFromMaster));
		}


	}
#endif /* PTPD_STATISTICS */

finish:
	logStatistics(ptpClock);

	DBGV("\n--Offset Correction-- \n");
	DBGV("Raw offset from master:  %10ds %11dns\n",
	    ptpClock->delayMS.seconds,
	    ptpClock->delayMS.nanoseconds);

	DBGV("\n--Offset and Delay filtered-- \n");

	if (ptpClock->portDS.delayMechanism == P2P) {
		DBGV("one-way delay averaged (P2P):  %10ds %11dns\n",
		    ptpClock->portDS.peerMeanPathDelay.seconds,
		    ptpClock->portDS.peerMeanPathDelay.nanoseconds);
	} else if (ptpClock->portDS.delayMechanism == E2E) {
		DBGV("one-way delay averaged (E2E):  %10ds %11dns\n",
		    ptpClock->currentDS.meanPathDelay.seconds,
		    ptpClock->currentDS.meanPathDelay.nanoseconds);
	}

	DBGV("offset from master:      %10ds %11dns\n",
	    ptpClock->currentDS.offsetFromMaster.seconds,
	    ptpClock->currentDS.offsetFromMaster.nanoseconds);
	DBGV("observed drift:          %10d\n", ptpClock->servo.observedDrift);

}

void
stepClock(const RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	if(rtOpts->noAdjust){
		WARNING("Could not step clock - clock adjustment disabled\n");
		return;
	}

	SET_ALARM(ALRM_CLOCK_STEP, TRUE);

	ptpClock->clockControl.stepRequired = FALSE;

	TimeInternal oldTime, newTime;
	/*No need to reset the frequency offset: if we're far off, it will quickly get back to a high value */
	g_impl.get_time(CLOCK_SYSTEM, &oldTime);
	ti_sub(&newTime, &oldTime, &ptpClock->currentDS.offsetFromMaster);

	g_impl.set_time(&newTime);

	ptpClock->clockStatus.majorChange = TRUE;

	initClock(rtOpts, ptpClock);

	if(ptpClock->defaultDS.clockQuality.clockClass > 127)
		restoreDrift(ptpClock, rtOpts, TRUE);

	ptpClock->servo.runningMaxOutput = FALSE;
	toState(PTP_FAULTY, rtOpts, ptpClock);		/* make a full protocol reset */

        if(rtOpts->calibrationDelay) {
                ptpClock->isCalibrated = FALSE;
        }

}

void
warn_operator_fast_slewing(const RunTimeOpts * rtOpts, PtpClock * ptpClock, double adj)
{
	if(ptpClock->warned_operator_fast_slewing == 0){
		if ((adj >= rtOpts->servoMaxPpb) || ((adj <= -rtOpts->servoMaxPpb))){
			ptpClock->warned_operator_fast_slewing = 1;
			NOTICE("Servo: Going to slew the clock with the maximum frequency adjustment\n");
		}
	}

}

void
warn_operator_slow_slewing(const RunTimeOpts * rtOpts, PtpClock * ptpClock )
{
	if(ptpClock->warned_operator_slow_slewing == 0){
		ptpClock->warned_operator_slow_slewing = 1;
		ptpClock->warned_operator_fast_slewing = 1;

		/* rule of thumb: at tick rate 10000, slewing at the maximum speed took 0.5ms per second */
		float estimated =
			(((abs(ti_seconds(&ptpClock->currentDS.offsetFromMaster))) + 0.0) * 2.0 *
			 1000.0 / 3600.0);

		ALERT("Servo: %d seconds offset detected, will take %.1f hours to slew\n",
		      ti_seconds(&ptpClock->currentDS.offsetFromMaster), estimated);
	}
}

/*
 * this is a wrapper around adjFreq to abstract extra operations
 */

void
adjFreq_wrapper(const RunTimeOpts * rtOpts, PtpClock * ptpClock, double adj)
{
    if (rtOpts->noAdjust){
		DBGV("adjFreq2: noAdjust on, returning\n");
		return;
	}

/*
 * adjFreq simulation for QNX: correct clock by x ns per tick over clock adjust interval,
 * to make it equal adj ns per second. Makes sense only if intervals are regular.
 */

#ifdef __QNXNTO__

      struct _clockadjust clockadj;
      struct _clockperiod period;
      if (ClockPeriod (CLOCK_REALTIME, 0, &period, 0) < 0)
          return;

	CLAMP(adj,ptpClock->servo.maxOutput);

	/* adjust clock for the duration of 0.9 clock update period in ticks (so we're done before the next) */
	clockadj.tick_count = 0.9 * ptpClock->servo.dT * 1E9 / (period.nsec + 0.0);

	/* scale adjustment per second to adjustment per single tick */
	clockadj.tick_nsec_inc = (adj * ptpClock->servo.dT / clockadj.tick_count) / 0.9;

	DBGV("QNX: adj: %.09f, dt: %.09f, ticks per dt: %d, inc per tick %d\n",
		adj, ptpClock->servo.dT, clockadj.tick_count, clockadj.tick_nsec_inc);

	if (ClockAdjust(CLOCK_REALTIME, &clockadj, NULL) < 0) {
	    DBGV("QNX: failed to call ClockAdjust: %s\n", strerror(errno));
	}
/* regular adjFreq */
#elif defined(HAVE_SYS_TIMEX_H)
	DBG2("     adjFreq2: call adjfreq to %.09f us \n", adj / DBG_UNIT);
	adjFreq(adj);
/* otherwise use adjtime */
#else
	struct timeval tv;

	CLAMP(adj,ptpClock->servo.maxOutput);

	tv.tv_sec = 0;
	tv.tv_usec = (adj / 1000);
	if((ptpClock->servo.dT > 0) && (ptpClock->servo.dT < 1.0)) {
	    tv.tv_usec *= ptpClock->servo.dT;
	}
	adjtime(&tv, NULL);
#endif
}

/* check if it's OK to update the clock, deal with panic mode, call for clock step */
void checkOffset(const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

	/* unless offset OK */
	ptpClock->clockControl.updateOK = FALSE;

	/* if false, updateOffset does not want us to continue */
	if(!ptpClock->clockControl.offsetOK) {
		DBGV("checkOffset: !offsetOK\n");
		return;
	}

	if(rtOpts->noAdjust) {
		DBGV("checkOffset: noAdjust\n");
		/* in case if noAdjust has changed */
		ptpClock->clockControl.available = FALSE;
		return;
	}

	/* Leap second pending: do not update clock */
        if(ptpClock->leapSecondInProgress) {
	    /* let the watchdog know that we still want to hold the clock control */
	    DBGV("checkOffset: leapSecondInProgress\n");
	    ptpClock->clockControl.activity = TRUE;
	    return;
	}

	/* check if we are allowed to step the clock */
	if (!ptpClock->pastStartup &&
	    (rtOpts->stepForce ||
	     (rtOpts->stepOnce && ti_seconds(&ptpClock->currentDS.offsetFromMaster)))) {
		if (rtOpts->stepForce)
			WARNING("First clock update - will step the clock\n");
		if (rtOpts->stepOnce)
			WARNING("First clock update and offset >= 1 second - will step the "
				"clock\n");
		ptpClock->clockControl.stepRequired = TRUE;
		ptpClock->clockControl.updateOK = TRUE;
		ptpClock->pastStartup = TRUE;
		return;
	}

	/* check if offset within allowed limit */
	if (rtOpts->maxOffset &&
	    ((llabs(ptpClock->currentDS.offsetFromMaster.nanoseconds) > abs(rtOpts->maxOffset)) ||
	     ti_seconds(&ptpClock->currentDS.offsetFromMaster))) {
		INFO("Offset %lf greater than "
		     "administratively set maximum %d\n. Will not update clock",
		     ti_to_double(&ptpClock->currentDS.offsetFromMaster), rtOpts->maxOffset);
		return;
	}

	/* offset above 1 second */
	if (ti_seconds(&ptpClock->currentDS.offsetFromMaster)) {

		if(!rtOpts->enablePanicMode) {
			if (!rtOpts->noResetClock)
				CRITICAL("Offset above 1 second (%.09f s). Clock will step.\n",
					 ti_to_double(&ptpClock->currentDS.offsetFromMaster));
			ptpClock->clockControl.stepRequired = TRUE;
			ptpClock->clockControl.updateOK = TRUE;
			ptpClock->pastStartup = TRUE;
			return;
		}

		/* still in panic mode, do nothing */
		if(ptpClock->panicMode) {
			DBG("checkOffset: still in panic mode\n");
			/* if we have not released clock control, keep the heartbeats going */
			if (ptpClock->clockControl.available) {
				ptpClock->clockControl.activity = TRUE;
			/* if for some reason we don't have clock control and not configured to release it, re-acquire it */
			} else if(!rtOpts->panicModeReleaseClock) {
				ptpClock->clockControl.available = TRUE;
				ptpClock->clockControl.activity = TRUE;
			/* and vice versa - in case the setting has changed */
			} else {
				ptpClock->clockControl.available = FALSE;
			}
			return;
		}

		if(ptpClock->panicOver) {
			if (rtOpts->noResetClock)
				CRITICAL("Panic mode timeout - accepting current offset. Clock will be slewed at maximum rate.\n");
			else
				CRITICAL("Panic mode timeout - accepting current offset. Clock will step.\n");
			ptpClock->panicOver = FALSE;
			tmr_stop(tmrs_get(PANIC_MODE_TIMER));
			ptpClock->clockControl.available = TRUE;
			ptpClock->clockControl.stepRequired = TRUE;
			ptpClock->clockControl.updateOK = TRUE;
			ptpClock->pastStartup = TRUE;
			ptpClock->isCalibrated = FALSE;
			return;
		}

		CRITICAL("Offset above 1 second (%.09f s)  - entering panic mode. Clock updates "
			 "paused.\n",
			 ti_to_double(&ptpClock->currentDS.offsetFromMaster));
		ptpClock->panicMode = TRUE;
		ptpClock->panicModeTimeLeft = 6 * rtOpts->panicModeDuration;
		tmr_start(tmrs_get(PANIC_MODE_TIMER), 10);
		/* do not release if not configured to do so */
		if(rtOpts->panicModeReleaseClock) {
			ptpClock->clockControl.available = FALSE;
		}
		return;
	}

	/* offset below 1 second - exit panic mode if no threshold or if below threshold,
	 * but make sure we stayed in panic mode for at least one interval,
	 * so that we avoid flapping.
	 */
	if(rtOpts->enablePanicMode && ptpClock->panicMode &&
	    (ptpClock->panicModeTimeLeft != (2 * rtOpts->panicModeDuration)) ) {

		if (rtOpts->panicModeExitThreshold == 0) {
			ptpClock->panicMode = FALSE;
			ptpClock->panicOver = FALSE;
			tmr_stop(tmrs_get(PANIC_MODE_TIMER));
			NOTICE("Offset below 1 second again: resuming clock control\n");
			/* we can control the clock again */
			ptpClock->clockControl.available = TRUE;
		} else if (llabs(ptpClock->currentDS.offsetFromMaster.nanoseconds) <
			   rtOpts->panicModeExitThreshold) {
			ptpClock->panicMode = FALSE;
			ptpClock->panicOver = FALSE;
			tmr_stop(tmrs_get(PANIC_MODE_TIMER));
			NOTICE("Offset below %d ns threshold: resuming clock control\n",
				    ptpClock->currentDS.offsetFromMaster.nanoseconds);
			/* we can control the clock again */
			ptpClock->clockControl.available = TRUE;
		}
	}

	/* can this even happen if offset is < 1 sec? */
	if(rtOpts->enablePanicMode && ptpClock->panicOver) {
		ptpClock->panicMode = FALSE;
		ptpClock->panicOver = FALSE;
		tmr_stop(tmrs_get(PANIC_MODE_TIMER));
		NOTICE("Panic mode timeout and offset below 1 second again: resuming clock "
		       "control\n");
		/* we can control the clock again */
		ptpClock->clockControl.available = TRUE;
	}



	ptpClock->clockControl.updateOK = TRUE;

}

void
updateClock(const RunTimeOpts * rtOpts, PtpClock * ptpClock)
{

	if(rtOpts->noAdjust) {
		ptpClock->clockControl.available = FALSE;
		DBGV("updateClock: noAdjust - skipped clock update\n");
		return;
	}
	
	if(!ptpClock->clockControl.updateOK) {
		DBGV("updateClock: !clockUpdateOK - skipped clock update\n");
		return;
	}

	DBGV("==> updateClock\n");

	if(ptpClock->clockControl.stepRequired) {
		if (!rtOpts->noResetClock) {
			stepClock(rtOpts, ptpClock);
			ptpClock->clockControl.stepRequired = FALSE;
			return;
		} else {
			if(ptpClock->currentDS.offsetFromMaster.nanoseconds > 0)
				ptpClock->servo.observedDrift = rtOpts->servoMaxPpb;
			else
				ptpClock->servo.observedDrift = -rtOpts->servoMaxPpb;
			warn_operator_slow_slewing(rtOpts, ptpClock);
			adjFreq_wrapper(rtOpts, ptpClock, -ptpClock->servo.observedDrift);
			ptpClock->clockControl.stepRequired = FALSE;
		}
		return;
	}

	if (ptpClock->clockControl.granted) {

	/* only run the servo if we are calibrted - if calibration delay configured */

	if((!rtOpts->calibrationDelay) || ptpClock->isCalibrated) {

		/* Adjust the clock first -> the PI controller runs here */
		adjFreq_wrapper(rtOpts, ptpClock, runPIservo(&ptpClock->servo, ptpClock->currentDS.offsetFromMaster.nanoseconds));
	}
		warn_operator_fast_slewing(rtOpts, ptpClock, ptpClock->servo.observedDrift);
		/* let the clock source know it's being synced */
		ptpClock->clockStatus.inSync = TRUE;
		ptpClock->clockStatus.clockOffset =
			ptpClock->currentDS.offsetFromMaster.nanoseconds / 1000;
		ptpClock->clockStatus.update = TRUE;
	}

	SET_ALARM(ALRM_FAST_ADJ, ptpClock->servo.runningMaxOutput);

	/* we are ready to control the clock */
	ptpClock->clockControl.available = TRUE;
	ptpClock->clockControl.activity = TRUE;

	/* Clock has been updated - or was eligible for an update - restart the timeout timer*/
	if(rtOpts->clockUpdateTimeout > 0) {
		DBG("Restarted clock update timeout timer\n");
		tmr_start(tmrs_get(CLOCK_UPDATE_TIMER), rtOpts->clockUpdateTimeout);
	}

	ptpClock->pastStartup = TRUE;
#ifdef PTPD_STATISTICS
	feedDoublePermanentStdDev(&ptpClock->servo.driftStats, ptpClock->servo.observedDrift);
	feedDoublePermanentMedian(&ptpClock->servo.driftMedianContainer, ptpClock->servo.observedDrift);
	if(!ptpClock->servo.statsUpdated) {
	    if(ptpClock->servo.observedDrift != 0.0){
		ptpClock->servo.driftMax = ptpClock->servo.observedDrift;
		ptpClock->servo.driftMin = ptpClock->servo.observedDrift;
		ptpClock->servo.statsUpdated = TRUE;
	    }
	} else {
	ptpClock->servo.driftMax = max(ptpClock->servo.driftMax, ptpClock->servo.observedDrift);
	ptpClock->servo.driftMin = min(ptpClock->servo.driftMin, ptpClock->servo.observedDrift);
	}
#endif

}

void
setupPIservo(PIservo* servo, const RunTimeOpts* rtOpts)
{
    servo->maxOutput = rtOpts->servoMaxPpb;
    servo->kP = rtOpts->servoKP;
    servo->kI = rtOpts->servoKI;
    servo->dTmethod = rtOpts->servoDtMethod;
#ifdef PTPD_STATISTICS
    servo->stabilityThreshold = rtOpts->servoStabilityThreshold;
    servo->stabilityPeriod = rtOpts->servoStabilityPeriod;
    servo->stabilityTimeout = (60 / rtOpts->statsUpdateInterval) * rtOpts->servoStabilityTimeout;
#endif
}

void
resetPIservo(PIservo* servo)
{
/* not needed: restoreDrift handles this */
/*   servo->observedDrift = 0; */
    servo->input = 0;
    servo->output = 0;
    ti_clear(&servo->lastUpdate);
}

double
runPIservo(PIservo* servo, const Integer32 input)
{

        double dt;

        TimeInternal now, delta;

        switch (servo->dTmethod) {

        case DT_MEASURED:

		g_impl.get_time(CLOCK_STEADY, &now);
		if (ti_is_zero(&servo->lastUpdate)) {
			dt = servo->dT;
		} else {
			ti_sub(&delta, &now, &servo->lastUpdate);
			dt = ti_to_double(&delta);
		}

		/* Don't use dT longer then max update interval multiplier */
		if (dt > (servo->maxdT * servo->dT))
			dt = (servo->maxdT + 0.0) * servo->dT;

		break;

	case DT_CONSTANT:

		dt = servo->dT;

		break;

        case DT_NONE:
        default:
                dt = 1.0;
                break;
        }

        if(dt <= 0.0)
            dt = 1.0;

	servo->input = input;

	if (servo->kP < 0.000001)
		servo->kP = 0.000001;
	if (servo->kI < 0.000001)
		servo->kI = 0.000001;

	servo->observedDrift +=
		dt * ((input + 0.0 ) * servo->kI);

	if(servo->observedDrift >= servo->maxOutput) {
		servo->observedDrift = servo->maxOutput;
		servo->runningMaxOutput = TRUE;
#ifdef PTPD_STATISTICS
		servo->stableCount = 0;
		servo->updateCount = 0;
		servo->isStable = FALSE;
#endif /* PTPD_STATISTICS */
	}
	else if(servo->observedDrift <= -servo->maxOutput) {
		servo->observedDrift = -servo->maxOutput;
		servo->runningMaxOutput = TRUE;
#ifdef PTPD_STATISTICS
		servo->stableCount = 0;
		servo->updateCount = 0;
		servo->isStable = FALSE;
#endif /* PTPD_STATISTICS */
	} else {
		servo->runningMaxOutput = FALSE;
	}

	servo->output = (servo->kP * (input + 0.0) ) + servo->observedDrift;

	if(servo->dTmethod == DT_MEASURED)
		servo->lastUpdate = now;

	DBGV("Servo dt: %.09f, input (ofm): %d, output(adj): %.09f, accumulator (observed drift): %.09f\n", dt, input, servo->output, servo->observedDrift);

	return -servo->output;

}

#ifdef PTPD_STATISTICS
static void
checkServoStable(PtpClock *ptpClock, const RunTimeOpts *rtOpts)
{

        DBG("servo stablecount: %d\n",ptpClock->servo.stableCount);

	/* if not calibrated, do nothing */
	if( !rtOpts->calibrationDelay || ptpClock->isCalibrated ) {
		++ptpClock->servo.updateCount;
	} else {
		return;
	}

	/* check if we're below the threshold or not */
	if(ptpClock->servo.runningMaxOutput || !ptpClock->acceptedUpdates ||
	    (ptpClock->servo.driftStdDev > ptpClock->servo.stabilityThreshold)) {
	    ptpClock->servo.stableCount = 0;
	} else if (ptpClock->servo.driftStdDev <= ptpClock->servo.stabilityThreshold) {
	    ptpClock->servo.stableCount++;
	}

	/* Servo considered stable - drift std dev below threshold for n measurements - saving drift*/
        if(ptpClock->servo.stableCount >= ptpClock->servo.stabilityPeriod) {

		if(!ptpClock->servo.isStable) {
                        NOTICE("Clock servo now within stability threshold of %.03f ppb\n",
				ptpClock->servo.stabilityThreshold);
		}

                saveDrift(ptpClock, rtOpts, ptpClock->servo.isStable);

		ptpClock->servo.isStable = TRUE;
		ptpClock->servo.stableCount = 0;
		ptpClock->servo.updateCount = 0;

	/* servo not stable within max interval */
        } else if(ptpClock->servo.updateCount >= ptpClock->servo.stabilityTimeout) {

		ptpClock->servo.stableCount = 0;
		ptpClock->servo.updateCount = 0;

		if(ptpClock->servo.isStable) {
                        WARNING("Clock servo outside stability threshold (%.03f ppb dev > %.03f ppb thr). Too many warnings may mean the threshold is too low.\n",
			    ptpClock->servo.driftStdDev,
			    ptpClock->servo.stabilityThreshold);
                        ptpClock->servo.isStable = FALSE;
		} else {
                        if(ptpClock->servo.runningMaxOutput) {
				WARNING("Clock servo outside stability threshold after %d seconds - running at maximum rate.\n",
					rtOpts->statsUpdateInterval * ptpClock->servo.stabilityTimeout);
                        } else {
                                WARNING("Clock servo outside stability threshold %d seconds after last check. Saving current observed drift.\n",
					rtOpts->statsUpdateInterval * ptpClock->servo.stabilityTimeout);
				saveDrift(ptpClock, rtOpts, FALSE);
                        }
		}
	}

}

void
updatePtpEngineStats (PtpClock* ptpClock, const RunTimeOpts* rtOpts)
{

	DBG("Refreshing slave engine stats counters\n");
	
		DBG("samples used: %d/%d = %.03f\n", ptpClock->acceptedUpdates, ptpClock->offsetUpdates, (ptpClock->acceptedUpdates + 0.0) / (ptpClock->offsetUpdates + 0.0));

	ptpClock->slaveStats.mpdMean = ptpClock->slaveStats.mpdStats.meanContainer.mean;
	ptpClock->slaveStats.mpdStdDev = ptpClock->slaveStats.mpdStats.stdDev;
	ptpClock->slaveStats.mpdMedian = ptpClock->slaveStats.mpdMedianContainer.median;
	ptpClock->slaveStats.mpdMinFinal = ptpClock->slaveStats.mpdMin;
	ptpClock->slaveStats.mpdMaxFinal = ptpClock->slaveStats.mpdMax;
	ptpClock->slaveStats.ofmMean = ptpClock->slaveStats.ofmStats.meanContainer.mean;
	ptpClock->slaveStats.ofmStdDev = ptpClock->slaveStats.ofmStats.stdDev;
	ptpClock->slaveStats.ofmMedian = ptpClock->slaveStats.ofmMedianContainer.median;
	ptpClock->slaveStats.ofmMinFinal = ptpClock->slaveStats.ofmMin;
	ptpClock->slaveStats.ofmMaxFinal = ptpClock->slaveStats.ofmMax;

	ptpClock->slaveStats.statsCalculated = TRUE;
	ptpClock->servo.driftMean = ptpClock->servo.driftStats.meanContainer.mean;
	ptpClock->servo.driftStdDev = ptpClock->servo.driftStats.stdDev;
	ptpClock->servo.driftMedian = ptpClock->servo.driftMedianContainer.median;
	ptpClock->servo.statsCalculated = TRUE;
	ptpClock->servo.driftMinFinal = ptpClock->servo.driftMin;
	ptpClock->servo.driftMaxFinal = ptpClock->servo.driftMax;

	resetDoublePermanentMean(&ptpClock->oFilterMS.acceptedStats);
	resetDoublePermanentMean(&ptpClock->oFilterSM.acceptedStats);

	if(ptpClock->slaveStats.mpdStats.meanContainer.count >= 10.0) {
		resetDoublePermanentStdDev(&ptpClock->slaveStats.mpdStats);
		resetDoublePermanentMedian(&ptpClock->slaveStats.mpdMedianContainer);
		ptpClock->slaveStats.ofmStatsUpdated = FALSE;
		ptpClock->slaveStats.mpdStatsUpdated = FALSE;
	}
	if(ptpClock->slaveStats.ofmStats.meanContainer.count >= 10.0) {
		resetDoublePermanentStdDev(&ptpClock->slaveStats.ofmStats);
		resetDoublePermanentMedian(&ptpClock->slaveStats.ofmMedianContainer);
	}
	if(ptpClock->servo.driftStats.meanContainer.count >= 10.0)
		resetDoublePermanentStdDev(&ptpClock->servo.driftStats);

	resetDoublePermanentMedian(&ptpClock->servo.driftMedianContainer);
	ptpClock->servo.statsUpdated = FALSE;

	if(rtOpts->servoStabilityDetection && ptpClock->clockControl.granted) {
		checkServoStable(ptpClock, rtOpts);
	}

	ptpClock->offsetUpdates = 0;
	ptpClock->acceptedUpdates = 0;

}

#endif /* PTPD_STATISTICS */
