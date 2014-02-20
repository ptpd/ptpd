/**
 * @file    exponencial_smooth.c
 * @authors Jan Breuer
 * @date   Thu Feb 20 10:25:22 CET 2014
 * 
 * Exponencial smooth filter
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "filter.h"
#include "exponencial_smooth.h"

typedef struct
{
	Filter filter;
	
	int16_t s;
	
	int32_t s_exp;
	int32_t x_prev;
	int32_t y;
} ExponencialSmooth;

static void ExponencialSmoothClear(Filter * filter)
{
	ExponencialSmooth * esf = (ExponencialSmooth *)filter;

	esf->y = 0;
	esf->x_prev = 0;
	esf->s_exp = 0;
}

static double ExponencialSmoothFeed(Filter * filter, double val)
{
	ExponencialSmooth * esf = (ExponencialSmooth *)filter;
	int32_t x = (int32_t)(val);
	int16_t s;

	/* avoid overflowing filter */
	s = esf->s;
	while (abs(esf->y) >> (31 - s))
		--s;

	/* crank down filter cutoff by increasing 's_exp' */
	if (esf->s_exp < 1)
		esf->s_exp = 1;
	else if (esf->s_exp < 1 << s)
		++esf->s_exp;
	else if (esf->s_exp > 1 << s)
		esf->s_exp = 1 << s;

	/* filter 'meanPathDelay' */
	esf->y = (esf->s_exp - 1) * esf->y / esf->s_exp +
		(x / 2 + esf->x_prev / 2) / esf->s_exp;

	esf->x_prev = x;
	return esf->y;
}

static void ExponencialSmoothConfigure(Filter *filter, const char * parameter, double value)
{
	ExponencialSmooth * esf = (ExponencialSmooth *)filter;

	if (strcmp(parameter, "s") == 0) {
		esf->s = (int32_t)(value);
	}
}

static void ExponencialSmoothDestroy(Filter * filter)
{
	ExponencialSmooth * esf = (ExponencialSmooth *)filter;
	free(esf);
}

Filter * ExponencialSmoothCreate(void)
{
	ExponencialSmooth * esf;

	esf = calloc(1, sizeof(ExponencialSmooth));

	if (!esf) {
		return NULL;
	}

	esf->filter.feed = ExponencialSmoothFeed;
	esf->filter.clear = ExponencialSmoothClear;
	esf->filter.destroy = ExponencialSmoothDestroy;
	esf->filter.configure = ExponencialSmoothConfigure;

	return &esf->filter;
}