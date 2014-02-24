#ifndef _LIBCCK_FILTER_MOVING_AVERAGE_H_
#define _LIBCCK_FILTER_MOVING_AVERAGE_H_

/**
 * @file    moving_average.h
 * @authors Jan Breuer
 * @date   Thu Feb 20 10:25:22 CET 2014
 * 
 * Moving average filter implementation
 */

#include "filter.h"

Filter * MovingAverageCreate(const char * type, const char * name);

#endif /* _LIBCCK_FILTER_MOVING_AVERAGE_H_ */

