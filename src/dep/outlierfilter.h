#ifndef OUTLIERFILTER_H_
#define OUTLIERFILTER_H_

#include <dep/statistics.h>

#define OUTLIERFILTER_MAX_DESC 20

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
 * @file   outlierfilter.h
 * @date   Fri Aug 22 16:18:33 2014
 *
 * @brief  Function definitions for the outlier filter
 *
 */

typedef struct {

    Boolean enabled;
    Boolean discard;
    Boolean autoTune;
    Boolean stepDelay;

    /* the user may have some conditions under which we should not filter.
       this is to hint them that we would like to always filter.
    */
    Boolean alwaysFilter;

    int capacity;
    double threshold;
    double weight;

    int minPercent;
    int maxPercent;
    double thresholdStep;

    double minThreshold;
    double maxThreshold;

    /* if absolute value outside this threshold, do not filter. negative = disabled */
    double maxAcceptable;

    /* accepted sample threshold at which step detection is active */
    int32_t stepThreshold;
    /* value which is considered a step */
    int32_t stepLevel;

    /* delay credit - gets used for waiting when step detected */
    int delayCredit;

    /* credit replenishment unit */
    int creditIncrement;

    /* safeguard - maximum credit */
    int maxDelay;

} OutlierFilterConfig;

typedef struct OutlierFilter OutlierFilter;

struct OutlierFilter {

    char id [OUTLIERFILTER_MAX_DESC + 1];
    OutlierFilterConfig config;

    DoubleMovingStdDev* rawStats;
    DoubleMovingMean* filteredStats;
    DoublePermanentMean outlierStats;
    DoublePermanentMean acceptedStats;
    Boolean lastOutlier;
    double threshold;
    double output;
    int autoTuneSamples;
    int autoTuneOutliers;
    int autoTuneScore;
    int consecutiveOutliers;
    int delay;
    int totalDelay;
    int delayCredit;
    Boolean blocking;

    /* 'methods' */

    int (*init)		(OutlierFilter *filter, OutlierFilterConfig *config, const char *id);
    int (*shutdown)	(OutlierFilter *filter);
    int (*reset)	(OutlierFilter *filter);
    Boolean (*filter)	(OutlierFilter *filter, double sample);
    Boolean (*configure) (OutlierFilter *filter, OutlierFilterConfig *config);
    int (*display)	(OutlierFilter *filter);
    void (*update)	(OutlierFilter *filter);

};



/* OutlierFilter *oFilterCreate(OutlierFilterConfig *options, const char *id); */
int outlierFilterSetup(OutlierFilter *filter);

#endif /*OUTLIERFILTER_H_*/
