#ifndef _LIBCCK_FILTER_FILTER_H_
#define _LIBCCK_FILTER_FILTER_H_

/**
 * @file    filter.h
 * @authors Jan Breuer
 * @date   Thu Feb 20 10:25:22 CET 2014
 * 
 * filter API
 */

#define FILTER_EXPONENTIAL_SMOOTH       1
#define FILTER_MOVING_AVERAGE           2

typedef struct _Filter Filter;

struct _Filter {
	double (*feed)(Filter *filter, double val);
	void (*clear)(Filter *filter);
	void (*destroy)(Filter *filter);
	void (*configure)(Filter *filter, const char * parameter, double value);
};


Filter * FilterCreate(int type);

void FilterDestroy(Filter * filter);

void FilterClear(Filter * filter);

double FilterFeed(Filter * filter, double val);

void FilterConfigure(Filter *filter, const char * parameter, double value);

#endif /* _LIBCCK_FILTER_FILTER_H_ */