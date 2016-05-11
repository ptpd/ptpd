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

/**
 * @file   ptp_messages.c
 * @date   Mon Apr 22 18:30:00 2016
 *
 * @brief  PTP message encoding/decoding routines
 *
 * PTP message packing and unpacking code, parsing
 * a message buffer to / from a unified PtpMessage
 * structure
 */

#include "ptp_tlv.h"
#include "ptp_tlv_signaling.h"
#include "ptp_message.h"

#include "tmp.h"

#include <stdlib.h>

int unpackPtpTlvData(PtpTlv *tlv, char* buf, char* boundary) {

    switch(tlv->tlvType) {

	case PTP_TLVTYPE_MANAGEMENT:
	case PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS:

		return unpackPtpManagementTlvData(tlv, buf, boundary);

	case PTP_TLVTYPE_REQUEST_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_GRANT_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_CANCEL_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:

		return unpackPtpSignalingTlvData(tlv, buf, boundary);

	case PTP_TLVTYPE_PTPMON_REQUEST:
	case PTP_TLVTYPE_PTPMON_RESPONSE:

		return unpackPtpOtherTlvData(tlv, buf, boundary);

	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	case PTP_TLVTYPE_PATH_TRACE:
	case PTP_TLVTYPE_ALTERNATE_TIME_OFFSET_INDICATOR:
	case PTP_TLVTYPE_AUTHENTICATION:
	case PTP_TLVTYPE_AUTHENTICATION_CHALLENGE:
	case PTP_TLVTYPE_SECURITY_ASSOCIATION_UPDATE:
	case PTP_TLVTYPE_CUM_FREQ_SCALE_FACTOR_OFFSET:

	    PTPDEBUG("ptp_tlv.c:unpackPtpTlvData(): Unsupported TLV type %04x\n", tlv->tlvType);
	    return PTP_MESSAGE_UNSUPPORTED_TLV;

	default:

	    PTPDEBUG("ptp_tlv.c:unpackPtpTlvData(): Unknown TLV type %04x\n", tlv->tlvType);
	    return PTP_MESSAGE_UNKNOWN_TLV;

    }

}

int packPtpTlvData(char *buf, PtpTlv *tlv, char *boundary) {

    switch(tlv->tlvType) {

	case PTP_TLVTYPE_MANAGEMENT:
	case PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS:

		return packPtpManagementTlvData(buf, tlv, boundary);

	case PTP_TLVTYPE_REQUEST_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_GRANT_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_CANCEL_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:

		return packPtpSignalingTlvData(buf, tlv, boundary);

	case PTP_TLVTYPE_PTPMON_REQUEST:
	case PTP_TLVTYPE_PTPMON_RESPONSE:

		return packPtpOtherTlvData(buf, tlv, boundary);

	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	case PTP_TLVTYPE_PATH_TRACE:
	case PTP_TLVTYPE_ALTERNATE_TIME_OFFSET_INDICATOR:
	case PTP_TLVTYPE_AUTHENTICATION:
	case PTP_TLVTYPE_AUTHENTICATION_CHALLENGE:
	case PTP_TLVTYPE_SECURITY_ASSOCIATION_UPDATE:
	case PTP_TLVTYPE_CUM_FREQ_SCALE_FACTOR_OFFSET:

	    PTPDEBUG("ptp_tlv.c:packPtpTlvData(): Unsupported TLV type %04x\n", tlv->tlvType);
	    return PTP_MESSAGE_UNSUPPORTED_TLV;

	default:

	    PTPDEBUG("ptp_tlv.c:packPtpTlvData(): Unknown TLV type %04x\n", tlv->tlvType);
	    return PTP_MESSAGE_UNKNOWN_TLV;

    }


}

void displayPtpTlvData(PtpTlv *tlv) {

    switch(tlv->tlvType) {

	case PTP_TLVTYPE_MANAGEMENT:
	case PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS:

		displayPtpManagementTlvData(tlv);
		break;

	case PTP_TLVTYPE_REQUEST_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_GRANT_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_CANCEL_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:

		displayPtpSignalingTlvData(tlv);
		break;

	case PTP_TLVTYPE_PTPMON_REQUEST:
	case PTP_TLVTYPE_PTPMON_RESPONSE:

		displayPtpOtherTlvData(tlv);
		break;

	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	case PTP_TLVTYPE_PATH_TRACE:
	case PTP_TLVTYPE_ALTERNATE_TIME_OFFSET_INDICATOR:
	case PTP_TLVTYPE_AUTHENTICATION:
	case PTP_TLVTYPE_AUTHENTICATION_CHALLENGE:
	case PTP_TLVTYPE_SECURITY_ASSOCIATION_UPDATE:
	case PTP_TLVTYPE_CUM_FREQ_SCALE_FACTOR_OFFSET:

	    PTPINFO("Unsupported TLV type %04x\n", tlv->tlvType);
	    return;

	default:

	    PTPINFO("Unknown TLV type %04x\n", tlv->tlvType);
	    return;

    }

}

void freePtpTlvData(PtpTlv *tlv) {

    switch(tlv->tlvType) {

	case PTP_TLVTYPE_MANAGEMENT:
	case PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS:

	    freePtpManagementTlvData(tlv);
	    break;

	case PTP_TLVTYPE_REQUEST_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_GRANT_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_CANCEL_UNICAST_TRANSMISSION:
	case PTP_TLVTYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:

	    freePtpSignalingTlvData(tlv);
	    break;

	case PTP_TLVTYPE_PTPMON_REQUEST:
	case PTP_TLVTYPE_PTPMON_RESPONSE:

	    freePtpOtherTlvData(tlv);
	    break;

	case PTP_TLVTYPE_ORGANIZATION_EXTENSION:
	case PTP_TLVTYPE_PATH_TRACE:
	case PTP_TLVTYPE_ALTERNATE_TIME_OFFSET_INDICATOR:
	case PTP_TLVTYPE_AUTHENTICATION:
	case PTP_TLVTYPE_AUTHENTICATION_CHALLENGE:
	case PTP_TLVTYPE_SECURITY_ASSOCIATION_UPDATE:
	case PTP_TLVTYPE_CUM_FREQ_SCALE_FACTOR_OFFSET:

	    PTPDEBUG("ptp_tlv.c:freePtpTlvData(): Unsupported TLV type %04x\n", tlv->tlvType);
	    return;

	default:

	    PTPDEBUG("ptp_tlv.c:freePtpTlvData(): Unknown TLV type %04x\n", tlv->tlvType);
	    return;

    }


}

PtpTlv* createPtpTlv() {
    PtpTlv *tlv = calloc(1, sizeof(PtpTlv));
    return tlv;
}

int unpackPtpTlv(PtpTlv *data, char *buf, char *boundary) {

    int offset = 0;
    int ret = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/derivedData/tlv.def"

    ret = unpackPtpTlvData(data, data->valueField, data->valueField + data->lengthField);

    if(ret < 0) {
	return ret;
    }

    if(ret != data->lengthField) {
	return PTP_MESSAGE_CORRUPT_TLV;
    }

    return offset;
}

int packPtpTlv(char *buf, PtpTlv *data, char *boundary) {

    int offset = 0;
    int ret = 0;
    int pad = 0;

    /* run with NULL to get the lengths only */
    data->lengthField = packPtpTlvData(NULL, data, NULL);

    if(data->lengthField % 2) {
	pad = 1;
    }

    data->valueField = calloc(1, data->lengthField + pad);

    if(data->valueField == NULL) {
	return PTP_MESSAGE_OTHER_ERROR;
    }

    ret = packPtpTlvData(buf, data, boundary);

    if(ret != data->lengthField) {
	return PTP_MESSAGE_CORRUPT_TLV;
    }

    data->lengthField += pad;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/derivedData/tlv.def"

    return offset + pad;
}

void displayPtpTlv(PtpTlv *data) {

    PTPINFO("PtpTlv: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/derivedData/tlv.def"

    displayPtpTlvData(data);

}

void freePtpTlv(PtpTlv *data) {

    if(data != NULL) {

	freePtpTlvData(data);

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/derivedData/tlv.def"

	free(data);
    }
}
