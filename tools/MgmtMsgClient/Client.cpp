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

#include "Client.h"
#include "Help.h"
#include "Network.h"
#include "OptBuffer.h"

/**
 * This method will be used to deliver all of the requested actions to the
 * user.
 * 
 * @param optBuf Buffer with arguments passed from the user.
 */
void mainClient(OptBuffer* optBuf) {
    int sockFd;
    
    if (optBuf->help_print) {
        printHelp(optBuf->help_arg);
        return;
    }
    
    if (optBuf->msg_print) {
        printMgmtMsgsList();
        return;
    }
    
    sockFd = initNetwork(optBuf->u_address, optBuf->u_port);
    
    /* send <--> receive will be here */
    
    disableNetwork(sockFd);
    
    return;
}