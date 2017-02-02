/*-
 * Copyright (c) 2014-2015 Wojciech Owczarek,
 *                         George V. Neville-Neil
 * Copyright (c) 2012-2013 George V. Neville-Neil,
 *                         Wojciech Owczarek.
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
 * @file   net.c
 * @date   Tue Jul 20 16:17:49 2010
 *
 * @brief  Functions to interact with the network sockets and NIC driver.
 *
 *
 */

#include "../ptpd.h"

#if defined PTPD_SNMP
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#endif

#include "cck_glue.h"
#include <libcck/net_utils.h>
#include <libcck/net_utils_linux.h>

ssize_t
netSendEvent(Octet * buf, UInteger16 length, PtpClock *ptpClock, void *dst, TimeInternal *timestamp)
{

	ssize_t ret = 0;

	TTransport *tr = ptpClock->eventTransport;
	CckTransportAddress *addr = ptpClock->eventDestination;

	if(dst) {
	    addr = dst;
	    setTransportAddressPort(addr, PTP_EVENT_PORT);
	}

	tr->outgoingMessage.destination = addr;
	tr->outgoingMessage.length = length;

	ret =  tr->sendMessage(tr, &tr->outgoingMessage);

	if(tr->outgoingMessage.hasTimestamp) {
	    timestamp->seconds = tr->outgoingMessage.timestamp.seconds;
	    timestamp->nanoseconds = tr->outgoingMessage.timestamp.nanoseconds;
	} else {
	    clearTime(timestamp);
	}

	return ret;
}

ssize_t
netSendGeneral(Octet * buf, UInteger16 length, PtpClock *ptpClock, void *dst)
{

	TTransport *tr = (TTransport*) ptpClock->generalTransport;

	CckTransportAddress *addr = ptpClock->generalDestination;

	tr->outgoingMessage.destination = dst ? dst : addr;
	tr->outgoingMessage.length = length;

	return tr->sendMessage(tr, &tr->outgoingMessage);

}

Boolean
prepareClockDrivers(PtpClock *ptpClock) {

	RunTimeOpts *rtOpts = ptpClock->rtOpts;

	TTransport *ev = ptpClock->eventTransport;

	ClockDriver *cd = ev->getClockDriver(ev);

	configureClockDriver(getSystemClock(), rtOpts);

	ptpClock->clockDriver = cd;

	configureClockDriver(cd, rtOpts);
	cd->inUse = TRUE;
	cd->config.required = TRUE;

	if(ptpClock->portDS.portState == PTP_SLAVE) {
	    cd->setExternalReference(ptpClock->clockDriver, "PTP", RC_PTP);
	    cd->owner = ptpClock;
	    cd->callbacks.onStep = myPtpClockStepNotify;
	    cd->callbacks.onLock = myPtpClockLocked;
	}

	ClockDriver *lastMaster = ptpClock->masterClock;

	if(strlen(rtOpts->masterClock) > 0) {
	    ptpClock->masterClock = getClockDriverByName(rtOpts->masterClock);
	    if(ptpClock->masterClock == NULL) {
		WARNING("Could not find designated master clock: %s\n", rtOpts->masterClock);
	    }
	} else {
	    ptpClock->masterClock = NULL;
	}

	if((lastMaster) && (lastMaster != ptpClock->masterClock)) {
	    lastMaster->setReference(lastMaster, NULL);
	    lastMaster->setState(lastMaster, CS_FREERUN);
	}

	if((ptpClock->masterClock) && (ptpClock->masterClock != ptpClock->clockDriver)) {
	    if( (ptpClock->portDS.portState == PTP_MASTER) || (ptpClock->portDS.portState == PTP_PASSIVE)) {
		((ClockDriver*)ptpClock->masterClock)->setExternalReference(ptpClock->masterClock, "PREFMST", RC_EXTERNAL);
	    }
	}

	cd->callbacks.onUpdate = myPtpClockUpdate;

	return TRUE;


}

