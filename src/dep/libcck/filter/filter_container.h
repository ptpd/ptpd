#ifndef _LIBCCK_FILTER_FILTER_CONTAINER_H_
#define _LIBCCK_FILTER_FILTER_CONTAINER_H_

/**
 * @file    filter_container.h
 * @authors Jan Breuer
 * @date   Mon Feb 24 10:30:00 CET 2014
 * 
 * filter container
 */

#include "../base/cckcontainer.h"
#include "../../iniparser/dictionary.h"


typedef struct _FilterContainer FilterContainer;

struct _FilterContainer {
	CCKContainer cckc;
};

FilterContainer * FilterContainerCreate(void);

void FilterContainerInit(FilterContainer * container);

void FilterContainerLoad(FilterContainer * container, dictionary * dict);

void FilterContainerDestroy(FilterContainer * container);

#endif /* _LIBCCK_FILTER_FILTER_CONTAINER_H_ */

