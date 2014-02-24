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

static BOOL MovingAverageFeed(Filter * filter, int32_t * value)
{
	MovingAverage * maf = (MovingAverage *)filter;
	int32_t x = *value;

	maf->y = x / 2 + maf->x_prev / 2;
	maf->x_prev = x;
	
	*value = maf->y;
	return TRUE;
}

static void MovingAverageConfigure(Filter *filter, const char * parameter, const char * value)
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

Filter * MovingAverageCreate(const char * type, const char * name)
{
	MovingAverage * maf;

	maf = calloc(1, sizeof(MovingAverage));

	if (!maf) {
		return NULL;
	}

	cckObjectInit(CCK_OBJECT(maf), type, name);

	maf->filter.feed = MovingAverageFeed;
	maf->filter.clear = MovingAverageClear;
	maf->filter.destroy = MovingAverageDestroy;
	maf->filter.configure = MovingAverageConfigure;

	return &maf->filter;
}




