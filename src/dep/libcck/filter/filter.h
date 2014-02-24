#ifndef _LIBCCK_FILTER_FILTER_H_
#define _LIBCCK_FILTER_FILTER_H_

/**
 * @file    filter.h
 * @authors Jan Breuer
 * @date   Thu Feb 20 10:25:22 CET 2014
 * 
 * filter API
 */

#include "../base/ccktypes.h"
#include "../base/cckobject.h"

#define FILTER_EXPONENTIAL_SMOOTH       "exps"
#define FILTER_MOVING_AVERAGE           "mav"

typedef struct _Filter Filter;

struct _Filter {
	CCKObject ccko;
	BOOL (*feed)(Filter *filter, int32_t * value);
	void (*clear)(Filter *filter);
	void (*destroy)(Filter *filter);
	void (*configure)(Filter *filter, const char * parameter, const char * value);
};


#define CCK_FILTER(obj)		((Filter *)obj)

Filter * FilterCreate(const char * type, const char * name);

void FilterDestroy(Filter * filter);

void FilterClear(Filter * filter);

BOOL FilterFeed(Filter * filter, int32_t * value);

void FilterConfigure(Filter *filter, const char * parameter, const char * value);

#endif /* _LIBCCK_FILTER_FILTER_H_ */