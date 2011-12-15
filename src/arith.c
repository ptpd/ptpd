/**
 * @file   arith.c
 * @date   Tue Jul 20 16:12:51 2010
 * 
 * @brief  Time format conversion routines and additional math functions.
 * 
 * 
 */

#include "ptpd.h"


void 
integer64_to_internalTime(Integer64 bigint, TimeInternal * internal)
{
	int s_msb;
	double ns_msb;
	int entire;
	char *p_lsb, *p_msb;
	Boolean negative = FALSE;

	p_lsb = (char *)&bigint.lsb;
	/* lsb type is unsigned int */
	p_msb = (char *)&bigint.msb;
	/* msb type is int */
	/* Test if bigint is a negative number */
	negative = ((bigint.msb & 0x80000000) == 0x80000000);

	if (!negative) {
		/* Positive case */

		/* fractional nanoseconds are excluded (see 5.3.2) */
		bigint.lsb = bigint.lsb >> 16;
		/*
		 * copy two least significant octet of msb to most
		 * significant octet of lsb
		 */
		*(p_lsb + 2) = *p_msb;
		*(p_lsb + 3) = *(p_msb + 1);
		bigint.msb = bigint.msb >> 16;

		internal->nanoseconds = bigint.lsb % 1000000000;
		internal->seconds = bigint.lsb / 1000000000;

		/* (2^32 / 10^9) = 4,294967296 */
		s_msb = 4 * bigint.msb;
		ns_msb = 0.294967296 * (double)bigint.msb;
		entire = (int)ns_msb;
		s_msb += entire;
		ns_msb -= entire;
		ns_msb *= 1000000000;
		internal->nanoseconds = (float)internal->nanoseconds + 
			(float)ns_msb;
		internal->seconds += s_msb;
		normalizeTime(internal);

	}
	/* End of positive Case */

	else {				/* Negative case */

		/* Take the two complement */
		bigint.lsb = ~bigint.lsb;
		bigint.msb = ~bigint.msb;

		if (bigint.lsb == 0xffffffff) {
			bigint.lsb = 0;
			bigint.msb++;
		} else {
			bigint.lsb++;
		}

		/* fractional nanoseconds are excluded (see 5.3.2) */
		bigint.lsb = bigint.lsb >> 16;
		/*
		 * copy two least significant octet of msb to most
		 * significant octet of lsb
		 */
		*(p_lsb + 2) = *p_msb;
		*(p_lsb + 3) = *(p_msb + 1);
		bigint.msb = bigint.msb >> 16;

		internal->nanoseconds = bigint.lsb % 1000000000;
		internal->seconds = bigint.lsb / 1000000000;

		/* (2^32 / 10^9) = 4,294967296 */
		s_msb = 4 * bigint.msb;
		ns_msb = 0.294967296 * (double)bigint.msb;
		entire = (int)ns_msb;
		s_msb += entire;
		ns_msb -= entire;
		ns_msb *= 1000000000;

		internal->nanoseconds = (float)internal->nanoseconds + 
			(float)ns_msb;
		internal->seconds += s_msb;
		normalizeTime(internal);

		internal->nanoseconds = -internal->nanoseconds;
		internal->seconds = -internal->seconds;
	}
	/* End of negative Case */
}


void 
fromInternalTime(TimeInternal * internal, Timestamp * external)
{

	/*
	 * fromInternalTime is only used to convert time given by the system
	 * to a timestamp As a consequence, no negative value can normally
	 * be found in (internal)
	 * 
	 * Note that offsets are also represented with TimeInternal structure,
	 * and can be negative, but offset are never convert into Timestamp
	 * so there is no problem here.
	 */

	if ((internal->seconds & ~INT_MAX) || 
	    (internal->nanoseconds & ~INT_MAX)) {
		DBG("Negative value canno't be converted into timestamp \n");
		return;
	} else {
		external->secondsField.lsb = internal->seconds;
		external->nanosecondsField = internal->nanoseconds;
		external->secondsField.msb = 0;
	}

}

void 
toInternalTime(TimeInternal * internal, Timestamp * external)
{

	/* Program will not run after 2038... */
	if (external->secondsField.lsb < INT_MAX) {
		internal->seconds = external->secondsField.lsb;
		internal->nanoseconds = external->nanosecondsField;
	} else {
		DBG("Clock servo canno't be executed : "
		    "seconds field is higher than signed integer (32bits) \n");
		return;
	}
}

void 
normalizeTime(TimeInternal * r)
{
	r->seconds += r->nanoseconds / 1000000000;
	r->nanoseconds -= r->nanoseconds / 1000000000 * 1000000000;

	if (r->seconds > 0 && r->nanoseconds < 0) {
		r->seconds -= 1;
		r->nanoseconds += 1000000000;
	} else if (r->seconds < 0 && r->nanoseconds > 0) {
		r->seconds += 1;
		r->nanoseconds -= 1000000000;
	}
}

void 
addTime(TimeInternal * r, TimeInternal * x, TimeInternal * y)
{
	r->seconds = x->seconds + y->seconds;
	r->nanoseconds = x->nanoseconds + y->nanoseconds;

	normalizeTime(r);
}

void 
subTime(TimeInternal * r, TimeInternal * x, TimeInternal * y)
{
	r->seconds = x->seconds - y->seconds;
	r->nanoseconds = x->nanoseconds - y->nanoseconds;

	normalizeTime(r);
}
