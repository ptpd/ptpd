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

#include "ptp_message.h"
#include "ptp_tlv.h"
#include "tmp.h"

#include <string.h>

static int unpackPtpTlvs(PtpMessage *data, char *buf, char *boundary);
static int packPtpTlvs(char *buf, PtpMessage *message, char *boundary);

#define PTP_TYPE_FUNCDEFS( type ) \
static int unpack##type (type *data, char *buf, char *boundary); \
static int pack##type (char *buf, type *data, char *boundary); \
static void display##type (type *data);

PTP_TYPE_FUNCDEFS(PtpSyncMessage);
PTP_TYPE_FUNCDEFS(PtpFollowUpMessage);
PTP_TYPE_FUNCDEFS(PtpDelayReqMessage);
PTP_TYPE_FUNCDEFS(PtpDelayRespMessage);
PTP_TYPE_FUNCDEFS(PtpPdelayReqMessage);
PTP_TYPE_FUNCDEFS(PtpPdelayRespMessage);
PTP_TYPE_FUNCDEFS(PtpPdelayRespFollowUpMessage);
PTP_TYPE_FUNCDEFS(PtpAnnounceMessage);
PTP_TYPE_FUNCDEFS(PtpManagementMessage);
PTP_TYPE_FUNCDEFS(PtpSignalingMessage);

#undef PTP_TYPE_FUNCDEFS

int unpackPtpMessageHeader(PtpMessageHeader *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/header.def"

    /* populate flag data from flag fields */

    data->flags.alternateMasterFlag = 		GET_BITFLAG(data->flagField0, PTP_FLAG_ALTERNATEMASTER);
    data->flags.twoStepFlag = 			GET_BITFLAG(data->flagField0, PTP_FLAG_TWOSTEP);
    data->flags.unicastFlag = 			GET_BITFLAG(data->flagField0, PTP_FLAG_UNICAST);
    data->flags.profileSpecific1 = 		GET_BITFLAG(data->flagField0, PTP_FLAG_PS1);
    data->flags.profileSpecific2 = 		GET_BITFLAG(data->flagField0, PTP_FLAG_PS2);
    data->flags.reserved  = 			GET_BITFLAG(data->flagField0, PTP_FLAG_RESERVED);

    data->flags.leap61 = 			GET_BITFLAG(data->flagField1, PTP_FLAG_LEAP61);
    data->flags.leap59 = 			GET_BITFLAG(data->flagField1, PTP_FLAG_LEAP59);
    data->flags.currentUtcOffsetValid  =	GET_BITFLAG(data->flagField1, PTP_FLAG_UTCV);
    data->flags.ptpTimescale = 			GET_BITFLAG(data->flagField1, PTP_FLAG_PTPTIMESCALE);
    data->flags.timeTraceable = 		GET_BITFLAG(data->flagField1, PTP_FLAG_TIMETRACEABLE);
    data->flags.frequencyTraceable = 		GET_BITFLAG(data->flagField1, PTP_FLAG_FREQTRACEABLE);

    return offset;
}

int packPtpMessageHeader(char *buf, PtpMessageHeader *data, char *boundary) {

    int offset = 0;

    /* populate flag fields from flag data */

    SET_BITFLAG(data->flagField0, PTP_FLAG_ALTERNATEMASTER, 	data->flags.alternateMasterFlag);
    SET_BITFLAG(data->flagField0, PTP_FLAG_TWOSTEP, 		data->flags.twoStepFlag);
    SET_BITFLAG(data->flagField0, PTP_FLAG_UNICAST, 		data->flags.unicastFlag);
    SET_BITFLAG(data->flagField0, PTP_FLAG_PS1, 		data->flags.profileSpecific1);
    SET_BITFLAG(data->flagField0, PTP_FLAG_PS2, 		data->flags.profileSpecific2);
    SET_BITFLAG(data->flagField0, PTP_FLAG_RESERVED, 		data->flags.reserved);

    SET_BITFLAG(data->flagField1, PTP_FLAG_LEAP61, 		data->flags.leap61);
    SET_BITFLAG(data->flagField1, PTP_FLAG_LEAP59, 		data->flags.leap59);
    SET_BITFLAG(data->flagField1, PTP_FLAG_UTCV, 		data->flags.currentUtcOffsetValid);
    SET_BITFLAG(data->flagField1, PTP_FLAG_PTPTIMESCALE, 	data->flags.ptpTimescale);
    SET_BITFLAG(data->flagField1, PTP_FLAG_TIMETRACEABLE, 	data->flags.timeTraceable);
    SET_BITFLAG(data->flagField1, PTP_FLAG_FREQTRACEABLE, 	data->flags.frequencyTraceable);

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/header.def"
    return offset;
}

void displayPtpMessageHeader(PtpMessageHeader *data) {

    PTPINFO("PtpMessageHeader: \n");
    PTPINFO("\tflags (flagField0)\t: %s%s%s%s%s%s\n",
	(data->flags.alternateMasterFlag) ? "ALT_MASTER " : "",
	(data->flags.twoStepFlag) ? "TWO_STEP " : "",
	(data->flags.unicastFlag) ? "UNICAST " : "",
	(data->flags.profileSpecific1) ? "PS1 " : "",
	(data->flags.profileSpecific2) ? "PS2 " : "",
	(data->flags.reserved) ? "RES " : ""
    );
    PTPINFO("\tflags (flagField1)\t: %s%s%s%s%s%s\n",
	(data->flags.leap61) ? "LEAP61 " : "",
	(data->flags.leap59) ? "LEAP59 " : "",
	(data->flags.currentUtcOffsetValid) ? "UTCV " : "",
	(data->flags.ptpTimescale) ? "PTPTSC " : "",
	(data->flags.timeTraceable) ? "TTRA " : "",
	(data->flags.frequencyTraceable) ? "FTRA " : ""
    );

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/header.def"
}

/* begin generated code */

static int unpackPtpSyncMessage(PtpSyncMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/sync.def"

    return offset;
}

static int packPtpSyncMessage(char *buf, PtpSyncMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/sync.def"
    return offset;
}

static void displayPtpSyncMessage(PtpSyncMessage *data) {

    PTPINFO("PtpSyncMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/sync.def"
}

static int unpackPtpFollowUpMessage(PtpFollowUpMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/followUp.def"

    return offset;
}

static int packPtpFollowUpMessage(char *buf, PtpFollowUpMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/followUp.def"
    return offset;
}

static void displayPtpFollowUpMessage(PtpFollowUpMessage *data) {

    PTPINFO("PtpFollowUpMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/followUp.def"
}

static int unpackPtpDelayReqMessage(PtpDelayReqMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/delayReq.def"

    return offset;
}

static int packPtpDelayReqMessage(char *buf, PtpDelayReqMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/delayReq.def"
    return offset;
}

static void displayPtpDelayReqMessage(PtpDelayReqMessage *data) {

    PTPINFO("PtpDelayReqMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/delayReq.def"
}

static int unpackPtpDelayRespMessage(PtpDelayRespMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/delayResp.def"

    return offset;
}

static int packPtpDelayRespMessage(char *buf, PtpDelayRespMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/delayResp.def"
    return offset;
}

static void displayPtpDelayRespMessage(PtpDelayRespMessage *data) {

    PTPINFO("PtpDelayRespMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/delayResp.def"
}

static int unpackPtpPdelayReqMessage(PtpPdelayReqMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/pdelayReq.def"

    return offset;
}

static int packPtpPdelayReqMessage(char *buf, PtpPdelayReqMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/pdelayReq.def"
    return offset;
}

static void displayPtpPdelayReqMessage(PtpPdelayReqMessage *data) {

    PTPINFO("PtpPdelayReqMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/pdelayReq.def"
}

static int unpackPtpPdelayRespMessage(PtpPdelayRespMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/pdelayResp.def"

    return offset;
}

static int packPtpPdelayRespMessage(char *buf, PtpPdelayRespMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/pdelayResp.def"
    return offset;
}

static void displayPtpPdelayRespMessage(PtpPdelayRespMessage *data) {

    PTPINFO("PtpPdelayRespMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/pdelayResp.def"
}

static int unpackPtpPdelayRespFollowUpMessage(PtpPdelayRespFollowUpMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/pdelayRespFollowUp.def"

    return offset;
}

static int packPtpPdelayRespFollowUpMessage(char *buf, PtpPdelayRespFollowUpMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/pdelayRespFollowUp.def"
    return offset;
}

static void displayPtpPdelayRespFollowUpMessage(PtpPdelayRespFollowUpMessage *data) {

    PTPINFO("PtpPdelayRespFollowUpMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/pdelayRespFollowUp.def"
}

static int unpackPtpAnnounceMessage(PtpAnnounceMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/announce.def"

    return offset;
}

static int packPtpAnnounceMessage(char *buf, PtpAnnounceMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/announce.def"
    return offset;
}

static void displayPtpAnnounceMessage(PtpAnnounceMessage *data) {

    PTPINFO("PtpAnnounceMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/announce.def"
}

static int unpackPtpManagementMessage(PtpManagementMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/management.def"

    return offset;
}

static int packPtpManagementMessage(char *buf, PtpManagementMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/management.def"
    return offset;
}

static void displayPtpManagementMessage(PtpManagementMessage *data) {

    PTPINFO("PtpManagementMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/management.def"
}

static int unpackPtpSignalingMessage(PtpSignalingMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/message/signaling.def"

    return offset;
}

static int packPtpSignalingMessage(char *buf, PtpSignalingMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/message/signaling.def"
    return offset;
}

static void displayPtpSignalingMessage(PtpSignalingMessage *data) {

    PTPINFO("PtpSignalingMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "def/message/signaling.def"
}

/* end generated code */

void attachPtpTlv(PtpMessage *message, struct PtpTlv *tlv) {

    PtpTlv *t;

    if((tlv == NULL) || (message == NULL)) {
	return;
    }

    tlv->parent = (struct PtpMessage*)message;

    if(message->firstTlv == NULL) {
	message->firstTlv = tlv;
	message->lastTlv = tlv;
	tlv->prev = NULL;
	tlv->next = NULL;
	message->tlvCount++;
    } else {
	for(t = message->firstTlv; t->next != NULL; t = t->next);
	t->next = tlv;
	tlv->prev = t;
	message->lastTlv = tlv;
	message->tlvCount++;
    }
}

static int unpackPtpTlvs(PtpMessage *message, char *buf, char *boundary) {

    PtpTlv *tlv = NULL;

    int offset = 0;
    int ret = 0;

    /* cycle through any TLVs available */
    do {

	/* movin' on up */
	offset += ret;

	/* no more TLVs */
	if((buf + offset + PTP_MSGLEN_TLV_HEADER) > boundary) {
	    break;
	}

	/* allocate the TLV */
	tlv = createPtpTlv();

	if(tlv == NULL) {
	    return PTP_MESSAGE_OTHER_ERROR;
	}

	/* attach parent message early so TLV parsers can refer to it */
	tlv->parent = (struct PtpMessage*)message;

	/* unpack the TLV and all underlying data */
	ret = unpackPtpTlv(tlv, buf + offset, boundary);

	/* TLV data unpacked must be at least 4 bytes long */
	if((ret >= 0) && (ret < PTP_MSGLEN_TLV_HEADER)) {
	    ret = PTP_MESSAGE_CORRUPT_TLV;
	}

	if(ret <= 0) {
	    freePtpTlv(tlv);
	} else {
	    attachPtpTlv(message, tlv);
	}
    } while(ret > 0);

    if(ret < 0) {
	return ret;
    }

    return offset;

}

static int packPtpTlvs(char *buf, PtpMessage *message, char *boundary) {

    PtpTlv *tlv = NULL;
    int ret = 0;
    int offset = 0;

    if(message == NULL) {
	return PTP_MESSAGE_OTHER_ERROR;
    }

    for(tlv = message->firstTlv; tlv != NULL; tlv = tlv->next) {
	ret = packPtpTlv(buf + offset, tlv, boundary);
	if(ret < 0 ) {
	    return ret;
	}
	offset += ret;
    }

    return offset;

}

void displayPtpTlvs(PtpMessage *message) {

    PtpTlv *tlv = NULL;

    if(message == NULL) {
	return;
    }

    for(tlv = message->firstTlv; tlv != NULL; tlv = tlv->next) {
	displayPtpTlv(tlv);
    }

}

void freePtpTlvs(PtpMessage *message) {

    PtpTlv *tlv = message->lastTlv;
    PtpTlv *prev = NULL;

    while(tlv != NULL) {
	prev = tlv->prev;
	freePtpTlv(tlv);
	tlv = prev;
    }

    message->firstTlv = NULL;
    message->lastTlv = NULL;
    message->tlvCount = 0;

}

int unpackPtpMessage(PtpMessage *data, char *buf, char *boundary) {

    PtpMessageHeader *header = &data->header;
    PtpMessageBody *body = &data->body;

    int expectedLen = 0;
    int offset = 0;
    int ret = 0;

    /* unpack header first */

    ret = unpackPtpMessageHeader(header, buf, boundary);

    /* handle errors */

    if (ret < 0 ) {
	return ret;
    }

    /* header too short */

    if (ret < PTP_MSGLEN_HEADER) {
	return PTP_MESSAGE_HEADER_TOO_SHORT;
    }

    /* movin' on up */

    offset += ret;

    /* unpack message body */

    switch(header->messageType) {

	case PTP_MSGTYPE_SYNC:
	    expectedLen = PTP_MSGLEN_SYNC;
	    ret = unpackPtpSyncMessage(&body->sync, buf + offset, boundary);
	break;

	case PTP_MSGTYPE_DELAY_REQ:
	    expectedLen = PTP_MSGLEN_DELAY_REQ;
	    ret = unpackPtpDelayReqMessage(&body->delayReq, buf + offset, boundary);
	break;

	case PTP_MSGTYPE_PDELAY_REQ:
	    expectedLen = PTP_MSGLEN_PDELAY_REQ;
	    ret = unpackPtpPdelayReqMessage(&body->pdelayReq, buf + offset, boundary);
	break;

	case PTP_MSGTYPE_PDELAY_RESP:
	    expectedLen = PTP_MSGLEN_PDELAY_RESP;
	    ret = unpackPtpPdelayRespMessage(&body->pdelayResp, buf + offset, boundary);
	break;

	case PTP_MSGTYPE_FOLLOW_UP:
	    expectedLen = PTP_MSGLEN_FOLLOW_UP;
	    ret = unpackPtpFollowUpMessage(&body->followUp, buf + offset, boundary);
	break;

	case PTP_MSGTYPE_DELAY_RESP:
	    expectedLen = PTP_MSGLEN_DELAY_RESP;
	    ret = unpackPtpDelayRespMessage(&body->delayResp, buf + offset, boundary);
	break;

	case PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP:
	    expectedLen = PTP_MSGLEN_PDELAY_RESP_FOLLOW_UP;
	    ret = unpackPtpPdelayRespFollowUpMessage(&body->pdelayRespFollowUp, buf + offset, boundary);
	break;

	case PTP_MSGTYPE_ANNOUNCE:
	    expectedLen = PTP_MSGLEN_ANNOUNCE;
	    ret = unpackPtpAnnounceMessage(&body->announce, buf + offset, boundary);
	break;

	case PTP_MSGTYPE_SIGNALING:
	    expectedLen = PTP_MSGLEN_SIGNALING;
	    ret = unpackPtpSignalingMessage(&body->signaling, buf + offset, boundary);
	break;

	case PTP_MSGTYPE_MANAGEMENT:
	    expectedLen = PTP_MSGLEN_MANAGEMENT;
	    ret = unpackPtpManagementMessage(&body->management, buf + offset, boundary);
	break;

	default:
	    ret = PTP_MESSAGE_UNKNOWN_TYPE;
	break;
    }

    /* handle errors */

    if (ret < 0 ) {
	return ret;
    }

    /* message body too short */

//    PTPINFO("******** l %d ex %d ret %d ret+h %d\n", header->messageLength, expectedLen, ret, ret + PTP_MSGLEN_HEADER);

    if(header->messageLength < expectedLen) {
	return PTP_MESSAGE_BODY_TOO_SHORT;
    }

    if ((ret + PTP_MSGLEN_HEADER) < expectedLen) {
	return PTP_MESSAGE_BODY_TOO_SHORT;
    }

    /* movin' on up */

    offset += ret;

    ret = unpackPtpTlvs(data, buf + offset, boundary);

    if (ret < 0 ) {
	return ret;
    }

    offset += ret;

    /* checks we can perform while unpacking */
    data->checks.isEvent = (header->messageType < PTP_MSGCLASS_GENERAL);

    return offset;
}

int packPtpMessage(char *buf, PtpMessage *data, char *boundary) {

    PtpMessageHeader *header = &data->header;
    PtpMessageBody *body = &data->body;

    int expectedLen = 0;
    int offset = 0;
    int ret = 0;

    /* skip the header - it will be packed last, to include message length */

    if( (boundary - buf) < PTP_MSGLEN_HEADER) {
	return PTP_MESSAGE_BUFFER_TOO_SMALL;
    }

    /* movin' on up */

    offset += PTP_MSGLEN_HEADER;

    /* pack message body */

    switch(header->messageType) {

	case PTP_MSGTYPE_SYNC:
	    expectedLen = PTP_MSGLEN_SYNC;
	    ret = packPtpSyncMessage(buf + offset, &body->sync, boundary);
	break;

	case PTP_MSGTYPE_DELAY_REQ:
	    expectedLen = PTP_MSGLEN_DELAY_REQ;
	    ret = packPtpDelayReqMessage(buf + offset, &body->delayReq, boundary);
	break;

	case PTP_MSGTYPE_PDELAY_REQ:
	    expectedLen = PTP_MSGLEN_PDELAY_REQ;
	    ret = packPtpPdelayReqMessage(buf + offset, &body->pdelayReq, boundary);
	break;

	case PTP_MSGTYPE_PDELAY_RESP:
	    expectedLen = PTP_MSGLEN_PDELAY_RESP;
	    ret = packPtpPdelayRespMessage(buf + offset, &body->pdelayResp, boundary);
	break;

	case PTP_MSGTYPE_FOLLOW_UP:
	    expectedLen = PTP_MSGLEN_FOLLOW_UP;
	    ret = packPtpFollowUpMessage(buf + offset, &body->followUp, boundary);
	break;

	case PTP_MSGTYPE_DELAY_RESP:
	    expectedLen = PTP_MSGLEN_DELAY_RESP;
	    ret = packPtpDelayRespMessage(buf + offset, &body->delayResp, boundary);
	break;

	case PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP:
	    expectedLen = PTP_MSGLEN_PDELAY_RESP_FOLLOW_UP;
	    ret = packPtpPdelayRespFollowUpMessage(buf + offset, &body->pdelayRespFollowUp, boundary);
	break;

	case PTP_MSGTYPE_ANNOUNCE:
	    expectedLen = PTP_MSGLEN_ANNOUNCE;
	    ret = packPtpAnnounceMessage(buf + offset, &body->announce, boundary);
	break;

	case PTP_MSGTYPE_SIGNALING:
	    expectedLen = PTP_MSGLEN_SIGNALING;
	    ret = packPtpSignalingMessage(buf + offset, &body->signaling, boundary);
	break;

	case PTP_MSGTYPE_MANAGEMENT:
	    expectedLen = PTP_MSGLEN_MANAGEMENT;
	    ret = packPtpManagementMessage(buf + offset, &body->management, boundary);
	break;

	default:
	    ret = PTP_MESSAGE_UNKNOWN_TYPE;
	break;
    }

    /* handle errors */

    if (ret < 0 ) {
	return ret;
    }

    /* message body too short */

    if ((ret + PTP_MSGLEN_HEADER) < expectedLen) {
	return PTP_MESSAGE_BODY_TOO_SHORT;
    }

    /* movin' on up */

    offset += ret;

    ret = packPtpTlvs(buf + offset, data, boundary);

    if (ret < 0 ) {
	return ret;
    }

    offset += ret;

    /* pack the header last to include messageLength */

    header->messageLength = offset + PTP_MSGLEN_HEADER;

    ret = packPtpMessageHeader(buf, header, boundary);

    /* handle errors */

    if (ret < 0 ) {
	return ret;
    }

    /* header too short */

    if (ret < PTP_MSGLEN_HEADER) {
	return PTP_MESSAGE_HEADER_TOO_SHORT;
    }

    return offset;
}


void displayPtpMessage(PtpMessage *data) {

    PTPINFO("PtpMessage: \n");

    displayPtpMessageHeader(&data->header);

    switch(data->header.messageType) {

	case PTP_MSGTYPE_SYNC:
	    displayPtpSyncMessage(&data->body.sync);
	break;

	case PTP_MSGTYPE_DELAY_REQ:
	    displayPtpDelayReqMessage(&data->body.delayReq);
	break;

	case PTP_MSGTYPE_PDELAY_REQ:
	    displayPtpPdelayReqMessage(&data->body.pdelayReq);
	break;

	case PTP_MSGTYPE_PDELAY_RESP:
	    displayPtpPdelayRespMessage(&data->body.pdelayResp);
	break;

	case PTP_MSGTYPE_FOLLOW_UP:
	    displayPtpFollowUpMessage(&data->body.followUp);
	break;

	case PTP_MSGTYPE_DELAY_RESP:
	    displayPtpDelayRespMessage(&data->body.delayResp);
	break;

	case PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP:
	    displayPtpPdelayRespFollowUpMessage(&data->body.pdelayRespFollowUp);
	break;

	case PTP_MSGTYPE_ANNOUNCE:
	    displayPtpAnnounceMessage(&data->body.announce);
	break;

	case PTP_MSGTYPE_SIGNALING:
	    displayPtpSignalingMessage(&data->body.signaling);
	break;

	case PTP_MSGTYPE_MANAGEMENT:
	    displayPtpManagementMessage(&data->body.management);
	break;

	default:
	    PTPINFO("(Unknown message type\t: %02x)\n", data->header.messageType);
	break;
    }

    displayPtpTlvs(data);

}

void freePtpMessage(PtpMessage *message) {

    freePtpTlvs(message);
    memset(message, 0, sizeof(PtpMessage));

}
