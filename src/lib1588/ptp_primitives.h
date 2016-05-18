/*-
 * Copyright (c) 2016 Wojciech Owczarek,
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

#ifndef PTP_xPRIMITIVES_H_
#define PTP_xPRIMITIVES_H_

#include <stdint.h>	/* integer types */
#include <stdio.h>	/* size_t */

typedef enum {FALSEx=0, TRUEx=1} PtpBoolean;
typedef char PtpOctet;
typedef int8_t PtpInteger8;
typedef int16_t PtpInteger16;
typedef int32_t PtpInteger32;
typedef uint8_t  PtpUInteger8;
typedef uint16_t PtpUInteger16;
typedef uint32_t PtpUInteger32;
typedef uint16_t PtpEnumeration16;
typedef unsigned char PtpEnumeration8;
typedef unsigned char PtpEnumeration4;
typedef unsigned char PtpEnumeration4Upper;
typedef unsigned char PtpEnumeration4Lower;
typedef unsigned char PtpUInteger4;
typedef unsigned char PtpUInteger4Upper;
typedef unsigned char PtpUInteger4Lower;
typedef unsigned char PtpNibble;
typedef unsigned char PtpNibbleUpper;
typedef unsigned char PtpNibbleLower;

/* helper types for easier data packing/unpacking */
typedef char* PtpOctetBuf; 		/* pre-defined array field, copy n bytes */
typedef char* PtpDynamicOctetBuf;	/* dynamically allocate n bytes, then copy */
typedef char* PtpSharedOctetBuf;	/* pre-allocated buffer */

typedef struct {
	uint32_t low;
	uint16_t high;
} PtpUInteger48;


typedef struct {
	uint32_t low;
	int32_t high;
} PtpInteger64;

#define PTP_TYPE_FUNCDEFS( type ) \
void pack##type (char *to, type *from, size_t len); \
void unpack##type (type *to, char *from, size_t len); \
void display##type (type var, const char *name, size_t len); \
void free##type (type *var);

PTP_TYPE_FUNCDEFS(PtpBoolean)
PTP_TYPE_FUNCDEFS(PtpOctet)
PTP_TYPE_FUNCDEFS(PtpOctetBuf)
PTP_TYPE_FUNCDEFS(PtpDynamicOctetBuf)
PTP_TYPE_FUNCDEFS(PtpInteger8)
PTP_TYPE_FUNCDEFS(PtpInteger16)
PTP_TYPE_FUNCDEFS(PtpInteger32)
PTP_TYPE_FUNCDEFS(PtpUInteger8)
PTP_TYPE_FUNCDEFS(PtpUInteger16)
PTP_TYPE_FUNCDEFS(PtpUInteger32)
PTP_TYPE_FUNCDEFS(PtpEnumeration16)
PTP_TYPE_FUNCDEFS(PtpEnumeration8)
PTP_TYPE_FUNCDEFS(PtpEnumeration4Upper)
PTP_TYPE_FUNCDEFS(PtpEnumeration4Lower)
PTP_TYPE_FUNCDEFS(PtpUInteger4Upper)
PTP_TYPE_FUNCDEFS(PtpUInteger4Lower)
PTP_TYPE_FUNCDEFS(PtpNibbleUpper)
PTP_TYPE_FUNCDEFS(PtpNibbleLower)
PTP_TYPE_FUNCDEFS(PtpUInteger48)
PTP_TYPE_FUNCDEFS(PtpInteger64)

#undef PTP_TYPE_FUNCDEFS

#endif /*PTP_xPRIMITIVES_H_*/
