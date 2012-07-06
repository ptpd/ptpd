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
unpackHeader(Octet *message, MsgHeader *h)
{
}

/*Function to unpack management header*/
void 
unpackManagementHeader(Octet *inmessage, MsgManagement *manage)
{
}

/*Function to handle management response and display the required fields*/
void 
handleManagementResponse(Octet *inmessage, MsgManagement *manage)
{

}

/*Function to handle management ack*/
void 
handleManagementAck(Octet *inmessage, MsgManagement *manage)
{
}

/*Function to handle management error message*/
void 
handleManagementError(Octet *inmessage, MsgManagement *manage)
{
}

void handleIncomingMsg(Octet *inmessage)
{
	MsgHeader *h; MsgManagement *manage;
	unpackHeader(inmessage, h);
	if (h->messageType == MANAGEMENT){
		receivedFlag = TRUE;
		unpackManagementHeader(inmessage, manage);
		if (manage->actionField == RESPONSE)
			handleManagementResponse(inmessage,manage);
		else if (manage->actionField == ACKNOWLEDGE)
			handleManagementAck(inmessage, manage);
		else if (manage->tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS)
			handleManagementError(inmessage, manage);
	}
	printf("Not implemented yet\n");
}
