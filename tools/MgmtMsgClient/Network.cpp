/** 
 * @file Network.cpp
 * @author Tomasz Kleinschmidt
 * 
 * @brief Network class implementation.
 * 
 * This class will be used to initialize and manage network connections
 * between the client and the server.
 */

#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Network.h"

/**
 * This method will be used to initialize socket communication between the client
 * and the server.
 * 
 * @param hostName      The hostname or the IP address of the server.
 * @param port          The port number that the server is listening on.
 */
int initNetwork(char* hostName, char* port, struct addrinfo** addrInfo) {
    int error, sockFd, yes = 1;
    struct addrinfo sockHints, *sockRes;
    
    memset(&sockHints, 0, sizeof(sockHints)); 
            
    sockHints.ai_socktype = SOCK_DGRAM;
    sockHints.ai_family = AF_INET;
	
    //get host address data
    error = getaddrinfo(hostName, port, &sockHints, &sockRes);
	
    if (error) {		
        errx(1, "%s", gai_strerror(error));		
        exit(1);	
    }
    
    //store address data for future use
    *addrInfo = sockRes;

    //make a socket with characteristics identical to server's
    sockFd = socket(sockRes->ai_family, sockRes->ai_socktype, sockRes->ai_protocol);

    if (sockFd == -1) {
        perror("socket()");
        exit(1);
    }
    
    //set reuse port to on to allow multiple binds per host
    error = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	
    if (error != 0) {		
        perror("setsockopt()");		
        exit(1);	
    }
    
    /* TODO: Bind to the PTP port needs root privileges.
     * Add an adequate verification and notification. */ 
    
    //bind to the port
    error = bind(sockFd, sockRes->ai_addr, sockRes->ai_addrlen);
	
    if (error != 0) {		
        perror("bind()");		
        exit(1);	
    }
    
    printf("Client connected to %s on port %s\n", hostName, port);
    
    return sockFd;
}

/**
 * This method will be used to free the address specification data and close
 * the socket.
 * 
 * @param sockFd        A descriptor identifying a socket.
 * @param addrInfo      A structure containing the destination address.
 */
void disableNetwork(int sockFd, struct addrinfo** addrInfo) {
    freeaddrinfo(*addrInfo);
    close(sockFd);
}

/**
 * This method will be used to send messages to the server.
 * 
 * @param sockFd        A descriptor identifying a socket.
 * @param buf           A buffer containing the message to be sent.
 * @param length        Specifies the size of the message.
 * @param addrInfo      A structure containing the destination address.
 */
void sendMessage(int sockFd, Octet* buf, UInteger16 length, struct addrinfo* addrInfo) {
    ssize_t ret;
    
    if ((ret = sendto(sockFd, buf, length, 0, addrInfo->ai_addr, addrInfo->ai_addrlen)) <= 0) {
        perror("send()");
	exit(1);
    }
}

/**
 * This method will be used to free the address specification data and close
 * the socket.
 * 
 * @param sockFd        A descriptor identifying a bound socket.
 * @param buf           A buffer for the incoming data.
 * @param length        The length of the buffer.
 * @param addr          A buffer in a sockaddr structure that will hold the source address.
 * @param len           A pointer to the size of the source address buffer.
 */
void receiveMessage(int sockFd, Octet* buf, UInteger16 length, struct sockaddr_storage* addr, socklen_t* len) {
    ssize_t ret;
    
    *len = sizeof(*addr);
    
    if ((ret = recvfrom(sockFd, buf, length, 0, (struct sockaddr *)addr, len)) == -1) {
        perror("recvfrom()");
        exit(1);
    }
}
