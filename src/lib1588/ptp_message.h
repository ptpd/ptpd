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

#ifndef PTP_MESSAGE_H_
#define PTP_MESSAGE_H_

#include <stdio.h>

#include "ptp_derived_types.h"
#include "ptp_tlv.h"

/* PTP message unpack error codes */
#define PTP_MESSAGE_OTHER_ERROR		-8
#define PTP_MESSAGE_BUFFER_TOO_SMALL	-7
#define PTP_MESSAGE_UNSUPPORTED_TLV	-6
#define PTP_MESSAGE_UNKNOWN_TLV		-5
#define PTP_MESSAGE_CORRUPT_TLV		-4
#define PTP_MESSAGE_BODY_TOO_SHORT	-3
#define PTP_MESSAGE_HEADER_TOO_SHORT	-2
#define PTP_MESSAGE_UNKNOWN_TYPE	-1
#define PTP_MESSAGE_OK			 0

/* PTP expected message lengths */

#define PTP_MSGLEN_TLV_HEADER			4
#define PTP_MSGLEN_HEADER			34
#define PTP_MSGLEN_SYNC				44
#define PTP_MSGLEN_DELAY_REQ			44
#define PTP_MSGLEN_PDELAY_REQ			54
#define PTP_MSGLEN_PDELAY_RESP			54
#define PTP_MSGLEN_FOLLOW_UP			44
#define PTP_MSGLEN_DELAY_RESP			54
#define PTP_MSGLEN_PDELAY_RESP_FOLLOW_UP	54
#define PTP_MSGLEN_ANNOUNCE			64
#define PTP_MSGLEN_SIGNALING			44
#define PTP_MSGLEN_MANAGEMENT			48

/* Bit flag helpers */
#define GET_BITFLAG(field, flag) (field & (flag)) ? 1 : 0
#define SET_BITFLAG(field, flag, value) \
field &= ~(flag);\
field |= (value);

/* PTP header flags */

/* flagField0 */
#define PTP_FLAG_ALTERNATEMASTER	1 << 0
#define PTP_FLAG_TWOSTEP		1 << 1
#define PTP_FLAG_UNICAST		1 << 2
#define PTP_FLAG_PS1			1 << 5
#define PTP_FLAG_PS2			1 << 6
#define PTP_FLAG_RESERVED		1 << 7

/* flagField1 */
#define PTP_FLAG_LEAP61			1 << 0
#define PTP_FLAG_LEAP59			1 << 1
#define PTP_FLAG_UTCV			1 << 2
#define PTP_FLAG_PTPTIMESCALE		1 << 3
#define PTP_FLAG_TIMETRACEABLE		1 << 4
#define PTP_FLAG_FREQTRACEABLE		1 << 5

/* PTP message types */
enum {
    PTP_MSGTYPE_SYNC =			0x00,
    PTP_MSGTYPE_DELAY_REQ =		0x01,
    PTP_MSGTYPE_PDELAY_REQ =		0x02,
    PTP_MSGTYPE_PDELAY_RESP =		0x03,
    PTP_MSGTYPE_FOLLOW_UP =		0x08,
    PTP_MSGTYPE_DELAY_RESP =		0x09,
    PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP =	0x0a,
    PTP_MSGTYPE_ANNOUNCE =		0x0b,
    PTP_MSGTYPE_SIGNALING =		0x0c,
    PTP_MSGTYPE_MANAGEMENT =		0x0d
};

#define PTP_MSGCLASS_GENERAL 0x08

/* PTP controlField values (deprecated) */
enum {
    PTP_CONTROL_SYNC =			0x00,
    PTP_CONTROL_DELAY_REQ =		0x01,
    PTP_CONTROL_FOLLOW_UP =		0x02,
    PTP_CONTROL_DELAY_RESP =		0x03,
    PTP_CONTROL_MANAGEMENT =		0x04,
    PTP_CONTROL_OTHER =			0x05
};

/* PTP header */

typedef struct {

    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/header.def"

    struct {
	/* field 1 */
	PtpBoolean alternateMasterFlag;
	PtpBoolean twoStepFlag;
	PtpBoolean unicastFlag;
	PtpBoolean profileSpecific1;
	PtpBoolean profileSpecific2;
	PtpBoolean reserved;
	/* field 2 */
	PtpBoolean leap61;
	PtpBoolean leap59;
	PtpBoolean currentUtcOffsetValid;
	PtpBoolean ptpTimescale;
	PtpBoolean timeTraceable;
	PtpBoolean frequencyTraceable;
    } flags;

} PtpMessageHeader;

/* PTP messages */

typedef struct {
    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/sync.def"
} PtpSyncMessage;

typedef struct {
    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/followUp.def"
} PtpFollowUpMessage;

typedef struct {
    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/delayReq.def"
} PtpDelayReqMessage;

typedef struct {
    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/delayResp.def"
} PtpDelayRespMessage;

typedef struct {
    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/pdelayReq.def"
} PtpPdelayReqMessage;

typedef struct {
    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/pdelayResp.def"
} PtpPdelayRespMessage;

typedef struct {
    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/pdelayRespFollowUp.def"
} PtpPdelayRespFollowUpMessage;

typedef struct {
    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/announce.def"
} PtpAnnounceMessage;

typedef struct {
    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/management.def"
} PtpManagementMessage;

typedef struct {
    #define PROCESS_FIELD( name, size, type ) type name;
    #include "def/message/signaling.def"
} PtpSignalingMessage;

typedef union {

    PtpSyncMessage			sync;
    PtpFollowUpMessage			followUp;
    PtpDelayReqMessage			delayReq;
    PtpDelayRespMessage			delayResp;
    PtpPdelayReqMessage			pdelayReq;
    PtpPdelayRespMessage		pdelayResp;
    PtpPdelayRespFollowUpMessage	pdelayRespFollowUp;
    PtpAnnounceMessage			announce;
    PtpManagementMessage		management;
    PtpSignalingMessage			signaling;

} PtpMessageBody;

typedef struct {

    struct PtpTlv *firstTlv;
    struct PtpTlv *lastTlv;

    PtpMessageHeader	header;
    PtpMessageBody	body;

    unsigned int tlvCount;

    struct {
	PtpBoolean fromSelf;
	PtpBoolean isEvent;
	PtpBoolean isPeer;
    } checks;

} PtpMessage;

#define PTP_TYPE_FUNCDEFS( type ) \
int unpack##type (type *data, char *buf, char *boundary); \
int pack##type (char *buf, type *data, char *boundary); \
void display##type (type *data);

PTP_TYPE_FUNCDEFS(PtpMessageHeader);
PTP_TYPE_FUNCDEFS(PtpMessage);

void freePtpMessage(PtpMessage *message);

void attachPtpTlv(PtpMessage *message, struct PtpTlv *tlv);
void displayPtpTlvs(PtpMessage *message);
void freePtpTlvs(PtpMessage *message);

#undef PTP_TYPE_FUNCDEFS

#endif /* PTP_MESSAGES_H_ */
