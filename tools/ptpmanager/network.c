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
 * @file   network.c
 * @date   Thurs July 5 01:24:10 2012 IST
 * @author Himanshu Singh
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
	if ((generalSock = socket(PF_INET, SOCK_DGRAM, 
					      IPPROTO_UDP)) < 0) {
		printf("failed to initalize sockets");
		return (FALSE);
	}
	
	temp = 1;			/* allow address reuse */
	if (setsockopt(generalSock, SOL_SOCKET, SO_REUSEADDR, 
			  &temp, sizeof(int)) < 0) {
		printf("failed to set socket reuse\n");
	}


	/* bind sockets */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PTP_GENERAL_PORT);

	if (bind(generalSock, (struct sockaddr *)&addr, 
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
	if (setsockopt(generalSock, SOL_SOCKET, SO_BINDTODEVICE,
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

	ret = sendto(generalSock, buf, out_length, 0, 
			(struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret <= 0)
	printf("error sending uni-cast general message\n");

	return (ret);
}

/*Function to set timeout as provided by user */
void
set_timeout()
{
	printf("Timeout value in seconds? ");
	scanf("%d",&timeout);
}

/* Function to receive the management response/ack/error message */
ssize_t
netRecv(Octet *message, char *dest)
{
	ssize_t ret = 0;
	char srcIp[16];
	int try;
	int ret_select;
	fd_set socks;
	struct sockaddr_in client_addr;
   	socklen_t len = sizeof(client_addr);
   	struct timeval timeout_val;
    
    FD_ZERO(&socks);
    FD_SET(generalSock, &socks);
    
    timeout_val.tv_sec = timeout;
    timeout_val.tv_usec = 0;
	
	if ((ret_select = select(generalSock + 1, &socks, NULL, NULL, 
			&timeout_val)) != 0) {
		ret = recvfrom(generalSock, message, PACKET_SIZE , 0 , 
							(struct sockaddr *)&client_addr, &len);
		if (ret == 0 || (strcmp(inet_ntop(AF_INET, &(client_addr.sin_addr), srcIp, 
				sizeof(srcIp)), dest) == 0))
			return (ret);
		else
			printf("false\n");
	}
	else if (ret_select < 0){
		printf("Network error (select)\n");
		return 0;
	}
	else {
		printf("Timed out. Please check that server is reachable and try again."
			" You can use 'send_previous'\n");
		return 0;
	}
}

/*shutdown the network layer*/
Boolean 
netShutdown()
{
	if (generalSock > 0)
		close(generalSock);
	generalSock = -1;

	return TRUE;
}
