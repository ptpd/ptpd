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
OptBuffer::OptBuffer(char* appName) {
    this->help_arg = appName;
    
    this->u_address = (char*)calloc(MAX_ADDR_STR_LEN, sizeof(char));
    sprintf(this->u_address, "%s", U_ADDRESS);
    
    this->u_port = (char*)calloc(MAX_PORT_STR_LEN, sizeof(char));
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
void OptBuffer::mgmtActionTypeParser(char* actionType)
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
void OptBuffer::mgmtIdParser(char* mgmtId) {
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
