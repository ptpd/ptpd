/**
 * @file   handleIncomingMsg.c
 * @date   Thurs July 5 01:40:38 2012
 * 
 * @brief  Functions  to parse the response/ack/error message received after 
 *         sending a management message and displays the field values 
 *         received message.
 */

#include "ptpmanager.h"

/*Function to unpack the header of received message*/
void 
unpackHeader(Octet *buf, MsgHeader *header)
{	
	header->transportSpecific = (*(Nibble *) (buf + 0)) >> 4;
	header->messageType = (*(Enumeration4 *) (buf + 0)) & 0x0F;
	header->versionPTP = (*(UInteger4 *) (buf + 1)) & 0x0F;
	/* force reserved bit to zero if not */
	header->messageLength = flip16(*(UInteger16 *) (buf + 2));
	header->domainNumber = (*(UInteger8 *) (buf + 4));
	header->flagField0 = (*(Octet *) (buf + 6));
	header->flagField1 = (*(Octet *) (buf + 7));
	memcpy(&header->correctionField.msb, (buf + 8), 4);
	memcpy(&header->correctionField.lsb, (buf + 12), 4);
	header->correctionField.msb = flip32(header->correctionField.msb);
	header->correctionField.lsb = flip32(header->correctionField.lsb);
//	copyClockIdentity(header->sourcePortIdentity.clockIdentity, (buf + 20));
	header->sourcePortIdentity.portNumber =
		flip16(*(UInteger16 *) (buf + 28));
	header->sequenceId = flip16(*(UInteger16 *) (buf + 30));
	header->controlField = (*(UInteger8 *) (buf + 32));
	header->logMessageInterval = (*(Integer8 *) (buf + 33));
}

/*Function to unpack management header*/
void 
unpackManagementHeader(Octet *inmessage, MsgManagement *manage)
{
	manage->startingBoundaryHops = (*(UInteger8 *)(inmessage + 44));
	manage->boundaryHops = (*(UInteger8 *)(inmessage + 45)); 
	manage->reserved0 = (*(Enumeration4 *)(inmessage + 46)) & 0xF0;
	manage->actionField = (*(Enumeration4 *)(inmessage + 46)) & 0x0F;
	manage->reserved1 = (*(UInteger8 *)(inmessage + 47)); 
	
	manage->tlv->tlvType = flip16(*(UInteger16 *) (inmessage + 48));
	manage->tlv->lengthField = flip16(*(UInteger16 *) (inmessage + 50));
	manage->tlv->managementId = flip16(*(UInteger16 *) (inmessage + 52));
}

/*Function to handle management response and display the required fields*/
void 
handleManagementResponse(Octet *inmessage, MsgManagement *manage)
{
	printf("Received a RESPONSE management message.\n");
	switch(manage->tlv->managementId){
    case MM_NULL_MANAGEMENT:
    case MM_CLOCK_DESCRIPTION:
    case MM_USER_DESCRIPTION:
    case MM_SAVE_IN_NON_VOLATILE_STORAGE:
    case MM_RESET_NON_VOLATILE_STORAGE:
    case MM_INITIALIZE:
    case MM_DEFAULT_DATA_SET:
    case MM_CURRENT_DATA_SET:
    case MM_PARENT_DATA_SET:
    case MM_TIME_PROPERTIES_DATA_SET:
    case MM_PORT_DATA_SET:
    case MM_PRIORITY1:
    case MM_PRIORITY2:
    case MM_DOMAIN:
    case MM_SLAVE_ONLY:
    case MM_LOG_ANNOUNCE_INTERVAL:
    case MM_ANNOUNCE_RECEIPT_TIMEOUT:
    case MM_LOG_SYNC_INTERVAL:
    case MM_VERSION_NUMBER:
    case MM_ENABLE_PORT:
    case MM_DISABLE_PORT:
    case MM_TIME:
    case MM_CLOCK_ACCURACY:
    case MM_UTC_PROPERTIES:
    case MM_TRACEABILITY_PROPERTIES:
    case MM_DELAY_MECHANISM:
    case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
    case MM_FAULT_LOG:
    case MM_FAULT_LOG_RESET:
    case MM_TIMESCALE_PROPERTIES:
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
    case MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
    case MM_TRANSPARENT_CLOCK_PORT_DATA_SET:
    case MM_PRIMARY_DOMAIN:
        printf("Currently unhandled managementTLV Response %d\n", 
        		manage->tlv->managementId);
		return;
    default:
        printf("Unknown managementTLV in response %d\n", 
        		manage->tlv->managementId);
        return;
    }
}

/*Function to handle management ack*/
void 
handleManagementAck(Octet *inmessage, MsgManagement *manage)
{
	if (outmessage != NULL)
		if (*(UInteger16 *) (outmessage + 52) == manage->tlv->managementId)
			printf("Received ACKNOWLEDGEMENT. Command executed.\n");
	else
		printf("Received ACKNOWLEDGEMENT. Not for your command.\n");
}

/*Function to handle management error message*/
void 
handleManagementError(Octet *inmessage, MsgManagement *manage)
{
	printf("Received TLV_MANAGEMENT_ERROR_STATUS.\n");
	if (outmessage != NULL && *(UInteger16 *) (outmessage + 52) == 
			*(UInteger16 *) (inmessage + 54))
		switch(manage->tlv->managementId){
		case RESPONSE_TOO_BIG:
			printf("Response Too Big\n");
			break;
		case NO_SUCH_ID:
			printf("The managementId is not recognized.\n");
			break;
		case WRONG_LENGTH:
			printf("The managementId was identified but the length of the data "
			 		"was wrong.\n");
			break;
		case WRONG_VALUE:
			printf("The managementId and length were correct but one or more values"
					" were wrong.\n");
			break;
		case NOT_SETABLE:
			printf("Some of the variables in the set command were not updated "
			"because they are not configurable.\n");
			break;
		case NOT_SUPPORTED:
			printf("The requested operation is not supported in this node.\n");
			break;
		case GENERAL_ERROR:
			printf("A general error occured.\n");
			break;
		default:
			printf("Unknown Error code\n");
		}
}

void handleIncomingMsg(Octet *inmessage)
{
	MsgHeader h; MsgManagement manage;
	manage.tlv = (ManagementTLV *)malloc(sizeof(ManagementTLV));
	unpackHeader(inmessage, &h);
	if (h.messageLength < MANAGEMENT_LENGTH){
		printf("Truncated message\n");
		return;
	}
	
	if (h.messageType == MANAGEMENT){
		receivedFlag = TRUE;
		unpackManagementHeader(inmessage, &manage);
		if (manage.tlv->tlvType == TLV_MANAGEMENT)
		{
			if (manage.actionField == RESPONSE)
				handleManagementResponse(inmessage,&manage);
			else if (manage.actionField == ACKNOWLEDGE)
				handleManagementAck(inmessage, &manage);
		} 
		else if (manage.tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS)
			handleManagementError(inmessage, &manage);
		else 
			printf("Received management message with unknown tlyType\n");
	} 
	else {
		printf("Received message is not a MANAGEMENT message\n");
		return;
	}
}
