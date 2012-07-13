/** 
 * @file        Display.cpp
 * @author      Tomasz Kleinschmidt
 *
 * @brief       Display functions.
 */

#include "display.h"

#include <stdio.h>

/**@brief Display an Integer64 type*/
void integer64_display(Integer64 * bigint)
{
	printf("Integer 64 : \n");
	printf("LSB : %u\n", bigint->lsb);
	printf("MSB : %d\n", bigint->msb);
}

/**@brief Display a Clockidentity Structure*/
void clockIdentity_display(ClockIdentity clockIdentity)
{
    printf("ClockIdentity : %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
        clockIdentity[0], clockIdentity[1], clockIdentity[2],
        clockIdentity[3], clockIdentity[4], clockIdentity[5],
        clockIdentity[6], clockIdentity[7]
    );
}

/**@brief Display a Portidentity Structure*/
void portIdentity_display(PortIdentity * portIdentity)
{
    clockIdentity_display(portIdentity->clockIdentity);
    printf("port number : %d \n", portIdentity->portNumber);
}


/**@brief Display Header message*/
void msgHeader_display(MsgHeader * header)
{
	printf("\nMessage header : \n");
	printf("\n");
	printf("transportSpecific : %d\n", header->transportSpecific);
	printf("messageType : %d\n", header->messageType);
	printf("versionPTP : %d\n", header->versionPTP);
	printf("messageLength : %d\n", header->messageLength);
	printf("domainNumber : %d\n", header->domainNumber);
	printf("FlagField %02hhx:%02hhx\n", header->flagField0, header->flagField1);
	printf("CorrectionField : \n");
	integer64_display(&header->correctionField);
	printf("SourcePortIdentity : \n");
	portIdentity_display(&header->sourcePortIdentity);
	printf("sequenceId : %d\n", header->sequenceId);
	printf("controlField : %d\n", header->controlField);
	printf("logMessageInterval : %d\n", header->logMessageInterval);
	printf("\n");
}

/**@brief Display Management message*/
void msgManagement_display(MsgManagement * manage)
{
    printf("\nManagement Message : \n");
    printf("\n");
    printf("targetPortIdentity : \n");
    portIdentity_display(&manage->targetPortIdentity);
    printf("startingBoundaryHops : %d \n", manage->startingBoundaryHops);
    printf("boundaryHops : %d \n", manage->boundaryHops);
    printf("actionField : %d\n", manage->actionField);
}

