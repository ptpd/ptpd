#ifndef _LIBCCK_BASE_PARAMETERS_H_
#define _LIBCCK_BASE_PARAMETERS_H_

/**
 * @file    parameters.h
 * @authors Jan Breuer
 * @date   Mon Feb 24 09:20:00 CET 2014
 * 
 * raw parameters handling
 */

#include <stdint.h>

double doubleGet(const char * val, double defValue);
int32_t intGet(const char * val, int32_t defValue);


#endif /* _LIBCCK_BASE_PARAMETERS_H_ */

