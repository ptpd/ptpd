/**
 * @file   msg.c
 * @author George Neville-Neil <gnn@neville-neil.com>
 * @date   Tue Jul 20 16:17:05 2010
 * 
 * @brief  Functions to pack and unpack messages.
 * 
 * See spec annex d
 */

#include "../ptpd.h"

/*Unpack Header from IN buffer to msgTmpHeader field */
void 
msgUnpackHeader(void *buf, MsgHeader * header)
{
	header->transportSpecific = (*(Nibble *) (buf + 0)) >> 4;
	header->messageType = (*(Enumeration4 *) (buf + 0)) & 0x0F;
	header->versionPTP = (*(UInteger4 *) (buf + 1)) & 0x0F;
	/* force reserved bit to zero if not */
	header->messageLength = flip16(*(UInteger16 *) (buf + 2));
	header->domainNumber = (*(UInteger8 *) (buf + 4));
	memcpy(header->flagField, (buf + 6), FLAG_FIELD_LENGTH);
	memcpy(&header->correctionfield.msb, (buf + 8), 4);
	memcpy(&header->correctionfield.lsb, (buf + 12), 4);
	header->correctionfield.msb = flip32(header->correctionfield.msb);
	header->correctionfield.lsb = flip32(header->correctionfield.lsb);
	memcpy(header->sourcePortIdentity.clockIdentity, (buf + 20), 
	       CLOCK_IDENTITY_LENGTH);
	header->sourcePortIdentity.portNumber = 
		flip16(*(UInteger16 *) (buf + 28));
	header->sequenceId = flip16(*(UInteger16 *) (buf + 30));
	header->controlField = (*(UInteger8 *) (buf + 32));
	header->logMessageInterval = (*(Integer8 *) (buf + 33));

#ifdef PTPD_DBG
	msgHeader_display(header);
#endif /* PTPD_DBG */
}

/*Pack header message into OUT buffer of ptpClock*/
void 
msgPackHeader(void *buf, PtpClock * ptpClock)
{
	Nibble transport = 0x80;

	/* (spec annex D) */
	*(UInteger8 *) (buf + 0) = transport;
	*(UInteger4 *) (buf + 1) = ptpClock->versionNumber;
	*(UInteger8 *) (buf + 4) = ptpClock->domainNumber;

	if (ptpClock->twoStepFlag)
		*(UInteger8 *) (buf + 6) = TWO_STEP_FLAG;

	memset((buf + 8), 0, 8);
	memcpy((buf + 20), ptpClock->portIdentity.clockIdentity, 
	       CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 28) = flip16(ptpClock->portIdentity.portNumber);
	*(UInteger8 *) (buf + 33) = 0x7F;
	/* Default value(spec Table 24) */
}



/*Pack SYNC message into OUT buffer of ptpClock*/
void 
msgPackSync(void *buf, Timestamp * originTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x00;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(SYNC_LENGTH);
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->sentSyncSequenceId);
	*(UInteger8 *) (buf + 32) = 0x00;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = ptpClock->logSyncInterval;
	memset((buf + 8), 0, 8);

	/* Sync message */
	*(UInteger16 *) (buf + 34) = flip16(originTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = flip32(originTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = flip32(originTimestamp->nanosecondsField);
}

/*Unpack Sync message from IN buffer */
void 
msgUnpackSync(void *buf, MsgSync * sync)
{
	sync->originTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	sync->originTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	sync->originTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));

#ifdef PTPD_DBG
	msgSync_display(sync);
#endif /* PTPD_DBG */
}



/*Pack Announce message into OUT buffer of ptpClock*/
void 
msgPackAnnounce(void *buf, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x0B;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(ANNOUNCE_LENGTH);
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->sentAnnounceSequenceId);
	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = ptpClock->logAnnounceInterval;

	/* Announce message */
	memset((buf + 34), 0, 10);
	*(Integer16 *) (buf + 44) = flip16(ptpClock->currentUtcOffset);
	*(UInteger8 *) (buf + 47) = ptpClock->grandmasterPriority1;
	*(UInteger8 *) (buf + 48) = ptpClock->clockQuality.clockClass;
	*(Enumeration8 *) (buf + 49) = ptpClock->clockQuality.clockAccuracy;
	*(UInteger16 *) (buf + 50) = 
		flip16(ptpClock->clockQuality.offsetScaledLogVariance);
	*(UInteger8 *) (buf + 52) = ptpClock->grandmasterPriority2;
	memcpy((buf + 53), ptpClock->grandmasterIdentity, CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 61) = flip16(ptpClock->stepsRemoved);
	*(Enumeration8 *) (buf + 63) = ptpClock->timeSource;
}

/*Unpack Announce message from IN buffer of ptpClock to msgtmp.Announce*/
void 
msgUnpackAnnounce(void *buf, MsgAnnounce * announce)
{
	announce->originTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	announce->originTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	announce->originTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));
	announce->currentUtcOffset = flip16(*(UInteger16 *) (buf + 44));
	announce->grandmasterPriority1 = *(UInteger8 *) (buf + 47);
	announce->grandmasterClockQuality.clockClass = 
		*(UInteger8 *) (buf + 48);
	announce->grandmasterClockQuality.clockAccuracy = 
		*(Enumeration8 *) (buf + 49);
	announce->grandmasterClockQuality.offsetScaledLogVariance = 
		flip16(*(UInteger16 *) (buf + 50));
	announce->grandmasterPriority2 = *(UInteger8 *) (buf + 52);
	memcpy(announce->grandmasterIdentity, (buf + 53), 
	       CLOCK_IDENTITY_LENGTH);
	announce->stepsRemoved = flip16(*(UInteger16 *) (buf + 61));
	announce->timeSource = *(Enumeration8 *) (buf + 63);
	
#ifdef PTPD_DBG
	msgAnnounce_display(announce);
#endif /* PTPD_DBG */
}

/*pack Follow_up message into OUT buffer of ptpClock*/
void 
msgPackFollowUp(void *buf, Timestamp * preciseOriginTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x08;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(FOLLOW_UP_LENGTH);
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->sentSyncSequenceId - 1);
	/* sentSyncSequenceId has already been incremented in "issueSync" */
	*(UInteger8 *) (buf + 32) = 0x02;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = ptpClock->logSyncInterval;

	/* Follow_up message */
	*(UInteger16 *) (buf + 34) = 
		flip16(preciseOriginTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = 
		flip32(preciseOriginTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = 
		flip32(preciseOriginTimestamp->nanosecondsField);
}

/*Unpack Follow_up message from IN buffer of ptpClock to msgtmp.follow*/
void 
msgUnpackFollowUp(void *buf, MsgFollowUp * follow)
{
	follow->preciseOriginTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	follow->preciseOriginTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	follow->preciseOriginTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));

#ifdef PTPD_DBG
	msgFollowUp_display(follow);
#endif /* PTPD_DBG */

}


/*pack PdelayReq message into OUT buffer of ptpClock*/
void 
msgPackPDelayReq(void *buf, Timestamp * originTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x02;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(PDELAY_REQ_LENGTH);
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->sentPDelayReqSequenceId);
	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;
	/* Table 24 */
	memset((buf + 8), 0, 8);

	/* Pdelay_req message */
	*(UInteger16 *) (buf + 34) = flip16(originTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = flip32(originTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = flip32(originTimestamp->nanosecondsField);

	memset((buf + 44), 0, 10);
	/* RAZ reserved octets */
}

/*pack delayReq message into OUT buffer of ptpClock*/
void 
msgPackDelayReq(void *buf, Timestamp * originTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x01;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(DELAY_REQ_LENGTH);
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->sentDelayReqSequenceId);
	*(UInteger8 *) (buf + 32) = 0x01;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;
	/* Table 24 */
	memset((buf + 8), 0, 8);

	/* Pdelay_req message */
	*(UInteger16 *) (buf + 34) = flip16(originTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = flip32(originTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = flip32(originTimestamp->nanosecondsField);
}

/*pack delayResp message into OUT buffer of ptpClock*/
void 
msgPackDelayResp(void *buf, MsgHeader * header, Timestamp * receiveTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x09;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(DELAY_RESP_LENGTH);
	*(UInteger8 *) (buf + 4) = header->domainNumber;
	memset((buf + 8), 0, 8);

	/* Copy correctionField of PdelayReqMessage */
	*(Integer32 *) (buf + 8) = flip32(header->correctionfield.msb);
	*(Integer32 *) (buf + 12) = flip32(header->correctionfield.lsb);

	*(UInteger16 *) (buf + 30) = flip16(header->sequenceId);
	
	*(UInteger8 *) (buf + 32) = 0x03;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = ptpClock->logMinDelayReqInterval;
	/* Table 24 */

	/* Pdelay_resp message */
	*(UInteger16 *) (buf + 34) = 
		flip16(receiveTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = flip32(receiveTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = flip32(receiveTimestamp->nanosecondsField);
	memcpy((buf + 44), header->sourcePortIdentity.clockIdentity, 
	       CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) = 
		flip16(header->sourcePortIdentity.portNumber);
}





/*pack PdelayResp message into OUT buffer of ptpClock*/
void 
msgPackPDelayResp(void *buf, MsgHeader * header, Timestamp * requestReceiptTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x03;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(PDELAY_RESP_LENGTH);
	*(UInteger8 *) (buf + 4) = header->domainNumber;
	memset((buf + 8), 0, 8);


	*(UInteger16 *) (buf + 30) = flip16(header->sequenceId);

	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;
	/* Table 24 */

	/* Pdelay_resp message */
	*(UInteger16 *) (buf + 34) = flip16(requestReceiptTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = flip32(requestReceiptTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = flip32(requestReceiptTimestamp->nanosecondsField);
	memcpy((buf + 44), header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) = flip16(header->sourcePortIdentity.portNumber);

}


/*Unpack delayReq message from IN buffer of ptpClock to msgtmp.req*/
void 
msgUnpackDelayReq(void *buf, MsgDelayReq * delayreq)
{
	delayreq->originTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	delayreq->originTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	delayreq->originTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));

#ifdef PTPD_DBG
	msgDelayReq_display(delayreq);
#endif /* PTPD_DBG */

}


/*Unpack PdelayReq message from IN buffer of ptpClock to msgtmp.req*/
void 
msgUnpackPDelayReq(void *buf, MsgPDelayReq * pdelayreq)
{
	pdelayreq->originTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	pdelayreq->originTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	pdelayreq->originTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));

#ifdef PTPD_DBG
	msgPDelayReq_display(pdelayreq);
#endif /* PTPD_DBG */

}


/*Unpack delayResp message from IN buffer of ptpClock to msgtmp.presp*/
void 
msgUnpackDelayResp(void *buf, MsgDelayResp * resp)
{
	resp->receiveTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	resp->receiveTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	resp->receiveTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));
	memcpy(resp->requestingPortIdentity.clockIdentity, 
	       (buf + 44), CLOCK_IDENTITY_LENGTH);
	resp->requestingPortIdentity.portNumber = 
		flip16(*(UInteger16 *) (buf + 52));

#ifdef PTPD_DBG
	msgDelayResp_display(resp);
#endif /* PTPD_DBG */
}


/*Unpack PdelayResp message from IN buffer of ptpClock to msgtmp.presp*/
void 
msgUnpackPDelayResp(void *buf, MsgPDelayResp * presp)
{
	presp->requestReceiptTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	presp->requestReceiptTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	presp->requestReceiptTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));
	memcpy(presp->requestingPortIdentity.clockIdentity, 
	       (buf + 44), CLOCK_IDENTITY_LENGTH);
	presp->requestingPortIdentity.portNumber = 
		flip16(*(UInteger16 *) (buf + 52));

#ifdef PTPD_DBG
	msgPDelayResp_display(presp);
#endif /* PTPD_DBG */
}

/*pack PdelayRespfollowup message into OUT buffer of ptpClock*/
void 
msgPackPDelayRespFollowUp(void *buf, MsgHeader * header, Timestamp * responseOriginTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x0A;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(PDELAY_RESP_FOLLOW_UP_LENGTH);
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->PdelayReqHeader.sequenceId);
	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;
	/* Table 24 */

	/* Copy correctionField of PdelayReqMessage */
	*(Integer32 *) (buf + 8) = flip32(header->correctionfield.msb);
	*(Integer32 *) (buf + 12) = flip32(header->correctionfield.lsb);

	/* Pdelay_resp_follow_up message */
	*(UInteger16 *) (buf + 34) = 
		flip16(responseOriginTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = 
		flip32(responseOriginTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = 
		flip32(responseOriginTimestamp->nanosecondsField);
	memcpy((buf + 44), header->sourcePortIdentity.clockIdentity, 
	       CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) = 
		flip16(header->sourcePortIdentity.portNumber);
}

/*Unpack PdelayResp message from IN buffer of ptpClock to msgtmp.presp*/
void 
msgUnpackPDelayRespFollowUp(void *buf, MsgPDelayRespFollowUp * prespfollow)
{
	prespfollow->responseOriginTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	prespfollow->responseOriginTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	prespfollow->responseOriginTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));
	memcpy(prespfollow->requestingPortIdentity.clockIdentity, 
	       (buf + 44), CLOCK_IDENTITY_LENGTH);
	prespfollow->requestingPortIdentity.portNumber = 
		flip16(*(UInteger16 *) (buf + 52));
}
