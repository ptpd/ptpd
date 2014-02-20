/**
 * @file    moving_average.c
 * @authors Jan Breuer
 * @date   Thu Feb 20 10:25:22 CET 2014
 * 
 * Moving average filer
 */

#include <stdint.h>
#include <stdlib.h>

#include "filter.h"
#include "moving_average.h"

typedef struct
{
	Filter filter;
	int32_t x_prev;
	int32_t y;
} MovingAverage;

static void MovingAverageClear(Filter * filter)
{
	MovingAverage * maf = (MovingAverage *)filter;

	maf->y = 0;
	maf->x_prev = 0;
}

static double MovingAverageFeed(Filter * filter, double val)
{
	MovingAverage * maf = (MovingAverage *)filter;
	int32_t x = (int32_t)(val);

	maf->y = x / 2 + maf->x_prev / 2;
	maf->x_prev = x;
	return maf->y;
}

static void MovingAverageConfigure(Filter *filter, const char * parameter, double value)
{
	(void) filter;
	(void) parameter;
	(void) value;
}

static void MovingAverageDestroy(Filter * filter)
{
	MovingAverage * maf = (MovingAverage *)filter;
	free(maf);
}

Filter * MovingAverageCreate(void)
{
	MovingAverage * maf;

	maf = calloc(1, sizeof(MovingAverage));

	if (!maf) {
		return NULL;
	}

	maf->filter.feed = MovingAverageFeed;
	maf->filter.clear = MovingAverageClear;
	maf->filter.destroy = MovingAverageDestroy;
	maf->filter.configure = MovingAverageConfigure;

	return &maf->filter;
}




