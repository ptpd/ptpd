/*-
 * Copyright (c) 2013-2015 Wojciech Owczarek
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
 * @file    statistics.c
 * @authors Wojciech Owczarek
 * @date   Sun Jul 7 13:28:10 2013
 * This souece file contains the implementations of functions maintaining and
 * computing statistical information.
 */

#include "../ptpd.h"

static int cmpInt32 (const void *vA, const void *vB) {

	int32_t a = *(int32_t*)vA;
	int32_t b = *(int32_t*)vB;

	return ((a < b) ? -1 : (a > b) ? 1 : 0);
}

static int cmpDouble (const void *vA, const void *vB) {

	double a = *(double*)vA;
	double b = *(double*)vB;

	return ((a < b) ? -1 : (a > b) ? 1 : 0);
}

static int cmpAbsInt32 (const void *vA, const void *vB) {

	int32_t a = abs(*(int32_t*)vA);
	int32_t b = abs(*(int32_t*)vB);

	return ((a < b) ? -1 : (a > b) ? 1 : 0);
}

static int cmpAbsDouble (const void *vA, const void *vB) {

	double a = fabs(*(double*)vA);
	double b = fabs(*(double*)vB);

	return ((a < b) ? -1 : (a > b) ? 1 : 0);
}

static int32_t median3Int(int32_t *bucket, int count)
{

	int32_t sortedSamples[count];
	memcpy(sortedSamples, bucket, count * sizeof(int32_t));

	switch (count) {

	    case 1:
		return bucket[0];
	    case 2:
		return (bucket[0] + bucket[1]) / 2;
	    case 3:
		qsort(sortedSamples, count, sizeof(int32_t), cmpInt32);
		return sortedSamples[1];
	    default:
		return 0;

	}

}

static double median3Double(double *bucket, int count)
{

	double sortedSamples[count];
	memcpy(sortedSamples, bucket, count * sizeof(double));

	switch (count) {

	    case 1:
		return bucket[0];
	    case 2:
		return (bucket[0] + bucket[1]) / 2.0;
	    case 3:
		qsort(sortedSamples, count, sizeof(double), cmpInt32);
		return sortedSamples[1];
	    default:
		return 0;

	}

}

void
resetIntPermanentMean(IntPermanentMean* container) {

	container->previous = container->mean;
	container->mean = 0;
	container->count = 0;

}

int32_t
feedIntPermanentMean(IntPermanentMean* container, int32_t sample)
{

	container->mean += ( sample - container-> mean ) / ++container->count;

	if(container->previous) {
	    container->bufferedMean = (container->previous + container->mean) / 2;
	} else {
	    container->bufferedMean = container->mean;
	}

	return container->mean;

}

void
resetIntPermanentStdDev(IntPermanentStdDev* container) {

	resetIntPermanentMean(&container->meanContainer);
	container->squareSum = 0;
	container->stdDev = 0;

}

int32_t
feedIntPermanentStdDev(IntPermanentStdDev* container, int32_t sample)
{

	container->squareSum += ( sample - container->meanContainer.mean) *
		    ( sample - feedIntPermanentMean(&container->meanContainer, sample ));

	if(container->meanContainer.count > 1) {
		container->stdDev = sqrt( container->squareSum /
			(container->meanContainer.count - 1) );
	} else {
		container->stdDev = 0;
	}

	return container->stdDev;

}

void
resetIntPermanentMedian(IntPermanentMedian* container) {

	memset(container, 0, sizeof(IntPermanentMedian));

}

int32_t
feedIntPermanentMedian(IntPermanentMedian* container, int32_t sample)
{

	    container->bucket[container->count] = sample;
	    container->count++;

	    container->median = median3Int(container->bucket, container->count);

	    if(container->count == 3) {
		container->count=1;
		container->bucket[1] = container->bucket[2] = 0;
		container->bucket[0] = container->median;
	    }

	    return container->median;

}


void
resetDoublePermanentMean(DoublePermanentMean* container)
{
	if(container == NULL)
	    return;
	container->previous = container->mean;
	container->mean = 0.0;
	container->count = 0;

}

double
feedDoublePermanentMean(DoublePermanentMean* container, double sample)
{

	container->mean += ( sample - container-> mean ) / ++container->count;

	if(container->previous) {
	    container->bufferedMean = (container->previous + container->mean) / 2;
	} else {
	    container->bufferedMean = container->mean;
	}

	return container->mean;

}

void
resetDoublePermanentStdDev(DoublePermanentStdDev* container)
{

	if(container == NULL)
	    return;
	resetDoublePermanentMean(&container->meanContainer);
	container->squareSum = 0.0;
	container->stdDev = 0.0;

}

double
feedDoublePermanentStdDev(DoublePermanentStdDev* container, double sample)
{

	container->squareSum += ( sample - container->meanContainer.mean) *
		    ( sample - feedDoublePermanentMean(&container->meanContainer, sample )) ;

	if(container->meanContainer.count > 1) {
		container->stdDev = sqrt(container->squareSum /
			((container->meanContainer.count+0.0) - 1.0)) ;
	} else {
		container->stdDev = 0;
	}

	return container->stdDev;

}

void
resetDoublePermanentMedian(DoublePermanentMedian* container)
{

	memset(container, 0, sizeof(DoublePermanentMedian));

}

double
feedDoublePermanentMedian(DoublePermanentMedian* container, double sample)
{

	container->bucket[container->count] = sample;
	container->count++;

	container->median = median3Double(container->bucket, container->count);

	if(container->count == 3) {
	    container->count=1;
	    container->bucket[1] = container->bucket[2] = 0;
	    container->bucket[0] = container->median;
	}

	return container->median;

}


/* Moving statistics - up to last n samples */

IntMovingMean*
createIntMovingMean(int capacity)
{

	IntMovingMean* container;
	if ( !(container = calloc (1, sizeof(IntMovingMean))) ) {
	    return NULL;
	}

	container->capacity = (capacity > STATCONTAINER_MAX_SAMPLES ) ?
			STATCONTAINER_MAX_SAMPLES : capacity;

	if ( !(container->samples = calloc (container->capacity, sizeof(int32_t))) ) {
	    free(container);
	    return NULL;
	}

	return container;

}

void
freeIntMovingMean(IntMovingMean** container)
{

	free((*container)->samples);
	free(*container);
	*container = NULL;

}

void
resetIntMovingMean(IntMovingMean* container)
{

	if(container == NULL)
	    return;
	container->sum = 0;
	container->mean = 0;
	container->count = 0;
	memset(container->samples, 0, sizeof(&container->samples));

}

int32_t
feedIntMovingMean(IntMovingMean* container, int32_t sample)
{

	if(container == NULL) return 0;

        /* sample buffer is full */
        if ( container->count == container->capacity ) {
		/* keep the sum current - drop the oldest value */
		container->sum -= container->samples[0];
		/* shift the sample buffer left, dropping the oldest sample */
		memmove(container->samples, container->samples + 1,
			sizeof(sample) * (container->capacity - 1) );
		/* counter will be incremented further, so we decrement it here */
		container->count--;
		container->full = TRUE;
	}

	container->samples[container->count++] = sample;
	container->sum += sample;
	container->mean = container->sum / container->count;

	return container->mean;

}

IntMovingStdDev* createIntMovingStdDev(int capacity)
{

	IntMovingStdDev* container;
	if ( !(container = calloc (1, sizeof(IntMovingStdDev))) ) {
		return NULL;
	}

	if((container->meanContainer = createIntMovingMean(capacity))
		== NULL) {
		free(container);
		return NULL;
	}

	return container;

}

void
freeIntMovingStdDev(IntMovingStdDev** container)
{

	freeIntMovingMean(&((*container)->meanContainer));
	free(*container);
	*container = NULL;

}

void
resetIntMovingStdDev(IntMovingStdDev* container)
{

	if(container == NULL)
	    return;
	resetIntMovingMean(container->meanContainer);
	container->squareSum = 0;
	container->stdDev = 0;

}

int32_t
feedIntMovingStdDev(IntMovingStdDev* container, int32_t sample)
{

	int i = 0;

	if(container == NULL)
		return 0;

	feedIntMovingMean(container->meanContainer, sample);

	if (container->meanContainer->count < 2) {
		container->stdDev = 0;
	} else {

		container->squareSum = 0;
		for(i = 0; i < container->meanContainer->count; i++) {
			container->squareSum += ( container->meanContainer->samples[i] - container->meanContainer->mean ) *
						( container->meanContainer->samples[i] - container->meanContainer->mean );
		}

		container->stdDev = sqrt ( container->squareSum /
					    (container->meanContainer->count - 1 ));

	}

	return container->stdDev;

}

DoubleMovingMean*
createDoubleMovingMean(int capacity)
{

	DoubleMovingMean* container;
	if ( !(container = calloc (1, sizeof(DoubleMovingMean))) ) {
	    return NULL;
	}

	container->capacity = (capacity > STATCONTAINER_MAX_SAMPLES ) ?
			STATCONTAINER_MAX_SAMPLES : capacity;
	if ( !(container->samples = calloc(container->capacity, sizeof(double))) ) {
	    free(container);
	    return NULL;
	}
	return container;

}

void
freeDoubleMovingMean(DoubleMovingMean** container)
{

	if(*container == NULL) {
	    return;
	}
	free((*container)->samples);
	free(*container);
	*container = NULL;

}

void
resetDoubleMovingMean(DoubleMovingMean* container)
{

	if(container == NULL)
	    return;
	container->sum = 0;
	container->mean = 0;
	container->count = 0;
	container->counter = 0;
	memset(container->samples, 0, sizeof(&container->samples));

}


double
feedDoubleMovingMean(DoubleMovingMean* container, double sample)
{

	if(container == NULL)
	    return 0;
        /* sample buffer is full */
        if ( container->count == container->capacity ) {
		/* keep the sum current - drop the oldest value */
		container->sum -= container->samples[0];
		/* shift the sample buffer left, dropping the oldest sample */
		memmove(container->samples, container->samples + 1,
			sizeof(sample) * (container->capacity - 1) );
		/* counter will be incremented further, so we decrement it here */
		container->count--;
		container->full = TRUE;
	}
	container->samples[container->count++] = sample;
	container->sum += sample;
	container->mean = container->sum / container->count;

		container->counter++;
		container->counter = container->counter % container->capacity;
//		INFO("c: %d\n", container->counter);

	return container->mean;

}

DoubleMovingStdDev* createDoubleMovingStdDev(int capacity)
{

	DoubleMovingStdDev* container;
	if ( !(container = calloc (1, sizeof(DoubleMovingStdDev))) ) {
		return NULL;
	}

	if((container->meanContainer = createDoubleMovingMean(capacity))
		== NULL) {
		free(container);
		return NULL;
	}

	return container;

}

void
freeDoubleMovingStdDev(DoubleMovingStdDev** container)
{

	freeDoubleMovingMean(&((*container)->meanContainer));
	free(*container);
	*container = NULL;

}

void
resetDoubleMovingStdDev(DoubleMovingStdDev* container)
{

	if(container == NULL)
	    return;
	resetDoubleMovingMean(container->meanContainer);
	container->squareSum = 0.0;
	container->stdDev = 0.0;

}
double
feedDoubleMovingStdDev(DoubleMovingStdDev* container, double sample)
{

	int i = 0;

	if(container == NULL)
		return 0.0;

	feedDoubleMovingMean(container->meanContainer, sample);

	if (container->meanContainer->count < 2) {
		container->stdDev = 0.0;
	} else {

		container->squareSum = 0.0;
		for(i = 0; i < container->meanContainer->count; i++) {
			container->squareSum += ( container->meanContainer->samples[i] - container->meanContainer->mean ) *
						( container->meanContainer->samples[i] - container->meanContainer->mean );
		}

		container->stdDev = sqrt ( container->squareSum /
					    (container->meanContainer->count - 1));

	}

	if(container->meanContainer->counter == 0) {
		container->periodicStdDev = container->stdDev;
	}

	return container->stdDev;

}

IntMovingStatFilter* createIntMovingStatFilter(StatFilterOptions *config, const char* id)
{

	IntMovingStatFilter* container;

	if(config->filterType > FILTER_MAXVALUE) {
		return NULL;
	}

	if ( !(container = calloc (1, sizeof(IntMovingStatFilter))) ) {
		return NULL;
	}

	if((container->meanContainer = createIntMovingMean(config->windowSize))
		== NULL) {
		free(container);
		return NULL;
	}

	container->filterType = config->filterType;
	container->windowType = config->windowType;

	if(config->windowSize < 2) container->windowType = WINDOW_SLIDING;

	strncpy(container->identifier, id, 10);

	return container;

}

void
freeIntMovingStatFilter(IntMovingStatFilter** container)
{

	freeIntMovingMean(&((*container)->meanContainer));
	free(*container);
	*container = NULL;

}

void
resetIntMovingStatFilter(IntMovingStatFilter* container)
{

	if(container == NULL)
	    return;
	resetIntMovingMean(container->meanContainer);
	container->output = 0;

}

Boolean
feedIntMovingStatFilter(IntMovingStatFilter* container, int32_t sample)
{
	if(container == NULL)
		return 0;

	if(container->filterType == FILTER_NONE) {

	    container->output = sample;
	    return TRUE;

	} else {
	    /* cheat - the mean container is used as a general purpose sliding window */
	    feedIntMovingMean(container->meanContainer, sample);

	}

	container->counter++;
	container->counter = container->counter % container->meanContainer->capacity;

	switch(container->filterType) {

	    case FILTER_MEAN:

		container->output = container->meanContainer->mean;
		break;

	    case FILTER_MEDIAN:
		{
		    int count = container->meanContainer->count;
		    int32_t sortedSamples[count];

		    Boolean odd = ((count %2 ) == 1);
	
		    memcpy(sortedSamples, container->meanContainer->samples, count * sizeof(sample));

		    qsort(sortedSamples, count, sizeof(sample), cmpInt32);

		    if(odd) {

			    container->output = sortedSamples[(count / 2)];

		    } else {

			    container->output = (sortedSamples[(count / 2) -1] + sortedSamples[(count / 2)]) / 2;

		    }

		}
		break;

	    case FILTER_MIN:
		{
		    int count = container->meanContainer->count;
		    int32_t sortedSamples[count];
		    memcpy(sortedSamples, container->meanContainer->samples, count * sizeof(sample));
		    qsort(sortedSamples, count, sizeof(sample), cmpInt32);
		    container->output = sortedSamples[0];

		}
		break;

	    case FILTER_MAX:
		{
		    int count = container->meanContainer->count;
		    int32_t sortedSamples[count];
		    memcpy(sortedSamples, container->meanContainer->samples, count * sizeof(sample));
		    qsort(sortedSamples, count, sizeof(sample), cmpInt32);
		    container->output = sortedSamples[count - 1];

		}
		break;

	    case FILTER_ABSMIN:
		{
		    int count = container->meanContainer->count;
		    int32_t sortedSamples[count];
		    memcpy(sortedSamples, container->meanContainer->samples, count * sizeof(sample));
		    qsort(sortedSamples, count, sizeof(sample), cmpAbsInt32);
		    container->output = sortedSamples[0];

		}
		break;

	    case FILTER_ABSMAX:
		{
		    int count = container->meanContainer->count;
		    int32_t sortedSamples[count];
		    memcpy(sortedSamples, container->meanContainer->samples, count * sizeof(sample));
		    qsort(sortedSamples, count, sizeof(sample), cmpAbsInt32);
		    container->output = sortedSamples[count - 1];

		}
		break;
	    default:
		container->output = sample;
		return TRUE;
	}
		

	DBGV("filter %s, Sample %d output %d\n", container->identifier, sample, container->output);

	if((container->windowType == WINDOW_INTERVAL) && (container->counter != 0)) {
		return FALSE;
	}
	return TRUE;

}

DoubleMovingStatFilter* createDoubleMovingStatFilter(StatFilterOptions *config, const char* id)
{
	DoubleMovingStatFilter* container;

	if(config->filterType > FILTER_MAXVALUE) {
		return NULL;
	}

	if ( !(container = calloc (1, sizeof(DoubleMovingStatFilter))) ) {
		return NULL;
	}

	if((container->meanContainer = createDoubleMovingMean(config->windowSize))
		== NULL) {
		free(container);
		return NULL;
	}

	container->filterType = config->filterType;
	container->windowType = config->windowType;

	if(config->windowSize < 2) container->windowType = WINDOW_SLIDING;

	strncpy(container->identifier, id, 10);

	return container;

}

void
freeDoubleMovingStatFilter(DoubleMovingStatFilter** container)
{

	if((container==NULL) || (*container==NULL)) {
	    return;
	}
	freeDoubleMovingMean(&((*container)->meanContainer));
	free(*container);
	*container = NULL;

}

void
resetDoubleMovingStatFilter(DoubleMovingStatFilter* container)
{

	if(container == NULL)
	    return;
	resetDoubleMovingMean(container->meanContainer);
	container->output = 0;

}

Boolean
feedDoubleMovingStatFilter(DoubleMovingStatFilter* container, double sample)
{

	if(container == NULL)
		return 0;

	if(container->filterType == FILTER_NONE) {

	    container->output = sample;
	    return TRUE;

	} else {
	    /* cheat - the mean container is used as a general purpose sliding window */
	    feedDoubleMovingMean(container->meanContainer, sample);
	}

	container->counter++;
	container->counter = container->counter % container->meanContainer->capacity;

	switch(container->filterType) {

	    case FILTER_MEAN:

		container->output = container->meanContainer->mean;
		break;

	    case FILTER_MEDIAN:
		{
		    int count = container->meanContainer->count;
		    double sortedSamples[count];

		    Boolean odd = ((count %2 ) == 1);
	
		    memcpy(sortedSamples, container->meanContainer->samples, count * sizeof(sample));

		    qsort(sortedSamples, count, sizeof(sample), cmpDouble);

		    if(odd) {

			    container->output = sortedSamples[(count / 2)];

		    } else {

			    container->output = (sortedSamples[(count / 2) -1] + sortedSamples[(count / 2)]) / 2;

		    }

		}
		break;

	    case FILTER_MIN:
		{
		    int count = container->meanContainer->count;
		    double sortedSamples[count];
		    memcpy(sortedSamples, container->meanContainer->samples, count * sizeof(sample));
		    qsort(sortedSamples, count, sizeof(sample), cmpDouble);
		    container->output = sortedSamples[0];

		}
		break;

	    case FILTER_MAX:
		{
		    int count = container->meanContainer->count;
		    double sortedSamples[count];
		    memcpy(sortedSamples, container->meanContainer->samples, count * sizeof(sample));
		    qsort(sortedSamples, count, sizeof(sample), cmpDouble);
		    container->output = sortedSamples[count - 1];

		}
		break;

	    case FILTER_ABSMIN:
		{
		    int count = container->meanContainer->count;
		    double sortedSamples[count];
		    memcpy(sortedSamples, container->meanContainer->samples, count * sizeof(sample));
		    qsort(sortedSamples, count, sizeof(sample), cmpAbsDouble);
		    container->output = sortedSamples[0];

		}
		break;

	    case FILTER_ABSMAX:
		{
		    int count = container->meanContainer->count;
		    double sortedSamples[count];
		    memcpy(sortedSamples, container->meanContainer->samples, count * sizeof(sample));
		    qsort(sortedSamples, count, sizeof(sample), cmpAbsDouble);
		    container->output = sortedSamples[count - 1];

		}
		break;
	    default:
		container->output = sample;
		return TRUE;
	}

	DBGV("Filter %s, Sample %.09f output %.09f\n", container->identifier, sample, container->output);

	if((container->windowType == WINDOW_INTERVAL) && (container->counter != 0)) {
		return FALSE;
	}
	return TRUE;

}

double getpeircesCriterion(int numObservations, int numDoubtful) {

    static const double peircesTable[60][9] = {
/* 1 - 10 samples */
        {-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
        {-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
	{1.196,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
	{1.383,	1.078,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
	{1.509,	1.2,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
	{1.61,	1.299,	1.099,	-1,	-1,	-1,	-1,	-1,	-1},
	{1.693,	1.382,	1.187,	1.022,	-1,	-1,	-1,	-1,	-1},
	{1.763,	1.453,	1.261,	1.109,	-1,	-1,	-1,	-1,	-1},
	{1.824,	1.515,	1.324,	1.178,	1.045,	-1,	-1,	-1,	-1},
	{1.878,	1.57,	1.38,	1.237,	1.114,	-1,	-1,	-1,	-1},
/* 11 - 20 samples */
	{1.925,	1.619,	1.43,	1.289,	1.172,	1.059,	-1,	-1,	-1},
	{1.969,	1.663,	1.475,	1.336,	1.221,	1.118,	1.009,	-1,	-1},
	{2.007,	1.704,	1.516,	1.379,	1.266,	1.167,	1.07,	-1,	-1},
	{2.043,	1.741,	1.554,	1.417,	1.307,	1.21,	1.12,	1.026,	-1},
	{2.076,	1.775,	1.589,	1.453,	1.344,	1.249,	1.164,	1.078,	-1},
	{2.106,	1.807,	1.622,	1.486,	1.378,	1.285,	1.202,	1.122,	1.039},
	{2.134,	1.836,	1.652,	1.517,	1.409,	1.318,	1.237,	1.161,	1.084},
	{2.161,	1.864,	1.68,	1.546,	1.438,	1.348,	1.268,	1.195,	1.123},
	{2.185,	1.89,	1.707,	1.573,	1.466,	1.377,	1.298,	1.226,	1.158},
	{2.209,	1.914,	1.732,	1.599,	1.492,	1.404,	1.326,	1.255,	1.19},
/* 21 - 30 samples */
	{2.23,	1.938,	1.756,	1.623,	1.517,	1.429,	1.352,	1.282,	1.218},
	{2.251,	1.96,	1.779,	1.646,	1.54,	1.452,	1.376,	1.308,	1.245},
	{2.271,	1.981,	1.8,	1.668,	1.563,	1.475,	1.399,	1.332,	1.27},
	{2.29,	2,	1.821,	1.689,	1.584,	1.497,	1.421,	1.354,	1.293},
	{2.307,	2.019,	1.84,	1.709,	1.604,	1.517,	1.442,	1.375,	1.315},
	{2.324,	2.037,	1.859,	1.728,	1.624,	1.537,	1.462,	1.396,	1.336},
	{2.341,	2.055,	1.877,	1.746,	1.642,	1.556,	1.481,	1.415,	1.356},
	{2.356,	2.071,	1.894,	1.764,	1.66,	1.574,	1.5,	1.434,	1.375},
	{2.371,	2.088,	1.911,	1.781,	1.677,	1.591,	1.517,	1.452,	1.393},
/* 31 - 40 samples */
	{2.385,	2.103,	1.927,	1.797,	1.694,	1.608,	1.534,	1.469,	1.411},
	{2.399,	2.118,	1.942,	1.812,	1.71,	1.624,	1.55,	1.486,	1.428},
	{2.412,	2.132,	1.957,	1.828,	1.725,	1.64,	1.567,	1.502,	1.444},
	{2.425,	2.146,	1.971,	1.842,	1.74,	1.655,	1.582,	1.517,	1.459},
	{2.438,	2.159,	1.985,	1.856,	1.754,	1.669,	1.597,	1.532,	1.475},
	{2.45,	2.172,	1.998,	1.87,	1.768,	1.683,	1.611,	1.547,	1.489},
	{2.461,	2.184,	2.011,	1.883,	1.782,	1.697,	1.624,	1.561,	1.504},
	{2.472,	2.196,	2.024,	1.896,	1.795,	1.711,	1.638,	1.574,	1.517},
	{2.483,	2.208,	2.036,	1.909,	1.807,	1.723,	1.651,	1.587,	1.531},
/* 41 - 50 samples */
	{2.494,	2.219,	2.047,	1.921,	1.82,	1.736,	1.664,	1.6,	1.544},
	{2.504,	2.23,	2.059,	1.932,	1.832,	1.748,	1.676,	1.613,	1.556},
	{2.514,	2.241,	2.07,	1.944,	1.843,	1.76,	1.688,	1.625,	1.568},
	{2.524,	2.251,	2.081,	1.955,	1.855,	1.771,	1.699,	1.636,	1.58},
	{2.533,	2.261,	2.092,	1.966,	1.866,	1.783,	1.711,	1.648,	1.592},
	{2.542,	2.271,	2.102,	1.976,	1.876,	1.794,	1.722,	1.659,	1.603},
	{2.551,	2.281,	2.112,	1.987,	1.887,	1.804,	1.733,	1.67,	1.614},
	{2.56,	2.29,	2.122,	1.997,	1.897,	1.815,	1.743,	1.681,	1.625},
	{2.568,	2.299,	2.131,	2.006,	1.907,	1.825,	1.754,	1.691,	1.636},
	{2.577,	2.308,	2.14,	2.016,	1.917,	1.835,	1.764,	1.701,	1.646},
/* 51 - 60 * samples */
	{2.585,	2.317,	2.149,	2.026,	1.927,	1.844,	1.773,	1.711,	1.656},
	{2.592,	2.326,	2.158,	2.035,	1.936,	1.854,	1.783,	1.721,	1.666},
	{2.6,	2.334,	2.167,	2.044,	1.945,	1.863,	1.792,	1.73,	1.675},
	{2.608,	2.342,	2.175,	2.052,	1.954,	1.872,	1.802,	1.74,	1.685},
	{2.615,	2.35,	2.184,	2.061,	1.963,	1.881,	1.811,	1.749,	1.694},
	{2.622,	2.358,	2.192,	2.069,	1.972,	1.89,	1.82,	1.758,	1.703},
	{2.629,	2.365,	2.2,	2.077,	1.98,	1.898,	1.828,	1.767,	1.711},
	{2.636,	2.373,	2.207,	2.085,	1.988,	1.907,	1.837,	1.775,	1.72},
	{2.643,	2.38,	2.215,	2.093,	1.996,	1.915,	1.845,	1.784,	1.729},
	{2.65,	2.387,	2.223,	2.101,	2.004,	1.923,	1.853,	1.792,	1.737},
	{2.656,	2.394,	2.23,	2.109,	2.012,	1.931,	1.861,	1.8,	1.745},
	{2.663,	2.401,	2.237,	2.116,	2.019,	1.939,	1.869,	1.808,	1.753},
    };

    if ( numObservations < 1 || numObservations > 60)
	return -1.0;
    if ( numDoubtful < 1 || numDoubtful > 9)
	return -1.0;

    return(peircesTable[numObservations - 1][numDoubtful - 1]);

}

Boolean isIntPeircesOutlier(IntMovingStdDev *container, int32_t sample, double threshold) {

	double maxDev;

	/* Sanity check - race condition was seen when enabling and disabling filters repeatedly */
	if(container == NULL || container->meanContainer == NULL)
		return FALSE;

	maxDev = container->stdDev * getpeircesCriterion(container->meanContainer->count, 1) * threshold;

	/*
	 * Two cases:
	 * - Too early - we got a -1 from Peirce's table,
	 * - safeguard: std dev is zero and filter is blocking
	 *   everything, hus, we let the sample through
	 */
	if (maxDev <= 0.0 ) return FALSE;

	if(fabs((double)(sample - container->meanContainer->mean)) > maxDev) {
	DBGV("Peirce %s outlier: val: %d, cnt: %d, mxd: %.09f (%.03f * dev * %.03f), dev: %.09f, mea: %.09f, dif: %.09f\n", container->identifier,
		sample, container->meanContainer->count, maxDev, getpeircesCriterion(container->meanContainer->count, 1), threshold,
		container->stdDev,
		container->meanContainer->mean, fabs(sample - container->meanContainer->mean));
            return TRUE;
	}

	return FALSE;

}

Boolean isDoublePeircesOutlier(DoubleMovingStdDev *container, double sample, double threshold) {

	double maxDev;

	/* Sanity check - race condition was seen when enabling and disabling filters repeatedly */
	if(container == NULL || container->meanContainer == NULL)
		return FALSE;

	maxDev = container->stdDev * getpeircesCriterion(container->meanContainer->count, 1) * threshold;

	/*
	 * Two cases:
	 * - Too early - we got a -1 from Peirce's table,
	 * - safeguard: std dev is zero and filter is blocking
	 *   everything, thus, we let the sample through
	 */
	if (maxDev <= 0.0 ) {
		return FALSE;
	}

	if(fabs((double)(sample - container->meanContainer->mean)) > maxDev) {
	DBG("Peirce %s outlier: val: %.09f, cnt: %d, mxd: %.09f (%.03f * dev * %.03f), dev: %.09f, mea: %.09f, dif: %.09f\n", container->identifier,
		sample, container->meanContainer->count, maxDev, getpeircesCriterion(container->meanContainer->count, 1), threshold,
		container->stdDev,
		container->meanContainer->mean, fabs(sample - container->meanContainer->mean));
            return TRUE;
	}

	return FALSE;

}

void
clearPtpEngineSlaveStats(PtpEngineSlaveStats* stats)
{
	memset(stats, 0, sizeof(*stats));
}

void
resetPtpEngineSlaveStats(PtpEngineSlaveStats* stats) {

	resetDoublePermanentStdDev(&stats->ofmStats);
	resetDoublePermanentStdDev(&stats->mpdStats);
	resetDoublePermanentMedian(&stats->ofmMedianContainer);
	resetDoublePermanentMedian(&stats->mpdMedianContainer);
	stats->ofmStatsUpdated = FALSE;
	stats->mpdStatsUpdated = FALSE;
}
