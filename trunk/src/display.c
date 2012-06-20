/*-
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
integer64_display(Integer64 * bigint)
{
	DBGV("Integer 64 : \n");
	DBGV("LSB : %u\n", bigint->lsb);
	DBGV("MSB : %d\n", bigint->msb);
}

/**\brief Display an UInteger48 type*/
void
uInteger48_display(UInteger48 * bigint)
{
	DBGV("Integer 48 : \n");
	DBGV("LSB : %u\n", bigint->lsb);
	DBGV("MSB : %u\n", bigint->msb);
}

/** \brief Display a TimeInternal Structure*/
void
timeInternal_display(TimeInternal * timeInternal)
{
	DBGV("seconds : %d \n", timeInternal->seconds);
	DBGV("nanoseconds %d \n", timeInternal->nanoseconds);
}

/** \brief Display a Timestamp Structure*/
void
timestamp_display(Timestamp * timestamp)
{
	uInteger48_display(&timestamp->secondsField);
	DBGV("nanoseconds %u \n", timestamp->nanosecondsField);
}

/**\brief Display a Clockidentity Structure*/
void
clockIdentity_display(ClockIdentity clockIdentity)
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
clockUUID_display(Octet * sourceUuid)
{
	DBGV(
	    "sourceUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	    sourceUuid[0], sourceUuid[1], sourceUuid[2], sourceUuid[3],
	    sourceUuid[4], sourceUuid[5]
	);
}

/**\brief Display Network info*/
void
netPath_display(NetPath * net)
{
	struct in_addr addr;

	DBGV("eventSock : %d \n", net->eventSock);
	DBGV("generalSock : %d \n", net->generalSock);
	addr.s_addr = net->multicastAddr;
	DBGV("multicastAdress : %s \n", inet_ntoa(addr));
	addr.s_addr = net->peerMulticastAddr;
	DBGV("peerMulticastAddress : %s \n", inet_ntoa(addr));
	addr.s_addr = net->unicastAddr;
	DBGV("unicastAddress : %s \n", inet_ntoa(addr));
}

/**\brief Display a IntervalTimer Structure*/
void
intervalTimer_display(IntervalTimer * ptimer)
{
	DBGV("interval : %d \n", ptimer->interval);
	DBGV("left : %d \n", ptimer->left);
	DBGV("expire : %d \n", ptimer->expire);
}

/**\brief Display a TimeInterval Structure*/
void
timeInterval_display(TimeInterval * timeInterval)
{
	integer64_display(&timeInterval->scaledNanoseconds);
}

/**\brief Display a Portidentity Structure*/
void
portIdentity_display(PortIdentity * portIdentity)
{
	clockIdentity_display(portIdentity->clockIdentity);
	DBGV("port number : %d \n", portIdentity->portNumber);
}

/**\brief Display a Clockquality Structure*/
void
clockQuality_display(ClockQuality * clockQuality)
{
	DBGV("clockClass : %d \n", clockQuality->clockClass);
	DBGV("clockAccuracy : %d \n", clockQuality->clockAccuracy);
	DBGV("offsetScaledLogVariance : %d \n", clockQuality->offsetScaledLogVariance);
}

/**\brief Display PTPText Structure*/
void
PTPText_display(PTPText *p, PtpClock *ptpClock)
{
	/* Allocate new memory to append null-terminator to the text field.
	 * This allows printing the textField as a string
	 */
	Octet *str;
	XMALLOC(str, p->lengthField + 1);
	memcpy(str, p->textField, p->lengthField);
	str[p->lengthField] = '\0';
	DBGV("    lengthField : %d \n", p->lengthField);
	DBGV("    textField : %s \n", str);
	free(str);
}

/**\brief Display the Network Interface Name*/
void
iFaceName_display(Octet * iFaceName)
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
unicast_display(Octet * unicast)
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
msgSync_display(MsgSync * sync)
{
	DBGV("Message Sync : \n");
	timestamp_display(&sync->originTimestamp);
	DBGV("\n");
}

/**\brief Display Header message*/
void
msgHeader_display(MsgHeader * header)
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
msgAnnounce_display(MsgAnnounce * announce)
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
msgFollowUp_display(MsgFollowUp * follow)
{
	timestamp_display(&follow->preciseOriginTimestamp);
}

/**\brief Display DelayReq message*/
void
msgDelayReq_display(MsgDelayReq * req)
{
	timestamp_display(&req->originTimestamp);
}

/**\brief Display DelayResp message*/
void
msgDelayResp_display(MsgDelayResp * resp)
{
	timestamp_display(&resp->receiveTimestamp);
	portIdentity_display(&resp->requestingPortIdentity);
}

/**\brief Display Pdelay_Req message*/
void
msgPDelayReq_display(MsgPDelayReq * preq)
{
	timestamp_display(&preq->originTimestamp);
}

/**\brief Display Pdelay_Resp message*/
void
msgPDelayResp_display(MsgPDelayResp * presp)
{

	timestamp_display(&presp->requestReceiptTimestamp);
	portIdentity_display(&presp->requestingPortIdentity);
}

/**\brief Display Pdelay_Resp Follow Up message*/
void
msgPDelayRespFollowUp_display(MsgPDelayRespFollowUp * prespfollow)
{

	timestamp_display(&prespfollow->responseOriginTimestamp);
	portIdentity_display(&prespfollow->requestingPortIdentity);
}

/**\brief Display Management message*/
void
msgManagement_display(MsgManagement * manage)
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
mMSlaveOnly_display(MMSlaveOnly *slaveOnly, PtpClock *ptpClock)
{
	DBGV("Slave Only ManagementTLV message \n");
	DBGV("SO : %d \n", slaveOnly->so);
}

/**\brief Display ManagementTLV Clock Description message*/
void
mMClockDescription_display(MMClockDescription *clockDescription, PtpClock *ptpClock)
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
mMUserDescription_display(MMUserDescription* userDescription, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMInitialize_display(MMInitialize* initialize, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMDefaultDataSet_display(MMDefaultDataSet* defaultDataSet, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMCurrentDataSet_display(MMCurrentDataSet* currentDataSet, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMParentDataSet_display(MMParentDataSet* parentDataSet, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMTimePropertiesDataSet_display(MMTimePropertiesDataSet* timePropertiesDataSet, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMPortDataSet_display(MMPortDataSet* portDataSet, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMPriority1_display(MMPriority1* priority1, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMPriority2_display(MMPriority2* priority2, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMDomain_display(MMDomain* domain, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMLogAnnounceInterval_display(MMLogAnnounceInterval* logAnnounceInterval, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMAnnounceReceiptTimeout_display(MMAnnounceReceiptTimeout* announceReceiptTimeout, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMLogSyncInterval_display(MMLogSyncInterval* logSyncInterval, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMVersionNumber_display(MMVersionNumber* versionNumber, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMTime_display(MMTime* time, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMClockAccuracy_display(MMClockAccuracy* clockAccuracy, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMUtcProperties_display(MMUtcProperties* utcProperties, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMTraceabilityProperties_display(MMTraceabilityProperties* traceabilityProperties, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMDelayMechanism_display(MMDelayMechanism* delayMechanism, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMLogMinPdelayReqInterval_display(MMLogMinPdelayReqInterval* logMinPdelayReqInterval, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

void
mMErrorStatus_display(MMErrorStatus* errorStatus, PtpClock *ptpClock)
{
	/* TODO: implement me */
}

/**\brief Display runTimeOptions structure*/
void
displayRunTimeOpts(RunTimeOpts * rtOpts)
{

	DBGV("---Run time Options Display-- \n");
	DBGV("\n");
	DBGV("announceInterval : %d \n", rtOpts->announceInterval);
	DBGV("syncInterval : %d \n", rtOpts->syncInterval);
	clockQuality_display(&(rtOpts->clockQuality));
	DBGV("priority1 : %d \n", rtOpts->priority1);
	DBGV("priority2 : %d \n", rtOpts->priority2);
	DBGV("domainNumber : %d \n", rtOpts->domainNumber);
	DBGV("slaveOnly : %d \n", rtOpts->slaveOnly);
	DBGV("currentUtcOffset : %d \n", rtOpts->currentUtcOffset);
	unicast_display(rtOpts->unicastAddress);
	DBGV("noAdjust : %d \n", rtOpts->noAdjust);
	DBGV("displayStats : %d \n", rtOpts->displayStats);
	iFaceName_display(rtOpts->ifaceName);
	DBGV("ap : %d \n", rtOpts->ap);
	DBGV("aI : %d \n", rtOpts->ai);
	DBGV("s : %d \n", rtOpts->s);
	DBGV("inbound latency : \n");
	timeInternal_display(&(rtOpts->inboundLatency));
	DBGV("outbound latency : \n");
	timeInternal_display(&(rtOpts->outboundLatency));
	DBGV("max_foreign_records : %d \n", rtOpts->max_foreign_records);
	DBGV("ethernet mode : %d \n", rtOpts->ethernet_mode);
	DBGV("\n");
}


/**\brief Display Default data set of a PtpClock*/
void
displayDefault(PtpClock * ptpClock)
{
	DBGV("---Ptp Clock Default Data Set-- \n");
	DBGV("\n");
	DBGV("twoStepFlag : %d \n", ptpClock->twoStepFlag);
	clockIdentity_display(ptpClock->clockIdentity);
	DBGV("numberPorts : %d \n", ptpClock->numberPorts);
	clockQuality_display(&(ptpClock->clockQuality));
	DBGV("priority1 : %d \n", ptpClock->priority1);
	DBGV("priority2 : %d \n", ptpClock->priority2);
	DBGV("domainNumber : %d \n", ptpClock->domainNumber);
	DBGV("slaveOnly : %d \n", ptpClock->slaveOnly);
	DBGV("\n");
}


/**\brief Display Current data set of a PtpClock*/
void
displayCurrent(PtpClock * ptpClock)
{
	DBGV("---Ptp Clock Current Data Set-- \n");
	DBGV("\n");

	DBGV("stepsremoved : %d \n", ptpClock->stepsRemoved);
	DBGV("Offset from master : \n");
	timeInternal_display(&ptpClock->offsetFromMaster);
	DBGV("Mean path delay : \n");
	timeInternal_display(&ptpClock->meanPathDelay);
	DBGV("\n");
}



/**\brief Display Parent data set of a PtpClock*/
void
displayParent(PtpClock * ptpClock)
{
	DBGV("---Ptp Clock Parent Data Set-- \n");
	DBGV("\n");
	portIdentity_display(&(ptpClock->parentPortIdentity));
	DBGV("parentStats : %d \n", ptpClock->parentStats);
	DBGV("observedParentOffsetScaledLogVariance : %d \n", ptpClock->observedParentOffsetScaledLogVariance);
	DBGV("observedParentClockPhaseChangeRate : %d \n", ptpClock->observedParentClockPhaseChangeRate);
	DBGV("--GrandMaster--\n");
	clockIdentity_display(ptpClock->grandmasterIdentity);
	clockQuality_display(&ptpClock->grandmasterClockQuality);
	DBGV("grandmasterpriority1 : %d \n", ptpClock->grandmasterPriority1);
	DBGV("grandmasterpriority2 : %d \n", ptpClock->grandmasterPriority2);
	DBGV("\n");
}

/**\brief Display Global data set of a PtpClock*/
void
displayGlobal(PtpClock * ptpClock)
{
	DBGV("---Ptp Clock Global Time Data Set-- \n");
	DBGV("\n");

	DBGV("currentUtcOffset : %d \n", ptpClock->currentUtcOffset);
	DBGV("currentUtcOffsetValid : %d \n", ptpClock->currentUtcOffsetValid);
	DBGV("leap59 : %d \n", ptpClock->leap59);
	DBGV("leap61 : %d \n", ptpClock->leap61);
	DBGV("timeTraceable : %d \n", ptpClock->timeTraceable);
	DBGV("frequencyTraceable : %d \n", ptpClock->frequencyTraceable);
	DBGV("ptpTimescale : %d \n", ptpClock->ptpTimescale);
	DBGV("timeSource : %d \n", ptpClock->timeSource);
	DBGV("\n");
}

/**\brief Display Port data set of a PtpClock*/
void
displayPort(PtpClock * ptpClock)
{
	DBGV("---Ptp Clock Port Data Set-- \n");
	DBGV("\n");

	portIdentity_display(&ptpClock->portIdentity);
	DBGV("port state : %d \n", ptpClock->portState);
	DBGV("logMinDelayReqInterval : %d \n", ptpClock->logMinDelayReqInterval);
	DBGV("peerMeanPathDelay : \n");
	timeInternal_display(&ptpClock->peerMeanPathDelay);
	DBGV("logAnnounceInterval : %d \n", ptpClock->logAnnounceInterval);
	DBGV("announceReceiptTimeout : %d \n", ptpClock->announceReceiptTimeout);
	DBGV("logSyncInterval : %d \n", ptpClock->logSyncInterval);
	DBGV("delayMechanism : %d \n", ptpClock->delayMechanism);
	DBGV("logMinPdelayReqInterval : %d \n", ptpClock->logMinPdelayReqInterval);
	DBGV("versionNumber : %d \n", ptpClock->versionNumber);
	DBGV("\n");
}

/**\brief Display ForeignMaster data set of a PtpClock*/
void
displayForeignMaster(PtpClock * ptpClock)
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
displayOthers(PtpClock * ptpClock)
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
	DBGV("sentPdelayReq : %d \n", ptpClock->sentPDelayReq);
	DBGV("sentPDelayReqSequenceId : %d \n", ptpClock->sentPDelayReqSequenceId);
	DBGV("waitingForFollow : %d \n", ptpClock->waitingForFollow);
	DBGV("\n");
	DBGV("Offset from master filter : \n");
	DBGV("nsec_prev : %d \n", ptpClock->ofm_filt.nsec_prev);
	DBGV("y : %d \n", ptpClock->ofm_filt.y);
	DBGV("\n");
	DBGV("One way delay filter : \n");
	DBGV("nsec_prev : %d \n", ptpClock->owd_filt.nsec_prev);
	DBGV("y : %d \n", ptpClock->owd_filt.y);
	DBGV("s_exp : %d \n", ptpClock->owd_filt.s_exp);
	DBGV("\n");
	DBGV("observed_drift : %d \n", ptpClock->observed_drift);
	DBGV("message activity %d \n", ptpClock->message_activity);
	DBGV("\n");

	for (i = 0; i < TIMER_ARRAY_SIZE; i++) {
		DBGV("%s : \n", timer[i]);
		intervalTimer_display(&ptpClock->itimer[i]);
		DBGV("\n");
	}

	netPath_display(&ptpClock->netPath);
	DBGV("mCommunication technology %d \n", ptpClock->port_communication_technology);
	clockUUID_display(ptpClock->port_uuid_field);
	DBGV("\n");
}


/**\brief Display Buffer in & out of a PtpClock*/
void
displayBuffer(PtpClock * ptpClock)
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




/**\brief Display All data set of a PtpClock*/
void
displayPtpClock(PtpClock * ptpClock)
{

	displayDefault(ptpClock);
	displayCurrent(ptpClock);
	displayParent(ptpClock);
	displayGlobal(ptpClock);
	displayPort(ptpClock);
	displayForeignMaster(ptpClock);
	displayBuffer(ptpClock);
	displayOthers(ptpClock);

}
