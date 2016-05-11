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

PTP_TYPE_FUNCDEFS(PtpTlvOrganizationExtension)
PTP_TYPE_FUNCDEFS(PtpTlvPtpMonRequest)
PTP_TYPE_FUNCDEFS(PtpTlvPtpMonResponse)

#undef PTP_TYPE_FUNCDEFS

static int unpackPtpTlvOrganizationExtension(PtpTlvOrganizationExtension *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/otherTlv/organizationExtension.def"

    return offset;
}

static int packPtpTlvOrganizationExtension(char *buf, PtpTlvOrganizationExtension *data, char *boundary) {

    int offset = 0;


    /* if no boundary and buffer provided, we only report the length. */
    /* this is used to help pre-allocate memory to fit this */
    #define PROCESS_FIELD( name, size, type) \
	if((buf == NULL) && (boundary == NULL)) {\
	    offset += size; \
	} else { \
	     if((buf + offset + size) > boundary) { \
		return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	    } \
	    pack##type (buf + offset, &data->name, size); \
	    offset += size; \
	}
    #include "definitions/otherTlv/organizationExtension.def"
    return offset;
}

static void freePtpTlvOrganizationExtension(PtpTlvOrganizationExtension *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/otherTlv/organizationExtension.def"

}

static void displayPtpTlvOrganizationExtension(PtpTlvOrganizationExtension *data) {

    PTPINFO("\tPtpTlvOrganizationExtension: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/otherTlv/organizationExtension.def"
}
static int unpackPtpTlvPtpMonRequest(PtpTlvPtpMonRequest *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/otherTlv/ptpMonRequest.def"

    return offset;
}

static int packPtpTlvPtpMonRequest(char *buf, PtpTlvPtpMonRequest *data, char *boundary) {

    int offset = 0;


    /* if no boundary and buffer provided, we only report the length. */
    /* this is used to help pre-allocate memory to fit this */
    #define PROCESS_FIELD( name, size, type) \
	if((buf == NULL) && (boundary == NULL)) {\
	    offset += size; \
	} else { \
	     if((buf + offset + size) > boundary) { \
		return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	    } \
	    pack##type (buf + offset, &data->name, size); \
	    offset += size; \
	}
    #include "definitions/otherTlv/ptpMonRequest.def"
    return offset;
}

static void freePtpTlvPtpMonRequest(PtpTlvPtpMonRequest *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/otherTlv/ptpMonRequest.def"

}

static void displayPtpTlvPtpMonRequest(PtpTlvPtpMonRequest *data) {

    PTPINFO("\tPtpTlvPtpMonRequest: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/otherTlv/ptpMonRequest.def"
}
static int unpackPtpTlvPtpMonResponse(PtpTlvPtpMonResponse *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/otherTlv/ptpMonResponse.def"

    return offset;
}

static int packPtpTlvPtpMonResponse(char *buf, PtpTlvPtpMonResponse *data, char *boundary) {

    int offset = 0;


    /* if no boundary and buffer provided, we only report the length. */
    /* this is used to help pre-allocate memory to fit this */
    #define PROCESS_FIELD( name, size, type) \
	if((buf == NULL) && (boundary == NULL)) {\
	    offset += size; \
	} else { \
	     if((buf + offset + size) > boundary) { \
		return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	    } \
	    pack##type (buf + offset, &data->name, size); \
	    offset += size; \
	}
    #include "definitions/otherTlv/ptpMonResponse.def"
    return offset;
}

static void freePtpTlvPtpMonResponse(PtpTlvPtpMonResponse *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/otherTlv/ptpMonResponse.def"

}

static void displayPtpTlvPtpMonResponse(PtpTlvPtpMonResponse *data) {

    PTPINFO("\tPtpTlvPtpMonResponse: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/otherTlv/ptpMonResponse.def"
}
int unpackPtpOtherTlvData(PtpTlv *tlv, char* buf, char* boundary) {

    int ret = 0;
    int expectedLen = -1;

    switch(tlv->tlvType) {
	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	    #ifdef PTP_TLVLEN_ORGANIZATION_EXTENSION
	    expectedLen = PTP_TLVLEN_ORGANIZATION_EXTENSION;
	    #endif
	    ret = unpackPtpTlvOrganizationExtension(&tlv->body.organizationExtension, buf, boundary);
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
default:
	    PTPDEBUG("../../ptp_tlv_other.c:unpackPtpOtherTlvData(): Unsupported TLV type %04x\n",
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
	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	    #ifdef PTP_TLVLEN_ORGANIZATION_EXTENSION
	    expectedLen = PTP_TLVLEN_ORGANIZATION_EXTENSION;
	    #endif
	    ret = packPtpTlvOrganizationExtension(buf, &tlv->body.organizationExtension, boundary);
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
default:
	    ret = PTP_MESSAGE_UNSUPPORTED_TLV;
	    PTPDEBUG("../../ptp_tlv_other.c:packPtpOtherTlvData(): Unsupported TLV type %04x\n",
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
	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	    displayPtpTlvOrganizationExtension(&tlv->body.organizationExtension);
	    break;
	case PTP_TLVTYPE_PTPMON_REQUEST:
	    displayPtpTlvPtpMonRequest(&tlv->body.ptpMonRequest);
	    break;
	case PTP_TLVTYPE_PTPMON_RESPONSE:
	    displayPtpTlvPtpMonResponse(&tlv->body.ptpMonResponse);
	    break;
default:
	    PTPINFO("../../ptp_tlv_other.c:displayPtpOtherTlvData(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    break;
    }

}
void freePtpOtherTlvData(PtpTlv *tlv) {

    switch(tlv->tlvType) {
	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	    freePtpTlvOrganizationExtension(&tlv->body.organizationExtension);
	    break;
	case PTP_TLVTYPE_PTPMON_REQUEST:
	    freePtpTlvPtpMonRequest(&tlv->body.ptpMonRequest);
	    break;
	case PTP_TLVTYPE_PTPMON_RESPONSE:
	    freePtpTlvPtpMonResponse(&tlv->body.ptpMonResponse);
	    break;
default:
	    PTPDEBUG("../../ptp_tlv_other.c:freePtpOtherTlvData(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    break;

    }

}
/* end generated code */

