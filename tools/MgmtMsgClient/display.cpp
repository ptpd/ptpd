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

#include "constants_dep.h"

/**@brief Display an Integer64 type*/
void integer64_display(Integer64 * bigint)
{
	printf("Integer 64 : \n");
	printf("LSB : %u\n", bigint->lsb);
	printf("MSB : %d\n", bigint->msb);
}

/**@brief Display an UInteger48 type*/
void uInteger48_display(UInteger48 * bigint)
{
    printf("Integer 48 : \n");
    printf("LSB : %u\n", bigint->lsb);
    printf("MSB : %u\n", bigint->msb);
}

/**@brief Display a Timestamp Structure*/
void timestamp_display(Timestamp * timestamp)
{
    uInteger48_display(&timestamp->secondsField);
    printf("nanoseconds %u \n", timestamp->nanosecondsField);
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

/**@brief Display a TimeInterval Structure*/
void timeInterval_display(TimeInterval * timeInterval)
{
    integer64_display(&timeInterval->scaledNanoseconds);
}

/**@brief Display a Portidentity Structure*/
void portIdentity_display(PortIdentity * portIdentity)
{
    clockIdentity_display(portIdentity->clockIdentity);
    printf("port number : %d \n", portIdentity->portNumber);
}

/**@brief Display a Clockquality Structure*/
void clockQuality_display(ClockQuality * clockQuality)
{
    printf("clockClass : %d \n", clockQuality->clockClass);
    printf("clockAccuracy : %d \n", clockQuality->clockAccuracy);
    printf("offsetScaledLogVariance : %d \n", clockQuality->offsetScaledLogVariance);
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
    printf("User Description ManagementTLV message \n");
    printf("userDescription : \n");
    PTPText_display((PTPText*)userDescription);
}

void mMInitialize_display(MMInitialize* initialize)
{
    printf("Initialize ManagementTLV message \n");
    printf("initializeKey : %u \n", initialize->initializeKey);
}

void mMDefaultDataSet_display(MMDefaultDataSet* defaultDataSet)
{
    printf("Default Data Set ManagementTLV message \n");
    bool tsc = ( defaultDataSet->so_tsc & 0x01 ) != 0x00;
    bool so = ( defaultDataSet->so_tsc & 0x02 ) != 0x00;
    printf("TSC : %d \n", tsc ? TRUE : FALSE);
    printf("SO : %d \n", so ? TRUE : FALSE);
    printf("numberPorts : %u \n", defaultDataSet->numberPorts);
    printf("priority1 : %u \n", defaultDataSet->priority1);
    clockQuality_display(&defaultDataSet->clockQuality);
    printf("priority2 : %u \n", defaultDataSet->priority2);
    clockIdentity_display(defaultDataSet->clockIdentity);
    printf("domainNumber : %u \n", defaultDataSet->domainNumber);
}

void mMCurrentDataSet_display(MMCurrentDataSet* currentDataSet)
{
    printf("Current Data Set ManagementTLV message \n");
    printf("stepsRemoved : %u \n", currentDataSet->stepsRemoved);
    printf("offsetFromMaster : \n");
    timeInterval_display(&currentDataSet->offsetFromMaster);
    printf("meanPathDelay : \n");
    timeInterval_display(&currentDataSet->meanPathDelay);
}

void mMParentDataSet_display(MMParentDataSet* parentDataSet)
{
    printf("Parent Data Set ManagementTLV message \n");
    printf("parentPortIdentity : \n");
    portIdentity_display(&parentDataSet->parentPortIdentity);
    printf("PS : %u \n", parentDataSet->PS);
    printf("observedParentOffsetScaledLogVariance : %u \n", parentDataSet->observedParentOffsetScaledLogVariance);
    printf("observedParentClockPhaseChangeRate : %d \n", parentDataSet->observedParentClockPhaseChangeRate);
    printf("grandmasterPriority1 : %u \n", parentDataSet->grandmasterPriority1);
    printf("grandmasterClockQuality : \n");
    clockQuality_display(&parentDataSet->grandmasterClockQuality);
    printf("grandmasterPriority2 : %u \n", parentDataSet->grandmasterPriority2);
    printf("grandmasterIdentity : \n");
    clockIdentity_display(parentDataSet->grandmasterIdentity);
}

void mMTimePropertiesDataSet_display(MMTimePropertiesDataSet* timePropertiesDataSet)
{
    printf("Time Properties Data Set ManagementTLV message \n");
    printf("currentUtcOffset : %d \n", timePropertiesDataSet->currentUtcOffset);
    printf("ftra : %d \n", READ_FIELD(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, FTRA));
    printf("ttra : %d \n", READ_FIELD(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, TTRA));
    printf("ptp : %d \n", READ_FIELD(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, PTPT));
    printf("utcv : %d \n", READ_FIELD(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, UTCV));
    printf("li59 : %d \n", READ_FIELD(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, LI59));
    printf("li61 : %d \n", READ_FIELD(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, LI61));
    printf("timeSource : %x \n", timePropertiesDataSet->timeSource);
}

void mMPortDataSet_display(MMPortDataSet* portDataSet)
{
    printf("Parent Data Set ManagementTLV message \n");
    printf("portIdentity : \n");
    portIdentity_display(&portDataSet->portIdentity);
    printf("portState : %x \n", portDataSet->portState);
    printf("logMinDelayReqInterval : %d \n", portDataSet->logMinDelayReqInterval);
    printf("peerMeanPathDelay : \n");
    timeInterval_display(&portDataSet->peerMeanPathDelay);
    printf("logAnnounceInterval : %d \n", portDataSet->logAnnounceInterval);
    printf("announceReceiptTimeout : %u \n", portDataSet->announceReceiptTimeout);
    printf("logSyncInterval : %d \n", portDataSet->logSyncInterval);
    printf("delayMechanism : %x \n", portDataSet->delayMechanism);
    printf("logMinPdelayReqInterval : %d \n", portDataSet->logMinPdelayReqInterval);
    printf("versionNumber : %x \n", portDataSet->versionNumber & 0x0F);
}

void mMPriority1_display(MMPriority1* priority1)
{
    printf("Priority1 ManagementTLV message \n");
    printf("priority1 : %u \n", priority1->priority1);
}

void mMPriority2_display(MMPriority2* priority2)
{
    printf("Priority2 ManagementTLV message \n");
    printf("priority2 : %u \n", priority2->priority2);
}

void mMDomain_display(MMDomain* domain)
{
    printf("Domain ManagementTLV message \n");
    printf("domainNumber : %u \n", domain->domainNumber);
}

/**
 * @brief Display ManagementTLV Slave Only message
 */
void mMSlaveOnly_display(MMSlaveOnly *slaveOnly)
{
    printf("Slave Only ManagementTLV message \n");
    printf("SO : %d \n", slaveOnly->so);
}

void mMLogAnnounceInterval_display(MMLogAnnounceInterval* logAnnounceInterval)
{
    printf("Log Announce Interval ManagementTLV message \n");
    printf("logAnnounceInterval : %d \n", logAnnounceInterval->logAnnounceInterval);
}

void mMAnnounceReceiptTimeout_display(MMAnnounceReceiptTimeout* announceReceiptTimeout)
{
    printf("Announce Receipt Timeout ManagementTLV message \n");
    printf("announceReceiptTimeout : %u \n", announceReceiptTimeout->announceReceiptTimeout);
}

void mMLogSyncInterval_display(MMLogSyncInterval* logSyncInterval)
{
    printf("Log Sync Interval ManagementTLV message \n");
    printf("logSyncInterval : %d \n", logSyncInterval->logSyncInterval);
}

void mMVersionNumber_display(MMVersionNumber* versionNumber)
{
    printf("Version Number ManagementTLV message \n");
    printf("versionNumber : %x \n", versionNumber->versionNumber);
}

void mMTime_display(MMTime* time)
{
    printf("Time ManagementTLV message \n");
    printf("currentTime : \n");
    timestamp_display(&time->currentTime);
}

void mMClockAccuracy_display(MMClockAccuracy* clockAccuracy)
{
    printf("Clock Accuracy ManagementTLV message \n");
    printf("clockAccuracy : %x \n", clockAccuracy->clockAccuracy);
}

void mMUtcProperties_display(MMUtcProperties* utcProperties)
{
    printf("Utc Properties ManagementTLV message \n");
    printf("currentUtcOffset : %d \n", utcProperties->currentUtcOffset);
    printf("utcv : %d \n", READ_FIELD(utcProperties->utcv_li59_li61, UTCV));
    printf("li59 : %d \n", READ_FIELD(utcProperties->utcv_li59_li61, LI59));
    printf("li61 : %d \n", READ_FIELD(utcProperties->utcv_li59_li61, LI61));
}

void mMTraceabilityProperties_display(MMTraceabilityProperties* traceabilityProperties)
{
    printf("Traceability Properties ManagementTLV message \n");
    printf("ftra : %d \n", READ_FIELD(traceabilityProperties->ftra_ttra, FTRA));
    printf("ttra : %d \n", READ_FIELD(traceabilityProperties->ftra_ttra, TTRA));
}

void mMDelayMechanism_display(MMDelayMechanism* delayMechanism)
{
    printf("Delay Mechanism ManagementTLV message \n");
    printf("delayMechanism : %x \n", delayMechanism->delayMechanism);
}

void mMLogMinPdelayReqInterval_display(MMLogMinPdelayReqInterval* logMinPdelayReqInterval)
{
    printf("Log Min Pdelay Req Interval ManagementTLV message \n");
    printf("logMinPdelayReqInterval : %d \n", logMinPdelayReqInterval->logMinPdelayReqInterval);
}

void mMErrorStatus_display(MMErrorStatus* errorStatus)
{
    printf("Error Status management message \n");
    printf("managementId : %u \n", errorStatus->managementId);
    printf("displayData : \n");
    PTPText_display(&errorStatus->displayData);
}

