/*-
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

/**\brief Initialize outgoing management message fields*/
void initOutgoingMsgManagement(MsgManagement* incoming, MsgManagement* outgoing, PtpClock *ptpClock)
{
	/* set header fields */
	outgoing->header.transportSpecific = 0x0;
	outgoing->header.messageType = MANAGEMENT;
        outgoing->header.versionPTP = ptpClock->versionNumber;
	outgoing->header.domainNumber = ptpClock->domainNumber;
        /* set header flagField to zero for management messages, Spec 13.3.2.6 */
        outgoing->header.flagField0 = 0x00;
        outgoing->header.flagField1 = 0x00;
        outgoing->header.correctionField.msb = 0;
        outgoing->header.correctionField.lsb = 0;
	copyPortIdentity(&outgoing->header.sourcePortIdentity, &ptpClock->portIdentity);
	outgoing->header.sequenceId = incoming->header.sequenceId;
	outgoing->header.controlField = 0x0; /* deprecrated for ptp version 2 */
	outgoing->header.logMessageInterval = 0x7F;

	/* set management message fields */
	copyPortIdentity( &outgoing->targetPortIdentity, &incoming->header.sourcePortIdentity );
	outgoing->startingBoundaryHops = incoming->startingBoundaryHops - incoming->boundaryHops;
	outgoing->boundaryHops = outgoing->startingBoundaryHops;
        outgoing->actionField = 0; /* set default action, avoid uninitialized value */

	/* init managementTLV */
	XMALLOC(outgoing->tlv, sizeof(ManagementTLV));
	outgoing->tlv->dataField = NULL;
}

/**\brief Handle incoming NULL_MANAGEMENT message*/
void handleMMNullManagement(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received NULL_MANAGEMENT message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_NULL_MANAGEMENT;

	switch(incoming->actionField)
	{
	case GET:
	case SET:
		DBGV(" GET or SET mgmt msg\n");
		break;
	case COMMAND:
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
void handleMMClockDescription(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received CLOCK_DESCRIPTION management message \n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->lengthField = 2;
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
                        ptpClock->port_uuid_field,
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
                data->productDescription.lengthField = sizeof(PRODUCT_DESCRIPTION) - 1;
                XMALLOC(data->productDescription.textField,
                                        data->productDescription.lengthField);
                memcpy(data->productDescription.textField,
                        &PRODUCT_DESCRIPTION,
                        data->productDescription.lengthField);
		/* revision data */
                data->revisionData.lengthField = sizeof(REVISION) - 1;
                XMALLOC(data->revisionData.textField,
                                        data->revisionData.lengthField);
                memcpy(data->revisionData.textField,
                        &REVISION,
                        data->revisionData.lengthField);
		/* user description */
                data->userDescription.lengthField = strlen(ptpClock->user_description);
                XMALLOC(data->userDescription.textField,
                                        data->userDescription.lengthField);
                memcpy(data->userDescription.textField,
                        ptpClock->user_description,
                        data->userDescription.lengthField);
		/* profile identity is zero unless specific profile is implemented */
		data->profileIdentity0 = 0;
		data->profileIdentity1 = 0;
		data->profileIdentity2 = 0;
		data->profileIdentity3 = 0;
		data->profileIdentity4 = 0;
		data->profileIdentity5 = 0;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_SLAVE_ONLY;

	MMSlaveOnly* data = NULL;
	switch (incoming->actionField)
	{
	case SET:
		DBGV(" SET action \n");
		data = (MMSlaveOnly*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->slaveOnly = data->so;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action \n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMSlaveOnly));
		data = (MMSlaveOnly*)outgoing->tlv->dataField;
		/* GET actions */
		data->so = ptpClock->slaveOnly;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_USER_DESCRIPTION;

	MMUserDescription* data = NULL;
	switch(incoming->actionField)
	{
	case SET:
		DBGV(" SET action \n");
		data = (MMUserDescription*)incoming->tlv->dataField;
		UInteger8 userDescriptionLength = data->userDescription.lengthField;
		if(userDescriptionLength <= USER_DESCRIPTION_MAX) {
			memset(ptpClock->user_description, 0, sizeof(ptpClock->user_description));
			memcpy(ptpClock->user_description, data->userDescription.textField,
					userDescriptionLength);
			/* add null-terminator to make use of C string function strlen later */
			ptpClock->user_description[userDescriptionLength] = '\0';
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
                data->userDescription.lengthField = strlen(ptpClock->user_description);
                XMALLOC(data->userDescription.textField,
                                        data->userDescription.lengthField);
                memcpy(data->userDescription.textField,
                        ptpClock->user_description,
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
	outgoing->tlv->lengthField = 2;
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
	outgoing->tlv->lengthField = 2;
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
	outgoing->tlv->lengthField = 2;
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
			ptpClock->portState = PTP_INITIALIZING;
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
	outgoing->tlv->lengthField = 2;
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
		Octet so = ptpClock->slaveOnly << 1;
		/* get bit and align by shifting right 1 since TWO_STEP_FLAG is either 0b00 or 0b10 */
		Octet tsc = ptpClock->twoStepFlag >> 1;
		data->so_tsc = so | tsc;
		data->reserved0 = 0x0;
		data->numberPorts = ptpClock->numberPorts;
		data->priority1 = ptpClock->priority1;
		data->clockQuality.clockAccuracy = ptpClock->clockQuality.clockAccuracy;
		data->clockQuality.clockClass = ptpClock->clockQuality.clockClass;
		data->clockQuality.offsetScaledLogVariance =
				ptpClock->clockQuality.offsetScaledLogVariance;
		data->priority2 = ptpClock->priority2;
		/* copy clockIdentity */
		copyClockIdentity(data->clockIdentity, ptpClock->clockIdentity);
		data->domainNumber = ptpClock->domainNumber;
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
	outgoing->tlv->lengthField = 2;
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
		data->stepsRemoved = ptpClock->stepsRemoved;
		TimeInterval oFM;
		oFM.scaledNanoseconds.lsb = 0;
		oFM.scaledNanoseconds.msb = 0;
		/* TODO: call function
		 * internalTime_to_integer64(ptpClock->offsetFromMaster, &oFM.scaledNanoseconds);
		 */
		data->offsetFromMaster.scaledNanoseconds.lsb = oFM.scaledNanoseconds.lsb;
		data->offsetFromMaster.scaledNanoseconds.msb = oFM.scaledNanoseconds.msb;
		TimeInterval mPD;
		mPD.scaledNanoseconds.lsb = 0;
		mPD.scaledNanoseconds.msb = 0;
		/* TODO: call function
		 * internalTime_to_integer64(ptpClock->meanPathDelay, &mPD.scaledNanoseconds);
		 */
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
	outgoing->tlv->lengthField = 2;
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
		copyPortIdentity(&data->parentPortIdentity, &ptpClock->parentPortIdentity);
		data->PS = ptpClock->parentStats;
		data->reserved = 0;
		data->observedParentOffsetScaledLogVariance =
				ptpClock->observedParentOffsetScaledLogVariance;
		data->observedParentClockPhaseChangeRate =
				ptpClock->observedParentClockPhaseChangeRate;
		data->grandmasterPriority1 = ptpClock->grandmasterPriority1;
		data->grandmasterClockQuality.clockAccuracy =
				ptpClock->grandmasterClockQuality.clockAccuracy;
		data->grandmasterClockQuality.clockClass =
				ptpClock->grandmasterClockQuality.clockClass;
		data->grandmasterClockQuality.offsetScaledLogVariance =
				ptpClock->grandmasterClockQuality.offsetScaledLogVariance;
		data->grandmasterPriority2 = ptpClock->grandmasterPriority2;
		copyClockIdentity(data->grandmasterIdentity, ptpClock->grandmasterIdentity);
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
	outgoing->tlv->lengthField = 2;
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
		data->currentUtcOffset = ptpClock->currentUtcOffset;
		Octet ftra = SET_FIELD(ptpClock->frequencyTraceable, FTRA);
		Octet ttra = SET_FIELD(ptpClock->timeTraceable, TTRA);
		Octet ptp = SET_FIELD(ptpClock->ptpTimescale, PTPT);
		Octet utcv = SET_FIELD(ptpClock->currentUtcOffsetValid, UTCV);
		Octet li59 = SET_FIELD(ptpClock->leap59, LI59);
		Octet li61 = SET_FIELD(ptpClock->leap61, LI61);
		data->ftra_ttra_ptp_utcv_li59_li61 = ftra | ttra | ptp | utcv | li59 | li61;
		data->timeSource = ptpClock->timeSource;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_PORT_DATA_SET;

	MMPortDataSet *data = NULL;
	switch( incoming->actionField )
	{
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMPortDataSet));
		data = (MMPortDataSet*)outgoing->tlv->dataField;
		copyPortIdentity(&data->portIdentity, &ptpClock->portIdentity);
		data->portState = ptpClock->portState;
		data->logMinDelayReqInterval = ptpClock->logMinDelayReqInterval;
		TimeInterval pMPD;
		pMPD.scaledNanoseconds.lsb = 0;
		pMPD.scaledNanoseconds.msb = 0;
		/* TODO: call function
		 * internalTime_to_integer64(ptpClock->peerMeanPathDelay, &pMPD.scaledNanoseconds);
		 */
		data->peerMeanPathDelay.scaledNanoseconds.lsb = pMPD.scaledNanoseconds.lsb;
		data->peerMeanPathDelay.scaledNanoseconds.msb = pMPD.scaledNanoseconds.msb;
		data->logAnnounceInterval = ptpClock->logAnnounceInterval;
		data->announceReceiptTimeout = ptpClock->announceReceiptTimeout;
		data->logSyncInterval = ptpClock->logSyncInterval;
		data->delayMechanism  = ptpClock->delayMechanism;
		data->logMinPdelayReqInterval = ptpClock->logMinPdelayReqInterval;
		data->reserved = 0;
		data->versionNumber = ptpClock->versionNumber;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_PRIORITY1;

	MMPriority1* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMPriority1*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->priority1 = data->priority1;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMPriority1));
		data = (MMPriority1*)outgoing->tlv->dataField;
		/* GET actions */
		data->priority1 = ptpClock->priority1;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_PRIORITY2;

	MMPriority2* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMPriority2*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->priority2 = data->priority2;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMPriority2));
		data = (MMPriority2*)outgoing->tlv->dataField;
		/* GET actions */
		data->priority2 = ptpClock->priority2;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_DOMAIN;

	MMDomain* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMDomain*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->domainNumber = data->domainNumber;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMDomain));
		data = (MMDomain*)outgoing->tlv->dataField;
		/* GET actions */
		data->domainNumber = ptpClock->domainNumber;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_LOG_ANNOUNCE_INTERVAL;

	MMLogAnnounceInterval* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMLogAnnounceInterval*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->logAnnounceInterval = data->logAnnounceInterval;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMLogAnnounceInterval));
		data = (MMLogAnnounceInterval*)outgoing->tlv->dataField;
		/* GET actions */
		data->logAnnounceInterval = ptpClock->logAnnounceInterval;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_ANNOUNCE_RECEIPT_TIMEOUT;

	MMAnnounceReceiptTimeout* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMAnnounceReceiptTimeout*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->announceReceiptTimeout = data->announceReceiptTimeout;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMAnnounceReceiptTimeout));
		data = (MMAnnounceReceiptTimeout*)outgoing->tlv->dataField;
		/* GET actions */
		data->announceReceiptTimeout = ptpClock->announceReceiptTimeout;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_LOG_SYNC_INTERVAL;

	MMLogSyncInterval* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMLogSyncInterval*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->logSyncInterval = data->logSyncInterval;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMLogSyncInterval));
		data = (MMLogSyncInterval*)outgoing->tlv->dataField;
		/* GET actions */
		data->logSyncInterval = ptpClock->logSyncInterval;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_VERSION_NUMBER;

	MMVersionNumber* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMVersionNumber*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->versionNumber = data->versionNumber;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMVersionNumber));
		data = (MMVersionNumber*)outgoing->tlv->dataField;
		/* GET actions */
		data->reserved0 = 0x0;
		data->versionNumber = ptpClock->versionNumber;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_ENABLE_PORT;

	switch( incoming->actionField )
	{
	case COMMAND:
		DBGV(" COMMAND action\n");
		outgoing->actionField = ACKNOWLEDGE;
		/* check if port is disabled, if so then initialize */
		if(ptpClock->portState == PTP_DISABLED) {
			ptpClock->portState = PTP_INITIALIZING;
		}
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_DISABLE_PORT;

	switch( incoming->actionField )
	{
	case COMMAND:
		DBGV(" COMMAND action\n");
		outgoing->actionField = ACKNOWLEDGE;
		/* disable port */
		ptpClock->portState = PTP_DISABLED;
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
void handleMMTime(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received TIME message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->lengthField = 2;
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
		/* TODO: implement action */
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_CLOCK_ACCURACY;

	MMClockAccuracy* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMClockAccuracy*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->clockQuality.clockAccuracy = data->clockAccuracy;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMClockAccuracy));
		data = (MMClockAccuracy*)outgoing->tlv->dataField;
		/* GET actions */
		data->clockAccuracy = ptpClock->clockQuality.clockAccuracy;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_UTC_PROPERTIES;

	MMUtcProperties* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMUtcProperties*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->currentUtcOffset = data->currentUtcOffset;
		/* set bit */
		ptpClock->currentUtcOffsetValid = IS_SET(data->utcv_li59_li61, UTCV);
		ptpClock->leap59 = IS_SET(data->utcv_li59_li61, LI59);
		ptpClock->leap61 = IS_SET(data->utcv_li59_li61, LI61);
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMUtcProperties));
		data = (MMUtcProperties*)outgoing->tlv->dataField;
		/* GET actions */
		data->currentUtcOffset = ptpClock->currentUtcOffset;
		Octet utcv = SET_FIELD(ptpClock->currentUtcOffsetValid, UTCV);
		Octet li59 = SET_FIELD(ptpClock->leap59, LI59);
		Octet li61 = SET_FIELD(ptpClock->leap61, LI61);
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_TRACEABILITY_PROPERTIES;

	MMTraceabilityProperties* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMTraceabilityProperties*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->frequencyTraceable = IS_SET(data->ftra_ttra, FTRA);
		ptpClock->timeTraceable = IS_SET(data->ftra_ttra, TTRA);
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMTraceabilityProperties));
		data = (MMTraceabilityProperties*)outgoing->tlv->dataField;
		/* GET actions */
		Octet ftra = SET_FIELD(ptpClock->frequencyTraceable, FTRA);
		Octet ttra = SET_FIELD(ptpClock->timeTraceable, TTRA);
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

/**\brief Handle incoming DELAY_MECHANISM management message type*/
void handleMMDelayMechanism(MsgManagement* incoming, MsgManagement* outgoing, PtpClock* ptpClock)
{
	DBGV("received DELAY_MECHANISM message\n");

	initOutgoingMsgManagement(incoming, outgoing, ptpClock);
	outgoing->tlv->tlvType = TLV_MANAGEMENT;
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_DELAY_MECHANISM;

	MMDelayMechanism *data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMDelayMechanism*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->delayMechanism = data->delayMechanism;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMDelayMechanism));
		data = (MMDelayMechanism*)outgoing->tlv->dataField;
		/* GET actions */
		data->delayMechanism = ptpClock->delayMechanism;
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
	outgoing->tlv->lengthField = 2;
	outgoing->tlv->managementId = MM_LOG_MIN_PDELAY_REQ_INTERVAL;

	MMLogMinPdelayReqInterval* data = NULL;
	switch( incoming->actionField )
	{
	case SET:
		DBGV(" SET action\n");
		data = (MMLogMinPdelayReqInterval*)incoming->tlv->dataField;
		/* SET actions */
		ptpClock->logMinPdelayReqInterval = data->logMinPdelayReqInterval;
		/* intentionally fall through to GET case */
	case GET:
		DBGV(" GET action\n");
		outgoing->actionField = RESPONSE;
		XMALLOC(outgoing->tlv->dataField, sizeof(MMLogMinPdelayReqInterval));
		data = (MMLogMinPdelayReqInterval*)outgoing->tlv->dataField;
		/* GET actions */
		data->logMinPdelayReqInterval = ptpClock->logMinPdelayReqInterval;
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
	outgoing->tlv->lengthField = 2;
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
