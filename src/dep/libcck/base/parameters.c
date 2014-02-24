/**
 * @file    parameters.c
 * @authors Jan Breuer
 * @date   Mon Feb 24 09:20:00 CET 2014
 * 
 * raw parameters handling
 */

#include <stdio.h>
#include "parameters.h"

double doubleGet(const char * val, double defValue) {
	double result;
	if (val == NULL) {
		result = defValue;
	} else {
		if (sscanf(val, " %lf ", &result) != 1) {
			result = defValue;
		}
	}

	return result;
}

int32_t intGet(const char * val, int32_t defValue) {
	int32_t result;
	if (val == NULL) {
		result = defValue;
	} else {
		if (sscanf(val, " %d ", &result) != 1) {
			result = defValue;
		}
	}

	return result;
}