/* Copyright (c) 2017 Wojciech Owczarek,
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
 * @file   cck_timestamp.h
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  CCK Timestamp and associated conversion and timescale management functions
 *
 */

#ifndef CCK_TIMESTAMP_H_
#define CCK_TIMESTAMP_H_

#include <config.h>

#include <stdint.h>
#include <stdbool.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#define CCK_TIMESTAMP_STRLEN 30

/* timescale definitions */
#define TO_GPS_TAI	19	/* as of 6 Jan 1980, wink wink, nudge nudge */

	/*
	 * DO NOT REFER TO THESE TWO CONSTANTS DIRECTLY,
	 * as values may have been updated by leap second files or PTP.
	 * Use getTimescaleOffset() instead. These are for initialisation only.
	 */

/* allow default UTC-TAI offset to be defined at build time */
#ifdef CCK_BUILD_UTC_TAI_OFFSET
    #define TO_UTC_TAI CCK_BUILD_UTC_TAI_OFFSET
#else
    #define TO_UTC_TAI 37	/* as of first half 2017 */
#endif /* CCK_BUILD_UTC_TAI_OFFSET */

#define TO_UTC_GPS	(TO_UTC_TAI - TO_GPS_TAI)

enum {
    TS_ARB	= 0,	/* arbitrary / unknown: use timestamps as they come in */
    TS_UTC	= 1,	/* UTC */
    TS_TAI	= 2,	/* TAI */
    TS_GPS	= 3,	/* GPS */
    TS_MAX		/* marker */
};

#define TS_MIN TS_ARB

/*
 * macros for printing timestamps:
 * USE CCK_TSFDATA to include timescale, CCK_TSDATA for timestamp only
 * variable needs to be a pointer, reference a variable if need be.
 *
 * CckTimestamp bob = { 1488560103, 0, TS_UTC };
 * printf("offset is"CCK_TSFRMT"\n", CCK_TSDATA(&bob));
 *
 */

#define CCK_TSFRMT "%s%d.%.09d s%s%s"

#define CCK_TSDATA(var) \
((var)->seconds < 0 || (var)->nanoseconds < 0) ? "-" : " ",\
abs((var)->seconds), abs((var)->nanoseconds), \
"", ""

#define CCK_TFSDATA(var) \
((var)->seconds < 0 || (var)->nanoseconds < 0) ? "-" : " ",\
abs((var)->seconds), abs((var)->nanoseconds), \
" ", \
getTimescaleName((var)->timescale)

#define NS_PER_SEC 1E9

typedef struct {
	int32_t		seconds;
	int32_t		nanoseconds;
	int		timescale;
} CckTimestamp;

/* offset sample for use with filters and servos */
typedef struct {
	CckTimestamp	value;		/* value of the offset */
	CckTimestamp	at;		/* when sample taken */
} CckOffsetSample;

/* a collection of timestamp operations */
typedef struct {
	void (*add) (CckTimestamp *, const CckTimestamp *, const CckTimestamp *);
	void (*sub) (CckTimestamp *, const CckTimestamp *, const CckTimestamp *);
	double (*toDouble) (const CckTimestamp *);
	CckTimestamp (*fromDouble) (const double);
	void (*div2) (CckTimestamp *);
	void (*clear) (CckTimestamp *);
	CckTimestamp (*negative) (const CckTimestamp *);
	void (*abs) (CckTimestamp *);
	bool (*isNegative) (const CckTimestamp *);
	bool (*isZero) (const CckTimestamp *);
	int (*cmp) (const void *, const void *);
	void (*mean2) (CckTimestamp *, CckTimestamp *, CckTimestamp *);
	void (*rttCor) (CckTimestamp *, CckTimestamp *, CckTimestamp *, CckTimestamp *);
	void (*convert) (CckTimestamp*, const int timescale);
	CckTimestamp (*getConversion) (CckTimestamp*, const int timescale);
} CckTimestampOps;

extern const CckTimestampOps tsOps; /* helper object -> cck_timestamp.c */

/* timescale management functions */
int		getTimescaleOffset(const int a, const int b); /* get offset of timescale a (TS_xx) vs. b */
void		setTimescaleOffset(const int a, const int b, const int offset); /* set offset of timescale a (TS_xx) vs. b */
const char*	getTimescaleName(const int type);
int		getTimescaleType(const char* name);
void		dumpTimescaleOffsets();

/* print formatted timestamp into buffer */
int		snprint_CckTimestamp(char *s, int max_len, const CckTimestamp * p);

#endif /* CCK_TIMESTAMP_H_ */
