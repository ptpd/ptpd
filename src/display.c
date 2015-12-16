/*-
 * Copyright (c) 2012-2013 George V. Neville-Neil,
 *                         Wojciech Owczarek
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments.
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
 * @file   display.c
 * @date   Thu Aug 12 09:06:21 2010
 *
 * @brief  General routines for displaying internal data.
 *
 *
 */

#include "ptpd.h"

/**\brief Display an Integer64 type*/
void
integer64_display(const Integer64 * bigint)
{
	DBGV("Integer 64 : \n");
	DBGV("LSB : %u\n", bigint->lsb);
	DBGV("MSB : %d\n", bigint->msb);
}

/**\brief Display an UInteger48 type*/
void
uInteger48_display(const UInteger48 * bigint)
{
	DBGV("Integer 48 : \n");
	DBGV("LSB : %u\n", bigint->lsb);
	DBGV("MSB : %u\n", bigint->msb);
}

/** \brief Display a TimeInternal Structure*/
void
timeInternal_display(const TimeInternal * timeInternal)
{
	DBGV("seconds : %d \n", timeInternal->seconds);
	DBGV("nanoseconds %d \n", timeInternal->nanoseconds);
}

/** \brief Display a Timestamp Structure*/
void
timestamp_display(const Timestamp * timestamp)
{
	uInteger48_display(&timestamp->secondsField);
	DBGV("nanoseconds %u \n", timestamp->nanosecondsField);
}

/**\brief Display a Clockidentity Structure*/
void
clockIdentity_display(const ClockIdentity clockIdentity)
{
	DBGV(
	    "ClockIdentity : %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	    clockIdentity[0], clockIdentity[1], clockIdentity[2],
	    clockIdentity[3], clockIdentity[4], clockIdentity[5],
	    clockIdentity[6], clockIdentity[7]
	);
}

/**\brief Display MAC address*/
void
clockUUID_display(const Octet * sourceUuid)
{
	DBGV(
	    "sourceUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	    sourceUuid[0], sourceUuid[1], sourceUuid[2], sourceUuid[3],
	    sourceUuid[4], sourceUuid[5]
	);
}

/**\brief Display Network info*/
void
netPath_display(const NetPath * net)
{
#ifdef RUNTIME_DEBUG
		struct in_addr tmpAddr;
	DBGV("eventSock : %d \n", net->eventSock);
	DBGV("generalSock : %d \n", net->generalSock);
	tmpAddr.s_addr = net->multicastAddr;
	DBGV("multicastAdress : %s \n", inet_ntoa(tmpAddr));
	tmpAddr.s_addr = net->peerMulticastAddr;
	DBGV("peerMulticastAddress : %s \n", inet_ntoa(tmpAddr));
#endif /* RUNTIME_DEBUG */
}

/**\brief Display a IntervalTimer Structure*/
void
intervalTimer_display(const IntervalTimer * ptimer)
{
	DBGV("interval : %.06f \n", ptimer->interval);
	DBGV("expire : %d \n", ptimer->expired);
}

/**\brief Display a TimeInterval Structure*/
void
timeInterval_display(const TimeInterval * timeInterval)
{
	integer64_display(&timeInterval->scaledNanoseconds);
}

/**\brief Display a Portidentity Structure*/
void
portIdentity_display(const PortIdentity * portIdentity)
{
	clockIdentity_display(portIdentity->clockIdentity);
	DBGV("port number : %d \n", portIdentity->portNumber);
}

/**\brief Display a Clockquality Structure*/
void
clockQuality_display(const ClockQuality * clockQuality)
{
	DBGV("clockClass : %d \n", clockQuality->clockClass);
	DBGV("clockAccuracy : %d \n", clockQuality->clockAccuracy);
	DBGV("offsetScaledLogVariance : %d \n", clockQuality->offsetScaledLogVariance);
}

/**\brief Display PTPText Structure*/
void
PTPText_display(const PTPText *p, const PtpClock *ptpClock)
{
	DBGV("    lengthField : %d \n", p->lengthField);
	DBGV("    textField : %.*s \n", (int)p->lengthField,  p->textField);
}

/**\brief Display the Network Interface Name*/
void
iFaceName_display(const Octet * iFaceName)
{

	int i;

	DBGV("iFaceName : ");

	for (i = 0; i < IFACE_NAME_LENGTH; i++) {
		DBGV("%c", iFaceName[i]);
	}
	DBGV("\n");

}

/**\brief Display an Unicast Adress*/
void
unicast_display(const Octet * unicast)
{

	int i;

	DBGV("Unicast adress : ");

	for (i = 0; i < NET_ADDRESS_LENGTH; i++) {
		DBGV("%c", unicast[i]);
	}
	DBGV("\n");

}


/**\brief Display Sync message*/
void
msgSync_display(const MsgSync * sync)
{
	DBGV("Message Sync : \n");
	timestamp_display(&sync->originTimestamp);
	DBGV("\n");
}

/**\brief Display Header message*/
void
msgHeader_display(const MsgHeader * header)
{
	DBGV("Message header : \n");
	DBGV("\n");
	DBGV("transportSpecific : %d\n", header->transportSpecific);
	DBGV("messageType : %d\n", header->messageType);
	DBGV("versionPTP : %d\n", header->versionPTP);
	DBGV("messageLength : %d\n", header->messageLength);
	DBGV("domainNumber : %d\n", header->domainNumber);
	DBGV("FlagField %02hhx:%02hhx\n", header->flagField0, header->flagField1);
	DBGV("CorrectionField : \n");
	integer64_display(&header->correctionField);
	DBGV("SourcePortIdentity : \n");
	portIdentity_display(&header->sourcePortIdentity);
	DBGV("sequenceId : %d\n", header->sequenceId);
	DBGV("controlField : %d\n", header->controlField);
	DBGV("logMessageInterval : %d\n", header->logMessageInterval);
	DBGV("\n");
}

/**\brief Display Announce message*/
void
msgAnnounce_display(const MsgAnnounce * announce)
{
	DBGV("Announce Message : \n");
	DBGV("\n");
	DBGV("originTimestamp : \n");
	DBGV("secondField  : \n");
	timestamp_display(&announce->originTimestamp);
	DBGV("currentUtcOffset : %d \n", announce->currentUtcOffset);
	DBGV("grandMasterPriority1 : %d \n", announce->grandmasterPriority1);
	DBGV("grandMasterClockQuality : \n");
	clockQuality_display(&announce->grandmasterClockQuality);
	DBGV("grandMasterPriority2 : %d \n", announce->grandmasterPriority2);
	DBGV("grandMasterIdentity : \n");
	clockIdentity_display(announce->grandmasterIdentity);
	DBGV("stepsRemoved : %d \n", announce->stepsRemoved);
	DBGV("timeSource : %d \n", announce->timeSource);
	DBGV("\n");
}

/**\brief Display Follow_UP message*/
void
msgFollowUp_display(const MsgFollowUp * follow)
{
	timestamp_display(&follow->preciseOriginTimestamp);
}

/**\brief Display DelayReq message*/
void
msgDelayReq_display(const MsgDelayReq * req)
{
	timestamp_display(&req->originTimestamp);
}

/**\brief Display DelayResp message*/
void
msgDelayResp_display(const MsgDelayResp * resp)
{
	timestamp_display(&resp->receiveTimestamp);
	portIdentity_display(&resp->requestingPortIdentity);
}

/**\brief Display Pdelay_Req message*/
void
msgPdelayReq_display(const MsgPdelayReq * preq)
{
	timestamp_display(&preq->originTimestamp);
}

/**\brief Display Pdelay_Resp message*/
void
msgPdelayResp_display(const MsgPdelayResp * presp)
{

	timestamp_display(&presp->requestReceiptTimestamp);
	portIdentity_display(&presp->requestingPortIdentity);
}

/**\brief Display Pdelay_Resp Follow Up message*/
void
msgPdelayRespFollowUp_display(const MsgPdelayRespFollowUp * prespfollow)
{

	timestamp_display(&prespfollow->responseOriginTimestamp);
	portIdentity_display(&prespfollow->requestingPortIdentity);
}

/**\brief Display Management message*/
void
msgManagement_display(const MsgManagement * manage)
{
        DBGV("Management Message : \n");
        DBGV("\n");
        DBGV("targetPortIdentity : \n");
	portIdentity_display(&manage->targetPortIdentity);
	DBGV("startingBoundaryHops : %d \n", manage->startingBoundaryHops);
	DBGV("boundaryHops : %d \n", manage->boundaryHops);
	DBGV("actionField : %d\n", manage->actionField);
}

/**\brief Display ManagementTLV Slave Only message*/
void
mMSlaveOnly_display(const MMSlaveOnly *slaveOnly, const PtpClock *ptpClock)
{
	DBGV("Slave Only ManagementTLV message \n");
	DBGV("SO : %d \n", slaveOnly->so);
}

/**\brief Display ManagementTLV Clock Description message*/
void
mMClockDescription_display(const MMClockDescription *clockDescription, const PtpClock *ptpClock)
{
	DBGV("Clock Description ManagementTLV message \n");
	DBGV("clockType0 : %d \n", clockDescription->clockType0);
	DBGV("clockType1 : %d \n", clockDescription->clockType1);
	DBGV("physicalLayerProtocol : \n");
	PTPText_display(&clockDescription->physicalLayerProtocol, ptpClock);
	DBGV("physicalAddressLength : %d \n", clockDescription->physicalAddress.addressLength);
	if(clockDescription->physicalAddress.addressField) {
		DBGV("physicalAddressField : \n");
		clockUUID_display(clockDescription->physicalAddress.addressField);
	}
	DBGV("protocolAddressNetworkProtocol : %d \n", clockDescription->protocolAddress.networkProtocol);
	DBGV("protocolAddressLength : %d \n", clockDescription->protocolAddress.addressLength);
	if(clockDescription->protocolAddress.addressField) {
		DBGV("protocolAddressField : %d.%d.%d.%d \n",
			(UInteger8)clockDescription->protocolAddress.addressField[0],
			(UInteger8)clockDescription->protocolAddress.addressField[1],
			(UInteger8)clockDescription->protocolAddress.addressField[2],
			(UInteger8)clockDescription->protocolAddress.addressField[3]);
	}
	DBGV("manufacturerIdentity0 : %d \n", clockDescription->manufacturerIdentity0);
	DBGV("manufacturerIdentity1 : %d \n", clockDescription->manufacturerIdentity1);
	DBGV("manufacturerIdentity2 : %d \n", clockDescription->manufacturerIdentity2);
	DBGV("productDescription : \n");
	PTPText_display(&clockDescription->productDescription, ptpClock);
	DBGV("revisionData : \n");
	PTPText_display(&clockDescription->revisionData, ptpClock);
	DBGV("userDescription : \n");
	PTPText_display(&clockDescription->userDescription, ptpClock);
	DBGV("profileIdentity0 : %d \n", clockDescription->profileIdentity0);
	DBGV("profileIdentity1 : %d \n", clockDescription->profileIdentity1);
	DBGV("profileIdentity2 : %d \n", clockDescription->profileIdentity2);
	DBGV("profileIdentity3 : %d \n", clockDescription->profileIdentity3);
	DBGV("profileIdentity4 : %d \n", clockDescription->profileIdentity4);
	DBGV("profileIdentity5 : %d \n", clockDescription->profileIdentity5);
}

void
mMUserDescription_display(const MMUserDescription* userDescription, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMInitialize_display(const MMInitialize* initialize, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMDefaultDataSet_display(const MMDefaultDataSet* defaultDataSet, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMCurrentDataSet_display(const MMCurrentDataSet* currentDataSet, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMParentDataSet_display(const MMParentDataSet* parentDataSet, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMTimePropertiesDataSet_display(const MMTimePropertiesDataSet* timePropertiesDataSet, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMPortDataSet_display(const MMPortDataSet* portDataSet, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMPriority1_display(const MMPriority1* priority1, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMPriority2_display(const MMPriority2* priority2, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMDomain_display(const MMDomain* domain, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMLogAnnounceInterval_display(const MMLogAnnounceInterval* logAnnounceInterval, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMAnnounceReceiptTimeout_display(const MMAnnounceReceiptTimeout* announceReceiptTimeout, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMLogSyncInterval_display(const MMLogSyncInterval* logSyncInterval, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMVersionNumber_display(const MMVersionNumber* versionNumber, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMTime_display(const MMTime* time, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMClockAccuracy_display(const MMClockAccuracy* clockAccuracy, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMUtcProperties_display(const MMUtcProperties* utcProperties, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMTraceabilityProperties_display(const MMTraceabilityProperties* traceabilityProperties, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMTimescaleProperties_display(const MMTimescaleProperties* TimescaleProperties, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMUnicastNegotiationEnable_display(const MMUnicastNegotiationEnable* unicastNegotiationEnable, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}


void
mMDelayMechanism_display(const MMDelayMechanism* delayMechanism, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMLogMinPdelayReqInterval_display(const MMLogMinPdelayReqInterval* logMinPdelayReqInterval, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMErrorStatus_display(const MMErrorStatus* errorStatus, const PtpClock *ptpClock)
{
	/* TODO: implement me */
}

/**\brief Display Signaling message*/
void
msgSignaling_display(const MsgSignaling * signaling)
{
        DBGV("Signaling Message : \n");
        DBGV("\n");
        DBGV("targetPortIdentity : \n");
	portIdentity_display(&signaling->targetPortIdentity);
}

void
sMRequestUnicastTransmission_display(const SMRequestUnicastTransmission* request, const PtpClock *ptpClock)
{
	DBGV("Request Unicast Transmission SignalingTLV message \n");
	DBGV("messageType: %d\n", request->messageType);
	DBGV("logInterMessagePeriod: %d\n", request->logInterMessagePeriod);
	DBGV("durationField: %d\n", request->durationField);
}

void
sMGrantUnicastTransmission_display(const SMGrantUnicastTransmission* grant, const PtpClock *ptpClock)
{
	DBGV("Grant Unicast Transmission SignalingTLV message \n");
	DBGV("messageType: %d\n", grant->messageType);
	DBGV("logInterMessagePeriod: %d\n", grant->logInterMessagePeriod);
	DBGV("durationField: %d\n", grant->durationField);
	DBGV("R (renewal invited): %d\n", grant->renewal_invited);
}

void
sMCancelUnicastTransmission_display(const SMCancelUnicastTransmission* tlv, const PtpClock *ptpClock)
{
	DBGV("Cancel Unicast Transmission SignalingTLV message \n");
	DBGV("messageType: %d\n", tlv->messageType);
}

void
sMAcknowledgeCancelUnicastTransmission_display(const SMAcknowledgeCancelUnicastTransmission* tlv, const PtpClock *ptpClock)
{
	DBGV("Acknowledge Cancel Unicast Transmission SignalingTLV message \n");
	DBGV("messageType: %d\n", tlv->messageType);
}

#define FORMAT_SERVO	"%f"

/**\brief Display runTimeOptions structure*/
void
displayRunTimeOpts(const RunTimeOpts * rtOpts)
{

	DBGV("---Run time Options Display-- \n");
	DBGV("\n");
	DBGV("announceInterval : %d \n", rtOpts->logAnnounceInterval);
	DBGV("syncInterval : %d \n", rtOpts->logSyncInterval);
	clockQuality_display(&(rtOpts->clockQuality));
	DBGV("priority1 : %d \n", rtOpts->priority1);
	DBGV("priority2 : %d \n", rtOpts->priority2);
	DBGV("domainNumber : %d \n", rtOpts->domainNumber);
	DBGV("slaveOnly : %d \n", rtOpts->slaveOnly);
	DBGV("currentUtcOffset : %d \n", rtOpts->timeProperties.currentUtcOffset);
	DBGV("noAdjust : %d \n", rtOpts->noAdjust);
	DBGV("logStatistics : %d \n", rtOpts->logStatistics);
	iFaceName_display(rtOpts->ifaceName);
	DBGV("kP : %d \n", rtOpts->servoKP);
	DBGV("kI : %d \n", rtOpts->servoKI);
	DBGV("s : %d \n", rtOpts->s);
	DBGV("inbound latency : \n");
	timeInternal_display(&(rtOpts->inboundLatency));
	DBGV("outbound latency : \n");
	timeInternal_display(&(rtOpts->outboundLatency));
	DBGV("max_foreign_records : %d \n", rtOpts->max_foreign_records);
	DBGV("transport : %d \n", rtOpts->transport);
	DBGV("\n");
}


/**\brief Display Default data set of a PtpClock*/
void
displayDefault(const PtpClock * ptpClock)
{
	DBGV("---Ptp Clock Default Data Set-- \n");
	DBGV("\n");
	DBGV("twoStepFlag : %d \n", ptpClock->defaultDS.twoStepFlag);
	clockIdentity_display(ptpClock->defaultDS.clockIdentity);
	DBGV("numberPorts : %d \n", ptpClock->defaultDS.numberPorts);
	clockQuality_display(&(ptpClock->defaultDS.clockQuality));
	DBGV("priority1 : %d \n", ptpClock->defaultDS.priority1);
	DBGV("priority2 : %d \n", ptpClock->defaultDS.priority2);
	DBGV("domainNumber : %d \n", ptpClock->defaultDS.domainNumber);
	DBGV("slaveOnly : %d \n", ptpClock->defaultDS.slaveOnly);
	DBGV("\n");
}


/**\brief Display Current data set of a PtpClock*/
void
displayCurrent(const PtpClock * ptpClock)
{
	DBGV("---Ptp Clock Current Data Set-- \n");
	DBGV("\n");

	DBGV("stepsremoved : %d \n", ptpClock->currentDS.stepsRemoved);
	DBGV("Offset from master : \n");
	timeInternal_display(&ptpClock->currentDS.offsetFromMaster);
	DBGV("Mean path delay : \n");
	timeInternal_display(&ptpClock->currentDS.meanPathDelay);
	DBGV("\n");
}



/**\brief Display Parent data set of a PtpClock*/
void
displayParent(const PtpClock * ptpClock)
{
	DBGV("---Ptp Clock Parent Data Set-- \n");
	DBGV("\n");
	portIdentity_display(&(ptpClock->parentDS.parentPortIdentity));
	DBGV("parentStats : %d \n", ptpClock->parentDS.parentStats);
	DBGV("observedParentOffsetScaledLogVariance : %d \n", ptpClock->parentDS.observedParentOffsetScaledLogVariance);
	DBGV("observedParentClockPhaseChangeRate : %d \n", ptpClock->parentDS.observedParentClockPhaseChangeRate);
	DBGV("--GrandMaster--\n");
	clockIdentity_display(ptpClock->parentDS.grandmasterIdentity);
	clockQuality_display(&ptpClock->parentDS.grandmasterClockQuality);
	DBGV("grandmasterpriority1 : %d \n", ptpClock->parentDS.grandmasterPriority1);
	DBGV("grandmasterpriority2 : %d \n", ptpClock->parentDS.grandmasterPriority2);
	DBGV("\n");
}

/**\brief Display Global data set of a PtpClock*/
void
displayGlobal(const PtpClock * ptpClock)
{
	DBGV("---Ptp Clock Global Time Data Set-- \n");
	DBGV("\n");

	DBGV("currentUtcOffset : %d \n", ptpClock->timePropertiesDS.currentUtcOffset);
	DBGV("currentUtcOffsetValid : %d \n", ptpClock->timePropertiesDS.currentUtcOffsetValid);
	DBGV("leap59 : %d \n", ptpClock->timePropertiesDS.leap59);
	DBGV("leap61 : %d \n", ptpClock->timePropertiesDS.leap61);
	DBGV("timeTraceable : %d \n", ptpClock->timePropertiesDS.timeTraceable);
	DBGV("frequencyTraceable : %d \n", ptpClock->timePropertiesDS.frequencyTraceable);
	DBGV("ptpTimescale : %d \n", ptpClock->timePropertiesDS.ptpTimescale);
	DBGV("timeSource : %d \n", ptpClock->timePropertiesDS.timeSource);
	DBGV("\n");
}

/**\brief Display Port data set of a PtpClock*/
void
displayPort(const PtpClock * ptpClock)
{
	DBGV("---Ptp Clock Port Data Set-- \n");
	DBGV("\n");

	portIdentity_display(&ptpClock->portDS.portIdentity);
	DBGV("port state : %d \n", ptpClock->portDS.portState);
	DBGV("logMinDelayReqInterval : %d \n", ptpClock->portDS.logMinDelayReqInterval);
	DBGV("peerMeanPathDelay : \n");
	timeInternal_display(&ptpClock->portDS.peerMeanPathDelay);
	DBGV("logAnnounceInterval : %d \n", ptpClock->portDS.logAnnounceInterval);
	DBGV("announceReceiptTimeout : %d \n", ptpClock->portDS.announceReceiptTimeout);
	DBGV("logSyncInterval : %d \n", ptpClock->portDS.logSyncInterval);
	DBGV("delayMechanism : %d \n", ptpClock->portDS.delayMechanism);
	DBGV("logMinPdelayReqInterval : %d \n", ptpClock->portDS.logMinPdelayReqInterval);
	DBGV("versionNumber : %d \n", ptpClock->portDS.versionNumber);
	DBGV("\n");
}

/**\brief Display ForeignMaster data set of a PtpClock*/
void
displayForeignMaster(const PtpClock * ptpClock)
{

	ForeignMasterRecord *foreign;
	int i;

	if (ptpClock->number_foreign_records > 0) {

		DBGV("---Ptp Clock Foreign Data Set-- \n");
		DBGV("\n");
		DBGV("There is %d Foreign master Recorded \n", ptpClock->number_foreign_records);
		foreign = ptpClock->foreign;

		for (i = 0; i < ptpClock->number_foreign_records; i++) {

			portIdentity_display(&foreign->foreignMasterPortIdentity);
			DBGV("number of Announce message received : %d \n", foreign->foreignMasterAnnounceMessages);
			msgHeader_display(&foreign->header);
			msgAnnounce_display(&foreign->announce);

			foreign++;
		}

	} else {
		DBGV("No Foreign masters recorded \n");
	}

	DBGV("\n");


}

/**\brief Display other data set of a PtpClock*/

void
displayOthers(const PtpClock * ptpClock)
{

	int i;

	/* Usefull to display name of timers */
#ifdef PTPD_DBGV
	    char timer[][26] = {
		"PDELAYREQ_INTERVAL_TIMER",
		"SYNC_INTERVAL_TIMER",
		"ANNOUNCE_RECEIPT_TIMER",
		"ANNOUNCE_INTERVAL_TIMER"
	};
#endif
	DBGV("---Ptp Others Data Set-- \n");
	DBGV("\n");

	/*DBGV("master_to_slave_delay : \n");
	timeInternal_display(&ptpClock->master_to_slave_delay);
	DBGV("\n");
	DBGV("slave_to_master_delay : \n");
	timeInternal_display(&ptpClock->slave_to_master_delay);
	*/
	
	DBGV("\n");
	DBGV("delay_req_receive_time : \n");
	timeInternal_display(&ptpClock->pdelay_req_receive_time);
	DBGV("\n");
	DBGV("delay_req_send_time : \n");
	timeInternal_display(&ptpClock->pdelay_req_send_time);
	DBGV("\n");
	DBGV("delay_resp_receive_time : \n");
	timeInternal_display(&ptpClock->pdelay_resp_receive_time);
	DBGV("\n");
	DBGV("delay_resp_send_time : \n");
	timeInternal_display(&ptpClock->pdelay_resp_send_time);
	DBGV("\n");
	DBGV("sync_receive_time : \n");
	timeInternal_display(&ptpClock->sync_receive_time);
	DBGV("\n");
	//DBGV("R : %f \n", ptpClock->R);
	DBGV("sentPdelayReq : %d \n", ptpClock->sentPdelayReq);
	DBGV("sentPdelayReqSequenceId : %d \n", ptpClock->sentPdelayReqSequenceId);
	DBGV("waitingForFollow : %d \n", ptpClock->waitingForFollow);
	DBGV("\n");
	DBGV("Offset from master filter : \n");
	DBGV("nsec_prev : %d \n", ptpClock->ofm_filt.nsec_prev);
	DBGV("y : %d \n", ptpClock->ofm_filt.y);
	DBGV("\n");
	DBGV("One way delay filter : \n");
	DBGV("nsec_prev : %d \n", ptpClock->mpd_filt.nsec_prev);
	DBGV("y : %d \n", ptpClock->mpd_filt.y);
	DBGV("s_exp : %d \n", ptpClock->mpd_filt.s_exp);
	DBGV("\n");
	DBGV("observed drift : "FORMAT_SERVO" \n", ptpClock->servo.observedDrift);
	DBGV("message activity %d \n", ptpClock->message_activity);
	DBGV("\n");

	for (i = 0; i < PTP_MAX_TIMER; i++) {
		DBGV("%s : \n", timer[i]);
		intervalTimer_display(&ptpClock->timers[i]);
		DBGV("\n");
	}

	netPath_display(&ptpClock->netPath);
	DBGV("mCommunication technology %d \n", ptpClock->port_communication_technology);
	clockUUID_display(ptpClock->netPath.interfaceID);
	DBGV("\n");
}


/**\brief Display Buffer in & out of a PtpClock*/
void
displayBuffer(const PtpClock * ptpClock)
{

	int i;
	int j;

	j = 0;

	DBGV("PtpClock Buffer Out  \n");
	DBGV("\n");

	for (i = 0; i < PACKET_SIZE; i++) {
		DBGV(":%02hhx", ptpClock->msgObuf[i]);
		j++;

		if (j == 8) {
			DBGV(" ");

		}
		if (j == 16) {
			DBGV("\n");
			j = 0;
		}
	}
	DBGV("\n");
	j = 0;
	DBGV("\n");

	DBGV("PtpClock Buffer In  \n");
	DBGV("\n");
	for (i = 0; i < PACKET_SIZE; i++) {
		DBGV(":%02hhx", ptpClock->msgIbuf[i]);
		j++;

		if (j == 8) {
			DBGV(" ");

		}
		if (j == 16) {
			DBGV("\n");
			j = 0;
		}
	}
	DBGV("\n");
	DBGV("\n");
}

/**\convert port state to string*/
const char
*portState_getName(Enumeration8 portState)
{
    static const char *ptpStates[] = {
        [PTP_INITIALIZING] = "PTP_INITIALIZING",
        [PTP_FAULTY] = "PTP_FAULTY",
        [PTP_DISABLED] = "PTP_DISABLED",
        [PTP_LISTENING] = "PTP_LISTENING",
        [PTP_PRE_MASTER] = "PTP_PRE_MASTER",
        [PTP_MASTER] = "PTP_MASTER",
        [PTP_PASSIVE] = "PTP_PASSIVE",
        [PTP_UNCALIBRATED] = "PTP_UNCALIBRATED",
        [PTP_SLAVE] = "PTP_SLAVE"
    };

    /* converting to int to avoid compiler warnings when comparing enum*/
    static const int max = PTP_SLAVE;
    int intstate = portState;

    if( intstate < 0 || intstate > max ) {
        return("PTP_UNKNOWN");
    }

    return(ptpStates[portState]);

}

/**\brief Display all PTP clock (port) counters*/
void
displayCounters(const PtpClock * ptpClock)
{

	/* TODO: print port identity */
	INFO("\n============= PTP port counters =============\n");

	INFO("Message counters:\n");
	INFO("              announceMessagesSent : %lu\n",
		(unsigned long)ptpClock->counters.announceMessagesSent);
	INFO("          announceMessagesReceived : %lu\n",
		(unsigned long)ptpClock->counters.announceMessagesReceived);
	INFO("                  syncMessagesSent : %lu\n",
		(unsigned long)ptpClock->counters.syncMessagesSent);
	INFO("              syncMessagesReceived : %lu\n",
		(unsigned long)ptpClock->counters.syncMessagesReceived);
	INFO("              followUpMessagesSent : %lu\n",
		(unsigned long)ptpClock->counters.followUpMessagesSent);
	INFO("          followUpMessagesReceived : %lu\n",
		(unsigned long)ptpClock->counters.followUpMessagesReceived);
	INFO("              delayReqMessagesSent : %lu\n",
		 (unsigned long)ptpClock->counters.delayReqMessagesSent);
	INFO("          delayReqMessagesReceived : %lu\n",
		(unsigned long)ptpClock->counters.delayReqMessagesReceived);
	INFO("             delayRespMessagesSent : %lu\n",
		(unsigned long)ptpClock->counters.delayRespMessagesSent);
	INFO("         delayRespMessagesReceived : %lu\n",
		(unsigned long)ptpClock->counters.delayRespMessagesReceived);
	INFO("             pdelayReqMessagesSent : %lu\n",
		(unsigned long)ptpClock->counters.pdelayReqMessagesSent);
	INFO("         pdelayReqMessagesReceived : %lu\n",
		(unsigned long)ptpClock->counters.pdelayReqMessagesReceived);
	INFO("            pdelayRespMessagesSent : %lu\n",
		(unsigned long)ptpClock->counters.pdelayRespMessagesSent);
	INFO("        pdelayRespMessagesReceived : %lu\n",
		(unsigned long)ptpClock->counters.pdelayRespMessagesReceived);
	INFO("    pdelayRespFollowUpMessagesSent : %lu\n",
		(unsigned long)ptpClock->counters.pdelayRespFollowUpMessagesSent);
	INFO("pdelayRespFollowUpMessagesReceived : %lu\n",
		(unsigned long)ptpClock->counters.pdelayRespFollowUpMessagesReceived);
	INFO("             signalingMessagesSent : %lu\n",
		(unsigned long)ptpClock->counters.signalingMessagesSent);
	INFO("         signalingMessagesReceived : %lu\n",
		(unsigned long)ptpClock->counters.signalingMessagesReceived);
	INFO("            managementMessagesSent : %lu\n",
		(unsigned long)ptpClock->counters.managementMessagesSent);
	INFO("        managementMessagesReceived : %lu\n",
		(unsigned long)ptpClock->counters.managementMessagesReceived);

    if(ptpClock->counters.signalingMessagesReceived ||
	    ptpClock->counters.signalingMessagesSent) {
	INFO("Unicast negotiation counters:\n");
	INFO("            unicastGrantsRequested : %lu\n",
		(unsigned long)ptpClock->counters.unicastGrantsRequested);
	INFO("              unicastGrantsGranted : %lu\n",
		(unsigned long)ptpClock->counters.unicastGrantsGranted);
	INFO("               unicastGrantsDenied : %lu\n",
		(unsigned long)ptpClock->counters.unicastGrantsDenied);
	INFO("           unicastGrantsCancelSent : %lu\n",
		(unsigned long)ptpClock->counters.unicastGrantsCancelSent);
	INFO("       unicastGrantsCancelReceived : %lu\n",
		(unsigned long)ptpClock->counters.unicastGrantsCancelReceived);
	INFO("    unicastGrantsCancelAckReceived : %lu\n",
		(unsigned long)ptpClock->counters.unicastGrantsCancelAckReceived);
	INFO("        unicastGrantsCancelAckSent : %lu\n",
		(unsigned long)ptpClock->counters.unicastGrantsCancelAckSent);

    }
/* not implemented yet */
#if 0
	INFO("FMR counters:\n");
	INFO("                      foreignAdded : %lu\n",
		(unsigned long)ptpClock->counters.foreignAdded);
	INFO("                        foreignMax : %lu\n",
		(unsigned long)ptpClock->counters.foreignMax);
	INFO("                    foreignRemoved : %lu\n",
		(unsigned long)ptpClock->counters.foreignRemoved);
	INFO("                   foreignOverflow : %lu\n",
		(unsigned long)ptpClock->counters.foreignOverflow);
#endif /* 0 */

	INFO("Protocol engine counters:\n");
	INFO("                  stateTransitions : %lu\n",
		(unsigned long)ptpClock->counters.stateTransitions);
	INFO("                     bestMasterChanges : %lu\n",
		(unsigned long)ptpClock->counters.bestMasterChanges);
	INFO("                  announceTimeouts : %lu\n",
		(unsigned long)ptpClock->counters.announceTimeouts);

	INFO("Discarded / unknown message counters:\n");
	INFO("                 discardedMessages : %lu\n",
		(unsigned long)ptpClock->counters.discardedMessages);
	INFO("                   unknownMessages : %lu\n",
		(unsigned long)ptpClock->counters.unknownMessages);
	INFO("                   ignoredAnnounce : %lu\n",
		(unsigned long)ptpClock->counters.ignoredAnnounce);
	INFO("    aclManagementMessagesDiscarded : %lu\n",
		(unsigned long)ptpClock->counters.aclManagementMessagesDiscarded);
	INFO("        aclTimingMessagesDiscarded : %lu\n",
		(unsigned long)ptpClock->counters.aclTimingMessagesDiscarded);

	INFO("Error counters:\n");
	INFO("                 messageSendErrors : %lu\n",
		(unsigned long)ptpClock->counters.messageSendErrors);
	INFO("                 messageRecvErrors : %lu\n",
		(unsigned long)ptpClock->counters.messageRecvErrors);
	INFO("               messageFormatErrors : %lu\n",
		(unsigned long)ptpClock->counters.messageFormatErrors);
	INFO("                    protocolErrors : %lu\n",
		(unsigned long)ptpClock->counters.protocolErrors);
	INFO("             versionMismatchErrors : %lu\n",
		(unsigned long)ptpClock->counters.versionMismatchErrors);
	INFO("              domainMismatchErrors : %lu\n",
		(unsigned long)ptpClock->counters.domainMismatchErrors);
	INFO("            sequenceMismatchErrors : %lu\n",
		(unsigned long)ptpClock->counters.sequenceMismatchErrors);
	INFO("         consecutiveSequenceErrors : %lu\n",
		(unsigned long)ptpClock->counters.consecutiveSequenceErrors);
	INFO("           delayMechanismMismatchErrors : %lu\n",
		(unsigned long)ptpClock->counters.delayMechanismMismatchErrors);
	INFO("           maxDelayDrops : %lu\n",
		(unsigned long)ptpClock->counters.maxDelayDrops);


#ifdef PTPD_STATISTICS
	INFO("Outlier filter hits:\n");
	INFO("              delayMSOutliersFound : %lu\n",
		(unsigned long)ptpClock->counters.delayMSOutliersFound);
	INFO("              delaySMOutliersFound : %lu\n",
		(unsigned long)ptpClock->counters.delaySMOutliersFound);
#endif /* PTPD_STATISTICS */

}

const char *
getTimeSourceName(Enumeration8 timeSource)
{

    switch(timeSource) {

        case  ATOMIC_CLOCK:
	    return "ATOMIC_CLOCK";
	case  GPS:
	    return "GPS";
	case  TERRESTRIAL_RADIO:
	    return "TERRERSTRIAL_RADIO";
	case  PTP:
	    return "PTP";
	case  NTP:
	    return "NTP";
	case  HAND_SET:
	    return "HAND_SET";
	case  OTHER:
	    return "OTHER";
	case  INTERNAL_OSCILLATOR:
	    return "INTERNAL_OSCILLATOR";
	default:
	    return "UNKNOWN";
    }

}

const char *
getMessageTypeName(Enumeration8 messageType)
{
    switch(messageType)
    {
    case ANNOUNCE:
	return("Announce");
	break;
    case SYNC:
	return("Sync");
	break;
    case FOLLOW_UP:
	return("FollowUp");
	break;
    case DELAY_REQ:
	return("DelayReq");
	break;
    case DELAY_RESP:
	return("DelayResp");
	break;
    case PDELAY_REQ:
	return("PdelayReq");
	break;
    case PDELAY_RESP:
	return("PdelayResp");
	break;
    case PDELAY_RESP_FOLLOW_UP:
	return("PdelayRespFollowUp");
	break;	
    case MANAGEMENT:
	return("Management");
	break;
    case SIGNALING:
	return("Signaling");
	break;
    default:
	return("Unknown");
	break;
    }

}

const char* accToString(uint8_t acc) {

	switch(acc) {

	    case ACC_25NS:
		return "ACC_25NS";
	    case ACC_100NS:
		return "ACC_100NS";
	    case ACC_250NS:
		return "ACC_250NS";
	    case ACC_1US:
		return "ACC_1US";
	    case ACC_2_5US:
		return "ACC_2_5US";
	    case ACC_10US:
		return "ACC_10US";
	    case ACC_25US:
		return "ACC_25US";
	    case ACC_100US:
		return "ACC_100US";
	    case ACC_250US:
		return "ACC_250US";
	    case ACC_1MS:
		return "ACC_1MS";
	    case ACC_2_5MS:
		return "ACC_2_5MS";
	    case ACC_10MS:
		return "ACC_10MS";
	    case ACC_25MS:
		return "ACC_25MS";
	    case ACC_100MS:
		return "ACC_100MS";
	    case ACC_250MS:
		return "ACC_250MS";
	    case ACC_1S:
		return "ACC_1S";
	    case ACC_10S:
		return "ACC_10S";
	    case ACC_10SPLUS:
		return "ACC_10SPLUS";
	    case ACC_UNKNOWN:
		return "ACC_UNKNOWN";
	    default:
		return NULL;

	}
}

const char* delayMechToString(uint8_t mech) {

	switch(mech) {
	    case E2E:
		return "E2E";
	    case P2P:
		return "P2P";
	    case DELAY_DISABLED:
		return "DELAY_DISABLED";
	    default:
		return NULL;
	}

}

/**\brief Display all PTP clock (port) statistics*/
void
displayStatistics(const PtpClock* ptpClock)
{

/*
	INFO("Clock stats: ofm mean: %lu, ofm median: %d,"
	     "ofm std dev: %lu, observed drift std dev: %d\n",
	     ptpClock->stats.ofmMean, ptpClock->stats.ofmMedian,
	     ptpClock->stats.ofmStdDev, ptpClock->stats.driftStdDev);
*/

}

/**\brief Display All data sets and counters of a PtpClock*/
void
displayPtpClock(const PtpClock * ptpClock)
{

	displayDefault(ptpClock);
	displayCurrent(ptpClock);
	displayParent(ptpClock);
	displayGlobal(ptpClock);
	displayPort(ptpClock);
	displayForeignMaster(ptpClock);
	displayBuffer(ptpClock);
	displayOthers(ptpClock);
	displayCounters(ptpClock);
	displayStatistics(ptpClock);

}
