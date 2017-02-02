/* Copyright (c) 2016-2017 Wojciech Owczarek,
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
 * @file   cck_utils.c
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  General helper functions used by libCCK
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include <libcck/cck.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>

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
static void tsRtt (CckTimestamp *, CckTimestamp *, CckTimestamp *, CckTimestamp *);
static void tsNorm (CckTimestamp *);

static const CckTimestampOps tOps = {
	tsAdd,
	tsSub,
	tsToDouble,
	tsFromDouble,
	tsDiv2,
	tsClear,
	tsNegative,
	tsAbs,
	tsIsNegative,
	tsIsZero,
	tsCmp,
	tsRtt
};

const CckTimestampOps*
tsOps(void) {
    return &tOps;
}


uint32_t
getFnvHash(const void *input, const size_t len, const int modulo)
{

    static const uint32_t prime = 16777619;
    static const uint32_t basis = 2166136261;

    int i = 0;

    uint32_t hash = basis;
    uint8_t *buf = (uint8_t*)input;

    for(i = 0; i < len; i++)  {
        hash *= prime;
        hash ^= *(buf + i);
    }

    return (modulo > 0 ? hash % modulo : hash);

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

static void tsRtt(CckTimestamp *rtt, CckTimestamp *before, CckTimestamp *meas, CckTimestamp *after)
{

	CckTimestamp tmpDelta;

	tsDiv2(before);
	tsDiv2(after);
	tsAdd(&tmpDelta, before, after);
	tsSub(rtt, &tmpDelta, meas);

}

int
hexDigitToInt(const char digit)
{

    int c = tolower(digit);

    if((c >= 'a') && (c <= 'f')) {
	return (c - 'a' + 10);
    }

    if((c >= '0') && (c <= '9')) {
	return c - '0';
    }

    return -1;

}

int
hexStringToBuffer(char *buf, const size_t len, const char *string) {

    int sLen = strlen(string);

    int written = 0;
    int read = 0;

    int hex;
    uint8_t acc = 0;

    uint8_t *marker = (uint8_t*) buf;

    while (written < len && read < sLen) {

	hex = hexDigitToInt(string[read]);

	if(hex < 0) {
	    CCK_DBG("hexStringToBuffer(): unknown character: '%c', pos %d\n",
		    string[read], read);
	    return -1;
	}

	if(read % 2) {
	    acc += hex;
	    *marker = acc;
	    acc = 0;
	    written++;
	    marker++;
	} else {
	    if(read < (sLen - 1)) {
		acc = hex << 4;
	    } else {
		/* last odd digit (will be 0x) */
		*marker = hex;
		written++;
	    }
	}

	read++;

    }

    return written;

}

/*
 * dump a data buffer as hex + ascii
 * ... with a DYNAMIC ASCII ART FRAME!
 */
void
dumpBuffer(void *buf, const int len, int perline, const char *title)
{

    /* ascii byte displayed as one character */
    static const int abytewidth = 1;
    /* hex byte displayed as three characters (2 plus space) */
    static const int hbytewidth = 3;
    /* total padding of frame elements */
    static const int framepad = 8;

    int titlelen = 0;

    if(title) {
	titlelen = strlen(title);
    }

    /* bytes dumped per line */
    if(!perline) {
	perline = 8;
    }

    /* sticking to 2-byte offset */
    if((len / perline) > 0xffff) {
	perline = len / 0xffff;
    }

    /* hex line width */
    int hlinewidth = hbytewidth * perline;
    /* ascii line width */
    int alinewidth = abytewidth * perline;

    /* sweeps across the buffer */
    uint8_t *marker = buf;

    int titlefillwidth = 1;
    int linefillwidth = 1;

    /* establish if title is wider or narrower than table row */
    if(title && titlelen) {
	if(titlelen > (alinewidth + hlinewidth + framepad)) {
	    linefillwidth = titlelen - (alinewidth + hlinewidth + framepad);
	} else {
	    titlefillwidth += (alinewidth + hlinewidth + framepad) - titlelen + 1;
	}
    }

    /* table row filler */
    char filler[linefillwidth + 1];
    filler[linefillwidth] = '\0';

    /* hex and ascii data */
    char hexline[hlinewidth + 1];
    char asciiline[alinewidth + 1];
    hexline[hlinewidth] = '\0';
    asciiline[alinewidth] = '\0';

    /* position in hex and ascii data */
    char *hexpos = hexline;
    char *asciipos = asciiline;

    /* line position modulo counter */
    int counter = 0;

    /* number of lines produced */
    int lineno = 0;

    /* total bytes read */
    int total = 0;

    unsigned char ch;

    if(!buf) {
	return;
    }

    /* pretty */
    memset(hexline, '-', hlinewidth);
    memset(asciiline, '-', alinewidth);

    /* title header */
    if(title && titlelen) {
	memset(filler, '-', linefillwidth);
	CCK_INFO("+--------%s--%s%s+\n",  hexline, asciiline, filler);

	/* center the title in a line fitting a table row */
	int titlewidth = titlelen + titlefillwidth;
	char titleline[titlewidth + 1];
	titleline[titlewidth] = '\0';
	memset(titleline, ' ', titlewidth);
	strncpy(titleline + (titlefillwidth / 2) , title, titlelen);
	CCK_INFO("| %s|\n", titleline);
    }

    memset(filler, '-', linefillwidth);
       CCK_INFO("+--------%s+-%s%s+\n",  hexline, asciiline, filler);

    /* inside table rows, the filler is spaces */
    memset(filler, ' ', linefillwidth);

    while (total < len) {

	/* character code */
	ch = *marker;

	/* exclude special characters */
	if(ch < 32 || ch > 126) {
	    ch = '.';
	}

	/* write hex and ascii data, + 1 because of traling \0 ! */
	snprintf(hexpos, hbytewidth + 1, "%02x ", *marker);
	snprintf(asciipos, abytewidth + 1, "%c", ch);

	/* move across hex and ascii data */
	hexpos += hbytewidth;
	asciipos += abytewidth;
	/* move across the buffer */
	marker++;

	/* update counters */
        counter++;
	total++;

	counter %= perline;

	/* end of line - print line */
	if(!counter) {
	    CCK_INFO("| 0x%04x %s| %s%s|\n", lineno * perline,
			 hexline, asciiline, filler);
	    /* reset lines and positions */
	    hexpos = hexline;
	    asciipos = asciiline;
	    lineno++;
	}

    }

    /* leftover data after loop ended - fill up with spaces and print last line */
    if(counter) {
	memset(hexpos,   ' ', hbytewidth * (perline - counter) );
	memset(asciipos, ' ', abytewidth * (perline - counter) );
	CCK_INFO("| 0x%04x %s| %s%s|\n", lineno * perline,
		hexline, asciiline, filler);
    }

    /* pretty */
    memset(hexline, '-', hlinewidth);
    memset(asciiline, '-', alinewidth);
    memset(filler, '-', linefillwidth);
    CCK_INFO("+--------%s+-%s%s+\n",  hexline, asciiline, filler);

}

bool
doubleToFile(const char *filename, double input)
{

	int ret;

	FILE *fp = fopen(filename,"w");

	if(fp == NULL) {
	    CCK_DBG("doubleToFile: could not open %s for writing: %s\n", filename, strerror(errno));
	    return false;
	}

	ret = fprintf(fp, "%f\n", input);
	fclose(fp);

	return(ret > 0);

}

bool
doubleFromFile(const char *filename, double *output)
{
	int ret;
	FILE *fp = fopen(filename,"r");

	if(fp == NULL) {
	    CCK_DBG("doubleFromFile: could not open %s: %s\n", filename, strerror(errno));
	    return false;
	}

	ret = fscanf(fp, "%lf", output);
	fclose(fp);

	return(ret == 1);
}

bool
tokenInList(const char *list, const char * search, const char * delim)
{

    bool ret = false;

    if((list == NULL) || (search == NULL) || (delim == NULL) )  {
	return false;
    }

    if(strlen(search) < 1) {
	return false;
    }

    if(strlen(list) < 1) {
	return false;
    }

    if(strlen(delim) < 1) {
	return false;
    }

    foreach_token_begin(tmp, list, token, delim);

	if(!strcmp(token, search)) {
	    ret = true;
	}

    foreach_token_end(tmp);

    return ret;

}

int
countTokens(const char* list, const char* delim, const bool nonEmpty)
{

    int count = 0;

    foreach_token_begin(tmp, list, token, delim);

    if(nonEmpty) {
	if(strlen(token)) {
	    count++;
	}
    } else {
	count++;
    }

    foreach_token_end(tmp);

    return count;

}

int snprint_CckTimestamp(char *s, int max_len, const CckTimestamp * p)
{
	int len = 0;

	len += snprintf(&s[len], max_len - len, "%c",
		tsIsNegative(p)? '-':' ');

	len += snprintf(&s[len], max_len - len, "%d.%09d",
	    abs(p->seconds), abs(p->nanoseconds));

	return len;
}

int maintainStrBuf(int len, char **marker, int *left)
{

    if(len <= 0) {
	return 0;
    }

    *left -= len;

    if(left <= 0) {
	return 0;
    }

    *marker += len;

    return 1;

}

int
offsetBuffer(char *dst, const int capacity, char *src, const size_t len, const int offset) {

    int bytes = min(len - offset, capacity);

    memcpy(dst, src + offset, bytes);

    return bytes;
}

int
shiftBuffer(char *buf, const int capacity, const size_t len, const int offset) {

    int bytes = len - offset;

    char tmpBuf[capacity - offset];

    memcpy(tmpBuf, buf + offset, bytes);
    memset(buf, 0, capacity);
    memcpy(buf, tmpBuf, bytes);

    return bytes;

}

/* set @bits rightmost bits in buffer @buf of size @size to value @value */
void
setBufferBits(void *buf, const size_t size, int bits, const bool value) {

    void *start = buf;

    int leftmost = bits % 8;
    int bytes = bits / 8;

    if(bytes <= size) {
	buf += size - bytes;
	memset(buf, value ? 0xff : 0, bytes);
    }

    if(buf > start) {
	buf--;
	unsigned char *last = buf;
	if(value) {
	    *last |= (0xff >> (8 - leftmost));
	} else {
	    *last &= (0xff >> leftmost) << leftmost;
	}
    }

}

/* "or" operation of two buffers */
void
orBuffer(void *vout, const void *va, const void *vb, const size_t len) {

    unsigned char *out = vout;
    const unsigned char *a = va;
    const unsigned char *b = vb;

    for(int i = 0; i < len; i++) {
	out[i] = a[i] || b[i];
    }

}

/* "and" operation of two buffers */
void
andBuffer(void *vout, const void *va, const void *vb, const size_t len) {

    unsigned char *out = vout;
    const unsigned char *a = va;
    const unsigned char *b = vb;

    for(int i = 0; i < len; i++) {
	out[i] = a[i] & b[i];
    }

}

/* "not" operation on a buffer */
void inverseBuffer(void *vout, const void *va, const size_t len) {

    unsigned char *out = vout;
    const unsigned char *a = va;

    for(int i = 0; i < len; i++) {
	out[i] = ~a[i];
    }

}

void cckVersion() {

    printf("\nLibCCK information:\n\n");
    printf("\tLibCCK version "CCK_API_VER_STR"\n");
    printf(CCK_API_INFO_STR"\n");

}