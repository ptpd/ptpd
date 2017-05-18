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
 * @file   transport.c
 * @date   Thu Nov 24 23:19:00 2016
 *
 * @brief  PTPd transport implementation using libCCK
 *
 *
 */

#include <string.h>
#include <config.h>
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
	{\
	    CckTransportAddress* tmp = var;\
	    freeCckTransportAddress(&tmp);\
	    var = NULL;\
	}

/* set PTP groups / destination addresses */
static void configurePtpAddrs(PtpClock *ptpClock, const int family);
/* get event transport configuration flags */
static int getEventTransportFlags(const GlobalConfig *global);
/* get general transport configuration flags */
static int getGeneralTransportFlags(const GlobalConfig *global);
/* generate transport configuration */
static TTransportConfig *getEventTransportConfig(const int type, const GlobalConfig *global);
static TTransportConfig *getGeneralTransportConfig(const int type, const GlobalConfig *global);
/* set common configuration for event and general transport */
static bool setCommonTransportConfig(TTransportConfig *config, const GlobalConfig *global);
/* set up access control lists */
static void aclSetup(TTransport *t, const GlobalConfig *global);
/* configure unicast addressing */
static bool configureUnicast(PtpClock *ptpClock, const GlobalConfig *global);
/* parse unicast destination list from global configuration */
static int parseUnicastDestinations(PtpClock *ptpClock, const GlobalConfig *global);
/* free all transport-related data in a PTP port structure */
static void freeTransportData(PtpClock *ptpClock);
/* update PTP message and data rate information */
static void updatePtpRates (void *transport, void *owner);
/* Populate network protocol and address information inside PTP clock */
static void setPtpNetworkInfo(PtpClock *ptpClock);

/* TTransport callbacks */

/* set the PTP unicast flag depending on isMulticast flag */
static int ptpPreSend (void *transport, void *owner, char *data, const size_t len, bool isMulticast);

/*
 * check if data in two buffers are matching PTP messages (same message ID and sequence number:
 * used to verify if TX timestamp data matches last send message
*/
static int matchPtpData (void *owner, void *transport, char *a, const size_t alen, char *b, const size_t blen);

/*
 * check if data in buffer is a management message or other message type:
 * used to decide which ACL to match data against
 */
static int isPtpRegularData (void *transport, void *owner, char *data, const size_t len, bool isMulticast);

/*
 * receive and process data when available (CckFd callback):
 * FD's owner is transport, transport's owner is PTP port
 */
static void ptpDataCallback(void *fd, void *owner);

/* handle a network fault or cleared fault */
static void ptpNetworkFault(void *transport, void *owner, const bool fault);

/* handle a network change - major requires protocol re-init, minor needs at least offset re-calibration. */
static void ptpNetworkChange(void *transport, void *owner, const bool major);

/* handle a network change requiring us to reconfigure or re-add clock drivers */
static int ptpClockDriverChange (void *transport, void *owner);

/* set common transport options */
static bool
setCommonTransportConfig(TTransportConfig *config, const GlobalConfig *global) {

	bool ret = false;
	CckAddressToolset *tools = getAddressToolset(getTTransportTypeFamily(config->_type));

	config->disabled = false;
	config->discarding = false;

	config->uidPid = global->pidAsClockId;

	bool mc = (global->transportMode == TMODE_MC || global->transportMode == TMODE_MIXED);

	switch(config->_type) {

#ifdef CCK_BUILD_TTRANSPORT_LINUXTS
	    case TT_TYPE_LINUXTS_UDPV4:
#endif

#ifdef CCK_BUILD_TTRANSPORT_DLPI
	    case TT_TYPE_DLPI_UDPV4:
#endif

#ifdef CCK_BUILD_TTRANSPORT_PCAP
	    case TT_TYPE_PCAP_UDPV4:
#endif
	    case TT_TYPE_SOCKET_UDPV4:
		{
		    CCK_GET_PCONFIG(TTransport, socket_udpv4, config, pConfig);

		    strncpy(pConfig->interface, global->ifName, IFNAMSIZ);

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

#ifdef CCK_BUILD_TTRANSPORT_LINUXTS
	    case TT_TYPE_LINUXTS_UDPV6:
#endif

#ifdef CCK_BUILD_TTRANSPORT_DLPI
	    case TT_TYPE_DLPI_UDPV6:
#endif

#ifdef CCK_BUILD_TTRANSPORT_PCAP
	    case TT_TYPE_PCAP_UDPV6:
#endif
	    case TT_TYPE_SOCKET_UDPV6:
		{
		    CCK_GET_PCONFIG(TTransport, socket_udpv6, config, pConfig);

		    strncpy(pConfig->interface, global->ifName, IFNAMSIZ);

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

		    LL_FOREACH_DYNAMIC(pConfig->multicastStreams, maddr) {
			setAddressScope_ipv6(maddr, global->ipv6Scope);
		    }

		    pConfig->bindToAddress = global->bindToInterface;
		    pConfig->disableChecksums = global->disableUdpChecksums;

		    ret = true;
		    break;
		}

#ifdef CCK_BUILD_TTRANSPORT_SOCK_RAWETH
	    case TT_TYPE_SOCKET_RAWETH:
		{
		    CCK_GET_PCONFIG(TTransport, socket_raweth, config, pConfig);

		    strncpy(pConfig->interface, global->ifName, IFNAMSIZ);

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
#endif

#ifdef CCK_BUILD_TTRANSPORT_PCAP
	    case TT_TYPE_PCAP_ETHERNET:
		{
		    CCK_GET_PCONFIG(TTransport, pcap_ethernet, config, pConfig);

		    strncpy(pConfig->interface, global->ifName, IFNAMSIZ);

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
#endif

#ifdef CCK_BUILD_TTRANSPORT_LINUXTS
	    case TT_TYPE_LINUXTS_RAWETH:
		{
		    CCK_GET_PCONFIG(TTransport, linuxts_raweth, config, pConfig);

		    strncpy(pConfig->interface, global->ifName, IFNAMSIZ);

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
#endif
	    default:
		ERROR("setCommonTransportConfig(): Unsupported transport type %02x\n", config->_type);
		break;
	}

	return ret;

}

/* set Event transport options */
static TTransportConfig
*getEventTransportConfig(const int type, const GlobalConfig *global) {

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

	if(!setCommonTransportConfig(config, global)) {
	    goto failure;
	}

	config->timestamping = TRUE;

	if(!global->hwTimestamping) {
	    config->flags |= TT_CAPS_PREFER_SW;
	}

	switch(type) {

#ifdef CCK_BUILD_TTRANSPORT_LINUXTS
	    case TT_TYPE_LINUXTS_UDPV6:
	    case TT_TYPE_LINUXTS_UDPV4:
#endif

#ifdef CCK_BUILD_TTRANSPORT_DLPI
	    case TT_TYPE_DLPI_UDPV6:
	    case TT_TYPE_DLPI_UDPV4:
#endif

#ifdef CCK_BUILD_TTRANSPORT_PCAP
	    case TT_TYPE_PCAP_UDPV6:
	    case TT_TYPE_PCAP_UDPV4:
#endif

	    case TT_TYPE_SOCKET_UDPV6:
	    case TT_TYPE_SOCKET_UDPV4:
		{
		    CCK_GET_PCONFIG(TTransport, udp_common, config, pConfig);
		    pConfig->listenPort = PTP_EVENT_PORT;
		    return config;

		}

#ifdef CCK_BUILD_TTRANSPORT_LINUXTS
	    case TT_TYPE_LINUXTS_RAWETH:
		    return config;
#endif

#ifdef CCK_BUILD_TTRANSPORT_PCAP
	    case TT_TYPE_PCAP_ETHERNET:
		    return config;
#endif

#ifdef CCK_BUILD_TTRANSPORT_SOCK_RAWETH
	    case TT_TYPE_SOCKET_RAWETH:
		    return config;
#endif
	    default:
		ERROR("getEventTransportConfig(): Unsupported transport type %02x\n", type);
		break;
	}

failure:

	freeTTransportConfig(&config);

	return NULL;

}

/* set general transport options */
static TTransportConfig
*getGeneralTransportConfig(const int type, const GlobalConfig *global) {

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

	if(!setCommonTransportConfig(config, global)) {
	    freeTTransportConfig(&config);
	    return NULL;
	}

	config->timestamping = false;
	config->unmonitored = true;

	switch(type) {

#ifdef CCK_BUILD_TTRANSPORT_LINUXTS
	    case TT_TYPE_LINUXTS_UDPV4:
	    case TT_TYPE_LINUXTS_UDPV6:
#endif

#ifdef CCK_BUILD_TTRANSPORT_DLPI
	    case TT_TYPE_DLPI_UDPV4:
	    case TT_TYPE_DLPI_UDPV6:
#endif

#ifdef CCK_BUILD_TTRANSPORT_PCAP
	    case TT_TYPE_PCAP_UDPV4:
	    case TT_TYPE_PCAP_UDPV6:
#endif
	    case TT_TYPE_SOCKET_UDPV4:
	    case TT_TYPE_SOCKET_UDPV6:

		{
		    CCK_GET_PCONFIG(TTransport, udp_common, config, pConfig);

		    pConfig->listenPort = PTP_GENERAL_PORT;

		    return config;

		}

#ifdef CCK_BUILD_TTRANSPORT_PCAP
	    case TT_TYPE_PCAP_ETHERNET:
		    return config;
#endif

#ifdef CCK_BUILD_TTRANSPORT_SOCK_RAWETH
	    case TT_TYPE_SOCKET_RAWETH:
		    return config;
#endif

#ifdef CCK_BUILD_TTRANSPORT_LINUXTS
	    case TT_TYPE_LINUXTS_RAWETH:
		    return config;
#endif

	    default:
		ERROR("getGeneralTransportConfig(): Unsupported transport type %02x\n", type);
		break;
	}

	freeTTransportConfig(&config);

	return NULL;

}

/* receive and process data when ready to read (this is a CckFd callback) */
static void
ptpDataCallback(void *fd, void *owner) {

    TTransport *tr = owner;
    PtpClock *clock = tr->owner;
    TimeInternal timestamp = {0 ,0};

    bool looped;
    
    int len = tr->receiveMessage(tr, &tr->incomingMessage);

    if(len < 0 ) {
	clock->counters.messageRecvErrors++;
	ptpInternalFault(clock);
	return;
    }

    if(tr->incomingMessage.hasTimestamp) {
	timestamp.seconds = tr->incomingMessage.timestamp.seconds;
	timestamp.nanoseconds = tr->incomingMessage.timestamp.nanoseconds;
    }

    /* if this is a looped message, set destination to NULL so the protocol engine knows it is 'unknown' */
    looped = tr->incomingMessage.fromSelf && tr->incomingMessage.toSelf;
    processPtpData(clock, &timestamp, len, &tr->incomingMessage.from, looped ? NULL : &tr->incomingMessage.to);

}

static void setPtpNetworkInfo(PtpClock *ptpClock)
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
isPtpRegularData (void *transport, void *owner, char *data, const size_t len, bool isMulticast)
{

    if(len < HEADER_LENGTH) {
	return 1;
    }

    Enumeration4 msgType = (*(Enumeration4 *) (data + 0)) & 0x0F;

    if(msgType == MANAGEMENT) {
	DBG("isPtpRegularData: data received is a management message\n");
	return 0;
    }

    return 1;

}

int matchPtpData (void *owner, void *transport, char *a, const size_t alen, char *b, const size_t blen)
{

    if(alen != blen) {
	DBG("matchPtpData: a.len=%d != b.len=%d, discarding\n", alen, blen);
	return 0;
    }

    uint8_t msgtypea= (*(uint8_t*)(a)) & 0x0F;
    uint16_t seqnoa = *(uint16_t*)(a + 30);
    uint8_t msgtypeb= (*(uint8_t*)(b)) & 0x0F;
    uint16_t seqnob = *(uint16_t*)(b + 30);

    if( seqnoa != seqnob || msgtypea != msgtypeb ) {
	DBG("matchPtpData: a.seqno=%d, b.seqno=%d, a.msgtype=%d, b.msgtype=%d, no match, discarding\n",
	    ntohs(seqnoa), ntohs(seqnob), msgtypea, msgtypeb);
	return 0;
    }

	DBG("matchPtpData: a.seqno=%d, b.seqno=%d, a.msgtype=%d, b.msgtype=%d, match, permitting\n",
	    ntohs(seqnoa), ntohs(seqnob), msgtypea, msgtypeb);

    return 1;

}

static void
aclSetup(TTransport *t, const GlobalConfig *global) {

    t->dataAcl->config.disabled = !global->timingAclEnabled;
    t->controlAcl->config.disabled = !global->managementAclEnabled;

    if(global->timingAclEnabled) {
	t->dataAcl->compile(t->dataAcl, global->timingAclPermitText,
					    global->timingAclDenyText,
					    global->timingAclOrder);
    }

    if(global->managementAclEnabled) {
	t->controlAcl->compile(t->controlAcl, global->managementAclPermitText,
					    global->managementAclDenyText,
					    global->managementAclOrder);
    }

}

void configureAcls(PtpClock *ptpClock, const GlobalConfig *global) {

    TTransport *ev = ptpClock->eventTransport;
    TTransport *gen = ptpClock->generalTransport;

    if(ev) {
	aclSetup(ev, global);
    }

    if(gen && (ev != gen)) {
	aclSetup(gen, global);
    }

}

static int
parseUnicastDestinations(PtpClock *ptpClock, const GlobalConfig *global) {

    int found = 0;
    int total = 0;
    int tmp = 0;

    UnicastDestination *output = ptpClock->unicastDestinations;
    CckTransportAddress *addr;
    int family = getConfiguredFamily(global);

    CckAddressToolset *ts = getAddressToolset(family);

    if(!strlen(global->unicastDestinations)) {
	return 0;
    }

    foreach_token_begin(dst, global->unicastDestinations, htoken, DEFAULT_TOKEN_DELIM);

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

    foreach_token_begin(dom, global->unicastDomains, dtoken, DEFAULT_TOKEN_DELIM);

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

    foreach_token_begin(pref, global->unicastLocalPreference, ptoken, DEFAULT_TOKEN_DELIM);

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
bool configureUnicast(PtpClock *ptpClock, const GlobalConfig *global)
{
    /* no need to go further */
    if(global->transportMode != TMODE_UC) {
	return true;
    }

    ptpClock->unicastDestinationCount = parseUnicastDestinations(ptpClock, global);

    /* configure unicast peer address (exotic, I know) */
    if(global->delayMechanism == P2P) {

	if(!strlen(global->unicastPeerDestination)) {
	    ERROR("No P2P unicast destination configured");
	    return false;
	}

	ptpClock->unicastPeerDestination.protocolAddress =
	    createAddressFromString(getConfiguredFamily(global), global->unicastPeerDestination);

	if(!ptpClock->unicastPeerDestination.protocolAddress) {
	    ERROR("Invalid P2P unicast destination: %s\n", global->unicastPeerDestination);
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

	    if(ptpClock->global->delayMechanism == P2P) {
		ptpClock->peerEventDestination = createAddressFromString(TT_FAMILY_IPV4, PTP_MCAST_PEER_GROUP_IPV4);
		setTransportAddressPort(ptpClock->peerEventDestination, PTP_EVENT_PORT);

		ptpClock->peerGeneralDestination = createAddressFromString(TT_FAMILY_IPV4, PTP_MCAST_PEER_GROUP_IPV4);
		setTransportAddressPort(ptpClock->peerGeneralDestination, PTP_GENERAL_PORT);
	    }

	    break;

	case TT_FAMILY_IPV6:

	    ptpClock->eventDestination = createAddressFromString(TT_FAMILY_IPV6, PTP_MCAST_GROUP_IPV6);
	    setTransportAddressPort(ptpClock->eventDestination, PTP_EVENT_PORT);
	    setAddressScope_ipv6(ptpClock->eventDestination, ptpClock->global->ipv6Scope);

	    ptpClock->generalDestination = createAddressFromString(TT_FAMILY_IPV6, PTP_MCAST_GROUP_IPV6);
	    setTransportAddressPort(ptpClock->generalDestination, PTP_GENERAL_PORT);
	    setAddressScope_ipv6(ptpClock->generalDestination, ptpClock->global->ipv6Scope);

	    if(ptpClock->global->delayMechanism == P2P) {
		ptpClock->peerEventDestination = createAddressFromString(TT_FAMILY_IPV6, PTP_MCAST_PEER_GROUP_IPV6);
		setTransportAddressPort(ptpClock->peerEventDestination, PTP_EVENT_PORT);

		ptpClock->peerGeneralDestination = createAddressFromString(TT_FAMILY_IPV6, PTP_MCAST_PEER_GROUP_IPV6);
		setTransportAddressPort(ptpClock->peerGeneralDestination, PTP_GENERAL_PORT);
	    }
	    break;

	case TT_FAMILY_ETHERNET:

	    ptpClock->eventDestination = createAddressFromString(TT_FAMILY_ETHERNET, PTP_ETHER_DST);
	    ptpClock->generalDestination = createAddressFromString(TT_FAMILY_ETHERNET, PTP_ETHER_DST);

	    if(ptpClock->global->delayMechanism == P2P) {
		ptpClock->peerEventDestination = createAddressFromString(TT_FAMILY_ETHERNET, PTP_ETHER_PEER);
		ptpClock->peerGeneralDestination = createAddressFromString(TT_FAMILY_ETHERNET, PTP_ETHER_PEER);
	    }

    }

}

/* get event transport configuration flags */
int
getEventTransportFlags(const GlobalConfig *global) {

	int flags = 0;

	if(global->transportMode == TMODE_MC || global->transportMode == TMODE_MIXED) {
		flags |= TT_CAPS_MCAST;
	}

	if(global->transportMode == TMODE_UC || global->transportMode == TMODE_MIXED) {
			flags |= TT_CAPS_UCAST;
	}

	if(!global->hwTimestamping) {
	    flags |= TT_CAPS_PREFER_SW;
	}

    return flags;

}

/* get general transport configuration flags */
int getGeneralTransportFlags(const GlobalConfig *global) {

	int flags = 0;

	if(global->transportMode == TMODE_MC || global->transportMode == TMODE_MIXED) {
		flags |= TT_CAPS_MCAST;
	}

	if(global->transportMode == TMODE_UC || global->transportMode == TMODE_MIXED) {
			flags |= TT_CAPS_UCAST;
	}

    return flags;

}

static void
updatePtpRates (void *transport, void *owner)
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

static void
freeTransportData(PtpClock *ptpClock)
{

    TTransport *event = ptpClock->eventTransport;
    TTransport *general = ptpClock->generalTransport;

    freeTTransport(&event);
    freeTTransport(&general);

    ptpClock->eventTransport = NULL;
    ptpClock->generalTransport = NULL;

    FREE_ADDR(ptpClock->eventDestination);
    FREE_ADDR(ptpClock->generalDestination);
    FREE_ADDR(ptpClock->peerEventDestination);
    FREE_ADDR(ptpClock->peerGeneralDestination);

}

bool
initPtpTransports(PtpClock *ptpClock, CckFdSet *fdSet, const GlobalConfig *global)
{

    bool ret = true;
    int transportType;

    int gFlags = getGeneralTransportFlags(ptpClock->global);
    int eFlags = getEventTransportFlags(ptpClock->global);

    int family = getConfiguredFamily(global);

    freeTransportData(ptpClock);

    if(!testInterface(global->ifName, family, global->sourceAddress)) {
	return false;
    }

    if(!configureUnicast(ptpClock, global)) {
	ERROR("initPtpTransports(): Unicast destination configuration failed\n");
	return false;
    }

    transportType = detectTTransport(family, global->ifName, eFlags, global->transportType);

    if(transportType == TT_TYPE_NONE) {
	ERROR("NetInit(): No usable transport found\n");
	return FALSE;
    }

    TTransportConfig *eventConfig = getEventTransportConfig(transportType, global);
    TTransportConfig *generalConfig = getGeneralTransportConfig(transportType, global);

    if(!eventConfig || !generalConfig) {
	CRITICAL("initPtpTransports(): Could not configure network transports\n");
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
	CRITICAL("initPtpTransports(): Could not create network transports\n");
	return FALSE;
    }

    event->setBuffers(event, ptpClock->msgIbuf, PACKET_SIZE, ptpClock->msgObuf, PACKET_SIZE);
    event->callbacks.preTx = ptpPreSend;
    event->callbacks.isRegularData = isPtpRegularData;
    event->callbacks.matchData = matchPtpData;
    event->myFd.callbacks.onData = ptpDataCallback;
    event->callbacks.onRateUpdate = updatePtpRates;
    event->callbacks.onNetworkChange = ptpNetworkChange;
    event->callbacks.onNetworkFault = ptpNetworkFault;
    event->callbacks.onClockDriverChange = ptpClockDriverChange;

    event->owner = ptpClock;
    if(event->init(event, eventConfig, fdSet) < 1) {
	CRITICAL("Could not start event transport\n");
	ret = FALSE;
	goto gameover;
    }

    freeTTransportConfig(&eventConfig);

    if(event != general) {
	general->setBuffers(general, ptpClock->msgIbuf, PACKET_SIZE, ptpClock->msgObuf, PACKET_SIZE);
	general->callbacks.preTx = ptpPreSend;
	general->callbacks.isRegularData = isPtpRegularData;
	general->myFd.callbacks.onData = ptpDataCallback;
	general->owner = ptpClock;
	event->slaveTransport = general;
	if(general->init(general, generalConfig, fdSet) < 1) {
	    CRITICAL("Could not start general transport\n");
	    ret = FALSE;
	    goto gameover;
	}

	freeTTransportConfig(&generalConfig);
    }

    configureAcls(ptpClock, global);

    controlClockDrivers(CD_NOTINUSE, NULL);

    if(!prepareClockDrivers(ptpClock, global)) {
	ERROR("Cannot start clock drivers - aborting mission\n");
	exit(-1);
    }

    /* inform network about the protocol and address we are using */
    setPtpNetworkInfo(ptpClock);

    /* create any extra clocks */
    if(!createClockDriversFromString(global->extraClocks, configureClockDriver, global, false)) {
	ERROR("Failed to create extra clocks\n");
	ret = FALSE;
	goto gameover;
    }

    /* check if we want a master clock */
    ClockDriver *masterClock = NULL;

    if(strlen(global->masterClock) > 0) {
	masterClock = getClockDriverByName(global->masterClock);
	if(masterClock == NULL) {
	    WARNING("Could not find designated master clock: %s\n", global->masterClock);
	}
    }

    setCckMasterClock(masterClock);

    /* clean up unused clock drivers */
    controlClockDrivers(CD_CLEANUP, NULL);

    reconfigureClockDrivers(configureClockDriver, global);

    return ret;

gameover:

    freeTransportData(ptpClock);

    return ret;

}

void
shutdownPtpTransports(PtpClock *ptpClock)
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

    FREE_ADDR(ptpClock->eventDestination);
    FREE_ADDR(ptpClock->generalDestination);
    FREE_ADDR(ptpClock->peerEventDestination);
    FREE_ADDR(ptpClock->peerGeneralDestination);

}

int
getConfiguredFamily(const GlobalConfig *global)
{

	int family = global->networkProtocol;
	if(global->transportType > TT_TYPE_NONE) {
	    family = getTTransportTypeFamily(global->transportType);
	}

	return family;

}

bool
testTransportConfig(const GlobalConfig *global)
{

	int family = getConfiguredFamily(global);

	if(!testInterface(global->ifName, family, global->sourceAddress) ||
		(detectTTransport(family, global->ifName,
		    getEventTransportFlags(global), global->transportType) == TT_TYPE_NONE)) {
	    ERROR("Error: Cannot use interface '%s'\n",global->ifName);
	    return false;
	}

	return true;

}

static void
ptpNetworkFault(void *transport, void *owner, const bool fault)
{

    PtpClock *ptpClock = owner;

    if(fault) {
	SET_ALARM(ALRM_NETWORK_FLT, TRUE);
	toState(PTP_FAULTY, ptpClock->global, ptpClock);
    } else {
	SET_ALARM(ALRM_NETWORK_FLT, FALSE);
	toState(PTP_INITIALIZING, ptpClock->global, ptpClock);
    }

}

static void
ptpNetworkChange(void *transport, void *owner, const bool major)
{

    PtpClock *port = owner;

    if(major) {
	NOTICE("Major transport change: re-initialising PTP\n");
	setPtpClockIdentity(port);
	setPtpNetworkInfo(port);
	toState(PTP_INITIALIZING, port->global, port);
    } else {

	recalibrateClock(port, true);

    }

}


static int
ptpClockDriverChange (void *transport, void *owner)
{

    /* skip one offset */
    int skipSync = 1;

    PtpClock *port = owner;

    prepareClockDrivers(port, port->global);

    /* all drivers skip one beat */
    controlClockDrivers(CD_SKIPSYNC, &skipSync);

    return 1;

}
