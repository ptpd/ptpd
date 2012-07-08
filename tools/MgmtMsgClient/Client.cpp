/** 
 * @file Client.cpp
 * @author Tomasz Kleinschmidt
 * 
 * @brief Client class implementation.
 * 
 * This class will be used as a glue for all of the functionalities that
 * the application is supposed to deliver.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Client.h"
#include "Help.h"
#include "Network.h"
#include "IncomingManagementMessage.h"
#include "OutgoingManagementMessage.h"

#include "constants.h"
#include "datatypes_dep.h"

/**
 * This method will be used to deliver all of the requested actions to the
 * user.
 * 
 * @param optBuf        A buffer with arguments passed from the user.
 */
void mainClient(OptBuffer* optBuf) {
    int sockFd;
    socklen_t fromLen;
    struct addrinfo* unicastAddress;
    struct sockaddr_storage fromAddr;
    
    if (optBuf->help_print) {
        printHelp(optBuf->help_arg);
        return;
    }
    
    if (optBuf->msg_print) {
        printMgmtMsgsList();
        return;
    }
    
    sockFd = initNetwork(optBuf->u_address, optBuf->u_port, &unicastAddress);
    
    /* send <--> receive will be here */
    Octet *buf = (Octet*)(malloc(PACKET_SIZE));
    
    OutgoingManagementMessage *outMessage = new OutgoingManagementMessage(buf, optBuf);
    free(outMessage);
    
    sendMessage(sockFd, buf, PACKET_SIZE, unicastAddress);
    
    //free(buf);
    memset(buf, 0, PACKET_SIZE);
    
    receiveMessage(sockFd, buf, PACKET_SIZE, &fromAddr, &fromLen);
    
    IncomingManagementMessage *inMessage = new IncomingManagementMessage(buf, optBuf);
    free(inMessage);
    
    //printf("Received:\n\n%s", buf);
    
    free(buf);
    
    /*Octet msg[300];
    receiveMessage(sockFd, msg, strlen(msg), &fromAddr, &fromLen);*/
    
    disableNetwork(sockFd, &unicastAddress);
    
    return;
}