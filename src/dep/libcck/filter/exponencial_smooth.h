#ifndef _LIBCCK_FILTER_EXPONENCIAL_SMOOTH_H_
#define _LIBCCK_FILTER_EXPONENCIAL_SMOOTH_H_

/**
 * @file    exponencial_smooth.h
 * @authors Jan Breuer
 * @date   Thu Feb 20 10:25:22 CET 2014
 * 
 * Exponencial smooth filter implementation
 */

#include "filter.h"

Filter * ExponencialSmoothCreate(const char * type, const char * name);


#endif /* _LIBCCK_FILTER_EXPONENCIAL_SMOOTH_H_ */

