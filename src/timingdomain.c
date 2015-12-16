#include "ptpd.h"
#include "dep/ntpengine/ntpdcontrol.h"

#ifdef LOCAL_PREFIX
#undef LOCAL_PREFIX
#endif

#define LOCAL_PREFIX "TimingService"

static int cmpTimingService(TimingService *a, TimingService *b, Boolean useableOnly);
static const char* reasonToString(int reason);
static void prepareLeapFlags(RunTimeOpts *rtOpts, PtpClock *ptpClock);

static int ptpServiceInit (TimingService* service);
static int ptpServiceShutdown (TimingService* service);
static int ptpServiceAcquire (TimingService* service);
static int ptpServiceRelease (TimingService* service, int reason);
static int ptpServiceUpdate (TimingService* service);
static int ptpServiceClockUpdate (TimingService* service);

static int ntpServiceInit (TimingService* service);
static int ntpServiceShutdown (TimingService* service);
static int ntpServiceAcquire (TimingService* service);
static int ntpServiceRelease (TimingService* service, int reason);
static int ntpServiceUpdate (TimingService* service);
static int ntpServiceClockUpdate (TimingService* service);


static int timingDomainInit(TimingDomain *domain);
static int timingDomainShutdown(TimingDomain *domain);
static int timingDomainUpdate(TimingDomain *domain);

int
timingDomainSetup(TimingDomain *domain)
{
	domain->init = timingDomainInit;
	domain->shutdown = timingDomainShutdown;
	domain->update = timingDomainUpdate;
	domain->current = NULL;
	return 1;
}

int
timingServiceSetup(TimingService *service)
{

	if(service == NULL) {
		return 0;
	}

	switch (service->dataSet.type) {

	    case TIMINGSERVICE_NTP:
		    service->init = ntpServiceInit;
		    service->shutdown = ntpServiceShutdown;
		    service->acquire = ntpServiceAcquire;
		    service->release = ntpServiceRelease;
		    service->update = ntpServiceUpdate;
		    service->clockUpdate = ntpServiceClockUpdate;
		break;
	    case TIMINGSERVICE_PTP:
		    service->init = ptpServiceInit;
		    service->shutdown = ptpServiceShutdown;
		    service->acquire = ptpServiceAcquire;
		    service->release = ptpServiceRelease;
		    service->update = ptpServiceUpdate;
		    service->clockUpdate = ptpServiceClockUpdate;
		break;
	    default:
		break;
	}

	return 1;

}

static
const char*
reasonToString(int reason) {
	switch(reason) {
	
	case REASON_IDLE:
	    return("idle");
	case REASON_ELECTION:
	    return("election");
	case REASON_CTRL_NOT_BEST:
	    return("in control but not elected");
	case REASON_ELIGIBLE:
	    return("no longer eligible");
	default:
	    return "";
	}
}


/* a'la BMCA. Eventually quality estimation will get here and the fun begins */
static int
cmpTimingService(TimingService *a, TimingService *b, Boolean useableOnly)
{

	/* if called with false, only dataset is examined */
	if(useableOnly) {
	/* operational is always better */
	/* well.. for now. */
	    CMP2H((a->flags & TIMINGSERVICE_OPERATIONAL),
		(b->flags & TIMINGSERVICE_OPERATIONAL))
		/* only evaluate if the service wants to control the clock. */
		/* this is meant to select the best service that can be used right now */
	    CMP2H((a->flags & TIMINGSERVICE_AVAILABLE),
		(b->flags & TIMINGSERVICE_AVAILABLE))
	    /* should not happen really, when idle is triggered, available goes */
//	    CMP2L((a->flags & TIMINGSERVICE_IDLE),
//		(b->flags & TIMINGSERVICE_IDLE))
	}
	/* lower p1 = better */
	CMP2L(a->dataSet.priority1, b->dataSet.priority1);
	/* lower type = better */
	CMP2L((uint8_t)a->dataSet.type, (uint8_t)b->dataSet.type);
	/* lower p2 = better */
	CMP2L(a->dataSet.priority2, b->dataSet.priority2);

	/* tiebreaker needed eventually: some uuid? age? */
	/* CMP2L(a->dataSet.TBD, b->dataSet.TBD); */

	/*
	 * Son, you don't have bad luck.
	 * The reason that bad things happen to you,
	 * is because you're a dumbass
	 */
	return 1;
}

/* version suitable for quicksort */
/*
static int
cmpTimingServiceQS (void *pA, void *pB)
{

	TimingService *a = (TimingService*)pA;
	TimingService *b = (TimingService*)pB;

	return cmpTimingService(a, b, TRUE);

}
*/

static int
ptpServiceInit (TimingService* service)
{
	RunTimeOpts *rtOpts = (RunTimeOpts*)service->config;
	PtpClock *ptpClock  = (PtpClock*)service->controller;

	memset(&rtOpts->leapInfo, 0, sizeof(LeapSecondInfo));
	if(strcmp(rtOpts->leapFile,"")) {
		parseLeapFile(rtOpts->leapFile, &rtOpts->leapInfo);
	}

	/* read current UTC offset from leap file or from kernel if not configured */
	if(ptpClock->timePropertiesDS.ptpTimescale &&
	    rtOpts->timeProperties.currentUtcOffset == 0) {
		prepareLeapFlags(rtOpts, ptpClock);
	}

	INFO_LOCAL_ID(service, "PTP service init\n");

	return 1;
}

static int
ptpServiceShutdown (TimingService* service)
{
	PtpClock *ptpClock  = (PtpClock*)service->controller;
	INFO_LOCAL_ID(service,"PTP service shutdown\n");
        ptpdShutdown(ptpClock);
	return 1;
}

static int
ptpServiceAcquire (TimingService* service)
{
//	RunTimeOpts *rtOpts = (RunTimeOpts*)service->config;
	PtpClock *ptpClock  = (PtpClock*)service->controller;
	ptpClock->clockControl.granted = TRUE;
	INFO_LOCAL_ID(service,"acquired clock control\n");
        FLAGS_SET(service->flags, TIMINGSERVICE_IN_CONTROL);
	return 1;
}

static int
ptpServiceRelease (TimingService* service, int reason)
{
//	RunTimeOpts *rtOpts = (RunTimeOpts*)service->config;
	PtpClock *ptpClock  = (PtpClock*)service->controller;
	ptpClock->clockControl.granted = FALSE;
	if(!service->released) INFO_LOCAL_ID(service,"released clock control, reason: %s\n",
		reasonToString(reason));
        FLAGS_UNSET(service->flags, TIMINGSERVICE_IN_CONTROL);
	return 1;
}

/*
 * configure the UTC offset and leap flags according to
 * information from kernel or leap file. Note: no updates to ptpClock.
 * only clockStatus is being picked up in protocol.c
 */
static void
prepareLeapFlags(RunTimeOpts *rtOpts, PtpClock *ptpClock) {

	TimeInternal now;
	Boolean leapInsert = FALSE, leapDelete = FALSE;
#ifdef HAVE_SYS_TIMEX_H
	int flags;

	/* first get the offset from kernel if we can */
#if defined(MOD_TAI) &&  NTP_API == 4
	int utcOffset;

	if(getKernelUtcOffset(&utcOffset) && utcOffset != 0) {
	    ptpClock->clockStatus.utcOffset = utcOffset;
	}
#endif /* MOD_TAI */

	flags= getTimexFlags();

	leapInsert = ((flags & STA_INS) == STA_INS);
	leapDelete = ((flags & STA_DEL) == STA_DEL);

#endif 	/* HAVE_SYS_TIMEX_H  */

	getTime(&now);


	ptpClock->clockStatus.override = FALSE;

	    /* then we try the offset from leap file if valid - takes priority over kernel */
	    if(rtOpts->leapInfo.offsetValid) {
		    ptpClock->clockStatus.utcOffset =
			rtOpts->leapInfo.currentOffset;
		    ptpClock->clockStatus.override = TRUE;
	    }

	    /* if we have valid leap second info from leap file, we use it */
	    if(rtOpts->leapInfo.valid) {

		ptpClock->clockStatus.leapInsert = FALSE;
		ptpClock->clockStatus.leapDelete = FALSE;

		if( now.seconds >= rtOpts->leapInfo.startTime &&
		    now.seconds < rtOpts->leapInfo.endTime) {
			DBG("Leap second pending - leap file\n");
			if(rtOpts->leapInfo.leapType == 1) {
			    ptpClock->clockStatus.leapInsert = TRUE;
			}
			if(rtOpts->leapInfo.leapType == -1) {
			    ptpClock->clockStatus.leapDelete = TRUE;
			}
			
			ptpClock->clockStatus.override = TRUE;

		}
		 if(now.seconds >= rtOpts->leapInfo.endTime) {
		    ptpClock->clockStatus.utcOffset =
			    rtOpts->leapInfo.nextOffset;
		    	    ptpClock->clockStatus.override = TRUE;
		    if(strcmp(rtOpts->leapFile,"")) {
			memset(&rtOpts->leapInfo, 0, sizeof(LeapSecondInfo));
			parseLeapFile(rtOpts->leapFile, &rtOpts->leapInfo);

			if(rtOpts->leapInfo.offsetValid) {
				ptpClock->clockStatus.utcOffset =
				rtOpts->leapInfo.currentOffset;

			}

		    }

		}
	    /* otherwise we try using the kernel info, but not when we're slave */
	    } else if(ptpClock->portDS.portState != PTP_SLAVE) {
		    ptpClock->clockStatus.leapInsert = leapInsert;
		    ptpClock->clockStatus.leapDelete = leapDelete;
	    }
	   
}

static int
ptpServiceUpdate (TimingService* service)
{
	RunTimeOpts *rtOpts = (RunTimeOpts*)service->config;
	PtpClock *ptpClock  = (PtpClock*)service->controller;

	ptpClock->counters.messageSendRate = ptpClock->netPath.sentPackets / service->updateInterval;
	ptpClock->counters.messageReceiveRate = ptpClock->netPath.receivedPackets / service->updateInterval;

	ptpClock->netPath.sentPackets = 0;
	ptpClock->netPath.receivedPackets = 0;

	if(service->reloadRequested && strcmp(rtOpts->leapFile,"")) {
		memset(&rtOpts->leapInfo, 0, sizeof(LeapSecondInfo));
		parseLeapFile(rtOpts->leapFile, &rtOpts->leapInfo);
		service->reloadRequested = FALSE;
	}

	/* read current UTC offset from leap file or from kernel if not configured */
	if(ptpClock->timePropertiesDS.ptpTimescale && ( ptpClock->portDS.portState == PTP_SLAVE ||
	    (ptpClock->portDS.portState == PTP_MASTER && rtOpts->timeProperties.currentUtcOffset == 0))) {
		prepareLeapFlags(rtOpts, ptpClock);
	}

	/* temporary: this is only to maintain PTPd's current config options */
	/* if NTP failover disabled, pretend PTP always owns the clock */
	if(!rtOpts->ntpOptions.enableFailover) {
		FLAGS_SET(service->flags, TIMINGSERVICE_OPERATIONAL);
		if(ptpClock->clockControl.available) {
			ptpClock->clockControl.granted = TRUE;
		}
		service->activity = TRUE;
		FLAGS_SET(service->flags, TIMINGSERVICE_AVAILABLE);
		return 1;
	}

	/* keep the activity heartbeat in check */
	if(ptpClock->clockControl.activity) {
		service->activity = TRUE;
		ptpClock->clockControl.activity = FALSE;
		DBGV("TimingService %s activity seen\n", service->id);
	} else {
		service->activity = FALSE;
	}

	/* initializing or faulty: not operational */
	if((ptpClock->portDS.portState == PTP_INITIALIZING) ||
	    (ptpClock->portDS.portState == PTP_FAULTY)) {
		FLAGS_UNSET(service->flags, TIMINGSERVICE_OPERATIONAL);
	} else {
		FLAGS_SET(service->flags, TIMINGSERVICE_OPERATIONAL);
		DBGV("TimingService %s is operational\n", service->id);
	}

	/* not slave: release control or start hold timer */
	if(ptpClock->portDS.portState != PTP_SLAVE) {
		FLAGS_UNSET(service->flags, TIMINGSERVICE_IDLE);
		if(service->flags & TIMINGSERVICE_HOLD) {
		    if((service->holdTimeLeft)<=0) {
			FLAGS_UNSET(service->flags, TIMINGSERVICE_AVAILABLE);
			FLAGS_UNSET(service->flags, TIMINGSERVICE_HOLD);

			ptpClock->clockControl.available = FALSE;
			if(service->holdTime) {
			    INFO_LOCAL_ID(service,"hold time expired\n");
			}
		    }
		} else if(ptpClock->clockControl.available) {
		        FLAGS_SET(service->flags, TIMINGSERVICE_HOLD);
			/* if we're already in hold time, don't restart it */
			if(service->holdTimeLeft <=0) {
			    service->holdTimeLeft = service->holdTime;
			}
		        if(service->holdTimeLeft>0) {
		    	    DBG_LOCAL_ID(service,"hold started - %d seconds\n",service->holdTimeLeft);
			}
		}
	} else {
	/* slave: ready to acquire clock control, hold time over */
		if(ptpClock->clockControl.available) {
		    if(!(service->flags & TIMINGSERVICE_AVAILABLE)) {

			INFO_LOCAL_ID(service,"now available\n");
			FLAGS_SET(service->flags, TIMINGSERVICE_AVAILABLE);

		    }

		    if(service->flags & TIMINGSERVICE_HOLD) {
			DBG_LOCAL_ID(service, "hold cancelled\n");
			FLAGS_UNSET(service->flags, TIMINGSERVICE_HOLD);
			service->holdTimeLeft = 0;
		    }

		    DBGV("TimingService %s available for clock control\n", service->id);
		} else {
		    FLAGS_UNSET(service->flags, TIMINGSERVICE_AVAILABLE);
		}
	}

	/* if we're idle, release clock control */
	if((service->flags & TIMINGSERVICE_IDLE) && (service->holdTimeLeft <= 0)) {
		ptpClock->clockControl.available = FALSE;
		FLAGS_UNSET(service->flags, TIMINGSERVICE_AVAILABLE);
	}

	return 1;
}

static int
ptpServiceClockUpdate (TimingService* service)
{
	RunTimeOpts *rtOpts = (RunTimeOpts*)service->config;
	PtpClock *ptpClock  = (PtpClock*)service->controller;

	TimeInternal newTime, oldTime;

#ifdef HAVE_SYS_TIMEX_H
	int flags = getTimexFlags();

	Boolean leapInsert = flags & STA_INS;
	Boolean leapDelete = flags & STA_DEL;
	Boolean inSync = !(flags & STA_UNSYNC);
#endif /* HAVE_SYS_TIMEX_H */

	ClockStatusInfo *clockStatus = &ptpClock->clockStatus;


	if(!clockStatus->update) {
		return 0;
	}

	DBG_LOCAL_ID(service, "clock status update\n");

#if defined(MOD_TAI) &&  NTP_API == 4
	setKernelUtcOffset(clockStatus->utcOffset);

	DBG_LOCAL_ID(service,"Set kernel UTC offset to %d\n",
	    clockStatus->utcOffset);
#endif

#ifdef HAVE_SYS_TIMEX_H

	if(clockStatus->inSync & !inSync) {
	    clockStatus->inSync = FALSE;
            unsetTimexFlags(STA_UNSYNC, TRUE);
	}

	    informClockSource(ptpClock);

    if(rtOpts->leapSecondHandling == LEAP_ACCEPT) {
	/* withdraw kernel flags if needed, unless this is during the event */
	if(!ptpClock->leapSecondInProgress) {
	    if(leapInsert && !clockStatus->leapInsert) {
		DBG_LOCAL_ID(service,"STA_INS in kernel but not in PTP: withdrawing\n");
		unsetTimexFlags(STA_INS, TRUE);
	    }

	    if(leapDelete && !clockStatus->leapDelete) {
		DBG_LOCAL_ID(service,"STA_DEL in kernel but not in PTP: withdrawing\n");
		unsetTimexFlags(STA_DEL, TRUE);
	    }
	}

	if(!leapInsert && clockStatus->leapInsert) {
		WARNING("Leap second pending! Setting clock to insert one second at midnight\n");
		setTimexFlags(STA_INS, FALSE);
	}

	if(!leapDelete && clockStatus->leapDelete) {
		WARNING("Leap second pending! Setting clock to delete one second at midnight\n");
		setTimexFlags(STA_DEL, FALSE);
	}
    } else {
	if(leapInsert || leapDelete) {
		DBG("leap second handling is not set to accept - withdrawing kernel leap flags!\n");
		unsetTimexFlags(STA_INS, TRUE);
		unsetTimexFlags(STA_DEL, TRUE);
	}
    }
#else
	if(clockStatus->leapInsert || clockStatus->leapDelete) {
		if(rtOpts->leapSecondHandling != LEAP_SMEAR) {
		    WARNING("Leap second pending! No kernel leap second "
			"API support - expect a clock offset at "
			"midnight!\n");
		}
	}
#endif /* HAVE_SYS_TIMEX_H */

	getTime(&oldTime);
	subTime(&newTime, &oldTime, &ptpClock->currentDS.offsetFromMaster);

	/* Major time change */
	if(clockStatus->majorChange){
	    /* re-parse leap seconds file */
	    if(strcmp(rtOpts->leapFile,"")) {
		memset(&rtOpts->leapInfo, 0, sizeof(LeapSecondInfo));
		parseLeapFile(rtOpts->leapFile, &rtOpts->leapInfo);
	    }
#ifdef HAVE_LINUX_RTC_H
	    if(rtOpts->setRtc) {
		NOTICE_LOCAL_ID(service, "Major time change - syncing the RTC\n");
		setRtc(&newTime);
		clockStatus->majorChange = FALSE;
	    }
#endif /* HAVE_LINUX_RTC_H */
	    /* need to inform utmp / wtmp */
	    if(oldTime.seconds != newTime.seconds) {
		updateXtmp(oldTime, newTime);
	    }
	}

	ptpClock->clockStatus.update = FALSE;
	return 1;
}

static int
ntpServiceInit (TimingService* service)
{
	NTPoptions *config = (NTPoptions*) service->config;
	NTPcontrol *controller = (NTPcontrol*) service->controller;

	INFO_LOCAL_ID(service,"NTP service init\n");

	if(!config->enableEngine) {
	    INFO_LOCAL_ID(service,"NTP service not enabled\n");
	    FLAGS_UNSET(service->flags, TIMINGSERVICE_OPERATIONAL);
	    FLAGS_UNSET(service->flags, TIMINGSERVICE_AVAILABLE);
	    return 1;
	}

	if(ntpInit(config, controller)) {
	    FLAGS_SET(service->flags, TIMINGSERVICE_OPERATIONAL);
	    INFO_LOCAL_ID(service,"NTP service started\n");
	    return 1;
	} else {
	    FLAGS_UNSET(service->flags, TIMINGSERVICE_OPERATIONAL);
	    FLAGS_UNSET(service->flags, TIMINGSERVICE_AVAILABLE);
	    INFO_LOCAL_ID(service,"Could not start NTP service. Will keep retrying.\n");
	    return 0;
	}

}

static int
ntpServiceShutdown (TimingService* service)
{
	NTPoptions *config = (NTPoptions*) service->config;
	NTPcontrol *controller = (NTPcontrol*) service->controller;

	INFO_LOCAL_ID(service,"NTP service shutting down\n");

	if(controller->flagsCaptured) {
	    INFO_LOCAL_ID(service,"Restoring original NTP state\n");
	    ntpShutdown(config, controller);
	}

	FLAGS_UNSET(service->flags, TIMINGSERVICE_OPERATIONAL);
	FLAGS_UNSET(service->flags, TIMINGSERVICE_AVAILABLE);
	return 1;
}

static int
ntpServiceAcquire (TimingService* service)
{
	NTPoptions *config = (NTPoptions*) service->config;
	NTPcontrol *controller = (NTPcontrol*) service->controller;

	if(!config->enableControl) {
		if (!controller->requestFailed) {
		    WARNING_LOCAL_ID(service, "control disabled, cannot acquire clock control\n");
		}
		controller->requestFailed = TRUE;
		return 1;
	}

	switch(ntpdSetFlags(config, controller, SYS_FLAG_KERNEL | SYS_FLAG_NTP)) {
		case INFO_OKAY:
			FLAGS_SET(service->flags, TIMINGSERVICE_IN_CONTROL);
			controller->requestFailed = FALSE;
			INFO_LOCAL_ID(service, "acquired clock control\n");
			return 1;
		default:
			if (!controller->requestFailed) {
			    WARNING_LOCAL_ID(service,"failed to acquire clock control - clock may drift!\n");
			}
			controller->requestFailed = TRUE;
			return 0;
	}

}

static int
ntpServiceRelease (TimingService* service, int reason)
{
	NTPoptions *config = (NTPoptions*) service->config;
	NTPcontrol *controller = (NTPcontrol*) service->controller;

	int res = 0;

	if(!config->enableControl) {
		if (!controller->requestFailed) {
		    WARNING_LOCAL_ID(service, "control disabled, cannot release clock control\n");
		}
		controller->requestFailed = TRUE;
		return 1;
	}

	res = ntpdClearFlags(config, controller, SYS_FLAG_KERNEL | SYS_FLAG_NTP);

	switch(res) {
		case INFO_OKAY:
			controller->requestFailed = FALSE;
			if(!service->released) INFO_LOCAL_ID(service, "released clock control, reason: %s\n",
				reasonToString(reason));
			FLAGS_UNSET(service->flags, TIMINGSERVICE_IN_CONTROL);
			return 1;
		default:
			if(!controller->requestFailed) {
			    WARNING_LOCAL_ID(service,"failed to release clock control, reason: %s - clock may be unstable!\n",
				reasonToString(reason));
			}
			controller->requestFailed = TRUE;
			return 0;
	}

}

static int
ntpServiceUpdate (TimingService* service)
{

	int res;

	NTPoptions *config = (NTPoptions*) service->config;
	NTPcontrol *controller = (NTPcontrol*) service->controller;

	if(!config->enableEngine) {
		return 0;
	}

	res = ntpdInControl(config, controller);

	if (res != INFO_YES && res != INFO_NO) {
		if(!controller->checkFailed) {
		    WARNING_LOCAL_ID(service,"Could not verify NTP status  - will keep checking\n");
		}
		controller->checkFailed = TRUE;
		FLAGS_UNSET(service->flags, TIMINGSERVICE_OPERATIONAL);
		FLAGS_UNSET(service->flags, TIMINGSERVICE_AVAILABLE);
		return 0;
	}

	FLAGS_SET(service->flags, TIMINGSERVICE_OPERATIONAL);

	if(service->flags & TIMINGSERVICE_AVAILABLE) {
	    service->activity = TRUE;
	} else {
	    controller->checkFailed = FALSE;
	    INFO_LOCAL_ID(service,"now available\n");
	    FLAGS_SET(service->flags, TIMINGSERVICE_AVAILABLE);
        }

	/*
	 * notify the service that we are in control,
	 * so that the watchdog can react if we are and it wasn't granted
	 */
	if (res == INFO_YES) {
	    FLAGS_SET(service->flags, TIMINGSERVICE_IN_CONTROL);
	}

	controller->checkFailed = FALSE;

	return 1;
}

static int
ntpServiceClockUpdate (TimingService* service)
{
	return 1;
}

static int
timingDomainInit(TimingDomain *domain)
{
	int i = 0;

	TimingService *service;

	DBG("Timing domain init\n");

	for(i=0; i < domain->serviceCount; i++) {
		service = domain->services[i];
		timingServiceSetup(service);
		service->init(service);
		service->parent = domain;
	}

	domain->current = NULL;
	domain->best = NULL;
	domain->preferred = NULL;

	return 1;

}

static int
timingDomainShutdown(TimingDomain *domain)
{
	int i = 0;

	TimingService *service;

	INFO_LOCAL("Timing domain shutting down\n");

	for(i=domain->serviceCount - 1; i >= 0; i--) {
		service = domain->services[i];
		if(service != NULL) {
		    service->shutdown(service);
		}
	}

	INFO_LOCAL("Timing domain shutdown complete\n");

	return 1;
}

static int
timingDomainUpdate(TimingDomain *domain)
{

	int i = 0;
	int cmp = 0;

	TimingService *best = NULL;
	TimingService *preferred = NULL;
	TimingService *service = NULL;


	/* update the election delay timer */	
	if(domain->electionLeft > 0) {
		if(domain->electionLeft == domain->electionDelay) {
			NOTIFY_LOCAL("election hold timer started: %d seconds\n", domain->electionDelay);
		}
		domain->electionLeft -= domain->updateInterval;
		DBG_LOCAL("electionLeft %d\n", domain->electionLeft);
	}

	DBGV("Timing domain update\n");

	if(domain->serviceCount < 1) {
	    DBG("No TimingServices in TimingDomain: nothing to do\n");
	    return 0;
	}

	/* first pass: check if alive, update idle times, release where needed */
	for(i=0; i < domain->serviceCount; i++) {

		service = domain->services[i];

		service->lastUpdate += domain->updateInterval;

		/* decrement the hold time counter */
		if(service->holdTimeLeft > 0) {
		    service->holdTimeLeft -= domain->updateInterval;
		    DBG_LOCAL_ID(service, "hold time left %d\n", service->holdTimeLeft);
		} else {
		    service->holdTimeLeft = 0;
		}

		/* each TimingService can have a different update interval: skip when not due */
		if(service->lastUpdate >= service->updateInterval) {
			service->updateDue = TRUE;
			service->lastUpdate = 0;
		} else {
		    continue;
		}

		DBG("TimingService %s due for update\n", service->id);
		service->update(service);


		DBGV("Service %s flags %03x\n",service->id, service->flags);

		/* update idle times for operational services */
		if(service->flags & TIMINGSERVICE_OPERATIONAL) {

			if(!service->activity) {
				service->idleTime += domain->updateInterval;
			} else {
				if(service->flags & TIMINGSERVICE_IDLE)
					INFO_LOCAL_ID(service,"no longer idle\n");
				FLAGS_UNSET(service->flags, TIMINGSERVICE_IDLE);
				service->idleTime = 0;
				if((service->holdTimeLeft>0) && !(service->flags & TIMINGSERVICE_HOLD)) {
				    service->holdTimeLeft = 0;
				}
			}

			service->activity = FALSE;

			if( (service->flags & TIMINGSERVICE_AVAILABLE) &&
			    !(service->flags & TIMINGSERVICE_HOLD) &&
				    (service->idleTime > service->minIdleTime) &&
				(service->idleTime > service->timeout)) {

				service->idleTime = 0;
				if(!(service->flags & TIMINGSERVICE_IDLE)) {
					INFO_LOCAL_ID(service, "has gone idle\n");
					if(service == domain->current) {
					    service->holdTimeLeft = service->holdTime;
					}
				}

				if(service->holdTimeLeft <= 0) {
				//	FLAGS_UNSET(service->flags, TIMINGSERVICE_AVAILABLE);
				}

				FLAGS_SET(service->flags, TIMINGSERVICE_IDLE);
					if((service == domain->current) &&
					    (service->holdTimeLeft <= 0)) {
						INFO_LOCAL_ID(service,"idle time hold start\n");
						service->release(service, REASON_IDLE);
						service->released = TRUE;
						domain->electionLeft = domain->electionDelay;
						domain->current = NULL;
					   
					}

			}
		}

		/* inactive or inoperational and in control: release */
		if((!(service->flags & TIMINGSERVICE_AVAILABLE) ||
		    !(service->flags & TIMINGSERVICE_OPERATIONAL)) &&
		    (service->flags & TIMINGSERVICE_IN_CONTROL) ) {
			if(service->holdTimeLeft <= 0) {
			    service->release(service, REASON_ELIGIBLE);
			    service->released = TRUE;
			    if(service == domain->current) {
				domain->electionLeft = domain->electionDelay;
				domain->current = NULL;
			    }
			}
		}

	}

	/* second pass: elect the best service, establish preferred */
	best = domain->services[0];
	preferred = domain->services[0];	

	for(i=0; i < domain->serviceCount; i++) {
		service = domain->services[i];
		cmp = cmpTimingService(service, best, TRUE);
		DBG("TimingService %s vs. best %s: cmp %d\n", service->id, best->id, cmp);
		if(cmp >= 0) {
			best = service;
		}

		if(cmpTimingService(service, preferred, FALSE) > 0) {
			preferred = service;
		}

	}

	/* best service, does not mean best selected */
	domain->best = best;

	/* best from DS perspective only */
	domain->preferred = preferred;

	/* best has changed - release previous, but let it know about the new best */
	if((best != domain->current) && (domain->electionLeft == 0)) {
		/* here used as temp variable */
		service = domain->current;

		/* release previous if we had one, kick off election hold */
		if (service != NULL && (service->holdTimeLeft <= 0)) {
			service->release(service, REASON_ELECTION);
			service->released = TRUE;
			domain->electionLeft = domain->electionDelay;
			domain->current = NULL;
			/* this way the election is 1 check interval minimum */
			return 1;
		}

		domain->current = best;

		if(FLAGS_ARESET(best->flags, TIMINGSERVICE_OPERATIONAL | TIMINGSERVICE_AVAILABLE)) {
		    NOTIFY_LOCAL_ID(best,"elected best TimingService\n");
		} else {
		    domain->current  = NULL;
		    domain->best = NULL;
		}
	}

	/* third pass: release services in control which aren't best (can happen) */
	for(i=0; i < domain->serviceCount; i++) {
		service = domain->services[i];
		if((service != domain->current) && (service->flags & TIMINGSERVICE_IN_CONTROL)) {
			if (service->updateDue) {
				if(service->holdTimeLeft <= 0) {
				    service->release(service, REASON_CTRL_NOT_BEST);
				    service->released = FALSE;
				}
			}
		}
	}

	/* fourth pass: do the sums */

	domain->availableCount = 0;
	domain->operationalCount = 0;
	domain->idleCount = 0;
	domain->controlCount = 0;

	for(i=0; i < domain->serviceCount; i++) {

		service = domain->services[i];

		if(service->flags & TIMINGSERVICE_OPERATIONAL)
		    domain->operationalCount++;

		if(service->flags & TIMINGSERVICE_AVAILABLE)
		    domain->availableCount++;

		if(service->flags & TIMINGSERVICE_IDLE)
		    domain->idleCount++;

		if(service->flags & TIMINGSERVICE_IN_CONTROL)
		    domain->controlCount++;

	}

	/* special case, aye */
	if((domain->serviceCount == 1) && (domain->current == NULL)) {
		domain->best = NULL;
	}

	/* clear updateDue */
	for(i=0; i < domain->serviceCount; i++) {
		domain->services[i]->updateDue = FALSE;
	}

	if(best == NULL) {
		if (!domain->noneAvailable)
		    WARNING_LOCAL("No TimingService available\n");
		domain->noneAvailable = TRUE;
		domain->current = NULL;
		return 0;
	}

	if(!(best->flags & TIMINGSERVICE_OPERATIONAL)) {
		if (!domain->noneAvailable)
		    WARNING_LOCAL("No operational TimingService available\n");
		domain->noneAvailable = TRUE;
		domain->current = NULL;
		return 0;
	}

	if(!(best->flags & TIMINGSERVICE_AVAILABLE)) {
		if (!domain->noneAvailable)
		    WARNING_LOCAL("No TimingService available for clock sync\n");
		domain->noneAvailable = TRUE;
		domain->current = NULL;
		return 0;
	}

	domain->noneAvailable = FALSE;

	if(!(best->flags & TIMINGSERVICE_IN_CONTROL) && domain->electionLeft == 0) {
		best->released = FALSE;
		best->acquire(best);
	}

	/* update clock source info */
	if(best->flags & TIMINGSERVICE_IN_CONTROL) {
	    best->clockUpdate(best);
	}

	return 1;
}

