/*-
 * Copyright (c) 2009-2011 George V. Neville-Neil, Steven Kreuzer, 
 *                         Martin Burnicki, Gael Mace, Alexandre Van Kempen
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
initClock(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	DBG("initClock\n");
	/* clear vars */
	ptpClock->master_to_slave_delay.seconds = 
		ptpClock->master_to_slave_delay.nanoseconds = 0;
	ptpClock->slave_to_master_delay.seconds = 
		ptpClock->slave_to_master_delay.nanoseconds = 0;
	ptpClock->owd_filt.s_exp = 0;	/* clears one-way delay filter */




#if !defined(__APPLE__)

	if (!rtOpts->noAdjust)
		adjFreq(0);
	ptpClock->observed_drift = 0;



#endif /* apple */
	
	/* clean more original filter variables */
	clearTime(&ptpClock->master_to_slave_delay);
	clearTime(&ptpClock->slave_to_master_delay);
	clearTime(&ptpClock->offsetFromMaster);   /* AKB: 9/18/2007 Clear offset from master */


	ptpClock->ofm_filt.y           = 0;
	ptpClock->ofm_filt.nsec_prev   = -1; /* AKB: -1 used for non-valid nsec time */
	ptpClock->owd_filt.s_exp       = 0;  /* clears one-way delay filter */
	rtOpts->offset_first_updated   = FALSE;


	ptpClock->meanPathDelay.seconds = 0;
	ptpClock->meanPathDelay.nanoseconds = 0;
	ptpClock->master_to_slave_delay.seconds = 0;
	ptpClock->master_to_slave_delay.nanoseconds = 0;
	ptpClock->ofm_filt.nsec_prev = 0;

	ptpClock->delayMS.seconds= 0;
	ptpClock->delayMS.nanoseconds= 0;
	ptpClock->warned_operator_slow_slewing= 0;
	ptpClock->warned_operator_fast_slewing= 0;
	ptpClock->char_last_msg='I';
}



void 
updateDelay(one_way_delay_filter * owd_filt, RunTimeOpts * rtOpts, PtpClock * ptpClock, TimeInternal * correctionField)
{

	TimeInternal slave_to_master_delay;

	/* calc 'slave_to_master_delay' */
	subTime(&slave_to_master_delay, &ptpClock->delay_req_receive_time, 
		&ptpClock->delay_req_send_time);

	ptpClock->char_last_msg='D';
	
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
			DBG(" update delay: cannot filter with secs, clearing filter \n");
			INFO("Servo: Ignoring delayResp because of large OFM\n");
			// ?? here
			
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
		displayStats(rtOpts, ptpClock);

		DBGV("delay filter %d, %d\n", owd_filt->y, owd_filt->s_exp);
	} else {
		INFO("Ignoring delayResp because we didn't receive any sync yet\n");

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

	DBGV("==> updateOffset\n");
	ptpClock->char_last_msg='S';

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
		rtOpts->offset_first_updated = TRUE;
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
		rtOpts->offset_first_updated = TRUE;
	}



void servo_perform_clock_step(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	if(rtOpts->noAdjust){
		WARNING("     Clock step bloqued because of option -t\n");
		return;
	}

	TimeInternal timeTmp;
	
	getTime(&timeTmp);
	subTime(&timeTmp, &timeTmp, &ptpClock->offsetFromMaster);

	WARNING("     Performing hard frequency reset, by setting frequency to zero\n");
	adjFreq(0);
	ptpClock->observed_drift = 0;

	setTime(&timeTmp);
	initClock(rtOpts, ptpClock);
}


void warn_operator_fast_slewing(RunTimeOpts * rtOpts, PtpClock * ptpClock, Integer32 adj)
{
	if(ptpClock->warned_operator_fast_slewing == 0){
		if ((adj >= ADJ_FREQ_MAX) || ((adj <= -ADJ_FREQ_MAX))){
			ptpClock->warned_operator_fast_slewing = 1;

			NOTICE("Servo: Going to slew the clock with the maximum frequency adjustment\n");
		}
	}

}

void warn_operator_slow_slewing(RunTimeOpts * rtOpts, PtpClock * ptpClock )
{
	if(ptpClock->warned_operator_slow_slewing == 0){
		ptpClock->warned_operator_slow_slewing = 1;
		ptpClock->warned_operator_fast_slewing = 1;

		/* rule of thumb: at tick rate 10000, slewing at the maximum speed took 0.5ms per second */
		float estimated = (((abs(ptpClock->offsetFromMaster.seconds)) + 0.0) * 2.0 * 1000.0 / 3600.0);

		// we don't want to arrive early 1s in an expiration opening, so all consoles get a message when the time is 1s off.
		EMERGENCY("Servo: %d seconds offset detected, will take %.1f hours to slew\n",
			ptpClock->offsetFromMaster.seconds,
			estimated
		);
	}
}


/*
 * this is a wrapper around adjFreq to abstract extra operations
 */
void adjFreq_wrapper(RunTimeOpts * rtOpts, PtpClock * ptpClock, Integer32 adj)
{
	if (rtOpts->noAdjust){
		DBGV("adjFreq2: noAdjust on, returning\n");
		return;
	}



   // go back to only sending delayreq after first sync
   // otherwise we lose the first sync always
   // test all combinations
   

	// call original adjtime
	DBG2("     adjFreq2: call adjfreq to %d \n", adj / DBG_UNIT);
	adjFreq(adj);

	warn_operator_fast_slewing(rtOpts, ptpClock, adj);


}




void 
updateClock(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	Integer32 adj;
	//TimeInternal timeTmp;

	DBGV("==> updateClock\n");



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

		/* 
		  if offset from master seconds is non-zero, then this is a "big jump:
		  in time.  Check Run Time options to see if we will reset the clock or
		  set frequency adjustment to max to adjust the time
		*/

		/*
		 * noAdjust     = cannot do any change to clock
		 * noResetClock = if can change the clock, can we also step it?
		 */
		if (!rtOpts->noAdjust) {
			/* */
			if (!rtOpts->noResetClock) {

				servo_perform_clock_step(rtOpts, ptpClock);
			} else {
				/*
				 * potential problem:    "-1.1" is   a) -1:0.1 or b) -1:-0.1?
				 * if this code is right it implies the second case
				 */
#if !defined(__APPLE__)
				adj = ptpClock->offsetFromMaster.nanoseconds
					> 0 ? ADJ_FREQ_MAX : -ADJ_FREQ_MAX;

				warn_operator_slow_slewing(rtOpts, ptpClock);
				adjFreq_wrapper(rtOpts, ptpClock, -adj);

				/* its not clear how the APPLE case works for large jumps */
#endif /* __APPLE__ */

			}
		}
	} else {
		/* the PI controller */
		/* Offset from master is less than one second.  Use the the PI controller
		 * to adjust the time
		 */


		/* no negative or zero attenuation */
		if (rtOpts->ap < 1)
			rtOpts->ap = 1;
		if (rtOpts->ai < 1)
			rtOpts->ai = 1;


		/*
		 *  optional new PI controller, with better accuracy with HW-timestamps:
		 *  http://sourceforge.net/tracker/?func=detail&aid=2995791&group_id=139814&atid=744632
		 */

		
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


		DBG2("     Observed_drift with AI component: %d\n",
			ptpClock->observed_drift / DBG_UNIT );

		DBG2("     After PI: Adj: %d   Drift: %d   OFM %d\n",
			adj, ptpClock->observed_drift / DBG_UNIT, ptpClock->offsetFromMaster.nanoseconds / DBG_UNIT);

#if defined(__APPLE__)
			adjTime(ptpClock->offsetFromMaster.nanoseconds);
#else
		adjFreq_wrapper(rtOpts, ptpClock, -adj);
#endif /* __APPLE__ */
	}



display:
		displayStats(rtOpts, ptpClock);


	DBGV("\n--Offset Correction-- \n");
	DBGV("Raw offset from master:  %10ds %11dns\n",
	    ptpClock->master_to_slave_delay.seconds, 
	    ptpClock->master_to_slave_delay.nanoseconds);

	DBGV("\n--Offset and Delay filtered-- \n");

	if (!rtOpts->E2E_mode) {
		DBGV("one-way delay averaged (P2P):  %10ds %11dns\n",
		    ptpClock->peerMeanPathDelay.seconds, 
		    ptpClock->peerMeanPathDelay.nanoseconds);
	} else {
		DBGV("one-way delay averaged (E2E):  %10ds %11dns\n",
		    ptpClock->meanPathDelay.seconds, 
		    ptpClock->meanPathDelay.nanoseconds);
	}

	DBGV("offset from master:      %10ds %11dns\n",
	    ptpClock->offsetFromMaster.seconds, 
	    ptpClock->offsetFromMaster.nanoseconds);
	DBGV("observed drift:          %10d\n", ptpClock->observed_drift);
}

