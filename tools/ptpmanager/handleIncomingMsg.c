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
	printf("Received a RESPONSE management message. Not handled yet\n");
}

/*Function to handle management ack*/
void 
handleManagementAck(Octet *inmessage, MsgManagement *manage)
{
	printf("Received ACKNOWLEDGEMENT management message\n");
}

/*Function to handle management error message*/
void 
handleManagementError(Octet *inmessage, MsgManagement *manage)
{
	printf("Received TLV_MANAGEMENT_ERROR_STATUS. Not handled yet\n");
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
