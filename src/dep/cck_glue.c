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
 * @file   cck_glue.c
 * @date   Thu Nov 24 23:19:00 2016
 *
 * @brief  Wrappers configuring LibCCK components from RunTimeOpts *
 *
 *
 */

#include <string.h>
#include <math.h>

#include "cck_glue.h"

#include <libcck/cck_utils.h>
#include <libcck/transport_address.h>
#include <libcck/clockdriver.h>
#include <libcck/ttransport.h>
#include <libcck/timer.h>

#include "../ptp_timers.h"

#define CREATE_ADDR(var) \
	var = createCckTransportAddress();

#define FREE_ADDR(var) \
	freeCckTransportAddress((CckTransportAddress **)(&(var)))

static bool pushClockDriverPrivateConfig_unix(ClockDriver *driver, const RunTimeOpts *global);
static bool pushClockDriverPrivateConfig_linuxphc(ClockDriver *driver, const RunTimeOpts *global);
static bool getCommonTransportConfig(TTransportConfig *config, const RunTimeOpts *global);
static void aclSetup(TTransport *t, const RunTimeOpts *rtOpts);
static int parseUnicastDestinations(PtpClock *ptpClock);

static void ptpTimerExpiry(void *self, void *owner);
static int ptpTimerSetup(PtpTimer *timer, CckFdSet *fdSet, const char *name);
static void ptpRateUpdate (void *transport, void *owner);

bool
configureClockDriver(ClockDriver *driver, const void *rtOpts)
{

    const RunTimeOpts *global = rtOpts;

    bool ret = TRUE;

    ClockDriverConfig *config = &driver->config;

    config->stepType = CSTEP_ALWAYS;

    if(global->noResetClock) {
	config->stepType = CSTEP_NEVER;
    }

    if(global->stepOnce) {
	config->stepType = CSTEP_STARTUP;
    }

    if(global->stepForce) {
	config->stepType = CSTEP_STARTUP_FORCE;
    }

    config->disabled = (tokenInList(global->disabledClocks, driver->name, DEFAULT_TOKEN_DELIM)) ? TRUE:FALSE;
    config->excluded = (tokenInList(global->excludedClocks, driver->name, DEFAULT_TOKEN_DELIM)) ? TRUE:FALSE;

    if(global->noAdjust) {
	config->readOnly = TRUE;
    } else {
	config->readOnly = (tokenInList(global->readOnlyClocks, driver->name, DEFAULT_TOKEN_DELIM)) ? TRUE:FALSE;
    }

    if(config->required && config->disabled) {
	if(!config->readOnly) {
	    WARNING("clock: clock %s cannot be disabled - set to read-only to exclude from sync\n", driver->name);
	} else {
	    WARNING("clock: clock %s cannot be disabled - already set to read-only\n", driver->name);
	}
	config->disabled = FALSE;
    }

    config->noStep = global->noResetClock;
    config->negativeStep = global->negativeStep;
    config->storeToFile = global->storeToFile;
    config->adevPeriod = global->adevPeriod;
    config->stableAdev = global->stableAdev;
    config->unstableAdev = global->unstableAdev;
    config->lockedAge = global->lockedAge;
    config->holdoverAge = global->holdoverAge;
    config->stepTimeout = (global->enablePanicMode) ? global-> panicModeDuration : 0;
    config->calibrationTime = global->clockCalibrationTime;
    config->stepExitThreshold = global->panicModeExitThreshold;
    config->strictSync = global->clockStrictSync;
    config->minStep = global->clockMinStep;

    config->statFilter = global->clockStatFilterEnable;
    config->filterWindowSize = global->clockStatFilterWindowSize;

    if(config->filterWindowSize == 0) {
	config->filterWindowSize = global->clockSyncRate;
    }

    config->filterType = global->clockStatFilterType;
    config->filterWindowType = global->clockStatFilterWindowType;

    config->outlierFilter = global->clockOutlierFilterEnable;
    config->madWindowSize = global->clockOutlierFilterWindowSize;
    config->madDelay = global->clockOutlierFilterDelay;
    config->madMax = global->clockOutlierFilterCutoff;
    config->outlierFilterBlockTimeout = global->clockOutlierFilterBlockTimeout;

    driver->servo.kP = global->servoKP;
    driver->servo.kI = global->servoKI;

    driver->servo.maxOutput = global->servoMaxPpb;
    driver->servo.tauMethod = global->servoDtMethod;
    driver->servo.maxTau = global->servoMaxdT;

    strncpy(config->frequencyDir, global->frequencyDir, PATH_MAX);

    configureClockDriverFilters(driver);

    #define REGISTER_COMPONENT(drvtype, suffix, strname) \
	if(driver->type == drvtype) { \
	    ret = ret && (pushClockDriverPrivateConfig_##suffix(driver, global)); \
	}

    #include <libcck/clockdriver.def>

    DBG("clock: Clock driver %s configured\n", driver->name);

    return ret;

}

static bool
pushClockDriverPrivateConfig_linuxphc(ClockDriver *driver, const RunTimeOpts *global)
{

    ClockDriverConfig *config = &driver->config;

    CCK_GET_PCONFIG(ClockDriver, linuxphc, driver, myConfig);

    config->stableAdev = global->stableAdev_hw;
    config->unstableAdev = global->unstableAdev_hw;
    config->lockedAge = global->lockedAge_hw;
    config->holdoverAge = global->holdoverAge_hw;
    config->negativeStep = global->negativeStep_hw;

    myConfig->lockDevice = global->lockClockDevice;

    driver->servo.kP = global->servoKP_hw;
    driver->servo.kI = global->servoKI_hw;
    driver->servo.maxOutput = global->servoMaxPpb_hw;

    return TRUE;

}

static bool
pushClockDriverPrivateConfig_unix(ClockDriver *driver, const RunTimeOpts *global)
{

    CCK_GET_PCONFIG(ClockDriver, unix, driver, myConfig);

    myConfig->setRtc = global->setRtc;

    return TRUE;

}

/* wrapper */
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



/* set common transport options */
static bool
getCommonTransportConfig(TTransportConfig *config, const RunTimeOpts *global) {

	bool ret = false;
	CckAddressToolset *tools = getAddressToolset(getTTransportTypeFamily(config->_type));

	config->disabled = false;
	config->discarding = false;

	config->uidPid = global->pidAsClockId;

	bool mc = (global->transportMode == TMODE_MC || global->transportMode == TMODE_MIXED);

	switch(config->_type) {

	    case TT_TYPE_LINUXTS_UDPV4:
	    case TT_TYPE_PCAP_UDPV4:
	    case TT_TYPE_SOCKET_UDPV4:
		{
		    CCK_GET_PCONFIG(TTransport, socket_udpv4, config, pConfig);

		    strncpy(pConfig->interface, global->ifaceName, IFNAMSIZ);

		    pConfig->multicastTtl = global->ttl;
		    pConfig->dscpValue = global->dscpValue;

		    if(strlen(global->sourceAddress)) {
			if(!tools->fromString(&pConfig->sourceAddress, global->sourceAddress )) {
			    break;
			}
		    }

		    if(mc) {

			if(!pConfig->multicastStreams->addString(pConfig->multicastStreams, PTP_MCAST_GROUP_IPV4)) {
			    break;
			}

			if(global->delayMechanism == P2P) {
			    if(!pConfig->multicastStreams->addString(pConfig->multicastStreams, PTP_MCAST_PEER_GROUP_IPV4)) {
				break;
			    }
			}

		    }

		    pConfig->bindToAddress = global->bindToInterface;
		    pConfig->disableChecksums = global->disableUdpChecksums;

		    ret = true;
		    break;
		}

	    case TT_TYPE_LINUXTS_UDPV6:
	    case TT_TYPE_PCAP_UDPV6:
	    case TT_TYPE_SOCKET_UDPV6:
		{
		    CCK_GET_PCONFIG(TTransport, socket_udpv6, config, pConfig);

		    strncpy(pConfig->interface, global->ifaceName, IFNAMSIZ);

		    pConfig->multicastTtl = global->ttl;
		    pConfig->dscpValue = global->dscpValue;

		    if(strlen(global->sourceAddress)) {
			if(!tools->fromString(&pConfig->sourceAddress, global->sourceAddress )) {
			    break;
			}
		    }

		    if(mc) {

			if(!pConfig->multicastStreams->addString(pConfig->multicastStreams, PTP_MCAST_GROUP_IPV6)) {
			    break;
			}

			if(global->delayMechanism == P2P) {
			    if(!pConfig->multicastStreams->addString(pConfig->multicastStreams, PTP_MCAST_PEER_GROUP_IPV6)) {
				break;
			    }
			}

		    }

		    CckTransportAddress *maddr;

		    LINKED_LIST_FOREACH_DYNAMIC(pConfig->multicastStreams, maddr) {
			setAddressScope_ipv6(maddr, global->ipv6Scope);
		    }

		    pConfig->bindToAddress = global->bindToInterface;
		    pConfig->disableChecksums = global->disableUdpChecksums;

		    ret = true;
		    break;
		}

	    case TT_TYPE_SOCKET_RAWETH:
		{
		    CCK_GET_PCONFIG(TTransport, socket_raweth, config, pConfig);

		    strncpy(pConfig->interface, global->ifaceName, IFNAMSIZ);

		    pConfig->etherType = PTP_ETHER_TYPE;

		    if(mc) {

			if(!pConfig->multicastStreams->addString(pConfig->multicastStreams, PTP_ETHER_DST)) {
			    break;
			}

			if(global->delayMechanism == P2P) {
			    if(!pConfig->multicastStreams->addString(pConfig->multicastStreams, PTP_ETHER_PEER)) {
				break;
			    }
			}

		    }

		    ret = true;
		    break;
		}

	    case TT_TYPE_PCAP_ETHERNET:
		{
		    CCK_GET_PCONFIG(TTransport, pcap_ethernet, config, pConfig);

		    strncpy(pConfig->interface, global->primaryIfaceName, IFNAMSIZ);

		    pConfig->etherType = PTP_ETHER_TYPE;

		    if(mc) {

			if(!pConfig->multicastStreams->addString(pConfig->multicastStreams, PTP_ETHER_DST)) {
			    break;
			}

			if(global->delayMechanism == P2P) {
			    if(!pConfig->multicastStreams->addString(pConfig->multicastStreams, PTP_ETHER_PEER)) {
				break;
			    }
			}

		    }

		    ret = true;
		    break;
		}

	    case TT_TYPE_LINUXTS_RAWETH:
		{
		    CCK_GET_PCONFIG(TTransport, linuxts_raweth, config, pConfig);

		    strncpy(pConfig->interface, global->primaryIfaceName, IFNAMSIZ);

		    pConfig->etherType = PTP_ETHER_TYPE;

		    if(mc) {

			if(!pConfig->multicastStreams->addString(pConfig->multicastStreams, PTP_ETHER_DST)) {
			    break;
			}

			if(global->delayMechanism == P2P) {
			    if(!pConfig->multicastStreams->addString(pConfig->multicastStreams, PTP_ETHER_PEER)) {
				break;
			    }
			}

		    }

		    ret = true;
		    break;
		}

	    default:
		ERROR("getCommonTransportConfig(): Unsupported transport type %02x\n", config->_type);
		break;
	}

	return ret;

}

/* set Event transport options */
TTransportConfig
*getEventTransportConfig(const int type, const PtpClock *ptpClock) {

	const RunTimeOpts* global = ptpClock->rtOpts;

	CckAddressToolset *tools = getAddressToolset(getTTransportTypeFamily(type));

	TTransportConfig *config;

	if(!tools) {
	    ERROR("getEventTransportConfig(): Could not get address management object! (incorrect transport type %02x/%s?)\n",
	    type, getTTransportTypeName(type));
	    return NULL;
	}

	config = createTTransportConfig(type);

	if(!config) {
	    ERROR("getEventTransportConfig(): Could not create transport configuration object! (incorrect transport type %02x/%s?)\n",
	    type, getTTransportTypeName(type));
	    return NULL;
	}

	if(!getCommonTransportConfig(config, global)) {
	    goto failure;
	}

	config->timestamping = TRUE;

	if(!global->hwTimestamping) {
	    config->flags |= TT_CAPS_PREFER_SW;
	}

	switch(type) {
	    case TT_TYPE_LINUXTS_UDPV6:
	    case TT_TYPE_PCAP_UDPV6:
	    case TT_TYPE_SOCKET_UDPV6:
	    case TT_TYPE_LINUXTS_UDPV4:
	    case TT_TYPE_PCAP_UDPV4:
	    case TT_TYPE_SOCKET_UDPV4:
		{
		    CCK_GET_PCONFIG(TTransport, udp_common, config, pConfig);
		    pConfig->listenPort = PTP_EVENT_PORT;
		    return config;

		}

	    case TT_TYPE_PCAP_ETHERNET:
	    case TT_TYPE_SOCKET_RAWETH:
	    case TT_TYPE_LINUXTS_RAWETH:
		    return config;
	    default:
		ERROR("getEventTransportConfig(): Unsupported transport type %02x\n", type);
		break;
	}

failure:

	freeTTransportConfig(&config);

	return NULL;

}

/* set general transport options */
TTransportConfig
*getGeneralTransportConfig(const int type, const PtpClock *ptpClock) {

	const RunTimeOpts* global = ptpClock->rtOpts;

	CckAddressToolset *tools = getAddressToolset(getTTransportTypeFamily(type));

	TTransportConfig *config;

	if(!tools) {
	    ERROR("getGeneralTransportConfig(): Could not get address management object! (incorrect transport type %02x/%s?)\n",
	    type, getTTransportTypeName(type));
	    return NULL;
	}

	config = createTTransportConfig(type);

	if(!config) {
	    ERROR("getGeneralTransportConfig(): Could not create transport configuration object! (incorrect transport type %02x/%s?)\n",
	    type, getTTransportTypeName(type));
	    return NULL;
	}

	if(!getCommonTransportConfig(config, global)) {
	    freeTTransportConfig(&config);
	    return NULL;
	}

	config->timestamping = false;
	config->unmonitored = true;

	switch(type) {

	    case TT_TYPE_LINUXTS_UDPV4:
	    case TT_TYPE_PCAP_UDPV4:
	    case TT_TYPE_SOCKET_UDPV4:
	    case TT_TYPE_LINUXTS_UDPV6:
	    case TT_TYPE_PCAP_UDPV6:
	    case TT_TYPE_SOCKET_UDPV6:

		{
		    CCK_GET_PCONFIG(TTransport, udp_common, config, pConfig);

		    pConfig->listenPort = PTP_GENERAL_PORT;

		    return config;

		}

	    case TT_TYPE_PCAP_ETHERNET:
	    case TT_TYPE_SOCKET_RAWETH:
	    case TT_TYPE_LINUXTS_RAWETH:
		{
		    return config;
		}

	    default:
		ERROR("getGeneralTransportConfig(): Unsupported transport type %02x\n", type);
		break;
	}

	freeTTransportConfig(&config);

	return NULL;

}

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

void ptpDataCallback(void *fd, void *transport) {

    TTransport *tr = transport;
    PtpClock *clock = tr->owner;
    TimeInternal timestamp = {0 ,0};

    int len = tr->receiveMessage(tr, &tr->incomingMessage);

    if(len > 0) {

	if(tr->incomingMessage.hasTimestamp) {
	    timestamp.seconds = tr->incomingMessage.timestamp.seconds;
	    timestamp.nanoseconds = tr->incomingMessage.timestamp.nanoseconds;
	}

	processPtpData(clock, &timestamp, len, &tr->incomingMessage.from, &tr->incomingMessage.to);

    }

}

void getPtpNetworkInfo(PtpClock *ptpClock)
{

    TTransport *event = ptpClock->eventTransport;

    if(!ptpClock->eventTransport) {
	return;
    }

    /* tell PTP about the network protocol it's using */
    switch(event->family) {
	case TT_FAMILY_LOCAL: /* 1588 has no notion of a local transport */
	case TT_FAMILY_IPV4:
	    ptpClock->portDS.networkProtocol = UDP_IPV4;
	    break;
	case TT_FAMILY_IPV6:
	    ptpClock->portDS.networkProtocol = UDP_IPV6;
	    break;
	case TT_FAMILY_ETHERNET:
	    ptpClock->portDS.networkProtocol = IEEE_802_3;
    }

    /* populate protocol address and physical address */
    ptpClock->portDS.addressLength = min(16, event->tools->addrSize);
    memcpy(ptpClock->portDS.addressField,
	    event->tools->getData(&event->ownAddress),
	    ptpClock->portDS.addressLength);

    if(event->hwAddress.populated) {
	CckAddressToolset *ts = getAddressToolset(event->hwAddress.family);
	ptpClock->portDS.physicalAddressLength = min(16, ts->addrSize);
	memcpy(ptpClock->portDS.physicalAddressField,
	    ts->getData(&event->hwAddress),
	    ptpClock->portDS.physicalAddressLength);

    }
}

void
myPtpClockSetClockIdentity(PtpClock *ptpClock)
{

    DBG("myPtpClockSetClockIdentity() called\n");

    TTransport* tr = ptpClock->eventTransport;

    memcpy(ptpClock->defaultDS.clockIdentity, tr->uid, CLOCK_IDENTITY_LENGTH);

}

void
myPtpClockPreInit(PtpClock *ptpClock)
{



    DBG("myPtpClockInit() called\n");

    for(int i = 0; i < UNICAST_MAX_DESTINATIONS; i++) {
	CREATE_ADDR(ptpClock->syncDestIndex[i].protocolAddress);
	CREATE_ADDR(ptpClock->unicastDestinations[i].protocolAddress);
	CREATE_ADDR(ptpClock->unicastGrants[i].protocolAddress);
    }

    for(int j = 0; j < ptpClock->rtOpts->fmrCapacity; j++) {
	CREATE_ADDR(ptpClock->foreign[j].protocolAddress);
    }

    CREATE_ADDR(ptpClock->generalDestination);
    CREATE_ADDR(ptpClock->eventDestination);

    CREATE_ADDR(ptpClock->peerGeneralDestination);
    CREATE_ADDR(ptpClock->peerEventDestination);

    CREATE_ADDR(ptpClock->unicastPeerDestination.protocolAddress);
    CREATE_ADDR(ptpClock->lastSyncDst);
    CREATE_ADDR(ptpClock->lastPdelayRespDst);

    /* set up callbacks */
    ptpClock->callbacks.onStateChange = myPtpClockStateChange;

}

void
myPtpClockPostShutdown(PtpClock *ptpClock)
{

    DBG("myPtpClockShutdown() called\n");

    for(int i = 0; i < UNICAST_MAX_DESTINATIONS; i++) {
	FREE_ADDR(ptpClock->syncDestIndex[i].protocolAddress);
	FREE_ADDR(ptpClock->unicastDestinations[i].protocolAddress);
	FREE_ADDR(ptpClock->unicastGrants[i].protocolAddress);
    }

    for(int j = 0; j < ptpClock->fmrCapacity; j++) {
	FREE_ADDR(ptpClock->foreign[j].protocolAddress);
    }

    FREE_ADDR(ptpClock->generalDestination);
    FREE_ADDR(ptpClock->eventDestination);

    FREE_ADDR(ptpClock->peerGeneralDestination);
    FREE_ADDR(ptpClock->peerEventDestination);

    FREE_ADDR(ptpClock->unicastPeerDestination.protocolAddress);
    FREE_ADDR(ptpClock->lastSyncDst);
    FREE_ADDR(ptpClock->lastPdelayRespDst);

}

void
myPtpClockStepNotify(void *owner)
{

    PtpClock *ptpClock = (PtpClock*)owner;

    DBG("PTP clock was notified of clock step - resetting outlier filters and re-calibrating offsets\n");

    recalibrateClock(ptpClock, true);
}

void
myPtpClockUpdate(void *owner)
{

    PtpClock *ptpClock = owner;

    if(ptpClock && ptpClock->masterClock) {

	ClockDriver *mc = ptpClock->masterClock;

	if((ptpClock->portDS.portState == PTP_MASTER) || (ptpClock->portDS.portState == PTP_PASSIVE)) {
	    mc->setState(mc, CS_LOCKED);
	}

	mc->touchClock(mc);

    }

}

void
myPtpClockLocked(void *owner, bool locked)
{

    PtpClock *ptpClock = owner;

    if(ptpClock) {
	ptpClock->clockLocked = locked;
    }

}

void
myPtpClockStateChange(PtpClock *ptpClock, const uint8_t from, const uint8_t to)
{

    ClockDriver *cd = ptpClock->clockDriver;
    ClockDriver *mc = ptpClock->masterClock;


    if(!cd) {
	return;
    }

    if(mc) {
	/* entering MASTER or PASSIVE */
	if((to == PTP_MASTER) || (to == PTP_PASSIVE)) {
	    mc->setExternalReference(mc, "PREFMST", RC_EXTERNAL);
	/* leaving MASTER or PASSIVE */
	} else if((from == PTP_MASTER) || (from == PTP_PASSIVE)) {
	    mc->setReference(mc, NULL);
	}
    }

    if(to == PTP_SLAVE) {
	    cd->setExternalReference(cd, "PTP", RC_PTP);
	    cd->owner = ptpClock;
	    cd->callbacks.onStep = myPtpClockStepNotify;
	    cd->callbacks.onLock = myPtpClockLocked;
    } else if (from == PTP_SLAVE) {
	    if(!mc || (mc != cd)) {
		cd->setReference(cd, NULL);
	    }
	    cd->callbacks.onStep = NULL;
	    cd->callbacks.onLock = NULL;
    }

}

int
myAddrStrLen(void *input)
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
myAddrDataSize(void *input)
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
myAddrToString(char *buf, const int len, void *input)
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
myAddrHash(void *input, const int modulo)
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
myAddrCompare(void *a, void *b)
{

    CckTransportAddress *pa = a;

    CckAddressToolset *ts = getAddressToolset(pa->family);

    if(ts == NULL) {
	    return 0;
    }

    return ts->compare(a, b);

}

bool
myAddrIsEmpty(void *input)
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
myAddrIsMulticast(void *input)
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
myAddrClear(void *input)
{

    if(input == NULL) {
	return;
    }

    CckTransportAddress *addr = input;

    clearTransportAddress(addr);

}

void
myAddrCopy(void *dst, void *src)
{

    CckTransportAddress *pdst = dst;
    CckTransportAddress *psrc = src;

    copyCckTransportAddress(pdst, psrc);

}

void*
myAddrDup(void *src)
{

    CckTransportAddress *addr = src;

    return (void*)duplicateCckTransportAddress(addr);

}

void
myAddrFree(void **input)
{

    CckTransportAddress **addr = (CckTransportAddress **)input;

    freeCckTransportAddress(addr);

}

int
myAddrSize()
{
    return sizeof(CckTransportAddress);
}

void*
myAddrGetData(void *input)
{
    CckTransportAddress *addr = input;
    return getAddressData(addr);

}



int
ptpPreSend (void *transport, void *owner, char *data, const size_t len, bool isMulticast)
{

    if(len < 7) {
	return 0;
    }

    if(isMulticast) {
	return 1;
    }

    *(data + 6) |= PTP_UNICAST;

    return 1;

}

int
ptpIsRegularData (void *transport, void *owner, char *data, const size_t len, bool isMulticast)
{

    if(len < HEADER_LENGTH) {
	return 1;
    }

    Enumeration4 msgType = (*(Enumeration4 *) (data + 0)) & 0x0F;

    if(msgType == MANAGEMENT) {
	DBG("ptpIsRegularData: data received is a management message\n");
	return 0;
    }

    return 1;

}

int ptpMatchData (void *owner, void *transport, char *a, const size_t alen, char *b, const size_t blen)
{

    if(alen != blen) {
	DBG("ptpMatchData: a.len=%d != b.len=%d, discarding\n", alen, blen);
	return 0;
    }

    uint8_t msgtypea= (*(uint8_t*)(a)) & 0x0F;
    uint16_t seqnoa = *(uint16_t*)(a + 30);
    uint8_t msgtypeb= (*(uint8_t*)(b)) & 0x0F;
    uint16_t seqnob = *(uint16_t*)(b + 30);

    if( seqnoa != seqnob || msgtypea != msgtypeb ) {
	DBG("ptpMatchData: a.seqno=%d, b.seqno=%d, a.msgtype=%d, b.msgtype=%d, no match, discarding\n",
	    ntohs(seqnoa), ntohs(seqnob), msgtypea, msgtypeb);
	return 0;
    }

	DBG("ptpMatchData: a.seqno=%d, b.seqno=%d, a.msgtype=%d, b.msgtype=%d, match, permitting\n",
	    ntohs(seqnoa), ntohs(seqnob), msgtypea, msgtypeb);

    return 1;

}

uint32_t
myPtpClockGetRxPackets(PtpClock *ptpClock)
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
myPtpClockGetTxPackets(PtpClock *ptpClock)
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

static void
aclSetup(TTransport *t, const RunTimeOpts *rtOpts) {

    t->dataAcl->config.disabled = !rtOpts->timingAclEnabled;
    t->controlAcl->config.disabled = !rtOpts->managementAclEnabled;

    if(rtOpts->timingAclEnabled) {
	t->dataAcl->compile(t->dataAcl, rtOpts->timingAclPermitText,
					    rtOpts->timingAclDenyText,
					    rtOpts->timingAclOrder);
    }

    if(rtOpts->managementAclEnabled) {
	t->controlAcl->compile(t->controlAcl, rtOpts->managementAclPermitText,
					    rtOpts->managementAclDenyText,
					    rtOpts->managementAclOrder);
    }

}


void configureAcls(PtpClock *ptpClock) {

    TTransport *ev = ptpClock->eventTransport;
    TTransport *gen = ptpClock->generalTransport;

    if(ev) {
	aclSetup(ev, ptpClock->rtOpts);
    }

    if(gen && (ev != gen)) {
	aclSetup(gen, ptpClock->rtOpts);
    }

}

static int
parseUnicastDestinations(PtpClock *ptpClock) {

    int found = 0;
    int total = 0;
    int tmp = 0;

    RunTimeOpts *rtOpts = ptpClock->rtOpts;
    UnicastDestination *output = ptpClock->unicastDestinations;
    CckTransportAddress *addr;
    int family = getConfiguredFamily(rtOpts);

    CckAddressToolset *ts = getAddressToolset(family);

    if(!strlen(rtOpts->unicastDestinations)) {
	return 0;
    }

    foreach_token_begin(dst, rtOpts->unicastDestinations, htoken, DEFAULT_TOKEN_DELIM);

	addr = createAddressFromString(family, htoken);

	if(addr) {
	    output[found].protocolAddress = addr;
	    found++;
	    tmpstr(strAddr, ts->strLen);
	    DBG("parseUnicastDestinations(): host #%d address %s\n", found,
		ts->toString(strAddr, strAddr_len, addr));
	    if (found >= UNICAST_MAX_DESTINATIONS) {
		break;
	    }
	}

    foreach_token_end(dst);

    if(!found) {
	return 0;
    }

    total = found;
    found = 0;

    foreach_token_begin(dom, rtOpts->unicastDomains, dtoken, DEFAULT_TOKEN_DELIM);

	if (sscanf(dtoken,"%d", &tmp)) {
	    DBG("parseUnicastDestinations(): host #%d domain %d\n", found, tmp);
	    output[found].domainNumber = tmp;
	    found++;
	    if(found >= total) {
		break;
	    }
	}

    foreach_token_end(dom);

    found = 0;

    foreach_token_begin(pref, rtOpts->unicastLocalPreference, ptoken, DEFAULT_TOKEN_DELIM);

	if (sscanf(ptoken,"%d", &tmp)) {
	    DBG("parseUnicastDestinations(): host #%d localpref %d\n", found, tmp);
	    output[found].localPreference = tmp;
	    found++;
	    if(found >= total) {
		break;
	    }
	}

    foreach_token_end(pref);

    DBG("parseUnicastDestinations(): %d destinations configured\n", total);

    return total;

}

/* parse a list of hosts to a list of IP addresses */
bool configureUnicast(PtpClock *ptpClock)
{

    RunTimeOpts *rtOpts = ptpClock->rtOpts;

    /* no need to go further */
    if(rtOpts->transportMode != TMODE_UC) {
	return true;
    }

    ptpClock->unicastDestinationCount = parseUnicastDestinations(ptpClock);

    /* configure unicast peer address (exotic, I know) */
    if(rtOpts->delayMechanism == P2P) {

	if(!strlen(rtOpts->unicastPeerDestination)) {
	    ERROR("No P2P unicast destination configured");
	    return false;
	}

	ptpClock->unicastPeerDestination.protocolAddress =
	    createAddressFromString(getConfiguredFamily(rtOpts), rtOpts->unicastPeerDestination);

	if(!ptpClock->unicastPeerDestination.protocolAddress) {
	    ERROR("Invalid P2P unicast destination: %s\n", rtOpts->unicastPeerDestination);
	    return false;
	}

    }

    return true;

}

void configurePtpAddrs(PtpClock *ptpClock, const int family) {

    switch(family) {

	case TT_FAMILY_IPV4:

	    ptpClock->eventDestination = createAddressFromString(TT_FAMILY_IPV4, PTP_MCAST_GROUP_IPV4);
	    setTransportAddressPort(ptpClock->eventDestination, PTP_EVENT_PORT);

	    ptpClock->generalDestination = createAddressFromString(TT_FAMILY_IPV4, PTP_MCAST_GROUP_IPV4);
	    setTransportAddressPort(ptpClock->generalDestination, PTP_GENERAL_PORT);

	    if(ptpClock->rtOpts->delayMechanism == P2P) {
		ptpClock->peerEventDestination = createAddressFromString(TT_FAMILY_IPV4, PTP_MCAST_PEER_GROUP_IPV4);
		setTransportAddressPort(ptpClock->peerEventDestination, PTP_EVENT_PORT);

		ptpClock->peerGeneralDestination = createAddressFromString(TT_FAMILY_IPV4, PTP_MCAST_PEER_GROUP_IPV4);
		setTransportAddressPort(ptpClock->peerGeneralDestination, PTP_GENERAL_PORT);
	    }

	    break;

	case TT_FAMILY_IPV6:

	    ptpClock->eventDestination = createAddressFromString(TT_FAMILY_IPV6, PTP_MCAST_GROUP_IPV6);
	    setTransportAddressPort(ptpClock->eventDestination, PTP_EVENT_PORT);
	    setAddressScope_ipv6(ptpClock->eventDestination, ptpClock->rtOpts->ipv6Scope);

	    ptpClock->generalDestination = createAddressFromString(TT_FAMILY_IPV6, PTP_MCAST_GROUP_IPV6);
	    setTransportAddressPort(ptpClock->generalDestination, PTP_GENERAL_PORT);
	    setAddressScope_ipv6(ptpClock->generalDestination, ptpClock->rtOpts->ipv6Scope);

	    if(ptpClock->rtOpts->delayMechanism == P2P) {
		ptpClock->peerEventDestination = createAddressFromString(TT_FAMILY_IPV6, PTP_MCAST_PEER_GROUP_IPV6);
		setTransportAddressPort(ptpClock->peerEventDestination, PTP_EVENT_PORT);

		ptpClock->peerGeneralDestination = createAddressFromString(TT_FAMILY_IPV6, PTP_MCAST_PEER_GROUP_IPV6);
		setTransportAddressPort(ptpClock->peerGeneralDestination, PTP_GENERAL_PORT);
	    }
	    break;

	case TT_FAMILY_ETHERNET:

	    ptpClock->eventDestination = createAddressFromString(TT_FAMILY_ETHERNET, PTP_ETHER_DST);
	    ptpClock->generalDestination = createAddressFromString(TT_FAMILY_ETHERNET, PTP_ETHER_DST);

	    if(ptpClock->rtOpts->delayMechanism == P2P) {
		ptpClock->peerEventDestination = createAddressFromString(TT_FAMILY_ETHERNET, PTP_ETHER_PEER);
		ptpClock->peerGeneralDestination = createAddressFromString(TT_FAMILY_ETHERNET, PTP_ETHER_PEER);
	    }

    }

}

/* get event transport configuration flags */
int
getEventTransportFlags(const RunTimeOpts *rtOpts) {

	int flags = 0;

	if(rtOpts->transportMode == TMODE_MC || rtOpts->transportMode == TMODE_MIXED) {
		flags |= TT_CAPS_MCAST;
	}

	if(rtOpts->transportMode == TMODE_UC || rtOpts->transportMode == TMODE_MIXED) {
			flags |= TT_CAPS_UCAST;
	}

	if(!rtOpts->hwTimestamping) {
	    flags |= TT_CAPS_PREFER_SW;
	}

    return flags;

}

/* get general transport configuration flags */
int getGeneralTransportFlags(const RunTimeOpts *rtOpts) {

	int flags = 0;

	if(rtOpts->transportMode == TMODE_MC || rtOpts->transportMode == TMODE_MIXED) {
		flags |= TT_CAPS_MCAST;
	}

	if(rtOpts->transportMode == TMODE_UC || rtOpts->transportMode == TMODE_MIXED) {
			flags |= TT_CAPS_UCAST;
	}

    return flags;

}

static void
ptpTimerExpiry(void *self, void *owner)
{

    PtpTimer *timer = owner;

    timer->expired = true;

    DBG("Timer %s expired\n", timer->name);

}

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
ptpTimerLogStart (PtpTimer *timer, int power2)
{

    if(timer->data) {
	CckTimer *myTimer = timer->data;
	myTimer->start(myTimer, pow(2, power2));
	timer->interval = myTimer->interval;
    }

}

int
ptpTimerSetup(PtpTimer *timer, CckFdSet *fdSet, const char *name)
{

    CckTimer *myTimer = NULL;

    memset(timer, 0, sizeof(PtpTimer));
    strncpy(timer->name, name, PTP_TIMER_NAME_MAX);

    myTimer = createCckTimer(CCKTIMER_ANY, name);

    if(!myTimer) {
	return 0;
    }

    if(myTimer->init(myTimer, false, fdSet) != 1) {
	freeCckTimer(&myTimer);
	return 0;
    }

    myTimer->owner = timer;
    myTimer->callbacks.onExpired = ptpTimerExpiry;
    timer->data = myTimer;

    return 1;

}

bool
setupPtpTimers(PtpClock *ptpClock, CckFdSet *fdSet)
{

    for(int i=0; i < PTP_MAX_TIMER; i++) {
	    if(ptpTimerSetup(&ptpClock->timers[i], fdSet, getPtpTimerName(i)) != 1) {
		return false;
	    }
    }

    return true;

}

void
shutdownPtpTimers(PtpClock *ptpClock)
{

    for(int i=0; i<PTP_MAX_TIMER; i++) {
	if(ptpClock->timers[i].data) {
	freeCckTimer((CckTimer**)&ptpClock->timers[i].data);
	    memset(&ptpClock->timers[i], 0, sizeof(PtpTimer));
	}
    }

}

static void
ptpRateUpdate (void *transport, void *owner)
{

    PtpClock *ptpClock = owner;

    TTransport *event = ptpClock->eventTransport;
    TTransport *general = ptpClock->generalTransport;

    if (owner && event) {

	ptpClock->counters.messageSendRate = event->counters.txRateMsg;
	ptpClock->counters.messageReceiveRate = event->counters.rxRateMsg;
	ptpClock->counters.bytesSendRate = event->counters.txRateBytes;
	ptpClock->counters.bytesReceiveRate = event->counters.rxRateBytes;

	if(event != general) {
	    ptpClock->counters.messageSendRate += general->counters.txRateMsg;
	    ptpClock->counters.messageReceiveRate += general->counters.rxRateMsg;
	    ptpClock->counters.bytesSendRate += general->counters.txRateBytes;
	    ptpClock->counters.bytesReceiveRate += general->counters.rxRateBytes;
	}

    }
}

bool
netInit(PtpClock *ptpClock, CckFdSet *fdSet)
{

    RunTimeOpts *rtOpts = ptpClock->rtOpts;
    Boolean ret = TRUE;
    int transportType;

    int gFlags = getGeneralTransportFlags(ptpClock->rtOpts);
    int eFlags = getEventTransportFlags(ptpClock->rtOpts);

    int family = getConfiguredFamily(rtOpts);

    freeTTransport((TTransport**)&ptpClock->generalTransport);
    freeTTransport((TTransport**)&ptpClock->eventTransport);

    freeCckTransportAddress((CckTransportAddress**)&ptpClock->eventDestination);
    freeCckTransportAddress((CckTransportAddress**)&ptpClock->peerEventDestination);

    freeCckTransportAddress((CckTransportAddress**)&ptpClock->generalDestination);
    freeCckTransportAddress((CckTransportAddress**)&ptpClock->peerGeneralDestination);

    if(rtOpts->backupIfaceEnabled &&
	    ptpClock->runningBackupInterface) {
		strncpy(rtOpts->ifaceName, rtOpts->backupIfaceName, IFNAMSIZ);
	} else {
		strncpy(rtOpts->ifaceName, rtOpts->primaryIfaceName, IFNAMSIZ);
	}

    if(!testInterface(rtOpts->ifaceName, family, rtOpts->sourceAddress)) {
	return false;
    }

    if(!configureUnicast(ptpClock)) {
	ERROR("netInit(): Unicast destination configuration failed\n");
	return false;
    }

    transportType = detectTTransport(family, rtOpts->ifaceName, eFlags, rtOpts->transportType);

    if(transportType == TT_TYPE_NONE) {
	ERROR("NetInit(): No usable transport found\n");
	return FALSE;
    }

    TTransportConfig *eventConfig = getEventTransportConfig(transportType, ptpClock);
    TTransportConfig *generalConfig = getGeneralTransportConfig(transportType, ptpClock);

    if(!eventConfig || !generalConfig) {
	CRITICAL("netInit(): Could not configure network transports\n");
	return FALSE;
    }

    eventConfig->flags = eFlags;
    generalConfig->flags = gFlags;

    configurePtpAddrs(ptpClock, family);

    if(family == TT_FAMILY_ETHERNET) {
	ptpClock->eventTransport = createTTransport(transportType, "Event / General");
	ptpClock->generalTransport = ptpClock->eventTransport;
    } else {
	ptpClock->eventTransport = createTTransport(transportType, "Event");
	ptpClock->generalTransport = createTTransport(transportType, "General");
    }

    TTransport *event = ptpClock->eventTransport;
    TTransport *general = ptpClock->generalTransport;

    if(!event || !general) {
	CRITICAL("netInit(): Could not create network transports\n");
	return FALSE;
    }

    event->setBuffers(event, ptpClock->msgIbuf, PACKET_SIZE, ptpClock->msgObuf, PACKET_SIZE);
    event->callbacks.preTx = ptpPreSend;
    event->callbacks.isRegularData = ptpIsRegularData;
    event->callbacks.matchData = ptpMatchData;
    event->myFd.callbacks.onData = ptpDataCallback;
    event->callbacks.onRateUpdate = ptpRateUpdate;
    event->owner = ptpClock;
    if(event->init(event, eventConfig, fdSet) < 1) {
	CRITICAL("Coult not start event transport\n");
	ret = FALSE;
	goto gameover;
    }

    freeTTransportConfig(&eventConfig);

    if(event != general) {
	general->setBuffers(general, ptpClock->msgIbuf, PACKET_SIZE, ptpClock->msgObuf, PACKET_SIZE);
	general->callbacks.preTx = ptpPreSend;
	general->callbacks.isRegularData = ptpIsRegularData;
	general->myFd.callbacks.onData = ptpDataCallback;
	general->owner = ptpClock;
	if(general->init(general, generalConfig, fdSet) < 1) {
	    CRITICAL("Coult not start event transport\n");
	    ret = FALSE;
	    goto gameover;
	}

	freeTTransportConfig(&generalConfig);
    }

    configureAcls(ptpClock);

    controlClockDrivers(CD_NOTINUSE);

    if(!prepareClockDrivers(ptpClock)) {
	ERROR("Cannot start clock drivers - aborting mission\n");
	exit(-1);
    }

    getPtpNetworkInfo(ptpClock);

    /* create any extra clocks */
    if(!createClockDriversFromString(rtOpts->extraClocks, configureClockDriver, rtOpts, false)) {
	ERROR("Failed to create extra clocks\n");
	ret = FALSE;
	goto gameover;
    }

    /* clean up unused clock drivers */
    controlClockDrivers(CD_CLEANUP);

    reconfigureClockDrivers(configureClockDriver, rtOpts);

    return ret;

gameover:

    freeTTransport((TTransport**)&ptpClock->generalTransport);
    freeTTransport((TTransport**)&ptpClock->eventTransport);

    freeCckTransportAddress((CckTransportAddress**)&ptpClock->eventDestination);
    freeCckTransportAddress((CckTransportAddress**)&ptpClock->peerEventDestination);

    freeCckTransportAddress((CckTransportAddress**)&ptpClock->generalDestination);
    freeCckTransportAddress((CckTransportAddress**)&ptpClock->peerGeneralDestination);

    return ret;

}

void
netShutdown(PtpClock *ptpClock)
{

    TTransport *event = ptpClock->eventTransport;
    TTransport *general = ptpClock->generalTransport;

    if(event == general) {
	general = NULL;
	ptpClock->generalTransport = NULL;
    } else  if(general) {
	general->shutdown(general);
    }

    if(event) {
	event->shutdown(event);
    }

    freeCckTransportAddress((CckTransportAddress**)&ptpClock->eventDestination);
    freeCckTransportAddress((CckTransportAddress**)&ptpClock->peerEventDestination);

    freeCckTransportAddress((CckTransportAddress**)&ptpClock->generalDestination);
    freeCckTransportAddress((CckTransportAddress**)&ptpClock->peerGeneralDestination);

}
