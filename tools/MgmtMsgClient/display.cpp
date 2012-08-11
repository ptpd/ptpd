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
 * @file        Display.cpp
 * @author      Tomasz Kleinschmidt
 *
 * @brief       Display functions.
 */

#include "display.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MgmtMsgClient.h"

#include "constants_dep.h"

/**
 * @brief Display an Integer64 type.
 * 
 * @param bigint        An Integer64 to be displayed.
 */
void integer64_display(Integer64 * bigint)
{
	printf("Integer 64 : \n");
	printf("LSB : %u\n", bigint->lsb);
	printf("MSB : %d\n", bigint->msb);
}

/**
 * @brief Display an UInteger48 type.
 * 
 * @param bigint        An Integer48 to be displayed.
 */
void uInteger48_display(UInteger48 * bigint)
{
    printf("Integer 48 : \n");
    printf("LSB : %u\n", bigint->lsb);
    printf("MSB : %u\n", bigint->msb);
}

/**
 * @brief Display a Timestamp Structure.
 * 
 * @param timestamp     A Timestamp Structure to be displayed.
 */
void timestamp_display(Timestamp * timestamp)
{
    uInteger48_display(&timestamp->secondsField);
    printf("nanoseconds %u \n", timestamp->nanosecondsField);
}

/**
 * @brief Display a Clockidentity Structure.
 * 
 * @param clockIdentity A Clockidentity Structure to be displayed.
 */
void clockIdentity_display(ClockIdentity clockIdentity)
{
    printf("ClockIdentity : %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
        clockIdentity[0], clockIdentity[1], clockIdentity[2],
        clockIdentity[3], clockIdentity[4], clockIdentity[5],
        clockIdentity[6], clockIdentity[7]
    );
}

/**
 * @brief Display MAC address.
 * 
 * @param sourceUuid    A MAC address to be displayed.
 */
void clockUUID_display(Octet * sourceUuid)
{
    printf(
        "sourceUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
        sourceUuid[0], sourceUuid[1], sourceUuid[2], sourceUuid[3],
        sourceUuid[4], sourceUuid[5]
    );
}

/**
 * @brief Display a TimeInterval Structure.
 * 
 * @param timeInterval  A TimeInterval Structure to be displayed.
 */
void timeInterval_display(TimeInterval * timeInterval)
{
    integer64_display(&timeInterval->scaledNanoseconds);
}

/**
 * @brief Display a Portidentity Structure.
 * 
 * @param portIdentity  A Portidentity Structure to be displayed.
 */
void portIdentity_display(PortIdentity * portIdentity)
{
    clockIdentity_display(portIdentity->clockIdentity);
    printf("port number : %d \n", portIdentity->portNumber);
}

/**
 * @brief Display a Clockquality Structure.
 * 
 * @param clockQuality  A Clockquality Structure to be displayed.
 */
void clockQuality_display(ClockQuality * clockQuality)
{
    printf("clockClass : %d \n", clockQuality->clockClass);
    printf("clockAccuracy : %d \n", clockQuality->clockAccuracy);
    printf("offsetScaledLogVariance : %d \n", clockQuality->offsetScaledLogVariance);
}

/**
 * @brief Display PTPText Structure.
 * 
 * @param p     A PTPText Structure to be displayed.
 */
void PTPText_display(PTPText *p)
{
    /* Allocate new memory to append null-terminator to the text field.
        * This allows printing the textField as a string
        */
    Octet *str;
    XMALLOC(str, Octet*, p->lengthField + 1);
    memcpy(str, p->textField, p->lengthField);
    str[p->lengthField] = '\0';
    printf("    lengthField : %d \n", p->lengthField);
    printf("    textField : %s \n", str);
    free(str);
}

/**
 * @brief Display Header message.
 * 
 * @param header        A Header to be displayed.
 */
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

/**
 * @brief Display Management message.
 * 
 * @param manage        A Management message to be displayed.
 */
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
 * @brief Display ManagementTLV Clock Description message.
 * 
 * @param clockDescription      A Clock Description ManagementTLV to be displayed.
 */
void mMClockDescription_display(MMClockDescription *clockDescription)
{
    printf("\nClock Description ManagementTLV message \n");
    printf("\n");
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

/**
 * @brief Display ManagementTLV User Description message.
 * 
 * @param userDescription       A User Description ManagementTLV to be displayed.
 */
void mMUserDescription_display(MMUserDescription* userDescription)
{
    printf("\nUser Description ManagementTLV message \n");
    printf("\n");
    printf("userDescription : \n");
    PTPText_display((PTPText*)userDescription);
}

/**
 * @brief Display ManagementTLV Initialize message.
 * 
 * @param initialize    An Initialize ManagementTLV to be displayed.
 */
void mMInitialize_display(MMInitialize* initialize)
{
    printf("\nInitialize ManagementTLV message \n");
    printf("\n");
    printf("initializeKey : %u \n", initialize->initializeKey);
}

/**
 * @brief Display ManagementTLV Default Data Set message.
 * 
 * @param defaultDataSet        A Default Data Set ManagementTLV to be displayed.
 */
void mMDefaultDataSet_display(MMDefaultDataSet* defaultDataSet)
{
    printf("\nDefault Data Set ManagementTLV message \n");
    printf("\n");
    printf("slaveOnly : %hhu \n", IS_SET(defaultDataSet->so_tsc, SO));
    printf("twoStepFlag : %hhu \n", IS_SET(defaultDataSet->so_tsc, TSC));
    printf("numberPorts : %u \n", defaultDataSet->numberPorts);
    printf("priority1 : %u \n", defaultDataSet->priority1);
    clockQuality_display(&defaultDataSet->clockQuality);
    printf("priority2 : %u \n", defaultDataSet->priority2);
    clockIdentity_display(defaultDataSet->clockIdentity);
    printf("domainNumber : %u \n", defaultDataSet->domainNumber);
}

/**
 * @brief Display ManagementTLV Current Data Set message.
 * 
 * @param currentDataSet        A Current Data Set ManagementTLV to be displayed.
 */
void mMCurrentDataSet_display(MMCurrentDataSet* currentDataSet)
{
    printf("\nCurrent Data Set ManagementTLV message \n");
    printf("\n");
    printf("stepsRemoved : %u \n", currentDataSet->stepsRemoved);
    printf("offsetFromMaster : \n");
    timeInterval_display(&currentDataSet->offsetFromMaster);
    printf("meanPathDelay : \n");
    timeInterval_display(&currentDataSet->meanPathDelay);
}

/**
 * @brief Display ManagementTLV Parent Data Set message.
 * 
 * @param parentDataSet A Parent Data Set ManagementTLV to be displayed.
 */
void mMParentDataSet_display(MMParentDataSet* parentDataSet)
{
    printf("\nParent Data Set ManagementTLV message \n");
    printf("\n");
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

/**
 * @brief Display ManagementTLV Time Properties Data Set message.
 * 
 * @param timePropertiesDataSet A Time Properties Data Set ManagementTLV to be displayed.
 */
void mMTimePropertiesDataSet_display(MMTimePropertiesDataSet* timePropertiesDataSet)
{
    printf("\nTime Properties Data Set ManagementTLV message \n");
    printf("\n");
    printf("currentUtcOffset : %d \n", timePropertiesDataSet->currentUtcOffset);
    printf("frequencyTraceable : %hhu \n", IS_SET(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, FTRA));
    printf("timeTraceable : %hhu \n", IS_SET(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, TTRA));
    printf("ptpTimescale : %hhu \n", IS_SET(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, PTPT));
    printf("currentUtcOffsetValid : %hhu \n", IS_SET(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, UTCV));
    printf("leap59 : %hhu \n", IS_SET(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, LI59));
    printf("leap61 : %hhu \n", IS_SET(timePropertiesDataSet->ftra_ttra_ptp_utcv_li59_li61, LI61));
    printf("timeSource : %d \n", timePropertiesDataSet->timeSource);
}

/**
 * @brief Display ManagementTLV Port Data Set message.
 * 
 * @param portDataSet   A Port Data Set ManagementTLV to be displayed.
 */
void mMPortDataSet_display(MMPortDataSet* portDataSet)
{
    printf("\nPort Data Set ManagementTLV message \n");
    printf("\n");
    printf("portIdentity : \n");
    portIdentity_display(&portDataSet->portIdentity);
    printf("portState : %d \n", portDataSet->portState);
    printf("logMinDelayReqInterval : %d \n", portDataSet->logMinDelayReqInterval);
    printf("peerMeanPathDelay : \n");
    timeInterval_display(&portDataSet->peerMeanPathDelay);
    printf("logAnnounceInterval : %d \n", portDataSet->logAnnounceInterval);
    printf("announceReceiptTimeout : %u \n", portDataSet->announceReceiptTimeout);
    printf("logSyncInterval : %d \n", portDataSet->logSyncInterval);
    printf("delayMechanism : %d \n", portDataSet->delayMechanism);
    printf("logMinPdelayReqInterval : %d \n", portDataSet->logMinPdelayReqInterval);
    printf("versionNumber : %d \n", portDataSet->versionNumber & 0x0F);
}

/**
 * @brief Display ManagementTLV Priority1 message.
 * 
 * @param priority1     A Priority1 ManagementTLV to be displayed.
 */
void mMPriority1_display(MMPriority1* priority1)
{
    printf("\nPriority1 ManagementTLV message \n");
    printf("\n");
    printf("priority1 : %u \n", priority1->priority1);
}

/**
 * @brief Display ManagementTLV Priority2 message.
 * 
 * @param priority2     A Priority2 ManagementTLV to be displayed.
 */
void mMPriority2_display(MMPriority2* priority2)
{
    printf("\nPriority2 ManagementTLV message \n");
    printf("\n");
    printf("priority2 : %u \n", priority2->priority2);
}

/**
 * @brief Display ManagementTLV Domain message.
 * 
 * @param domain        A Domain ManagementTLV to be displayed.
 */
void mMDomain_display(MMDomain* domain)
{
    printf("\nDomain ManagementTLV message \n");
    printf("\n");
    printf("domainNumber : %u \n", domain->domainNumber);
}

/**
 * @brief Display ManagementTLV Slave Only message.
 * 
 * @param slaveOnly     A Slave Only ManagementTLV to be displayed.
 */
void mMSlaveOnly_display(MMSlaveOnly *slaveOnly)
{
    printf("\nSlave Only ManagementTLV message \n");
    printf("\n");
    printf("SO : %d \n", slaveOnly->so);
}

/**
 * @brief Display ManagementTLV Log Announce Interval message.
 * 
 * @param logAnnounceInterval   A Log Announce Interval ManagementTLV to be displayed.
 */
void mMLogAnnounceInterval_display(MMLogAnnounceInterval* logAnnounceInterval)
{
    printf("\nLog Announce Interval ManagementTLV message \n");
    printf("\n");
    printf("logAnnounceInterval : %d \n", logAnnounceInterval->logAnnounceInterval);
}

/**
 * @brief Display ManagementTLV Announce Receipt Timeout message.
 * 
 * @param announceReceiptTimeout        A Announce Receipt Timeout ManagementTLV to be displayed.
 */
void mMAnnounceReceiptTimeout_display(MMAnnounceReceiptTimeout* announceReceiptTimeout)
{
    printf("\nAnnounce Receipt Timeout ManagementTLV message \n");
    printf("\n");
    printf("announceReceiptTimeout : %u \n", announceReceiptTimeout->announceReceiptTimeout);
}

/**
 * @brief Display ManagementTLV Log Sync Interval message.
 * 
 * @param logSyncInterval       A Log Sync Interval ManagementTLV to be displayed.
 */
void mMLogSyncInterval_display(MMLogSyncInterval* logSyncInterval)
{
    printf("\nLog Sync Interval ManagementTLV message \n");
    printf("\n");
    printf("logSyncInterval : %d \n", logSyncInterval->logSyncInterval);
}

/**
 * @brief Display ManagementTLV Version Number message.
 * 
 * @param versionNumber A Version Number ManagementTLV to be displayed.
 */
void mMVersionNumber_display(MMVersionNumber* versionNumber)
{
    printf("\nVersion Number ManagementTLV message \n");
    printf("\n");
    printf("versionNumber : %d \n", versionNumber->versionNumber);
}

/**
 * @brief Display ManagementTLV Time message.
 * 
 * @param time  A Time ManagementTLV to be displayed.
 */
void mMTime_display(MMTime* time)
{
    printf("\nTime ManagementTLV message \n");
    printf("\n");
    printf("currentTime : \n");
    timestamp_display(&time->currentTime);
}

/**
 * @brief Display ManagementTLV Clock Accuracy message.
 * 
 * @param clockAccuracy A Clock Accuracy ManagementTLV to be displayed.
 */
void mMClockAccuracy_display(MMClockAccuracy* clockAccuracy)
{
    printf("\nClock Accuracy ManagementTLV message \n");
    printf("\n");
    printf("clockAccuracy : %d \n", clockAccuracy->clockAccuracy);
}

/**
 * @brief Display ManagementTLV Utc Properties message.
 * 
 * @param utcProperties A Utc Properties ManagementTLV to be displayed.
 */
void mMUtcProperties_display(MMUtcProperties* utcProperties)
{
    printf("\nUtc Properties ManagementTLV message \n");
    printf("\n");
    printf("currentUtcOffset : %d \n", utcProperties->currentUtcOffset);
    printf("currentUtcOffsetValid : %hhu \n", IS_SET(utcProperties->utcv_li59_li61, UTCV));
    printf("leap59 : %hhu \n", IS_SET(utcProperties->utcv_li59_li61, LI59));
    printf("leap61 : %hhu \n", IS_SET(utcProperties->utcv_li59_li61, LI61));
}

/**
 * @brief Display ManagementTLV Traceability Properties message.
 * 
 * @param traceabilityProperties        A Traceability Properties ManagementTLV to be displayed.
 */
void mMTraceabilityProperties_display(MMTraceabilityProperties* traceabilityProperties)
{
    printf("\nTraceability Properties ManagementTLV message \n");
    printf("\n");
    printf("frequencyTraceable : %hhu \n", IS_SET(traceabilityProperties->ftra_ttra, FTRA));
    printf("timeTraceable : %hhu \n", IS_SET(traceabilityProperties->ftra_ttra, TTRA));
}

/**
 * @brief Display ManagementTLV Delay Mechanism message.
 * 
 * @param delayMechanism        A Delay Mechanism ManagementTLV to be displayed.
 */
void mMDelayMechanism_display(MMDelayMechanism* delayMechanism)
{
    printf("\nDelay Mechanism ManagementTLV message \n");
    printf("\n");
    printf("delayMechanism : %d \n", delayMechanism->delayMechanism);
}

/**
 * @brief Display ManagementTLV Log Min Pdelay Req Interval message.
 * 
 * @param logMinPdelayReqInterval       A Log Min Pdelay Req Interval ManagementTLV to be displayed.
 */
void mMLogMinPdelayReqInterval_display(MMLogMinPdelayReqInterval* logMinPdelayReqInterval)
{
    printf("\nLog Min Pdelay Req Interval ManagementTLV message \n");
    printf("\n");
    printf("logMinPdelayReqInterval : %d \n", logMinPdelayReqInterval->logMinPdelayReqInterval);
}

/**
 * @brief Display ManagementTLV Error Status message.
 * 
 * @param errorStatus   An Error Status ManagementTLV to be displayed.
 */
void mMErrorStatus_display(MMErrorStatus* errorStatus)
{
    printf("\nError Status management message \n");
    printf("\n");
    printf("managementId : %u \n", errorStatus->managementId);
    printf("displayData : \n");
    PTPText_display(&errorStatus->displayData);
}

