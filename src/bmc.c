/*-
 * Copyright (c) 2012-2015 Wojciech Owczarek,
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
 * @file   bmc.c
 * @date   Wed Jun 23 09:36:09 2010
 *
 * @brief  Best master clock selection code.
 *
 * The functions in this file are used by the daemon to select the
 * best master clock from any number of possibilities.
 */

#include "ptpd.h"


/* Init ptpClock with run time values (initialization constants are in constants.h)*/
void initData(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	int i,j;
	j=0;
	DBG("initData\n");
	
	/* Default data set */
	ptpClock->defaultDS.twoStepFlag = TWO_STEP_FLAG;

	/*
	 * init clockIdentity with MAC address and 0xFF and 0xFE. see
	 * spec 7.5.2.2.2
	 */
	for (i=0;i<CLOCK_IDENTITY_LENGTH;i++)
	{
		if (i==3) ptpClock->defaultDS.clockIdentity[i]=0xFF;
		else if (i==4) ptpClock->defaultDS.clockIdentity[i]=0xFE;
		else
		{
		  ptpClock->defaultDS.clockIdentity[i]=ptpClock->netPath.interfaceID[j];
		  j++;
		}
	}

	ptpClock->portDS.lastMismatchedDomain = -1;

	if(rtOpts->pidAsClockId) {
	    uint16_t pid = htons(getpid());
	    memcpy(ptpClock->defaultDS.clockIdentity + 3, &pid, 2);
	}

	ptpClock->bestMaster = NULL;
	ptpClock->defaultDS.numberPorts = NUMBER_PORTS;

	ptpClock->disabled = rtOpts->portDisabled;

	memset(ptpClock->userDescription, 0, sizeof(ptpClock->userDescription));
	memcpy(ptpClock->userDescription, rtOpts->portDescription, strlen(rtOpts->portDescription));

	memset(&ptpClock->profileIdentity,0,6);

	if(rtOpts->ipMode == IPMODE_UNICAST && rtOpts->unicastNegotiation) {
		memcpy(&ptpClock->profileIdentity, &PROFILE_ID_TELECOM,6);
	}

	if(rtOpts->ipMode == IPMODE_MULTICAST &&rtOpts->delayMechanism == E2E) {
		memcpy(&ptpClock->profileIdentity, &PROFILE_ID_DEFAULT_E2E,6);
	}

	if(rtOpts->ipMode == IPMODE_MULTICAST &&rtOpts->delayMechanism == P2P) {
		memcpy(&ptpClock->profileIdentity, &PROFILE_ID_DEFAULT_P2P,6);
	}

	if(rtOpts->dot1AS) {
		memcpy(&ptpClock->profileIdentity, &PROFILE_ID_802_1AS,6);
	}

	ptpClock->defaultDS.clockQuality.clockAccuracy =
		rtOpts->clockQuality.clockAccuracy;
	ptpClock->defaultDS.clockQuality.clockClass = rtOpts->clockQuality.clockClass;
	ptpClock->defaultDS.clockQuality.offsetScaledLogVariance =
		rtOpts->clockQuality.offsetScaledLogVariance;

	ptpClock->defaultDS.priority1 = rtOpts->priority1;
	ptpClock->defaultDS.priority2 = rtOpts->priority2;

	ptpClock->defaultDS.domainNumber = rtOpts->domainNumber;

	if(rtOpts->slaveOnly) {
		ptpClock->defaultDS.slaveOnly = TRUE;
		rtOpts->clockQuality.clockClass = SLAVE_ONLY_CLOCK_CLASS;
		ptpClock->defaultDS.clockQuality.clockClass = SLAVE_ONLY_CLOCK_CLASS;
	}

/* Port configuration data set */

	/*
	 * PortIdentity Init (portNumber = 1 for an ardinary clock spec
	 * 7.5.2.3)
	 */
	copyClockIdentity(ptpClock->portDS.portIdentity.clockIdentity,
			ptpClock->defaultDS.clockIdentity);
	ptpClock->portDS.portIdentity.portNumber = rtOpts->portNumber;

	/* select the initial rate of delayreqs until we receive the first announce message */

	ptpClock->portDS.logMinDelayReqInterval = rtOpts->initial_delayreq;

	clearTime(&ptpClock->portDS.peerMeanPathDelay);

	ptpClock->portDS.logAnnounceInterval = rtOpts->logAnnounceInterval;
	ptpClock->portDS.announceReceiptTimeout = rtOpts->announceReceiptTimeout;
	ptpClock->portDS.logSyncInterval = rtOpts->logSyncInterval;
	ptpClock->portDS.delayMechanism = rtOpts->delayMechanism;
	ptpClock->portDS.logMinPdelayReqInterval = rtOpts->logMinPdelayReqInterval;
	ptpClock->portDS.versionNumber = VERSION_PTP;

	if(rtOpts->dot1AS) {
	    ptpClock->portDS.transportSpecific = TSP_ETHERNET_AVB;
	} else {
	    ptpClock->portDS.transportSpecific = TSP_DEFAULT;
	}

 	/*
	 *  Initialize random number generator using same method as ptpv1:
	 *  seed is now initialized from the last bytes of our mac addres (collected in net.c:findIface())
	 */
	srand((ptpClock->netPath.interfaceID[PTP_UUID_LENGTH - 1] << 8) +
	    ptpClock->netPath.interfaceID[PTP_UUID_LENGTH - 2]);

	/*Init other stuff*/
	ptpClock->number_foreign_records = 0;
  	ptpClock->max_foreign_records = rtOpts->max_foreign_records;
}

/* memcmp behaviour: -1: a<b, 1: a>b, 0: a=b */
int
cmpPortIdentity(const PortIdentity *a, const PortIdentity *b)
{

    int comp;

    /* compare clock identity first */

    comp = memcmp(a->clockIdentity, b->clockIdentity, CLOCK_IDENTITY_LENGTH);

    if(comp !=0) {
	return comp;
    }

    /* then compare portNumber */

    if(a->portNumber > b->portNumber) {
	return 1;
    }

    if(a->portNumber < b->portNumber) {
	return -1;
    }

    return 0;

}

/* compare portIdentity to an empty one */
Boolean portIdentityEmpty(PortIdentity *portIdentity) {

    PortIdentity zero;
    memset(&zero, 0, sizeof(PortIdentity));

    if (!cmpPortIdentity(portIdentity, &zero)) {
	return TRUE;
    }

    return FALSE;
}

/* compare portIdentity to all ones port identity */
Boolean portIdentityAllOnes(PortIdentity *portIdentity) {

    PortIdentity allOnes;
    memset(&allOnes, 0xFF, sizeof(PortIdentity));

    if (!cmpPortIdentity(portIdentity, &allOnes)) {
        return TRUE;
    }

    return FALSE;
}

/*Local clock is becoming Master. Table 13 (9.3.5) of the spec.*/
void m1(const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	/*Current data set update*/
	ptpClock->currentDS.stepsRemoved = 0;
	
	clearTime(&ptpClock->currentDS.offsetFromMaster);
	clearTime(&ptpClock->currentDS.meanPathDelay);

	copyClockIdentity(ptpClock->parentDS.parentPortIdentity.clockIdentity,
	       ptpClock->defaultDS.clockIdentity);

	ptpClock->parentDS.parentPortIdentity.portNumber = ptpClock->portDS.portIdentity.portNumber;
	ptpClock->parentDS.parentStats = DEFAULT_PARENTS_STATS;
	ptpClock->parentDS.observedParentClockPhaseChangeRate = 0;
	ptpClock->parentDS.observedParentOffsetScaledLogVariance = 0;
	copyClockIdentity(ptpClock->parentDS.grandmasterIdentity,
			ptpClock->defaultDS.clockIdentity);
	ptpClock->parentDS.grandmasterClockQuality.clockAccuracy =
		ptpClock->defaultDS.clockQuality.clockAccuracy;
	ptpClock->parentDS.grandmasterClockQuality.clockClass =
		ptpClock->defaultDS.clockQuality.clockClass;
	ptpClock->parentDS.grandmasterClockQuality.offsetScaledLogVariance =
		ptpClock->defaultDS.clockQuality.offsetScaledLogVariance;
	ptpClock->parentDS.grandmasterPriority1 = ptpClock->defaultDS.priority1;
	ptpClock->parentDS.grandmasterPriority2 = ptpClock->defaultDS.priority2;
        ptpClock->portDS.logMinDelayReqInterval = rtOpts->logMinDelayReqInterval;

	/*Time Properties data set*/
	ptpClock->timePropertiesDS.currentUtcOffsetValid = rtOpts->timeProperties.currentUtcOffsetValid;
	ptpClock->timePropertiesDS.currentUtcOffset = rtOpts->timeProperties.currentUtcOffset;
	ptpClock->timePropertiesDS.timeTraceable = rtOpts->timeProperties.timeTraceable;
	ptpClock->timePropertiesDS.frequencyTraceable = rtOpts->timeProperties.frequencyTraceable;
	ptpClock->timePropertiesDS.ptpTimescale = rtOpts->timeProperties.ptpTimescale;
	ptpClock->timePropertiesDS.timeSource = rtOpts->timeProperties.timeSource;

	if(ptpClock->timePropertiesDS.ptpTimescale &&
	    (secondsToMidnight() < rtOpts->leapSecondNoticePeriod)) {
	    ptpClock->timePropertiesDS.leap59 = ptpClock->clockStatus.leapDelete;
	    ptpClock->timePropertiesDS.leap61 = ptpClock->clockStatus.leapInsert;
	} else {
	    ptpClock->timePropertiesDS.leap59 = FALSE;
	    ptpClock->timePropertiesDS.leap61 = FALSE;
	}

}


/* first cut on a passive mode specific BMC actions */
void p1(PtpClock *ptpClock, const RunTimeOpts *rtOpts)
{
	/* make sure we revert to ARB timescale in Passive mode*/
	if(ptpClock->portDS.portState == PTP_PASSIVE){
		ptpClock->timePropertiesDS.currentUtcOffsetValid = rtOpts->timeProperties.currentUtcOffsetValid;
		ptpClock->timePropertiesDS.currentUtcOffset = rtOpts->timeProperties.currentUtcOffset;
	}
	
}


/*Local clock is synchronized to Ebest Table 16 (9.3.5) of the spec*/
void s1(MsgHeader *header,MsgAnnounce *announce,PtpClock *ptpClock, const RunTimeOpts *rtOpts)
{

	Boolean firstUpdate = !cmpPortIdentity(&ptpClock->parentDS.parentPortIdentity, &ptpClock->portDS.portIdentity);
	TimePropertiesDS tpPrevious = ptpClock->timePropertiesDS;

	Boolean previousLeap59 = FALSE;
	Boolean previousLeap61 = FALSE;

	Boolean leapChange = FALSE;

	Integer16 previousUtcOffset = 0;

	previousLeap59 = ptpClock->timePropertiesDS.leap59;
	previousLeap61 = ptpClock->timePropertiesDS.leap61;

	previousUtcOffset = ptpClock->timePropertiesDS.currentUtcOffset;

	/* Current DS */
	ptpClock->currentDS.stepsRemoved = announce->stepsRemoved + 1;

	/* Parent DS */
	copyClockIdentity(ptpClock->parentDS.parentPortIdentity.clockIdentity,
	       header->sourcePortIdentity.clockIdentity);
	ptpClock->parentDS.parentPortIdentity.portNumber =
		header->sourcePortIdentity.portNumber;
	copyClockIdentity(ptpClock->parentDS.grandmasterIdentity,
			announce->grandmasterIdentity);
	ptpClock->parentDS.grandmasterClockQuality.clockAccuracy =
		announce->grandmasterClockQuality.clockAccuracy;
	ptpClock->parentDS.grandmasterClockQuality.clockClass =
		announce->grandmasterClockQuality.clockClass;
	ptpClock->parentDS.grandmasterClockQuality.offsetScaledLogVariance =
		announce->grandmasterClockQuality.offsetScaledLogVariance;
	ptpClock->parentDS.grandmasterPriority1 = announce->grandmasterPriority1;
	ptpClock->parentDS.grandmasterPriority2 = announce->grandmasterPriority2;

	/* use the granted interval if using signaling, otherwise we would try to arm a timer for 2^127! */
	if(rtOpts->unicastNegotiation && ptpClock->parentGrants != NULL && ptpClock->parentGrants->grantData[ANNOUNCE_INDEXED].granted) {
            ptpClock->portDS.logAnnounceInterval = ptpClock->parentGrants->grantData[ANNOUNCE_INDEXED].logInterval;
        } else if (header->logMessageInterval != UNICAST_MESSAGEINTERVAL) {
    	    ptpClock->portDS.logAnnounceInterval = header->logMessageInterval;
	}

	/* Timeproperties DS */
	ptpClock->timePropertiesDS.currentUtcOffset = announce->currentUtcOffset;

	if (ptpClock->portDS.portState != PTP_PASSIVE && ptpClock->timePropertiesDS.currentUtcOffsetValid &&
			!IS_SET(header->flagField1, UTCV)) {
		if(rtOpts->alwaysRespectUtcOffset)
			WARNING("UTC Offset no longer valid and ptpengine:always_respect_utc_offset is set: continuing as normal\n");
		else
			WARNING("UTC Offset no longer valid - clock jump expected\n");
	}

        /* "Valid" is bit 2 in second octet of flagfield */
        ptpClock->timePropertiesDS.currentUtcOffsetValid = IS_SET(header->flagField1, UTCV);

	/* set PTP_PASSIVE-specific state */
	p1(ptpClock, rtOpts);

	/* only set leap flags in slave state - info from leap file takes priority*/
	if (ptpClock->portDS.portState == PTP_SLAVE) {
	    if(ptpClock->clockStatus.override) {
		ptpClock->timePropertiesDS.currentUtcOffset = ptpClock->clockStatus.utcOffset;
		ptpClock->timePropertiesDS.leap59 = ptpClock->clockStatus.leapDelete;
		ptpClock->timePropertiesDS.leap61 = ptpClock->clockStatus.leapInsert;
	    } else {
		ptpClock->timePropertiesDS.leap59 = IS_SET(header->flagField1, LI59);
		ptpClock->timePropertiesDS.leap61 = IS_SET(header->flagField1, LI61);
	    }
	}

        ptpClock->timePropertiesDS.timeTraceable = IS_SET(header->flagField1, TTRA);
        ptpClock->timePropertiesDS.frequencyTraceable = IS_SET(header->flagField1, FTRA);
        ptpClock->timePropertiesDS.ptpTimescale = IS_SET(header->flagField1, PTPT);
        ptpClock->timePropertiesDS.timeSource = announce->timeSource;

	/* if Announce was accepted from some domain, so be it */
	if(rtOpts->anyDomain || rtOpts->unicastNegotiation) {
	    ptpClock->defaultDS.domainNumber = header->domainNumber;
	}

	/*
	 * tell the clock source to update TAI offset, but only if timescale is
	 * PTP not ARB - spec section 7.2
	 */
        if (ptpClock->timePropertiesDS.ptpTimescale &&
	    (ptpClock->timePropertiesDS.currentUtcOffsetValid || rtOpts->alwaysRespectUtcOffset) &&
            (ptpClock->timePropertiesDS.currentUtcOffset != previousUtcOffset)) {
	      INFO("UTC offset is now %d\n",
		    ptpClock->timePropertiesDS.currentUtcOffset);
		ptpClock->clockStatus.utcOffset = ptpClock->timePropertiesDS.currentUtcOffset;
		ptpClock->clockStatus.update = TRUE;

        }

	if(!firstUpdate && memcmp(&tpPrevious,&ptpClock->timePropertiesDS, sizeof(TimePropertiesDS))) {
	    /* this is an event - will be picked up and dispatched, no need to set false */
	    SET_ALARM(ALRM_TIMEPROP_CHANGE, TRUE);
	} 

	/* non-slave logic done, exit if not slave */
        if (ptpClock->portDS.portState != PTP_SLAVE) {
		return;
	}

	/* Leap second handling */

	if(ptpClock->timePropertiesDS.leap59 && ptpClock->timePropertiesDS.leap61) {
		DBG("Both Leap59 and Leap61 flags set!\n");
		ptpClock->counters.protocolErrors++;
		return;
	}

	/* One of the flags has been cleared */
	leapChange = ((previousLeap59 && !ptpClock->timePropertiesDS.leap59) ||
            		    (previousLeap61 && !ptpClock->timePropertiesDS.leap61));

	if(ptpClock->leapSecondPending && leapChange && !ptpClock->leapSecondInProgress) {
		WARNING("Leap second event aborted by GM!\n");
		ptpClock->leapSecondPending = FALSE;
		ptpClock->leapSecondInProgress = FALSE;
		timerStop(&ptpClock->timers[LEAP_SECOND_PAUSE_TIMER]);
		ptpClock->clockStatus.leapInsert = FALSE;
		ptpClock->clockStatus.leapDelete = FALSE;
		ptpClock->clockStatus.update = TRUE;
	}

	/*
	 * one of the leap second flags is lit
	 * but we have no event pending
	 */

	/* changes in flags while not waiting for leap second */

	if(!ptpClock->leapSecondPending &&  !ptpClock->leapSecondInProgress) {
	
	    if(rtOpts->leapSecondHandling == LEAP_ACCEPT) {
		ptpClock->clockStatus.leapInsert = ptpClock->timePropertiesDS.leap61;
		ptpClock->clockStatus.leapDelete = ptpClock->timePropertiesDS.leap59;
	    }

		if(ptpClock->timePropertiesDS.leap61 || ptpClock->timePropertiesDS.leap59) {
		    ptpClock->clockStatus.update = TRUE;
		    /* only set the flag, the rest happens in doState() */
		    ptpClock->leapSecondPending = TRUE;
		}
	}

	/* UTC offset has changed */
	if(previousUtcOffset != ptpClock->timePropertiesDS.currentUtcOffset) {
	    if(!ptpClock->leapSecondPending &&  !ptpClock->leapSecondInProgress) {
		WARNING("UTC offset changed from %d to %d with "
			 "no leap second pending!\n",
				previousUtcOffset,
				ptpClock->timePropertiesDS.currentUtcOffset);
	    } else {
			WARNING("UTC offset changed from %d to %d\n",
				previousUtcOffset,ptpClock->timePropertiesDS.currentUtcOffset);
	    }
	}
}


/*Copy local data set into header and announce message. 9.3.4 table 12*/
static void
copyD0(MsgHeader *header, MsgAnnounce *announce, PtpClock *ptpClock)
{
	announce->grandmasterPriority1 = ptpClock->defaultDS.priority1;
	copyClockIdentity(announce->grandmasterIdentity,
			ptpClock->defaultDS.clockIdentity);
	announce->grandmasterClockQuality.clockClass =
		ptpClock->defaultDS.clockQuality.clockClass;
	announce->grandmasterClockQuality.clockAccuracy =
		ptpClock->defaultDS.clockQuality.clockAccuracy;
	announce->grandmasterClockQuality.offsetScaledLogVariance =
		ptpClock->defaultDS.clockQuality.offsetScaledLogVariance;
	announce->grandmasterPriority2 = ptpClock->defaultDS.priority2;
	announce->stepsRemoved = 0;
	copyClockIdentity(header->sourcePortIdentity.clockIdentity,
	       ptpClock->defaultDS.clockIdentity);

	/* Copy TimePropertiesDS into FlagField1 */
        header->flagField1 = ptpClock->timePropertiesDS.leap61			<< 0;
        header->flagField1 |= ptpClock->timePropertiesDS.leap59			<< 1;
        header->flagField1 |= ptpClock->timePropertiesDS.currentUtcOffsetValid	<< 2;
        header->flagField1 |= ptpClock->timePropertiesDS.ptpTimescale		<< 3;
        header->flagField1 |= ptpClock->timePropertiesDS.timeTraceable		<< 4;
        header->flagField1 |= ptpClock->timePropertiesDS.frequencyTraceable	<< 5;

}


/*Data set comparison bewteen two foreign masters (9.3.4 fig 27)
 * return similar to memcmp() */

static Integer8
bmcDataSetComparison(const ForeignMasterRecord *a, const ForeignMasterRecord *b, const PtpClock *ptpClock, const RunTimeOpts *rtOpts)
{


	DBGV("Data set comparison \n");
	short comp = 0;


	/* disqualification comes above anything else */

	if(a->disqualified > b->disqualified) {
	    return -1;
	}

	if(a->disqualified < b->disqualified) {
	    return 1;
	}

	/*Identity comparison*/
	comp = memcmp(a->announce.grandmasterIdentity,b->announce.grandmasterIdentity,CLOCK_IDENTITY_LENGTH);

	if (comp!=0)
		goto dataset_comp_part_1;

	  /* Algorithm part2 Fig 28 */
	if (a->announce.stepsRemoved > b->announce.stepsRemoved+1)
		return 1;
	if (a->announce.stepsRemoved+1 < b->announce.stepsRemoved)
		return -1;

	/* A within 1 of B */

	if (a->announce.stepsRemoved > b->announce.stepsRemoved) {
		comp = memcmp(a->header.sourcePortIdentity.clockIdentity,ptpClock->parentDS.parentPortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH);
		if(comp < 0)
			return -1;
		if(comp > 0)
			return 1;
		DBG("Sender=Receiver : Error -1");
		return 0;
	}

	if (a->announce.stepsRemoved < b->announce.stepsRemoved) {
		comp = memcmp(b->header.sourcePortIdentity.clockIdentity,ptpClock->parentDS.parentPortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH);

		if(comp < 0)
			return -1;
		if(comp > 0)
			return 1;
		DBG("Sender=Receiver : Error -1");
		return 0;
	}
	/*  steps removed A = steps removed B */
	comp = memcmp(a->header.sourcePortIdentity.clockIdentity,b->header.sourcePortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH);

	if (comp<0) {
		return -1;
	}

	if (comp>0) {
		return 1;
	}

	/* identity A = identity B */

	if (a->header.sourcePortIdentity.portNumber < b->header.sourcePortIdentity.portNumber)
		return -1;
	if (a->header.sourcePortIdentity.portNumber > b->header.sourcePortIdentity.portNumber)
		return 1;

	DBG("Sender=Receiver : Error -2");
	return 0;

	  /* Algorithm part 1 Fig 27 */
dataset_comp_part_1:

	/* OPTIONAL domain comparison / any domain */
	
	if(rtOpts->anyDomain) {
	    /* part 1: preferred domain wins */
	    if(a->header.domainNumber == rtOpts->domainNumber && b->header.domainNumber != ptpClock->defaultDS.domainNumber)
		return -1;
	    if(a->header.domainNumber != rtOpts->domainNumber && b->header.domainNumber == ptpClock->defaultDS.domainNumber)
		return 1;
	
	    /* part 2: lower domain wins */
	    if(a->header.domainNumber < b->header.domainNumber)
		return -1;

	    if(a->header.domainNumber > b->header.domainNumber)
		return 1;
	}

	/* Compare localPreference - only used by slaves when using unicast negotiation */
	DBGV("bmcDataSetComparison a->localPreference: %d, b->localPreference: %d\n", a->localPreference, b->localPreference);
	if(a->localPreference < b->localPreference) {
	    return -1;
	}
	if(a->localPreference > b->localPreference) {
	    return 1;
	}

	/* Compare GM priority1 */
	if (a->announce.grandmasterPriority1 < b->announce.grandmasterPriority1)
		return -1;
	if (a->announce.grandmasterPriority1 > b->announce.grandmasterPriority1)
		return 1;

	/* non-standard BMC extension to prioritise GMs with UTC valid */
	if(rtOpts->preferUtcValid) {
		Boolean utcA = IS_SET(a->header.flagField1, UTCV);
		Boolean utcB = IS_SET(b->header.flagField1, UTCV);
		if(utcA > utcB)
			return -1;
		if(utcA < utcB)
			return 1;
	}

	/* Compare GM class */
	if (a->announce.grandmasterClockQuality.clockClass <
			b->announce.grandmasterClockQuality.clockClass)
		return -1;
	if (a->announce.grandmasterClockQuality.clockClass >
			b->announce.grandmasterClockQuality.clockClass)
		return 1;
	
	/* Compare GM accuracy */
	if (a->announce.grandmasterClockQuality.clockAccuracy <
			b->announce.grandmasterClockQuality.clockAccuracy)
		return -1;
	if (a->announce.grandmasterClockQuality.clockAccuracy >
			b->announce.grandmasterClockQuality.clockAccuracy)
		return 1;

	/* Compare GM offsetScaledLogVariance */
	if (a->announce.grandmasterClockQuality.offsetScaledLogVariance <
			b->announce.grandmasterClockQuality.offsetScaledLogVariance)
		return -1;
	if (a->announce.grandmasterClockQuality.offsetScaledLogVariance >
			b->announce.grandmasterClockQuality.offsetScaledLogVariance)
		return 1;
	
	/* Compare GM priority2 */
	if (a->announce.grandmasterPriority2 < b->announce.grandmasterPriority2)
		return -1;
	if (a->announce.grandmasterPriority2 > b->announce.grandmasterPriority2)
		return 1;

	/* Compare GM identity */
	if (comp < 0)
		return -1;
	else if (comp > 0)
		return 1;
	return 0;
}

/*State decision algorithm 9.3.3 Fig 26*/
static UInteger8
bmcStateDecision(ForeignMasterRecord *foreign, const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Integer8 comp;
	Boolean newBM;
	ForeignMasterRecord me;

	memset(&me, 0, sizeof(me));
	me.localPreference = LOWEST_LOCALPREFERENCE;

	newBM = ((memcmp(foreign->header.sourcePortIdentity.clockIdentity,
			    ptpClock->parentDS.parentPortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH)) ||
		(foreign->header.sourcePortIdentity.portNumber != ptpClock->parentDS.parentPortIdentity.portNumber));


	
	if (ptpClock->defaultDS.slaveOnly) {
		/* master has changed: mark old grants for cancellation - refreshUnicastGrants will pick this up */
		if(newBM && (ptpClock->parentGrants != NULL)) {
		    ptpClock->previousGrants = ptpClock->parentGrants;
		}
		s1(&foreign->header,&foreign->announce,ptpClock, rtOpts);
		if(rtOpts->unicastNegotiation) {
			ptpClock->parentGrants = findUnicastGrants(&ptpClock->parentDS.parentPortIdentity, 0,
						ptpClock->unicastGrants, &ptpClock->grantIndex, ptpClock->unicastDestinationCount,
					    FALSE);
		}
		if (newBM) {
		/* New BM #1 */
			SET_ALARM(ALRM_MASTER_CHANGE, TRUE);
			displayPortIdentity(&foreign->header.sourcePortIdentity,
					    "New best master selected:");
			ptpClock->counters.bestMasterChanges++;
			if (ptpClock->portDS.portState == PTP_SLAVE)
				displayStatus(ptpClock, "State: ");
				if(rtOpts->calibrationDelay) {
					ptpClock->isCalibrated = FALSE;
					timerStart(&ptpClock->timers[CALIBRATION_DELAY_TIMER], rtOpts->calibrationDelay);
				}
		}
                if(rtOpts->unicastNegotiation && ptpClock->parentGrants != NULL) {
                        ptpClock->portDS.logAnnounceInterval = ptpClock->parentGrants->grantData[ANNOUNCE_INDEXED].logInterval;
			me.localPreference = ptpClock->parentGrants->localPreference;
		}

		return PTP_SLAVE;
	}

	if ((!ptpClock->number_foreign_records) &&
	    (ptpClock->portDS.portState == PTP_LISTENING))
		return PTP_LISTENING;

//	copyD0(&me.headerk->msgTmpHeader,&ptpClock->msgTmp.announce,ptpClock);
	copyD0(&me.header, &me.announce, ptpClock);

	DBGV("local clockQuality.clockClass: %d \n", ptpClock->defaultDS.clockQuality.clockClass);

	comp = bmcDataSetComparison(&me, foreign, ptpClock, rtOpts);
	if (ptpClock->defaultDS.clockQuality.clockClass < 128) {
		if (comp < 0) {
			m1(rtOpts, ptpClock);
			return PTP_MASTER;
		} else if (comp > 0) {
			s1(&foreign->header, &foreign->announce, ptpClock, rtOpts);
			if (newBM) {
			/* New BM #2 */
				SET_ALARM(ALRM_MASTER_CHANGE, TRUE);
				displayPortIdentity(&foreign->header.sourcePortIdentity,
						    "New best master selected:");
				ptpClock->counters.bestMasterChanges++;
				if(ptpClock->portDS.portState == PTP_PASSIVE)
					displayStatus(ptpClock, "State: ");
			}
			return PTP_PASSIVE;
		} else {
			DBG("Error in bmcDataSetComparison..\n");
		}
	} else {
		if (comp < 0) {
			m1(rtOpts,ptpClock);
			return PTP_MASTER;
		} else if (comp > 0) {
			s1(&foreign->header, &foreign->announce,ptpClock, rtOpts);
			if (newBM) {
			/* New BM #3 */
				SET_ALARM(ALRM_MASTER_CHANGE, TRUE);
				displayPortIdentity(&foreign->header.sourcePortIdentity,
						    "New best master selected:");
				ptpClock->counters.bestMasterChanges++;
				if(ptpClock->portDS.portState == PTP_SLAVE)
					displayStatus(ptpClock, "State: ");
				if(rtOpts->calibrationDelay) {
					ptpClock->isCalibrated = FALSE;
					timerStart(&ptpClock->timers[CALIBRATION_DELAY_TIMER], rtOpts->calibrationDelay);
				}
			}
			return PTP_SLAVE;
		} else {
			DBG("Error in bmcDataSetComparison..\n");
		}
	}

	ptpClock->counters.protocolErrors++;
	/*  MB: Is this the return code below correct? */
	/*  Anyway, it's a valid return code. */

	return PTP_FAULTY;
}



UInteger8
bmc(ForeignMasterRecord *foreignMaster,
    const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Integer16 i,best;

	DBGV("number_foreign_records : %d \n", ptpClock->number_foreign_records);
	if (!ptpClock->number_foreign_records)
		if (ptpClock->portDS.portState == PTP_MASTER)	{
			m1(rtOpts,ptpClock);
			return ptpClock->portDS.portState;
		}

	for (i=1,best = 0; i<ptpClock->number_foreign_records;i++)
		if ((bmcDataSetComparison(&foreignMaster[i], &foreignMaster[best],
					  ptpClock, rtOpts)) < 0)
			best = i;

	DBGV("Best record : %d \n",best);
	ptpClock->foreign_record_best = best;
	ptpClock->bestMaster = &foreignMaster[best];
	return (bmcStateDecision(ptpClock->bestMaster,
				 rtOpts,ptpClock));
}
