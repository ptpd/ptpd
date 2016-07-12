/*-
 * Copyright (c) 2012-2015 Wojciech Owczarek,
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments.
 * Copyright (c) 2009-2010 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen
 *
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
 * @file   management.c
 * @date   Wed Jul 27 13:07:30 CDT 2011
 *
 * @brief  Routines to handle incoming management messages
 *
 *
 */

#include "ptpd.h"

static void handleMMNullManagement(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMClockDescription(MsgManagement*, MsgManagement*, RunTimeOpts*, PtpClock*);
static void handleMMSlaveOnly(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMUserDescription(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMSaveInNonVolatileStorage(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMResetNonVolatileStorage(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMInitialize(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMDefaultDataSet(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMCurrentDataSet(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMParentDataSet(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMTimePropertiesDataSet(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMPortDataSet(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMPriority1(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMPriority2(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMDomain(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMLogAnnounceInterval(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMAnnounceReceiptTimeout(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMLogSyncInterval(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMVersionNumber(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMEnablePort(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMDisablePort(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMTime(MsgManagement*, MsgManagement*, PtpClock*, const RunTimeOpts*);
static void handleMMClockAccuracy(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMUtcProperties(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMTraceabilityProperties(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMTimescaleProperties(MsgManagement*, MsgManagement*, PtpClock*);

static void handleMMUnicastNegotiationEnable(MsgManagement*, MsgManagement*, PtpClock*, RunTimeOpts*);
static void handleMMDelayMechanism(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMLogMinPdelayReqInterval(MsgManagement*, MsgManagement*, PtpClock*);
static void handleMMErrorStatus(MsgManagement*);
static void handleErrorManagementMessage(MsgManagement *incoming, MsgManagement *outgoing,
                                PtpClock *ptpClock, Enumeration16 mgmtId,
                                Enumeration16 errorId);
#if 0
static void issueManagement(MsgHeader*,MsgManagement*,const RunTimeOpts*,PtpClock*);
#endif
static void issueManagementRespOrAck(MsgManagement*, Integer32, const RunTimeOpts*,PtpClock*);
static void issueManagementErrorStatus(MsgManagement*, Integer32, const RunTimeOpts*,PtpClock*);

void
handleManagement(MsgHeader *header,
		 Boolean isFromSelf, Integer32 sourceAddress, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBGV("Management message received : \n");
	Integer32 dst;

	int tlvOffset = 0;
	int tlvFound = 0;

	MsgManagement *mgmtMsg = &ptpClock->msgTmp.manage;

	/*
	 * If request was unicast, reply to source, always, even if we ourselves are running multicast.
	 * This is not in line with the standard, but allows us to always reply to unicast management messages.
	 * This is a good thing - this allows us to always be able to monitor the node using unicast management messages.
	 */

	if ( (header->flagField0 & PTP_UNICAST) == PTP_UNICAST) {
		dst = sourceAddress;
	} else {
		dst = 0;
	}

	if (isFromSelf) {
		DBGV("handleManagement: Ignore message from self \n");
		return;
	}

	if(!rtOpts->managementEnabled) {
		DBGV("Dropping management message - management message support disabled");
		ptpClock->counters.discardedMessages++;
		return;
	}

	/* loop over all supported TLVs as if they came in separate messages */
	while(msgUnpackManagement(ptpClock->msgIbuf,mgmtMsg, header, ptpClock, tlvOffset)) {

	if(mgmtMsg->tlv == NULL) {

	    	if(tlvFound==0) {
		    DBGV("handleManagement: No TLVs in message\n");
		    ptpClock->counters.messageFormatErrors++;
		} else {
		    DBGV("handleManagement: No more TLVs\n");
		}
		return;
	}

	tlvFound++;

	/* accept the message if directed either to us or to all-ones */
        if(!acceptPortIdentity(ptpClock->portDS.portIdentity, mgmtMsg->targetPortIdentity))
        {
                DBGV("handleManagement: The management message was not accepted");
		ptpClock->counters.discardedMessages++;
                goto end;
        }

	if(!rtOpts->managementSetEnable &&
	    (mgmtMsg->actionField == SET ||
	    mgmtMsg->actionField == COMMAND)) {
		DBGV("Dropping SET/COMMAND management message - read-only mode enabled");
		ptpClock->counters.discardedMessages++;
		goto end;
	}

	/* is this an error status management TLV? */
	if(mgmtMsg->tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS) {
		DBGV("handleManagement: Error Status TLV\n");
		unpackMMErrorStatus(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock);
		handleMMErrorStatus(mgmtMsg);
		ptpClock->counters.managementMessagesReceived++;
		goto end;
	} else if (mgmtMsg->tlv->tlvType != TLV_MANAGEMENT) {
		/* do nothing, implemention specific handling */
		DBGV("handleManagement: Currently unsupported management TLV type\n");
		ptpClock->counters.discardedMessages++;
		goto end;
	}

	/* if this is a SET, there is potential for applying new config */
	if (mgmtMsg->actionField & (SET | COMMAND)) {
	    ptpClock->managementConfig = dictionary_new(0);
	    dictionary_merge(rtOpts->currentConfig, ptpClock->managementConfig, 1, 0, NULL);
	}

	switch(mgmtMsg->tlv->managementId)
	{
	case MM_NULL_MANAGEMENT:
		DBGV("handleManagement: Null Management\n");
		handleMMNullManagement(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_CLOCK_DESCRIPTION:
		DBGV("handleManagement: Clock Description\n");
		if(mgmtMsg->actionField != GET && !unpackMMClockDescription(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
		handleMMClockDescription(mgmtMsg, &ptpClock->outgoingManageTmp, rtOpts, ptpClock);
		break;
	case MM_USER_DESCRIPTION:
		DBGV("handleManagement: User Description\n");
		if(mgmtMsg->actionField != GET && !unpackMMUserDescription(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
		handleMMUserDescription(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_SAVE_IN_NON_VOLATILE_STORAGE:
		DBGV("handleManagement: Save In Non-Volatile Storage\n");
		handleMMSaveInNonVolatileStorage(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_RESET_NON_VOLATILE_STORAGE:
		DBGV("handleManagement: Reset Non-Volatile Storage\n");
		handleMMResetNonVolatileStorage(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_INITIALIZE:
		DBGV("handleManagement: Initialize\n");
		if(mgmtMsg->actionField != GET && !unpackMMInitialize(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
		handleMMInitialize(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_DEFAULT_DATA_SET:
		DBGV("handleManagement: Default Data Set\n");
		if(mgmtMsg->actionField != GET && !unpackMMDefaultDataSet(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
		handleMMDefaultDataSet(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
		break;
	case MM_CURRENT_DATA_SET:
		DBGV("handleManagement: Current Data Set\n");
		if(mgmtMsg->actionField != GET && !unpackMMCurrentDataSet(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
		handleMMCurrentDataSet(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
		break;
        case MM_PARENT_DATA_SET:
                DBGV("handleManagement: Parent Data Set\n");
                if(mgmtMsg->actionField != GET && !unpackMMParentDataSet(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMParentDataSet(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_TIME_PROPERTIES_DATA_SET:
                DBGV("handleManagement: TimeProperties Data Set\n");
                if(mgmtMsg->actionField != GET && !unpackMMTimePropertiesDataSet(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMTimePropertiesDataSet(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_PORT_DATA_SET:
                DBGV("handleManagement: Port Data Set\n");
                if(mgmtMsg->actionField != GET && !unpackMMPortDataSet(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMPortDataSet(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_PRIORITY1:
                DBGV("handleManagement: Priority1\n");
                if(mgmtMsg->actionField != GET && !unpackMMPriority1(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMPriority1(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_PRIORITY2:
                DBGV("handleManagement: Priority2\n");
                if(mgmtMsg->actionField != GET && !unpackMMPriority2(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMPriority2(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_DOMAIN:
                DBGV("handleManagement: Domain\n");
                if(mgmtMsg->actionField != GET && !unpackMMDomain(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMDomain(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
	case MM_SLAVE_ONLY:
		DBGV("handleManagement: Slave Only\n");
		if(mgmtMsg->actionField != GET && !unpackMMSlaveOnly(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
		handleMMSlaveOnly(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
		break;
        case MM_LOG_ANNOUNCE_INTERVAL:
                DBGV("handleManagement: Log Announce Interval\n");
                if(mgmtMsg->actionField != GET && !unpackMMLogAnnounceInterval(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMLogAnnounceInterval(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_ANNOUNCE_RECEIPT_TIMEOUT:
                DBGV("handleManagement: Announce Receipt Timeout\n");
                if(mgmtMsg->actionField != GET && !unpackMMAnnounceReceiptTimeout(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMAnnounceReceiptTimeout(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_LOG_SYNC_INTERVAL:
                DBGV("handleManagement: Log Sync Interval\n");
                if(mgmtMsg->actionField != GET && !unpackMMLogSyncInterval(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMLogSyncInterval(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_VERSION_NUMBER:
                DBGV("handleManagement: Version Number\n");
                if(mgmtMsg->actionField != GET && !unpackMMVersionNumber(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMVersionNumber(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_ENABLE_PORT:
                DBGV("handleManagement: Enable Port\n");
                handleMMEnablePort(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_DISABLE_PORT:
                DBGV("handleManagement: Disable Port\n");
                handleMMDisablePort(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_TIME:
                DBGV("handleManagement: Time\n");
                if(mgmtMsg->actionField != GET && !unpackMMTime(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMTime(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock, rtOpts);
                break;
        case MM_CLOCK_ACCURACY:
                DBGV("handleManagement: Clock Accuracy\n");
                if(mgmtMsg->actionField != GET && !unpackMMClockAccuracy(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMClockAccuracy(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_UTC_PROPERTIES:
                DBGV("handleManagement: Utc Properties\n");
                if(mgmtMsg->actionField != GET && !unpackMMUtcProperties(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMUtcProperties(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_TRACEABILITY_PROPERTIES:
                DBGV("handleManagement: Traceability Properties\n");
                if(mgmtMsg->actionField != GET && !unpackMMTraceabilityProperties(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMTraceabilityProperties(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_TIMESCALE_PROPERTIES:
                DBGV("handleManagement: Timescale Properties\n");
                if(mgmtMsg->actionField != GET && !unpackMMTimescaleProperties(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMTimescaleProperties(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_UNICAST_NEGOTIATION_ENABLE:
                DBGV("handleManagement: Unicast Negotiation Enable\n");
                if(mgmtMsg->actionField != GET && !unpackMMUnicastNegotiationEnable(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMUnicastNegotiationEnable(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock, rtOpts);
                break;
        case MM_DELAY_MECHANISM:
                DBGV("handleManagement: Delay Mechanism\n");
                if(mgmtMsg->actionField != GET && !unpackMMDelayMechanism(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMDelayMechanism(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
        case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
                DBGV("handleManagement: Log Min Pdelay Req Interval\n");
                if(mgmtMsg->actionField != GET && !unpackMMLogMinPdelayReqInterval(ptpClock->msgIbuf, tlvOffset, mgmtMsg, ptpClock)) {
			DBG("handleManagement: (bufGuard) error while unpacking management %02x\n", mgmtMsg->tlv->managementId);
			ptpClock->counters.messageFormatErrors++;
			goto end;
		}
                handleMMLogMinPdelayReqInterval(mgmtMsg, &ptpClock->outgoingManageTmp, ptpClock);
                break;
	case MM_FAULT_LOG:
	case MM_FAULT_LOG_RESET:
	case MM_PATH_TRACE_LIST:
	case MM_PATH_TRACE_ENABLE:
	case MM_GRANDMASTER_CLUSTER_TABLE:
	case MM_UNICAST_MASTER_TABLE:
	case MM_UNICAST_MASTER_MAX_TABLE_SIZE:
	case MM_ACCEPTABLE_MASTER_TABLE:
	case MM_ACCEPTABLE_MASTER_TABLE_ENABLED:
	case MM_ACCEPTABLE_MASTER_MAX_TABLE_SIZE:
	case MM_ALTERNATE_MASTER:
	case MM_ALTERNATE_TIME_OFFSET_ENABLE:
	case MM_ALTERNATE_TIME_OFFSET_NAME:
	case MM_ALTERNATE_TIME_OFFSET_MAX_KEY:
	case MM_ALTERNATE_TIME_OFFSET_PROPERTIES:
	case MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
	case MM_TRANSPARENT_CLOCK_PORT_DATA_SET:
	case MM_PRIMARY_DOMAIN:
		DBGV("handleManagement: Currently unsupported managementTLV %d\n",
				mgmtMsg->tlv->managementId);
		handleErrorManagementMessage(mgmtMsg, &ptpClock->outgoingManageTmp,
			ptpClock, mgmtMsg->tlv->managementId,
			NOT_SUPPORTED);
		ptpClock->counters.discardedMessages++;
		break;
	default:
		DBGV("handleManagement: Unknown managementTLV %d\n",
				mgmtMsg->tlv->managementId);
		handleErrorManagementMessage(mgmtMsg, &ptpClock->outgoingManageTmp,
			ptpClock, mgmtMsg->tlv->managementId,
			NO_SUCH_ID);
		ptpClock->counters.discardedMessages++;
	}
	/* send management message response or acknowledge */
	if(ptpClock->outgoingManageTmp.tlv->tlvType == TLV_MANAGEMENT) {
		if(ptpClock->outgoingManageTmp.actionField == RESPONSE ||
				ptpClock->outgoingManageTmp.actionField == ACKNOWLEDGE) {
			issueManagementRespOrAck(&ptpClock->outgoingManageTmp, dst, rtOpts, ptpClock);
		}
	} else if(ptpClock->outgoingManageTmp.tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS) {
		issueManagementErrorStatus(&ptpClock->outgoingManageTmp, dst, rtOpts, ptpClock);
	}

	end:
	/* Movin' on up! */
	tlvOffset += TL_LENGTH + mgmtMsg->tlv->lengthField;
	/* cleanup msgTmp managementTLV */
	freeManagementTLV(mgmtMsg);
	/* cleanup outgoing managementTLV */
	freeManagementTLV(&ptpClock->outgoingManageTmp);

	}

	if(ptpClock->managementConfig != NULL) {
	    NOTICE("SET / COMMAND management message received - looking for configuration changes\n");
	    applyConfig(ptpClock->managementConfig, rtOpts, ptpClock);
	    dictionary_del(&ptpClock->managementConfig);
	}

    	if(!tlvFound) {
	    DBGV("handleManagement: No TLVs in message\n");
	    ptpClock->counters.messageFormatErrors++;
	} else {
	    ptpClock->counters.managementMessagesReceived++;
	    DBGV("handleManagement: No more TLVs\n");
	}


}


/**\brief Initialize outgoing management message fields*/
void initOutgoingMsgManagement(MsgManagement* incoming, MsgManagement* outgoing, PtpClock *ptpClock)
{
	/* set header fields */
	outgoing->header.transportSpecific = ptpClock->portDS.transportSpecific;
	outgoing->header.messageType = MANAGEMENT;
        outgoing->header.versionPTP = ptpClock->portDS.versionNumber;
	outgoing->header.domainNumber = ptpClock->defaultDS.domainNumber;
        /* set header flagField to zero for management messages, Spec 13.3.2.6 */
        outgoing->header.flagField0 = 0x00;
        outgoing->header.flagField1 = 0x00;
        outgoing->header.correctionField.msb = 0;
        outgoing->header.correctionField.lsb = 0;
	copyPortIdentity(&outgoing->header.sourcePortIdentity, &ptpClock->portDS.portIdentity);
	outgoing->header.sequenceId = incoming->header.sequenceId;
	outgoing->header.controlField = 0x4; /* deprecrated for ptp version 2 */
	outgoing->header.logMessageInterval = 0x7F;

	/* set management message fields */
	copyPortIdentity( &outgoing->targetPortIdentity, &incoming->header.sourcePortIdentity );
	outgoing->startingBoundaryHops = incoming->startingBoundaryHops - incoming->boundaryHops;
	outgoing->boundaryHops = outgoing->startingBoundaryHops;
        outgoing->actionField = 0; /* set default action, avoid uninitialized value */

	/* init managementTLV */
	XMALLOC(outgoing->tlv, sizeof(ManagementTLV));
	outgoing->tlv->dataField = NULL;
	outgoing->tlv->lengthField = 0;
}

/**\brief Handle incoming NULL_MANAGEMENT message*/
void handleMMNullManagement(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received NULL_MANAGEMENT message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_NULL_MANAGEMENT;

	switch(incoming->actionField)
	{
	case GET:
		outgoing->actionField = RESPONSE;
		DBGV(" GET mgmt msg\n");
		break;
	case SET:
		outgoing->actionField = RESPONSE;
		DBGV(" SET mgmt msg\n");
		break;
	case COMMAND:
		outgoing->actionField = ACKNOWLEDGE;
		DBGV(" COMMAND mgmt msg\n");
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_NULL_MANAGEMENT,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming CLOCK_DESCRIPTION management message*/
void handleMMClockDescription(MsgManagement* incoming, MsgManagement* outgoing, RunTimeOpts *rtOpts, PtpClock* ptpClock)
{
	DBGV("received CLOCK_DESCRIPTION management message \n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_CLOCK_DESCRIPTION;

	MMClockDescription *data = NULL;
	switch(incoming->actionField)
	{
	case GET:
		DBGV(" GET action \n");
		/* Table 38 */
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof( MMClockDescription));
		data = (MMClockDescription*)outgoing->tlv->dataField;
		memset(data, 0, sizeof( MMClockDescription));
		/* GET actions */
		/* this is an ordnary node, clockType field bit 0 is one */
		data->clockType0 = 0x80;
		data->clockType1 = 0x00;
		/* physical layer protocol */
                data->physicalLayerProtocol.lengthField = sizeof(PROTOCOL) - 1;
                XMALLOC(data->physicalLayerProtocol.textField,
                                data->physicalLayerProtocol.lengthField);
                memcpy(data->physicalLayerProtocol.textField,
                        &PROTOCOL,
                        data->physicalLayerProtocol.lengthField);
		/* physical address */
                data->physicalAddress.addressLength = PTP_UUID_LENGTH;
                XMALLOC(data->physicalAddress.addressField, PTP_UUID_LENGTH);
                memcpy(data->physicalAddress.addressField,
                        ptpClock->netPath.interfaceID,
                        PTP_UUID_LENGTH);
		/* protocol address */
                data->protocolAddress.addressLength = 4;
                data->protocolAddress.networkProtocol = 1;
                XMALLOC(data->protocolAddress.addressField,
                        data->protocolAddress.addressLength);
                memcpy(data->protocolAddress.addressField,
                        &ptpClock->netPath.interfaceAddr.s_addr,
                        data->protocolAddress.addressLength);
		/* manufacturerIdentity OUI */
		data->manufacturerIdentity0 = MANUFACTURER_ID_OUI0;
		data->manufacturerIdentity1 = MANUFACTURER_ID_OUI1;
		data->manufacturerIdentity2 = MANUFACTURER_ID_OUI2;
		/* reserved */
		data->reserved = 0;
		/* product description */
		tmpsnprintf(tmpStr, 64, PRODUCT_DESCRIPTION, rtOpts->productDescription);
                data->productDescription.lengthField = strlen(tmpStr);
                XMALLOC(data->productDescription.textField,
                                        data->productDescription.lengthField);
                memcpy(data->productDescription.textField,
                        tmpStr,
                        data->productDescription.lengthField);
		/* revision data */
                data->revisionData.lengthField = sizeof(REVISION) - 1;
                XMALLOC(data->revisionData.textField,
                                        data->revisionData.lengthField);
                memcpy(data->revisionData.textField,
                        &REVISION,
                        data->revisionData.lengthField);
		/* user description */
                data->userDescription.lengthField = strlen(ptpClock->userDescription);
                XMALLOC(data->userDescription.textField,
                                        data->userDescription.lengthField);
                memcpy(data->userDescription.textField,
                        ptpClock->userDescription,
                        data->userDescription.lengthField);
		/* profile identity is zero unless specific profile is implemented */
		memcpy(&data->profileIdentity0, &ptpClock->profileIdentity, 6);
		break;
	case RESPONSE:
		DBGV(" RESPONSE action \n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_CLOCK_DESCRIPTION,
			NOT_SUPPORTED);
	}
}

/**\brief Handle incoming SLAVE_ONLY management message type*/
void handleMMSlaveOnly(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received SLAVE_ONLY management message \n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_SLAVE_ONLY;

	MMSlaveOnly* data = NULL;
	switch (incoming->actionField)
	{
	case SET:
		DBGV(" SET action \n");
		data = (MMSlaveOnly*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->defaultDS.slaveOnly = data->so;
		ptpClock->record_update = TRUE;
		setConfig(ptpClock->managementConfig, "ptpengine:slave_only", ptpClock->defaultDS.slaveOnly ? "Y" : "N");
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action \n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMSlaveOnly));
		data = (MMSlaveOnly*)outgoing->tlv->dataField;
		/* GET actions */
		data->so = ptpClock->defaultDS.slaveOnly;
		data->reserved = 0x0;
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_SLAVE_ONLY,
			NOT_SUPPORTED);
	}
}

/**\brief Handle incoming USER_DESCRIPTION management message type*/
void handleMMUserDescription(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received USER_DESCRIPTION message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_USER_DESCRIPTION;

	MMUserDescription* data = NULL;
	switch(incoming->actionField)
	{
	case SET:
		DBGV(" SET action \n");
		data = (MMUserDescription*)incoming->tlv->dataField;
		UInteger8 userDescriptionLength = data->userDescription.lengthField;
		if(userDescriptionLength <= USER_DESCRIPTION_MAX) {
			memset(ptpClock->userDescription, 0, sizeof(ptpClock->userDescription));
			memcpy(ptpClock->userDescription, data->userDescription.textField,
					userDescriptionLength);
			/* add null-terminator to make use of C string function strlen later */
			ptpClock->userDescription[userDescriptionLength] = '\0';
			tmpsnprintf(ud, data->userDescription.lengthField+1, "%s", (char*)data->userDescription.textField);
			setConfig(ptpClock->managementConfig, "ptpengine:port_description", ud);
		} else {
			WARNING("management user description exceeds specification length \n");
		}
		
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action \n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof( MMUserDescription));
		data = (MMUserDescription*)outgoing->tlv->dataField;
		memset(data, 0, sizeof(MMUserDescription));
		/* GET actions */
                data->userDescription.lengthField = strlen(ptpClock->userDescription);
                XMALLOC(data->userDescription.textField,
                                        data->userDescription.lengthField);
                memcpy(data->userDescription.textField,
                        ptpClock->userDescription,
                        data->userDescription.lengthField);
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_USER_DESCRIPTION,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming SAVE_IN_NON_VOLATILE_STORAGE management message type*/
void handleMMSaveInNonVolatileStorage(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received SAVE_IN_NON_VOLATILE_STORAGE message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_SAVE_IN_NON_VOLATILE_STORAGE;

	switch( incoming->actionField )
	{
	case COMMAND:
		/* issue a NOT_SUPPORTED error management message, intentionally fall through */
	case ACKNOWLEDGE:
		/* issue a NOT_SUPPORTED error management message, intentionally fall through */
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_SAVE_IN_NON_VOLATILE_STORAGE,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming RESET_NON_VOLATILE_STORAGE management message type*/
void handleMMResetNonVolatileStorage(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received RESET_NON_VOLATILE_STORAGE message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_RESET_NON_VOLATILE_STORAGE;

	switch( incoming->actionField )
	{
	case COMMAND:
		/* issue a NOT_SUPPORTED error management message, intentionally fall through */
	case ACKNOWLEDGE:
		/* issue a NOT_SUPPORTED error management message, intentionally fall through */
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_RESET_NON_VOLATILE_STORAGE,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming INITIALIZE management message type*/
void handleMMInitialize(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received INITIALIZE message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_INITIALIZE;

	MMInitialize* incomingData = NULL;
	MMInitialize* outgoingData = NULL;
	switch( incoming->actionField )
	{
	case COMMAND:
		DBGV(" COMMAND action\n");
		outgoing->actionField = ACKNOWLEDGE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMInitialize));
		incomingData = (MMInitialize*)incoming->tlv->dataField;
		outgoingData = (MMInitialize*)outgoing->tlv->dataField;
		/* Table 45 - INITIALIZATION_KEY enumeration */
		switch( incomingData->initializeKey )
		{
		case INITIALIZE_EVENT:
			/* cause INITIALIZE event */
			setPortState(ptpClock, PTP_INITIALIZING);
			break;
		default:
			/* do nothing, implementation specific */
			DBGV("initializeKey != 0, do nothing\n");
		}
		outgoingData->initializeKey = incomingData->initializeKey;
		break;
	case ACKNOWLEDGE:
		DBGV(" ACKNOWLEDGE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_INITIALIZE,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming DEFAULT_DATA_SET management message type*/
void handleMMDefaultDataSet(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received DEFAULT_DATA_SET message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_DEFAULT_DATA_SET;

	MMDefaultDataSet* data = NULL;
	switch( incoming->actionField )
	{
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMDefaultDataSet));
		data = (MMDefaultDataSet*)outgoing->tlv->dataField;
		/* GET actions */
		/* get bit and align for slave only */
		Octet so = ptpClock->defaultDS.slaveOnly << 1;
		/* get bit and align by shifting right 1 since TWO_STEP_FLAG is either 0b00 or 0b10 */
		Octet tsc = ptpClock->defaultDS.twoStepFlag >> 1;
		data->so_tsc = so | tsc;
		data->reserved0 = 0x0;
		data->numberPorts = ptpClock->defaultDS.numberPorts;
		data->priority1 = ptpClock->defaultDS.priority1;
		data->clockQuality.clockAccuracy = ptpClock->defaultDS.clockQuality.clockAccuracy;
		data->clockQuality.clockClass = ptpClock->defaultDS.clockQuality.clockClass;
		data->clockQuality.offsetScaledLogVariance =
				ptpClock->defaultDS.clockQuality.offsetScaledLogVariance;
		data->priority2 = ptpClock->defaultDS.priority2;
		/* copy clockIdentity */
		copyClockIdentity(data->clockIdentity, ptpClock->defaultDS.clockIdentity);
		data->domainNumber = ptpClock->defaultDS.domainNumber;
		data->reserved1 = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_DEFAULT_DATA_SET,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming CURRENT_DATA_SET management message type*/
void handleMMCurrentDataSet(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received CURRENT_DATA_SET message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_CURRENT_DATA_SET;

	MMCurrentDataSet *data = NULL;
	switch( incoming->actionField )
	{
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof( MMCurrentDataSet));
		data = (MMCurrentDataSet*)outgoing->tlv->dataField;
		/* GET actions */
		data->stepsRemoved = ptpClock->currentDS.stepsRemoved;
		TimeInterval oFM;
		oFM.scaledNanoseconds.lsb = 0;
		oFM.scaledNanoseconds.msb = 0;
		internalTime_to_integer64(ptpClock->currentDS.offsetFromMaster, &oFM.scaledNanoseconds);
		data->offsetFromMaster.scaledNanoseconds.lsb = oFM.scaledNanoseconds.lsb;
		data->offsetFromMaster.scaledNanoseconds.msb = oFM.scaledNanoseconds.msb;
		TimeInterval mPD;
		mPD.scaledNanoseconds.lsb = 0;
		mPD.scaledNanoseconds.msb = 0;
		internalTime_to_integer64(ptpClock->currentDS.meanPathDelay, &mPD.scaledNanoseconds);
		data->meanPathDelay.scaledNanoseconds.lsb = mPD.scaledNanoseconds.lsb;
		data->meanPathDelay.scaledNanoseconds.msb = mPD.scaledNanoseconds.msb;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_CURRENT_DATA_SET,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming PARENT_DATA_SET management message type*/
void handleMMParentDataSet(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received PARENT_DATA_SET message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_PARENT_DATA_SET;

	MMParentDataSet *data = NULL;
	switch( incoming->actionField )
	{
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMParentDataSet));
		data = (MMParentDataSet*)outgoing->tlv->dataField;
		/* GET actions */
		copyPortIdentity(&data->parentPortIdentity, &ptpClock->parentDS.parentPortIdentity);
		data->PS = ptpClock->parentDS.parentStats;
		data->reserved = 0;
		data->observedParentOffsetScaledLogVariance =
				ptpClock->parentDS.observedParentOffsetScaledLogVariance;
		data->observedParentClockPhaseChangeRate =
				ptpClock->parentDS.observedParentClockPhaseChangeRate;
		data->grandmasterPriority1 = ptpClock->parentDS.grandmasterPriority1;
		data->grandmasterClockQuality.clockAccuracy =
				ptpClock->parentDS.grandmasterClockQuality.clockAccuracy;
		data->grandmasterClockQuality.clockClass =
				ptpClock->parentDS.grandmasterClockQuality.clockClass;
		data->grandmasterClockQuality.offsetScaledLogVariance =
				ptpClock->parentDS.grandmasterClockQuality.offsetScaledLogVariance;
		data->grandmasterPriority2 = ptpClock->parentDS.grandmasterPriority2;
		copyClockIdentity(data->grandmasterIdentity, ptpClock->parentDS.grandmasterIdentity);
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_PARENT_DATA_SET,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming PROPERTIES_DATA_SET management message type*/
void handleMMTimePropertiesDataSet(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received TIME_PROPERTIES message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_TIME_PROPERTIES_DATA_SET;

	MMTimePropertiesDataSet *data = NULL;
	switch( incoming->actionField )
	{
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMTimePropertiesDataSet));
		data = (MMTimePropertiesDataSet*)outgoing->tlv->dataField;
		/* GET actions */
		data->currentUtcOffset = ptpClock->timePropertiesDS.currentUtcOffset;
		Octet ftra = SET_FIELD(ptpClock->timePropertiesDS.frequencyTraceable, FTRA);
		Octet ttra = SET_FIELD(ptpClock->timePropertiesDS.timeTraceable, TTRA);
		Octet ptp = SET_FIELD(ptpClock->timePropertiesDS.ptpTimescale, PTPT);
		Octet utcv = SET_FIELD(ptpClock->timePropertiesDS.currentUtcOffsetValid, UTCV);
		Octet li59 = SET_FIELD(ptpClock->timePropertiesDS.leap59, LI59);
		Octet li61 = SET_FIELD(ptpClock->timePropertiesDS.leap61, LI61);
		data->ftra_ttra_ptp_utcv_li59_li61 = ftra | ttra | ptp | utcv | li59 | li61;
		data->timeSource = ptpClock->timePropertiesDS.timeSource;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_TIME_PROPERTIES_DATA_SET,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming PORT_DATA_SET management message type*/
void handleMMPortDataSet(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received PORT_DATA_SET message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_PORT_DATA_SET;

	MMPortDataSet *data = NULL;
	switch( incoming->actionField )
	{
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMPortDataSet));
		data = (MMPortDataSet*)outgoing->tlv->dataField;
		copyPortIdentity(&data->portIdentity, &ptpClock->portDS.portIdentity);
		data->portState = ptpClock->portDS.portState;
		data->logMinDelayReqInterval = ptpClock->portDS.logMinDelayReqInterval;
		TimeInterval pMPD;
		pMPD.scaledNanoseconds.lsb = 0;
		pMPD.scaledNanoseconds.msb = 0;
		internalTime_to_integer64(ptpClock->portDS.peerMeanPathDelay, &pMPD.scaledNanoseconds);
		data->peerMeanPathDelay.scaledNanoseconds.lsb = pMPD.scaledNanoseconds.lsb;
		data->peerMeanPathDelay.scaledNanoseconds.msb = pMPD.scaledNanoseconds.msb;
		data->logAnnounceInterval = ptpClock->portDS.logAnnounceInterval;
		data->announceReceiptTimeout = ptpClock->portDS.announceReceiptTimeout;
		data->logSyncInterval = ptpClock->portDS.logSyncInterval;
		data->delayMechanism  = ptpClock->portDS.delayMechanism;
		data->logMinPdelayReqInterval = ptpClock->portDS.logMinPdelayReqInterval;
		data->reserved = 0;
		data->versionNumber = ptpClock->portDS.versionNumber;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action \n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_PORT_DATA_SET,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming PRIORITY1 management message type*/
void handleMMPriority1(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received PRIORITY1 message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_PRIORITY1;

	MMPriority1* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMPriority1*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->defaultDS.priority1 = data->priority1;
		tmpsnprintf(tmpStr, 4, "%d", data->priority1);
		setConfig(ptpClock->managementConfig, "ptpengine:priority1", tmpStr);
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMPriority1));
		data = (MMPriority1*)outgoing->tlv->dataField;
		/* GET actions */
		data->priority1 = ptpClock->defaultDS.priority1;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_PRIORITY1,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming PRIORITY2 management message type*/
void handleMMPriority2(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received PRIORITY2 message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_PRIORITY2;

	MMPriority2* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMPriority2*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->defaultDS.priority2 = data->priority2;
		tmpsnprintf(tmpStr, 4, "%d", data->priority2);
		setConfig(ptpClock->managementConfig, "ptpengine:priority2", tmpStr);
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMPriority2));
		data = (MMPriority2*)outgoing->tlv->dataField;
		/* GET actions */
		data->priority2 = ptpClock->defaultDS.priority2;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_PRIORITY2,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming DOMAIN management message type*/
void handleMMDomain(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received DOMAIN message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_DOMAIN;

	MMDomain* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMDomain*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->defaultDS.domainNumber = data->domainNumber;
		tmpsnprintf(tmpStr, 4, "%d", data->domainNumber);
		setConfig(ptpClock->managementConfig, "ptpengine:domain", tmpStr);
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMDomain));
		data = (MMDomain*)outgoing->tlv->dataField;
		/* GET actions */
		data->domainNumber = ptpClock->defaultDS.domainNumber;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_DOMAIN,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming LOG_ANNOUNCE_INTERVAL management message type*/
void handleMMLogAnnounceInterval(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received LOG_ANNOUNCE_INTERVAL message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_LOG_ANNOUNCE_INTERVAL;

	MMLogAnnounceInterval* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMLogAnnounceInterval*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->portDS.logAnnounceInterval = data->logAnnounceInterval;
		tmpsnprintf(tmpStr, 4, "%d", data->logAnnounceInterval);
		setConfig(ptpClock->managementConfig, "ptpengine:log_announce_interval", tmpStr);
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMLogAnnounceInterval));
		data = (MMLogAnnounceInterval*)outgoing->tlv->dataField;
		/* GET actions */
		data->logAnnounceInterval = ptpClock->portDS.logAnnounceInterval;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_LOG_ANNOUNCE_INTERVAL,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming ANNOUNCE_RECEIPT_TIMEOUT management message type*/
void handleMMAnnounceReceiptTimeout(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received ANNOUNCE_RECEIPT_TIMEOUT message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_ANNOUNCE_RECEIPT_TIMEOUT;

	MMAnnounceReceiptTimeout* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMAnnounceReceiptTimeout*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->portDS.announceReceiptTimeout = data->announceReceiptTimeout;
		tmpsnprintf(tmpStr, 4, "%d", data->announceReceiptTimeout);
		setConfig(ptpClock->managementConfig, "ptpengine:announce_receipt_timeout", tmpStr);
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMAnnounceReceiptTimeout));
		data = (MMAnnounceReceiptTimeout*)outgoing->tlv->dataField;
		/* GET actions */
		data->announceReceiptTimeout = ptpClock->portDS.announceReceiptTimeout;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_ANNOUNCE_RECEIPT_TIMEOUT,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming LOG_SYNC_INTERVAL management message type*/
void handleMMLogSyncInterval(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received LOG_SYNC_INTERVAL message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_LOG_SYNC_INTERVAL;

	MMLogSyncInterval* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMLogSyncInterval*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->portDS.logSyncInterval = data->logSyncInterval;
		tmpsnprintf(tmpStr, 4, "%d", data->logSyncInterval);
		setConfig(ptpClock->managementConfig, "ptpengine:log_sync_interval", tmpStr);
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMLogSyncInterval));
		data = (MMLogSyncInterval*)outgoing->tlv->dataField;
		/* GET actions */
		data->logSyncInterval = ptpClock->portDS.logSyncInterval;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_LOG_SYNC_INTERVAL,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming VERSION_NUMBER management message type*/
void handleMMVersionNumber(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received VERSION_NUMBER message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_VERSION_NUMBER;

	MMVersionNumber* data = NULL;
	switch( incoming->actionField )
	{
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMVersionNumber));
		data = (MMVersionNumber*)outgoing->tlv->dataField;
		/* GET actions */
		data->reserved0 = 0x0;
		data->versionNumber = ptpClock->portDS.versionNumber;
		data->reserved1 = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	/* only version 2. go away. */
	case SET:
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_VERSION_NUMBER,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming ENABLE_PORT management message type*/
void handleMMEnablePort(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received ENABLE_PORT message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_ENABLE_PORT;

	switch( incoming->actionField )
	{
	case COMMAND:
		DBGV(" COMMAND action\n");
		outgoing->actionField = ACKNOWLEDGE;
		ptpClock->disabled = FALSE;
		setConfig(ptpClock->managementConfig, "ptpengine:disabled", "N");
		break;
	case ACKNOWLEDGE:
		DBGV(" ACKNOWLEDGE action\n");
		/* TODO: implementation specific */
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_ENABLE_PORT,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming DISABLE_PORT management message type*/
void handleMMDisablePort(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received DISABLE_PORT message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_DISABLE_PORT;

	switch( incoming->actionField )
	{
	case COMMAND:
		DBGV(" COMMAND action\n");
		outgoing->actionField = ACKNOWLEDGE;
		/* the state machine needs to know that we are not yet disabled but want to be */
		/* ptpClock->disabled = TRUE; */
		setConfig(ptpClock->managementConfig, "ptpengine:disabled", "Y");
		break;
	case ACKNOWLEDGE:
		DBGV(" ACKNOWLEDGE action\n");
		/* TODO: implementation specific */
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_DISABLE_PORT,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming TIME management message type*/
void handleMMTime(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock, const RunTimeOpts* rtOpts)
{
	DBGV("received TIME message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_TIME;

	MMTime* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMTime*)incoming->tlv->dataField;
		/* SET actions */
		/* TODO: add currentTime */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMTime));
		data = (MMTime*)outgoing->tlv->dataField;
		/* GET actions */
		TimeInternal internalTime;
		getTime(&internalTime);
		if (respectUtcOffset(rtOpts, ptpClock) == TRUE) {
			internalTime.seconds += ptpClock->timePropertiesDS.currentUtcOffset;
		}
		fromInternalTime(&internalTime, &data->currentTime);
		timestamp_display(&data->currentTime);
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_TIME,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming CLOCK_ACCURACY management message type*/
void handleMMClockAccuracy(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received CLOCK_ACCURACY message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_CLOCK_ACCURACY;

	MMClockAccuracy* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMClockAccuracy*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->defaultDS.clockQuality.clockAccuracy = data->clockAccuracy;
		const char * acc = accToString(data->clockAccuracy);
		if(acc != NULL) {
		    setConfig(ptpClock->managementConfig, "ptpengine:clock_accuracy", acc);
		}
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMClockAccuracy));
		data = (MMClockAccuracy*)outgoing->tlv->dataField;
		/* GET actions */
		data->clockAccuracy = ptpClock->defaultDS.clockQuality.clockAccuracy;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_CLOCK_ACCURACY,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming UTC_PROPERTIES management message type*/
void handleMMUtcProperties(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received UTC_PROPERTIES message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_UTC_PROPERTIES;

	MMUtcProperties* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMUtcProperties*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->timePropertiesDS.currentUtcOffset = data->currentUtcOffset;
		/* set bit */
		ptpClock->timePropertiesDS.currentUtcOffsetValid = IS_SET(data->utcv_li59_li61, UTCV);
		ptpClock->timePropertiesDS.leap59 = IS_SET(data->utcv_li59_li61, LI59);
		ptpClock->timePropertiesDS.leap61 = IS_SET(data->utcv_li59_li61, LI61);
		tmpsnprintf(tmpStr, 20, "%d", data->currentUtcOffset);
		/* todo: setting leap flags can be handy when we remotely controll PTPd GMs */
		setConfig(ptpClock->managementConfig, "ptpengine:utc_offset", tmpStr);
		setConfig(ptpClock->managementConfig, "ptpengine:utc_offset_valid", IS_SET(data->utcv_li59_li61, UTCV) ? "Y" : "N");
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMUtcProperties));
		data = (MMUtcProperties*)outgoing->tlv->dataField;
		/* GET actions */
		data->currentUtcOffset = ptpClock->timePropertiesDS.currentUtcOffset;
		Octet utcv = SET_FIELD(ptpClock->timePropertiesDS.currentUtcOffsetValid, UTCV);
		Octet li59 = SET_FIELD(ptpClock->timePropertiesDS.leap59, LI59);
		Octet li61 = SET_FIELD(ptpClock->timePropertiesDS.leap61, LI61);
		data->utcv_li59_li61 = utcv | li59 | li61;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_UTC_PROPERTIES,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming TRACEABILITY_PROPERTIES management message type*/
void handleMMTraceabilityProperties(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received TRACEABILITY_PROPERTIES message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_TRACEABILITY_PROPERTIES;

	MMTraceabilityProperties* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMTraceabilityProperties*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->timePropertiesDS.frequencyTraceable = IS_SET(data->ftra_ttra, FTRA);
		ptpClock->timePropertiesDS.timeTraceable = IS_SET(data->ftra_ttra, TTRA);
		setConfig(ptpClock->managementConfig, "ptpengine:frequency_traceable", IS_SET(data->ftra_ttra, FTRA) ? "Y" : "N");
		setConfig(ptpClock->managementConfig, "ptpengine:time_traceable", IS_SET(data->ftra_ttra, TTRA) ? "Y" : "N");
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMTraceabilityProperties));
		data = (MMTraceabilityProperties*)outgoing->tlv->dataField;
		/* GET actions */
		Octet ftra = SET_FIELD(ptpClock->timePropertiesDS.frequencyTraceable, FTRA);
		Octet ttra = SET_FIELD(ptpClock->timePropertiesDS.timeTraceable, TTRA);
		data->ftra_ttra = ftra | ttra;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_TRACEABILITY_PROPERTIES,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming TIMESCALE_PROPERTIES management message type*/
void handleMMTimescaleProperties(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received TIMESCALE_PROPERTIES message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_TRACEABILITY_PROPERTIES;

	MMTimescaleProperties* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMTimescaleProperties*)incoming->tlv->dataField;
		/* SET actions */
		setConfig(ptpClock->managementConfig, "ptpengine:ptp_timesource", getTimeSourceName(data->timeSource));
		setConfig(ptpClock->managementConfig, "ptpengine:ptp_timescale", data->ptp ? "PTP" : "ARB");
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMTimescaleProperties));
		data = (MMTimescaleProperties*)outgoing->tlv->dataField;
		/* GET actions */
		data->ptp = ptpClock->timePropertiesDS.ptpTimescale;
		data->timeSource = ptpClock->timePropertiesDS.timeSource;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_TRACEABILITY_PROPERTIES,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming UNICAST_NEGOTIATION_ENABLE management message type*/
void handleMMUnicastNegotiationEnable(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock, RunTimeOpts *rtOpts)
{
	DBGV("received UNICAST_NEGOTIATION_ENABLE message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_UNICAST_NEGOTIATION_ENABLE;

	MMUnicastNegotiationEnable* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMUnicastNegotiationEnable*)incoming->tlv->dataField;
		/* SET actions */
		setConfig(ptpClock->managementConfig, "ptpengine:unicast_negotiation", data->en ? "Y" : "N");
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMUnicastNegotiationEnable));
		data = (MMUnicastNegotiationEnable*)outgoing->tlv->dataField;
		/* GET actions */
		data->en = rtOpts->unicastNegotiation;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_UNICAST_NEGOTIATION_ENABLE,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming DELAY_MECHANISM management message type*/
void handleMMDelayMechanism(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received DELAY_MECHANISM message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_DELAY_MECHANISM;

	MMDelayMechanism *data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMDelayMechanism*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->portDS.delayMechanism = data->delayMechanism;
		const char * mech = delayMechToString(data->delayMechanism);
		if(mech != NULL) {
		    setConfig(ptpClock->managementConfig, "ptpengine:delay_mechanism", mech);
		}
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMDelayMechanism));
		data = (MMDelayMechanism*)outgoing->tlv->dataField;
		/* GET actions */
		data->delayMechanism = ptpClock->portDS.delayMechanism;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_DELAY_MECHANISM,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming LOG_MIN_PDELAY_REQ_INTERVAL management message type*/
void handleMMLogMinPdelayReqInterval(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received LOG_MIN_PDELAY_REQ_INTERVAL message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->managementId = MM_LOG_MIN_PDELAY_REQ_INTERVAL;

	MMLogMinPdelayReqInterval* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMLogMinPdelayReqInterval*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->portDS.logMinPdelayReqInterval = data->logMinPdelayReqInterval;
		tmpsnprintf(tmpStr, 4, "%d", data->logMinPdelayReqInterval);
		setConfig(ptpClock->managementConfig, "ptpengine:log_peer_delayreq_interval", tmpStr);
		ptpClock->record_update = TRUE;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMLogMinPdelayReqInterval));
		data = (MMLogMinPdelayReqInterval*)outgoing->tlv->dataField;
		/* GET actions */
		data->logMinPdelayReqInterval = ptpClock->portDS.logMinPdelayReqInterval;
		data->reserved = 0x0;
		break;
	case RESPONSE:
		DBGV(" RESPONSE action\n");
		/* TODO: implementation specific */
		break;
	default:
		DBGV(" unknown actionType \n");
		free(outgoing->tlv);
		handleErrorManagementMessage(incoming, outgoing,
			ptpClock, MM_LOG_MIN_PDELAY_REQ_INTERVAL,
			NOT_SUPPORTED);
	}

}

/**\brief Handle incoming ERROR_STATUS management message type*/
void handleMMErrorStatus(MsgManagement *incoming)
{
	(void)incoming;
	DBGV("received MANAGEMENT_ERROR_STATUS message \n");
	/* implementation specific */
}

/**\brief Handle issuing ERROR_STATUS management message type*/
void handleErrorManagementMessage(MsgManagement *incoming, MsgManagement *outgoing, PtpClock *ptpClock, Enumeration16 mgmtId, Enumeration16 errorId)
{
        /* init management header fields */
        initOutgoingMsgManagement(incoming, outgoing, ptpClock);
        /* init management error status tlv fields */
        outgoing->tlv->tlvType = TLV_MANAGEMENT_ERROR_STATUS;
	/* management error status managementId field is the errorId */
	outgoing->tlv->managementId = errorId;
	switch(incoming->actionField)
	{
	case GET:
	case SET:
		outgoing->actionField = RESPONSE;
		break;
	case RESPONSE:
		outgoing->actionField = ACKNOWLEDGE;
		break;
	default:
		outgoing->actionField = 0;
	}

	XMALLOC(outgoing->tlv->dataField, sizeof( MMErrorStatus));
	MMErrorStatus *data = (MMErrorStatus*)outgoing->tlv->dataField;
	/* set managementId */
	data->managementId = mgmtId;
	data->reserved = 0x00;
	data->displayData.lengthField = 0;
	data->displayData.textField = NULL;

}

#if 0
static void
issueManagement(MsgHeader *header,MsgManagement *manage,const RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{

	ptpClock->counters.managementMessagesSent++;

}
#endif

static void
issueManagementRespOrAck(MsgManagement *outgoing, Integer32 dst, const RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{

	/* pack ManagementTLV */
	msgPackManagementTLV( ptpClock->msgObuf, outgoing, ptpClock);

	/* set header messageLength, the outgoing->tlv->lengthField is now valid */
	outgoing->header.messageLength = MANAGEMENT_LENGTH +
					TL_LENGTH +
					outgoing->tlv->lengthField;

	msgPackManagement( ptpClock->msgObuf, outgoing, ptpClock);


	if(!netSendGeneral(ptpClock->msgObuf, outgoing->header.messageLength,
			   &ptpClock->netPath, rtOpts, dst)) {
		DBGV("Management response/acknowledge can't be sent -> FAULTY state \n");
		ptpClock->counters.messageSendErrors++;
		toState(PTP_FAULTY, rtOpts, ptpClock);
	} else {
		DBGV("Management response/acknowledge msg sent \n");
		ptpClock->counters.managementMessagesSent++;
	}
}

static void
issueManagementErrorStatus(MsgManagement *outgoing, Integer32 dst, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

	/* pack ManagementErrorStatusTLV */
	msgPackManagementErrorStatusTLV( ptpClock->msgObuf, outgoing, ptpClock);

	/* set header messageLength, the outgoing->tlv->lengthField is now valid */
	outgoing->header.messageLength = MANAGEMENT_LENGTH +
					TL_LENGTH +
					outgoing->tlv->lengthField;

	msgPackManagement( ptpClock->msgObuf, outgoing, ptpClock);

	if(!netSendGeneral(ptpClock->msgObuf, outgoing->header.messageLength,
			   &ptpClock->netPath, rtOpts, dst)) {
		DBGV("Management error status can't be sent -> FAULTY state \n");
		ptpClock->counters.messageSendErrors++;
		toState(PTP_FAULTY, rtOpts, ptpClock);
	} else {
		DBGV("Management error status msg sent \n");
		ptpClock->counters.managementMessagesSent++;
	}

}
