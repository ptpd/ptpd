/** 
 * @file OptBuffer.cpp
 * @author Tomasz Kleinschmidt
 * 
 * @brief OptBuffer class implementation
 */

#include <stdio.h>
#include <stdlib.h>

#include "OptBuffer.h"
#include "const.h"

/**
 * The constructor should allocate memory for all the options and assign default values
 */
OptBuffer::OptBuffer(char* appName) {
    this->help_arg = appName;
    
    this->u_address = (char*)calloc(MAX_ADDR_STR_LEN, sizeof(char));
    sprintf(this->u_address, "%s", U_ADDRESS);
    
    this->u_port = (char*)calloc(MAX_PORT_STR_LEN, sizeof(char));
    sprintf(this->u_port, "%s", PTP_GENERAL_PORT);  
    
    this->msg_print = false;
    this->help_print = false;
}

/**
 * The deconstructor should free the memory
 */
OptBuffer::~OptBuffer() {
    free(this->u_address);
    free(this->u_port);
}
