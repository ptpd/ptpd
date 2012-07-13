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

#include <unistd.h>
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
    
    if (!optBuf->interface_set) {
        printf("ERROR: Interface name not defined\n");
        exit(1);
    }
    
    if (!optBuf->mgmt_id_set) {
        printf("ERROR: managementTLV not defined\n");
        exit(1);
    }
    
    if (!optBuf->action_type_set) {
        printf("ERROR: actionType not defined\n");
        exit(1);
    }
    
    sockFd = initNetwork(optBuf->u_address, optBuf->u_port, optBuf->interface, &unicastAddress);
    
    /* send <--> receive */
    Octet *buf = (Octet*)(malloc(PACKET_SIZE));
    
    OutgoingManagementMessage *outMessage = new OutgoingManagementMessage(buf, optBuf);
    free(outMessage);
    
    sendMessage(sockFd, buf, PACKET_SIZE, unicastAddress);
    
    memset(buf, 0, PACKET_SIZE);
    
    receiveMessage(sockFd, buf, PACKET_SIZE, &fromAddr, &fromLen);
    
    IncomingManagementMessage *inMessage = new IncomingManagementMessage(buf, optBuf);
    free(inMessage);  
    
    free(buf);
    
    disableNetwork(sockFd, &unicastAddress);
    
    return;
}