/*-
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments,
 *                         Tomasz Kleinschmidt
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
 * @file        client.cpp
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       Main function of the application.
 * 
 * This function is used as a glue for all of the functionalities that
 * the application is supposed to deliver.
 */

#include "client.h"

#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef linux
#include <netinet/in.h>
#endif

#include "IncomingManagementMessage.h"
#include "MgmtMsgClient.h"
#include "OutgoingManagementMessage.h"
#include "SequnceIdConfiguration.h"

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
    bool ret;
    int sockFd;
    Octet *buf;
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
    findIface(sockFd, optBuf->interface, optBuf->hw_address);
    
    SequnceIdConfiguration *seqIdCfg = new SequnceIdConfiguration(optBuf);
    delete(seqIdCfg);
    
    /* send <--> receive */
    XMALLOC(buf, Octet*, PACKET_SIZE);
    
    OutgoingManagementMessage *outMessage = new OutgoingManagementMessage(buf, optBuf);
    delete(outMessage);
    
    sendMessage(sockFd, buf, PACKET_SIZE, unicastAddress);
    
    memset(buf, 0, PACKET_SIZE);
    
    /* initiate message receiveing until a received message is a mangement message
     * or timeout excedeed
     */
    time_t timeout = time(NULL) + optBuf->timeout;
    do {
        receiveMessage(sockFd, buf, PACKET_SIZE, &fromAddr, &fromLen, isNullManagement, optBuf->timeout);

        IncomingManagementMessage *inMessage = new IncomingManagementMessage(buf, optBuf);
        ret = inMessage->handleManagement(optBuf, buf, inMessage->incoming);
        delete(inMessage);

        memset(buf, 0, PACKET_SIZE);
        
        if (ret == TRUE)
            break;
        
    } while (time(NULL) <= timeout);
    
    if (ret == FALSE)
        printf("timeout exceeded...\n");
    
    free(buf);
    
    disableNetwork(sockFd, &unicastAddress);
    
    return;
}

