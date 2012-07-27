/** 
 * @file        Display.cpp
 * @author      Tomasz Kleinschmidt
 *
 * @brief       Display functions.
 */

#include "display.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/**@brief Display MAC address*/
void clockUUID_display(Octet * sourceUuid)
{
    printf(
        "sourceUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
        sourceUuid[0], sourceUuid[1], sourceUuid[2], sourceUuid[3],
        sourceUuid[4], sourceUuid[5]
    );
}

/**@brief Display a Portidentity Structure*/
void portIdentity_display(PortIdentity * portIdentity)
{
    clockIdentity_display(portIdentity->clockIdentity);
    printf("port number : %d \n", portIdentity->portNumber);
}

/**@brief Display PTPText Structure*/
void PTPText_display(PTPText *p)
{
    /* Allocate new memory to append null-terminator to the text field.
        * This allows printing the textField as a string
        */
    Octet *str;
    //XMALLOC(str, p->lengthField + 1);
    str = (Octet*) malloc (p->lengthField + 1);
    memcpy(str, p->textField, p->lengthField);
    str[p->lengthField] = '\0';
    printf("    lengthField : %d \n", p->lengthField);
    printf("    textField : %s \n", str);
    free(str);
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

/**
 * @brief Display ManagementTLV Clock Description message
 */
void mMClockDescription_display(MMClockDescription *clockDescription)
{
	printf("Clock Description ManagementTLV message \n");
	printf("clockType0 : %d \n", clockDescription->clockType0);
	printf("clockType1 : %d \n", clockDescription->clockType1);
	printf("physicalLayerProtocol : \n");
	PTPText_display(&clockDescription->physicalLayerProtocol);
	printf("physicalAddressLength : %d \n", clockDescription->physicalAddress.addressLength);
	if(clockDescription->physicalAddress.addressField) {
		printf("physicalAddressField : \n");
		clockUUID_display(clockDescription->physicalAddress.addressField);
	}
	printf("protocolAddressNetworkProtocol : %d \n", clockDescription->protocolAddress.networkProtocol);
	printf("protocolAddressLength : %d \n", clockDescription->protocolAddress.addressLength);
	if(clockDescription->protocolAddress.addressField) {
		printf("protocolAddressField : %d.%d.%d.%d \n",
			(UInteger8)clockDescription->protocolAddress.addressField[0],
			(UInteger8)clockDescription->protocolAddress.addressField[1],
			(UInteger8)clockDescription->protocolAddress.addressField[2],
			(UInteger8)clockDescription->protocolAddress.addressField[3]);
	}
	printf("manufacturerIdentity0 : %d \n", clockDescription->manufacturerIdentity0);
	printf("manufacturerIdentity1 : %d \n", clockDescription->manufacturerIdentity1);
	printf("manufacturerIdentity2 : %d \n", clockDescription->manufacturerIdentity2);
	printf("productDescription : \n");
	PTPText_display(&clockDescription->productDescription);
	printf("revisionData : \n");
	PTPText_display(&clockDescription->revisionData);
	printf("userDescription : \n");
	PTPText_display(&clockDescription->userDescription);
	printf("profileIdentity0 : %d \n", clockDescription->profileIdentity0);
	printf("profileIdentity1 : %d \n", clockDescription->profileIdentity1);
	printf("profileIdentity2 : %d \n", clockDescription->profileIdentity2);
	printf("profileIdentity3 : %d \n", clockDescription->profileIdentity3);
	printf("profileIdentity4 : %d \n", clockDescription->profileIdentity4);
	printf("profileIdentity5 : %d \n", clockDescription->profileIdentity5);
}

void mMUserDescription_display(MMUserDescription* userDescription)
{
	/* TODO: implement me */
}

void mMInitialize_display(MMInitialize* initialize)
{
	/* TODO: implement me */
}

//void mMDefaultDataSet_display(MMDefaultDataSet* defaultDataSet)
//{
//	/* TODO: implement me */
//}
//
//void mMCurrentDataSet_display(MMCurrentDataSet* currentDataSet)
//{
//	/* TODO: implement me */
//}
//
//void mMParentDataSet_display(MMParentDataSet* parentDataSet)
//{
//	/* TODO: implement me */
//}
//
//void mMTimePropertiesDataSet_display(MMTimePropertiesDataSet* timePropertiesDataSet)
//{
//	/* TODO: implement me */
//}
//
//void mMPortDataSet_display(MMPortDataSet* portDataSet)
//{
//	/* TODO: implement me */
//}
//
//void mMPriority1_display(MMPriority1* priority1)
//{
//	/* TODO: implement me */
//}
//
//void mMPriority2_display(MMPriority2* priority2)
//{
//	/* TODO: implement me */
//}
//
//void mMDomain_display(MMDomain* domain)
//{
//	/* TODO: implement me */
//}
//
//void mMLogAnnounceInterval_display(MMLogAnnounceInterval* logAnnounceInterval)
//{
//	/* TODO: implement me */
//}
//
//void mMAnnounceReceiptTimeout_display(MMAnnounceReceiptTimeout* announceReceiptTimeout)
//{
//	/* TODO: implement me */
//}
//
//void mMLogSyncInterval_display(MMLogSyncInterval* logSyncInterval)
//{
//	/* TODO: implement me */
//}
//
//void mMVersionNumber_display(MMVersionNumber* versionNumber)
//{
//	/* TODO: implement me */
//}
//
//void mMTime_display(MMTime* time)
//{
//	/* TODO: implement me */
//}
//
//void mMClockAccuracy_display(MMClockAccuracy* clockAccuracy)
//{
//	/* TODO: implement me */
//}
//
//void mMUtcProperties_display(MMUtcProperties* utcProperties)
//{
//	/* TODO: implement me */
//}
//
//void mMTraceabilityProperties_display(MMTraceabilityProperties* traceabilityProperties)
//{
//	/* TODO: implement me */
//}
//
//void mMDelayMechanism_display(MMDelayMechanism* delayMechanism)
//{
//	/* TODO: implement me */
//}
//
//void mMLogMinPdelayReqInterval_display(MMLogMinPdelayReqInterval* logMinPdelayReqInterval)
//{
//	/* TODO: implement me */
//}

void mMErrorStatus_display(MMErrorStatus* errorStatus)
{
	/* TODO: implement me */
}

