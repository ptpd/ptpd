#include "ptpmanager.h"
#include "stdlib.h"

#define PTP_GENERAL_PORT 4201
#define PTP_EVENT_PORT 4200


int out_sequence;
int in_sequence;
NetPath * netPath;

/* Function to pack the header of outgoing message*/
void packCommonHeader(Octet *message)
{
	printf("Taking default values..\n");

//packing to be done	
}

/*Function to pack Management Header*/
void packManagementHeader(Octet *message)
{
	//Take inputs for packing Management Header for eg: actionField
	/*
	printf("Enter action of management message, i.e. SET or GET\n");
	scanf("%c",&actionField);
	*/
	printf("Taking default values...\n");
	//packing to be done with proper values, currently header is not packed properly
	MsgManagement *outgoing = (MsgManagement*)(message);
	
	outgoing->header.transportSpecific = 0x0;
	outgoing->header.messageType = MANAGEMENT;
    outgoing->header.versionPTP = 2;
	outgoing->header.domainNumber = 0;

    outgoing->header.flagField0 = 0x00;
    outgoing->header.flagField1 = 0x00;
    outgoing->header.correctionField.msb = 0;
    outgoing->header.correctionField.lsb = 0;

	memcpy(outgoing->header.sourcePortIdentity.clockIdentity, \
			"00:26:9e:ff:fe:a65b:7e", CLOCK_IDENTITY_LENGTH);	
	outgoing->header.sourcePortIdentity.portNumber = 1;
	outgoing->header.sequenceId = in_sequence+1;
	outgoing->header.controlField = 0x0; 
	outgoing->header.logMessageInterval = 0x7F;

	out_length += HEADER_LENGTH;
}

/*A function to management payload for each type of managementId. These will ask questions from user and set
	the data fields accordingly.
*/
void packMMClockDescription(); 

/*function to initialize the UDP networking stuff*/
Boolean 
netInit(char *ifaceName)
{
	int temp;
	struct in_addr interfaceAddr, netAddr;
	struct sockaddr_in addr;

	
	//printf("netInit\n");

	/* open sockets */
	if ((netPath->eventSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0
	    || (netPath->generalSock = socket(PF_INET, SOCK_DGRAM, 
					      IPPROTO_UDP)) < 0) {
		printf("failed to initalize sockets");
		return FALSE;
	}
	
	temp = 1;			/* allow address reuse */
	if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_REUSEADDR, 
		       &temp, sizeof(int)) < 0
	    || setsockopt(netPath->generalSock, SOL_SOCKET, SO_REUSEADDR, 
			  &temp, sizeof(int)) < 0) {
		printf("failed to set socket reuse\n");
	}


	/* bind sockets */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PTP_GENERAL_PORT);


//To be used for receiving management response
/*	if (bind(netPath->generalSock, (struct sockaddr *)&addr, 
		 sizeof(struct sockaddr_in)) < 0) {
		printf("failed to bind general socket");
		return FALSE;
	}

	addr.sin_port = htons(PTP_EVENT_PORT);
	if (bind(netPath->eventSock, (struct sockaddr *)&addr, 
		 sizeof(struct sockaddr_in)) < 0) {
		printf("failed to bind event socket");
		return FALSE;
	}
*/

#ifdef linux
	/*
	 * The following code makes sure that the data is only received on the specified interface.
	 * Without this option, it's possible to receive PTP from another interface, and confuse the protocol.
	 * Calling bind() with the IP address of the device instead of INADDR_ANY does not work.
	 *
	 * More info:
	 *   http://developerweb.net/viewtopic.php?id=6471
	 *   http://stackoverflow.com/questions/1207746/problems-with-so-bindtodevice-linux-socket-option
	 */
	if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_BINDTODEVICE,
			ifaceName, strlen(ifaceName)) < 0
		|| setsockopt(netPath->generalSock, SOL_SOCKET, SO_BINDTODEVICE,
			ifaceName, strlen(ifaceName)) < 0){
			
		printf("failed to call SO_BINDTODEVICE on the interface");
		return FALSE;
		
	}

#endif
	return TRUE;
}

/*Function to send management messages*/
ssize_t 
netSendGeneral(Octet * buf, UInteger16 length, char *ip)
{
	ssize_t ret;
	struct sockaddr_in addr;
	int i;
	int x = 10;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_GENERAL_PORT);
	if( (inet_pton(PF_INET, ip, &addr.sin_addr) ) == 0)
	{
		printf("\nAddress is not parsable\n");
		return 0;
	}
	else if ( (inet_pton(AF_INET, ip, &addr.sin_addr) ) < 0)
	{
		printf("\nAddress family is not supproted by this function\n");
		return 0;
	}
	
	ret = sendto(netPath->generalSock, buf, out_length, 0, \
			(struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret <= 0)
	printf("error sending uni-cast general message\n");

	return ret;
}

/*Function to receive the management response/ack/error message*/
ssize_t
netRecv(Octet *message)
{}

/*Function to pack MMClockDescription*/
void packMMClockDescription()
{}

/*Function to unpack the header of received message*/
void unpackHeader(Octet *message, MsgHeader *h)
{}

/*Function to unpack management header*/
void unpackManagementHeader(Octet *inmessage, MsgManagement *manage)
{}

/*Function to handle management response and display the required fields*/
void handleManagementResponse(Octet *inmessage, MsgManagement *manage)
{}

/*Function to handle management ack*/
void handleManagementAck(Octet *inmessage, MsgManagement *manage)
{}

/*Function to handle management error message*/
void handleManagementError(Octet *inmessage, MsgManagement *manage)
{}

/*Function to display last received message*/
void displayMessage(Octet *inmessage)
{}

/*Function to display all management messages or any one based on user input*/
void displayManagementFields()
{}

/*shutdown the network layer*/
void netShutdown()
{}

/*Function to parse the command and return a command id*/
int getCommandId(char *command)
{

	if(strcmp(command,"send") == 0)
		return 1;
	if(strcmp(command,"show_prev") == 0)
		return 2;
	if(strcmp(command,"show_fields")==0)
		return 3;
	
	return 0; //wrong command
}



int main(int argc, char *argv[ ])
{

	Octet *outmessage = (Octet*)(malloc(PACKET_SIZE));
	memset(outmessage,0,sizeof(MANAGEMENT_LENGTH));
	Octet *inmessage = (Octet*)(malloc(PACKET_SIZE));
	memset(inmessage,0,sizeof(MANAGEMENT_LENGTH));
	
	netPath = (NetPath*)malloc(sizeof(NetPath));
	unsigned short tlvtype;
	out_sequence = 0;
	in_sequence = 0;
	out_length = 0;
	int managementId;
	char ch;
	

	char command[10];
	// Command line arguments would include the IP address of PTP daemon and
	// on which it runs	
	if(argc !=3)
	{
		printf("\nThe input is not in the required format. Please give the IP of PTP daemon and and its interface name\n");
		exit(0);
	}

	if (!netInit(argv[2])) {
		printf("failed to initialize network\n");
		return;
	}
	
	printf("Enter your command: 'send' for sending management message, 'show_previous' to display received response, 'show_fields' to see management message fields\n>");

	command[0] = '\0';
	scanf("%s",command);

	while(strcmp(command,"quit"))
	{
		
	
		switch(getCommandId(command))
		{

			case 1:
				{
					printf("Packing Header..\n");

					packCommonHeader(outmessage);	
					
					printf("Packing Management Header...\n");		
					packManagementHeader(outmessage);
					
					
					printf("Please enter Tlv type and management id\n");
					scanf("%d, %d",&tlvtype,&managementId);
					
					if(tlvtype==TLV_MANAGEMENT)
					{
						switch(managementId)
						{
							case MM_CLOCK_DESCRIPTION:
								packMMClockDescription(); 	
								break;
							//similarly for other managementIds	
						}
					}

					printf("Sending message....\n");
					
					if (!netSendGeneral(outmessage, sizeof(outmessage), argv[1]))
					{
						printf("Error sending message\n");
					}
					else
					{
						printf("Management message sent, waiting for response...\n");
					}

					receivedFlag = FALSE;
					for (;;) 
					{
					
					//TODO wait for some time to receive a response or ack, if not received till
					// timeout send the management message again, Use receivedFlag for this.

						if (netRecv(inmessage)) {
							MsgHeader *h; MsgManagement *manage;
							unpackHeader(inmessage, h);
							if(h->messageType == MANAGEMENT)
							{
								receivedFlag = TRUE;
								unpackManagementHeader(inmessage, manage);
								if(manage->actionField = RESPONSE)
								{
									handleManagementResponse(inmessage, manage);
						
								}
								else if(manage->actionField = RESPONSE)
								{
									handleManagementAck(inmessage, manage);
								}
								else if(manage->tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS)
								{
									handleManagementError(inmessage, manage);
								}
							}
						}
					}

				}
				printf("\n");
				break;
			
			case 2:
				displayMessage(inmessage);
				break;
			
			case 3:
				displayManagementFields();
				break;
			
			default:
				printf("Invalid command\n>");
		}
		
		printf(">");
		scanf("%s\n",command);
	}
	
	
	netShutdown();
}


