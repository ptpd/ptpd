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
 * This method will be used to initialize socket communication between client
 * and server.
 * 
 * @param hostName Hostname or IP address of the server.
 * @param port Port number that the server is listening on
 */
int initNetwork(char* hostName, char* port) {
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

    //make a socket with characteristics identical to server's
    sockFd = socket(sockRes->ai_family, sockRes->ai_socktype, sockRes->ai_protocol);

    if (sockFd == -1) {
        perror("socket()");
        exit(1);
    }
    
    /*//set reuse port to on to allow multiple binds per host
    error = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	
    if (error != 0) {		
        perror("setsockopt()");		
        exit(1);	
    }
    
    //bind to the port
    error = bind(sockFd, sockRes->ai_addr, sockRes->ai_addrlen);
	
    if (error != 0) {		
        perror("bind()");		
        exit(1);	
    }*/
   
    //A test send message
    /*int numbytes;
    char msg[10] = "ala";
    if ((numbytes = sendto(sockFd, msg, strlen(msg), 0, sockRes->ai_addr, sockRes->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }*/

    freeaddrinfo(sockRes);
    
    printf("Client connected to %s on port %s\n", hostName, port);
    
    return sockFd;
}

void disableNetwork(int sockFd) {
    close(sockFd);
}

/* TODO: Send and receive methods will be implemented when the socket structure
 * will be ready */ 
void sendMessage() {
    
}

void receiveMessage() {
    
}
