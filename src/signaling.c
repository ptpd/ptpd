/*-
 * Copyright (c) 2015 	   Wojciech Owczarek
 * Copyright (c) 2014 	   Perseus Telecom
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
 * @file   signaling.c
 * @date   Mon Jan 14 00:40:12 GMT 2014
 *
 * @brief  Routines to handle incoming signaling messages
 *
 *
 */

#include "ptpd.h"

/* how many times we send a cancel before we stop waiting for ack */
#define GRANT_CANCEL_ACK_TIMEOUT 3
/* every N grant refreshes, we re-request messages we don't seem to be receiving */
#define GRANT_KEEPALIVE_INTERVAL 5
/* maximum number of missed messages of given type before we re-request */
#define GRANT_MAX_MISSED 10

static void updateUnicastIndex(UnicastGrantTable *table, UnicastGrantTable **index);
static UnicastGrantTable* lookupUnicastIndex(const PortIdentity *portIdentity, Integer32 transportAddress, UnicastGrantTable **index);

/* update index table */
static void
updateUnicastIndex(UnicastGrantTable *table, UnicastGrantTable **index)
{
    uint32_t hash = fnvHash(&table->portIdentity, sizeof(PortIdentity), UNICAST_MAX_DESTINATIONS);

    /* peer table normally has one entry: if we got here, we might pollute the main index */
    if(!table->isPeer && index != NULL) {
	index[hash] = table;
    }

}

/* return matching entry from index table */
static UnicastGrantTable*
lookupUnicastIndex(const PortIdentity *portIdentity, Integer32 transportAddress, UnicastGrantTable **index)
{
    uint32_t hash = fnvHash((void*)portIdentity, sizeof(PortIdentity), UNICAST_MAX_DESTINATIONS);

    UnicastGrantTable* table;

    if(index == NULL) {
	return NULL;
    }

    table  = index[hash];

    if(table != NULL && (!cmpPortIdentity(portIdentity, &table->portIdentity))) {
	DBG("lookupUnicastIndex: cache hit\n");
	return table;
    } else {
	/* hash collision */
	DBG("lookupUnicastIndex: hash collision\n");
	return NULL;
    }

}


/* find which grant table entry the given port belongs to:
   - if not found, return first free entry, store portID and/or address
   - if found, find the entry it belongs to
   - look up the index table, only iterate on miss
   - if update is FALSE, only a search is performed
*/
UnicastGrantTable*
findUnicastGrants
(const PortIdentity* portIdentity, Integer32 transportAddress, UnicastGrantTable *grantTable, UnicastGrantTable **index, int nodeCount, Boolean update)
{

	int i;

	UnicastGrantTable *found = NULL;
	UnicastGrantTable *firstFree = NULL;

	UnicastGrantTable *nodeTable;

	/* look up the index table*/
	if(index != NULL) {
	    found = lookupUnicastIndex(portIdentity, transportAddress, index);
	}
	
	if(found != NULL) {
	    DBG("findUnicastGrants: cache hit\n");
		/* do not overwrite address if zero given
		 * (used by slave to preserve configured master addresses)
		 */
		if(update && transportAddress) {
		    found->transportAddress = transportAddress;
		}
		if(update) {
		    found->portIdentity = *portIdentity;
		}

		if(update) {
		    updateUnicastIndex(found, index);
		}

	} else for(i=0; i < nodeCount; i++) {

	    nodeTable = &grantTable[i];

	    /* first free entry */
	    if(firstFree == NULL) {
		    if(portIdentityEmpty(&nodeTable->portIdentity) ||
			(nodeTable->timeLeft == 0)) {
			    firstFree = nodeTable;
		    }
	    }

	    /* port identity matches */
	    if(!cmpPortIdentity(portIdentity, &nodeTable->portIdentity)) {

		found = nodeTable;

		/* do not overwrite address if zero given 
		 * (used by slave to preserve configured master addresses)
		 */

		if(update && transportAddress) {
		    found->transportAddress = transportAddress;
		}

		if(update) {
		    found->portIdentity = *portIdentity;
		}

		DBG("findUnicastGrants: cache miss - %d iterations %d\n", i);

		if(update) {
		    updateUnicastIndex(found, index);
		}

		break;

	    }

	    /* no port identity match but we have a transport address match */
	    if(nodeTable->transportAddress && 
		(nodeTable->transportAddress==transportAddress)) {

		found = nodeTable;
		if(update) {
			found->portIdentity = *portIdentity;
		}
		break;
	    }

	}

    if(found != NULL) {
	return found;
    }

    /* will return NULL if there are no free slots, otherwise the first free slot */
    if(update && firstFree != NULL) {
	firstFree->portIdentity = *portIdentity;
	firstFree->transportAddress = transportAddress;

	/* new set of grants - reset sequence numbers */
	for(i=0; i < PTP_MAX_MESSAGE; i++) {
	    firstFree->grantData[i].sentSeqId = 0;
	}

	/* - You know, we could as well have sex now.
         * - Yes, dear, but, let's not.
	 */
	/* memset(firstFree->grantData,0,PTP_MAX_MESSAGE * sizeof(UnicastGrantData)); */
    }

    return firstFree;

}


/**\brief Initialise outgoing signaling message fields*/
void initOutgoingMsgSignaling(PortIdentity* targetPortIdentity, MsgSignaling* outgoing, PtpClock *ptpClock)
{
	/* set header fields */
	outgoing->header.transportSpecific = ptpClock->transportSpecific;
	outgoing->header.messageType = SIGNALING;
        outgoing->header.versionPTP = ptpClock->versionNumber;
	outgoing->header.domainNumber = ptpClock->domainNumber;
        /* set header flagField to zero for management messages, Spec 13.3.2.6 */
        outgoing->header.flagField0 = 0x00;
        outgoing->header.flagField1 = 0x00;
        outgoing->header.correctionField.msb = 0;
        outgoing->header.correctionField.lsb = 0;
	copyPortIdentity(&outgoing->header.sourcePortIdentity, &ptpClock->portIdentity);

	outgoing->header.sequenceId = ptpClock->sentSignalingSequenceId++;

	outgoing->header.controlField = 0x5; /* deprecrated for ptp version 2 */
	outgoing->header.logMessageInterval = 0x7F;

	/* set management message fields */
	copyPortIdentity( &outgoing->targetPortIdentity, targetPortIdentity);

	/* init managementTLV */
	XMALLOC(outgoing->tlv, sizeof(SignalingTLV));
	outgoing->tlv->valueField = NULL;
	outgoing->tlv->lengthField = 0;
}



/**\brief Handle incoming REQUEST_UNICAST_TRANSMISSION signaling TLV type*/
void handleSMRequestUnicastTransmission(MsgSignaling* incoming, MsgSignaling* outgoing, Integer32 sourceAddress, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

	char portId[PATH_MAX];
	UnicastGrantData *myGrant;
	UnicastGrantTable *nodeTable;
	Boolean granted = TRUE;
	SMRequestUnicastTransmission* requestData = (SMRequestUnicastTransmission*)incoming->tlv->valueField;
	SMGrantUnicastTransmission* grantData = NULL;
	Enumeration8 messageType = requestData->messageType;
#ifdef RUNTIME_DEBUG
	struct in_addr tmpAddr;
	tmpAddr.s_addr = sourceAddress;
#endif /* RUNTIME_DEBUG */
	snprint_PortIdentity(portId, PATH_MAX, &incoming->header.sourcePortIdentity);

	initOutgoingMsgSignaling(&incoming->header.sourcePortIdentity, outgoing, ptpClock);
	XMALLOC(outgoing->tlv->valueField, sizeof(SMGrantUnicastTransmission));
	grantData = (SMGrantUnicastTransmission*)outgoing->tlv->valueField;

        outgoing->header.flagField0 |= PTP_UNICAST;
	outgoing->tlv->tlvType = TLV_GRANT_UNICAST_TRANSMISSION;
	outgoing->tlv->lengthField = 8;


	DBG("Received REQUEST_UNICAST_TRANSMISSION message for message %s from %s(%s) - duration %d interval %d\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr), requestData->durationField,
			requestData->logInterMessagePeriod);

	nodeTable = findUnicastGrants(&incoming->header.sourcePortIdentity, sourceAddress, ptpClock->unicastGrants, ptpClock->unicastGrantIndex, UNICAST_MAX_DESTINATIONS, TRUE);

	if(nodeTable == NULL) {
		if(ptpClock->slaveCount >= UNICAST_MAX_DESTINATIONS) {
			DBG("REQUEST_UNICAST_TRANSMISSION (%s): did not find node in slave table : %s (%s) - table full\n", getMessageTypeName(messageType),
			inet_ntoa(tmpAddr),portId);
		} else {
		DBG("REQUEST_UNICAST_TRANSMISSION (%s): did not find node in slave table: %s (%s)\n", getMessageTypeName(messageType),
			inet_ntoa(tmpAddr),portId);
		}
		/* fill the response with basic fields (namely messageType so the deny is valid */
		goto finaliseResponse;
	}

	myGrant = &nodeTable->grantData[messageType];


	myGrant->requested = TRUE;

	/* We assume the request is denied */
	grantData->renewal_invited = 0;
	grantData->durationField = 0;

	ptpClock->counters.unicastGrantsRequested++;

	if(!myGrant->requestable) {
		DBG("denied unicast transmission request for non-requestable message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));
		myGrant->granted = FALSE;
	}

	if(!requestData->durationField) {
		DBG("denied unicast transmission request for zero duration - message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));
		granted = FALSE;
	}

	if(requestData->logInterMessagePeriod < myGrant->logMinInterval) {
		DBG("denied unicast transmission request for too short interval - message %s from %s(%s), interval %d\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr), requestData->logInterMessagePeriod);
		granted = FALSE;
	}


	if (granted) {
	    
	    /* do not deny if requested longer interval than supported - just offer the longest */
	    if(requestData->logInterMessagePeriod > myGrant->logMaxInterval) {
		    grantData->logInterMessagePeriod = myGrant->logMaxInterval;
	    } else {
		grantData->logInterMessagePeriod = requestData->logInterMessagePeriod;
	    }

	    /* only offer up to the maximum duration configured, but preserve a 30 second minimum */
	    if(requestData->durationField > rtOpts->unicastGrantDuration) {
		grantData->durationField = rtOpts->unicastGrantDuration;
	    } else if(requestData->durationField <= 30) {
		grantData->durationField = 30;
	    } else {
		grantData->durationField = requestData->durationField;
	    }

	    DBG("granted unicast transmission request - message %s to %s(%s), interval %d, duration %d s\n",
		getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr), grantData->logInterMessagePeriod,
		grantData->durationField);

	    myGrant->duration = grantData->durationField;
	    /* NEW! 5 seconds for free! Why 5? refreshUnicastGrants expires the grant when it's 5 */
	    myGrant->timeLeft = grantData->durationField + 10;

	    /* do not reset the counter if this is being re-requested */
	    if(!myGrant->granted || (myGrant->logInterval != grantData->logInterMessagePeriod)) {
		myGrant->intervalCounter = 0;
	    }

	    ptpClock->counters.unicastGrantsGranted++;

	    myGrant->granted = TRUE;
	    myGrant->canceled = FALSE;
	    myGrant->cancelCount = 0;
	    myGrant->logInterval = grantData->logInterMessagePeriod;

	    /* this could be the very first grant for this node - update node's timeLeft so it's not seen as free anymore */
	    if(nodeTable->timeLeft <= 0) {
		/* + 10 seconds for a grace period */
		nodeTable->timeLeft = myGrant->timeLeft + 10;
	    }

	    /* If we've granted once, we're likely to grant again */
	    grantData->renewal_invited = 1;

	    outgoing->header.sequenceId = myGrant->parent->grantData[SIGNALING].sentSeqId;
	    myGrant->parent->grantData[SIGNALING].sentSeqId++;

	} else {
		ptpClock->counters.unicastGrantsDenied++;
	}

	/* Testing only */
	/* grantData->logInterMessagePeriod = requestData->logInterMessagePeriod; */
	/* grantData->durationField = requestData->durationField; */

finaliseResponse:
	grantData->messageType = requestData->messageType;
	grantData->reserved0 = 0;
	grantData->reserved1 = 0;
}

/**\brief Handle incoming GRANT_UNICAST_TRANSMISSION signaling message type*/
void handleSMGrantUnicastTransmission(MsgSignaling* incoming, Integer32 sourceAddress, UnicastGrantTable *grantTable, int nodeCount, PtpClock *ptpClock)
{

	char portId[PATH_MAX];
	SMGrantUnicastTransmission *incomingGrant = (SMGrantUnicastTransmission*)incoming->tlv->valueField;
	UnicastGrantData *myGrant;
	Enumeration8 messageType = incomingGrant->messageType;
	UnicastGrantTable *nodeTable;
#ifdef RUNTIME_DEBUG
	struct in_addr tmpAddr;
	tmpAddr.s_addr = sourceAddress;
#endif /* RUNTIME_DEBUG */

	snprint_PortIdentity(portId, PATH_MAX, &incoming->header.sourcePortIdentity);

	DBGV("Received GRANT_UNICAST_TRANSMISSION message for message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));

	nodeTable = findUnicastGrants(&incoming->header.sourcePortIdentity, sourceAddress, grantTable, ptpClock->unicastGrantIndex, nodeCount, TRUE);

	if(nodeTable == NULL) {
		DBG("GRANT_UNICAST_TRANSMISSION: did not find node in master table: %s\n", portId);
		return;
	}

	myGrant = &nodeTable->grantData[messageType];

	if(!myGrant->requestable) {
		DBG("received unicat grant for non-requestable message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));
		return;
	}

	if(!myGrant->requested) {
		DBG("received unicast grant for not requested message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));
		return;
	}

	if(incomingGrant->durationField == 0) {
		DBG("unicast transmission request for message %s interval %d duration %d s denied by %s(%s)\n",
			getMessageTypeName(messageType), myGrant->logInterval, myGrant->duration,
			portId, inet_ntoa(tmpAddr));

		ptpClock->counters.unicastGrantsDenied++;


		/* grant was denied so let's try a higher interval next time */
    		myGrant->logInterval++;
		
		/* if we're above max, cycle through back to minimum */
		if(myGrant->logInterval > myGrant->logMaxInterval) {
			myGrant->logInterval = myGrant->logMinInterval;
		}

		/* this is so that we request again */
		myGrant->requested = FALSE;

		return;
	}

	DBG("received unicast transmission grant for message %s, interval %d, duration %d s\n",
			getMessageTypeName(messageType), incomingGrant->logInterMessagePeriod,
			incomingGrant->durationField);

	ptpClock->counters.unicastGrantsGranted++;

	myGrant->granted = TRUE;
	myGrant->logInterval = incomingGrant->logInterMessagePeriod;
	myGrant->duration = incomingGrant->durationField;
	myGrant->timeLeft = myGrant->duration;
	myGrant->canceled = FALSE;
	myGrant->cancelCount = 0;

}

/**\brief Handle incoming CANCEL_UNICAST_TRANSMISSION signaling message type*/
Boolean handleSMCancelUnicastTransmission(MsgSignaling* incoming, MsgSignaling* outgoing, Integer32 sourceAddress, PtpClock* ptpClock)
{
	DBGV("Received CANCEL_UNICAST_TRANSMISSION message\n");

	char portId[PATH_MAX];
	SMCancelUnicastTransmission* requestData = (SMCancelUnicastTransmission*)incoming->tlv->valueField;
	SMAcknowledgeCancelUnicastTransmission* acknowledgeData = NULL;

	UnicastGrantData *myGrant;
	Enumeration8 messageType = requestData->messageType;
	UnicastGrantTable *nodeTable;
#ifdef RUNTIME_DEBUG
	struct in_addr tmpAddr;
	tmpAddr.s_addr = sourceAddress;
#endif /* RUNTIME_DEBUG */

	initOutgoingMsgSignaling(&incoming->header.sourcePortIdentity, outgoing, ptpClock);
        outgoing->header.flagField0 |= PTP_UNICAST;
	outgoing->tlv->tlvType = TLV_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION;
	outgoing->tlv->lengthField = 2;

	XMALLOC(outgoing->tlv->valueField, sizeof(SMAcknowledgeCancelUnicastTransmission));
	acknowledgeData = (SMAcknowledgeCancelUnicastTransmission*)outgoing->tlv->valueField;
	snprint_PortIdentity(portId, PATH_MAX, &incoming->header.sourcePortIdentity);

	DBGV("Received CANCEL_UNICAST_TRANSMISSION message for message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));

	ptpClock->counters.unicastGrantsCancelReceived++;

	nodeTable = findUnicastGrants(&incoming->header.sourcePortIdentity, sourceAddress, ptpClock->unicastGrants, ptpClock->unicastGrantIndex, UNICAST_MAX_DESTINATIONS, FALSE);

	if(nodeTable == NULL) {
		DBG("CANCEL_UNICAST_TRANSMISSION: did not find node in slave table: %s\n", portId);
		return FALSE;
	}

	myGrant = &nodeTable->grantData[messageType];

	if(!myGrant->requestable) {
		DBG("cancel grant attempt for non-requestable message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));
		return FALSE;
	}

	if(!myGrant->requested) {
		DBG("cancel grant attempt for not requested message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));
		return FALSE;
	}

	if(!myGrant->granted) {
		DBG("cancel grant attempt for not granted message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));
		return FALSE;
	}

	myGrant->granted = FALSE;
	myGrant->requested = FALSE;

	outgoing->header.sequenceId = myGrant->parent->grantData[SIGNALING].sentSeqId;
	myGrant->parent->grantData[SIGNALING].sentSeqId++;

	myGrant->sentSeqId = 0;
	myGrant->cancelCount = 0;
	myGrant->timeLeft = 0;
	myGrant->duration = 0;

	DBG("Accepted CANCEL_UNICAST_TRANSMISSION message for message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));

	acknowledgeData->messageType = requestData->messageType;
	acknowledgeData->reserved0 = 0;
	acknowledgeData->reserved1 = 0;

	return TRUE;
}

/**\brief Handle incoming ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION signaling message type*/
void handleSMAcknowledgeCancelUnicastTransmission(MsgSignaling* incoming, Integer32 sourceAddress, PtpClock* ptpClock)
{

	char portId[PATH_MAX];
	SMAcknowledgeCancelUnicastTransmission* requestData = (SMAcknowledgeCancelUnicastTransmission*)incoming->tlv->valueField;

	UnicastGrantData *myGrant;
	Enumeration8 messageType = requestData->messageType;
	UnicastGrantTable *nodeTable;

#ifdef RUNTIME_DEBUG
	struct in_addr tmpAddr;
	tmpAddr.s_addr = sourceAddress;
#endif /* RUNTIME_DEBUG */

	snprint_PortIdentity(portId, PATH_MAX, &incoming->header.sourcePortIdentity);

	DBGV("Received ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION message for message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));

	nodeTable = findUnicastGrants(&incoming->header.sourcePortIdentity, sourceAddress, ptpClock->unicastGrants, ptpClock->unicastGrantIndex, UNICAST_MAX_DESTINATIONS, FALSE);

	if(nodeTable == NULL) {
		DBG("ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION: did not find node in slave table: %s\n", portId);
	}

	myGrant = &nodeTable->grantData[messageType];

	if(!myGrant->requestable) {
		DBG("acknowledge cancel grant attempt for non-requestable message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));
	}

	if(!myGrant->canceled) {
		DBG("acknowledge cancel grant received for not canceled message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));
	}

	myGrant->granted = FALSE;
	myGrant->requested = FALSE;
	myGrant->sentSeqId = 0;
	myGrant->cancelCount = 0;
	myGrant->timeLeft = 0;
	myGrant->duration = 0;
	myGrant->canceled = FALSE;
	myGrant->cancelCount = 0;

	ptpClock->counters.unicastGrantsCancelAckReceived++;

	DBG("Accepted ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION message for message %s from %s(%s)\n",
			getMessageTypeName(messageType), portId, inet_ntoa(tmpAddr));

}

Boolean prepareSMRequestUnicastTransmission(MsgSignaling* outgoing, UnicastGrantData *grant, PtpClock* ptpClock)
{

	DBGV("Preparing CANCEL_UNICAST_TRANSMISSION message\n");

	/* Clause 16.1.4.1.3 */
	if(!grant->requestable) {
		return FALSE;
	}

	initOutgoingMsgSignaling(&grant->parent->portIdentity, outgoing, ptpClock);

        outgoing->header.flagField0 |= PTP_UNICAST;
	outgoing->tlv->tlvType = TLV_REQUEST_UNICAST_TRANSMISSION;
	outgoing->tlv->lengthField = 6;

	SMRequestUnicastTransmission* requestData = NULL;

	XMALLOC(outgoing->tlv->valueField, sizeof(SMRequestUnicastTransmission));
	requestData = (SMRequestUnicastTransmission*)outgoing->tlv->valueField;

	requestData->messageType = grant->messageType;
	requestData->reserved0 = 0;
	requestData->logInterMessagePeriod = grant->logInterval;
	requestData->durationField = grant->duration;

	DBG(" prepared request unicast transmission request for message type 0x%0x\n",
		grant->messageType);

	outgoing->header.sequenceId = grant->parent->grantData[SIGNALING].sentSeqId;
	grant->parent->grantData[SIGNALING].sentSeqId++;

	return TRUE;

}

Boolean prepareSMCancelUnicastTransmission(MsgSignaling* outgoing, UnicastGrantData* grant, PtpClock* ptpClock)
{

	DBGV("Preparing CANCEL_UNICAST_TRANSMISSION message\n");

	initOutgoingMsgSignaling(&grant->parent->portIdentity, outgoing, ptpClock);

        outgoing->header.flagField0 |= PTP_UNICAST;
	outgoing->tlv->tlvType = TLV_CANCEL_UNICAST_TRANSMISSION;
	outgoing->tlv->lengthField = 2;

	SMCancelUnicastTransmission* cancelData = NULL;

	XMALLOC(outgoing->tlv->valueField, sizeof(SMCancelUnicastTransmission));
	cancelData = (SMCancelUnicastTransmission*)outgoing->tlv->valueField;

	grant->requested = FALSE;
	grant->timeLeft = 0;
	grant->expired = FALSE;

	if(!grant->requestable) {
		DBG("Will not issue cancel unicast transmission for non-requestable message type 0x%0x\n", grant->messageType);
		    return FALSE;
	}

	if(!grant->granted) {
		DBG("Will not issue cancel unicast transmission for message type 0x%0x - not granted\n", grant->messageType);
			return FALSE;
	}

	grant->canceled = TRUE;
	grant->cancelCount++;

	cancelData->messageType = grant->messageType;

	DBG(" prepared cancel unicast transmission request for message type %s\n",
		getMessageTypeName(grant->messageType));

	cancelData->reserved0 = 0;
	cancelData->reserved1 = 0;

	outgoing->header.sequenceId = grant->parent->grantData[SIGNALING].sentSeqId;
	grant->parent->grantData[SIGNALING].sentSeqId++;

	return TRUE;

}

/* prepare unicast grant table for use, and mark the right ones requestable */
void
initUnicastGrantTable(UnicastGrantTable *grantTable, Enumeration8 delayMechanism, int nodeCount, UnicastDestination *destinations,
			const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    int i,j;

    UnicastGrantData *grantData;
    UnicastGrantTable *nodeTable;

    /* initialise the index table */
    for(i=0; i < UNICAST_MAX_DESTINATIONS; i++) {
	ptpClock->unicastGrantIndex[i] = NULL;
	ptpClock->syncDestIndex[i].transportAddress = 0;
    }

    for(j=0; j<nodeCount; j++) {    

	nodeTable = &grantTable[j];
	
	memset(nodeTable, 0, sizeof(UnicastGrantTable));

	if(destinations != NULL && (destinations[j].transportAddress != 0)) {
	    nodeTable->transportAddress = destinations[j].transportAddress;
	    nodeTable->domainNumber = destinations[j].domainNumber;
	    nodeTable->localPreference = destinations[j].localPreference;
	    if(nodeTable->domainNumber == 0) {
		nodeTable->domainNumber = rtOpts->domainNumber;
	    }
	    /* for masters: all-ones initially */
	    nodeTable->portIdentity.portNumber = 0xFFFF;
	    memset(&nodeTable->portIdentity.clockIdentity, 0xFF, CLOCK_IDENTITY_LENGTH);
	}

	for(i=0; i<PTP_MAX_MESSAGE; i++) {

	    grantData = &nodeTable->grantData[i];

	    memset(grantData, 0, sizeof(UnicastGrantData));

	    grantData->parent = nodeTable;
	    grantData->messageType = i;

	    switch(grantData->messageType) {

		case PDELAY_RESP:

		    grantData->logMinInterval = rtOpts->logMinPdelayReqInterval;
		    grantData->logMaxInterval = rtOpts->logMaxPdelayReqInterval;
		    grantData->logInterval = grantData->logMinInterval;
		    if(delayMechanism != P2P) break;
		    grantData->requestable = TRUE;

		    break;

		case ANNOUNCE:

		    grantData->logMinInterval = rtOpts->logAnnounceInterval;
		    grantData->logMaxInterval = rtOpts->logMaxAnnounceInterval;
		    grantData->logInterval = grantData->logMinInterval;
		    grantData->requestable = TRUE;
		    break;		    

		case SYNC:

		    grantData->logMinInterval = rtOpts->logSyncInterval;
		    grantData->logMaxInterval = rtOpts->logMaxSyncInterval;
		    grantData->logInterval = grantData->logMinInterval;
		    grantData->requestable = TRUE;
		    break;		    

		case DELAY_RESP:

		    if(delayMechanism != E2E) break;
		    grantData->logMinInterval = rtOpts->logMinDelayReqInterval;
		    grantData->logMaxInterval = rtOpts->logMaxDelayReqInterval;
		    grantData->logInterval = grantData->logMinInterval;
		    grantData->requestable = TRUE;
		    break;		    

		default:
		    break;
	    }

	}

    }


}


/* update unicast grant table with configured intervals and expire them,
 * so that messages are re-requested
 */
void
updateUnicastGrantTable(UnicastGrantTable *grantTable, int nodeCount, const RunTimeOpts *rtOpts)
{

    int i,j;

    UnicastGrantData *grantData;
    UnicastGrantTable *nodeTable;

    for(j=0; j<nodeCount; j++) {    
	nodeTable = &grantTable[j];
	
	for(i=0; i<PTP_MAX_MESSAGE; i++) {

	    grantData = &nodeTable->grantData[i];

	    if(!grantData->requestable) {
		continue;
	    }

	    switch(grantData->messageType) {

		case PDELAY_RESP:

		    grantData->logMinInterval = rtOpts->logMinPdelayReqInterval;
		    grantData->logMaxInterval = rtOpts->logMaxPdelayReqInterval;
		    grantData->logInterval = grantData->logMinInterval;
		    grantData->timeLeft = 0;
		    break;

		case ANNOUNCE:

		    
		    grantData->logMinInterval = rtOpts->logAnnounceInterval;
		    grantData->logMaxInterval = rtOpts->logMaxAnnounceInterval;
		    grantData->logInterval = grantData->logMinInterval;
		    grantData->timeLeft = 0;
		    break;		    

		case SYNC:

		    grantData->logMinInterval = rtOpts->logSyncInterval;
		    grantData->logMaxInterval = rtOpts->logMaxSyncInterval;
		    grantData->logInterval = grantData->logMinInterval;
		    grantData->timeLeft = 0;
		    break;		    

		case DELAY_RESP:

		    grantData->logMinInterval = rtOpts->logMinDelayReqInterval;
		    grantData->logMaxInterval = rtOpts->logMaxDelayReqInterval;
		    grantData->logInterval = grantData->logMinInterval;
		    grantData->timeLeft = 0;
		    break;		    

		default:
		    break;
	    }



	}

    }


}

void requestUnicastTransmission(UnicastGrantData *grant, UInteger32 duration, const RunTimeOpts* rtOpts, PtpClock* ptpClock)
{

	if(duration == 0) {
		DBG("Will not request unicast transmission for 0 duration\n");
	}

	grant->duration = duration;

	/* safeguard */
	if(duration < 5) {
	    grant->duration = 5;
	}

	/* pack and send */
	if(prepareSMRequestUnicastTransmission(&ptpClock->outgoingSignalingTmp, grant, ptpClock)) {
			if(grant->parent->domainNumber != 0) {
			    ptpClock->outgoingSignalingTmp.header.domainNumber = grant->parent->domainNumber;
			}
			issueSignaling(&ptpClock->outgoingSignalingTmp, grant->parent->transportAddress, rtOpts, ptpClock);
			ptpClock->counters.unicastGrantsRequested++;
			/* ready to be monitored */
			grant->requested = TRUE;
			grant->timeLeft = 0;
			grant->granted = FALSE;
			grant->expired = FALSE;
	} 

	/* cleanup msgTmp signalingTLV */
	freeSignalingTLV(&ptpClock->msgTmp.signaling);
	/* cleanup outgoing signalingTLV */
	freeSignalingTLV(&ptpClock->outgoingSignalingTmp);
}

void cancelUnicastTransmission(UnicastGrantData* grant, const const RunTimeOpts* rtOpts, PtpClock* ptpClock)
{

/* todo: dbg sending */

	if(prepareSMCancelUnicastTransmission(&ptpClock->outgoingSignalingTmp, grant, ptpClock)) {
			if(grant->parent->domainNumber != 0) {
			    ptpClock->outgoingSignalingTmp.header.domainNumber = grant->parent->domainNumber;
			}
			issueSignaling(&ptpClock->outgoingSignalingTmp, grant->parent->transportAddress, rtOpts, ptpClock);
			ptpClock->counters.unicastGrantsCancelSent++;
	} 

	/* cleanup msgTmp signalingTLV */
	freeSignalingTLV(&ptpClock->msgTmp.signaling);
	/* cleanup outgoing signalingTLV */
	freeSignalingTLV(&ptpClock->outgoingSignalingTmp);
}

void issueSignaling(MsgSignaling *outgoing, Integer32 destination, const const RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{

	/* pack SignalingTLV */
	msgPackSignalingTLV( ptpClock->msgObuf, outgoing, ptpClock);

	/* set header messageLength, the outgoing->tlv->lengthField is now valid */
	outgoing->header.messageLength = SIGNALING_LENGTH +
					TL_LENGTH +
					outgoing->tlv->lengthField;

	msgPackSignaling( ptpClock->msgObuf, outgoing, ptpClock);

	if(!netSendGeneral(ptpClock->msgObuf, outgoing->header.messageLength,
			   &ptpClock->netPath, rtOpts, destination)) {
		DBGV("Signaling message  can't be sent -> FAULTY state \n");
		ptpClock->counters.messageSendErrors++;
		toState(PTP_FAULTY, rtOpts, ptpClock);
	} else {
		DBG("Signaling msg sent \n");
		ptpClock->counters.signalingMessagesSent++;
	}
}

/* cancel all given or requested grants for a given clock node */
void
cancelNodeGrants(UnicastGrantTable *nodeTable, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    int i;
    UnicastGrantData *grantData;

    for(i=0; i<PTP_MAX_MESSAGE; i++) {
	grantData = &nodeTable->grantData[i];

	if(!grantData->requestable) {
	    continue;
	}

	if(grantData->granted) {
	    cancelUnicastTransmission(grantData, rtOpts, ptpClock);
	    /* sleep 250 to 500 us so that we don't flood the node */
	    usleep(250+round(getRand()*250));
	}

    }
}

/* cancel all given or requested unicast grants */
void
cancelAllGrants(UnicastGrantTable *grantTable, int nodeCount, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    int i;

    for(i=0; i<nodeCount; i++) {    
	cancelNodeGrants(&grantTable[i], rtOpts, ptpClock);
    }

}

void
handleSignaling(MsgHeader *header,
		 Boolean isFromSelf, Integer32 sourceAddress, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBGV("Signaling message received : \n");


	if (isFromSelf) {
		DBGV("handleSignaling: Ignore message from self \n");
		return;
	}

	int tlvOffset = 0;
	int tlvFound = 0;

	while(msgUnpackSignaling(ptpClock->msgIbuf,&ptpClock->msgTmp.signaling, header, ptpClock, tlvOffset)) {

	if(ptpClock->msgTmp.signaling.tlv == NULL) {

	    	if(tlvFound==0) {
		    DBGV("handleSignaling: No TLVs in message\n");
		    ptpClock->counters.messageFormatErrors++;
		} else {
		    DBGV("handleSignaling: No more TLVs\n");
		}
		return;
	}

	tlvFound++;

	/* accept the message if directed either to us or to all-ones */
        if(!acceptPortIdentity(ptpClock->portIdentity, ptpClock->msgTmp.signaling.targetPortIdentity))
        {
                DBGV("handleSignaling: The signaling message was not accepted");
		ptpClock->counters.discardedMessages++;
                goto end;
        }

	/* Can we handle this? */

	switch(ptpClock->msgTmp.signaling.tlv->tlvType)
	{
	case TLV_REQUEST_UNICAST_TRANSMISSION:
		DBGV("handleSignaling: Request Unicast Transmission\n");
		if(!rtOpts->unicastNegotiation || rtOpts->ipMode!=IPMODE_UNICAST) {
			DBGV("handleSignaling: Ignoring unicast negotiation message - not running unicast or negotiation not enabled\n");
			ptpClock->counters.discardedMessages++;
			goto end;
		}
		if( (ptpClock->portState==PTP_LISTENING && !rtOpts->unicastNegotiationListening) || 
		    ptpClock->portState==PTP_DISABLED || ptpClock->portState == PTP_INITIALIZING ||
		    ptpClock->portState==PTP_FAULTY) {
	        	    DBG("Will not grant unicast transmission requests in %s state\n",
                			portState_getName(ptpClock->portState));
			    ptpClock->counters.discardedMessages++;
        		    goto end;
		}
		unpackSMRequestUnicastTransmission(ptpClock->msgIbuf + tlvOffset, &ptpClock->msgTmp.signaling, ptpClock);
		handleSMRequestUnicastTransmission(&ptpClock->msgTmp.signaling, &ptpClock->outgoingSignalingTmp, sourceAddress, rtOpts, ptpClock);
		/* send back the outgoing signaling message */
		if(ptpClock->outgoingSignalingTmp.tlv->tlvType == TLV_GRANT_UNICAST_TRANSMISSION) {
			issueSignaling(&ptpClock->outgoingSignalingTmp, sourceAddress,rtOpts, ptpClock);
		} 
		break;
	case TLV_CANCEL_UNICAST_TRANSMISSION:
		DBGV("handleSignaling: Cancel Unicast Transmission\n");
		if(!rtOpts->unicastNegotiation || rtOpts->ipMode!=IPMODE_UNICAST) {
			DBGV("handleSignaling: Ignoring unicast negotiation message - not running unicast or negotiation not enabled\n");
			ptpClock->counters.discardedMessages++;
			goto end;
		}
		if( (ptpClock->portState==PTP_LISTENING && !rtOpts->unicastNegotiationListening) || 
		    ptpClock->portState==PTP_DISABLED || ptpClock->portState == PTP_INITIALIZING ||
		    ptpClock->portState==PTP_FAULTY) {
	        	    DBG("Will not cancel unicast transmission requests in %s state\n",
                			portState_getName(ptpClock->portState));
			    ptpClock->counters.discardedMessages++;
        		    goto end;
		}
		unpackSMCancelUnicastTransmission(ptpClock->msgIbuf + tlvOffset, &ptpClock->msgTmp.signaling, ptpClock);
		/* send back the cancel acknowledgment - if we have something to acknowledge*/
		if(handleSMCancelUnicastTransmission(&ptpClock->msgTmp.signaling, &ptpClock->outgoingSignalingTmp, sourceAddress, ptpClock)) {
			issueSignaling(&ptpClock->outgoingSignalingTmp, sourceAddress,rtOpts, ptpClock);
		} 
		break;
	case TLV_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:
		DBGV("handleSignaling: Acknowledge Cancel Unicast Transmission\n");
		if(!rtOpts->unicastNegotiation || rtOpts->ipMode!=IPMODE_UNICAST) {
			DBGV("handleSignaling: Ignoring unicast negotiation message - not running unicast or negotiation not enabled\n");
			ptpClock->counters.discardedMessages++;
			goto end;
		}
		if( (ptpClock->portState==PTP_LISTENING && !rtOpts->unicastNegotiationListening) || 
		    ptpClock->portState==PTP_DISABLED || ptpClock->portState == PTP_INITIALIZING ||
		    ptpClock->portState==PTP_FAULTY) {
	        	    DBG("Will not process acknowledge cancel unicast transmission in %s state\n",
                			portState_getName(ptpClock->portState));
			    ptpClock->counters.discardedMessages++;
        		    goto end;
		}
		unpackSMAcknowledgeCancelUnicastTransmission(ptpClock->msgIbuf + tlvOffset, &ptpClock->msgTmp.signaling, ptpClock);
		handleSMAcknowledgeCancelUnicastTransmission(&ptpClock->msgTmp.signaling, sourceAddress, ptpClock);
		break;
	case TLV_GRANT_UNICAST_TRANSMISSION:
		DBGV("handleSignaling: Grant Unicast Transmission\n");
		if(!rtOpts->unicastNegotiation || rtOpts->ipMode!=IPMODE_UNICAST) {
			DBGV("handleSignaling: Ignoring unicast negotiation message - not running unicast or negotiation not enabled\n");
			ptpClock->counters.discardedMessages++;
			goto end;
		}
		if( 
		    ptpClock->portState==PTP_DISABLED || ptpClock->portState == PTP_INITIALIZING ||
		    ptpClock->portState==PTP_FAULTY) {
	        	    DBG("Will not process acknowledge cancel unicast transmission in %s state\n",
                			portState_getName(ptpClock->portState));
			    ptpClock->counters.discardedMessages++;
        		    goto end;
		}
		unpackSMGrantUnicastTransmission(ptpClock->msgIbuf + tlvOffset, &ptpClock->msgTmp.signaling, ptpClock);
		handleSMGrantUnicastTransmission(&ptpClock->msgTmp.signaling, sourceAddress, ptpClock->unicastGrants, ptpClock->unicastDestinationCount, ptpClock);
		if(ptpClock->delayMechanism == P2P) {
		    handleSMGrantUnicastTransmission(&ptpClock->msgTmp.signaling, sourceAddress, &ptpClock->peerGrants, 1, ptpClock);
		}
		break;

	default:

		DBGV("handleSignaling: received unsupported TLV type %04x\n",
		ptpClock->msgTmp.signaling.tlv->tlvType );
		ptpClock->counters.discardedMessages++;
		goto end;
	}

	end:
	/* Movin' on up! */
	tlvOffset += TL_LENGTH + ptpClock->msgTmp.signaling.tlv->lengthField;
	/* cleanup msgTmp signalingTLV */
	freeSignalingTLV(&ptpClock->msgTmp.signaling);
	/* cleanup outgoing signalingTLV */
	freeSignalingTLV(&ptpClock->outgoingSignalingTmp);
	}

    	if(!tlvFound) {
	    DBGV("handleSignaling: No TLVs in message\n");
	    ptpClock->counters.messageFormatErrors++;
	} else {
	    ptpClock->counters.signalingMessagesReceived++;
	    DBGV("handleSignaling: No more TLVs\n");
	}

}

void
refreshUnicastGrants(UnicastGrantTable *grantTable, int nodeCount, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    static int everyN = 0;
    int i,j;

    UnicastGrantData *grantData = NULL;
    UnicastGrantTable *nodeTable = NULL;
    Boolean actionRequired;
    int maxTime = 0;

    /* modulo N counter: used for requesting announce while other master is selected */
    everyN++;
    everyN %= GRANT_KEEPALIVE_INTERVAL;

     /* only notSlave or slaveOnly - nothing inbetween */
     if(ptpClock->clockQuality.clockClass > 127 && ptpClock->clockQuality.clockClass < 255)  {
	return;
     }


    ptpClock->slaveCount = 0;

	for(j=0; j<nodeCount; j++) {    

	    nodeTable = &grantTable[j];
	    maxTime = 0;
	    for(i=0; i<PTP_MAX_MESSAGE; i++) {
		grantData = &nodeTable->grantData[i];
		if(grantData->granted && !grantData->expired) {
		    maxTime = grantData->timeLeft;
		    break;
		}
	    }

	    for(i=0; i<PTP_MAX_MESSAGE; i++) {

		grantData = &nodeTable->grantData[i];

		if(!grantData->requestable) {
		    continue;
		}

		actionRequired = FALSE;
	
		if(grantData->granted) {
		    /* re-request 5 seconds before expiry for continuous service.
		     * masters set this to +10 sec so will keep +5 sec extra 
		     */
		    if(grantData->timeLeft <= 5) {
			DBG("grant for message %s expired\n", getMessageTypeName(grantData->messageType));
			grantData->expired = TRUE;
		    } else {
			if(grantData->timeLeft > maxTime) {
			    maxTime = grantData->timeLeft;
			}
			grantData->timeLeft--;
		    }


		}

		if(grantData->canceled && grantData->cancelCount >= GRANT_CANCEL_ACK_TIMEOUT) {
		    grantData->cancelCount = 0;
		    grantData->canceled = FALSE;
		    grantData->granted = FALSE;
		    grantData->requested = FALSE;
		    grantData->sentSeqId = 0;
		    grantData->timeLeft = 0;
		    grantData->duration = 0;
		}

		if(grantData->expired || (ptpClock->slaveOnly && grantData->requested && !grantData->granted)) {
		    actionRequired = TRUE;
		}

		if(nodeTable->isPeer && grantData->messageType==PDELAY_RESP && !grantData->granted) {
		    actionRequired = TRUE;
		}

		if(!nodeTable->isPeer && ptpClock->slaveOnly) {
		    if(grantData->messageType == ANNOUNCE && !grantData->requested) {
			actionRequired = TRUE;
		    }
		}

		if((ptpClock->slaveOnly || nodeTable->isPeer) && (everyN == (GRANT_KEEPALIVE_INTERVAL -1))){
			if(grantData->receiving == 0 && grantData->granted ) {
			    /* if we mixed n consecutive messages (checked every m seconds), re-request */
 			    if( (everyN * UNICAST_GRANT_REFRESH_INTERVAL) > (GRANT_MAX_MISSED * grantData->logInterval)) {
			    DBG("foreign master: no %s being received - will request again\n",
				    getMessageTypeName(grantData->messageType));
				actionRequired = TRUE;
			    }
			}
			    grantData->receiving = 0;
		}

		/* if we're slave, we request; if we're master, we cancel */
		if(actionRequired) {
		    if (ptpClock->slaveOnly || nodeTable->isPeer) {
			requestUnicastTransmission(grantData, rtOpts->unicastGrantDuration, rtOpts, ptpClock);
		    } else {
			cancelUnicastTransmission(grantData, rtOpts, ptpClock);
		    }
		}

		if(grantData->messageType == ANNOUNCE && ptpClock->portState == PTP_MASTER
		    && grantData->granted) {
			ptpClock->slaveCount++;
		}

	    }

	    nodeTable->timeLeft = maxTime;
	    /* Wild West version:  Murdering Murphy! You done killed my paw! */
	    /* Reggae version:     Matic in dem way, chopper in dem hand, hey, some a dem have M16 'pon dem shoulder */
	    /* Factual version:    Make sure the node is re-usable: reset PortIdentity to all-ones again */
	    if(nodeTable->timeLeft == 0) {
		nodeTable->portIdentity.portNumber = 0xFFFF;
		memset(&nodeTable->portIdentity.clockIdentity, 0xFF, CLOCK_IDENTITY_LENGTH);
		DBG("Unicast node %d now free and reusable\n", j);
	    }
	}

	if(nodeCount == 1 && nodeTable && nodeTable->isPeer) {
		return;
	}
	/* we have some old requests to cancel, we changed the GM */
	if(ptpClock->previousGrants != NULL) {
	    cancelUnicastTransmission(&(ptpClock->previousGrants->grantData[SYNC]), rtOpts, ptpClock);
	    cancelUnicastTransmission(&(ptpClock->previousGrants->grantData[DELAY_RESP]), rtOpts, ptpClock);
	    cancelUnicastTransmission(&(ptpClock->previousGrants->grantData[PDELAY_RESP]), rtOpts, ptpClock);
	    ptpClock->previousGrants->portIdentity.portNumber = 0xFFFF;
	    memset(&ptpClock->previousGrants->portIdentity.clockIdentity, 0xFF, CLOCK_IDENTITY_LENGTH);
	    ptpClock->previousGrants = NULL;
	}

	if(ptpClock->slaveOnly && ptpClock->parentGrants != NULL && ptpClock->portState == PTP_SLAVE) {

		nodeTable = ptpClock->parentGrants;

		if (!nodeTable->grantData[SYNC].requested) {
			grantData=&nodeTable->grantData[SYNC];
			requestUnicastTransmission(grantData, 
			    rtOpts->unicastGrantDuration, rtOpts, ptpClock);
		
		}

		if (nodeTable->grantData[SYNC].granted) {
		    switch(ptpClock->delayMechanism) {
			case E2E:
			    if(!nodeTable->grantData[DELAY_RESP].requested) {
				grantData=&nodeTable->grantData[DELAY_RESP];
				requestUnicastTransmission(grantData, 
				    rtOpts->unicastGrantDuration, rtOpts, ptpClock);
			    }
			    break;
			case P2P:
			    if(!nodeTable->grantData[PDELAY_RESP].requested) {
				grantData=&nodeTable->grantData[PDELAY_RESP];
				requestUnicastTransmission(grantData, 
				    rtOpts->unicastGrantDuration, rtOpts, ptpClock);
			    }
			    break;
			default:
			    break;
		    }

		}
	
	}

}
