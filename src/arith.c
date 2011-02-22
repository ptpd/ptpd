/*-
 * Copyright (c) 2009-2011 George V. Neville-Neil, Steven Kreuzer, 
 *                         Martin Burnicki
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
 *
 * All Rights Reserved
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

/// Divide an internal time value
///
/// @param r the time to convert
/// @param divisor 
///
void
divTime(TimeInternal *r, int divisor)
{

	uint64_t nanoseconds;

	if (divisor <= 0)
		return;

	nanoseconds = ((uint64_t)r->seconds * 1000000000) + r->nanoseconds;
	nanoseconds /= divisor;

	r->seconds = nanoseconds / 1000000000;
	r->nanoseconds = nanoseconds % 1000000000;

}
