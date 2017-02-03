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
 * @file   servo.c
 * @date   Tue Jul 20 16:19:19 2010
 *
 * @brief  Code which implements the clock servo in software.
 *
 *
 */

#include "../ptpd.h"

void
resetWarnings(const RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	ptpClock->warnedUnicastCapacity = FALSE;
}

void
initClock(const RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	DBG("initClock\n");

	/* If we've been suppressing ntpdc error messages, show them once again */
	ptpClock->ntpControl.requestFailed = FALSE;
	ptpClock->disabled = rtOpts->portDisabled;

	/* clean more original filter variables */
	clearTime(&ptpClock->currentDS.offsetFromMaster);
	clearTime(&ptpClock->currentDS.meanPathDelay);
	clearTime(&ptpClock->delaySM);
	clearTime(&ptpClock->delayMS);

	ptpClock->ofm_filt.y           = 0;
	ptpClock->ofm_filt.nsec_prev   = 0;

	ptpClock->mpdIirFilter.s_exp       = 0;  /* clears one-way delay filter */
	ptpClock->offsetFirstUpdated   = FALSE;

	ptpClock->char_last_msg='I';

	resetWarnings(rtOpts, ptpClock);

	ptpClock->maxDelayRejected = 0;

}

void
updateDelay(IIRfilter * mpdIirFilter, const RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField)
{

	ClockDriver *cd = ptpClock->clockDriver;

	if(ptpClock->ignoreDelayUpdates > 0) {
	    ptpClock->ignoreDelayUpdates--;
	    DBG("updateDelay: Ignoring delay update: ptpClock->ignoreDelayUpdates\n");
	    return;
	}

	if(isTimeZero(&ptpClock->delay_req_send_time) || isTimeZero(&ptpClock->delay_req_receive_time)) {
	    DBG("updateDelay: Ignoring empty time\n");
	    return;
	}

	/* updates paused, leap second pending - do nothing */
	if(ptpClock->leapSecondInProgress)
		return;

	DBGV("updateDelay\n");

	/* todo: do all intermediate calculations on temp vars */
	TimeInternal prev_meanPathDelay = ptpClock->currentDS.meanPathDelay;

	ptpClock->char_last_msg = 'D';

	Boolean maxDelayHit = FALSE;

	{

		/* if maxDelayStableOnly configured, only check once servo is stable */
		Boolean checkThreshold = rtOpts->maxDelayStableOnly ?
		    (ptpClock->clockLocked && rtOpts->maxDelay) :
		    rtOpts->maxDelay;

		//perform basic checks, using local variables only
		TimeInternal slave_to_master_delay;

		/* calc 'slave_to_master_delay' */
		subTime(&slave_to_master_delay, &ptpClock->delay_req_receive_time,
			&ptpClock->delay_req_send_time);

		if (checkThreshold && /* If maxDelay is 0 then it's OFF */
		    ptpClock->offsetFirstUpdated) {

			if ((slave_to_master_delay.nanoseconds < 0) &&
			    (abs(slave_to_master_delay.nanoseconds) > rtOpts->maxDelay)) {
				INFO("updateDelay aborted, "
				     "delay (sec: %d ns: %d) is negative\n",
				     slave_to_master_delay.seconds,
				     slave_to_master_delay.nanoseconds);
				INFO("send (sec: %d ns: %d)\n",
				     ptpClock->delay_req_send_time.seconds,
				     ptpClock->delay_req_send_time.nanoseconds);
				INFO("recv (sec: %d n	s: %d)\n",
				     ptpClock->delay_req_receive_time.seconds,
				     ptpClock->delay_req_receive_time.nanoseconds);
				goto finish;
			}

			if (slave_to_master_delay.seconds && checkThreshold) {
				INFO("updateDelay aborted, slave to master delay %d.%d greater than 1 second\n",
				     slave_to_master_delay.seconds,
				     slave_to_master_delay.nanoseconds);
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
	subTime(&ptpClock->rawDelaySM, &ptpClock->delay_req_receive_time,
		&ptpClock->delay_req_send_time);

	/* subtract CorrectionField *before* filtering */
	subTime(&ptpClock->rawDelaySM, &ptpClock->rawDelaySM,
		correctionField);


/* testing only: step detection */
#if 0
	TimeInternal bob;
	bob.nanoseconds = -1000000;
	bob.seconds = 0;
	if(ptpClock->addOffset) {
	    	addTime(&ptpClock->rawDelaySM, &ptpClock->rawDelaySM, &bob);
	}
#endif

	/* run the delayMS stats filter */
	if(rtOpts->filterSMOpts.enabled) {
	    if(!feedDoubleMovingStatFilter(ptpClock->filterSM, timeInternalToDouble(&ptpClock->rawDelaySM))) {
		    return;
	    }
	    ptpClock->rawDelaySM = doubleToTimeInternal(ptpClock->filterSM->output);
	}

	/* don't update if MS had an outlier */
	if(ptpClock->oFilterMS.config.enabled && ptpClock->oFilterMS.lastOutlier) {
	    return;
	}
	/* run the delaySM outlier filter */
	if(!rtOpts->noAdjust && ptpClock->oFilterSM.config.enabled && (ptpClock->oFilterSM.config.alwaysFilter || !cd->servo.runningMaxOutput) ) {
		if(ptpClock->oFilterSM.filter(&ptpClock->oFilterSM, timeInternalToDouble(&ptpClock->rawDelaySM))) {
			ptpClock->delaySM = doubleToTimeInternal(ptpClock->oFilterSM.output);
//INFO("SM: NOUTL %.09f\n", timeInternalToDouble(&ptpClock->rawDelaySM));
		} else {
			ptpClock->counters.delaySMOutliers++;
//INFO("SM:  OUTL %.09f\n", timeInternalToDouble(&ptpClock->rawDelaySM));

			/* If the outlier filter has blocked the sample, "reverse" the last maxDelay action */
			if (maxDelayHit) {
				    ptpClock->maxDelayRejected--;
			}
			goto finish;
		}
	} else {
		ptpClock->delaySM = ptpClock->rawDelaySM;
	}

		/* update MeanPathDelay */
		addTime(&ptpClock->currentDS.meanPathDelay, &ptpClock->delaySM,
			&ptpClock->delayMS);

		/* Compute one-way delay */
		div2Time(&ptpClock->currentDS.meanPathDelay);
		
		if (ptpClock->currentDS.meanPathDelay.seconds) {
			DBG("update delay: cannot filter with large OFM, "
				"clearing filter\n");
			DBG("Servo: Ignoring delayResp because of large OFM\n");
			
			mpdIirFilter->s_exp = mpdIirFilter->nsec_prev = 0;
			/* revert back to previous value */
			ptpClock->currentDS.meanPathDelay = prev_meanPathDelay;
			goto finish;
		}

		if(ptpClock->currentDS.meanPathDelay.nanoseconds < 0){
			DBG("update delay: found negative value for OWD, "
			    "so ignoring this value: %d\n",
				ptpClock->currentDS.meanPathDelay.nanoseconds);
			/* revert back to previous value */
			ptpClock->currentDS.meanPathDelay = prev_meanPathDelay;
			goto finish;
		}

		/* avoid overflowing filter */
		s = rtOpts->s;
		while (abs(mpdIirFilter->y) >> (31 - s))
			--s;

		/* crank down filter cutoff by increasing 's_exp' */
		if (mpdIirFilter->s_exp < 1)
			mpdIirFilter->s_exp = 1;
		else if (mpdIirFilter->s_exp < 1 << s)
			++mpdIirFilter->s_exp;
		else if (mpdIirFilter->s_exp > 1 << s)
			mpdIirFilter->s_exp = 1 << s;

		/* filter 'meanPathDelay' */
		double fy =
			(double)((mpdIirFilter->s_exp - 1.0) *
			mpdIirFilter->y / (mpdIirFilter->s_exp + 0.0) +
			(ptpClock->currentDS.meanPathDelay.nanoseconds / 2.0 +
			 mpdIirFilter->nsec_prev / 2.0) / (mpdIirFilter->s_exp + 0.0));

		mpdIirFilter->nsec_prev = ptpClock->currentDS.meanPathDelay.nanoseconds;

		mpdIirFilter->y = round(fy);

		ptpClock->currentDS.meanPathDelay.nanoseconds = mpdIirFilter->y;

		DBGV("delay filter %d, %d\n", mpdIirFilter->y, mpdIirFilter->s_exp);
	} else {
		DBG("Ignoring delayResp because we didn't receive any sync yet\n");
		ptpClock->counters.discardedMessages++;
	}

finish:

DBG("UpdateDelay: Max delay hit: %d\n", maxDelayHit);

	/* don't churn on stats containers with the old value if we've discarded an outlier */
	if(!(ptpClock->oFilterSM.config.enabled && ptpClock->oFilterSM.config.discard && ptpClock->oFilterSM.lastOutlier)) {
		feedDoublePermanentStdDev(&ptpClock->slaveStats.mpdStats, timeInternalToDouble(&ptpClock->currentDS.meanPathDelay));
		feedDoublePermanentMedian(&ptpClock->slaveStats.mpdMedianContainer, timeInternalToDouble(&ptpClock->currentDS.meanPathDelay));
		if(!ptpClock->slaveStats.mpdStatsUpdated) {
			if(timeInternalToDouble(&ptpClock->currentDS.meanPathDelay) != 0.0){
			ptpClock->slaveStats.mpdMax = timeInternalToDouble(&ptpClock->currentDS.meanPathDelay);
			ptpClock->slaveStats.mpdMin = timeInternalToDouble(&ptpClock->currentDS.meanPathDelay);
			ptpClock->slaveStats.mpdStatsUpdated = TRUE;
			}
		} else {
		    ptpClock->slaveStats.mpdMax = max(ptpClock->slaveStats.mpdMax, timeInternalToDouble(&ptpClock->currentDS.meanPathDelay));
		    ptpClock->slaveStats.mpdMin = min(ptpClock->slaveStats.mpdMin, timeInternalToDouble(&ptpClock->currentDS.meanPathDelay));
		}
	}

	logStatistics(ptpClock);

}

void
updatePeerDelay(IIRfilter * mpdIirFilter, const RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField, Boolean twoStep)
{
	Integer16 s;

	/* updates paused, leap second pending - do nothing */
	if(ptpClock->leapSecondInProgress)
		return;


	DBGV("updatePeerDelay\n");

	ptpClock->char_last_msg = 'P';	

	if (twoStep) {
		/* calc 'slave_to_master_delay' */
		subTime(&ptpClock->pdelayMS,
			&ptpClock->pdelay_resp_receive_time,
			&ptpClock->pdelay_resp_send_time);
		subTime(&ptpClock->pdelaySM,
			&ptpClock->pdelay_req_receive_time,
			&ptpClock->pdelay_req_send_time);

		/* update 'one_way_delay' */
		addTime(&ptpClock->portDS.peerMeanPathDelay,
			&ptpClock->pdelayMS,
			&ptpClock->pdelaySM);

		/* Subtract correctionField */
		subTime(&ptpClock->portDS.peerMeanPathDelay,
			&ptpClock->portDS.peerMeanPathDelay, correctionField);

		/* Compute one-way delay */
		div2Time(&ptpClock->portDS.peerMeanPathDelay);
	} else {
		/* One step clock */

		subTime(&ptpClock->portDS.peerMeanPathDelay,
			&ptpClock->pdelay_resp_receive_time,
			&ptpClock->pdelay_req_send_time);

		/* Subtract correctionField */
		subTime(&ptpClock->portDS.peerMeanPathDelay,
			&ptpClock->portDS.peerMeanPathDelay, correctionField);

		/* Compute one-way delay */
		div2Time(&ptpClock->portDS.peerMeanPathDelay);
	}

	if (ptpClock->portDS.peerMeanPathDelay.seconds) {
		/* cannot filter with secs, clear filter */
		mpdIirFilter->s_exp = mpdIirFilter->nsec_prev = 0;
		return;
	}
	/* avoid overflowing filter */
	s = rtOpts->s;
	while (abs(mpdIirFilter->y) >> (31 - s))
		--s;

	/* crank down filter cutoff by increasing 's_exp' */
	if (mpdIirFilter->s_exp < 1)
		mpdIirFilter->s_exp = 1;
	else if (mpdIirFilter->s_exp < 1 << s)
		++mpdIirFilter->s_exp;
	else if (mpdIirFilter->s_exp > 1 << s)
		mpdIirFilter->s_exp = 1 << s;

	/* filter 'meanPathDelay' */
	mpdIirFilter->y = (mpdIirFilter->s_exp - 1) *
		mpdIirFilter->y / mpdIirFilter->s_exp +
		(ptpClock->portDS.peerMeanPathDelay.nanoseconds / 2 +
		 mpdIirFilter->nsec_prev / 2) / mpdIirFilter->s_exp;

	mpdIirFilter->nsec_prev = ptpClock->portDS.peerMeanPathDelay.nanoseconds;
	ptpClock->portDS.peerMeanPathDelay.nanoseconds = mpdIirFilter->y;

	DBGV("delay filter %d, %d\n", mpdIirFilter->y, mpdIirFilter->s_exp);


	if(ptpClock->portDS.portState == PTP_SLAVE)
	logStatistics(ptpClock);
}

void
updateOffset(TimeInternal * send_time, TimeInternal * recv_time,
    offset_from_master_filter * ofm_filt, const RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField)
{

	ClockDriver *cd = ptpClock->clockDriver;

	if(ptpClock->ignoreOffsetUpdates > 0) {
	    ptpClock->ignoreOffsetUpdates--;
	    DBG("updateOffset: Ignoring offset update\n");
	    return;
	}

	if(isTimeZero(send_time) || isTimeZero(recv_time)) {
	    DBG("updateOffset: Ignoring empty time\n");
	    return;
	}

	ptpClock->clockControl.offsetOK = FALSE;

	Boolean maxDelayHit = FALSE;

	DBGV("UTCOffset: %d | leap 59: %d |  leap61: %d\n",
	     ptpClock->timePropertiesDS.currentUtcOffset,ptpClock->timePropertiesDS.leap59,ptpClock->timePropertiesDS.leap61);

	/* prepare time constant for servo*/
	if(ptpClock->portDS.logSyncInterval == UNICAST_MESSAGEINTERVAL) {
		ptpClock->dT = 1;
		if(rtOpts->unicastNegotiation && ptpClock->parentGrants && ptpClock->parentGrants->grantData[SYNC].granted) {
			ptpClock->dT = pow(2,ptpClock->parentGrants->grantData[SYNC].logInterval);
		}

	} else {
		ptpClock->dT = pow(2, ptpClock->portDS.logSyncInterval);
	}

	/* Ensure that servo time constant is corrected to factor in filter delay */
	if(rtOpts->filterMSOpts.enabled && rtOpts->filterMSOpts.windowType==WINDOW_INTERVAL) {
	    cd->servo.delayFactor = rtOpts->filterMSOpts.windowSize;
	} else {
	    cd->servo.delayFactor = 1.0;
	}

        /* updates paused, leap second pending - do nothing */
        if(ptpClock->leapSecondInProgress)
		return;

	DBGV("==> updateOffset\n");

	/* if maxDelayStableOnly configured, only check once servo is stable */
	Boolean checkThreshold = (rtOpts->maxDelayStableOnly ?
	    (ptpClock->clockLocked && rtOpts->maxDelay) :
	    (rtOpts->maxDelay));

	//perform basic checks, using only local variables
	TimeInternal master_to_slave_delay;

	/* calc 'master_to_slave_delay' */
	subTime(&master_to_slave_delay, recv_time, send_time);

	if (checkThreshold) { /* If maxDelay is 0 then it's OFF */
		if (master_to_slave_delay.seconds && checkThreshold) {
			INFO("updateOffset aborted, master to slave delay greater than 1"
			     " second.\n");
			/* msgDump(ptpClock); */
			return;
		}

		if (abs(master_to_slave_delay.nanoseconds) > rtOpts->maxDelay) {
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
	}

	// used for stats feedback
	ptpClock->char_last_msg='S';

	/*
	 * The packet has passed basic checks, so we'll:
	 *   - run the filters
	 *   - update the global delayMS variable
	 *   - calculate the new filtered OFM
	 */

	/* raw value before filtering */
	subTime(&ptpClock->rawDelayMS, recv_time, send_time);

	/* subtract CorrectionField *before* filtering */
	subTime(&ptpClock->rawDelayMS, &ptpClock->rawDelayMS,
		correctionField);

	DBG("UpdateOffset: max delay hit: %d\n", maxDelayHit);

	/* run the delayMS stats filter */
	if(rtOpts->filterMSOpts.enabled) {
	    /* FALSE if filter wants to skip the update */
	    if(!feedDoubleMovingStatFilter(ptpClock->filterMS, timeInternalToDouble(&ptpClock->rawDelayMS))) {
		goto finish;
	    }
	    ptpClock->rawDelayMS = doubleToTimeInternal(ptpClock->filterMS->output);
	}

	/* run the delayMS outlier filter */
	if(!rtOpts->noAdjust && ptpClock->oFilterMS.config.enabled && (ptpClock->oFilterMS.config.alwaysFilter || !cd->servo.runningMaxOutput)) {
		if(ptpClock->oFilterMS.filter(&ptpClock->oFilterMS, timeInternalToDouble(&ptpClock->rawDelayMS))) {
			ptpClock->delayMS = doubleToTimeInternal(ptpClock->oFilterMS.output);
//INFO("MS: NOUTL %.09f\n", timeInternalToDouble(&ptpClock->rawDelayMS));
		} else {
			ptpClock->counters.delayMSOutliers++;
//INFO("MS:  OUTL %.09f\n", timeInternalToDouble(&ptpClock->rawDelayMS));
			/* If the outlier filter has blocked the sample, "reverse" the last maxDelay action */
			if (maxDelayHit) {
				    ptpClock->maxDelayRejected--;
			}
			goto finish;
		}
	} else {
		ptpClock->delayMS = ptpClock->rawDelayMS;
	}

	/* update 'offsetFromMaster' */
	if (ptpClock->portDS.delayMechanism == P2P) {
		subTime(&ptpClock->currentDS.offsetFromMaster,
			&ptpClock->delayMS,
			&ptpClock->portDS.peerMeanPathDelay);
	/* (End to End mode or disabled - if disabled, meanpath delay is zero) */
	} else if (ptpClock->portDS.delayMechanism == E2E ||
	    ptpClock->portDS.delayMechanism == DELAY_DISABLED ) {

		subTime(&ptpClock->currentDS.offsetFromMaster,
			&ptpClock->delayMS,
			&ptpClock->currentDS.meanPathDelay);
	}

	if (ptpClock->currentDS.offsetFromMaster.seconds) {
		/* cannot filter with secs, clear filter */
		ofm_filt->nsec_prev = 0;
		ptpClock->offsetFirstUpdated = TRUE;
		ptpClock->clockControl.offsetOK = TRUE;
		SET_ALARM(ALRM_OFM_SECONDS, TRUE);
		goto finish;
	} else {
		SET_ALARM(ALRM_OFM_SECONDS, FALSE);
		if(rtOpts->ofmAlarmThreshold) {
		    if( abs(ptpClock->currentDS.offsetFromMaster.nanoseconds) 
			> rtOpts->ofmAlarmThreshold) {
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
	addTime(&ptpClock->currentDS.offsetFromMaster, &ptpClock->currentDS.offsetFromMaster,
	&rtOpts->ofmCorrection);

	DBGV("offset filter %d\n", ofm_filt->y);

	/*
	 * Offset must have been computed at least one time before
	 * computing end to end delay
	 */
	ptpClock->offsetFirstUpdated = TRUE;
	ptpClock->clockControl.offsetOK = TRUE;

	if(!ptpClock->oFilterMS.lastOutlier) {
            feedDoublePermanentStdDev(&ptpClock->slaveStats.ofmStats, timeInternalToDouble(&ptpClock->currentDS.offsetFromMaster));
            feedDoublePermanentMedian(&ptpClock->slaveStats.ofmMedianContainer, timeInternalToDouble(&ptpClock->currentDS.offsetFromMaster));
		if(!ptpClock->slaveStats.ofmStatsUpdated) {
			if(timeInternalToDouble(&ptpClock->currentDS.offsetFromMaster) != 0.0){
			ptpClock->slaveStats.ofmMax = timeInternalToDouble(&ptpClock->currentDS.offsetFromMaster);
			ptpClock->slaveStats.ofmMin = timeInternalToDouble(&ptpClock->currentDS.offsetFromMaster);
			ptpClock->slaveStats.ofmStatsUpdated = TRUE;
			}
		} else {
		    ptpClock->slaveStats.ofmMax = max(ptpClock->slaveStats.ofmMax, timeInternalToDouble(&ptpClock->currentDS.offsetFromMaster));
		    ptpClock->slaveStats.ofmMin = min(ptpClock->slaveStats.ofmMin, timeInternalToDouble(&ptpClock->currentDS.offsetFromMaster));
		}
	}
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
	if(cd) {
	    DBGV("observed drift:          %10d\n", cd->servo.integral);
	}

}

void
stepClock(const RunTimeOpts * rtOpts, PtpClock * ptpClock, Boolean force)
{

	ClockDriver *cd = ptpClock->clockDriver;

	if(rtOpts->noAdjust){
		WARNING("Could not step clock - clock adjustment disabled\n");
		return;
	}

	TimeInternal delta = negativeTime(&ptpClock->currentDS.offsetFromMaster);

	if(!force && ptpClock->clockControl.stepRequired &&  ptpClock->clockControl.stepFailed) {
	    return;
	}

	if(!cd->stepTime(cd, (CckTimestamp *)&delta, force)) {
	    CRITICAL("Driver refused to step clock. Manual intervention required or SIGUSR1 to force step.\n");
	    ptpClock->clockControl.stepFailed = TRUE;
	    return;
	}

	SET_ALARM(ALRM_CLOCK_STEP, TRUE);

	ptpClock->clockControl.stepRequired = FALSE;
	ptpClock->clockControl.stepFailed = FALSE;

	ptpClock->clockStatus.majorChange = TRUE;

	initClock(rtOpts, ptpClock);

	if(ptpClock->defaultDS.clockQuality.clockClass > 127) {
	    cd->restoreFrequency(cd);
	}

	recalibrateClock(ptpClock);

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

	/* check if offset within allowed limit */
	if (rtOpts->maxOffset && ((abs(ptpClock->currentDS.offsetFromMaster.nanoseconds) > abs(rtOpts->maxOffset)) ||
		ptpClock->currentDS.offsetFromMaster.seconds)) {
		INFO("Offset %d.%09d greater than "
			 "administratively set maximum %d\n. Will not update clock",
		ptpClock->currentDS.offsetFromMaster.seconds,
		ptpClock->currentDS.offsetFromMaster.nanoseconds, rtOpts->maxOffset);
		return;
	}

	/* offset above 1 second */
	if (ptpClock->currentDS.offsetFromMaster.seconds) {
			ptpClock->clockControl.available = TRUE;
			ptpClock->clockControl.activity = TRUE;

	}

	ptpClock->clockControl.updateOK = TRUE;

}

void
updateClock(const RunTimeOpts * rtOpts, PtpClock * ptpClock)
{

	ClockDriver *cd = ptpClock->clockDriver;

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

	/* only run the servo if we are calibrted - if calibration delay configured */

	if((!rtOpts->calibrationDelay) || ptpClock->isCalibrated) {
		/* SYNC ! */
		/* NEGATIVE - ofm is offset from master, not offset to master. */
		cd->syncClockExternal(cd, (CckTimestamp)negativeTime(&ptpClock->currentDS.offsetFromMaster), ptpClock->dT);
	}
		/* let the clock source know it's being synced */
		ptpClock->clockStatus.inSync = TRUE;
		ptpClock->clockStatus.clockOffset = (ptpClock->currentDS.offsetFromMaster.seconds * 1E9 +
						ptpClock->currentDS.offsetFromMaster.nanoseconds) / 1000;
		ptpClock->clockStatus.update = TRUE;

	SET_ALARM(ALRM_FAST_ADJ, cd->servo.runningMaxOutput);

	/* we are ready to control the clock */
	ptpClock->clockControl.available = TRUE;
	ptpClock->clockControl.activity = TRUE;

	/* Clock has been updated - or was eligible for an update - restart the timeout timer*/
	if(rtOpts->clockUpdateTimeout > 0) {
		DBG("Restarted clock update timeout timer\n");
		ptpTimerStart(&ptpClock->timers[CLOCK_UPDATE_TIMER],rtOpts->clockUpdateTimeout);
	}

	ptpClock->pastStartup = TRUE;
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
	ptpClock->slaveStats.windowNumber++;

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

	ptpClock->offsetUpdates = 0;
	ptpClock->acceptedUpdates = 0;

}

void
recalibrateClock(PtpClock *ptpClock) {


    NOTIFY("Re-calibrating PTP offsets\n");

    ptpClock->oFilterMS.reset(&ptpClock->oFilterMS);
    ptpClock->oFilterSM.reset(&ptpClock->oFilterSM);

    clearTime(&ptpClock->currentDS.offsetFromMaster);
    clearTime(&ptpClock->currentDS.meanPathDelay);
    clearTime(&ptpClock->delaySM);
    clearTime(&ptpClock->delayMS);

    ptpClock->ofm_filt.y           = 0;
    ptpClock->ofm_filt.nsec_prev   = 0;
    ptpClock->mpdIirFilter.s_exp       = 0;
    ptpClock->maxDelayRejected = 0;

    if(ptpClock->rtOpts->calibrationDelay) {
	ptpClock->isCalibrated = FALSE;
	ptpTimerStart(&ptpClock->timers[CALIBRATION_DELAY_TIMER], ptpClock->rtOpts->calibrationDelay);
    }

}
