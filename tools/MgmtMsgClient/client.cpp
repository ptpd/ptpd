/** 
 * @file        client.cpp
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       Main function of the application.
 * 
 * This function is used as a glue for all of the functionalities that
 * the application is supposed to deliver.
 */

#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "IncomingManagementMessage.h"
#include "MgmtMsgClient.h"
#include "OutgoingManagementMessage.h"

#include "constants.h"
#include "constants_dep.h"
#include "datatypes_dep.h"
#include "help.h"
#include "network.h"

/**
 * This function is used to deliver all of requested actions to a user.
 * 
 * @param optBuf        A buffer with arguments passed from a user.
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
        ERROR("Interface name not defined\n");
        exit(1);
    }
    
    if (!optBuf->mgmt_id_set) {
        ERROR("Management TLV not defined\n");
        exit(1);
    }
    
    if (!optBuf->action_type_set) {
        ERROR("Action type not defined\n");
        exit(1);
    }
    
    bool isNullManagement = true ? optBuf->mgmt_id == MM_NULL_MANAGEMENT : false;
    
    sockFd = initNetwork(optBuf->u_address, optBuf->u_port, optBuf->interface, &unicastAddress);
    
    /* send <--> receive */
    Octet *buf = (Octet*)(malloc(PACKET_SIZE));
    
    OutgoingManagementMessage *outMessage = new OutgoingManagementMessage(buf, optBuf);
    free(outMessage);
    
    sendMessage(sockFd, buf, PACKET_SIZE, unicastAddress);
    
    memset(buf, 0, PACKET_SIZE);
    
    receiveMessage(sockFd, buf, PACKET_SIZE, &fromAddr, &fromLen, isNullManagement, optBuf->timeout);
    
    IncomingManagementMessage *inMessage = new IncomingManagementMessage(buf, optBuf);
    free(inMessage);  
    
    free(buf);
    
    disableNetwork(sockFd, &unicastAddress);
    
    return;
}