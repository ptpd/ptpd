int unpackPtpSyncMessage(PtpSyncMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/message/sync.def"

    return offset;
}

int packPtpSyncMessage(char *buf, PtpSyncMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/message/sync.def"
    return offset;
}

void freePtpSyncMessage(PtpSyncMessage *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/message/sync.def"

}

void displayPtpSyncMessage(PtpSyncMessage *data) {

    PTPINFO("PtpSyncMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/message/sync.def"
}
int unpackPtpFollowUpMessage(PtpFollowUpMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/message/followUp.def"

    return offset;
}

int packPtpFollowUpMessage(char *buf, PtpFollowUpMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/message/followUp.def"
    return offset;
}

void freePtpFollowUpMessage(PtpFollowUpMessage *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/message/followUp.def"

}

void displayPtpFollowUpMessage(PtpFollowUpMessage *data) {

    PTPINFO("PtpFollowUpMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/message/followUp.def"
}
int unpackPtpDelayReqMessage(PtpDelayReqMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/message/delayReq.def"

    return offset;
}

int packPtpDelayReqMessage(char *buf, PtpDelayReqMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/message/delayReq.def"
    return offset;
}

void freePtpDelayReqMessage(PtpDelayReqMessage *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/message/delayReq.def"

}

void displayPtpDelayReqMessage(PtpDelayReqMessage *data) {

    PTPINFO("PtpDelayReqMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/message/delayReq.def"
}
int unpackPtpDelayRespMessage(PtpDelayRespMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/message/delayResp.def"

    return offset;
}

int packPtpDelayRespMessage(char *buf, PtpDelayRespMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/message/delayResp.def"
    return offset;
}

void freePtpDelayRespMessage(PtpDelayRespMessage *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/message/delayResp.def"

}

void displayPtpDelayRespMessage(PtpDelayRespMessage *data) {

    PTPINFO("PtpDelayRespMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/message/delayResp.def"
}
int unpackPtpPdelayReqMessage(PtpPdelayReqMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/message/pdelayReq.def"

    return offset;
}

int packPtpPdelayReqMessage(char *buf, PtpPdelayReqMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/message/pdelayReq.def"
    return offset;
}

void freePtpPdelayReqMessage(PtpPdelayReqMessage *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/message/pdelayReq.def"

}

void displayPtpPdelayReqMessage(PtpPdelayReqMessage *data) {

    PTPINFO("PtpPdelayReqMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/message/pdelayReq.def"
}
int unpackPtpPdelayRespMessage(PtpPdelayRespMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/message/pdelayResp.def"

    return offset;
}

int packPtpPdelayRespMessage(char *buf, PtpPdelayRespMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/message/pdelayResp.def"
    return offset;
}

void freePtpPdelayRespMessage(PtpPdelayRespMessage *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/message/pdelayResp.def"

}

void displayPtpPdelayRespMessage(PtpPdelayRespMessage *data) {

    PTPINFO("PtpPdelayRespMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/message/pdelayResp.def"
}
int unpackPtpPdelayRespFollowUpMessage(PtpPdelayRespFollowUpMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/message/pdelayRespFollowUp.def"

    return offset;
}

int packPtpPdelayRespFollowUpMessage(char *buf, PtpPdelayRespFollowUpMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/message/pdelayRespFollowUp.def"
    return offset;
}

void freePtpPdelayRespFollowUpMessage(PtpPdelayRespFollowUpMessage *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/message/pdelayRespFollowUp.def"

}

void displayPtpPdelayRespFollowUpMessage(PtpPdelayRespFollowUpMessage *data) {

    PTPINFO("PtpPdelayRespFollowUpMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/message/pdelayRespFollowUp.def"
}
int unpackPtpAnnounceMessage(PtpAnnounceMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/message/announce.def"

    return offset;
}

int packPtpAnnounceMessage(char *buf, PtpAnnounceMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/message/announce.def"
    return offset;
}

void freePtpAnnounceMessage(PtpAnnounceMessage *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/message/announce.def"

}

void displayPtpAnnounceMessage(PtpAnnounceMessage *data) {

    PTPINFO("PtpAnnounceMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/message/announce.def"
}
int unpackPtpManagementMessage(PtpManagementMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/message/management.def"

    return offset;
}

int packPtpManagementMessage(char *buf, PtpManagementMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/message/management.def"
    return offset;
}

void freePtpManagementMessage(PtpManagementMessage *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/message/management.def"

}

void displayPtpManagementMessage(PtpManagementMessage *data) {

    PTPINFO("PtpManagementMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/message/management.def"
}
int unpackPtpSignalingMessage(PtpSignalingMessage *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/message/signaling.def"

    return offset;
}

int packPtpSignalingMessage(char *buf, PtpSignalingMessage *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/message/signaling.def"
    return offset;
}

void freePtpSignalingMessage(PtpSignalingMessage *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/message/signaling.def"

}

void displayPtpSignalingMessage(PtpSignalingMessage *data) {

    PTPINFO("PtpSignalingMessage: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/message/signaling.def"
}

* ==================== *\/

int unpackPtpTlvAcknowledgeCancelUnicastTransmission(PtpTlvAcknowledgeCancelUnicastTransmission *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/signalingTLV/acknowledgeCancelUnicastTransmission.def"

    return offset;
}

int packPtpTlvAcknowledgeCancelUnicastTransmission(char *buf, PtpTlvAcknowledgeCancelUnicastTransmission *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/signalingTLV/acknowledgeCancelUnicastTransmission.def"
    return offset;
}

void freePtpTlvAcknowledgeCancelUnicastTransmission(PtpTlvAcknowledgeCancelUnicastTransmission *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/signalingTLV/acknowledgeCancelUnicastTransmission.def"

}

void displayPtpTlvAcknowledgeCancelUnicastTransmission(PtpTlvAcknowledgeCancelUnicastTransmission *data) {

    PTPINFO("PtpTlvAcknowledgeCancelUnicastTransmission: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/signalingTLV/acknowledgeCancelUnicastTransmission.def"
}
int unpackPtpTlvCancelUnicastTransmission(PtpTlvCancelUnicastTransmission *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/signalingTLV/cancelUnicastTransmission.def"

    return offset;
}

int packPtpTlvCancelUnicastTransmission(char *buf, PtpTlvCancelUnicastTransmission *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/signalingTLV/cancelUnicastTransmission.def"
    return offset;
}

void freePtpTlvCancelUnicastTransmission(PtpTlvCancelUnicastTransmission *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/signalingTLV/cancelUnicastTransmission.def"

}

void displayPtpTlvCancelUnicastTransmission(PtpTlvCancelUnicastTransmission *data) {

    PTPINFO("PtpTlvCancelUnicastTransmission: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/signalingTLV/cancelUnicastTransmission.def"
}
int unpackPtpTlvGrantUnicastTransmission(PtpTlvGrantUnicastTransmission *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/signalingTLV/grantUnicastTransmission.def"

    return offset;
}

int packPtpTlvGrantUnicastTransmission(char *buf, PtpTlvGrantUnicastTransmission *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/signalingTLV/grantUnicastTransmission.def"
    return offset;
}

void freePtpTlvGrantUnicastTransmission(PtpTlvGrantUnicastTransmission *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/signalingTLV/grantUnicastTransmission.def"

}

void displayPtpTlvGrantUnicastTransmission(PtpTlvGrantUnicastTransmission *data) {

    PTPINFO("PtpTlvGrantUnicastTransmission: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/signalingTLV/grantUnicastTransmission.def"
}
int unpackPtpTlvRequestUnicastTransmission(PtpTlvRequestUnicastTransmission *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return offset; \
	} \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "definitions/signalingTLV/requestUnicastTransmission.def"

    return offset;
}

int packPtpTlvRequestUnicastTransmission(char *buf, PtpTlvRequestUnicastTransmission *data, char *boundary) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
	if((buf + offset + size) > boundary) { \
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \
	} \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "definitions/signalingTLV/requestUnicastTransmission.def"
    return offset;
}

void freePtpTlvRequestUnicastTransmission(PtpTlvRequestUnicastTransmission *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name); \
    #include "definitions/signalingTLV/requestUnicastTransmission.def"

}

void displayPtpTlvRequestUnicastTransmission(PtpTlvRequestUnicastTransmission *data) {

    PTPINFO("PtpTlvRequestUnicastTransmission: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t"#name, size);
    #include "definitions/signalingTLV/requestUnicastTransmission.def"
}
