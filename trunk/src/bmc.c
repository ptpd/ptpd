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
	ptpClock->twoStepFlag = TWO_STEP_FLAG;

	/*
	 * init clockIdentity with MAC address and 0xFF and 0xFE. see
	 * spec 7.5.2.2.2
	 */
	for (i=0;i<CLOCK_IDENTITY_LENGTH;i++)
	{
		if (i==3) ptpClock->clockIdentity[i]=0xFF;
		else if (i==4) ptpClock->clockIdentity[i]=0xFE;
		else
		{
		  ptpClock->clockIdentity[i]=ptpClock->port_uuid_field[j];
		  j++;
		}
	}
	ptpClock->numberPorts = NUMBER_PORTS;

	ptpClock->clockQuality.clockAccuracy = 
		rtOpts->clockQuality.clockAccuracy;
	ptpClock->clockQuality.clockClass = rtOpts->clockQuality.clockClass;
	ptpClock->clockQuality.offsetScaledLogVariance = 
		rtOpts->clockQuality.offsetScaledLogVariance;

	ptpClock->priority1 = rtOpts->priority1;
	ptpClock->priority2 = rtOpts->priority2;

	ptpClock->domainNumber = rtOpts->domainNumber;
	ptpClock->slaveOnly = rtOpts->slaveOnly;
	if(rtOpts->slaveOnly)
		rtOpts->clockQuality.clockClass = SLAVE_ONLY_CLOCK_CLASS;

/* Port configuration data set */

	/*
	 * PortIdentity Init (portNumber = 1 for an ardinary clock spec
	 * 7.5.2.3)
	 */
	copyClockIdentity(ptpClock->portIdentity.clockIdentity,
			ptpClock->clockIdentity);
	ptpClock->portIdentity.portNumber = NUMBER_PORTS;

	/* select the initial rate of delayreqs until we receive the first announce message */
	ptpClock->logMinDelayReqInterval = rtOpts->initial_delayreq;

	clearTime(&ptpClock->peerMeanPathDelay);

	ptpClock->logAnnounceInterval = rtOpts->announceInterval;
	ptpClock->announceReceiptTimeout = rtOpts->announceReceiptTimeout;
	ptpClock->logSyncInterval = rtOpts->syncInterval;
	ptpClock->delayMechanism = rtOpts->delayMechanism;
	ptpClock->logMinPdelayReqInterval = DEFAULT_PDELAYREQ_INTERVAL;
	ptpClock->versionNumber = VERSION_PTP;

 	/*
	 *  Initialize random number generator using same method as ptpv1:
	 *  seed is now initialized from the last bytes of our mac addres (collected in net.c:findIface())
	 */
	srand((ptpClock->port_uuid_field[PTP_UUID_LENGTH - 1] << 8) + ptpClock->port_uuid_field[PTP_UUID_LENGTH - 2]);

	/*Init other stuff*/
	ptpClock->number_foreign_records = 0;
  	ptpClock->max_foreign_records = rtOpts->max_foreign_records;
}


/*Local clock is becoming Master. Table 13 (9.3.5) of the spec.*/
void m1(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	/*Current data set update*/
	ptpClock->stepsRemoved = 0;
	
	clearTime(&ptpClock->offsetFromMaster);
	clearTime(&ptpClock->meanPathDelay);

	/*Parent data set*/
	copyClockIdentity(ptpClock->parentPortIdentity.clockIdentity,
	       ptpClock->clockIdentity);
	ptpClock->parentPortIdentity.portNumber = 0;
	ptpClock->parentStats = DEFAULT_PARENTS_STATS;
	ptpClock->observedParentClockPhaseChangeRate = 0;
	ptpClock->observedParentOffsetScaledLogVariance = 0;
	copyClockIdentity(ptpClock->grandmasterIdentity,
			ptpClock->clockIdentity);
	ptpClock->grandmasterClockQuality.clockAccuracy = 
		ptpClock->clockQuality.clockAccuracy;
	ptpClock->grandmasterClockQuality.clockClass = 
		ptpClock->clockQuality.clockClass;
	ptpClock->grandmasterClockQuality.offsetScaledLogVariance = 
		ptpClock->clockQuality.offsetScaledLogVariance;
	ptpClock->grandmasterPriority1 = ptpClock->priority1;
	ptpClock->grandmasterPriority2 = ptpClock->priority2;

	/*Time Properties data set*/
	ptpClock->timeSource = INTERNAL_OSCILLATOR;

	/* UTC vs TAI timescales */
	ptpClock->currentUtcOffsetValid = DEFAULT_UTC_VALID;
	ptpClock->currentUtcOffset = rtOpts->currentUtcOffset;
	
}


/* first cut on a passive mode specific BMC actions */
void p1(PtpClock *ptpClock, RunTimeOpts *rtOpts)
{
	/* make sure we revert to ARB timescale in Passive mode*/
	if(ptpClock->portState == PTP_PASSIVE){
		ptpClock->currentUtcOffsetValid = DEFAULT_UTC_VALID;
		ptpClock->currentUtcOffset = rtOpts->currentUtcOffset;
	}
	
}


/*Local clock is synchronized to Ebest Table 16 (9.3.5) of the spec*/
void s1(MsgHeader *header,MsgAnnounce *announce,PtpClock *ptpClock, RunTimeOpts *rtOpts)
{

	Boolean previousLeap59 = FALSE, previousLeap61 = FALSE;
	Integer16 previousUtcOffset = 0;

	if (ptpClock->portState == PTP_SLAVE) {
		previousLeap59 = ptpClock->leap59;
		previousLeap61 = ptpClock->leap61;
		previousUtcOffset = ptpClock->currentUtcOffset;
	}

	/* Current DS */
	ptpClock->stepsRemoved = announce->stepsRemoved + 1;

	/* Parent DS */
	copyClockIdentity(ptpClock->parentPortIdentity.clockIdentity,
	       header->sourcePortIdentity.clockIdentity);
	ptpClock->parentPortIdentity.portNumber = 
		header->sourcePortIdentity.portNumber;
	copyClockIdentity(ptpClock->grandmasterIdentity,
			announce->grandmasterIdentity);
	ptpClock->grandmasterClockQuality.clockAccuracy = 
		announce->grandmasterClockQuality.clockAccuracy;
	ptpClock->grandmasterClockQuality.clockClass = 
		announce->grandmasterClockQuality.clockClass;
	ptpClock->grandmasterClockQuality.offsetScaledLogVariance = 
		announce->grandmasterClockQuality.offsetScaledLogVariance;
	ptpClock->grandmasterPriority1 = announce->grandmasterPriority1;
	ptpClock->grandmasterPriority2 = announce->grandmasterPriority2;

	/* Timeproperties DS */
	ptpClock->currentUtcOffset = announce->currentUtcOffset;

        /* "Valid" is bit 2 in second octet of flagfield */
        ptpClock->currentUtcOffsetValid = IS_SET(header->flagField1, UTCV);

	/* set PTP_PASSIVE-specific state */
	p1(ptpClock, rtOpts);

	/* only set leap state in slave mode */
	if (ptpClock->portState == PTP_SLAVE) {
		ptpClock->leap59 = IS_SET(header->flagField1, LI59);
		ptpClock->leap61 = IS_SET(header->flagField1, LI61);
	}

        ptpClock->timeTraceable = IS_SET(header->flagField1, TTRA);
        ptpClock->frequencyTraceable = IS_SET(header->flagField1, FTRA);
        ptpClock->ptpTimescale = IS_SET(header->flagField1, PTPT);
        ptpClock->timeSource = announce->timeSource;

#if defined(MOD_TAI) &&  NTP_API == 4
	/*
	 * update kernel TAI offset, but only if timescale is
	 * PTP not ARB - spec section 7.2
	 */
        if (ptpClock->ptpTimescale &&
            (ptpClock->currentUtcOffset != previousUtcOffset)) {
		setKernelUtcOffset(ptpClock->currentUtcOffset);
        }
#endif /* MOD_TAI */

	/* Leap second handling */

        if (ptpClock->portState == PTP_SLAVE) {
		if(ptpClock->leap59 && ptpClock->leap61) {
			ERROR("Both Leap59 and Leap61 flags set!\n");
			toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		}

		/* one of the leap second flags has suddenly been unset */
		if(ptpClock->leapSecondPending && 
		    !ptpClock->leapSecondInProgress &&
		    ((previousLeap59 != ptpClock->leap59) || 
		     (previousLeap61 != ptpClock->leap61))) {
			WARNING("=== Leap second event aborted by GM!");
			ptpClock->leapSecondPending = FALSE;
			ptpClock->leapSecondInProgress = FALSE;
			timerStop(LEAP_SECOND_PAUSE_TIMER, ptpClock->itimer);
#if !defined(__APPLE__)
			unsetTimexFlags(STA_INS | STA_DEL,TRUE);
#endif /* apple */
		}

		/*
		 * one of the leap second flags has been set
		 * or flags are lit but we have no event pending
		 */
		if( (ptpClock->leap59 || ptpClock->leap61) && (
		    (!ptpClock->leapSecondPending && 
		    !ptpClock->leapSecondInProgress ) ||
		    ((!previousLeap59 && ptpClock->leap59) ||
		    (!previousLeap61 && ptpClock->leap61)))) {
#if !defined(__APPLE__)
			WARNING("=== Leap second pending! Setting kernel to %s "
				"one second at midnight\n",
				ptpClock->leap61 ? "add" : "delete");
		    if (!checkTimexFlags(ptpClock->leap61 ? STA_INS : STA_DEL)) {
			    unsetTimexFlags(ptpClock->leap61 ? STA_DEL : STA_INS,
					    TRUE);
			    setTimexFlags(ptpClock->leap61 ? STA_INS : STA_DEL,
					  FALSE);
		    }
#else
			WARNING("=== Leap second pending! No kernel leap second "
				"API support - expect a clock jump at "
				"midnight!\n");
#endif /* apple */
			/* only set the flag, the rest happens in doState() */
			ptpClock->leapSecondPending = TRUE;
		}

		if((previousUtcOffset != ptpClock->currentUtcOffset) && 
		   !ptpClock->leapSecondPending && 
		   !ptpClock->leapSecondInProgress ) {
			WARNING("=== UTC offset changed from %d to %d with "
				"no leap second pending!\n",
				previousUtcOffset, ptpClock->currentUtcOffset);
		} else if( previousUtcOffset != ptpClock->currentUtcOffset) {
			WARNING("=== UTC offset changed from %d to %d\n",
				previousUtcOffset,ptpClock->currentUtcOffset);
		}
	}
}


/*Copy local data set into header and announce message. 9.3.4 table 12*/
void copyD0(MsgHeader *header, MsgAnnounce *announce, PtpClock *ptpClock)
{
	announce->grandmasterPriority1 = ptpClock->priority1;
	copyClockIdentity(announce->grandmasterIdentity,
			ptpClock->clockIdentity);
	announce->grandmasterClockQuality.clockClass = 
		ptpClock->clockQuality.clockClass;
	announce->grandmasterClockQuality.clockAccuracy = 
		ptpClock->clockQuality.clockAccuracy;
	announce->grandmasterClockQuality.offsetScaledLogVariance = 
		ptpClock->clockQuality.offsetScaledLogVariance;
	announce->grandmasterPriority2 = ptpClock->priority2;
	announce->stepsRemoved = 0;
	copyClockIdentity(header->sourcePortIdentity.clockIdentity,
	       ptpClock->clockIdentity);
}


/*Data set comparison bewteen two foreign masters (9.3.4 fig 27)
 * return similar to memcmp() */

Integer8 
bmcDataSetComparison(MsgHeader *headerA, MsgAnnounce *announceA,
		     MsgHeader *headerB,MsgAnnounce *announceB,
		     PtpClock *ptpClock)
{
	DBGV("Data set comparison \n");
	short comp = 0;
	/*Identity comparison*/
	if (!memcmp(announceA->grandmasterIdentity,
		    announceB->grandmasterIdentity,CLOCK_IDENTITY_LENGTH)){
	  /* Algorithm part2 Fig 28 */
		if (announceA->stepsRemoved > announceB->stepsRemoved+1) {
			return 1;
		}
		else if (announceB->stepsRemoved > announceA->stepsRemoved+1) {
			return -1;
		} else { /* A within 1 of B */
			if (announceA->stepsRemoved > announceB->stepsRemoved) {
				if (!memcmp(headerA->sourcePortIdentity.clockIdentity,ptpClock->parentPortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH)) {
					DBG("Sender=Receiver : Error -1");
					return 0;
				} else {
					return 1;
				}
			} else if (announceB->stepsRemoved > 
				   announceA->stepsRemoved) {
				if (!memcmp(headerB->sourcePortIdentity.clockIdentity,ptpClock->parentPortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH)) {
					DBG("Sender=Receiver : Error -1");
					return 0;
				} else {
					return -1;
				}
			}
			else { /*  steps removed A = steps removed B */
				comp = memcmp(headerA->sourcePortIdentity.clockIdentity,headerB->sourcePortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH);
				if (!comp) {
					DBG("Sender=Receiver : Error -2");
					return 0;
				} else if (comp<0) {
					return -1;
				} else {
					return 1;
				}
			}

		}
	} else { /* GrandMaster are not identical */
		if(announceA->grandmasterPriority1 == 
		   announceB->grandmasterPriority1) {
			if (announceA->grandmasterClockQuality.clockClass == 
			    announceB->grandmasterClockQuality.clockClass) {
				if (announceA->grandmasterClockQuality.clockAccuracy == announceB->grandmasterClockQuality.clockAccuracy) {
					if (announceA->grandmasterClockQuality.offsetScaledLogVariance == announceB->grandmasterClockQuality.offsetScaledLogVariance) {
						if (announceA->grandmasterPriority2 == announceB->grandmasterPriority2)		{
							comp = memcmp(announceA->grandmasterIdentity,announceB->grandmasterIdentity,CLOCK_IDENTITY_LENGTH);
							if (comp < 0)
								return -1;
							else if (comp > 0)
								return 1;
							else
								return 0;
						} else {
/* Priority2 are not identical */
							comp =memcmp(&announceA->grandmasterPriority2,&announceB->grandmasterPriority2,1);
							if (comp < 0)
								return -1;
							else if (comp > 0)
								return 1;
							else
								return 0;
						}
					} else {
/* offsetScaledLogVariance are not identical */
						comp = announceA->grandmasterClockQuality.offsetScaledLogVariance < announceB->grandmasterClockQuality.offsetScaledLogVariance ? -1 : 1;
						if (comp < 0)
							return -1;
						else if (comp > 0)
							return 1;
						else
							return 0;
					}

				} else { /*  Accuracy are not identitcal */
					comp = memcmp(&announceA->grandmasterClockQuality.clockAccuracy,&announceB->grandmasterClockQuality.clockAccuracy,1);
					if (comp < 0)
						return -1;
					else if (comp > 0)
						return 1;
					else
						return 0;
				}
			} else { /* ClockClass are not identical */
				comp =  memcmp(&announceA->grandmasterClockQuality.clockClass,&announceB->grandmasterClockQuality.clockClass,1);
				if (comp < 0)
					return -1;
				else if (comp > 0)
					return 1;
				else
					return 0;
			}
		} else { /*  Priority1 are not identical */
			comp =  memcmp(&announceA->grandmasterPriority1,&announceB->grandmasterPriority1,1);
			if (comp < 0)
				return -1;
			else if (comp > 0)
				return 1;
			else
				return 0;
		}
	}
}

/*State decision algorithm 9.3.3 Fig 26*/
UInteger8 
bmcStateDecision(MsgHeader *header, MsgAnnounce *announce,
		 RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Integer8 comp;
	
	
	if (rtOpts->slaveOnly)	{
		s1(header,announce,ptpClock, rtOpts);
		return PTP_SLAVE;
	}

	if ((!ptpClock->number_foreign_records) && 
	    (ptpClock->portState == PTP_LISTENING))
		return PTP_LISTENING;

	copyD0(&ptpClock->msgTmpHeader,&ptpClock->msgTmp.announce,ptpClock);

	DBGV("local clockQuality.clockClass: %d \n", ptpClock->clockQuality.clockClass);
	comp = bmcDataSetComparison(&ptpClock->msgTmpHeader, &ptpClock->msgTmp.announce, header, announce, ptpClock);
	if (ptpClock->clockQuality.clockClass < 128) {
		if (comp < 0) {
			m1(rtOpts, ptpClock);
			return PTP_MASTER;
		} else if (comp > 0) {
			s1(header,announce,ptpClock, rtOpts);
			return PTP_PASSIVE;
		} else {
			DBG("Error in bmcDataSetComparison..\n");
		}
	} else {
		if (comp < 0) {
			m1(rtOpts,ptpClock);
			return PTP_MASTER;
		} else if (comp > 0) {
			s1(header,announce,ptpClock, rtOpts);
			return PTP_SLAVE;
		} else {
			DBG("Error in bmcDataSetComparison..\n");
		}
	}

	/*  MB: Is this the return code below correct? */
	/*  Anyway, it's a valid return code. */
	return PTP_FAULTY;
}



UInteger8 
bmc(ForeignMasterRecord *foreignMaster,
    RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Integer16 i,best;

	DBGV("number_foreign_records : %d \n", ptpClock->number_foreign_records);
	if (!ptpClock->number_foreign_records)
		if (ptpClock->portState == PTP_MASTER)	{
			m1(rtOpts,ptpClock);
			return ptpClock->portState;
		}

	for (i=1,best = 0; i<ptpClock->number_foreign_records;i++)
		if ((bmcDataSetComparison(&foreignMaster[i].header,
					  &foreignMaster[i].announce,
					  &foreignMaster[best].header,
					  &foreignMaster[best].announce,
					  ptpClock)) < 0)
			best = i;

	DBGV("Best record : %d \n",best);
	ptpClock->foreign_record_best = best;

	return (bmcStateDecision(&foreignMaster[best].header,
				 &foreignMaster[best].announce,
				 rtOpts,ptpClock));
}



/*

13.3.2.6, page 126

PTPv2 valid flags per packet type:

ALL:
   .... .0.. .... .... = PTP_UNICAST
SYNC+Pdelay Resp:
   .... ..0. .... .... = PTP_TWO_STEP

Announce only:
   .... .... ..0. .... = FREQUENCY_TRACEABLE
   .... .... ...0 .... = TIME_TRACEABLE
   .... .... .... 0... = PTP_TIMESCALE
   .... .... .... .0.. = PTP_UTC_REASONABLE
   .... .... .... ..0. = PTP_LI_59
   .... .... .... ...0 = PTP_LI_61

*/
