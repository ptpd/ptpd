/*-
 * Copyright (c) 2016      Wojciech Owczarek,
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

#include "ptp_primitives.h"
#include "ptp_tlv.h"
#include "ptp_message.h"

#include "tmp.h"

#include <stdlib.h>

/* begin generated code */

#define PTP_TYPE_FUNCDEFS( type ) \
static int unpack##type (type *data, char *buf, char *boundary); \
static int pack##type (char *buf, type *data, char *boundary); \
static void display##type (type *data); \
static void free##type (type *var);

PTP_TYPE_FUNCDEFS(PtpTlvAlternateTimeOffsetIndicator)
PTP_TYPE_FUNCDEFS(PtpTlvAuthenticationChallenge)
PTP_TYPE_FUNCDEFS(PtpTlvAuthentication)
PTP_TYPE_FUNCDEFS(PtpTlvCumFreqScaleFactorOffset)
PTP_TYPE_FUNCDEFS(PtpTlvOrganizationExtension)
PTP_TYPE_FUNCDEFS(PtpTlvPathTrace)
PTP_TYPE_FUNCDEFS(PtpTlvPtpMonMtieRequest)
PTP_TYPE_FUNCDEFS(PtpTlvPtpMonMtieResponse)
PTP_TYPE_FUNCDEFS(PtpTlvPtpMonRequest)
PTP_TYPE_FUNCDEFS(PtpTlvPtpMonResponse)
PTP_TYPE_FUNCDEFS(PtpTlvSecurityAssociationUpdate)

#undef PTP_TYPE_FUNCDEFS

static int unpackPtpTlvAlternateTimeOffsetIndicator(PtpTlvAlternateTimeOffsetIndicator *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/alternateTimeOffsetIndicator.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvAlternateTimeOffsetIndicator(char *buf, PtpTlvAlternateTimeOffsetIndicator *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/alternateTimeOffsetIndicator.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvAlternateTimeOffsetIndicator(PtpTlvAlternateTimeOffsetIndicator *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/alternateTimeOffsetIndicator.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvAlternateTimeOffsetIndicator(PtpTlvAlternateTimeOffsetIndicator *data) {

    PTPINFO("\tPtpTlvAlternateTimeOffsetIndicator: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/alternateTimeOffsetIndicator.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvAuthenticationChallenge(PtpTlvAuthenticationChallenge *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/authenticationChallenge.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvAuthenticationChallenge(char *buf, PtpTlvAuthenticationChallenge *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/authenticationChallenge.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvAuthenticationChallenge(PtpTlvAuthenticationChallenge *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/authenticationChallenge.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvAuthenticationChallenge(PtpTlvAuthenticationChallenge *data) {

    PTPINFO("\tPtpTlvAuthenticationChallenge: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/authenticationChallenge.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvAuthentication(PtpTlvAuthentication *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/authentication.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvAuthentication(char *buf, PtpTlvAuthentication *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/authentication.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvAuthentication(PtpTlvAuthentication *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/authentication.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvAuthentication(PtpTlvAuthentication *data) {

    PTPINFO("\tPtpTlvAuthentication: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/authentication.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvCumFreqScaleFactorOffset(PtpTlvCumFreqScaleFactorOffset *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/cumFreqScaleFactorOffset.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvCumFreqScaleFactorOffset(char *buf, PtpTlvCumFreqScaleFactorOffset *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/cumFreqScaleFactorOffset.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvCumFreqScaleFactorOffset(PtpTlvCumFreqScaleFactorOffset *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/cumFreqScaleFactorOffset.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvCumFreqScaleFactorOffset(PtpTlvCumFreqScaleFactorOffset *data) {

    PTPINFO("\tPtpTlvCumFreqScaleFactorOffset: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/cumFreqScaleFactorOffset.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvOrganizationExtension(PtpTlvOrganizationExtension *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/organizationExtension.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvOrganizationExtension(char *buf, PtpTlvOrganizationExtension *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/organizationExtension.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvOrganizationExtension(PtpTlvOrganizationExtension *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/organizationExtension.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvOrganizationExtension(PtpTlvOrganizationExtension *data) {

    PTPINFO("\tPtpTlvOrganizationExtension: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/organizationExtension.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvPathTrace(PtpTlvPathTrace *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/pathTrace.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvPathTrace(char *buf, PtpTlvPathTrace *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/pathTrace.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvPathTrace(PtpTlvPathTrace *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/pathTrace.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvPathTrace(PtpTlvPathTrace *data) {

    PTPINFO("\tPtpTlvPathTrace: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/pathTrace.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvPtpMonMtieRequest(PtpTlvPtpMonMtieRequest *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/ptpMonMtieRequest.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvPtpMonMtieRequest(char *buf, PtpTlvPtpMonMtieRequest *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/ptpMonMtieRequest.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvPtpMonMtieRequest(PtpTlvPtpMonMtieRequest *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/ptpMonMtieRequest.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvPtpMonMtieRequest(PtpTlvPtpMonMtieRequest *data) {

    PTPINFO("\tPtpTlvPtpMonMtieRequest: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/ptpMonMtieRequest.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvPtpMonMtieResponse(PtpTlvPtpMonMtieResponse *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/ptpMonMtieResponse.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvPtpMonMtieResponse(char *buf, PtpTlvPtpMonMtieResponse *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/ptpMonMtieResponse.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvPtpMonMtieResponse(PtpTlvPtpMonMtieResponse *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/ptpMonMtieResponse.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvPtpMonMtieResponse(PtpTlvPtpMonMtieResponse *data) {

    PTPINFO("\tPtpTlvPtpMonMtieResponse: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/ptpMonMtieResponse.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvPtpMonRequest(PtpTlvPtpMonRequest *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/ptpMonRequest.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvPtpMonRequest(char *buf, PtpTlvPtpMonRequest *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/ptpMonRequest.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvPtpMonRequest(PtpTlvPtpMonRequest *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/ptpMonRequest.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvPtpMonRequest(PtpTlvPtpMonRequest *data) {

    PTPINFO("\tPtpTlvPtpMonRequest: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/ptpMonRequest.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvPtpMonResponse(PtpTlvPtpMonResponse *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/ptpMonResponse.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvPtpMonResponse(char *buf, PtpTlvPtpMonResponse *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/ptpMonResponse.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvPtpMonResponse(PtpTlvPtpMonResponse *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/ptpMonResponse.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvPtpMonResponse(PtpTlvPtpMonResponse *data) {

    PTPINFO("\tPtpTlvPtpMonResponse: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/ptpMonResponse.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvSecurityAssociationUpdate(PtpTlvSecurityAssociationUpdate *data, char *buf, char *boundary) {

    int offset = 0;
    #include "def/field_unpack_bufcheck.h"
    #include "def/otherTlv/securityAssociationUpdate.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvSecurityAssociationUpdate(char *buf, PtpTlvSecurityAssociationUpdate *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/otherTlv/securityAssociationUpdate.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvSecurityAssociationUpdate(PtpTlvSecurityAssociationUpdate *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/otherTlv/securityAssociationUpdate.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvSecurityAssociationUpdate(PtpTlvSecurityAssociationUpdate *data) {

    PTPINFO("\tPtpTlvSecurityAssociationUpdate: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "def/otherTlv/securityAssociationUpdate.def"
    #undef PROCESS_FIELD
}

int unpackPtpOtherTlvData(PtpTlv *tlv, char* buf, char* boundary) {

    int ret = 0;
    int expectedLen = -1;

    switch(tlv->tlvType) {
	case PTP_TLVTYPE_ALTERNATE_TIME_OFFSET_INDICATOR:
	    #ifdef PTP_TLVLEN_ALTERNATE_TIME_OFFSET_INDICATOR
	    expectedLen = PTP_TLVLEN_ALTERNATE_TIME_OFFSET_INDICATOR;
	    #endif
	    ret = unpackPtpTlvAlternateTimeOffsetIndicator(&tlv->body.alternateTimeOffsetIndicator, buf, boundary);
	    break;
	case PTP_TLVTYPE_AUTHENTICATION_CHALLENGE:
	    #ifdef PTP_TLVLEN_AUTHENTICATION_CHALLENGE
	    expectedLen = PTP_TLVLEN_AUTHENTICATION_CHALLENGE;
	    #endif
	    ret = unpackPtpTlvAuthenticationChallenge(&tlv->body.authenticationChallenge, buf, boundary);
	    break;
	case PTP_TLVTYPE_AUTHENTICATION:
	    #ifdef PTP_TLVLEN_AUTHENTICATION
	    expectedLen = PTP_TLVLEN_AUTHENTICATION;
	    #endif
	    ret = unpackPtpTlvAuthentication(&tlv->body.authentication, buf, boundary);
	    break;
	case PTP_TLVTYPE_CUM_FREQ_SCALE_FACTOR_OFFSET:
	    #ifdef PTP_TLVLEN_CUM_FREQ_SCALE_FACTOR_OFFSET
	    expectedLen = PTP_TLVLEN_CUM_FREQ_SCALE_FACTOR_OFFSET;
	    #endif
	    ret = unpackPtpTlvCumFreqScaleFactorOffset(&tlv->body.cumFreqScaleFactorOffset, buf, boundary);
	    break;
	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	    #ifdef PTP_TLVLEN_ORGANIZATION_EXTENSION
	    expectedLen = PTP_TLVLEN_ORGANIZATION_EXTENSION;
	    #endif
	    ret = unpackPtpTlvOrganizationExtension(&tlv->body.organizationExtension, buf, boundary);
	    break;
	case PTP_TLVTYPE_PATH_TRACE:
	    #ifdef PTP_TLVLEN_PATH_TRACE
	    expectedLen = PTP_TLVLEN_PATH_TRACE;
	    #endif
	    ret = unpackPtpTlvPathTrace(&tlv->body.pathTrace, buf, boundary);
	    break;
	case PTP_TLVTYPE_PTPMON_MTIE_REQUEST:
	    #ifdef PTP_TLVLEN_PTPMON_MTIE_REQUEST
	    expectedLen = PTP_TLVLEN_PTPMON_MTIE_REQUEST;
	    #endif
	    ret = unpackPtpTlvPtpMonMtieRequest(&tlv->body.ptpMonMtieRequest, buf, boundary);
	    break;
	case PTP_TLVTYPE_PTPMON_MTIE_RESPONSE:
	    #ifdef PTP_TLVLEN_PTPMON_MTIE_RESPONSE
	    expectedLen = PTP_TLVLEN_PTPMON_MTIE_RESPONSE;
	    #endif
	    ret = unpackPtpTlvPtpMonMtieResponse(&tlv->body.ptpMonMtieResponse, buf, boundary);
	    break;
	case PTP_TLVTYPE_PTPMON_REQUEST:
	    #ifdef PTP_TLVLEN_PTPMON_REQUEST
	    expectedLen = PTP_TLVLEN_PTPMON_REQUEST;
	    #endif
	    ret = unpackPtpTlvPtpMonRequest(&tlv->body.ptpMonRequest, buf, boundary);
	    break;
	case PTP_TLVTYPE_PTPMON_RESPONSE:
	    #ifdef PTP_TLVLEN_PTPMON_RESPONSE
	    expectedLen = PTP_TLVLEN_PTPMON_RESPONSE;
	    #endif
	    ret = unpackPtpTlvPtpMonResponse(&tlv->body.ptpMonResponse, buf, boundary);
	    break;
	case PTP_TLVTYPE_SECURITY_ASSOCIATION_UPDATE:
	    #ifdef PTP_TLVLEN_SECURITY_ASSOCIATION_UPDATE
	    expectedLen = PTP_TLVLEN_SECURITY_ASSOCIATION_UPDATE;
	    #endif
	    ret = unpackPtpTlvSecurityAssociationUpdate(&tlv->body.securityAssociationUpdate, buf, boundary);
	    break;
	default:
	    PTPDEBUG("ptp_tlv_other.c:unpackPtpOtherTlvData(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    ret = PTP_MESSAGE_UNSUPPORTED_TLV;
	    break;
    }

    if (ret < 0 ) {
	return ret;
    }

    if(expectedLen >= 0) {
	if(tlv->lengthField < expectedLen) {
	    return PTP_MESSAGE_CORRUPT_TLV;
	}

	if (ret < expectedLen) {
	    return PTP_MESSAGE_BUFFER_TOO_SMALL;
	}
    }


    return ret;

}

int packPtpOtherTlvData(char *buf, PtpTlv *tlv, char *boundary) {

    int ret = 0;
    int expectedLen = -1;

    switch(tlv->tlvType) {
	case PTP_TLVTYPE_ALTERNATE_TIME_OFFSET_INDICATOR:
	    #ifdef PTP_TLVLEN_ALTERNATE_TIME_OFFSET_INDICATOR
	    expectedLen = PTP_TLVLEN_ALTERNATE_TIME_OFFSET_INDICATOR;
	    #endif
	    ret = packPtpTlvAlternateTimeOffsetIndicator(buf, &tlv->body.alternateTimeOffsetIndicator, boundary);
	    break;
	case PTP_TLVTYPE_AUTHENTICATION_CHALLENGE:
	    #ifdef PTP_TLVLEN_AUTHENTICATION_CHALLENGE
	    expectedLen = PTP_TLVLEN_AUTHENTICATION_CHALLENGE;
	    #endif
	    ret = packPtpTlvAuthenticationChallenge(buf, &tlv->body.authenticationChallenge, boundary);
	    break;
	case PTP_TLVTYPE_AUTHENTICATION:
	    #ifdef PTP_TLVLEN_AUTHENTICATION
	    expectedLen = PTP_TLVLEN_AUTHENTICATION;
	    #endif
	    ret = packPtpTlvAuthentication(buf, &tlv->body.authentication, boundary);
	    break;
	case PTP_TLVTYPE_CUM_FREQ_SCALE_FACTOR_OFFSET:
	    #ifdef PTP_TLVLEN_CUM_FREQ_SCALE_FACTOR_OFFSET
	    expectedLen = PTP_TLVLEN_CUM_FREQ_SCALE_FACTOR_OFFSET;
	    #endif
	    ret = packPtpTlvCumFreqScaleFactorOffset(buf, &tlv->body.cumFreqScaleFactorOffset, boundary);
	    break;
	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	    #ifdef PTP_TLVLEN_ORGANIZATION_EXTENSION
	    expectedLen = PTP_TLVLEN_ORGANIZATION_EXTENSION;
	    #endif
	    ret = packPtpTlvOrganizationExtension(buf, &tlv->body.organizationExtension, boundary);
	    break;
	case PTP_TLVTYPE_PATH_TRACE:
	    #ifdef PTP_TLVLEN_PATH_TRACE
	    expectedLen = PTP_TLVLEN_PATH_TRACE;
	    #endif
	    ret = packPtpTlvPathTrace(buf, &tlv->body.pathTrace, boundary);
	    break;
	case PTP_TLVTYPE_PTPMON_MTIE_REQUEST:
	    #ifdef PTP_TLVLEN_PTPMON_MTIE_REQUEST
	    expectedLen = PTP_TLVLEN_PTPMON_MTIE_REQUEST;
	    #endif
	    ret = packPtpTlvPtpMonMtieRequest(buf, &tlv->body.ptpMonMtieRequest, boundary);
	    break;
	case PTP_TLVTYPE_PTPMON_MTIE_RESPONSE:
	    #ifdef PTP_TLVLEN_PTPMON_MTIE_RESPONSE
	    expectedLen = PTP_TLVLEN_PTPMON_MTIE_RESPONSE;
	    #endif
	    ret = packPtpTlvPtpMonMtieResponse(buf, &tlv->body.ptpMonMtieResponse, boundary);
	    break;
	case PTP_TLVTYPE_PTPMON_REQUEST:
	    #ifdef PTP_TLVLEN_PTPMON_REQUEST
	    expectedLen = PTP_TLVLEN_PTPMON_REQUEST;
	    #endif
	    ret = packPtpTlvPtpMonRequest(buf, &tlv->body.ptpMonRequest, boundary);
	    break;
	case PTP_TLVTYPE_PTPMON_RESPONSE:
	    #ifdef PTP_TLVLEN_PTPMON_RESPONSE
	    expectedLen = PTP_TLVLEN_PTPMON_RESPONSE;
	    #endif
	    ret = packPtpTlvPtpMonResponse(buf, &tlv->body.ptpMonResponse, boundary);
	    break;
	case PTP_TLVTYPE_SECURITY_ASSOCIATION_UPDATE:
	    #ifdef PTP_TLVLEN_SECURITY_ASSOCIATION_UPDATE
	    expectedLen = PTP_TLVLEN_SECURITY_ASSOCIATION_UPDATE;
	    #endif
	    ret = packPtpTlvSecurityAssociationUpdate(buf, &tlv->body.securityAssociationUpdate, boundary);
	    break;
	default:
	    ret = PTP_MESSAGE_UNSUPPORTED_TLV;
	    PTPDEBUG("ptp_tlv_other.c:packPtpOtherTlvData(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    break;
    }

    /* no buffer and boundary given - only report length */
    if((buf == NULL) && (boundary == NULL)) {
	return ret;
    }

    if (ret < 0 ) {
	return ret;
    }

    if(expectedLen >= 0) {
	if(tlv->lengthField < expectedLen) {
	    return PTP_MESSAGE_CORRUPT_TLV;
	}

	if (ret < expectedLen) {
	    return PTP_MESSAGE_BUFFER_TOO_SMALL;
	}
    }

    if (ret < tlv->lengthField) {
	return PTP_MESSAGE_BUFFER_TOO_SMALL;
    }

    return ret;

}

void displayPtpOtherTlvData(PtpTlv *tlv) {

    switch(tlv->tlvType) {
	case PTP_TLVTYPE_ALTERNATE_TIME_OFFSET_INDICATOR:
	    displayPtpTlvAlternateTimeOffsetIndicator(&tlv->body.alternateTimeOffsetIndicator);
	    break;
	case PTP_TLVTYPE_AUTHENTICATION_CHALLENGE:
	    displayPtpTlvAuthenticationChallenge(&tlv->body.authenticationChallenge);
	    break;
	case PTP_TLVTYPE_AUTHENTICATION:
	    displayPtpTlvAuthentication(&tlv->body.authentication);
	    break;
	case PTP_TLVTYPE_CUM_FREQ_SCALE_FACTOR_OFFSET:
	    displayPtpTlvCumFreqScaleFactorOffset(&tlv->body.cumFreqScaleFactorOffset);
	    break;
	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	    displayPtpTlvOrganizationExtension(&tlv->body.organizationExtension);
	    break;
	case PTP_TLVTYPE_PATH_TRACE:
	    displayPtpTlvPathTrace(&tlv->body.pathTrace);
	    break;
	case PTP_TLVTYPE_PTPMON_MTIE_REQUEST:
	    displayPtpTlvPtpMonMtieRequest(&tlv->body.ptpMonMtieRequest);
	    break;
	case PTP_TLVTYPE_PTPMON_MTIE_RESPONSE:
	    displayPtpTlvPtpMonMtieResponse(&tlv->body.ptpMonMtieResponse);
	    break;
	case PTP_TLVTYPE_PTPMON_REQUEST:
	    displayPtpTlvPtpMonRequest(&tlv->body.ptpMonRequest);
	    break;
	case PTP_TLVTYPE_PTPMON_RESPONSE:
	    displayPtpTlvPtpMonResponse(&tlv->body.ptpMonResponse);
	    break;
	case PTP_TLVTYPE_SECURITY_ASSOCIATION_UPDATE:
	    displayPtpTlvSecurityAssociationUpdate(&tlv->body.securityAssociationUpdate);
	    break;
	default:
	    PTPINFO("ptp_tlv_other.c:displayPtpOtherTlvData(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    break;
    }

}

void freePtpOtherTlvData(PtpTlv *tlv) {

    switch(tlv->tlvType) {
	case PTP_TLVTYPE_ALTERNATE_TIME_OFFSET_INDICATOR:
	    freePtpTlvAlternateTimeOffsetIndicator(&tlv->body.alternateTimeOffsetIndicator);
	    break;
	case PTP_TLVTYPE_AUTHENTICATION_CHALLENGE:
	    freePtpTlvAuthenticationChallenge(&tlv->body.authenticationChallenge);
	    break;
	case PTP_TLVTYPE_AUTHENTICATION:
	    freePtpTlvAuthentication(&tlv->body.authentication);
	    break;
	case PTP_TLVTYPE_CUM_FREQ_SCALE_FACTOR_OFFSET:
	    freePtpTlvCumFreqScaleFactorOffset(&tlv->body.cumFreqScaleFactorOffset);
	    break;
	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	    freePtpTlvOrganizationExtension(&tlv->body.organizationExtension);
	    break;
	case PTP_TLVTYPE_PATH_TRACE:
	    freePtpTlvPathTrace(&tlv->body.pathTrace);
	    break;
	case PTP_TLVTYPE_PTPMON_MTIE_REQUEST:
	    freePtpTlvPtpMonMtieRequest(&tlv->body.ptpMonMtieRequest);
	    break;
	case PTP_TLVTYPE_PTPMON_MTIE_RESPONSE:
	    freePtpTlvPtpMonMtieResponse(&tlv->body.ptpMonMtieResponse);
	    break;
	case PTP_TLVTYPE_PTPMON_REQUEST:
	    freePtpTlvPtpMonRequest(&tlv->body.ptpMonRequest);
	    break;
	case PTP_TLVTYPE_PTPMON_RESPONSE:
	    freePtpTlvPtpMonResponse(&tlv->body.ptpMonResponse);
	    break;
	case PTP_TLVTYPE_SECURITY_ASSOCIATION_UPDATE:
	    freePtpTlvSecurityAssociationUpdate(&tlv->body.securityAssociationUpdate);
	    break;
	default:
	    PTPDEBUG("ptp_tlv_other.c:freePtpOtherTlvData(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    break;

    }

}

/* end generated code */

