
#include "ptpd.h"

/**\brief Display an Integer64 type*/
void integer64_display (Integer64 *bigint){
	printf("Integer 64 : \n");
	printf("LSB : %u\n",bigint->lsb);
	printf("MSB : %d\n",bigint->msb);
}

/**\brief Display an UInteger48 type*/
void uInteger48_display(UInteger48 *bigint){
	printf("Integer 48 : \n");
	printf("LSB : %u\n",bigint->lsb);
	printf("MSB : %u\n",bigint->msb);
}

/** \brief Display a TimeInternal Structure*/
void timeInternal_display(TimeInternal *timeInternal) {
	printf("seconds : %d \n",timeInternal->seconds);
	printf("nanoseconds %d \n",timeInternal->nanoseconds);
}

/** \brief Display a Timestamp Structure*/
void timestamp_display(Timestamp *timestamp) {
	uInteger48_display(&timestamp->secondsField);
	printf("nanoseconds %u \n",timestamp->nanosecondsField);
}

/**\brief Display a Clockidentity Structure*/
void clockIdentity_display(ClockIdentity clockIdentity){
		
printf(
    "ClockIdentity : %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
    clockIdentity[0], clockIdentity[1], clockIdentity[2],
    clockIdentity[3], clockIdentity[4], clockIdentity[5],
    clockIdentity[6],clockIdentity[7]
);

}

/**\brief Display MAC address*/
void clockUUID_display(Octet *sourceUuid){
		
printf(
    "sourceUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
    sourceUuid[0], sourceUuid[1], sourceUuid[2],
    sourceUuid[3], sourceUuid[4], sourceUuid[5]
);

}


/**\brief Display Network info*/
void netPath_display(NetPath *net){
	struct in_addr addr;
	
	printf("eventSock : %d \n",net->eventSock);
	printf("generalSock : %d \n",net->generalSock);
	addr.s_addr=net->multicastAddr;
	printf("multicastAdress : %s \n",inet_ntoa(addr));
	addr.s_addr=net->peerMulticastAddr;
	printf("peerMulticastAddress : %s \n",inet_ntoa(addr));
	addr.s_addr=net->unicastAddr;
	printf("unicastAddress : %s \n",inet_ntoa(addr));
}	

/**\brief Display a IntervalTimer Structure*/
void intervalTimer_display(IntervalTimer *ptimer){
	printf("interval : %d \n",ptimer->interval);
	printf("left : %d \n",ptimer->left);
	printf("expire : %d \n",ptimer->expire);
}




/**\brief Display a TimeInterval Structure*/
void timeInterval_display(TimeInterval *timeInterval){
	integer64_display(&timeInterval->scaledNanoseconds);
}
	

/**\brief Display a Portidentity Structure*/	
void portIdentity_display(PortIdentity *portIdentity){	
	clockIdentity_display((char*)portIdentity->clockIdentity);
	printf("port number : %d \n",portIdentity->portNumber);
	
}

/**\brief Display a Clockquality Structure*/
void clockQuality_display (ClockQuality *clockQuality){
	printf("clockClass : %d \n",clockQuality->clockClass);
	printf("clockAccuracy : %d \n",clockQuality->clockAccuracy);
	printf("offsetScaledLogVariance : %d \n",clockQuality->offsetScaledLogVariance);
}


/**\brief Display the Network Interface Name*/
void iFaceName_display(Octet *iFaceName){
	
int i ;
printf("iFaceName : "); 

for (i=0;i<IFACE_NAME_LENGTH;i++){
	printf("%c",iFaceName[i]); 
}
printf("\n");

}

/**\brief Display an Unicast Adress*/
void unicast_display(Octet *unicast){
	
int i ;
printf("Unicast adress : "); 

for (i=0;i<NET_ADDRESS_LENGTH;i++){
	printf("%c",unicast[i]); 
}
printf("\n");

}	


/**\brief Display Sync message*/
void msgSync_display(MsgSync *sync){
	printf("Message Sync : \n");
	printf("\n");
	timestamp_display(&sync->originTimestamp);
	printf("\n");
}

/**\brief Display Header message*/
void msgHeader_display(MsgHeader *header){
	printf("Message header : \n");
	printf("\n");
 	printf("transportSpecific : %d\n",header->transportSpecific);
 	printf("messageType : %d\n",header->messageType);
 	printf("versionPTP : %d\n",header->versionPTP);
 	printf("messageLength : %d\n",header->messageLength);
 	printf("domainNumber : %d\n",header->domainNumber);
 	printf("FlagField %02hhx:%02hhx\n",header->flagField[0],header->flagField[1]);
 	integer64_display(&header->correctionfield);
	portIdentity_display(&header->sourcePortIdentity);
	printf("sequenceId : %d\n",header->sequenceId);
	printf("controlField : %d\n",header->controlField);
	printf("logMessageInterval : %d\n",header->logMessageInterval);
	printf("\n");
}

/**\brief Display Announce message*/
void msgAnnounce_display(MsgAnnounce *announce){
	printf("Announce Message : \n");
	printf("\n");
	printf("originTimestamp : \n");
	printf("secondField  : \n");
	timestamp_display(&announce->originTimestamp);
	printf("currentUtcOffset : %d \n",announce->currentUtcOffset);
	printf("grandMasterPriority1 : %d \n",announce->grandmasterPriority1);
	printf("grandMasterClockQuality : \n");
	clockQuality_display(&announce->grandmasterClockQuality);
	printf("grandMasterPriority2 : %d \n",announce->grandmasterPriority2);
	printf("grandMasterIdentity : \n");
	clockIdentity_display(announce->grandmasterIdentity);
	printf("stepsRemoved : %d \n",announce->stepsRemoved);
	printf("timeSource : %d \n",announce->timeSource);
	printf("\n");
}

/**\brief Display Follow_UP message*/
void msgFollowUp_display(MsgFollowUp *follow){
	timestamp_display(&follow->preciseOriginTimestamp);
}


/**\brief Display Pdelay_Req message*/
void msgPDelayReq_display(MsgPDelayReq *preq){

timestamp_display(&preq->originTimestamp);
}

/**\brief Display Pdelay_Resp message*/
void msgPDelayResp_display(MsgPDelayResp *presp){

timestamp_display(&presp->requestReceiptTimestamp);
portIdentity_display(&presp->requestingPortIdentity);
}

/**\brief Display Pdelay_Resp Follow Up message*/
void msgPDelayRespFollowUp_display(MsgPDelayRespFollowUp *prespfollow){

timestamp_display(&prespfollow->responseOriginTimestamp);
portIdentity_display(&prespfollow->requestingPortIdentity);
}
	
/**\brief Display runTimeOptions structure*/
void displayRunTimeOpts(RunTimeOpts* rtOpts){

printf("---Run time Options Display-- \n");
printf("\n");
printf("announceInterval : %d \n",rtOpts->announceInterval);	
printf("syncInterval : %d \n",rtOpts->syncInterval);
clockQuality_display(&(rtOpts->clockQuality));
printf("priority1 : %d \n",rtOpts->priority1);
printf("priority2 : %d \n",rtOpts->priority2);
printf("domainNumber : %d \n",rtOpts->domainNumber);
printf("slaveOnly : %d \n",rtOpts->slaveOnly);
printf("currentUtcOffset : %d \n",rtOpts->currentUtcOffset);
unicast_display(rtOpts->unicastAddress);
printf("noResetClock : %d \n",rtOpts->noResetClock);
printf("noAdjust : %d \n",rtOpts->noAdjust);
printf("displayStats : %d \n",rtOpts->displayStats);
printf("csvStats : %d \n",rtOpts->csvStats);
iFaceName_display(rtOpts->ifaceName);
printf("ap : %d \n",rtOpts->ap);
printf("aI : %d \n",rtOpts->ai);
printf("s : %d \n",rtOpts->s);
printf("inbound latency : \n");
timeInternal_display(&(rtOpts->inboundLatency));
printf("outbound latency : \n");
timeInternal_display(&(rtOpts->outboundLatency));
printf("max_foreign_records : %d \n",rtOpts->max_foreign_records);
printf("ethernet mode : %d \n",rtOpts->ethernet_mode);
printf("\n");
}


/**\brief Display Default data set of a PtpClock*/
void displayDefault (PtpClock *ptpClock){
	
printf("---Ptp Clock Default Data Set-- \n");
printf("\n");
printf("twoStepFlag : %d \n",ptpClock->twoStepFlag);
clockIdentity_display(ptpClock->clockIdentity);
printf("numberPorts : %d \n",ptpClock->numberPorts);
clockQuality_display(&(ptpClock->clockQuality));		
printf("priority1 : %d \n",ptpClock->priority1);
printf("priority2 : %d \n",ptpClock->priority2);
printf("domainNumber : %d \n",ptpClock->domainNumber);
printf("slaveOnly : %d \n",ptpClock->slaveOnly);
printf("\n");
}


/**\brief Display Current data set of a PtpClock*/
void displayCurrent (PtpClock *ptpClock){
	
printf("---Ptp Clock Current Data Set-- \n");
printf("\n");

printf("stepsremoved : %d \n",ptpClock->stepsRemoved);
printf("Offset from master : \n");
timeInternal_display(&ptpClock->offsetFromMaster);
printf("Mean path delay : \n");
timeInternal_display(&ptpClock->meanPathDelay);
printf("\n");
}



/**\brief Display Parent data set of a PtpClock*/
void displayParent (PtpClock *ptpClock){
	
printf("---Ptp Clock Parent Data Set-- \n");
printf("\n");
portIdentity_display(&(ptpClock->parentPortIdentity));
printf("parentStats : %d \n",ptpClock->parentStats);
printf("observedParentOffsetScaledLogVariance : %d \n",ptpClock->observedParentOffsetScaledLogVariance);
printf("observedParentClockPhaseChangeRate : %d \n",ptpClock->observedParentClockPhaseChangeRate); 
printf("--GrandMaster--\n"); 		
clockIdentity_display(ptpClock->grandmasterIdentity);
clockQuality_display(&ptpClock->grandmasterClockQuality);		
printf("grandmasterpriority1 : %d \n",ptpClock->grandmasterPriority1);
printf("grandmasterpriority2 : %d \n",ptpClock->grandmasterPriority2); 		
printf("\n");
}
	
/**\brief Display Global data set of a PtpClock*/	
void displayGlobal (PtpClock *ptpClock){
	
printf("---Ptp Clock Global Time Data Set-- \n");
printf("\n");	

printf("currentUtcOffset : %d \n",ptpClock->currentUtcOffset);
printf("currentUtcOffsetValid : %d \n",ptpClock->currentUtcOffsetValid);
printf("leap59 : %d \n",ptpClock->leap59);
printf("leap61 : %d \n",ptpClock->leap61);
printf("timeTraceable : %d \n",ptpClock->timeTraceable);
printf("frequencyTraceable : %d \n",ptpClock->frequencyTraceable);
printf("ptpTimescale : %d \n",ptpClock->ptpTimescale);
printf("timeSource : %d \n",ptpClock->timeSource);
printf("\n");		
}

/**\brief Display Port data set of a PtpClock*/
void displayPort (PtpClock *ptpClock){
	
printf("---Ptp Clock Port Data Set-- \n");
printf("\n");

portIdentity_display(&ptpClock->portIdentity);
printf("port state : %d \n",ptpClock->portState);
printf("logMinDelayReqInterval : %d \n",ptpClock->logMinDelayReqInterval);
printf("peerMeanPathDelay : \n");
timeInternal_display(&ptpClock->peerMeanPathDelay);
printf("logAnnounceInterval : %d \n",ptpClock->logAnnounceInterval);
printf("announceReceiptTimeout : %d \n",ptpClock->announceReceiptTimeout);
printf("logSyncInterval : %d \n",ptpClock->logSyncInterval);
printf("delayMechanism : %d \n",ptpClock->delayMechanism);
printf("logMinPdelayReqInterval : %d \n",ptpClock->logMinPdelayReqInterval);
printf("versionNumber : %d \n",ptpClock->versionNumber);
printf("\n");		
}
	
/**\brief Display ForeignMaster data set of a PtpClock*/
void displayForeignMaster (PtpClock *ptpClock){
	
	ForeignMasterRecord *foreign;
	int i;
	
	if (ptpClock->number_foreign_records > 0){
		
		printf("---Ptp Clock Foreign Data Set-- \n");
		printf("\n");
		printf("There is %d Foreign master Recorded \n",ptpClock->number_foreign_records);
		foreign = ptpClock->foreign;

			for (i=0;i<ptpClock->number_foreign_records;i++){
	
				portIdentity_display(&foreign->foreignMasterPortIdentity);
				printf("number of Announce message received : %d \n",foreign->foreignMasterAnnounceMessages);
				msgHeader_display(&foreign->header);
				msgAnnounce_display(&foreign->announce);
				
				foreign++;
			}

	}
	
	else 
	{printf("No Foreign masters recorded \n");}
	
	printf("\n");

	
}

/**\brief Display other data set of a PtpClock*/

void displayOthers (PtpClock *ptpClock){
	
int i ;

//Usefull to display name of timers
char timer[][26] = {
"PDELAYREQ_INTERVAL_TIMER",
"SYNC_INTERVAL_TIMER",
"ANNOUNCE_RECEIPT_TIMER",
"ANNOUNCE_INTERVAL_TIMER"
};

printf("---Ptp Others Data Set-- \n");
printf("\n");
printf("master_to_slave_delay : \n");
timeInternal_display(&ptpClock->master_to_slave_delay);
printf("\n");
printf("slave_to_master_delay : \n");
timeInternal_display(&ptpClock->slave_to_master_delay);
printf("\n");
printf("delay_req_receive_time : \n");
timeInternal_display(&ptpClock->pdelay_req_receive_time);
printf("\n");
printf("delay_req_send_time : \n");
timeInternal_display(&ptpClock->pdelay_req_send_time);
printf("\n");
printf("delay_resp_receive_time : \n");
timeInternal_display(&ptpClock->pdelay_resp_receive_time);
printf("\n");
printf("delay_resp_send_time : \n");
timeInternal_display(&ptpClock->pdelay_resp_send_time);
printf("\n");
printf("sync_receive_time : \n");
timeInternal_display(&ptpClock->sync_receive_time);
printf("\n");
printf("R : %f \n",ptpClock->R);
printf("sentPdelayReq : %d \n",ptpClock->sentPDelayReq);
printf("sentPDelayReqSequenceId : %d \n",ptpClock->sentPDelayReqSequenceId);  
printf("waitingForFollow : %d \n",ptpClock->waitingForFollow);
printf("\n");
printf("Offset from master filter : \n");
printf("nsec_prev : %d \n",ptpClock->ofm_filt.nsec_prev);
printf("y : %d \n",ptpClock->ofm_filt.y);  
printf("\n");
printf("One way delay filter : \n");
printf("nsec_prev : %d \n",ptpClock->owd_filt.nsec_prev);
printf("y : %d \n",ptpClock->owd_filt.y); 
printf("s_exp : %d \n",ptpClock->owd_filt.s_exp);
printf("\n");
printf("observed_drift : %d \n",ptpClock->observed_drift); 
printf("message activity %d \n",ptpClock->message_activity);
printf("\n");

for (i=0;i<TIMER_ARRAY_SIZE;i++){
	printf("%s : \n",timer[i]);
	intervalTimer_display(&ptpClock->itimer[i]);
	printf("\n");	
}

netPath_display(&ptpClock->netPath);
printf("mCommunication technology %d \n",ptpClock->port_communication_technology);
clockUUID_display(ptpClock->port_uuid_field);
printf("\n");
}


/**\brief Display Buffer in & out of a PtpClock*/
void displayBuffer (PtpClock *ptpClock){
	
	int i;
	int j;
	j=0;
	
	printf("PtpClock Buffer Out  \n");
	printf("\n");
	
 	for (i=0;i<PACKET_SIZE;i++){
 		printf(":%02hhx",ptpClock->msgObuf[i]);
 		j++;
 		
 		if (j==8){
 			printf(" ");
 			
 		}
 		
 		if (j==16){
 			printf("\n");
 			j=0;
 		}
 	}
 printf("\n");
 j=0;
 printf("\n");
 
	printf("PtpClock Buffer In  \n");
	printf("\n");
 	for (i=0;i<PACKET_SIZE;i++){
 		printf(":%02hhx",ptpClock->msgIbuf[i]);
 		j++;
 		
 		if (j==8){
 			printf(" ");
 			
 		}
 		
 		if (j==16){
 			printf("\n");
 			j=0;
 		}
 	}
 	printf("\n");
 	printf("\n");
}




/**\brief Display All data set of a PtpClock*/
void displayPtpClock (PtpClock *ptpClock){
	
displayDefault (ptpClock);
displayCurrent (ptpClock);
displayParent (ptpClock);
displayGlobal(ptpClock);
displayPort(ptpClock);
displayForeignMaster(ptpClock);
displayBuffer(ptpClock);
displayOthers (ptpClock);
	
}

