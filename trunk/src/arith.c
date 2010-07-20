/**
 * @file   arith.c
 * @date   Tue Jul 20 16:12:51 2010
 * 
 * @brief  Time format conversion routines and additional math functions.
 * 
 * 
 */

#include "ptpd.h"

/* from annex C of the spec */
UInteger32 
crc_algorithm(Octet * buf, Integer16 length)
{
	Integer16 i;
	UInteger8 data;
	UInteger32 polynomial = 0xedb88320, crc = 0xffffffff;

	while (length-- > 0) {
		data = *(UInteger8 *) (buf++);

		for (i = 0; i < 8; i++) {
			if ((crc ^ data) & 1) {
				crc = (crc >> 1);
				crc ^= polynomial;
			} else {
				crc = (crc >> 1);
			}
			data >>= 1;
		}
	}

	return crc ^ 0xffffffff;
}

UInteger32 
sum(Octet * buf, Integer16 length)
{
	UInteger32 sum = 0;

	while (length-- > 0)
		sum += *(UInteger8 *) (buf++);

	return sum;
}

void 
fromInternalTime(TimeInternal * internal, TimeRepresentation * external, Boolean halfEpoch)
{
	external->seconds = labs(internal->seconds) + halfEpoch * INT_MAX;

	if (internal->seconds < 0 || internal->nanoseconds < 0) {
		external->nanoseconds = labs(internal->nanoseconds) | ~INT_MAX;
	} else {
		external->nanoseconds = labs(internal->nanoseconds);
	}

	DBGV("fromInternalTime: %10ds %11dns -> %10us %11dns\n",
	    internal->seconds, internal->nanoseconds,
	    external->seconds, external->nanoseconds);
}

void 
toInternalTime(TimeInternal * internal, TimeRepresentation * external, Boolean * halfEpoch)
{
	*halfEpoch = external->seconds / INT_MAX;

	if (external->nanoseconds & ~INT_MAX) {
		internal->seconds = -(external->seconds % INT_MAX);
		internal->nanoseconds = -(external->nanoseconds & INT_MAX);
	} else {
		internal->seconds = external->seconds % INT_MAX;
		internal->nanoseconds = external->nanoseconds;
	}

	DBGV("toInternalTime: %10ds %11dns <- %10us %11dns\n",
	    internal->seconds, internal->nanoseconds,
	    external->seconds, external->nanoseconds);
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
