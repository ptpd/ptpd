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
 * @file   packOutgoingMsg.c
 * @date   Thurs July 5 01:32:03 2012 IST
 * @author Himanshu Singh
 * @brief  This file packs the outgoing message buffer
 *         by taking inputs from user.
 */

#include "ptpmanager.h"

void packCommonHeader(Octet*);
void packManagementHeader(Octet*);
Boolean packCommandOnly(Octet*, MsgManagement*);
Boolean packGETOnly(Octet*, MsgManagement*);
Boolean packMMNullManagement(Octet*);
Boolean packMMClockDescription(Octet*);
Boolean packMMUserDescription(Octet*);
Boolean packMMSaveInNonVolatileStorage(Octet*);
Boolean packMMResetNonVolatileStorage(Octet*);
Boolean packMMInitialize(Octet*);
Boolean packMMFaultLog(Octet*);
Boolean packMMFaultLogReset(Octet*);
Boolean packMMTime(Octet*);
Boolean packMMClockAccuracy(Octet*);
Boolean packMMEnablePort(Octet*);
Boolean packMMDisablePort(Octet*);
Boolean packMMDefaultDataSet(Octet*);
Boolean packMMPriority1(Octet*);
Boolean packMMPriority2(Octet*);
Boolean packMMDomain(Octet*);
Boolean packMMSlaveOnly(Octet*);
Boolean packMMCurrentDataSet(Octet*);
Boolean packMMParentDataSet(Octet*);
Boolean packMMTimePropertiesDataSet(Octet*);
Boolean packMMPortDataSet(Octet*);
Boolean packMMTransparentClockDefaultDataSet(Octet*);
Boolean packMMTransparentClockPortDataSet(Octet*);
Boolean packMMPrimaryDomain(Octet*);
Boolean packMMLogAnnounceInterval(Octet*);
Boolean packMMAnnounceReceiptTimeout(Octet*);
Boolean packMMLogSyncInterval(Octet*);
Boolean packMMVersionNumber(Octet*);
Boolean packMMLogMinPdelayReqInterval(Octet*);
Boolean packMMDelayMechanism(Octet*);
Boolean packMMUtcProperties(Octet*);
Boolean packMMTraceabilityProperties(Octet*);
Boolean packMMTimescaleProperties(Octet*);

/**\brief Function takes managementId and calls methods to pack headers
 * and tlv datafields 
 */
Boolean 
packOutgoingMsg()
{
	Boolean toSend;
	int tlvtype;
	int managementId;
	
	out_length = 0;
	
	packCommonHeader(outmessage);	
	packManagementHeader(outmessage);

	/*Currently supporting only TLV_MANAGEMENT, so tlvType is TLV_MANAGEMENT*/
	
//	printf(">tlvType (1 for TLV_MANAGEMENT) ?");
//	scanf("%d", &tlvtype);

	tlvtype = TLV_MANAGEMENT;

//	if (tlvtype==TLV_MANAGEMENT){
		printf(">managementId (use command 'show_mgmtIds' to find managementId)?");
		scanf("%x", &managementId);

		switch(managementId){
		case MM_NULL_MANAGEMENT:
			toSend = packMMNullManagement(outmessage);
			break;
		case MM_CLOCK_DESCRIPTION:
			toSend = packMMClockDescription(outmessage); 	
			break;
		case MM_USER_DESCRIPTION:
			toSend = packMMUserDescription(outmessage);
			break;
		case MM_SAVE_IN_NON_VOLATILE_STORAGE:
			toSend = packMMSaveInNonVolatileStorage(outmessage);
			break;
		case MM_RESET_NON_VOLATILE_STORAGE:
			toSend = packMMResetNonVolatileStorage(outmessage);	
			break;
		case MM_INITIALIZE:
			toSend = packMMInitialize(outmessage);
			break;
		case MM_FAULT_LOG:
			toSend = packMMFaultLog(outmessage);
			break;
		case MM_FAULT_LOG_RESET:
			toSend = packMMFaultLogReset(outmessage);
			break;
		case MM_DEFAULT_DATA_SET:
			toSend = packMMDefaultDataSet(outmessage);
			break;
		case MM_CURRENT_DATA_SET:
			toSend = packMMCurrentDataSet(outmessage);
			break;
		case MM_PARENT_DATA_SET:
			toSend = packMMParentDataSet(outmessage);
			break;
		case MM_TIME_PROPERTIES_DATA_SET:
			toSend = packMMTimePropertiesDataSet(outmessage);
			break;
		case MM_PORT_DATA_SET:
			toSend = packMMPortDataSet(outmessage);
			break;
		case MM_PRIORITY1:
			toSend = packMMPriority1(outmessage);
			break;
		case MM_PRIORITY2:
			toSend = packMMPriority2(outmessage);
			break;
		case MM_DOMAIN:
			toSend = packMMDomain(outmessage);
			break;
		case MM_SLAVE_ONLY:
			toSend = packMMSlaveOnly(outmessage);
			break;
		case MM_LOG_ANNOUNCE_INTERVAL:
			toSend = packMMLogAnnounceInterval(outmessage);
			break;
		case MM_ANNOUNCE_RECEIPT_TIMEOUT:
			toSend = packMMAnnounceReceiptTimeout(outmessage);
			break;
		case MM_LOG_SYNC_INTERVAL:
			toSend = packMMLogSyncInterval(outmessage);
			break;
		case MM_VERSION_NUMBER:
			toSend = packMMVersionNumber(outmessage);
			break;
		case MM_ENABLE_PORT:
			toSend = packMMEnablePort(outmessage);
			break;
		case MM_DISABLE_PORT:
			toSend = packMMDisablePort(outmessage);
			break;
		case MM_TIME:
			toSend = packMMTime(outmessage);
			break;
		case MM_CLOCK_ACCURACY:
			toSend = packMMClockAccuracy(outmessage);
			break;
		case MM_UTC_PROPERTIES:
			toSend = packMMUtcProperties(outmessage);
			break;
		case MM_TRACEABILITY_PROPERTIES:
			toSend = packMMTraceabilityProperties(outmessage);
			break;
		case MM_DELAY_MECHANISM:
			toSend = packMMDelayMechanism(outmessage);
			break;
		case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
			toSend = packMMLogMinPdelayReqInterval(outmessage);
			break;
		case MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
			toSend = packMMTransparentClockDefaultDataSet(outmessage);
			break;
		case MM_TRANSPARENT_CLOCK_PORT_DATA_SET:
			toSend = packMMTransparentClockPortDataSet(outmessage);
			break;
		case MM_PRIMARY_DOMAIN:
			toSend = packMMPrimaryDomain(outmessage);
			break;
		case MM_TIMESCALE_PROPERTIES:
			toSend = packMMTimescaleProperties(outmessage);
			break;

		case MM_UNICAST_NEGOTIATION_ENABLE:
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

		default:
			printf("This managementId is not supported.\n");
			toSend = FALSE;
		}
		
/*	} else {
		printf("Only TLV_MANAGEMENT (enter 1 for this) is allowed yet\n");
		toSend = FALSE;
	}
*/
	return toSend;
}

/**\brief Function to pack the header of outgoing message */
void 
packCommonHeader(Octet *buf)
{
	Nibble transport = 0x80;
	Nibble versionNumber = 0x02;
	UInteger8 domainNumber = 0x00;
	
	*(UInteger8 *) (buf + 0) = transport;
	/*Message Type = 13 */
	*(UInteger8 *) (buf + 0) = 	*(UInteger8 *) (buf + 0) | 0x0D;
	
	*(unsigned char *)(buf + 1) = *(unsigned char *)(buf + 1) & 0x00;
	*(UInteger4 *) (buf + 1) = *(unsigned char *)(buf + 1) | versionNumber;

	*(UInteger8 *) (buf + 4) = domainNumber;
	*(unsigned char *)(buf + 5) = 0x00;
	*(unsigned char *)(buf + 6) = 0x04; /*Table 20*/
	*(unsigned char *)(buf + 7) = 0x00; /*Table 20*/
	
	/*correction field to be zero for management messages*/
	memset((buf + 8), 0, 8);
	
	/*reserved2 to be 0*/
	memset((buf + 16), 0, 4);
	
	/* To be corrected: temporarily setting sourcePortIdentity to be zero*/
	memset((buf + 20), 0, 10);
	
	/* sequence id */
	*(UInteger16 *) (buf + 30) = flip16(out_sequence);
	
	/*Table 23*/
	*(UInteger8 *) (buf + 32) = 0x04;	
	
	/*Table 24*/
	*(UInteger8 *) (buf + 33) = 0x7F;

}

/**\brief Function to pack Management Header*/
void 
packManagementHeader(Octet *buf)
{
	/* targetPortIdentity to be zero for now*/
	memset((buf + 34), 1, 10);
	
	/* assuming that management message doesn't need to
	 * be retransmitted by boundary clocks
	 */  
	*(UInteger8 *) (buf + 44) = 0x00;
	*(UInteger8 *) (buf + 45) = 0x00;
	
	/* reserved fields to be zero
	 * and actionField to be set during packing tlv data
	 */
	*(UInteger8 *) (buf + 46) = 0x00;
	*(UInteger8 *) (buf + 47) = 0x00;

	out_length += MANAGEMENT_LENGTH;
}

/**\brief Function to pack common fields of all command actionField messages*/
Boolean 
packCommandOnly(Octet *buf, MsgManagement* manage)
{
	int actionField;
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	manage->tlv->lengthField = flip16(0x0002);
	
	printf(">actionField (3 for Command)?");
	scanf("%d",&actionField);
	switch(actionField){
	case COMMAND:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | COMMAND;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	out_length += TLV_LENGTH;
	return TRUE;
}

/**\brief Function to pack common fields of all messages which support 
 *only GET actionField
 */
Boolean
packGETOnly(Octet *buf, MsgManagement* manage)
{
	int actionField;
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	manage->tlv->lengthField = flip16(0x0002);
	
	printf(">actionField (0 for GET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	out_length += TLV_LENGTH;
	return TRUE;
}

/************************************************************************
 * Functions to pack payload for each type of managementId. 
 * These will ask questions from user and set the data fields accordingly.
 ************************************************************************/
 
/**\brief Pack NullManagement, tlv dataField does not contain anything.*/ 
Boolean
packMMNullManagement(Octet *buf)
{
	int actionField;
	MsgManagement *manage = (MsgManagement*)(buf);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	*(UInteger16 *) (buf + 48) = flip16(TLV_MANAGEMENT);
	/* lengthField = 2+N*/
	*(UInteger16 *) (buf + 50) = flip16(0x0002);
	/*managementId Table 40*/
	*(UInteger16 *) (buf + 52) = flip16(MM_NULL_MANAGEMENT);
	
	printf(">actionField (0 for GET, 1 for SET, 3 for Command) ?");
	scanf("%d",&actionField);
	if (actionField == 0)
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
	else if (actionField == 1)
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
	else if (actionField == 3)
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | COMMAND;
	else {
		printf("This action is not allowed for this managementId\n");
		return FALSE;
	}
		
	out_length += TLV_LENGTH;
	return TRUE;
} 

/**\brief Pack Clock Description, supports only GET (Table 40) */
Boolean 
packMMClockDescription(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_CLOCK_DESCRIPTION);
	return packGETOnly(buf,manage);
}

/**\brief Pack User description based on Table 43 of spec, supports GET and SET. */
Boolean
packMMUserDescription(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	MMUserDescription* data = NULL;
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	manage->tlv->lengthField = flip16(0x0002);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_USER_DESCRIPTION);

	printf(">actionField (0 for GET, 1 for SET) ?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		manage->tlv->dataField = NULL;
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
		out_length += TLV_LENGTH;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		data = (MMUserDescription*)malloc(sizeof(MMUserDescription));
		if (data == NULL){
			printf("malloc failed\n");
			return FALSE;
		}
		char text[128];
		int dataFieldLength = 0;
		printf(">UserDescription (DeviceName;PhysicalLocation)?");
		scanf("%s",text);
		data->userDescription.lengthField = strlen(text);
		
		if (data->userDescription.lengthField > USER_DESCRIPTION_MAX){
			printf("user description exceeds specification length");
			return FALSE;
		}
		
		data->userDescription.textField = 
					(Octet*)malloc(data->userDescription.lengthField);
		if (data->userDescription.textField == NULL){
			printf("malloc failed\n");
			return FALSE;
		}
		memcpy(data->userDescription.textField,
                    text, data->userDescription.lengthField);

		memcpy((Octet*)manage->tlv + TLV_LENGTH, data, 1);	
		memcpy((Octet*)manage->tlv +  TLV_LENGTH + 1,
			data->userDescription.textField,
			data->userDescription.lengthField);
	
		               
        /* is the TLV length odd? TLV must be even according to Spec 5.3.8 */
        if (data->userDescription.lengthField % 2 == 0){
        	memset(buf + MANAGEMENT_LENGTH + 
	        		TLV_LENGTH + data->userDescription.lengthField + 1, 0, 1);
			dataFieldLength = data->userDescription.lengthField + 2;
		} 
		else {	
			dataFieldLength = data->userDescription.lengthField + 1;
		}

		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH + TLV_LENGTH + 
										dataFieldLength);		
		out_length += TLV_LENGTH + dataFieldLength;

		free(data->userDescription.textField);
		free(data);
		break;		       
	default:
		printf("This action is not allowed for this managementId\n");
		return FALSE;
	}
		
	return TRUE;
}

/**\brief Pack SaveInNonVolatileStorage TLV, supports command actionField*/ 
Boolean
packMMSaveInNonVolatileStorage(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_SAVE_IN_NON_VOLATILE_STORAGE);
	return packCommandOnly(buf,manage);
}

/**\brief Pack ResetNonVolatileStorage TLV, supports command actionField*/ 
Boolean
packMMResetNonVolatileStorage(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_RESET_NON_VOLATILE_STORAGE);
	return packCommandOnly(buf,manage);
}

/**\brief Pack Initialize TLV based on table 44, supports command actionField*/ 
Boolean
packMMInitialize(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_INITIALIZE);
	out_length += TLV_LENGTH;	

	printf(">actionField (3 for Command) ?");
	scanf("%d",&actionField);
	switch(actionField){
	case COMMAND:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | COMMAND;
		printf("Initialize key (0 for Initialize event)?");
		scanf("%hu",(UInteger16*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH));
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n");
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack FaultLog Tlv based on Table 47, supports GET only*/
Boolean
packMMFaultLog(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_FAULT_LOG);
	out_length += TLV_LENGTH;

	printf(">actionField (0 for GET) ?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		printf("Number of fault logs to be returned (0-128)?");
		scanf("%hu",(UInteger16*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH));
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}

	return TRUE;
}

/**\brief Pack FaultLogReset, supports command actionField only*/
Boolean
packMMFaultLogReset(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->managementId = flip16(MM_FAULT_LOG_RESET);
	return packCommandOnly(buf,manage);
}

/**\brief Pack Time tlv based on table 48, supports GET and SET*/
Boolean
packMMTime(Octet *buf)
{
	int actionField;

	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TIME);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;

		UInteger32 hours;
		UInteger32 mins;
		UInteger32 secs;
		UInteger32 nanosecs;
		UInteger16 epoch;
		printf(">Timestamp - Epoch Number?");
		scanf("%hu",&epoch);
		*(UInteger16 *)((Octet*)(manage->tlv) + TLV_LENGTH) = flip16(epoch);
		printf(">Timestamp 'hours mins secs nanosecs' Eg - '1 20 30 400' ?");
		scanf("%u %u %u %u",&hours,&mins,&secs,&nanosecs);
		*(UInteger32*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH + 2) = 
								flip32(hours*60*60 + mins*60 + secs);
		*(UInteger32*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH + 6) = 
								flip32(nanosecs);								
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 10);
		manage->tlv->lengthField = flip16(0x000C);
		out_length += 10;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack ClockAccuracy based on table 49, supports GET and SET*/
Boolean
packMMClockAccuracy(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_CLOCK_ACCURACY);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;

	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf(">clock accuracy number (32-49) see table 6?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;

		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack EnablePort which cause the DESIGNATED_ENABLED event to occur*/
Boolean
packMMEnablePort(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->managementId = flip16(MM_ENABLE_PORT);
	return packCommandOnly(buf,manage);
}

/**\brief Pack DisablePort command which cause the DESIGNATED_DISABLED event to occur*/
Boolean
packMMDisablePort(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->managementId = flip16(MM_DISABLE_PORT);
	return packCommandOnly(buf,manage);
}

/**\brief  Pack DefaultDataSet based on table 50, which is used to 
 * GET members of defaultDS
 */
Boolean 
packMMDefaultDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_DEFAULT_DATA_SET);
	return 	packGETOnly(buf,manage);
}

/**\brief  Pack Priority1 of defaultDS, based on table 51, supports GET and SET*/
Boolean
packMMPriority1(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PRIORITY1);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
		
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of Priority1?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack Priority1 of defaultDS, based on table 52, supports GET and SET*/
Boolean
packMMPriority2(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PRIORITY2);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;

	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of Priority2?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack Domain of defaultDS, based on table 53, supports GET and SET*/
Boolean
packMMDomain(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_DOMAIN);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of domainNumber?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack SlaveOnly of defaultDS, based on table 54, supports GET and SET*/
Boolean
packMMSlaveOnly(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_SLAVE_ONLY);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;

	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		unsigned int slaveOnly;
		printf("Slave Only (1 for yes, 0 for no)?");
		scanf("%u",&slaveOnly);
		if (slaveOnly == 1)
			*((UInteger8 *)(manage->tlv) + TLV_LENGTH) = 0x01;
		 else if (slaveOnly == 0)
			*((UInteger8 *)(manage->tlv) + TLV_LENGTH) = 0x00;
		 else {
		 	printf("Invalid value\n");
			return FALSE;
		}
		
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack CurrentDataSet based on Table 55, to GET the members of currentDS*/
Boolean 
packMMCurrentDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_CURRENT_DATA_SET);
	return 	packGETOnly(buf,manage);
}

/**\brief Pack ParentDataSet based on Table 56, to GET the members of parentDS*/
Boolean 
packMMParentDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PARENT_DATA_SET);
	return packGETOnly(buf,manage);
}

/**\brief  Pack TimePropertiesDataSet based on Table 57, 
 * to GET the members of timePropertiesDS
 */
Boolean 
packMMTimePropertiesDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TIME_PROPERTIES_DATA_SET);
	return packGETOnly(buf,manage);
}


/**\brief Pack UTC properties, table 58*/
Boolean 
packMMUtcProperties(Octet *buf)
{
	int actionField;
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_UTC_PROPERTIES);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		Integer16 currentUtcOffset;
		UInteger8 l161;
		UInteger8 l159;
		UInteger8 utcv;
		printf("value of currentUtcOffset?");
		scanf("%hd",&currentUtcOffset);
		printf("value of UTCV,L1-59,L1-61 (Boolean) Eg - 1,0,0 ?");
		scanf("%hhu,%hhu,%hhu",&utcv,&l159,&l161);
		if (utcv > 1 || l159 > 1 || l161 > 1){
			printf("Enter only boolean values (1 or 0) separated by comma\n");
			return FALSE;
		}
		
		*(Integer16*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH) =
							flip16(currentUtcOffset);
		*(UInteger8*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH + 2) = 
							(utcv<<2 & 0x04) | (l159<<1 & 0x02) | (l161 & 0x01);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 4);
		*(UInteger8*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH + 3) = 0x00;
		out_length += 4;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack Tracebility properties, table 59*/
Boolean 
packMMTraceabilityProperties(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TRACEABILITY_PROPERTIES);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		UInteger8 ftra;
		UInteger8 ttra;
		printf("value of FTRA,TTRA (Boolean) Eg - 1,0 ?");
		scanf("%hhu,%hhu",&ftra,&ttra);
		if (ftra > 1 || ttra > 1){
			printf("Enter only boolean values (1 or 0) separated by comma\n");
			return FALSE;
		}
		*(UInteger8*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH) = 
							(ftra<<5 & 0x20) | (ttra<<4 & 0x10);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	return TRUE;
}

/**\brief Pack TimescaleProperties, table 60*/
Boolean 
packMMTimescaleProperties(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TIMESCALE_PROPERTIES);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		UInteger8 ptp;
		printf("value of timeSource?");
		scanf("%hhu",(Enumeration8*)(manage->tlv) + TLV_LENGTH + 1);
		printf("value of ptp (Boolean)?");
		scanf("%hhu",&ptp);
		if (ptp > 1){
			printf("Enter only boolean values (1 or 0)\n");
			return FALSE;
		}
		*(UInteger8*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH) = (ptp<<3 & 0x08);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	return TRUE;
}

/**\brief Pack PortDataSet based on table 61, to GET members of portDS*/
Boolean 
packMMPortDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PORT_DATA_SET);
	return packGETOnly(buf,manage);
}

/**\brief Pack TransparentClockDefaultDataSet based on table 68, 
 * to GET members of defaultDS.
 */
Boolean 
packMMTransparentClockDefaultDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET);
	return packGETOnly(buf,manage);
}

/**\brief Pack TransparentClockPortDataSet based on table 70, 
 * to GET members of portDS
 */
Boolean 
packMMTransparentClockPortDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TRANSPARENT_CLOCK_PORT_DATA_SET);
	return packGETOnly(buf,manage);
}

/**\brief Pack Primary Domain, table 69*/
Boolean
packMMPrimaryDomain(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PRIMARY_DOMAIN);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of primaryDomain?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack LogAnnounceInterval, table 62*/
Boolean
packMMLogAnnounceInterval(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_LOG_ANNOUNCE_INTERVAL);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of log announce interval?");
		scanf("%hhd",(Integer8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack AnnounceReceiptTimeout, table 63*/
Boolean
packMMAnnounceReceiptTimeout(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_ANNOUNCE_RECEIPT_TIMEOUT);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of Announce Receipt Timeout?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack LogSyncInterval, table 64*/
Boolean
packMMLogSyncInterval(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_LOG_SYNC_INTERVAL);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of log sync interval?");
		scanf("%hhd",(Integer8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack delayMechanism, table 65*/
Boolean
packMMDelayMechanism(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_DELAY_MECHANISM);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("delay mechanism?");
		scanf("%hhu",(Enumeration8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack LogMinPdelayReqInterval, table 66*/
Boolean
packMMLogMinPdelayReqInterval(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_LOG_MIN_PDELAY_REQ_INTERVAL);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of log_min_pdelay_req_interval?");
		scanf("%hhd",(Integer8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

/**\brief Pack Version number, table 68*/
Boolean
packMMVersionNumber(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_VERSION_NUMBER);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf(">actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		UInteger8 versionNumber;
		printf("version number?");
		scanf("%hhd",&versionNumber);
		*((UInteger8*)(manage->tlv) + TLV_LENGTH) = versionNumber & 0x0F;
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

