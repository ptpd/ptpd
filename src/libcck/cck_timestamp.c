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
 * @file   cck_timestamp.c
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  CCK Timestamp and associated conversion and timescale management functions
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include <libcck/cck_timestamp.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>

#define THIS_COMPONENT "timestamp: "

/* timescale offset table, updateable using setTimescaleOffset() */
static int _timescaleOffsets[TS_MAX][TS_MAX] = {

			/* TS_ARB,	TS_UTC,		TS_TAI,		TS_GPS */
/* TS_ARB */	{	0,		0,		0,		0		},
/* TS_UTC */	{	0,		0,		TO_UTC_TAI,	TO_UTC_GPS	},
/* TS_TAI */	{	0,		-TO_UTC_TAI,	0,		-TO_GPS_TAI	},
/* TS_GPS */	{	0,		-TO_UTC_GPS,	TO_GPS_TAI,	0		},

};

static const char* timescaleNames[TS_MAX] = {

	[TS_ARB] =	"ARB",
	[TS_UTC] =	"UTC",
	[TS_TAI] =	"TAI",
	[TS_GPS] =	"GPS",

};

static void tsAdd (CckTimestamp *, const CckTimestamp *, const CckTimestamp *);
static void tsSub (CckTimestamp *, const CckTimestamp *, const CckTimestamp *);
static double tsToDouble (const CckTimestamp *);
static CckTimestamp tsFromDouble (const double);
static void tsDiv2 (CckTimestamp *);
static void tsClear (CckTimestamp *);
static CckTimestamp tsNegative (const CckTimestamp *);
static void tsAbs (CckTimestamp *);
static bool tsIsNegative (const CckTimestamp *);
static bool tsIsZero (const CckTimestamp *);
static int tsCmp (const void *, const void *);
static void tsMean2 (CckTimestamp *mean, CckTimestamp *t1, CckTimestamp *t2);
static void tsRttCor (CckTimestamp *, CckTimestamp *, CckTimestamp *, CckTimestamp *);
static void tsConvert(CckTimestamp*, const int);
static CckTimestamp tsGetConversion(CckTimestamp*, const int);
static void tsNorm (CckTimestamp *);

const CckTimestampOps tsOps = {
	.add =		tsAdd,
	.sub =		tsSub,
	.toDouble =	tsToDouble,
	.fromDouble =	tsFromDouble,
	.div2 =		tsDiv2,
	.clear =	tsClear,
	.negative =	tsNegative,
	.abs =		tsAbs,
	.isNegative =	tsIsNegative,
	.isZero =	tsIsZero,
	.cmp =		tsCmp,
	.mean2 =	tsMean2,
	.rttCor =	tsRttCor,
	.convert =	tsConvert,
	.getConversion = tsGetConversion
};

int snprint_CckTimestamp(char *s, int max_len, const CckTimestamp * p)
{
	int len = 0;

	len += snprintf(&s[len], max_len - len, "%c",
		tsIsNegative(p)? '-':' ');

	len += snprintf(&s[len], max_len - len, "%d.%09d",
	    abs(p->seconds), abs(p->nanoseconds));

	return len;
}

int
getTimescaleOffset(const int a, const int b)
{

    if(a < TS_MIN || a >= TS_MAX) {
	return 0;
    }

    if(b < TS_MIN || b >= TS_MAX) {
	return 0;
    }

    return _timescaleOffsets[a][b];

}


void
setTimescaleOffset(const int a, const int b, const int offset)
{

    if(a < TS_MIN || a >= TS_MAX) {
	return;
    }

    if(b < TS_MIN || b >= TS_MAX) {
	return;
    }

    /* leave me alone */
    if(a == TS_ARB || b == TS_ARB) {
	return;
    }

    _timescaleOffsets[a][b] = offset;
    _timescaleOffsets[b][a] = -offset;

    /* process chained updates to related timescales */

    /* as if this will ever happen */
    if(a == TS_GPS && b == TS_TAI) {
	_timescaleOffsets[TS_UTC][TS_GPS] = _timescaleOffsets[TS_UTC][TS_TAI] - offset;
	_timescaleOffsets[TS_GPS][TS_UTC] = -_timescaleOffsets[TS_UTC][TS_GPS];
    }
    if(a == TS_TAI && b == TS_GPS) {
	_timescaleOffsets[TS_UTC][TS_GPS] = - (_timescaleOffsets[TS_UTC][TS_TAI] - offset);
	_timescaleOffsets[TS_GPS][TS_UTC] = -_timescaleOffsets[TS_UTC][TS_GPS];
    }

    /* this _does_ happen */
    if (a == TS_UTC && b == TS_TAI) {
	_timescaleOffsets[TS_UTC][TS_GPS] = offset - _timescaleOffsets[TS_GPS][TS_TAI];
	_timescaleOffsets[TS_GPS][TS_UTC] = -_timescaleOffsets[TS_UTC][TS_GPS];
    }
    if (a == TS_TAI && b == TS_UTC) {
	_timescaleOffsets[TS_UTC][TS_GPS] = - (offset - _timescaleOffsets[TS_GPS][TS_TAI]);
	_timescaleOffsets[TS_GPS][TS_UTC] = -_timescaleOffsets[TS_UTC][TS_GPS];
    }

}

const char*
getTimescaleName(const int type)
{

    if(type < TS_MIN || type >= TS_MAX) {
	CCK_DBG(THIS_COMPONENT"getTimescaleName(%d): unknown timescale type\n", type);
	return NULL;
    }

    return timescaleNames[type];

}

int
getTimescaleType(const char* name)
{

    if(name == NULL || !strlen(name)) {
	return -1;
    }

    for(int i = TS_MIN; i < TS_MAX; i++) {
	if(!strcmp(name, timescaleNames[i])) {
	    return i;
	}
    }

    CCK_DBG(THIS_COMPONENT"getTimescaleType('%s'): unknown timescale\n", name);

    return -1;

}

void
dumpTimescaleOffsets()
{

    CCK_INFO("\n");
    CCK_INFO("Timescale offset table:\n");
    CCK_INFO("\n");
    for(int a = TS_MIN; a < TS_MAX; a++) {

	for(int b = TS_MIN; b < TS_MAX; b++) {

	    if(a == TS_ARB || b == TS_ARB) {
		continue;
	    }

	    if(a == b) {
		continue;
	    }

	    const char* ca = getTimescaleName(a);
	    const char* cb = getTimescaleName(b);
	    int offset = _timescaleOffsets[a][b];

	    CCK_INFO("\t%s to %s offset: % 02d seconds (%s is %02d s %s %s)\n",
		ca, cb, _timescaleOffsets[a][b],
		ca, abs(offset), offset < 0 ? "behind" : offset > 0 ? "ahead of" : "in line with", cb);
		
	}

    }
    CCK_INFO("\n");

}

static double
tsToDouble(const CckTimestamp * p)
{

	double sign = (p->seconds < 0 || p->nanoseconds < 0 ) ? -1.0 : 1.0;
	return (sign * ( abs(p->seconds) + abs(p->nanoseconds) / 1E9 ));

}

static CckTimestamp
tsFromDouble(const double d)
{

	CckTimestamp t = {0, 0};

	t.seconds = trunc(d);
	t.nanoseconds = (d - (t.seconds + 0.0)) * 1E9;

	return t;

}

static void
tsNorm(CckTimestamp * r)
{
	r->seconds += r->nanoseconds / 1000000000;
	r->nanoseconds -= (r->nanoseconds / 1000000000) * 1000000000;

	if (r->seconds > 0 && r->nanoseconds < 0) {
		r->seconds -= 1;
		r->nanoseconds += 1000000000;
	} else if (r->seconds < 0 && r->nanoseconds > 0) {
		r->seconds += 1;
		r->nanoseconds -= 1000000000;
	}
}

static void
tsAdd(CckTimestamp * r, const CckTimestamp * x, const CckTimestamp * y)
{
	r->seconds = x->seconds + y->seconds;
	r->nanoseconds = x->nanoseconds + y->nanoseconds;

	tsNorm(r);
}

static void
tsSub(CckTimestamp * r, const CckTimestamp * x, const CckTimestamp * y)
{
	r->seconds = x->seconds - y->seconds;
	r->nanoseconds = x->nanoseconds - y->nanoseconds;

	tsNorm(r);
}

static void
tsDiv2(CckTimestamp *r)
{
	r->nanoseconds += (r->seconds % 2) * 1000000000;
	r->seconds /= 2;
	r->nanoseconds /= 2;

	tsNorm(r);
}


static void
tsClear(CckTimestamp *r)
{
	r->seconds     = 0;
	r->nanoseconds = 0;
}

static CckTimestamp
tsNegative(const CckTimestamp *time)
{
    CckTimestamp neg = {-time->seconds, -time->nanoseconds};
    return neg;
}

static void
tsAbs(CckTimestamp *r)
{
	/* Make sure signs are the same */
	tsNorm(r);
	r->seconds       = abs(r->seconds);
	r->nanoseconds   = abs(r->nanoseconds);
}

static bool
tsIsNegative(const CckTimestamp *p)
{
	return (p->seconds < 0) || (p->nanoseconds < 0);
}

static bool
tsIsZero(const CckTimestamp *time)
{
    return(!time->seconds && !time->nanoseconds);
}


static int tsCmp(const void *a, const void *b)
{

	const CckTimestamp *x = a;
	const CckTimestamp *y = b;

	CckTimestamp r;

	tsSub(&r, x, y);

	if(tsIsNegative(&r)) {
	    return -1;
	}

	if(tsIsZero(&r)) {
	    return 0;
	}

	return 1;
}

static void tsMean2(CckTimestamp *mean, CckTimestamp *t1, CckTimestamp *t2)
{

	CckTimestamp t1c = *t1;
	CckTimestamp t2c = *t2;

	tsDiv2(&t1c);
	tsDiv2(&t2c);
	tsClear(mean);
	tsAdd(mean, &t1c, &t2c);
}

static void tsRttCor(CckTimestamp *rtt, CckTimestamp *before, CckTimestamp *meas, CckTimestamp *after)
{

	CckTimestamp mean;
	CckTimestamp cmeas = *meas;

	tsMean2(&mean, before, after);
	tsSub(rtt, &mean, &cmeas);

}

static void
tsConvert(CckTimestamp* ts, const int timescale)
{

    int offset = getTimescaleOffset(timescale, ts->timescale);

    ts->seconds += (int32_t)offset;

    ts->timescale = timescale;

}

static CckTimestamp
tsGetConversion(CckTimestamp* ts, const int timescale)
{

    CckTimestamp ret = *ts;

    tsConvert(&ret, timescale);

    return ret;

}
