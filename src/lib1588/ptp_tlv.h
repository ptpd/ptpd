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

#ifndef PTP_TLV_H_
#define PTP_TLV_H_

#include "ptp_message.h"
#include "ptp_tlv_signaling.h"
#include "ptp_tlv_management.h"
#include "ptp_tlv_other.h"

/* Table 34: tlvType values */
enum {
	/* Standard TLVs */
	PTP_TLVTYPE_MANAGEMENT					= 0x0001,
	PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS			= 0x0002,
	PTP_TLVTYPE_ORGANIZATION_EXTENSION			= 0x0003,
	/* Optional unicast message negotiation TLVs */
	PTP_TLVTYPE_REQUEST_UNICAST_TRANSMISSION		= 0x0004,
	PTP_TLVTYPE_GRANT_UNICAST_TRANSMISSION			= 0x0005,
	PTP_TLVTYPE_CANCEL_UNICAST_TRANSMISSION			= 0x0006,
	PTP_TLVTYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION	= 0x0007,
	/* Optional path trace mechanism TLV */
	PTP_TLVTYPE_PATH_TRACE					= 0x0008,
	/* Optional alternate timescale TLV */
	PTP_TLVTYPE_ALTERNATE_TIME_OFFSET_INDICATOR		= 0x0009,
	/*Security TLVs */
	PTP_TLVTYPE_AUTHENTICATION				= 0x2000,
	PTP_TLVTYPE_AUTHENTICATION_CHALLENGE			= 0x2001,
	PTP_TLVTYPE_SECURITY_ASSOCIATION_UPDATE			= 0x2002,
	/* Cumulative frequency scale factor offset */
	PTP_TLVTYPE_CUM_FREQ_SCALE_FACTOR_OFFSET		= 0x2003,
	/* PTPMon extension */
	PTP_TLVTYPE_PTPMON_REQUEST				= 0x21FE,
	PTP_TLVTYPE_PTPMON_RESPONSE				= 0x21FF,
	PTP_TLVTYPE_PTPMON_MTIE_REQUEST				= 0x2200,
	PTP_TLVTYPE_PTPMON_MTIE_RESPONSE			= 0x2201
};

/* known TLV lengths */
#define PTP_TLVLEN_REQUEST_UNICAST_TRANSMISSION			6
#define PTP_TLVLEN_GRANT_UNICAST_TRANSMISSION			8
#define PTP_TLVLEN_CANCEL_UNICAST_TRANSMISSION			2
#define PTP_TLVLEN_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION	2

/* Table 38: Management TLV action types */
enum {
	PTP_MTLVACTION_GET		= 0x00,
	PTP_MTLVACTION_SET		= 0x01,
	PTP_MTLVACTION_RESPONSE		= 0x02,
	PTP_MTLVACTION_COMMAND		= 0x03,
	PTP_MTLVACTION_ACKNOWLEDGE	= 0x04
};

/* TLV datatypes */

/* Bodybag, bodybag, what's in that bodybag... [1] */
typedef union {
    PtpTlvRequestUnicastTransmission		requestUnicastTransmission;
    PtpTlvGrantUnicastTransmission		grantUnicastTransmission;
    PtpTlvCancelUnicastTransmission		cancelUnicastTransmission;
    PtpTlvAcknowledgeCancelUnicastTransmission	acknowledgeCancelUnicastTransmission;

    PtpTlvPtpMonRequest				ptpMonRequest;
    PtpTlvPtpMonResponse			ptpMonResponse;
    PtpTlvPtpMonMtieRequest			ptpMonMtieRequest;
    PtpTlvPtpMonMtieResponse			ptpMonMtieResponse;

    PtpTlvManagement				management;
    PtpTlvManagementErrorStatus			managementErrorStatus;

    PtpTlvOrganizationExtension			organizationExtension;
    PtpTlvAlternateTimeOffsetIndicator		alternateTimeOffsetIndicator;
    PtpTlvAuthentication			authentication;
    PtpTlvAuthenticationChallenge		authenticationChallenge;
    PtpTlvCumFreqScaleFactorOffset		cumFreqScaleFactorOffset;
    PtpTlvPathTrace				pathTrace;
    PtpTlvSecurityAssociationUpdate		securityAssociationUpdate;
} PtpTlvBody;

/* The TLV itself */

typedef struct PtpTlv PtpTlv;

struct PtpTlv {
	/* standard TLVs */
	PtpEnumeration16 tlvType;
	PtpUInteger16 lengthField;
	PtpDynamicOctetBuf valueField;

	/* parent (message) */
	struct PtpMessage *parent;

	/* [1] ...another G.I. Joe */
	PtpTlvBody body;
	/* Truncated TLV */
	PtpBoolean empty;

	/* linked list */
	PtpTlv *next;
	PtpTlv *prev;
};

PtpTlv* createPtpTlv();

#define PTP_TYPE_FUNCDEFS( type ) \
int unpack##type (PtpTlv *tlv, char *buf, char *boundary); \
int pack##type (char *buf, PtpTlv *tlv, char *boundary); \
void display##type (PtpTlv *tlv); \
void free##type (PtpTlv *var);

PTP_TYPE_FUNCDEFS(PtpTlv)
PTP_TYPE_FUNCDEFS(PtpTlvData)
PTP_TYPE_FUNCDEFS(PtpSignalingTlvData)
PTP_TYPE_FUNCDEFS(PtpManagementTlvData)
PTP_TYPE_FUNCDEFS(PtpOtherTlvData)

#undef PTP_TYPE_FUNCDEFS


#endif /* PTP_TLV_H */
