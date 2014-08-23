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

void
oFilterInit(OutlierFilter *filter, OutlierFilterOptions *options, const char* id) {

	 if (options->enabled) {
                filter->rawStats = createDoubleMovingStdDev(options->capacity);
                strncpy(filter->rawStats->identifier, id, 10);
                filter->filteredStats = createDoubleMovingMean(options->capacity);
                filter->threshold = options->threshold;
        } else {
                filter->rawStats = NULL;
                filter->filteredStats = NULL;
        }

}

void
oFilterReset(OutlierFilter *filter, OutlierFilterOptions *options) {

                resetDoubleMovingStdDev(filter->rawStats);
                resetDoubleMovingMean(filter->filteredStats);
                filter->lastOutlier = FALSE;
                filter->threshold = options->threshold;
}

void
oFilterDestroy(OutlierFilter *filter) {

         if(filter->rawStats != NULL ) {
        	freeDoubleMovingStdDev(&filter->rawStats);
	}
        
	if(filter->filteredStats != NULL ) {
                freeDoubleMovingMean(&filter->filteredStats);
	}

}

void oFilterTune(OutlierFilter *filter, OutlierFilterOptions *options) {

	if(!options->autoTune) {
	    return;
	}

	filter->autoTuneScore = round(((filter->autoTuneOutliers + 0.0)  / (filter->autoTuneSamples + 0.0)) * 100.0);

	if(filter->autoTuneScore < options->minPercentage) {

				filter->threshold -= options->step;

				if(filter->threshold <= 0.0) {
				    filter->threshold = options->step;
				}
			}


	DBG("%s filter autotune: samples %d, outliers, %d percentage %d, threshold %.02f\n",
		filter->rawStats->identifier,
		filter->autoTuneSamples,
		filter->autoTuneOutliers,
		filter->autoTuneScore,
		filter->threshold);

        filter->autoTuneSamples  = 0;
        filter->autoTuneOutliers  = 0;

}

