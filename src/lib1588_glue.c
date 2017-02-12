/*-
 * Copyright (c) 2016      Wojciech Owczarek
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
 * @file   lib1588_glue.c
 * @date   Thu Nov 24 23:19:00 2016
 *
 * @brief  Wrappers binding lib1588 protocol code with LibCCK
 *
 *
 */


#include "lib1588_glue.h"
#include "ptp_timers.h"

#include <string.h>

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/clockdriver.h>
#include <libcck/transport_address.h>
#include  <libcck/ttransport.h>
#include <libcck/timer.h>

/* timer support - lib1588 externs */

bool
ptpTimerExpired(PtpTimer *timer)
{

    bool ret = timer->expired;
    timer->expired = false;

    return ret;

}

void
ptpTimerStop(PtpTimer *timer)
{

    if(timer->data) {
	CckTimer *myTimer = timer->data;
	myTimer->stop(myTimer);
	timer->running = false;
	timer->expired = false;
    }

}

void
ptpTimerStart(PtpTimer *timer, const double interval)
{

    if(timer->data) {
	CckTimer *myTimer = timer->data;
	myTimer->start(myTimer, interval);
	timer->interval = myTimer->interval;
	timer->running = true;
	timer->expired = false;

	DBG("Timer %s started at %.03f\n", timer->name, timer->interval);

    }

}

void
ptpTimerLog2Start(PtpTimer *timer, const int power2)
{

    if(timer->data) {
	CckTimer *myTimer = timer->data;
	myTimer->start(myTimer, pow(2, power2));
	timer->interval = myTimer->interval;
    }

}

/* address management - lib1588 externs */

int
ptpAddrStrLen(void *input)
{

    if(input == NULL) {
	return 0;
    }

    CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	    return 0;
    }

    return ts->strLen;

}

int
ptpAddrDataSize(void *input)
{

    if(input == NULL) {
	return 0;
    }

    CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	    return 0;
    }

    return ts->addrSize;

}

char *
ptpAddrToString(char *buf, const int len, void *input)
{

    if(input == NULL) {
	return NULL;
    }

    const CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	    return NULL;
    }

    return ts->toString(buf, len, addr);

}

uint32_t
ptpAddrHash(void *input, const int modulo)
{

    if(input == NULL) {
	return 0;
    }

    const CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	    return 0;
    }

    return ts->hash(addr, modulo);

}

int
ptpAddrCompare(void *a, void *b)
{

    CckTransportAddress *pa = a;

    CckAddressToolset *ts = getAddressToolset(pa->family);

    if(ts == NULL) {
	    return 0;
    }

    return ts->compare(a, b);

}

bool
ptpAddrIsEmpty(void *input)
{

    if(input == NULL) {
	return true;
    }

    const CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {

	    return true;
    }

    return ts->isEmpty(addr);

}

bool
ptpAddrIsMulticast(void *input)
{

    if(input == NULL) {
	return false;
    }

    const CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	    return false;
    }

    return ts->isMulticast(addr);

}

void
ptpAddrClear(void *input)
{

    if(input == NULL) {
	return;
    }

    CckTransportAddress *addr = input;

    clearTransportAddress(addr);

}

void
ptpAddrCopy(void *dst, void *src)
{

    CckTransportAddress *pdst = dst;
    CckTransportAddress *psrc = src;

    copyCckTransportAddress(pdst, psrc);

}

void*
ptpAddrDup(void *src)
{

    CckTransportAddress *addr = src;

    return (void*)duplicateCckTransportAddress(addr);

}

void
ptpAddrFree(void **input)
{

    CckTransportAddress **addr = (CckTransportAddress **)input;

    freeCckTransportAddress(addr);

}

int
ptpAddrSize()
{
    return sizeof(CckTransportAddress);
}

void*
ptpAddrGetData(void *input)
{
    CckTransportAddress *addr = input;
    return getAddressData(addr);

}

/* lib1588 transport-related externs */

void
ptpNetworkRefresh(PtpClock *ptpClock)
{

    TTransport *event = ptpClock->eventTransport;
    TTransport *general = ptpClock->generalTransport;

    event->refresh(event);
    if(event != general) {
	general->refresh(general);
    }

}

uint32_t
getPtpPortRxPackets(PtpClock *ptpClock)
{

    uint32_t ret = 0;

    TTransport *ev = ptpClock->eventTransport;
    TTransport *gen = ptpClock->generalTransport;

    if(ev) {
	ret = ev->counters.rxMsg;
    }

    if(gen && (ev != gen)) {
	ret += gen->counters.rxMsg;
    }

    return ret;

}

uint32_t
getPtpPortTxPackets(PtpClock *ptpClock)
{

    uint32_t ret = 0;

    TTransport *ev = ptpClock->eventTransport;
    TTransport *gen = ptpClock->generalTransport;

    if(ev) {
	ret = ev->counters.txMsg;
    }

    if(gen && (ev != gen)) {
	ret += gen->counters.txMsg;
    }

    return ret;

}

ssize_t
sendPtpData(PtpClock *ptpClock, bool event, char *data, ssize_t len, void *dst, TimeInternal *timestamp)
{

	ssize_t ret = 0;

	TTransport *tr = event ? ptpClock->eventTransport : ptpClock->generalTransport;
	CckTransportAddress *addr = dst ? dst : ( event ? ptpClock->eventDestination : ptpClock->generalDestination);

	if(event) {
	    setTransportAddressPort(addr, PTP_EVENT_PORT);
	}

	tr->outgoingMessage.length = len;
	tr->outgoingMessage.destination = addr;

	ret =  tr->sendMessage(tr, &tr->outgoingMessage);

	if(ret < 0 ) {
	    ptpClock->counters.messageSendErrors++;
	    DBG("Error while sending %s message!\n", event ? "event" : "general");
	    ptpInternalFault(ptpClock);
	    return 0;
	}

	if(timestamp) {

	    if(tr->outgoingMessage.hasTimestamp) {
		timestamp->seconds = tr->outgoingMessage.timestamp.seconds;
		timestamp->nanoseconds = tr->outgoingMessage.timestamp.nanoseconds;
	    } else {
		clearTime(timestamp);
	    }

	}

	return ret;

}

/* lib1588 clock externs */

void
getSystemTime(TimeInternal *time)
{
    getSystemClock()->getTime(getSystemClock(), time);
}

void
getPtpClockTime(TimeInternal *time, PtpClock *ptpClock)
{
    ClockDriver *cd = ptpClock->clockDriver;
    cd->getTime(cd, time);
}


/* lib1588 protocol-related externs */

void
setPtpClockIdentity(PtpClock *ptpClock)
{

    DBG("setPtpClockIdentity() called\n");

    TTransport* tr = ptpClock->eventTransport;

    memcpy(ptpClock->defaultDS.clockIdentity, tr->uid, CLOCK_IDENTITY_LENGTH);

}
