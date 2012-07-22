/**
 * @file   packOutgoingMsg.c
 * @date   Thurs July 5 01:32:03 2012
 * 
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


Boolean 
packOutgoingMsg(Octet *buf)
{
	Boolean toSend;
	int tlvtype;
	int managementId;
	
	packCommonHeader(outmessage);	
	packManagementHeader(outmessage);

	printf(">tlvType (1 for TLV_MANAGEMENT) ?");
	scanf("%d", &tlvtype);

	if (tlvtype==TLV_MANAGEMENT){
		printf(">managementId (see Table 40) (Eg: '2010' for ClockAccuracy)?");
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
		
	} else {
		printf("Only TLV_MANAGEMENT (enter 1 for this) is allowed yet\n");
		toSend = FALSE;
	}

	return toSend;
}
/* Function to pack the header of outgoing message */
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

/*Function to pack Management Header*/
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

/*
 * Functions to pack payload for each type of managementId. 
 * These will ask questions from user and set the data fields accordingly.
 */
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

Boolean 
packMMClockDescription(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_CLOCK_DESCRIPTION);
	return packGETOnly(buf,manage);
}

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

	//*(UInteger16 *) (buf + 52) = flip16(0x0002);		
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


Boolean
packMMSaveInNonVolatileStorage(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_SAVE_IN_NON_VOLATILE_STORAGE);
	return packCommandOnly(buf,manage);
}

Boolean
packMMResetNonVolatileStorage(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_RESET_NON_VOLATILE_STORAGE);
	return packCommandOnly(buf,manage);
}

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

Boolean
packMMFaultLogReset(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->managementId = flip16(MM_FAULT_LOG_RESET);
	return packCommandOnly(buf,manage);
}

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
		printf(">clock accuracy number (20-31) see table 6?");
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

Boolean
packMMEnablePort(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->managementId = flip16(MM_ENABLE_PORT);
	return packCommandOnly(buf,manage);
}


Boolean
packMMDisablePort(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->managementId = flip16(MM_DISABLE_PORT);
	return packCommandOnly(buf,manage);
}

Boolean 
packMMDefaultDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_DEFAULT_DATA_SET);
	return 	packGETOnly(buf,manage);
}


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


Boolean 
packMMCurrentDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_CURRENT_DATA_SET);
	return 	packGETOnly(buf,manage);
}

Boolean 
packMMParentDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PARENT_DATA_SET);
	return packGETOnly(buf,manage);
}

Boolean 
packMMTimePropertiesDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TIME_PROPERTIES_DATA_SET);
	return packGETOnly(buf,manage);
}

Boolean 
packMMPortDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PORT_DATA_SET);
	return packGETOnly(buf,manage);
}

Boolean 
packMMTransparentClockDefaultDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET);
	return packGETOnly(buf,manage);
}

Boolean 
packMMTransparentClockPortDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TRANSPARENT_CLOCK_PORT_DATA_SET);
	return packGETOnly(buf,manage);
}


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
