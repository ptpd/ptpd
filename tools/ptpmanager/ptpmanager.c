/**
 * @file   ptpmanager.c
 * @date   Wed May 29 11:40:38 2012
 * 
 * @brief  The code implements ptpmanager responsible for sending and 
 *         receiving management messages
 */

#include "ptpmanager.h"

/*Function to parse the command and return a command id*/
int 
getCommandId(char *command)
{

	if (strcmp(command, "send") == 0)
		return (1);
	else if (strcmp(command, "send_previous") == 0)
		return (2);
	else if (strcmp(command, "help")==0)
		return (3);
	else if (strcmp(command, "show_previous_inmessage")==0)
		return (4);
	else if (strcmp(command, "show_commonheader")==0)
		return (5);
	else if (strcmp(command, "show_managementheader")==0)
		return (6);
	else if (strcmp(command, "show_tlv")==0)
		return (7);
	else if (strcmp(command, "show_mgmtIds") == 0)
		return (8);
	else if (strcmp(command, "set_timeout") == 0)
		return (9);
	
	return (0); /* for wrong command */
}

void 
sendMessage(Octet* outmessage, char *dest)
{
	if (!netSendGeneral(outmessage, sizeof(outmessage),
		 dest))
		printf("Error sending message\n");
	else {
	
		printf("Message sent, waiting for response...\n");

		receivedFlag = FALSE;
		memset(inmessage, 0, PACKET_SIZE);
		if (netRecv(inmessage, dest)) {
			handleIncomingMsg(inmessage);
			in_sequence++;
		}
	}
}

/* 
 * The code implements ptpmanager responsible for sending and 
 * receiving management messages. Main function receives user commands and 
 * executes it. 
 */
int 
main(int argc, char *argv[ ])
{

	outmessage = (Octet*)(malloc(PACKET_SIZE));
	inmessage = (Octet*)(malloc(PACKET_SIZE));
	
	Boolean toSend = FALSE;
	Boolean toRec = TRUE;	
	out_sequence = 0;
	in_sequence = 0;
	out_length = 0;
	timeout = DEFAULT_TIMEOUT;
	netPath = (NetPath*)malloc(sizeof(NetPath));
	
	if (outmessage == NULL || inmessage == NULL || netPath == NULL){
		err(1, "Malloc error");
		exit(1);
	}
	
	char command[10];
	/* 
	 * Command line arguments would include the IP address of PTP daemon and
	 * interface on which it runs	
	 */
	if (argc != 3){
		printf("The input is not in the required format. \n\
		Please give the IP of PTP daemon and and its interface name. Eg\n\
		sudo ./ptpmanager 10.8.48.19 eth0\n");
		exit(1);
	}

	if (!netInit(argv[2])) {
		printf("failed to initialize network. (Need to be sudo user)\n");
		return (1);
	}
	
	printf("Enter command. Enter 'help' to see available commands.\ncommand>");

	scanf("%s", command);

	int j = 0;
	while (strcmp(command,"quit")){
		
		switch (getCommandId(command)){

		case 1:
				toSend = TRUE;
				memset(outmessage, 0, PACKET_SIZE);
				toSend = packOutgoingMsg(outmessage);
				
				if (toSend){
					sendMessage(outmessage,argv[1]);
					out_sequence++;
				}
			break;
		
		case 2:
			if (toSend && outmessage != NULL){
				sendMessage(outmessage,argv[1]);
				out_sequence++;
			}
			else
				printf("Outmessage does not contain valid data\n");
				
			break;
		
		case 3:
			show_help();
			break;
		
		case 4:
			handleIncomingMsg(inmessage);
			break;
		
		case 5:
			show_commonheader();
			break;
		
		case 6:
			show_managementheader();
			break;
		
		case 7:
			show_tlv();
			break;
			
		case 8:
			show_mgmtIds();
			break;
			
		case 9:
			set_timeout();
			break;
			
		default:
			printf("Invalid command. Enter 'help' to see available commands.\n");
		}

		printf("\ncommand>");
		scanf("%s", command);
	}
	
	free(outmessage);
	free(inmessage);	
	free(netPath);
	netShutdown();
	return (0);
}
