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

PTP_TYPE_FUNCDEFS(PtpTlvAcknowledgeCancelUnicastTransmission)
PTP_TYPE_FUNCDEFS(PtpTlvCancelUnicastTransmission)
PTP_TYPE_FUNCDEFS(PtpTlvGrantUnicastTransmission)
PTP_TYPE_FUNCDEFS(PtpTlvRequestUnicastTransmission)

#undef PTP_TYPE_FUNCDEFS

static int unpackPtpTlvAcknowledgeCancelUnicastTransmission(PtpTlvAcknowledgeCancelUnicastTransmission *data, char *buf, char *boundary) {

    int offset = 0;
    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/signalingTlv/acknowledgeCancelUnicastTransmission.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvAcknowledgeCancelUnicastTransmission(char *buf, PtpTlvAcknowledgeCancelUnicastTransmission *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/signalingTlv/acknowledgeCancelUnicastTransmission.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvAcknowledgeCancelUnicastTransmission(PtpTlvAcknowledgeCancelUnicastTransmission *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/signalingTlv/acknowledgeCancelUnicastTransmission.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvAcknowledgeCancelUnicastTransmission(PtpTlvAcknowledgeCancelUnicastTransmission *data) {

    PTPINFO("\tPtpTlvAcknowledgeCancelUnicastTransmission: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "definitions/signalingTlv/acknowledgeCancelUnicastTransmission.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvCancelUnicastTransmission(PtpTlvCancelUnicastTransmission *data, char *buf, char *boundary) {

    int offset = 0;
    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/signalingTlv/cancelUnicastTransmission.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvCancelUnicastTransmission(char *buf, PtpTlvCancelUnicastTransmission *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/signalingTlv/cancelUnicastTransmission.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvCancelUnicastTransmission(PtpTlvCancelUnicastTransmission *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/signalingTlv/cancelUnicastTransmission.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvCancelUnicastTransmission(PtpTlvCancelUnicastTransmission *data) {

    PTPINFO("\tPtpTlvCancelUnicastTransmission: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "definitions/signalingTlv/cancelUnicastTransmission.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvGrantUnicastTransmission(PtpTlvGrantUnicastTransmission *data, char *buf, char *boundary) {

    int offset = 0;
    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/signalingTlv/grantUnicastTransmission.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvGrantUnicastTransmission(char *buf, PtpTlvGrantUnicastTransmission *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/signalingTlv/grantUnicastTransmission.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvGrantUnicastTransmission(PtpTlvGrantUnicastTransmission *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/signalingTlv/grantUnicastTransmission.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvGrantUnicastTransmission(PtpTlvGrantUnicastTransmission *data) {

    PTPINFO("\tPtpTlvGrantUnicastTransmission: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "definitions/signalingTlv/grantUnicastTransmission.def"
    #undef PROCESS_FIELD
}

static int unpackPtpTlvRequestUnicastTransmission(PtpTlvRequestUnicastTransmission *data, char *buf, char *boundary) {

    int offset = 0;
    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/signalingTlv/requestUnicastTransmission.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvRequestUnicastTransmission(char *buf, PtpTlvRequestUnicastTransmission *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/signalingTlv/requestUnicastTransmission.def"
    #undef PROCESS_FIELD

    return offset;
}

static void freePtpTlvRequestUnicastTransmission(PtpTlvRequestUnicastTransmission *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/signalingTlv/requestUnicastTransmission.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvRequestUnicastTransmission(PtpTlvRequestUnicastTransmission *data) {

    PTPINFO("\tPtpTlvRequestUnicastTransmission: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "		"#name, size);
    #include "definitions/signalingTlv/requestUnicastTransmission.def"
    #undef PROCESS_FIELD
}

int unpackPtpSignalingTlvData(PtpTlv *tlv, char* buf, char* boundary) {

    int ret = 0;
    int expectedLen = -1;

    switch(tlv->tlvType) {
	case PTP_TLVTYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:
	    #ifdef PTP_TLVLEN_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION
	    expectedLen = PTP_TLVLEN_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION;
	    #endif
	    ret = unpackPtpTlvAcknowledgeCancelUnicastTransmission(&tlv->body.acknowledgeCancelUnicastTransmission, buf, boundary);
	    break;
	case PTP_TLVTYPE_CANCEL_UNICAST_TRANSMISSION:
	    #ifdef PTP_TLVLEN_CANCEL_UNICAST_TRANSMISSION
	    expectedLen = PTP_TLVLEN_CANCEL_UNICAST_TRANSMISSION;
	    #endif
	    ret = unpackPtpTlvCancelUnicastTransmission(&tlv->body.cancelUnicastTransmission, buf, boundary);
	    break;
	case PTP_TLVTYPE_GRANT_UNICAST_TRANSMISSION:
	    #ifdef PTP_TLVLEN_GRANT_UNICAST_TRANSMISSION
	    expectedLen = PTP_TLVLEN_GRANT_UNICAST_TRANSMISSION;
	    #endif
	    ret = unpackPtpTlvGrantUnicastTransmission(&tlv->body.grantUnicastTransmission, buf, boundary);
	    break;
	case PTP_TLVTYPE_REQUEST_UNICAST_TRANSMISSION:
	    #ifdef PTP_TLVLEN_REQUEST_UNICAST_TRANSMISSION
	    expectedLen = PTP_TLVLEN_REQUEST_UNICAST_TRANSMISSION;
	    #endif
	    ret = unpackPtpTlvRequestUnicastTransmission(&tlv->body.requestUnicastTransmission, buf, boundary);
	    break;
	default:
	    PTPDEBUG("ptp_tlv_signaling.c:unpackPtpSignalingTlvData(): Unsupported TLV type %04x\n",
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

int packPtpSignalingTlvData(char *buf, PtpTlv *tlv, char *boundary) {

    int ret = 0;
    int expectedLen = -1;

    switch(tlv->tlvType) {
	case PTP_TLVTYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:
	    #ifdef PTP_TLVLEN_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION
	    expectedLen = PTP_TLVLEN_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION;
	    #endif
	    ret = packPtpTlvAcknowledgeCancelUnicastTransmission(buf, &tlv->body.acknowledgeCancelUnicastTransmission, boundary);
	    break;
	case PTP_TLVTYPE_CANCEL_UNICAST_TRANSMISSION:
	    #ifdef PTP_TLVLEN_CANCEL_UNICAST_TRANSMISSION
	    expectedLen = PTP_TLVLEN_CANCEL_UNICAST_TRANSMISSION;
	    #endif
	    ret = packPtpTlvCancelUnicastTransmission(buf, &tlv->body.cancelUnicastTransmission, boundary);
	    break;
	case PTP_TLVTYPE_GRANT_UNICAST_TRANSMISSION:
	    #ifdef PTP_TLVLEN_GRANT_UNICAST_TRANSMISSION
	    expectedLen = PTP_TLVLEN_GRANT_UNICAST_TRANSMISSION;
	    #endif
	    ret = packPtpTlvGrantUnicastTransmission(buf, &tlv->body.grantUnicastTransmission, boundary);
	    break;
	case PTP_TLVTYPE_REQUEST_UNICAST_TRANSMISSION:
	    #ifdef PTP_TLVLEN_REQUEST_UNICAST_TRANSMISSION
	    expectedLen = PTP_TLVLEN_REQUEST_UNICAST_TRANSMISSION;
	    #endif
	    ret = packPtpTlvRequestUnicastTransmission(buf, &tlv->body.requestUnicastTransmission, boundary);
	    break;
	default:
	    ret = PTP_MESSAGE_UNSUPPORTED_TLV;
	    PTPDEBUG("ptp_tlv_signaling.c:packPtpSignalingTlvData(): Unsupported TLV type %04x\n",
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

void displayPtpSignalingTlvData(PtpTlv *tlv) {

    switch(tlv->tlvType) {
	case PTP_TLVTYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:
	    displayPtpTlvAcknowledgeCancelUnicastTransmission(&tlv->body.acknowledgeCancelUnicastTransmission);
	    break;
	case PTP_TLVTYPE_CANCEL_UNICAST_TRANSMISSION:
	    displayPtpTlvCancelUnicastTransmission(&tlv->body.cancelUnicastTransmission);
	    break;
	case PTP_TLVTYPE_GRANT_UNICAST_TRANSMISSION:
	    displayPtpTlvGrantUnicastTransmission(&tlv->body.grantUnicastTransmission);
	    break;
	case PTP_TLVTYPE_REQUEST_UNICAST_TRANSMISSION:
	    displayPtpTlvRequestUnicastTransmission(&tlv->body.requestUnicastTransmission);
	    break;
	default:
	    PTPINFO("ptp_tlv_signaling.c:displayPtpSignalingTlvData(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    break;
    }

}

void freePtpSignalingTlvData(PtpTlv *tlv) {

    switch(tlv->tlvType) {
	case PTP_TLVTYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:
	    freePtpTlvAcknowledgeCancelUnicastTransmission(&tlv->body.acknowledgeCancelUnicastTransmission);
	    break;
	case PTP_TLVTYPE_CANCEL_UNICAST_TRANSMISSION:
	    freePtpTlvCancelUnicastTransmission(&tlv->body.cancelUnicastTransmission);
	    break;
	case PTP_TLVTYPE_GRANT_UNICAST_TRANSMISSION:
	    freePtpTlvGrantUnicastTransmission(&tlv->body.grantUnicastTransmission);
	    break;
	case PTP_TLVTYPE_REQUEST_UNICAST_TRANSMISSION:
	    freePtpTlvRequestUnicastTransmission(&tlv->body.requestUnicastTransmission);
	    break;
	default:
	    PTPDEBUG("ptp_tlv_signaling.c:freePtpSignalingTlvData(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    break;

    }

}

/* end generated code */

