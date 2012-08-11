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
 * @file        Help.cpp
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       Help messages.
 * 
 * Functions to print help messages.
 */

#include "help.h"

#include <stdio.h>

#include "constants.h"
#include "constants_dep.h"

/**
 * @brief Print help message.
 * 
 * @param appName       Name of the application.
 */
void printHelp(const char* appName) {
    printf("PTPd Management Message Client usage: %s [options]\n"
            "Options:\n"
            "   -a --address [address]                  IPv4 address of a server (default set to %s)\n"
            "   -c --action [action]                    Type of action to be handled by a management message\n"
            "   -h --help                               Display this message\n"
            "   -i --interface [name]                   Bind to network interface of given name\n"
            "   -m --message [type]                     Handle management message of given type\n"
            "   -p --port [port]                        Port number a server is listening on (default set to %s)\n"
            "   -t --timeout [timeout]                  Time in seconds to wait for a message to be received (default set to %us)\n"
            "   -v --value [value]                      Value for a 'SET' or a 'COMMAND' action\n"
            "   --verbose                               Print additional status messages (useful for debugging)\n"
            "\n"
            "Type '%s {-m --message} print' to print a list of management messages and assigned actions.\n",
            appName, U_ADDRESS, PTP_GENERAL_PORT, RECV_TIMEOUT, appName);
}

/**
 * @brief Print a brief list of management messages and assigned actions.
 */
void printMgmtMsgsList() {
    printf("managementId name                   \tmanagementId value (hex)      \tAllowed actions               \tApplies to\n"
            "\n"
            "===Applicable to all node types===================================================================================\n"
            "NULL_MANAGEMENT                    \t0000                          \tGET, SET, COMMAND             \tport\n"
            "CLOCK_DESCRIPTION                  \t0001                          \tGET                           \tport\n"
            "USER_DESCRIPTION                   \t0002                          \tGET, SET                      \tclock\n"
            "SAVE_IN_NON_VOLATILE_STORAGE       \t0003                          \tCOMMAND                       \tclock\n"
            "RESET_NON_VOLATILE_STORAGE         \t0004                          \tCOMMAND                       \tclock\n"
            "INITIALIZE                         \t0005                          \tCOMMAND                       \tclock\n"
            "FAULT_LOG                          \t0006                          \tGET                           \tclock\n"
            "FAULT_LOG_RESET                    \t0007                          \tCOMMAND                       \tclock\n"
            "\n"
            "===Applicable to ordinary and boundary clocks=====================================================================\n"
            "DEFAULT_DATA_SET                   \t2000                          \tGET                           \tclock\n"
            "CURRENT_DATA_SET                   \t2001                          \tGET                           \tclock\n"
            "PARENT_DATA_SET                    \t2002                          \tGET                           \tclock\n"
            "TIME_PROPERTIES_DATA_SET           \t2003                          \tGET                           \tclock\n"
            "PORT_DATA_SET                      \t2004                          \tGET                           \tport\n"
            "PRIORITY1                          \t2005                          \tGET, SET                      \tclock\n"
            "PRIORITY2                          \t2006                          \tGET, SET                      \tclock\n"
            "DOMAIN                             \t2007                          \tGET, SET                      \tclock\n"
            "SLAVE_ONLY                         \t2008                          \tGET, SET                      \tclock\n"
            "LOG_ ANNOUNCE_INTERVAL             \t2009                          \tGET, SET                      \tport\n"
            "ANNOUNCE_RECEIPT_TIMEOUT           \t200A                          \tGET, SET                      \tport\n"
            "LOG_ SYNC_INTERVAL                 \t200B                          \tGET, SET                      \tport\n"
            "VERSION_NUMBER                     \t200C                          \tGET, SET                      \tport\n"
            "ENABLE_PORT                        \t200D                          \tCOMMAND                       \tport\n"
            "DISABLE_PORT                       \t200E                          \tCOMMAND                       \tport\n"
            "TIME                               \t200F                          \tGET, SET                      \tclock\n"
            "CLOCK_ACCURACY                     \t2010                          \tGET, SET                      \tclock\n"
            "UTC_PROPERTIES                     \t2011                          \tGET, SET                      \tclock\n"
            "TRACEABILITY_PROPERTIES            \t2012                          \tGET, SET                      \tclock\n"
            "TIMESCALE_PROPERTIES               \t2013                          \tGET, SET                      \tclock\n"
            "UNICAST_NEGOTIATION_ENABLE         \t2014                          \tGET, SET                      \tport\n"
            "PATH_TRACE_LIST                    \t2015                          \tGET                           \tclock\n"
            "PATH_TRACE_ENABLE                  \t2016                          \tGET, SET                      \tclock\n"
            "GRANDMASTER_CLUSTER_TABLE          \t2017                          \tGET, SET                      \tclock\n"
            "UNICAST_MASTER_TABLE               \t2018                          \tGET, SET                      \tport\n"
            "UNICAST_MASTER_MAX_TABLE_SIZE      \t2019                          \tGET                           \tport\n"
            "ACCEPTABLE_MASTER_TABLE            \t201A                          \tGET, SET                      \tclock\n"
            "ACCEPTABLE_MASTER_TABLE_ENABLED    \t201B                          \tGET, SET                      \tport\n"
            "ACCEPTABLE_MASTER_MAX_TABLE_SIZE   \t201C                          \tGET                           \tclock\n"
            "ALTERNATE_MASTER                   \t201D                          \tGET, SET                      \tport\n"
            "ALTERNATE_TIME_OFFSET_ENABLE       \t201E                          \tGET, SET                      \tclock\n"
            "ALTERNATE_TIME_OFFSET_NAME         \t201F                          \tGET, SET                      \tclock\n"
            "ALTERNATE_TIME_OFFSET_MAX_KEY      \t2020                          \tGET                           \tclock\n"
            "ALTERNATE_TIME_OFFSET_PROPERTIES   \t2021                          \tGET, SET                      \tclock\n"
            "\n"
            "===Applicable to transparent clocks==============================================================================\n"
            "TRANSPARENT_CLOCK_DEFAULT_DATA_SET \t4000                          \tGET                           \tclock\n"
            "TRANSPARENT_CLOCK_PORT_DATA_SET    \t4001                          \tGET                           \tport\n"
            "PRIMARY_DOMAIN                     \t4002                          \tGET, SET                      \tclock\n"
            "\n"
            "===Applicable to ordinary, boundary, and transparent clocks======================================================\n"
            "DELAY_MECHANISM                    \t6000                          \tGET, SET                      \tport\n"
            "LOG_MIN_ PDELAY_REQ_INTERVAL       \t6001                          \tGET, SET                      \tport\n");
;}