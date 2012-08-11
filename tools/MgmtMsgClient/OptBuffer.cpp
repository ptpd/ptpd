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
 * @file        OptBuffer.cpp
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       OptBuffer class implementation.
 * 
 * This class is used to store application options passed by a user.
 */

#include "OptBuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MgmtMsgClient.h"

#include "constants.h"
#include "constants_dep.h"
#include "freeing.h"

/**
 * @brief OptBuffer constructor.
 * 
 * @param appName       Name of the application.
 * 
 * The constructor allocates memory and assigns default values.
 */
OptBuffer::OptBuffer(Octet* appName) {
    this->help_arg = appName;
    
    XCALLOC(this->u_address, Octet*, MAX_ADDR_STR_LEN, sizeof(Octet));
    sprintf(this->u_address, "%s", U_ADDRESS);
    
    XCALLOC(this->u_port, Octet*, MAX_PORT_STR_LEN, sizeof(Octet));
    sprintf(this->u_port, "%s", PTP_GENERAL_PORT);  
    
    this->action_type_set = false;
    this->help_print = false;
    this->interface_set = false;
    this->mgmt_id_set = false;
    this->msg_print = false;
    this->value_set = false;
    
    this->timeout = RECV_TIMEOUT;
}

/**
 * @brief OptBuffer deconstructor.
 * 
 * The deconstructor frees memory.
 */
OptBuffer::~OptBuffer() {
    free(this->u_address);
    free(this->u_port);
    
    if (this->interface_set)
        free(this->interface);
                
    if (this->value_set)
        freePTPText(&(this->value));
}

/**
 * @brief Parse action type.
 * 
 * @param actionType    Action type to be parsed.
 */
void OptBuffer::mgmtActionTypeParser(Octet* actionType)
{
    if (!strcmp("GET", actionType))
        this->action_type = GET;
    
    else if (!strcmp("SET", actionType))
        this->action_type = SET;
    
    else if (!strcmp("RESPONSE", actionType))
        this->action_type = RESPONSE;
    
    else if (!strcmp("COMMAND", actionType))
        this->action_type = COMMAND;
    
    else if (!strcmp("ACKNOWLEDGE", actionType))
        this->action_type = ACKNOWLEDGE;
    
    else {
        ERROR("unknown actionType\n");
        exit(1);
    }
    
    this->action_type_set = true;
    return;
}

/**
 * @brief Parse management id.
 * 
 * @param mgmtId    Management id to be parsed.
 */
void OptBuffer::mgmtIdParser(Octet* mgmtId) {
    if (!strcmp("NULL_MANAGEMENT", mgmtId))
        this->mgmt_id = MM_NULL_MANAGEMENT;
    
    else if (!strcmp("CLOCK_DESCRIPTION", mgmtId))
        this->mgmt_id = MM_CLOCK_DESCRIPTION;
    
    else if (!strcmp("USER_DESCRIPTION", mgmtId))
        this->mgmt_id = MM_USER_DESCRIPTION;
    
    else if (!strcmp("SAVE_IN_NON_VOLATILE_STORAGE", mgmtId))
        this->mgmt_id = MM_SAVE_IN_NON_VOLATILE_STORAGE;
    
    else if (!strcmp("RESET_NON_VOLATILE_STORAGE", mgmtId))
        this->mgmt_id = MM_RESET_NON_VOLATILE_STORAGE;
    
    else if (!strcmp("INITIALIZE", mgmtId))
        this->mgmt_id = MM_INITIALIZE;
    
    else if (!strcmp("DEFAULT_DATA_SET", mgmtId))
        this->mgmt_id = MM_DEFAULT_DATA_SET;
    
    else if (!strcmp("CURRENT_DATA_SET", mgmtId))
        this->mgmt_id = MM_CURRENT_DATA_SET;
    
    else if (!strcmp("PARENT_DATA_SET", mgmtId))
        this->mgmt_id = MM_PARENT_DATA_SET;
    
    else if (!strcmp("TIME_PROPERTIES_DATA_SET", mgmtId))
        this->mgmt_id = MM_TIME_PROPERTIES_DATA_SET;
    
    else if (!strcmp("PORT_DATA_SET", mgmtId))
        this->mgmt_id = MM_PORT_DATA_SET;
    
    else if (!strcmp("PRIORITY1", mgmtId))
        this->mgmt_id = MM_PRIORITY1;
    
    else if (!strcmp("PRIORITY2", mgmtId))
        this->mgmt_id = MM_PRIORITY2;
    
    else if (!strcmp("DOMAIN", mgmtId))
        this->mgmt_id = MM_DOMAIN;
    
    else if (!strcmp("SLAVE_ONLY", mgmtId))
        this->mgmt_id = MM_SLAVE_ONLY;
    
    else if (!strcmp("LOG_ANNOUNCE_INTERVAL", mgmtId))
        this->mgmt_id = MM_LOG_ANNOUNCE_INTERVAL;
    
    else if (!strcmp("ANNOUNCE_RECEIPT_TIMEOUT", mgmtId))
        this->mgmt_id = MM_ANNOUNCE_RECEIPT_TIMEOUT;
    
    else if (!strcmp("LOG_SYNC_INTERVAL", mgmtId))
        this->mgmt_id = MM_LOG_SYNC_INTERVAL;
    
    else if (!strcmp("VERSION_NUMBER", mgmtId))
        this->mgmt_id = MM_VERSION_NUMBER;
    
    else if (!strcmp("ENABLE_PORT", mgmtId))
        this->mgmt_id = MM_ENABLE_PORT;
    
    else if (!strcmp("DISABLE_PORT", mgmtId))
        this->mgmt_id = MM_DISABLE_PORT;
    
    else if (!strcmp("TIME", mgmtId))
        this->mgmt_id = MM_TIME;
    
    else if (!strcmp("CLOCK_ACCURACY", mgmtId))
        this->mgmt_id = MM_CLOCK_ACCURACY;
    
    else if (!strcmp("UTC_PROPERTIES", mgmtId))
        this->mgmt_id = MM_UTC_PROPERTIES;
    
    else if (!strcmp("TRACEABILITY_PROPERTIES", mgmtId))
        this->mgmt_id = MM_TRACEABILITY_PROPERTIES;
    
    else if (!strcmp("DELAY_MECHANISM", mgmtId))
        this->mgmt_id = MM_DELAY_MECHANISM;
    
    else if (!strcmp("LOG_MIN_PDELAY_REQ_INTERVAL", mgmtId))
        this->mgmt_id = MM_LOG_MIN_PDELAY_REQ_INTERVAL;
    
    else if (!strcmp("FAULT_LOG", mgmtId))
        this->mgmt_id = MM_FAULT_LOG;
    
    else if (!strcmp("FAULT_LOG_RESET", mgmtId))
        this->mgmt_id = MM_FAULT_LOG_RESET;
    
    else if (!strcmp("TIMESCALE_PROPERTIES", mgmtId))
        this->mgmt_id = MM_TIMESCALE_PROPERTIES;
    
    else if (!strcmp("UNICAST_NEGOTIATION_ENABLE", mgmtId))
        this->mgmt_id = MM_UNICAST_NEGOTIATION_ENABLE;
    
    else if (!strcmp("PATH_TRACE_LIST", mgmtId))
        this->mgmt_id = MM_PATH_TRACE_LIST;
    
    else if (!strcmp("PATH_TRACE_ENABLE", mgmtId))
        this->mgmt_id = MM_PATH_TRACE_ENABLE;
    
    else if (!strcmp("GRANDMASTER_CLUSTER_TABLE", mgmtId))
        this->mgmt_id = MM_GRANDMASTER_CLUSTER_TABLE;
    
    else if (!strcmp("UNICAST_MASTER_TABLE", mgmtId))
        this->mgmt_id = MM_UNICAST_MASTER_TABLE;
    
    else if (!strcmp("UNICAST_MASTER_MAX_TABLE_SIZE", mgmtId))
        this->mgmt_id = MM_UNICAST_MASTER_MAX_TABLE_SIZE;
    
    else if (!strcmp("ACCEPTABLE_MASTER_TABLE", mgmtId))
        this->mgmt_id = MM_ACCEPTABLE_MASTER_TABLE;
    
    else if (!strcmp("ACCEPTABLE_MASTER_TABLE_ENABLED", mgmtId))
        this->mgmt_id = MM_ACCEPTABLE_MASTER_TABLE_ENABLED;
    
    else if (!strcmp("ACCEPTABLE_MASTER_MAX_TABLE_SIZE", mgmtId))
        this->mgmt_id = MM_ACCEPTABLE_MASTER_MAX_TABLE_SIZE;
    
    else if (!strcmp("ALTERNATE_MASTER", mgmtId))
        this->mgmt_id = MM_ALTERNATE_MASTER;
    
    else if (!strcmp("ALTERNATE_TIME_OFFSET_ENABLE", mgmtId))
        this->mgmt_id = MM_ALTERNATE_TIME_OFFSET_ENABLE;
    
    else if (!strcmp("ALTERNATE_TIME_OFFSET_NAME", mgmtId))
        this->mgmt_id = MM_ALTERNATE_TIME_OFFSET_NAME;
    
    else if (!strcmp("ALTERNATE_TIME_OFFSET_MAX_KEY", mgmtId))
        this->mgmt_id = MM_ALTERNATE_TIME_OFFSET_MAX_KEY;
    
    else if (!strcmp("ALTERNATE_TIME_OFFSET_PROPERTIES", mgmtId))
        this->mgmt_id = MM_ALTERNATE_TIME_OFFSET_PROPERTIES;
    
    else if (!strcmp("TRANSPARENT_CLOCK_DEFAULT_DATA_SET", mgmtId))
        this->mgmt_id = MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET;
    
    else if (!strcmp("TRANSPARENT_CLOCK_PORT_DATA_SET", mgmtId))
        this->mgmt_id = MM_TRANSPARENT_CLOCK_PORT_DATA_SET;
    
    else if (!strcmp("PRIMARY_DOMAIN", mgmtId))
        this->mgmt_id = MM_PRIMARY_DOMAIN;
    
    else {
        ERROR("unknown managementTLV\n");
        exit(1);
    }

    this->mgmt_id_set = true;
    return;
}
