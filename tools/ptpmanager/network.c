/**
 * @file   network.c
 * @date   Thurs July 5 01:24:10 2012
 * 
 * @brief  Functions to intialize network, send and receive packets and close 
 *         network.
 */

#include "ptpmanager.h"


/*function to initialize the UDP networking stuff*/
Boolean 
netInit(char *ifaceName)
{
	int temp;
	struct sockaddr_in addr;
	
	/* open sockets */
	if ((netPath->generalSock = socket(PF_INET, SOCK_DGRAM, 
					      IPPROTO_UDP)) < 0) {
		printf("failed to initalize sockets");
		return (FALSE);
	}
	
	temp = 1;			/* allow address reuse */
	if (setsockopt(netPath->generalSock, SOL_SOCKET, SO_REUSEADDR, 
			  &temp, sizeof(int)) < 0) {
		printf("failed to set socket reuse\n");
	}


	/* bind sockets */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PTP_GENERAL_PORT);

	if (bind(netPath->generalSock, (struct sockaddr *)&addr, 
		 sizeof(struct sockaddr_in)) < 0) {
		printf("failed to bind general socket");
		return (FALSE);
	}


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
	if (setsockopt(netPath->generalSock, SOL_SOCKET, SO_BINDTODEVICE,
			ifaceName, strlen(ifaceName)) < 0){
			
		printf("failed to call SO_BINDTODEVICE on the interface\n");
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
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_GENERAL_PORT);
	if ((inet_pton(PF_INET, ip, &addr.sin_addr) ) == 0)	{
		printf("\nAddress is not parsable\n");
		return (0);
	} else if ( (inet_pton(AF_INET, ip, &addr.sin_addr) ) < 0){
		printf("\nAddress family is not supproted by this function\n");
		return (0);
	}
	
	ret = sendto(netPath->generalSock, buf, out_length, 0, 
			(struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	
	if (ret <= 0)
	printf("error sending uni-cast general message\n");

	return (ret);
}

/* Function to receive the management response/ack/error message */
ssize_t
netRecv(Octet *message, char *src)
{
	ssize_t ret = 0;
	char srcIp[16];
	int try;
	struct sockaddr_in client_addr;
   	socklen_t len = sizeof(client_addr);
	
	for (try = 0;try < 3; try++){
		ret = recvfrom(netPath->generalSock, message, PACKET_SIZE , 0 , 
							(struct sockaddr *)&client_addr, &len);
		if (ret == 0 || (strcmp(inet_ntop(AF_INET, &(client_addr.sin_addr), srcIp, 
				sizeof(srcIp)), src) == 0))
			return (ret);
		else
			printf("false\n");
	}
}

/*shutdown the network layer*/
Boolean 
netShutdown()
{
	if (netPath->generalSock > 0)
		close(netPath->generalSock);
	netPath->generalSock = -1;

	return TRUE;
}
