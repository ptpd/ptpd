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
initClock(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	DBG("initClock\n");
	/* clear vars */
	ptpClock->master_to_slave_delay.seconds = 
		ptpClock->master_to_slave_delay.nanoseconds = 0;
	ptpClock->slave_to_master_delay.seconds = 
		ptpClock->slave_to_master_delay.nanoseconds = 0;
	ptpClock->observed_drift = 0;	/* clears clock servo accumulator (the
					 * I term) */
	ptpClock->owd_filt.s_exp = 0;	/* clears one-way delay filter */

	/* level clock */
	if (!rtOpts->noAdjust)
		adjFreq(0);
}

void 
updateDelay(one_way_delay_filter * owd_filt, RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField)
{

	TimeInternal slave_to_master_delay;

	/* calc 'slave_to_master_delay' */
	subTime(&slave_to_master_delay, &ptpClock->delay_req_receive_time, 
		&ptpClock->delay_req_send_time);

	if (rtOpts->maxDelay) { /* If maxDelay is 0 then it's OFF */
		if (slave_to_master_delay.seconds && rtOpts->maxDelay) {
			INFO("updateDelay aborted, delay greater than 1"
			     " second.");
			msgDump(ptpClock);
			return;
		}

		if (slave_to_master_delay.nanoseconds > rtOpts->maxDelay) {
			INFO("updateDelay aborted, delay %d greater than "
			     "administratively set maximum %d\n",
			     slave_to_master_delay.nanoseconds, 
			     rtOpts->maxDelay);
			msgDump(ptpClock);
			return;
		}
	}

	if (rtOpts->offset_first_updated) {
		Integer16 s;

		/*
		 * calc 'slave_to_master_delay' (Master to Slave delay is
		 * already computed in updateOffset )
		 */
		subTime(&ptpClock->delaySM, &ptpClock->delay_req_receive_time, 
			&ptpClock->delay_req_send_time);

		/* update 'one_way_delay' */
		addTime(&ptpClock->meanPathDelay, &ptpClock->delaySM, 
			&ptpClock->delayMS);

		/* Substract correctionField */
		subTime(&ptpClock->meanPathDelay, &ptpClock->meanPathDelay, 
			correctionField);

		/* Compute one-way delay */
		divTime(&ptpClock->meanPathDelay, 2);

		if (ptpClock->meanPathDelay.seconds) {
			/* cannot filter with secs, clear filter */
			owd_filt->s_exp = owd_filt->nsec_prev = 0;
			return;
		}
		/* avoid overflowing filter */
		s = rtOpts->s;
		while (abs(owd_filt->y) >> (31 - s))
			--s;

		/* crank down filter cutoff by increasing 's_exp' */
		if (owd_filt->s_exp < 1)
			owd_filt->s_exp = 1;
		else if (owd_filt->s_exp < 1 << s)
			++owd_filt->s_exp;
		else if (owd_filt->s_exp > 1 << s)
			owd_filt->s_exp = 1 << s;

		/* filter 'meanPathDelay' */
		owd_filt->y = (owd_filt->s_exp - 1) * 
			owd_filt->y / owd_filt->s_exp +
			(ptpClock->meanPathDelay.nanoseconds / 2 + 
			 owd_filt->nsec_prev / 2) / owd_filt->s_exp;

		owd_filt->nsec_prev = ptpClock->meanPathDelay.nanoseconds;
		ptpClock->meanPathDelay.nanoseconds = owd_filt->y;

		DBGV("delay filter %d, %d\n", owd_filt->y, owd_filt->s_exp);
	}
}


void 
updatePeerDelay(one_way_delay_filter * owd_filt, RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField, Boolean twoStep)
{
	Integer16 s;

	DBGV("updateDelay\n");

	if (twoStep) {
		/* calc 'slave_to_master_delay' */
		subTime(&ptpClock->pdelayMS, 
			&ptpClock->pdelay_resp_receive_time, 
			&ptpClock->pdelay_resp_send_time);
		subTime(&ptpClock->pdelaySM, 
			&ptpClock->pdelay_req_receive_time, 
			&ptpClock->pdelay_req_send_time);

		/* update 'one_way_delay' */
		addTime(&ptpClock->peerMeanPathDelay, 
			&ptpClock->pdelayMS, 
			&ptpClock->pdelaySM);

		/* Substract correctionField */
		subTime(&ptpClock->peerMeanPathDelay, 
			&ptpClock->peerMeanPathDelay, correctionField);

		/* Compute one-way delay */
		ptpClock->peerMeanPathDelay.seconds /= 2;
		ptpClock->peerMeanPathDelay.nanoseconds /= 2;
	} else {
		/* One step clock */

		subTime(&ptpClock->peerMeanPathDelay, 
			&ptpClock->pdelay_resp_receive_time, 
			&ptpClock->pdelay_req_send_time);

		/* Substract correctionField */
		subTime(&ptpClock->peerMeanPathDelay, 
			&ptpClock->peerMeanPathDelay, correctionField);

		/* Compute one-way delay */
		ptpClock->peerMeanPathDelay.seconds /= 2;
		ptpClock->peerMeanPathDelay.nanoseconds /= 2;
	}

	if (ptpClock->peerMeanPathDelay.seconds) {
		/* cannot filter with secs, clear filter */
		owd_filt->s_exp = owd_filt->nsec_prev = 0;
		return;
	}
	/* avoid overflowing filter */
	s = rtOpts->s;
	while (abs(owd_filt->y) >> (31 - s))
		--s;

	/* crank down filter cutoff by increasing 's_exp' */
	if (owd_filt->s_exp < 1)
		owd_filt->s_exp = 1;
	else if (owd_filt->s_exp < 1 << s)
		++owd_filt->s_exp;
	else if (owd_filt->s_exp > 1 << s)
		owd_filt->s_exp = 1 << s;

	/* filter 'meanPathDelay' */
	owd_filt->y = (owd_filt->s_exp - 1) * 
		owd_filt->y / owd_filt->s_exp +
		(ptpClock->peerMeanPathDelay.nanoseconds / 2 + 
		 owd_filt->nsec_prev / 2) / owd_filt->s_exp;

	owd_filt->nsec_prev = ptpClock->peerMeanPathDelay.nanoseconds;
	ptpClock->peerMeanPathDelay.nanoseconds = owd_filt->y;

	DBGV("delay filter %d, %d\n", owd_filt->y, owd_filt->s_exp);
}

void 
updateOffset(TimeInternal * send_time, TimeInternal * recv_time,
    offset_from_master_filter * ofm_filt, RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField)
{
	TimeInternal master_to_slave_delay;

	DBGV("updateOffset\n");

	/* calc 'master_to_slave_delay' */
	subTime(&master_to_slave_delay, recv_time, send_time);

	if (rtOpts->maxDelay) { /* If maxDelay is 0 then it's OFF */
		if (master_to_slave_delay.seconds && rtOpts->maxDelay) {
			INFO("updateDelay aborted, delay greater than 1"
			     " second.");
			msgDump(ptpClock);
			return;
		}

		if (master_to_slave_delay.nanoseconds > rtOpts->maxDelay) {
			INFO("updateDelay aborted, delay %d greater than "
			     "administratively set maximum %d\n",
			     master_to_slave_delay.nanoseconds, 
			     rtOpts->maxDelay);
			msgDump(ptpClock);
			return;
		}
	}

	ptpClock->master_to_slave_delay = master_to_slave_delay;

	subTime(&ptpClock->delayMS, recv_time, send_time);
	/* Used just for End to End mode. */

	/* Take care about correctionField */
	subTime(&ptpClock->master_to_slave_delay, 
		&ptpClock->master_to_slave_delay, correctionField);

	/* update 'offsetFromMaster' */
	if (!rtOpts->E2E_mode) {
		subTime(&ptpClock->offsetFromMaster, 
			&ptpClock->master_to_slave_delay, 
			&ptpClock->peerMeanPathDelay);
	} else {
		/* (End to End mode) */
		subTime(&ptpClock->offsetFromMaster, 
			&ptpClock->master_to_slave_delay, 
			&ptpClock->meanPathDelay);
	}

	if (ptpClock->offsetFromMaster.seconds) {
		/* cannot filter with secs, clear filter */
		ofm_filt->nsec_prev = 0;
		return;
	}
	/* filter 'offsetFromMaster' */
	ofm_filt->y = ptpClock->offsetFromMaster.nanoseconds / 2 + 
		ofm_filt->nsec_prev / 2;
	ofm_filt->nsec_prev = ptpClock->offsetFromMaster.nanoseconds;
	ptpClock->offsetFromMaster.nanoseconds = ofm_filt->y;

	DBGV("offset filter %d\n", ofm_filt->y);

	/*
	 * Offset must have been computed at least one time before 
	 * computing end to end delay
	 */
	if (!rtOpts->offset_first_updated) {
		rtOpts->offset_first_updated = TRUE;
	}
}

void 
updateClock(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	Integer32 adj;
	TimeInternal timeTmp;

	DBGV("updateClock\n");

	if (rtOpts->maxReset) { /* If maxReset is 0 then it's OFF */
		if (ptpClock->offsetFromMaster.seconds && rtOpts->maxReset) {
			INFO("updateClock aborted, offset greater than 1"
			     " second.");
			msgDump(ptpClock);
			goto display;
		}

		if (ptpClock->offsetFromMaster.nanoseconds > rtOpts->maxReset) {
			INFO("updateClock aborted, offset %d greater than "
			     "administratively set maximum %d\n",
			     ptpClock->offsetFromMaster.nanoseconds, 
			     rtOpts->maxReset);
			msgDump(ptpClock);
			goto display;
		}
	}

	if (ptpClock->offsetFromMaster.seconds) {
		/* if secs, reset clock or set freq adjustment to max */
		if (!rtOpts->noAdjust) {
			if (!rtOpts->noResetClock) {
				getTime(&timeTmp);
				subTime(&timeTmp, &timeTmp, 
					&ptpClock->offsetFromMaster);
				setTime(&timeTmp);
				initClock(rtOpts, ptpClock);
			} else {
				adj = ptpClock->offsetFromMaster.nanoseconds
					> 0 ? ADJ_FREQ_MAX : -ADJ_FREQ_MAX;
				adjFreq(-adj);
			}
		}
	} else {
		/* the PI controller */

		/* no negative or zero attenuation */
		if (rtOpts->ap < 1)
			rtOpts->ap = 1;
		if (rtOpts->ai < 1)
			rtOpts->ai = 1;

		/* the accumulator for the I component */
		ptpClock->observed_drift += 
			ptpClock->offsetFromMaster.nanoseconds / rtOpts->ai;

		/* clamp the accumulator to ADJ_FREQ_MAX for sanity */
		if (ptpClock->observed_drift > ADJ_FREQ_MAX)
			ptpClock->observed_drift = ADJ_FREQ_MAX;
		else if (ptpClock->observed_drift < -ADJ_FREQ_MAX)
			ptpClock->observed_drift = -ADJ_FREQ_MAX;

		adj = ptpClock->offsetFromMaster.nanoseconds / rtOpts->ap + 
			ptpClock->observed_drift;

		/* apply controller output as a clock tick rate adjustment */
		if (!rtOpts->noAdjust)
			adjFreq(-adj);
	}

display:
	if (rtOpts->displayStats)
		displayStats(rtOpts, ptpClock);


	DBG("\n--Offset Correction-- \n");
	DBG("Raw offset from master:  %10ds %11dns\n",
	    ptpClock->master_to_slave_delay.seconds, 
	    ptpClock->master_to_slave_delay.nanoseconds);

	DBG("\n--Offset and Delay filtered-- \n");

	if (!rtOpts->E2E_mode) {
		DBG("one-way delay averaged (P2P):  %10ds %11dns\n",
		    ptpClock->peerMeanPathDelay.seconds, 
		    ptpClock->peerMeanPathDelay.nanoseconds);
	} else {
		DBG("one-way delay averaged (E2E):  %10ds %11dns\n",
		    ptpClock->meanPathDelay.seconds, 
		    ptpClock->meanPathDelay.nanoseconds);
	}

	DBG("offset from master:      %10ds %11dns\n",
	    ptpClock->offsetFromMaster.seconds, 
	    ptpClock->offsetFromMaster.nanoseconds);
	DBG("observed drift:          %10d\n", ptpClock->observed_drift);
}
