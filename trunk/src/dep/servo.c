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
	ptpClock->master_to_slave_delay.seconds = ptpClock->master_to_slave_delay.nanoseconds = 0;
	ptpClock->slave_to_master_delay.seconds = ptpClock->slave_to_master_delay.nanoseconds = 0;
	ptpClock->observed_variance = 0;
	ptpClock->observed_drift = 0;	/* clears clock servo accumulator (the
					 * I term) */
	ptpClock->owd_filt.s_exp = 0;	/* clears one-way delay filter */
	ptpClock->halfEpoch = ptpClock->halfEpoch || rtOpts->halfEpoch;
	rtOpts->halfEpoch = 0;

	/* level clock */
	if (!rtOpts->noAdjust)
		adjFreq(0);
}

void 
updateDelay(TimeInternal * send_time, TimeInternal * recv_time,
    one_way_delay_filter * owd_filt, RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	Integer16 s;
	TimeInternal slave_to_master_delay;

	DBGV("updateDelay\n");

	/* calc 'slave_to_master_delay' */
	subTime(&slave_to_master_delay, recv_time, send_time);

	if (rtOpts->maxDelay) { /* If maxDelay is 0 then it's OFF */
		if (slave_to_master_delay.seconds && rtOpts->maxDelay) {
			INFO("updateDelay aborted, delay greater than 1"
			     " second.");
			return;
		}

		if (slave_to_master_delay.nanoseconds > rtOpts->maxDelay) {
			INFO("updateDelay aborted, delay %d greater than "
			     "administratively set maximum %d\n",
			     slave_to_master_delay.nanoseconds, 
			     rtOpts->maxDelay);
			return;
		}
	}

	ptpClock->slave_to_master_delay = slave_to_master_delay;

	/* update 'one_way_delay' */
	addTime(&ptpClock->one_way_delay, &ptpClock->master_to_slave_delay, &ptpClock->slave_to_master_delay);
	ptpClock->one_way_delay.seconds /= 2;
	ptpClock->one_way_delay.nanoseconds /= 2;

	if (ptpClock->one_way_delay.seconds) {
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

	/* filter 'one_way_delay' */
	owd_filt->y = (owd_filt->s_exp - 1) * owd_filt->y / owd_filt->s_exp +
	    (ptpClock->one_way_delay.nanoseconds / 2 + owd_filt->nsec_prev / 2) / owd_filt->s_exp;

	owd_filt->nsec_prev = ptpClock->one_way_delay.nanoseconds;
	ptpClock->one_way_delay.nanoseconds = owd_filt->y;

	DBG("delay filter %d, %d\n", owd_filt->y, owd_filt->s_exp);
}

void 
updateOffset(TimeInternal * send_time, TimeInternal * recv_time,
    offset_from_master_filter * ofm_filt, RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	TimeInternal master_to_slave_delay;
	DBGV("updateOffset\n");

	/* calc 'master_to_slave_delay' */
	subTime(&master_to_slave_delay, recv_time, send_time);

	if (rtOpts->maxDelay) { /* If maxDelay is 0 then it's OFF */
		if (master_to_slave_delay.seconds && rtOpts->maxDelay) {
			INFO("updateDelay aborted, delay greater than 1"
			     " second.");
			return;
		}

		if (master_to_slave_delay.nanoseconds > rtOpts->maxDelay) {
			INFO("updateDelay aborted, delay %d greater than "
			     "administratively set maximum %d\n",
			     master_to_slave_delay.nanoseconds, 
			     rtOpts->maxDelay);
			return;
		}
	}
	ptpClock->master_to_slave_delay = master_to_slave_delay;

	/* update 'offset_from_master' */
	subTime(&ptpClock->offset_from_master, &ptpClock->master_to_slave_delay, &ptpClock->one_way_delay);

	if (ptpClock->offset_from_master.seconds) {
		/* cannot filter with secs, clear filter */
		ofm_filt->nsec_prev = 0;
		return;
	}
	/* filter 'offset_from_master' */
	ofm_filt->y = ptpClock->offset_from_master.nanoseconds / 2 + ofm_filt->nsec_prev / 2;
	ofm_filt->nsec_prev = ptpClock->offset_from_master.nanoseconds;
	ptpClock->offset_from_master.nanoseconds = ofm_filt->y;

	DBGV("offset filter %d\n", ofm_filt->y);
}

void 
updateClock(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	Integer32 adj;
	TimeInternal timeTmp;

	DBGV("updateClock\n");

	if (rtOpts->maxReset) { /* If maxReset is 0 then it's OFF */
		if (ptpClock->offset_from_master.seconds) {
			INFO("updateClock aborted, offset greater than 1"
			     " second.");
			goto display;
		}
		
		if (ptpClock->offset_from_master.nanoseconds > 
		    rtOpts->maxReset) {
			INFO("updateClock aborted, offset %d greater than "
			     "administratively set maximum %d\n",
			     ptpClock->offset_from_master.nanoseconds, 
			     rtOpts->maxReset);
			goto display;
		}
	}

	if (ptpClock->offset_from_master.seconds) {
		/* if secs, reset clock or set freq adjustment to max */
		if (!rtOpts->noAdjust) {
			if (!rtOpts->noResetClock) {
				getTime(&timeTmp);
				subTime(&timeTmp, &timeTmp, &ptpClock->offset_from_master);
				setTime(&timeTmp);
				initClock(rtOpts, ptpClock);
			} else {
				adj = ptpClock->offset_from_master.nanoseconds > 0 ? ADJ_FREQ_MAX : -ADJ_FREQ_MAX;
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
		ptpClock->observed_drift += ptpClock->offset_from_master.nanoseconds / rtOpts->ai;

		/* clamp the accumulator to ADJ_FREQ_MAX for sanity */
		if (ptpClock->observed_drift > ADJ_FREQ_MAX)
			ptpClock->observed_drift = ADJ_FREQ_MAX;
		else if (ptpClock->observed_drift < -ADJ_FREQ_MAX)
			ptpClock->observed_drift = -ADJ_FREQ_MAX;

		adj = ptpClock->offset_from_master.nanoseconds / rtOpts->ap + ptpClock->observed_drift;

		/* apply controller output as a clock tick rate adjustment */
		if (!rtOpts->noAdjust)
			adjFreq(-adj);
	}

display:

	if (rtOpts->displayStats)
		displayStats(rtOpts, ptpClock);

	DBGV("master-to-slave delay:   %10ds %11dns\n",
	    ptpClock->master_to_slave_delay.seconds, ptpClock->master_to_slave_delay.nanoseconds);
	DBGV("slave-to-master delay:   %10ds %11dns\n",
	    ptpClock->slave_to_master_delay.seconds, ptpClock->slave_to_master_delay.nanoseconds);
	DBGV("one-way delay:           %10ds %11dns\n",
	    ptpClock->one_way_delay.seconds, ptpClock->one_way_delay.nanoseconds);
	DBG("offset from master:      %10ds %11dns\n",
	    ptpClock->offset_from_master.seconds, ptpClock->offset_from_master.nanoseconds);
	DBG("observed drift: %10d\n", ptpClock->observed_drift);
}
