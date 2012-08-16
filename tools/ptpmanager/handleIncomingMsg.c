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
 * @file   handleIncomingMsg.c
 * @date   Thurs July 5 01:40:38 2012 IST
 * @author Himanshu Singh
 * @brief  Functions  to parse the response/ack/error message received after 
 *         sending a management message and displays the field values 
 *         received message.
 */

#include "ptpmanager.h"

/* Unpack ClockIdentity Structure */
void
unpackClockIdentity(ClockIdentity dest, ClockIdentity src)
{
	memcpy(dest, src, CLOCK_IDENTITY_LENGTH);
}

/*Unpack Port Identity structure */
void
unpackPortIdentity(PortIdentity *dest, Octet *src)
{
	unpackClockIdentity(dest->clockIdentity, src);
	dest->portNumber = flip16(*((UInteger16*)(src + 8)));
}

/*Unpack Clock Quality structure */
void
unpackClockQuality(ClockQuality *dest, Octet *src)
{
	dest->clockClass = *(UInteger8*)(src);
	dest->clockAccuracy = *(Enumeration8*)(src + 1);
	dest->offsetScaledLogVariance = flip16(*(UInteger16*)(src + 2));
}

/* brief display clockIdentity*/
display_clockIdentity(ClockIdentity clockIdentity)
{
	printf("	clockIdentity = "
 		" %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	    clockIdentity[0], clockIdentity[1], clockIdentity[2], clockIdentity[3], 
	    clockIdentity[4], clockIdentity[5], clockIdentity[6], clockIdentity[7]);
}

/* brief display portIdentity*/
void
display_portIdentity(PortIdentity *portIdentity)
{
	display_clockIdentity(portIdentity->clockIdentity);
	printf("	portNumber = %hu\n", portIdentity->portNumber);
}

/**\brief Display PTPText Structure*/
void
PTPText_display(int *offset)
{
  	int i;
  	UInteger8 lengthField = *(UInteger8*)(inmessage + *offset);
    printf("    lengthField : %hhu \n", lengthField);
    printf("    textField : ");
	for(i = 0; i < lengthField; i++)
		printf("%c",*(UInteger8*)(inmessage + *offset + 1 + i));
	printf("\n");
		
	*offset = *offset + 1 + lengthField;
 
}

/**\brief Display a Clockquality Structure*/
void
display_clockQuality(ClockQuality *clockQuality)
{
	printf("	clockClass : %hhu \n", clockQuality->clockClass);
	printf("	clockAccuracy : %hhu \n", clockQuality->clockAccuracy);
	printf("	offsetScaledLogVariance : %hu \n", 
		clockQuality->offsetScaledLogVariance);
}

/**\brief Display MAC address*/
void
clockUUID_display(Octet * sourceUuid)
{
	printf("sourceUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	    sourceUuid[0], sourceUuid[1], sourceUuid[2], sourceUuid[3],
	    sourceUuid[4], sourceUuid[5]
	);
}

/**\brief Display Common Header of all messages*/
void
display_commonHeader(MsgHeader *header)
{
	printf("\nMessage header :: \n");
	printf("transportSpecific : %d\n", header->transportSpecific);
	printf("messageType : %d\n", header->messageType);
	printf("versionPTP : %d\n", header->versionPTP);
	printf("messageLength : %d\n", header->messageLength);
	printf("domainNumber : %d\n", header->domainNumber);
	printf("FlagField %02hhx:%02hhx\n", header->flagField0, header->flagField1);
	printf("CorrectionField.msb : \n", header->correctionField.msb);
	printf("CorrectionField.lsb : \n", header->correctionField.lsb);
	printf("SourcePortIdentity : \n");
	display_portIdentity(&header->sourcePortIdentity);
	printf("sequenceId : %d\n", header->sequenceId);
	printf("controlField : %d\n", header->controlField);
	printf("logMessageInterval : %d\n", header->logMessageInterval);
	printf("\n");
}

/**\brief Display Management header*/
void display_managementHeader(MsgManagement * manage)
{
    printf("\nManagement Message :: \n");
    printf("targetPortIdentity : \n");
    display_portIdentity(&manage->targetPortIdentity);
    printf("startingBoundaryHops : %d \n", manage->startingBoundaryHops);
    printf("boundaryHops : %d \n", manage->boundaryHops);
    printf("actionField : %d\n", manage->actionField);
}

/**\brief Display tlv header*/
void display_tlvHeader(ManagementTLV * tlv)
{
	tlv->tlvType = flip16(*(UInteger16 *) (inmessage + 48));
	tlv->lengthField = flip16(*(UInteger16 *) (inmessage + 50));
	tlv->managementId = flip16(*(UInteger16 *) (inmessage + 52));

    printf("\nTLV Header :: \n");
    printf("tlvType : %d\n",tlv->tlvType);
    printf("lengthField : %d\n", tlv->lengthField);
    printf("managementId : %d\n", tlv->managementId);
}

/*Function to unpack the header of received message*/
void 
unpackHeader(Octet *buf, MsgHeader *header)
{	
	header->transportSpecific = (*(Nibble *) (buf + 0)) >> 4;
	header->messageType = (*(Enumeration4 *) (buf + 0)) & 0x0F;
	header->versionPTP = (*(UInteger4 *) (buf + 1)) & 0x0F;
	/* force reserved bit to zero if not */
	header->messageLength = flip16(*(UInteger16 *) (buf + 2));
	header->domainNumber = (*(UInteger8 *) (buf + 4));
	header->flagField0 = (*(Octet *) (buf + 6));
	header->flagField1 = (*(Octet *) (buf + 7));
	memcpy(&header->correctionField.msb, (buf + 8), 4);
	memcpy(&header->correctionField.lsb, (buf + 12), 4);
	header->correctionField.msb = flip32(header->correctionField.msb);
	header->correctionField.lsb = flip32(header->correctionField.lsb);
	unpackPortIdentity(&(header->sourcePortIdentity), buf + 20);
	header->sequenceId = flip16(*(UInteger16 *) (buf + 30));
	header->controlField = (*(UInteger8 *) (buf + 32));
	header->logMessageInterval = (*(Integer8 *) (buf + 33));
	
//	display_commonHeader(header);
}

/*Function to unpack management header*/
void 
unpackManagementHeader(Octet *inmessage, MsgManagement *manage)
{
	unpackPortIdentity(&(manage->targetPortIdentity), inmessage + 34);
	manage->startingBoundaryHops = (*(UInteger8 *)(inmessage + 44));
	manage->boundaryHops = (*(UInteger8 *)(inmessage + 45)); 
	manage->reserved0 = (*(Enumeration4 *)(inmessage + 46)) & 0xF0;
	manage->actionField = (*(Enumeration4 *)(inmessage + 46)) & 0x0F;
	manage->reserved1 = (*(UInteger8 *)(inmessage + 47)); 
	
	manage->tlv->tlvType = flip16(*(UInteger16 *) (inmessage + 48));
	manage->tlv->lengthField = flip16(*(UInteger16 *) (inmessage + 50));
	manage->tlv->managementId = flip16(*(UInteger16 *) (inmessage + 52));
	
//	display_managementHeader(manage);
}

/* Handle and display UserDescription*/
void
handleUserDescription( )
{
	MMUserDescription *data = (MMUserDescription *)(inmessage + 54);
	int offset = 54;
	printf("User Description:\n");
	PTPText_display(&offset);	
}

/* Unpack and display ClockDescription */
void 
handleClockDescription( )
{
	MMClockDescription* data = (MMClockDescription *)(inmessage + 54);
	int offset = 54, len;
	
	printf("Clock Description ManagementTLV message \n");
	
	data->clockType0 = *((UInteger8*)( inmessage + offset));
	offset++;
	data->clockType1 = *((UInteger8*)( inmessage + offset));
	offset++;
	printf("clockType0 : %hhu \n", data->clockType0);
	printf("clockType1 : %hhu \n", data->clockType1);
	
	printf("physicalLayerProtocol : \n");
	PTPText_display(&offset);

	data->physicalAddress.addressLength = flip16(*(UInteger16*)
			(inmessage + offset));
	offset += 2;
	printf("physicalAddressLength : %hu \n", 
		data->physicalAddress.addressLength);
	if(data->physicalAddress.addressField) {
		printf("physicalAddressField : \n");
		clockUUID_display(inmessage + offset);
	}
	offset += data->physicalAddress.addressLength;

	data->protocolAddress.networkProtocol = flip16(
		*(Enumeration16*)(inmessage + offset));
	offset += 2;
	data->protocolAddress.addressLength = flip16(
		*(UInteger16*)(inmessage + offset));
	offset += 2;
	printf("protocolAddressNetworkProtocol : %hu \n", 
		data->protocolAddress.networkProtocol);
	printf("protocolAddressLength : %hu \n", 
		data->protocolAddress.addressLength);
	printf("3 %d\n", offset);
	if(data->protocolAddress.addressField) {
		printf("protocolAddressField : %d.%d.%d.%d \n",
			*(UInteger8*)(inmessage + offset),
			*(UInteger8*)(inmessage + offset + 1),
			*(UInteger8*)(inmessage + offset + 2),
			*(UInteger8*)(inmessage + offset + 3));
	}
	offset += data->protocolAddress.addressLength;

	printf("manufacturerIdentity0 : %d \n", data->manufacturerIdentity0);
	printf("manufacturerIdentity1 : %d \n", data->manufacturerIdentity1);
	printf("manufacturerIdentity2 : %d \n", data->manufacturerIdentity2);
	offset += 4; //One byte for reserved field.
	
	printf("productDescription : \n");
	PTPText_display(&offset);

	printf("revisionData : \n");
	PTPText_display(&offset);

	printf("userDescription : \n");
	PTPText_display(&offset);
	
	printf("profileIdentity0 : %hhu \n", data->profileIdentity0);
	printf("profileIdentity1 : %hhu \n", data->profileIdentity1);
	printf("profileIdentity2 : %hhu \n", data->profileIdentity2);
	printf("profileIdentity3 : %hhu \n", data->profileIdentity3);
	printf("profileIdentity4 : %hhu \n", data->profileIdentity4);
	printf("profileIdentity5 : %hhu \n", data->profileIdentity5);
}

/* unpack and display default data set */
void
handleDefaultDataSet( )
{
	MMDefaultDataSet* data = (MMDefaultDataSet *)(inmessage + 54);

	data->so_tsc = *((UInteger8*)( inmessage + 54));
	data->numberPorts = flip16(*(UInteger16*)(inmessage + 56));
	data->priority1 = *((UInteger8*) (inmessage + 58));
	data->domainNumber = *((UInteger8*) (inmessage + 72));
	unpackClockIdentity(data->clockIdentity, inmessage + 64);
	data->priority2 = *((UInteger8*) (inmessage + 63));
	unpackClockQuality(&(data->clockQuality), inmessage + 59);

	
	printf("defaultDS.twoStepFlag : %hhu \n", data->so_tsc & 0x01);
	printf("defaultDS.slaveOnly : %hhu \n", (data->so_tsc >> 1));
	printf("defaultDS.numberPorts : %hu \n", data->numberPorts);
	printf("defaultDS.priority1 : %hhu \n", data->priority1);
	printf("defaultDS.priority2 : %hhu \n", data->priority2);
	printf("defaultDS.clockQuality:\n");
	display_clockQuality(&(data->clockQuality));
	display_clockIdentity(data->clockIdentity);
	printf("defaultDS.domainNumber : %hhu \n", data->domainNumber);

}

/*Unpack and display ParentDataSet */
void
handleParentDataSet( )
{
	MMParentDataSet* data = (MMParentDataSet *)(inmessage + 54);
	unpackPortIdentity(&(data->parentPortIdentity),inmessage + 54);

	unpackClockIdentity(data->grandmasterIdentity, inmessage + 78);
	data->grandmasterPriority1 = *((UInteger8*)(inmessage + 72));
	unpackClockQuality(&(data->grandmasterClockQuality), inmessage + 73);
	data->grandmasterPriority2 = *((UInteger8*)(inmessage + 77));
	data->observedParentOffsetScaledLogVariance = flip16(
		*(UInteger16*)(inmessage + 66));
	data->observedParentClockPhaseChangeRate = flip32(
		*(Integer32*)(inmessage + 68));


	printf("parentDS.parentPortIdentity:\n");
	display_portIdentity(&(data->parentPortIdentity));
	printf("parentDS.parentStats : %hhu \n", data->PS);
	printf("parentDS.observedParentOffsetScaledLogVariance : %hu \n", 
		data->observedParentOffsetScaledLogVariance);
	printf("parentDS.observedParentClockPhaseChangeRate : %d \n",
		data->observedParentClockPhaseChangeRate);
	printf("parentDS.grandmasterClockQuality:\n");
	display_clockQuality(&(data->grandmasterClockQuality));
	display_clockIdentity(data->grandmasterIdentity);
	printf("parentDS.grandmasterPriority1 : %hhu \n", 
		data->grandmasterPriority1);
	printf("parentDS.grandmasterPriority2 : %hhu \n",
		data->grandmasterPriority2);

}

/* Unpack and Display Priority1 */
void
handlePriority1( )
{
	MMPriority1* data = (MMPriority1 *)(inmessage + 54);
	printf("Priority1 = %hhu\n",data->priority1);
}

/* Unpack and display Priority2*/
void
handlePriority2( )
{
	MMPriority2* data = (MMPriority2 *)(inmessage + 54);
	printf("Priority2 = %hhu\n",data->priority2);
}

/* Unpack and display Domain*/
void
handleDomain( )
{
	MMDomain* data = (MMDomain*)(inmessage + 54);
	printf("Domain = %hhu\n",data->domainNumber);
}

/* Unpack and display SlaveOnly*/
void
handleSlaveOnly( )
{
	MMSlaveOnly* data = (MMSlaveOnly*)(inmessage + 54);
	if ((data->so && 01) == 0)
		printf("SlaveOnly = False\n");
	else
		printf("SlaveOnly = True\n");
}

/* Unpack and display LogAnnounceInterval */
void
handleLogAnnounceInterval( )
{
	MMLogAnnounceInterval* data = (MMLogAnnounceInterval*)(inmessage + 54);
	printf("LogAnnounceInterval = %hhd\n",data->logAnnounceInterval);
}

/* Unpack and display AnnounceReceiptTimeout */
void
handleAnnounceReceiptTimeout( )
{
	MMAnnounceReceiptTimeout* data = (MMAnnounceReceiptTimeout*)(inmessage + 54);
	printf("AnnounceReceiptTimeout = %hhu\n",data->announceReceiptTimeout);
}

/* Unpack and display LogSyncInterval */
void
handleLogSyncInterval( )
{
	MMLogSyncInterval* data = (MMLogSyncInterval*)(inmessage + 54);
	printf("LogSyncInterval = %hhd\n",data->logSyncInterval);
}

/* Unpack and display VersionNumber */
void
handleVersionNumber( )
{
	MMVersionNumber* data = (MMVersionNumber*)(inmessage + 54);
	printf("VersionNumber = %hhu\n",data->versionNumber);
}

/* Unpack and display ClockAccuracy */
void
handleClockAccuracy( )
{
	MMClockAccuracy* data = (MMClockAccuracy*)(inmessage + 54);
	printf("ClockAccuracy = %hhu\n",data->clockAccuracy);
	switch(data->clockAccuracy){
	case 32:
		printf("The time is accurate to within 25 ns\n");
		break;
	case 33:
		printf("The time is accurate to within 100 ns\n");
		break;
	case 34:
		printf("The time is accurate to within 250 ns\n");
		break;
	case 35:
		printf("The time is accurate to within 1 microsec\n");
		break;
	case 36:
		printf("The time is accurate to within 2.5 microsec\n");
		break;
	case 37:
		printf("The time is accurate to within 10 microsec\n");
		break;
	case 38:
		printf("The time is accurate to within 25 microsec\n");
		break;
	case 39:
		printf("The time is accurate to within 100 microsec\n");
		break;
	case 40:
		printf("The time is accurate to within 250 microsec\n");
		break;
	case 41:
		printf("The time is accurate to within 1 millisec\n");
		break;
	case 42:
		printf("The time is accurate to within 2.5 millisec\n");
		break;
	case 43:
		printf("The time is accurate to within 10 millisec\n");
		break;
	case 44:
		printf("The time is accurate to within 25 millisec\n");
		break;
	case 45:
		printf("The time is accurate to within 100 millisec\n");
		break;
	case 46:
		printf("The time is accurate to within 250 millisec\n");
		break;
	case 47:
		printf("The time is accurate to within 1 sec\n");
		break;
	case 48:
		printf("The time is accurate to within 10 sec\n");
		break;
	case 49:
		printf("The time is accurate to > 10 sec\n");
		break;
	default:
		printf("Unknown accuracy\n");
	}
}

/* Unpack and displayUtcProperties*/
void
handleUtcProperties( )
{
	MMUtcProperties* data = (MMUtcProperties*)(inmessage + 54);
	printf("currentUtcOffset = %hd\n",data->currentUtcOffset);
	if ((data->utcv_li59_li61 & 0x04) == 0)
		printf("UTCV = False, ");
	else
		printf("UTCV = True, ");
		
	if ((data->utcv_li59_li61 & 0x02) == 0)
		printf("L1-59 = False, ");
	else
		printf("L1-59 = True, ");
		
	if ((data->utcv_li59_li61 & 0x01) == 0)
		printf("L1-61 = False\n");
	else
		printf("L1-61 = True\n");
}

/* Unpack and display TraceabilityProperties*/
void
handleTraceabilityProperties( )
{
	MMTraceabilityProperties* data = (MMTraceabilityProperties*)(inmessage + 54);
	if ((data->ftra_ttra & 0x20) == 0)
		printf("FTRA = False, ");
	else
		printf("FTRA = True, ");
		
	if ((data->ftra_ttra & 0x10) == 0)
		printf("TTRA = False\n");
	else
		printf("TTRA = True\n");
		
}

/* Unpack and display DelayMechanism*/
void
handleDelayMechanism( )
{
	MMDelayMechanism* data = (MMDelayMechanism*)(inmessage + 54);
	printf("DelayMechanism = %hhu\n",data->delayMechanism);
}

/* Handle and display LogMinRequirements */
void
handleLogMinRequirement( )
{
	MMLogMinPdelayReqInterval* data = (MMLogMinPdelayReqInterval*)(inmessage + 54);
	printf("LogMinPdelayReqInterval = %hhu\n",data->logMinPdelayReqInterval);
}

/* Unpack and display CurrentDataSet */
void
handleCurrentDataSet( )
{
	MMCurrentDataSet* data = (MMCurrentDataSet*)(inmessage + 54);
	printf("currentDS.stepsRemoved = %hu\n",data->stepsRemoved);
	printf("currentDS.offsetFromMaster.scaledNanoseconds.lsb = %u\n",
		data->offsetFromMaster.scaledNanoseconds.lsb);
	printf("currentDS.offsetFromMaster.scaledNanoseconds.msb = %u\n",
		data->offsetFromMaster.scaledNanoseconds.msb);
	printf("currentDS.meanPathDelay.scaledNanoseconds.lsb = %u\n",
		data->meanPathDelay.scaledNanoseconds.lsb);
	printf("currentDS.meanPathDelay.scaledNanoseconds.msb = %d\n",
		data->meanPathDelay.scaledNanoseconds.lsb);
	
}

/* Unpack and display TimePropertiesDataSet */
void
handleTimePropertiesDataSet( )
{
	MMTimePropertiesDataSet* data = (MMTimePropertiesDataSet*)(inmessage + 54);
	printf("LogMinPdelayReqInterval = %hu\n",data->currentUtcOffset);
	if ((data->ftra_ttra_ptp_utcv_li59_li61 & 0x20) == 0)
		printf("FTRA = False, ");
	else
		printf("FTRA = True, ");

	if ((data->ftra_ttra_ptp_utcv_li59_li61 & 0x10) == 0)
		printf("TTRA = False, ");
	else
		printf("TTRA = True, ");

	if ((data->ftra_ttra_ptp_utcv_li59_li61 & 0x04) == 0)
		printf("UTCV = False, ");
	else
		printf("UTCV = True, ");
		
	if ((data->ftra_ttra_ptp_utcv_li59_li61 & 0x02) == 0)
		printf("L1-59 = False, ");
	else
		printf("L1-59 = True, ");
		
	if ((data->ftra_ttra_ptp_utcv_li59_li61 & 0x01) == 0)
		printf("L1-61 = False\n");
	else
		printf("L1-61 = True\n");
		
	printf("timePropertiesDS.timeSource = %hhu\n",data->timeSource);
	
}

/* Unpack and display PortDataSet */
void
handlePortDataSet()
{
	MMPortDataSet* data = (MMPortDataSet*)(inmessage + 54);
	unpackPortIdentity(&(data->portIdentity),inmessage + 54);
	printf("portDS.portIdentity:\n");
	display_portIdentity(&(data->portIdentity));
	printf("portDS.portState = %hhu\n", data->portState);
	printf("portDS.logMinDelayReqInterval = %hhd\n", 
		data->logMinDelayReqInterval);
	printf("portDS.peerMeanPathDelay.scaledNanoseconds.lsb = %u\n", 
		data->peerMeanPathDelay.scaledNanoseconds.lsb);
	printf("portDS.peerMeanPathDelay.scaledNanoseconds.msb = %d\n", 
		data->peerMeanPathDelay.scaledNanoseconds.msb);
	printf("portDS.logAnnounceInterval = %hhd\n", data->logAnnounceInterval);
	printf("portDS.announceReceiptTimeout = %hhu\n", data->announceReceiptTimeout);
	printf("portDS.logSyncInterval = %hhu\n", data->logSyncInterval);
	printf("portDS.delayMechanism = %hhu\n", data->delayMechanism);
	printf("portDS.logMinPdelayReqInterval = %hhu\n", data->delayMechanism);
	printf("portDS.versionNumber = %hhu\n", data->versionNumber);
	
}

/* Function to handle management response and display the required fields.
 * This function is called only when a message is received with
 * 'Response' actionfield. Appropriate handling function is called on the 
 * basis of managementId. 
 */
void 
handleManagementResponse(MsgManagement *manage)
{
	printf("Received a RESPONSE management message.\n");
	
	switch(manage->tlv->managementId){
    case MM_NULL_MANAGEMENT:
    	//nothing to handle
    	break;

    case MM_USER_DESCRIPTION:
    	handleUserDescription();
    	break;
    case MM_PRIORITY1:
       	handlePriority1();
    	break;
    case MM_PRIORITY2:
       	handlePriority2();
    	break;
    case MM_DOMAIN:
       	handleDomain();
       	break;
    case MM_SLAVE_ONLY:
       	handleSlaveOnly();
       	break;
    case MM_LOG_ANNOUNCE_INTERVAL:
       	handleLogAnnounceInterval();
       	break;
    case MM_ANNOUNCE_RECEIPT_TIMEOUT:
       	handleAnnounceReceiptTimeout();
       	break;
    case MM_LOG_SYNC_INTERVAL:
       	handleLogSyncInterval();
       	break;
    case MM_VERSION_NUMBER:
       	handleVersionNumber();
       	break;
    case MM_CLOCK_ACCURACY:
       	handleClockAccuracy();
       	break;
    case MM_UTC_PROPERTIES:
       	handleUtcProperties();
       	break;
    case MM_TRACEABILITY_PROPERTIES:
       	handleTraceabilityProperties();
       	break;
    case MM_DELAY_MECHANISM:
       	handleDelayMechanism();
       	break;
    case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
    	handleLogMinRequirement();
	    break;
    case MM_CURRENT_DATA_SET:
    	handleCurrentDataSet();
    	break;
    case MM_TIME_PROPERTIES_DATA_SET:
    	handleTimePropertiesDataSet();
    	break;
    case MM_PORT_DATA_SET:
    	handlePortDataSet();
    	break;
    case MM_PARENT_DATA_SET:
    	handleParentDataSet();
    	break;
    case MM_DEFAULT_DATA_SET:
    	handleDefaultDataSet();
    	break;
    case MM_CLOCK_DESCRIPTION:
    	handleClockDescription();
		break;
		
	case MM_SAVE_IN_NON_VOLATILE_STORAGE:
    case MM_RESET_NON_VOLATILE_STORAGE:
    case MM_INITIALIZE:
    case MM_ENABLE_PORT:
    case MM_DISABLE_PORT:
    	printf("But Ack was expected\n");
    	break;

	case MM_TIME:    	
    case MM_FAULT_LOG:
    case MM_FAULT_LOG_RESET:
    case MM_TIMESCALE_PROPERTIES:
    case MM_UNICAST_NEGOTIATION_ENABLE:
    case MM_PATH_TRACE_LIST:
    case MM_PATH_TRACE_ENABLE:
    case MM_GRANDMASTER_CLUSTER_TABLE:
    case MM_UNICAST_MASTER_TABLE:
    case MM_UNICAST_MASTER_MAX_TABLE_SIZE:
    case MM_ACCEPTABLE_MASTER_TABLE:
    case MM_ACCEPTABLE_MASTER_TABLE_ENABLED:
    case MM_ACCEPTABLE_MASTER_MAX_TABLE_SIZE:
    case MM_ALTERNATE_MASTER:
    case MM_ALTERNATE_TIME_OFFSET_ENABLE:
    case MM_ALTERNATE_TIME_OFFSET_NAME:
    case MM_ALTERNATE_TIME_OFFSET_MAX_KEY:
    case MM_ALTERNATE_TIME_OFFSET_PROPERTIES:
    case MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
    case MM_TRANSPARENT_CLOCK_PORT_DATA_SET:
    case MM_PRIMARY_DOMAIN:
        printf("Currently unhandled managementTLV Response %d\n", 
        		manage->tlv->managementId);
		return;
    default:
        printf("Unknown managementTLV in response %d\n", 
        		manage->tlv->managementId);
        return;
    }
}

/* Function to handle management ack, received when a command gets executed
 * at server
 */
void 
handleManagementAck(MsgManagement *manage)
{
	if (outmessage != NULL)
		if (*(UInteger16 *) (outmessage + 52) == *(UInteger16 *) (inmessage + 52))
			printf("Received ACKNOWLEDGEMENT. Command executed.\n");
		else
			printf("Received ACKNOWLEDGEMENT. Not for your command.\n");
}

/*Function to handle management error message, received when a sent message is 
 * not supported at server or some error happens
 */
void 
handleManagementError(MsgManagement *manage)
{
	printf("Received TLV_MANAGEMENT_ERROR_STATUS.\n");
	if (outmessage != NULL && *(UInteger16 *) (outmessage + 52) == 
			*(UInteger16 *) (inmessage + 54))
		switch(manage->tlv->managementId){
		case RESPONSE_TOO_BIG:
			printf("Response Too Big\n");
			break;
		case NO_SUCH_ID:
			printf("The managementId is not recognized.\n");
			break;
		case WRONG_LENGTH:
			printf("The managementId was identified but the length of the data "
			 		"was wrong.\n");
			break;
		case WRONG_VALUE:
			printf("The managementId and length were correct but one or" 
				"more values were wrong.\n");
			break;
		case NOT_SETABLE:
			printf("Some of the variables in the set command were not updated "
			"because they are not configurable.\n");
			break;
		case NOT_SUPPORTED:
			printf("The requested operation is not supported in this node.\n");
			break;
		case GENERAL_ERROR:
			printf("A general error occured.\n");
			break;
		default:
			printf("Unknown Error code\n");
		}
}

/* Function which handles all received messages, checks the tlvtype and actionField
 * and then passes it to appropriate method for unpacking and displaying
 */
void handleIncomingMsg()
{
	MsgHeader h; MsgManagement manage;
	manage.tlv = (ManagementTLV *)malloc(sizeof(ManagementTLV));
	if (manage.tlv == NULL){
		printf("malloc failed\n");
		exit(1);
	}
	unpackHeader(inmessage, &h);
	if (h.messageLength < MANAGEMENT_LENGTH){
		printf("Truncated message\n");
		return;
	}
	
	if (h.messageType == MANAGEMENT){
		receivedFlag = TRUE;
		unpackManagementHeader(inmessage, &manage);
		if (manage.tlv->tlvType == TLV_MANAGEMENT)
		{
			if (manage.actionField == RESPONSE)
				handleManagementResponse(&manage);
			else if (manage.actionField == ACKNOWLEDGE)
				handleManagementAck(&manage);
		} 
		else if (manage.tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS)
			handleManagementError(&manage);
		else 
			printf("Received management message with unknown tlyType\n");
	} 
	else {
		printf("Received message is not a MANAGEMENT message\n");
		return;
	}
	
	free(manage.tlv);
}
