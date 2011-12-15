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
		rtOpts->clockQuality.clockClass = 255;

/* Port configuration data set */

	/*
	 * PortIdentity Init (portNumber = 1 for an ardinary clock spec
	 * 7.5.2.3)
	 */
	memcpy(ptpClock->portIdentity.clockIdentity,ptpClock->clockIdentity,
	       CLOCK_IDENTITY_LENGTH);
	ptpClock->portIdentity.portNumber = NUMBER_PORTS;

	ptpClock->logMinDelayReqInterval = DEFAULT_DELAYREQ_INTERVAL;

	ptpClock->peerMeanPathDelay.seconds = 0;
	ptpClock->peerMeanPathDelay.nanoseconds = 0;

	ptpClock->logAnnounceInterval = rtOpts->announceInterval;
	ptpClock->announceReceiptTimeout = DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
	ptpClock->logSyncInterval = rtOpts->syncInterval;
	ptpClock->delayMechanism = DEFAULT_DELAY_MECHANISM;
	ptpClock->logMinPdelayReqInterval = DEFAULT_PDELAYREQ_INTERVAL;
	ptpClock->versionNumber = VERSION_PTP;

 	/*
	 * Initialize seed for random number used with Announce Timeout
	 * (spec 9.2.6.11)
	 */
	srand(time(NULL));
	ptpClock->R = getRand();

	/*Init other stuff*/
	ptpClock->number_foreign_records = 0;
  	ptpClock->max_foreign_records = rtOpts->max_foreign_records;
}


/*Local clock is becoming Master. Table 13 (9.3.5) of the spec.*/
void m1(PtpClock *ptpClock)
{
	/*Current data set update*/
	ptpClock->stepsRemoved = 0;
	ptpClock->offsetFromMaster.nanoseconds = 0;
	ptpClock->offsetFromMaster.seconds = 0;
	ptpClock->meanPathDelay.nanoseconds = 0;
	ptpClock->meanPathDelay.seconds = 0;

	/*Parent data set*/
	memcpy(ptpClock->parentPortIdentity.clockIdentity,
	       ptpClock->clockIdentity,CLOCK_IDENTITY_LENGTH);
	ptpClock->parentPortIdentity.portNumber = 0;
	ptpClock->parentStats = DEFAULT_PARENTS_STATS;
	ptpClock->observedParentClockPhaseChangeRate = 0;
	ptpClock->observedParentOffsetScaledLogVariance = 0;
	memcpy(ptpClock->grandmasterIdentity,ptpClock->clockIdentity,
	       CLOCK_IDENTITY_LENGTH);
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
}


/*Local clock is synchronized to Ebest Table 16 (9.3.5) of the spec*/
void s1(MsgHeader *header,MsgAnnounce *announce,PtpClock *ptpClock)
{
	/* Current DS */
	ptpClock->stepsRemoved = announce->stepsRemoved + 1;

	/* Parent DS */
	memcpy(ptpClock->parentPortIdentity.clockIdentity,
	       header->sourcePortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH);
	ptpClock->parentPortIdentity.portNumber = 
		header->sourcePortIdentity.portNumber;
	memcpy(ptpClock->grandmasterIdentity,announce->grandmasterIdentity,
	       CLOCK_IDENTITY_LENGTH);
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
	ptpClock->currentUtcOffsetValid = ((header->flagField[1] & 0x04) == 
					   0x04); 

	ptpClock->leap59 = ((header->flagField[1] & 0x02) == 0x02);
	ptpClock->leap61 = ((header->flagField[1] & 0x01) == 0x01);
	ptpClock->timeTraceable = ((header->flagField[1] & 0x10) == 0x10);
	ptpClock->frequencyTraceable = ((header->flagField[1] & 0x20) == 0x20);
	ptpClock->ptpTimescale = ((header->flagField[1] & 0x08) == 0x08);
	ptpClock->timeSource = announce->timeSource;
}


/*Copy local data set into header and announce message. 9.3.4 table 12*/
void copyD0(MsgHeader *header, MsgAnnounce *announce, PtpClock *ptpClock)
{
	announce->grandmasterPriority1 = ptpClock->priority1;
	memcpy(announce->grandmasterIdentity,ptpClock->clockIdentity,
	       CLOCK_IDENTITY_LENGTH);
	announce->grandmasterClockQuality.clockClass = 
		ptpClock->clockQuality.clockClass;
	announce->grandmasterClockQuality.clockAccuracy = 
		ptpClock->clockQuality.clockAccuracy;
	announce->grandmasterClockQuality.offsetScaledLogVariance = 
		ptpClock->clockQuality.offsetScaledLogVariance;
	announce->grandmasterPriority2 = ptpClock->priority2;
	announce->stepsRemoved = 0;
	memcpy(header->sourcePortIdentity.clockIdentity,
	       ptpClock->clockIdentity,CLOCK_IDENTITY_LENGTH);
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
				if (!memcmp(headerA->sourcePortIdentity.clockIdentity,headerB->sourcePortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH)) {
					DBG("Sender=Receiver : Error -2");
					return 0;
				} else if ((memcmp(headerA->sourcePortIdentity.clockIdentity,
						   headerB->sourcePortIdentity.clockIdentity,
						   CLOCK_IDENTITY_LENGTH))<0) {
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
						comp= memcmp(&announceA->grandmasterClockQuality.clockClass,&announceB->grandmasterClockQuality.clockClass,1);
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
	if (rtOpts->slaveOnly)	{
		s1(header,announce,ptpClock);
		return PTP_SLAVE;
	}

	if ((!ptpClock->number_foreign_records) && 
	    (ptpClock->portState == PTP_LISTENING))
		return PTP_LISTENING;

	copyD0(&ptpClock->msgTmpHeader,&ptpClock->msgTmp.announce,ptpClock);

	if (ptpClock->clockQuality.clockClass < 128) {
		if ((bmcDataSetComparison(&ptpClock->msgTmpHeader,
					  &ptpClock->msgTmp.announce,
					  header,announce,ptpClock)<0)) {
			m1(ptpClock);
			return PTP_MASTER;
		} else if ((bmcDataSetComparison(&ptpClock->msgTmpHeader,
						 &ptpClock->msgTmp.announce,
						 header,announce,ptpClock)>0)) {
			s1(header,announce,ptpClock);
			return PTP_PASSIVE;
		} else {
			DBG("Error in bmcDataSetComparison..\n");
		}
	} else {
		if ((bmcDataSetComparison(&ptpClock->msgTmpHeader,
					  &ptpClock->msgTmp.announce,
					  header,announce,ptpClock))<0) {
			m1(ptpClock);
			return PTP_MASTER;
		} else if ((bmcDataSetComparison(&ptpClock->msgTmpHeader,
						 &ptpClock->msgTmp.announce,
						 header,announce,ptpClock)>0)) {
			s1(header,announce,ptpClock);
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

	if (!ptpClock->number_foreign_records)
		if (ptpClock->portState == PTP_MASTER)	{
			m1(ptpClock);
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


