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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AS IS'' AND ANY EXPRESS OR
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

#ifndef PTP_TLV_SIGNALING_H_
#define PTP_TLV_SIGNALING_H_

#include "ptp_tlv.h"

/* begin generated code */

/* signaling TLV data types */

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/signalingTlv/acknowledgeCancelUnicastTransmission.def"
} PtpTlvAcknowledgeCancelUnicastTransmission;

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/signalingTlv/cancelUnicastTransmission.def"
} PtpTlvCancelUnicastTransmission;

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/signalingTlv/grantUnicastTransmission.def"
} PtpTlvGrantUnicastTransmission;

typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "definitions/signalingTlv/requestUnicastTransmission.def"
} PtpTlvRequestUnicastTransmission;

/* end generated code */

#endif /* PTP_TLV_SIGNALING_H */
