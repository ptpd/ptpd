/** 
 * @file Help.cpp
 * @author Tomasz Kleinschmidt
 * 
 * @brief Help class implementation
 * 
 * This class will be used to print help messages.
 */

#include <stdio.h>

#include "Help.h"
#include "constants.h"
#include "constants_dep.h"

/**
 * This method will be used to print the help message.
 * 
 * @param appName       The name of the application.
 */
void printHelp(const char* appName) {
    printf("PTPd Management Message Client usage: %s [options]\n"
            "Options:\n"
            "   -a --address [address]                  Set IPv4 address of the server (default set to %s)\n"
            "   -m --message [type] [action] <value>    Send management message of given type and value\n"
            "   -h --help                               Display this message\n"
            "   -p --port [port]                        Set port number the server is listening on (default set to %s)\n\n"
            "Type '%s {-m --message} print' to print a list of management messages and assigned actions.\n",
            appName, U_ADDRESS, PTP_GENERAL_PORT, appName);
}

/**
 * This method will be used to print a brief list of management messages and assigned actions
 */
void printMgmtMsgsList() {
    // TODO: Table 40 - managementId values
    printf("Management Message Name             \tAllowed actions\n"
            "NULL_MANAGEMENT                    \tGET, SET, COMMAND\n");
;}