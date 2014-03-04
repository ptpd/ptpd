/*-
 * Copyright (c) 2014	   Wojciech Owczarek,
 *			   George V. Neville-Neil
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

#ifdef PTPD_PCAP
#ifdef HAVE_PCAP_PCAP_H
#include <pcap/pcap.h>
#else /* !HAVE_PCAP_PCAP_H */
/* Cases like RHEL5 and others where only pcap.h exists */
#ifdef HAVE_PCAP_H
#include <pcap.h>
#endif /* HAVE_PCAP_H */
#endif
#define PCAP_TIMEOUT 1 /* expressed in milliseconds */
#endif

#if defined PTPD_SNMP
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#endif

/* choose kernel-level nanoseconds or microseconds resolution on the client-side */
#if !defined(SO_TIMESTAMPING) && !defined(SO_TIMESTAMPNS) && !defined(SO_TIMESTAMP) && !defined(SO_BINTIME)
#error No kernel-level support for packet timestamping detected!
#endif

#ifdef SO_TIMESTAMPING
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#endif /* SO_TIMESTAMPING */

/* shut down network transports */
Boolean 
netShutdown(RunTimeOpts* rtOpts, PtpClock* ptpClock)
{

	freeCckTransport(&ptpClock->generalTransport);
	freeCckTransport(&ptpClock->eventTransport);

	if(rtOpts->delayMechanism == P2P) {
	    freeCckTransport(&ptpClock->peerGeneralTransport);
	    freeCckTransport(&ptpClock->peerEventTransport);
	}

	freeCckAcl(&ptpClock->timingAcl);
	freeCckAcl(&ptpClock->managementAcl);

	return TRUE;
}

/* Check if interface ifaceName exists. Return 1 on success, 0 when interface doesn't exists, -1 on failure.
 */

static int
interfaceExists(char* ifaceName)
{

    int ret;
    struct ifaddrs *ifaddr, *ifa;

    if(!strlen(ifaceName)) {
	DBG("interfaceExists called for an empty interface!");
	return 0;
    }

    if(getifaddrs(&ifaddr) == -1) {
	PERROR("Could not get interface list");
	ret = -1;
	goto end;

    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

	if(!strcmp(ifaceName, ifa->ifa_name)) {
	    ret = 1;
	    goto end;
	}

    }

    ret = 0;
    DBG("Interface not found: %s\n", ifaceName);

end:
   freeifaddrs(ifaddr);
    return ret;
}

static int
getInterfaceFlags(char* ifaceName, unsigned int* flags)
{

    int ret;
    struct ifaddrs *ifaddr, *ifa;

    if(!strlen(ifaceName)) {
	DBG("interfaceExists called for an empty interface!");
	return 0;
    }

    if(getifaddrs(&ifaddr) == -1) {
	PERROR("Could not get interface list");
	ret = -1;
	goto end;

    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

	if(!strcmp(ifaceName, ifa->ifa_name)) {
	    *flags = ifa->ifa_flags;
	    ret = 1;
	    goto end;
	}

    }

    ret = 0;
    DBG("Interface not found: %s\n", ifaceName);

end:
   freeifaddrs(ifaddr);
    return ret;
}


/* Try getting addr address of family family from interface ifaceName.
   Return 1 on success, 0 when no suitable address available, -1 on failure.
 */
static int
getInterfaceAddress(char* ifaceName, int family, struct sockaddr* addr) {

    int ret;
    struct ifaddrs *ifaddr, *ifa;

    if(getifaddrs(&ifaddr) == -1) {
	PERROR("Could not get interface list");
	ret = -1;
	goto end;

    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

	if(!strcmp(ifaceName, ifa->ifa_name) && ifa->ifa_addr->sa_family == family) {

		memcpy(addr, ifa->ifa_addr, sizeof(struct sockaddr));

    		ret = 1;
    		goto end;

	}

    }

    ret = 0;
    DBG("Interface not found: %s\n", ifaceName);

end:

    freeifaddrs(ifaddr);
    return ret;
}


/* Try getting hwAddrSize bytes of ifaceName hardware address,
   and place them in hwAddr. Return 1 on success, 0 when no suitable
   hw address available, -1 on failure.
 */
static int
getHwAddress (char* ifaceName, unsigned char* hwAddr, int hwAddrSize)
{

    int ret;
    if(!strlen(ifaceName))
	return 0;

/* BSD* - AF_LINK gives us access to the hw address via struct sockaddr_dl */
#ifdef AF_LINK

    struct ifaddrs *ifaddr, *ifa;

    if(getifaddrs(&ifaddr) == -1) {
	PERROR("Could not get interface list");
	ret = -1;
	goto end;

    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

	if(!strcmp(ifaceName, ifa->ifa_name) && ifa->ifa_addr->sa_family == AF_LINK) {

		struct sockaddr_dl* sdl = (struct sockaddr_dl *)ifa->ifa_addr;
		if(sdl->sdl_type == IFT_ETHER || sdl->sdl_type == IFT_L2VLAN) {

			memcpy(hwAddr, LLADDR(sdl),
			hwAddrSize <= sizeof(sdl->sdl_data) ?
			hwAddrSize : sizeof(sdl->sdl_data));
			ret = 1;
			goto end;
		} else {
			DBGV("Unsupported hardware address family on %s\n", ifaceName);
			ret = 0;
			goto end;
		}
	}

    }

    ret = 0;
    DBG("Interface not found: %s\n", ifaceName);

end:

    freeifaddrs(ifaddr);
    return ret;

/* Linux, basically */
#else

    int sockfd;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
	PERROR("Could not open test socket");
	return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));

    strncpy(ifr.ifr_name, ifaceName, IFACE_NAME_LENGTH);

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
            DBGV("failed to request hardware address for %s", ifaceName);
	    ret = -1;
	    goto end;
    }


    int af = ifr.ifr_hwaddr.sa_family;

    if (    af == ARPHRD_ETHER
	 || af == ARPHRD_IEEE802
#ifdef ARPHRD_INFINIBAND
	 || af == ARPHRD_INFINIBAND
#endif
	) {
	    memcpy(hwAddr, ifr.ifr_hwaddr.sa_data, hwAddrSize);
	    ret = 1;
	} else {
	    DBGV("Unsupported hardware address family on %s\n", ifaceName);
	    ret = 0;
	}
end:
    close(sockfd);
    return ret;

#endif /* AF_LINK */

}

static Boolean getInterfaceInfo(char* ifaceName, InterfaceInfo* ifaceInfo)
{

    int res;

    res = interfaceExists(ifaceName);

    if (res == -1) {

	return FALSE;

    } else if (res == 0) {

	ERROR("Interface %s does not exist.\n", ifaceName);
	return FALSE;
    }

    res = getInterfaceAddress(ifaceName, ifaceInfo->addressFamily, &ifaceInfo->afAddress);

    if (res == -1) {

	return FALSE;

    }

    ifaceInfo->hasAfAddress = res;

    res = getHwAddress(ifaceName, (unsigned char*)ifaceInfo->hwAddress, 6);

    if (res == -1) {

	return FALSE;

    }

    ifaceInfo->hasHwAddress = res;

    res = getInterfaceFlags(ifaceName, &ifaceInfo->flags);

    if (res == -1) {

	return FALSE;

    }

    return TRUE;

}

#include <linux/if_packet.h>

Boolean
testInterface(char * ifaceName, RunTimeOpts* rtOpts)
{

	InterfaceInfo info;

	info.addressFamily = AF_INET;

	if(getInterfaceInfo(ifaceName, &info) != 1)
		return FALSE;

	switch(rtOpts->transport) {

	    case UDP_IPV4:
		if(!info.hasAfAddress) {
		    ERROR("Interface %s has no IPv4 address set\n", ifaceName);
		    return FALSE;
		}
		break;

	    case IEEE_802_3:
		if(!info.hasHwAddress) {
		    ERROR("Interface %s has no supported hardware address - possibly not an Ethernet interface\n", ifaceName);
		    return FALSE;
		}
		break;

	    default:
		ERROR("Unsupported transport: %d\n", rtOpts->transport);
		return FALSE;

	}

    if(!(info.flags & IFF_UP) || !(info.flags & IFF_RUNNING))
	    WARNING("Interface %s seems to be down. PTPd will not operate correctly until it's up.\n", ifaceName);

    if(info.flags & IFF_LOOPBACK)
	    WARNING("Interface %s is a loopback interface.\n", ifaceName);

    if(!(info.flags & IFF_MULTICAST) 
	    && rtOpts->transport==UDP_IPV4 
	    && rtOpts->transport_mode != TRANSPORTMODE_UNICAST) {
	    WARNING("Interface %s is not multicast capable.\n", ifaceName);
    }

	return TRUE;

}

Boolean
hostLookup(const char* hostname, Integer32* addr)
{
	if (hostname[0]) {
		/* Attempt a DNS lookup first. */
		struct hostent *host;
		host = gethostbyname2(hostname, AF_INET);
                if (host != NULL) {
			if (host->h_length != 4) {
				PERROR("unicast host resolved to non ipv4"
				       "address");
				return FALSE;
			}
			*addr = 
				*(uint32_t *)host->h_addr_list[0];
			return TRUE;
		} else {
			struct in_addr netAddr;
			/* Maybe it's a dotted quad. */
			if (!inet_aton(hostname, &netAddr)) {
				ERROR("failed to encode unicast address: %s\n",
				      hostname);
				return FALSE;
				*addr = netAddr.s_addr;
				return TRUE;
			}
                }
	}

return FALSE;

}


/**
 * Init all network transports
 *
 * @param netPath 
 * @param rtOpts 
 * @param ptpClock 
 * 
 * @return TRUE if successful
 */
Boolean 
netInit(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{

	CckTransportConfig config;

	memset(&config, 0, sizeof(CckTransportConfig));

	config.transportType = CCK_TRANSPORT_UDP_IPV4;
	strncpy(config.transportEndpoint, rtOpts->ifaceName, IFACE_NAME_LENGTH);

	/* create standard transports */

	ptpClock->eventTransport = createCckTransport(CCK_TRANSPORT_UDP_IPV4, "event");
	ptpClock->generalTransport = createCckTransport(CCK_TRANSPORT_UDP_IPV4, "general");

	ptpClock->eventTransport->unicastCallback = setPtpUnicastFlags;
	ptpClock->generalTransport->unicastCallback = setPtpUnicastFlags;

	ptpClock->eventTransport->watcher = &ptpClock->watcher;
	ptpClock->generalTransport->watcher = &ptpClock->watcher;

	/* init event transport */

	config.sourceId = PTP_EVENT_PORT;
	config.destinationId = PTP_EVENT_PORT;
	config.swTimestamping = CCK_TRUE;
	config.param1 = rtOpts->ttl;
	config.param2 = rtOpts->dscpValue;

	if (!(ptpClock->eventTransport->addressFromString(DEFAULT_PTP_IPV4_ADDRESS, &config.defaultDestination)))
		return FALSE;
	if((ptpClock->eventTransport->init(ptpClock->eventTransport, &config)) < 1)
	    return FALSE;

	/* init general transport */
	config.sourceId = PTP_GENERAL_PORT;
	config.destinationId = PTP_GENERAL_PORT;
	config.swTimestamping = CCK_FALSE;

	if((ptpClock->generalTransport->init(ptpClock->generalTransport, &config)) < 1)
	    return FALSE;

	/* init P2P transports */
	if(rtOpts->delayMechanism == P2P) {

	    ptpClock->peerEventTransport = createCckTransport(CCK_TRANSPORT_UDP_IPV4, "peerEvent");
	    ptpClock->peerGeneralTransport = createCckTransport(CCK_TRANSPORT_UDP_IPV4, "peerGeneral");
	    ptpClock->eventTransport->unicastCallback = setPtpUnicastFlags;
	    ptpClock->generalTransport->unicastCallback = setPtpUnicastFlags;
	    ptpClock->peerEventTransport->watcher = &ptpClock->watcher;
	    ptpClock->peerGeneralTransport->watcher = &ptpClock->watcher;

	    /* peer event transport config - ttl 1 */
	    config.sourceId = PTP_EVENT_PORT;
	    config.destinationId = PTP_EVENT_PORT;
	    config.swTimestamping = CCK_TRUE;
	    config.param1 = 1;

	    if (!(ptpClock->peerEventTransport->addressFromString(PEER_PTP_IPV4_ADDRESS,
							    &config.defaultDestination)))
		return FALSE;
	    if((ptpClock->peerEventTransport->init(ptpClock->peerEventTransport, &config)) < 1)
		return FALSE;

	    config.sourceId = PTP_GENERAL_PORT;
	    config.destinationId = PTP_GENERAL_PORT;
	    config.swTimestamping = CCK_FALSE;

	    if (!(ptpClock->peerGeneralTransport->addressFromString(PEER_PTP_IPV4_ADDRESS,
							    &config.defaultDestination)))
		return FALSE;
	    if((ptpClock->peerGeneralTransport->init(ptpClock->peerGeneralTransport, &config)) < 1)
		return FALSE;

	}

	/* Compile ACLs */
	if(rtOpts->timingAclEnabled) {
    		freeCckAcl(&ptpClock->timingAcl);
		ptpClock->timingAcl = createCckAcl(rtOpts->transportType,
						    rtOpts->timingAclOrder, "timingAcl");
		ptpClock->timingAcl->compileAcl(ptpClock->timingAcl,
						rtOpts->timingAclPermitText,
						rtOpts->timingAclDenyText);
	}
	if(rtOpts->managementAclEnabled) {
    		freeCckAcl(&ptpClock->managementAcl);
		ptpClock->managementAcl = createCckAcl(rtOpts->transportType,
						    rtOpts->managementAclOrder, "managementAcl");
		ptpClock->managementAcl->compileAcl(ptpClock->managementAcl,
						rtOpts->managementAclPermitText,
						rtOpts->managementAclDenyText);
	}

	return TRUE;
}

/*Check if data has been received*/
int 
netSelect(TimeInternal * timeout, NetPath * netPath, fd_set *readfds)
{
	int ret = 0;
#if defined PTPD_SNMP
	int nfds;
	struct timeval tv, *tv_ptr;

	extern RunTimeOpts rtOpts;
	struct timeval snmp_timer_wait = { 0, 0}; // initialise to avoid unused warnings when SNMP disabled
	int snmpblock = 0;

if (rtOpts.snmp_enabled) {
	snmpblock = 1;
	if (tv_ptr) {
		snmpblock = 0;
		memcpy(&snmp_timer_wait, tv_ptr, sizeof(struct timeval));
	}
	snmp_select_info(&nfds, readfds, &snmp_timer_wait, &snmpblock);
	if (snmpblock == 0)
		tv_ptr = &snmp_timer_wait;

	ret = select(nfds, readfds, 0, 0, tv_ptr);

	if (ret < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return 0;
	}

	/* Maybe we have received SNMP related data */
	if (ret > 0) {
		snmp_read(readfds);
	} else if (ret == 0) {
		snmp_timeout();
		run_alarms();
	}
	netsnmp_check_outstanding_agent_requests();

}
#endif
	return ret;
}

Boolean
netRefresh(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	DBG("netRefresh\n");

	ptpClock->eventTransport->refresh(ptpClock->eventTransport);
	ptpClock->generalTransport->refresh(ptpClock->generalTransport);

	if(rtOpts->delayMechanism == P2P) {
	    ptpClock->peerEventTransport->refresh(ptpClock->peerEventTransport);
	    ptpClock->peerGeneralTransport->refresh(ptpClock->peerGeneralTransport);
	}

	return TRUE;
}
