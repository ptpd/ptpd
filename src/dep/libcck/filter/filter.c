/**
 * @file    filter.c
 * @authors Jan Breuer
 * @date   Thu Feb 20 10:25:22 CET 2014
 * 
 * filter API
 */

#include <stdlib.h>

#include "filter.h"

#include "exponencial_smooth.h"
#include "moving_average.h"

Filter * FilterCreate(int type)
{
	switch(type) {
		case 1:
			return ExponencialSmoothCreate();
		case 2:
			return MovingAverageCreate();
		default:
			return NULL;
	}
}

void FilterDestroy(Filter * filter)
{
	filter->destroy(filter);
}

void FilterClear(Filter * filter)
{
	filter->clear(filter);
}

double FilterFeed(Filter * filter, double val)
{
	return filter->feed(filter, val);
}

void FilterConfigure(Filter *filter, const char * parameter, double value)
{
	return filter->configure(filter, parameter, value);
}