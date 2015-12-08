/*-
 * Copyright (c) 2014      Wojciech Owczarek,
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
 * @file   outlierfilter.c
 * @date   Fri Aug 22 16:18:33 2014
 *
 * @brief  Code to handle outlier filter routines
 *
 * Outlier filter mechanics extracted into separate source
 * in preparation for 2.4's OOP-like model
 */

#include "../ptpd.h"

#ifdef LOCAL_PREFIX
#undef LOCAL_PREFIX
#endif

#define LOCAL_PREFIX "OutlierFilter"

static int outlierFilterInit(OutlierFilter *filter, OutlierFilterConfig *config, const char* id);
static int outlierFilterReset(OutlierFilter *filter);
static int outlierFilterShutdown(OutlierFilter *filter);
static int outlierFilterTune(OutlierFilter *filter);
static void outlierFilterUpdate(OutlierFilter *filter);
static Boolean outlierFilterConfigure(OutlierFilter *filter, OutlierFilterConfig *config);
static Boolean outlierFilterFilter(OutlierFilter *filter, double sample);
static int outlierFilterDisplay(OutlierFilter *filter);

int
outlierFilterSetup(OutlierFilter *filter)
{

	if(filter == NULL) {
	    return 0;
	}

	memset(filter, 0, sizeof(OutlierFilter));

	filter->init 		= outlierFilterInit;
	filter->shutdown 	= outlierFilterShutdown;
	filter->reset 		= outlierFilterReset;
	filter->filter		= outlierFilterFilter;
	filter->display		= outlierFilterDisplay;
	filter->configure	= outlierFilterConfigure;
	filter->update		= outlierFilterUpdate;

	return 1;

}

static int
outlierFilterInit(OutlierFilter *filter, OutlierFilterConfig *config, const char* id) {

	filter->config = *config;

	 if (config->enabled) {
                filter->rawStats = createDoubleMovingStdDev(config->capacity);
                strncpy(filter->id, id, OUTLIERFILTER_MAX_DESC);
                strncpy(filter->rawStats->identifier, id, 10);
                filter->filteredStats = createDoubleMovingMean(config->capacity);
                filter->threshold = config->threshold;
        } else {
                filter->rawStats = NULL;
                filter->filteredStats = NULL;
        }
                resetDoublePermanentMean(&filter->outlierStats);
                resetDoublePermanentMean(&filter->acceptedStats);

		filter->delayCredit = filter->config.delayCredit;

	return 1;

}

static int
outlierFilterReset(OutlierFilter *filter) {

	resetDoubleMovingStdDev(filter->rawStats);
	resetDoubleMovingMean(filter->filteredStats);
	filter->lastOutlier = FALSE;
	filter->threshold = filter->config.threshold;
	resetDoublePermanentMean(&filter->outlierStats);
	resetDoublePermanentMean(&filter->acceptedStats);
	filter->delay = 0;
	filter->totalDelay = 0;
	filter->delayCredit = filter->config.delayCredit;
	filter->blocking = FALSE;

	return 1;

}

static int
outlierFilterShutdown(OutlierFilter *filter) {

         if(filter->rawStats != NULL ) {
        	freeDoubleMovingStdDev(&filter->rawStats);
	}

	if(filter->filteredStats != NULL ) {
                freeDoubleMovingMean(&filter->filteredStats);
	}
        resetDoublePermanentMean(&filter->outlierStats);
        resetDoublePermanentMean(&filter->acceptedStats);

	return 1;

}

static
int outlierFilterTune(OutlierFilter *filter) {

	DBG("tuning outlier filter: %s\n", filter->id);

	if(!filter->config.autoTune || (filter->autoTuneSamples < 1)) {
	    return 1;
	}

	filter->autoTuneScore = round(((filter->autoTuneOutliers + 0.0)  / (filter->autoTuneSamples + 0.0)) * 100.0);

	if(filter->autoTuneScore < filter->config.minPercent) {

		filter->threshold -= filter->config.thresholdStep;

		if(filter->threshold < filter->config.minThreshold) {
			filter->threshold = filter->config.minThreshold;
		}

	}

	if(filter->autoTuneScore > filter->config.maxPercent) {

		filter->threshold += filter->config.thresholdStep;

		if(filter->threshold > filter->config.maxThreshold) {
			filter->threshold = filter->config.maxThreshold;
		}

	}

	DBG("%s filter autotune: counter %d, samples %d, outliers, %d percentage %d, new threshold %.02f\n",
		filter->id, filter->rawStats->meanContainer->counter, filter->autoTuneSamples,
		filter->autoTuneOutliers, filter->autoTuneScore,
		filter->threshold);

	filter->autoTuneSamples  = 0;
	filter->autoTuneOutliers  = 0;

	return 1;

}

static void
outlierFilterUpdate(OutlierFilter *filter)
{

	/* not used yet */


}

static Boolean
outlierFilterConfigure(OutlierFilter *filter, OutlierFilterConfig *config)
{

	/* restart needed */
	if(filter->config.capacity != config->capacity) {
		return TRUE;
	}
	
	filter->config = *config;

	filter->reset(filter);

	return FALSE;

}

/* 2 x fairy dust, 3 x unicorn droppings, 1 x magic beanstalk juice. blend, spray on the affected area twice per day */
static Boolean
outlierFilterFilter(OutlierFilter *filter, double sample)
{

	/* true = accepted - this is to tell the user if we advised to throw away the sample */
	Boolean ret = TRUE;

	/* step change: outlier mean - accepted mean from last sampling period */
	double step = 0.0;

	if(!filter->config.enabled) {
		filter->output = sample;
		return TRUE;
	}

	step = fabs(filter->outlierStats.mean - filter->acceptedStats.bufferedMean);

	if(filter->config.autoTune) {
		filter->autoTuneSamples++;
	}

	/* no outlier first - more convenient this way */
	if(!isDoublePeircesOutlier(filter->rawStats, sample, filter->threshold) && (filter->delay == 0)) {

		filter->lastOutlier = FALSE;
		filter->output = sample;

		/* filter is about to accept after a blocking period */
		if(filter->consecutiveOutliers) {
			DBG_LOCAL_ID(filter,"consecutive: %d, mean: %.09fm accepted bmean: %.09f\n", filter->consecutiveOutliers,
				filter->outlierStats.mean,filter->acceptedStats.bufferedMean);

				/* we are about to open up but the offset has risen above step level, we will block again, but not forever */
				if(filter->config.stepDelay &&
				    (fabs(filter->acceptedStats.bufferedMean) < ((filter->config.stepThreshold + 0.0) / 1E9)) &&
				    (step > ((filter->config.stepLevel + 0.0) / 1E9))) {
				    /* if we're to enter blocking, we need 2 * consecutiveOutliers credit */
				    /* if we're already blocking, we just need enough credit */
				    /* if we're already blocking, make sure we block no more than maxDelay */
				    if((filter->blocking && ((filter->config.maxDelay > filter->totalDelay) && (filter->delayCredit >= filter->consecutiveOutliers))) ||
					    (!filter->blocking && (filter->delayCredit >= filter->consecutiveOutliers * 2 ))) {
					    if(!filter->blocking) {
						INFO_LOCAL_ID(filter,"%.03f us step detected, filter will now block\n", step * 1E6);
					    }
					    DBG_LOCAL_ID(filter,"step: %.09f, credit left %d, requesting %d\n",step,
						    filter->delayCredit, filter->consecutiveOutliers);
					    filter->delay = filter->consecutiveOutliers;
					    filter->totalDelay += filter->consecutiveOutliers;
					    filter->delayCredit -= filter->consecutiveOutliers;
					    filter->blocking = TRUE;
					    resetDoublePermanentMean(&filter->outlierStats);
					    filter->lastOutlier = TRUE;
					    DBG_LOCAL_ID(filter,"maxdelay: %d, totaldelay: %d\n",filter->config.maxDelay, filter->totalDelay);
					    return FALSE;


				    /* much love for the ultra magnetic, cause everybody knows you never got enough credit */
				    /* we either ran out of credit while blocking, or we did not have enough to start with */
				    } else {
					if(filter->blocking) {
					    INFO_LOCAL_ID(filter,"blocking time exhausted, filter will stop blocking\n");
					} else {
					    INFO_LOCAL_ID(filter,"%.03f us step detected but filter cannot block\n", step * 1E6);
					}
					DBG_LOCAL_ID(filter,"credit out (has %d, needed %d)\n", filter->delayCredit, filter->consecutiveOutliers);
				    }
				/* NO STEP */
				} else {

					if (filter->blocking) {
					    INFO_LOCAL_ID(filter,"step event over, filter will stop blocking\n");
					}
					filter->blocking = FALSE;
				}

				if(filter->totalDelay != 0) {
					DBG_LOCAL_ID(filter,"Total waited %d\n", filter->totalDelay);
					filter->totalDelay = 0;
				}
		}

		filter->consecutiveOutliers = 0;
		resetDoublePermanentMean(&filter->outlierStats);
		feedDoublePermanentMean(&filter->acceptedStats, sample);

	/* it's an outlier, Sir! */
	} else {

		filter->lastOutlier = TRUE;
		feedDoublePermanentMean(&filter->outlierStats, sample);

		if(filter->delay) {
			DBG_LOCAL_ID(filter,"delay left: %d\n", filter->delay);
			filter->delay--;
			return FALSE;
		}

		filter->autoTuneOutliers++;
		filter->consecutiveOutliers++;

		if(filter->config.discard) {
			ret = FALSE;
		} else {
			filter->output = filter->filteredStats->mean;
		}


		DBG_LOCAL_ID(filter,"Outlier: %.09f\n", sample);
		/* Allow [weight] * [deviation from mean] to influence std dev in the next outlier checks */
		sample = filter->rawStats->meanContainer->mean + filter->config.weight * ( sample - filter->rawStats->meanContainer->mean);

	}

	/* keep stats containers updated */
	feedDoubleMovingStdDev(filter->rawStats, sample);
	feedDoubleMovingMean(filter->filteredStats, filter->output);

	/* re-tune filter twice per window */
	if( (filter->rawStats->meanContainer->counter % ( filter->rawStats->meanContainer->capacity / 2)) == 0) {
		outlierFilterTune(filter);
	}

	/* replenish filter credit once per window */
	if( filter->config.stepDelay && ((filter->rawStats->meanContainer->counter % filter->rawStats->meanContainer->capacity) == 0)) {
		filter->delayCredit += filter->config.creditIncrement;
		if(filter->delayCredit >= filter->config.delayCredit) {
		    filter->delayCredit = filter->config.delayCredit;
		}
		DBG_LOCAL_ID(filter,"credit added, now %d\n", filter->delayCredit);
	}

	return ret;

}


static int outlierFilterDisplay(OutlierFilter *filter) {

	char *id = filter->id;

	INFO("%s outlier filter info:\n",
	    id);
	INFO("            %s.threshold : %.02f\n",
	    id, filter->config.threshold);
	INFO("             %s.autotune : %s\n",
	    id, (filter->config.autoTune) ? "Y" : "N");

	if(!filter->config.autoTune) {
	    return 0;
	}

	INFO("              %s.percent : %d\n",
	    id, filter->autoTuneScore);
	INFO("         %s.minThreshold : %.02f\n",
	    id, filter->config.minThreshold);
	INFO("         %s.maxThreshold : %.02f\n",
	    id, filter->config.maxThreshold);
	INFO("                 %s.thresholdStep : %.02f\n",
	    id, filter->config.thresholdStep);


	return 1;
}

