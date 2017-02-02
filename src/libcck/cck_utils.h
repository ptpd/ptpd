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
 * @file   cck_utils.h
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  Declarations for general utility functions used by libCCK, various helper macros
 *
 */

#ifndef CCK_UTILS_H_
#define CCK_UTILS_H_

#include <config.h>

#include <stdio.h>

#ifdef HAVE_MACHINE_ENDIAN_H
#	include <machine/endian.h>
#endif /* HAVE_MACHINE_ENDIAN_H */

#ifdef HAVE_ENDIAN_H
#	include <endian.h>
#endif /* HAVE_ENDIAN_H */

#include <libcck/cck_types.h>

/* min, max */
#ifndef min
#define min(a,b) ((a < b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a > b) ? (a) : (b))
#endif

/* clamp a value to boundary */
#define clamp(val,bound) ( (val > bound) ? bound : (val < -bound) ? -bound : val )

/* declare a string buffer of given length (+1) and initialise it */
#ifndef tmpstr
#define tmpstr(name, len) char name[len + 1];\
int name ## _len = len + 1;\
memset(name, 0, name ## _len);
#endif

/* clear a string buffer (assuming _len suffix declared using tmpstr() */
#ifndef clearstr
#define clearstr(name)\
memset(name, 0, name ## _len);
#endif

/* default token delimiter to be used with foreach_token_* */
#define DEFAULT_TOKEN_DELIM ", ;\t"

/* quick macros to convert bool to string */
#define boolstr(var) ( var ? "true" : "false" )
#define boolyes(var) ( var ? "yes" : "no" )

/* macros to check if a flag appeared or disappeared between one variable and another */
#define flag_came(a,b,flag) ((a & flag) && !(b & flag))
#define flag_went(a,b,flag) (!(a & flag) && (b & flag))

/* "safe" callback: run only if callback pointer not null */
#define SAFE_CALLBACK(fun, ...) { if(fun) { fun( __VA_ARGS__ ); } }

/* byte swap */
#define swap16(var) (((var & 0xff00) >> 8) | ((var & 0x00ff) << 8))
#define swap32(var) (((var & 0xff000000) >> 24) | ((var & 0x00ff0000) >> 8) | ((var & 0x0000ff00) << 8) | ((var & 0x000000ff) << 24))

/* explicit endian conversion */
#if BYTE_ORDER == BIG_ENDIAN || defined(_BIG_ENDIAN) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
    #define tobe16(var) (var)
    #define tobe32(var) (var)
    #define tole16(var) swap16(var)
    #define tole32(var) swap32(var)
#elif BYTE_ORDER == LITTLE_ENDIAN || defined(_LITTLE_ENDIAN) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN)
    #define tobe16(var) swap16(var)
    #define tobe32(var) swap32(var)
    #define tole16(var) (var)
    #define tole32(var) (var)
#endif

/*
 * foreach loop across substrings from var, delimited by delim, placing
 * each token in targetvar on iteration, using id variable name prefix
 * to allow nesting (each loop uses an individual set of variables)
 */
#define foreach_token_begin(id, var, targetvar, delim) \
    int counter_##id = -1; \
    char* stash_##id = NULL; \
    char* text_##id; \
    char* text__##id; \
    char* targetvar; \
    text_##id=strdup(var); \
    for(text__##id = text_##id;; text__##id=NULL) { \
	targetvar = strtok_r(text__##id, delim, &stash_##id); \
	if(targetvar==NULL) break; \
	counter_##id++;

#define foreach_token_end(id) } \
    if(text_##id != NULL) { \
	free(text_##id); \
    } \
    counter_##id++;

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
	void (*rtt) (CckTimestamp *, CckTimestamp *, CckTimestamp *, CckTimestamp *);
} CckTimestampOps;

/* grab the timestamp ops helper object */
const	CckTimestampOps *tsOps(void);
/* print formatted timestamp into buffer */
int	snprint_CckTimestamp(char *s, int max_len, const CckTimestamp * p);
/* get a FNV hash of @input buffer of size @len, modulo divide result by @modulo */
uint32_t getFnvHash(const void *input, const size_t len, const int modulo);
/* convert a hex digit (0-f) to int */
int	hexDigitToInt(const char digit);
/* load string of hex digits to @buf of size @len */
int	hexStringToBuffer(char *buf, const size_t len, const char *string);
/* dump binary data buffer to screen as table with header @title and max @perline bytes per line */
void	dumpBuffer(void *buf, const int len, int perline, const char *title);
/* write a double precision value to file */
bool	doubleToFile(const char *filename, double input);
/* load a double precision value from file */
bool	doubleFromFile(const char *filename, double *output);
/* count tokens in @list delimited by @delim, count only non-empty tokens if@nonEmpty is true */
int	countTokens(const char* list, const char* delim, const bool nonEmpty);
/* check if @search is present in @list of tokens delimited by @delim */
bool	tokenInList(const char *list, const char * search, const char * delim);
/*
 * ensure that there is enough space left in a char buffer to store len:
 * move marker and decrease @left if there is, otherwise return -1;
 */
int	maintainStrBuf(int len, char **marker, int *left);
/* copy data from @src into @dst, skipping @offset bytes, @return number of bytes copied */
int	offsetBuffer(char *dst, const int capacity, char *src, const size_t len, const int offset);
/* shift data in @buf by @offset bytes in place (using a temporary buffer), @return number of bytes copied */
int	shiftBuffer(char *buf, const int capacity, const size_t len, const int offset);
/* set @bits rightmost bits in buffer @buf of size @size */
void setBufferBits(void *buf, const size_t size, int bits, const bool value);
/* "or" operation of two buffers */
void orBuffer(void *vout, const void *va, const void *vb, const size_t len);
/* "and" operation of two buffers */
void andBuffer(void *vout, const void *va, const void *vb, const size_t len);
/* "not" operation on a buffer */
void inverseBuffer(void *vout, const void *va, const size_t len);

#endif /* CCK_UTILS_H_ */
