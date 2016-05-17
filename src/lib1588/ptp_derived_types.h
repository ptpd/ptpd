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

#ifndef PTP_DERIVED_TYPES_H_
#define PTP_DERIVED_TYPES_H_

#include "ptp_primitives.h"

#include <stdio.h>

/* known derived type lenths */
#define PTP_TYPELEN_TIMEINTERVAL		8
#define PTP_TYPELEN_TIMESTAMP			10
#define PTP_TYPELEN_CLOCKIDENTITY		8
#define PTP_TYPELEN_PORTIDENTITY		10
#define PTP_TYPELEN_CLOCKQUALITY		4

typedef struct {
    PtpInteger32 seconds;
    PtpInteger32 nanoseconds;
} PtpTimeInternal;

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/derivedData/timeInterval.def"
	PtpTimeInternal internalTime;
} PtpTimeInterval;

typedef struct  {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/derivedData/timestamp.def"
	PtpTimeInternal internalTime;
} PtpTimestamp;

typedef PtpOctet PtpClockIdentity[PTP_TYPELEN_CLOCKIDENTITY];

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/derivedData/portIdentity.def"
} PtpPortIdentity;

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/derivedData/portAddress.def"
} PtpPortAddress;

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/derivedData/clockQuality.def"
} PtpClockQuality;

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/derivedData/timePropertiesDS.def"
} PtpTimePropertiesDS;

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/derivedData/ptpText.def"
} PtpText;

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/derivedData/faultRecord.def"
} PtpFaultRecord;

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/derivedData/physicalAddress.def"
} PtpPhysicalAddress;

#define PTP_TYPE_FUNCDEFS( type ) \
void pack##type (char *buf, type *data, size_t len); \
void unpack##type (type* data, char *buf, size_t len); \
void display##type (type data, const char *name, size_t len); \
void free##type (type *data);

PTP_TYPE_FUNCDEFS(PtpTimeInterval)
PTP_TYPE_FUNCDEFS(PtpTimestamp)
PTP_TYPE_FUNCDEFS(PtpClockIdentity)
PTP_TYPE_FUNCDEFS(PtpPortIdentity)
PTP_TYPE_FUNCDEFS(PtpPortAddress)
PTP_TYPE_FUNCDEFS(PtpClockQuality)
PTP_TYPE_FUNCDEFS(PtpTimePropertiesDS)
PTP_TYPE_FUNCDEFS(PtpText)
PTP_TYPE_FUNCDEFS(PtpFaultRecord)
PTP_TYPE_FUNCDEFS(PtpPhysicalAddress)

#undef PTP_TYPE_FUNCDEFS

/* create PtpText from string with a given length */
PtpText createPtpTextLen(const char *text, size_t len);
/* create PtpText from string and determine length */
PtpText createPtpText(const char *text);

void displayPtpTimeInternal(PtpTimeInternal data, const char *name);



#endif /* PTP_DERIVED_TYPES_H_ */
