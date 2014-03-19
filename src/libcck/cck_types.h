/*-
 * libCCK - Clock Construction Kit
 *
 * Copyright (c) 2014 Wojciech Owczarek,
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
 * @file   cck_types.h
 * 
 * @brief  libCCK type definitions
 *
 */

#ifndef CCK_TYPES_H_
#define CCK_TYPES_H_

#ifndef CCK_H_INSIDE_
#error libCCK component headers should not be uncluded directly - please include cck.h only.
#endif


#include <stdint.h>
#include <stdio.h>

typedef struct {

    FILE* fp;

} CckLogHandler;

/* LibCCK bool type */
typedef enum {CCK_FALSE=0, CCK_TRUE=1} CckBool;


/* Generic integer types */

typedef uint8_t 		CckUShort;
typedef unsigned char 		CckUChar;
typedef unsigned char 		CckOctet;

typedef uint8_t 		CckUInt8;
typedef int8_t 			CckInt8;
typedef uint16_t 		CckUInt16;
typedef int16_t 		CckInt16;
typedef uint32_t 		CckUInt32;
typedef int32_t 		CckInt32;

/* different timestamp container types */
typedef enum {

	/* software timestamping methods */
	CCK_TIMESPEC = 1,
	CCK_TIMEVAL,
	CCK_BINTIME,
	/* hardware timestamping methods */
	CCK_SCMTIMESTAMPING

} CckTimestampType;


/* Timestamp type */
typedef struct {

	CckUInt32 seconds;
	CckUInt32 nanoseconds;

} CckTimestamp;

#endif /* CCK_TYPES_H_ */
