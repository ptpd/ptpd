/*-
 * Copyright (c) 2009-2011 George V. Neville-Neil, Steven Kreuzer, 
 *                         Martin Burnicki, Gael Mace, Alexandre Van Kempen
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
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
 * @file   msg.c
 * @author George Neville-Neil <gnn@neville-neil.com>
 * @date   Tue Jul 20 16:17:05 2010
 * 
 * @brief  Functions to pack and unpack messages.
 * 
 * See spec annex d
 */

#include "../ptpd.h"

#ifdef PTP_EXPERIMENTAL
extern RunTimeOpts rtOpts; 
#endif

/*Unpack Header from IN buffer to msgTmpHeader field */
void 
msgUnpackHeader(Octet * buf, MsgHeader * header)
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
msgPackHeader(Octet * buf, PtpClock * ptpClock)
{
	Nibble transport = 0x80;

	/* (spec annex D) */
	*(UInteger8 *) (buf + 0) = transport;
	*(UInteger4 *) (buf + 1) = ptpClock->versionNumber;
	*(UInteger8 *) (buf + 4) = ptpClock->domainNumber;

	/* TODO: this bit should have been active only for sync and PdelayResp */
	if (ptpClock->twoStepFlag)
		*(UInteger8 *) (buf + 6) = PTP_TWO_STEP;

	memset((buf + 8), 0, 8);
	memcpy((buf + 20), ptpClock->portIdentity.clockIdentity, 
	       CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 28) = flip16(ptpClock->portIdentity.portNumber);
	*(UInteger8 *) (buf + 33) = 0x7F;
	/* Default value(spec Table 24) */
}



/*Pack SYNC message into OUT buffer of ptpClock*/
void 
msgPackSync(Octet * buf, Timestamp * originTimestamp, PtpClock * ptpClock)
{
	msgPackHeader(buf, ptpClock);
	
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
msgUnpackSync(Octet * buf, MsgSync * sync)
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
msgPackAnnounce(Octet * buf, PtpClock * ptpClock)
{
	msgPackHeader(buf, ptpClock);

	
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
msgUnpackAnnounce(Octet * buf, MsgAnnounce * announce)
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
msgPackFollowUp(Octet * buf, Timestamp * preciseOriginTimestamp, PtpClock * ptpClock)
{
	msgPackHeader(buf, ptpClock);
	
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
msgUnpackFollowUp(Octet * buf, MsgFollowUp * follow)
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
msgPackPDelayReq(Octet * buf, Timestamp * originTimestamp, PtpClock * ptpClock)
{
	msgPackHeader(buf, ptpClock);

	
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
msgPackDelayReq(Octet * buf, Timestamp * originTimestamp, PtpClock * ptpClock)
{
	msgPackHeader(buf, ptpClock);
	
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x01;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(DELAY_REQ_LENGTH);

#ifdef PTP_EXPERIMENTAL
	if(rtOpts.do_hybrid_mode)
		*(char *)(buf + 6) |= PTP_UNICAST;
#endif

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
msgPackDelayResp(Octet * buf, MsgHeader * header, Timestamp * receiveTimestamp, PtpClock * ptpClock)
{
	msgPackHeader(buf, ptpClock);
	
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x09;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(DELAY_RESP_LENGTH);
	*(UInteger8 *) (buf + 4) = header->domainNumber;

#ifdef PTP_EXPERIMENTAL
	if(rtOpts.do_hybrid_mode)    
		*(char *)(buf + 6) |= PTP_UNICAST;
#endif

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
msgPackPDelayResp(Octet * buf, MsgHeader * header, Timestamp * requestReceiptTimestamp, PtpClock * ptpClock)
{
	msgPackHeader(buf, ptpClock);
	
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
msgUnpackDelayReq(Octet * buf, MsgDelayReq * delayreq)
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
msgUnpackPDelayReq(Octet * buf, MsgPDelayReq * pdelayreq)
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
msgUnpackDelayResp(Octet * buf, MsgDelayResp * resp)
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
msgUnpackPDelayResp(Octet * buf, MsgPDelayResp * presp)
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
msgPackPDelayRespFollowUp(Octet * buf, MsgHeader * header, Timestamp * responseOriginTimestamp, PtpClock * ptpClock)
{
	msgPackHeader(buf, ptpClock);
	
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
msgUnpackPDelayRespFollowUp(Octet * buf, MsgPDelayRespFollowUp * prespfollow)
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


/** 
 * Dump the most recent packet in the daemon
 * 
 * @param ptpClock The central clock structure
 */
void msgDump(PtpClock *ptpClock)
{

#if defined(freebsd)
	static int dumped = 0;
#endif /* FreeBSD */

	msgDebugHeader(&ptpClock->msgTmpHeader);
	switch (ptpClock->msgTmpHeader.messageType) {
	case SYNC:
		msgDebugSync(&ptpClock->msgTmp.sync);
		break;
    
	case ANNOUNCE:
		msgDebugAnnounce(&ptpClock->msgTmp.announce);
		break;
    
	case FOLLOW_UP:
		msgDebugFollowUp(&ptpClock->msgTmp.follow);
		break;
    
	case DELAY_REQ:
		msgDebugDelayReq(&ptpClock->msgTmp.req);
		break;
    
	case DELAY_RESP:
		msgDebugDelayResp(&ptpClock->msgTmp.resp);
		break;
    
	case MANAGEMENT:
		msgDebugManagement(&ptpClock->msgTmp.manage);
		break;
    
	default:
		NOTIFY("msgDump:unrecognized message\n");
		break;
	}

#if defined(freebsd)
	/* Only dump the first time, after that just do a message. */
	if (dumped != 0) 
		return;

	dumped++;
	NOTIFY("msgDump: core file created.\n");    

	switch(rfork(RFFDG|RFPROC|RFNOWAIT)) {
	case -1:
		NOTIFY("could not fork to core dump! errno: %s", 
		       strerror(errno));
		break;
	case 0:
		abort(); /* Generate a core dump */
	default:
		/* This default intentionally left blank. */
		break;
	}
#endif /* FreeBSD */
}

/** 
 * Dump a PTP message header
 * 
 * @param header a pre-filled msg header structure
 */

void msgDebugHeader(MsgHeader *header)
{
	NOTIFY("msgDebugHeader: messageType %d\n", header->messageType);
	NOTIFY("msgDebugHeader: versionPTP %d\n", header->versionPTP);
	NOTIFY("msgDebugHeader: messageLength %d\n", header->messageLength);
	NOTIFY("msgDebugHeader: domainNumber %d\n", header->domainNumber);
	NOTIFY("msgDebugHeader: flags %02hhx %02hhx\n", 
	       header->flagField[0], header->flagField[1]);
	NOTIFY("msgDebugHeader: correctionfield %d\n", header->correctionfield);
	NOTIFY("msgDebugHeader: sourcePortIdentity.clockIdentity "
	       "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx%02hhx:%02hhx\n",
	       header->sourcePortIdentity.clockIdentity[0], 
	       header->sourcePortIdentity.clockIdentity[1], 
	       header->sourcePortIdentity.clockIdentity[2], 
	       header->sourcePortIdentity.clockIdentity[3], 
	       header->sourcePortIdentity.clockIdentity[4], 
	       header->sourcePortIdentity.clockIdentity[5], 
	       header->sourcePortIdentity.clockIdentity[6], 
	       header->sourcePortIdentity.clockIdentity[7]);
	NOTIFY("msgDebugHeader: sourcePortIdentity.portNumber %d\n",
	       header->sourcePortIdentity.portNumber);
	NOTIFY("msgDebugHeader: sequenceId %d\n", header->sequenceId);
	NOTIFY("msgDebugHeader: controlField %d\n", header->controlField);
	NOTIFY("msgDebugHeader: logMessageIntervale %d\n", 
	       header->logMessageInterval);

}

/** 
 * Dump the contents of a sync packet
 * 
 * @param sync A pre-filled MsgSync structure
 */

void msgDebugSync(MsgSync *sync)
{
	NOTIFY("msgDebugSync: originTimestamp.seconds %u\n",
	       sync->originTimestamp.secondsField);
	NOTIFY("msgDebugSync: originTimestamp.nanoseconds %d\n",
	       sync->originTimestamp.nanosecondsField);
}

/** 
 * Dump the contents of a announce packet
 * 
 * @param sync A pre-filled MsgAnnounce structure
 */

void msgDebugAnnounce(MsgAnnounce *announce)
{
	NOTIFY("msgDebugAnnounce: originTimestamp.seconds %u\n",
	       announce->originTimestamp.secondsField);
	NOTIFY("msgDebugAnnounce: originTimestamp.nanoseconds %d\n",
	       announce->originTimestamp.nanosecondsField);
	NOTIFY("msgDebugAnnounce: currentUTCOffset %d\n", 
	       announce->currentUtcOffset);
	NOTIFY("msgDebugAnnounce: grandmasterPriority1 %d\n", 
	       announce->grandmasterPriority1);
	NOTIFY("msgDebugAnnounce: grandmasterClockQuality.clockClass %d\n",
	       announce->grandmasterClockQuality.clockClass);
	NOTIFY("msgDebugAnnounce: grandmasterClockQuality.clockAccuracy %d\n",
	       announce->grandmasterClockQuality.clockAccuracy);
	NOTIFY("msgDebugAnnounce: "
	       "grandmasterClockQuality.offsetScaledLogVariance %d\n",
	       announce->grandmasterClockQuality.offsetScaledLogVariance);
	NOTIFY("msgDebugAnnounce: grandmasterPriority2 %d\n", 
	       announce->grandmasterPriority2);
	NOTIFY("msgDebugAnnounce: grandmasterClockIdentity "
	       "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx%02hhx:%02hhx\n",
	       announce->grandmasterIdentity[0], 
	       announce->grandmasterIdentity[1], 
	       announce->grandmasterIdentity[2], 
	       announce->grandmasterIdentity[3], 
	       announce->grandmasterIdentity[4], 
	       announce->grandmasterIdentity[5], 
	       announce->grandmasterIdentity[6], 
	       announce->grandmasterIdentity[7]);
	NOTIFY("msgDebugAnnounce: stepsRemoved %d\n", 
	       announce->stepsRemoved);
	NOTIFY("msgDebugAnnounce: timeSource %d\n", 
	       announce->timeSource);
}

/** 
 * NOT IMPLEMENTED
 * 
 * @param req 
 */
void msgDebugDelayReq(MsgDelayReq *req) {}

/** 
 * Dump the contents of a followup packet
 * 
 * @param follow A pre-fille MsgFollowUp structure
 */
void msgDebugFollowUp(MsgFollowUp *follow)
{
	NOTIFY("msgDebugFollowUp: preciseOriginTimestamp.seconds %u\n",
	       follow->preciseOriginTimestamp.secondsField);
	NOTIFY("msgDebugFollowUp: preciseOriginTimestamp.nanoseconds %d\n",
	       follow->preciseOriginTimestamp.nanosecondsField);
}

/** 
 * Dump the contents of a delay response packet
 * 
 * @param resp a pre-filled MsgDelayResp structure
 */
void msgDebugDelayResp(MsgDelayResp *resp)
{
	NOTIFY("msgDebugDelayResp: delayReceiptTimestamp.seconds %u\n",
	       resp->receiveTimestamp.secondsField);
	NOTIFY("msgDebugDelayResp: delayReceiptTimestamp.nanoseconds %d\n",
	       resp->receiveTimestamp.nanosecondsField);
	NOTIFY("msgDebugDelayResp: requestingPortIdentity.clockIdentity "
	       "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx%02hhx:%02hhx\n",
	       resp->requestingPortIdentity.clockIdentity[0], 
	       resp->requestingPortIdentity.clockIdentity[1], 
	       resp->requestingPortIdentity.clockIdentity[2], 
	       resp->requestingPortIdentity.clockIdentity[3], 
	       resp->requestingPortIdentity.clockIdentity[4], 
	       resp->requestingPortIdentity.clockIdentity[5], 
	       resp->requestingPortIdentity.clockIdentity[6], 
	       resp->requestingPortIdentity.clockIdentity[7]);
	NOTIFY("msgDebugDelayResp: requestingPortIdentity.portNumber %d\n",
	       resp->requestingPortIdentity.portNumber);
}

/** 
 * Dump the contents of management packet
 * 
 * @param manage a pre-filled MsgManagement structure
 */

void msgDebugManagement(MsgManagement *manage)
{
	NOTIFY("msgDebugDelayManage: targetPortIdentity.clockIdentity "
	       "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	       manage->targetPortIdentity.clockIdentity[0], 
	       manage->targetPortIdentity.clockIdentity[1], 
	       manage->targetPortIdentity.clockIdentity[2], 
	       manage->targetPortIdentity.clockIdentity[3], 
	       manage->targetPortIdentity.clockIdentity[4], 
	       manage->targetPortIdentity.clockIdentity[5], 
	       manage->targetPortIdentity.clockIdentity[6], 
	       manage->targetPortIdentity.clockIdentity[7]);
	NOTIFY("msgDebugDelayManage: targetPortIdentity.portNumber %d\n",
	       manage->targetPortIdentity.portNumber);
	NOTIFY("msgDebugManagement: startingBoundaryHops %d\n",
	       manage->startingBoundaryHops);
	NOTIFY("msgDebugManagement: boundaryHops %d\n", manage->boundaryHops);
	NOTIFY("msgDebugManagement: actionField %d\n", manage->actionField);
	NOTIFY("msgDebugManagement: tvl %s\n", manage->tlv);
}
