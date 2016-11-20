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

#include "linux/if_bonding.h"
#include "linux/if_vlan.h"

static Boolean bondQuery(char *ifaceName, ifbond *ifb);
static Boolean bondSlaveQuery(char *ifaceName, ifslave *ifs, int member);
static void getBondInfo(char *ifaceName, BondInfo *info);
static void getVlanInfo(char* ifaceName, VlanInfo *info);
static Boolean getHwTs(const char *ifaceName, const RunTimeOpts *rtOpts, HwTsInfo *target, Boolean quiet);
static Boolean initHwTs(char *ifaceName, HwTsInfo *info);
static ssize_t netr(Octet * buf, TimeInternal * time, NetPath * netPath, int flags);

/**
 * shutdown the IPv4 multicast for specific address
 *
 * @param netPath
 * @param multicastAddr
 *
 * @return TRUE if successful
 */
static Boolean
netShutdownMulticastIPv4(NetPath * netPath, Integer32 multicastAddr)
{
	struct ip_mreq imr;

	if(!multicastAddr || !netPath->interfaceAddr.s_addr) {
		return TRUE;
	}

	/* Close General Multicast */
	imr.imr_multiaddr.s_addr = multicastAddr;
	imr.imr_interface.s_addr = netPath->interfaceAddr.s_addr;

	setsockopt(netPath->eventSock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		   &imr, sizeof(struct ip_mreq));
	setsockopt(netPath->generalSock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		   &imr, sizeof(struct ip_mreq));

	return TRUE;
}

/**
 * shutdown the multicast (both General and Peer)
 *
 * @param netPath
 *
 * @return TRUE if successful
 */
static Boolean
netShutdownMulticast(NetPath * netPath)
{
	/* Close General Multicast */
	netShutdownMulticastIPv4(netPath, netPath->multicastAddr);
	netPath->multicastAddr = 0;

	/* Close Peer Multicast */
	netShutdownMulticastIPv4(netPath, netPath->peerMulticastAddr);
	netPath->peerMulticastAddr = 0;
	
	return TRUE;
}

/*
 * For future use: Check if IPv4 address is multiast -
 * If first 4 bits of an address are 0xE (1110), it's multicast
 */
/*
static Boolean
isIpMulticast(struct in_addr in)
{
        if((ntohl(in.s_addr) >> 28) == 0x0E )
	    return TRUE;
	return FALSE;
}
*/

/* shut down the UDP stuff */
Boolean
netShutdown(NetPath * netPath, PtpClock *ptpClock)
{

	netShutdownMulticast(netPath);

	/* Close sockets */
	if (netPath->eventSock >= 0)
		close(netPath->eventSock);
	netPath->eventSock = -1;

	if (netPath->generalSock >= 0)
		close(netPath->generalSock);
	netPath->generalSock = -1;

#ifdef PTPD_PCAP
	if (netPath->pcapEvent != NULL) {
		pcap_close(netPath->pcapEvent);
		netPath->pcapEventSock = -1;
	}
	if (netPath->pcapGeneral != NULL) {
		pcap_close(netPath->pcapGeneral);
		netPath->pcapGeneralSock = -1;
	}
#endif

	freeIpv4AccessList(&netPath->timingAcl);
	freeIpv4AccessList(&netPath->managementAcl);

	return TRUE;
}

/* Check if interface ifaceName exists. Return 1 on success, 0 when interface doesn't exists, -1 on failure.
 */

int
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
	if(ifa->ifa_name == NULL) continue;
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
	if(ifa->ifa_name == NULL) continue;
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

	if(ifa->ifa_name == NULL) continue;
	/* ifa_addr not always present - link layer may come first */
	if(ifa->ifa_addr == NULL) continue;

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
#if defined(AF_LINK) && !defined(__sun)

    struct ifaddrs *ifaddr, *ifa;

    if(getifaddrs(&ifaddr) == -1) {
	PERROR("Could not get interface list");
	ret = -1;
	goto end;

    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
	if(ifa->ifa_name == NULL) continue;
	if(ifa->ifa_addr == NULL) continue;
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


#else
/* Linux and Solaris family which also have SIOCGIFHWADDR/SIOCGLIFHWADDR */
    int sockfd;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
	PERROR("Could not open test socket");
	return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, ifaceName, IFACE_NAME_LENGTH);

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
            DBGV("failed to request hardware address for %s", ifaceName);
	    ret = -1;
	    goto end;
    }

#ifdef HAVE_STRUCT_IFREQ_IFR_HWADDR
    int af = ifr.ifr_hwaddr.sa_family;
#else
    int af = ifr.ifr_addr.sa_family;
#endif /* HAVE_STRUCT_IFREQ_IFR_HWADDR */

    if (    af == ARPHRD_ETHER
	 || af == ARPHRD_IEEE802
#ifdef ARPHRD_INFINIBAND
	 || af == ARPHRD_INFINIBAND
#endif
	) {
#ifdef HAVE_STRUCT_IFREQ_IFR_HWADDR
	    memcpy(hwAddr, ifr.ifr_hwaddr.sa_data, hwAddrSize);
#else
    	    memcpy(hwAddr, ifr.ifr_addr.sa_data, hwAddrSize);
#endif /* HAVE_STRUCT_IFREQ_IFR_HWADDR */

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

static int getInterfaceIndex(char *ifaceName)
{

#ifndef SIOCGIFINDEX
    return -1;
#else
    int sockfd;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
	PERROR("Could not retrieve interface index for %s",ifaceName);
	return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, ifaceName, IFACE_NAME_LENGTH);

    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
            DBGV("failed to request hardware address for %s", ifaceName);
	    close(sockfd);
	    return -1;

    }

    close(sockfd);

#if defined(HAVE_STRUCT_IFREQ_IFR_INDEX)
    return ifr.ifr_index;
#elif defined(HAVE_STRUCT_IFREQ_IFR_IFINDEX)
    return ifr.ifr_ifindex;
#else
    return 0;
#endif

#endif /* !SIOCGIFINDEX */

}

static Boolean getInterfaceInfo(char* ifaceName, InterfaceInfo* ifaceInfo)
{

    int res;

    char *realDevice = ifaceName;

    BondInfo *bondInfo = &ifaceInfo->bondInfo;
    VlanInfo *vlanInfo = &ifaceInfo->vlanInfo;

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

    res  = getInterfaceIndex(ifaceName);

    if(res == -1) {
	ifaceInfo->ifIndex = 0;
    } else {
	ifaceInfo->ifIndex = res;
    }

    getVlanInfo(ifaceName, vlanInfo);

    if(vlanInfo->vlan) {
	    realDevice = vlanInfo->realDevice;

    }

    getBondInfo(realDevice, bondInfo);

    if(bondInfo->bonded && bondInfo->activeCount > 0) {
	    realDevice = bondInfo->activeSlave.name;
    }

    strncpy(ifaceInfo->physicalDevice, realDevice, IFACE_NAME_LENGTH);

    DBG("Underlying physical device: %s\n", ifaceInfo->physicalDevice);

    return TRUE;


}

Boolean
testInterface(char * ifaceName, const RunTimeOpts* rtOpts)
{
	char *realDevice = ifaceName;
	InterfaceInfo info;
	memset(&info, 0, sizeof(InterfaceInfo));
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

    if(info.vlanInfo.vlan) {
	    realDevice = info.vlanInfo.realDevice;
	    INFO("%s is a VLAN interface, VLAN ID %d, underlying device %s\n", ifaceName, info.vlanInfo.vlanId, info.vlanInfo.realDevice);
    }

    if(info.bondInfo.bonded) {
	    if(!info.bondInfo.activeBackup && rtOpts->hwTimestamping) {
		WARNING("%s is a bonded interface but is not running Active Backup. Cannot use hardware timestamping\n", ifaceName);
	    }

	    if(info.bondInfo.activeCount == 0) {
		WARNING("%s is a bonded interface with no active slaves\n");
	    } else {
		INFO("%s is a bonded interface, current active slave: %s\n", realDevice, info.bondInfo.activeSlave.name);
	    }
    }

    if(strcmp(ifaceName, info.physicalDevice)) {
	    INFO("Underlying physical device: %s\n", info.physicalDevice);
    }

    if(!(info.flags & IFF_MULTICAST)
	    && rtOpts->transport==UDP_IPV4
	    && rtOpts->ipMode != IPMODE_UNICAST) {
	    WARNING("Interface %s is not multicast capable.\n", ifaceName);
    }

	return TRUE;

}

/**
 * Init the multcast for specific IPv4 address
 *
 * @param netPath
 * @param multicastAddr
 *
 * @return TRUE if successful
 */
static Boolean
netInitMulticastIPv4(NetPath * netPath, Integer32 multicastAddr)
{
	struct ip_mreq imr;

	/* multicast send only on specified interface */
	imr.imr_multiaddr.s_addr = multicastAddr;
	imr.imr_interface.s_addr = netPath->interfaceAddr.s_addr;

	if (setsockopt(netPath->eventSock, IPPROTO_IP, IP_MULTICAST_IF,
		       &netPath->interfaceAddr, sizeof(struct in_addr)) < 0
	    || setsockopt(netPath->generalSock, IPPROTO_IP, IP_MULTICAST_IF,
			  &netPath->interfaceAddr, sizeof(struct in_addr))
	    < 0) {
		PERROR("error while setting outgoig multicast interface "
			"(IP_MULTICAST_IF)");
		return FALSE;
	}
	/* join multicast group (for receiving) on specified interface */
	if (setsockopt(netPath->eventSock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       &imr, sizeof(struct ip_mreq)) < 0
	    || setsockopt(netPath->generalSock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			  &imr, sizeof(struct ip_mreq)) < 0) {
		PERROR("failed to join the multicast group");
		return FALSE;
	}
	return TRUE;
}

/**
 * Init the multcast (both General and Peer)
 *
 * @param netPath
 * @param rtOpts
 *
 * @return TRUE if successful
 */
static Boolean
netInitMulticast(NetPath * netPath,  const RunTimeOpts * rtOpts)
{
	struct in_addr netAddr;
	char addrStr[NET_ADDRESS_LENGTH+1];

	/* do not join multicast in unicast mode */
	if(rtOpts->ipMode == IPMODE_UNICAST)
		return TRUE;

	/* Init General multicast IP address */
	strncpy(addrStr, DEFAULT_PTP_DOMAIN_ADDRESS, NET_ADDRESS_LENGTH);
	if (!inet_aton(addrStr, &netAddr)) {
		ERROR("failed to encode multicast address: %s\n", addrStr);
		return FALSE;
	}

	/* this allows for leaving groups only if joined */
	netPath->joinedGeneral = TRUE;

	netPath->multicastAddr = netAddr.s_addr;
	if(!netInitMulticastIPv4(netPath, netPath->multicastAddr)) {
		return FALSE;
	}

	/* End of General multicast Ip address init */

	if(rtOpts->delayMechanism != P2P) {
		return TRUE;
	}

	/* Init Peer multicast IP address */
	strncpy(addrStr, PEER_PTP_DOMAIN_ADDRESS, NET_ADDRESS_LENGTH);
	if (!inet_aton(addrStr, &netAddr)) {
		ERROR("failed to encode multicast address: %s\n", addrStr);
		return FALSE;
	}

	/* track if we have joined the p2p mcast group */
	netPath->joinedPeer = TRUE;

	netPath->peerMulticastAddr = netAddr.s_addr;
	if(!netInitMulticastIPv4(netPath, netPath->peerMulticastAddr)) {
		return FALSE;
	}
	/* End of Peer multicast Ip address init */
	
	return TRUE;
}

static Boolean
netSetMulticastTTL(int sockfd, int ttl) {

#if defined(__OpenBSD__) || defined(__sun)
	uint8_t temp = (uint8_t) ttl;
#else
	int temp = ttl;
#endif

	if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
		       &temp, sizeof(temp)) < 0) {
	    PERROR("Failed to set socket multicast time-to-live");
	    return FALSE;
	}
	return TRUE;
}

static Boolean
netSetMulticastLoopback(NetPath * netPath, Boolean value) {
#if defined(__OpenBSD__) || defined(__sun)
	uint8_t temp = value ? 1 : 0;
#else
	int temp = value ? 1 : 0;
#endif
	DBG("Going to set multicast loopback with %d \n", temp);

	if (setsockopt(netPath->eventSock, IPPROTO_IP, IP_MULTICAST_LOOP,
	       &temp, sizeof(temp)) < 0) {
		PERROR("Failed to set multicast loopback");
		return FALSE;
	}
	
	return TRUE;
}

#if defined(SO_TIMESTAMPING) && defined(SO_TIMESTAMPNS)
static Boolean
getTxTimestamp(NetPath* netPath,TimeInternal* timeStamp) {
	extern PtpClock *G_ptpClock;
	ssize_t length;
	fd_set tmpSet;
	struct timeval timeOut = {0,1500};
	int i = 0;
	int backoff = 10;
				clearTime(timeStamp);
	FD_ZERO(&tmpSet);
	FD_SET(netPath->eventSock, &tmpSet);

	if(select(netPath->eventSock + 1, &tmpSet, NULL, NULL, &timeOut) > 0) {
		if (FD_ISSET(netPath->eventSock, &tmpSet)) {
			length = netr(G_ptpClock->msgIbuf, timeStamp,
			    netPath, MSG_ERRQUEUE);
			if (length > 0) {
				DBG("getTxTimestamp: Grabbed sent msg via errqueue: %d bytes, at %d.%d\n", length, timeStamp->seconds, timeStamp->nanoseconds);
				return TRUE;
			} else if (length < 0) {
				clearTime(timeStamp);
				G_ptpClock->counters.txTimestampFailures++;
				    ERROR("getTxTimestamp: Failed to poll error queue for SO_TIMESTAMPING transmit time: %s\n", strerror(errno));
				return FALSE;
			} else if (length == 0) {
				DBG("getTxTimestamp: Received no data from TX error queue, retrying\n");
			}
		}
	}

	/* we're desperate here, aren't we... */

	for(i = 0; i <= LATE_TXTIMESTAMP_RETRIES; i++) {
	    DBG("getTxTimestamp backoff: %d\n", backoff);
	    length = netr(G_ptpClock->msgIbuf, timeStamp, netPath, MSG_ERRQUEUE);
	    if(length > 0) {
		DBG("getTxTimestamp: SO_TIMESTAMPING - delayed TX timestamp caught after %d retries\n", i+1);
		return TRUE;
	    }
	    usleep(backoff);
	    backoff *= 2;
	}

	DBG("getTxTimestamp: NIC failed to deliver TX timestamp in time\n");
	clearTime(timeStamp);
	G_ptpClock->counters.txTimestampFailures++;
	return FALSE;

}
#endif /* SO_TIMESTAMPING */


static Boolean
netInitHwTimestamping(NetPath *netPath, const RunTimeOpts *rtOpts) {

	HwTsInfo info, primaryInfo;
	BondInfo *bi = &netPath->interfaceInfo.bondInfo;
	char *primaryName = netPath->interfaceInfo.physicalDevice;
	char *ifaceName;

	Boolean ret = TRUE;

	memset(&primaryInfo, 0, sizeof(HwTsInfo));

	if(!getHwTs((const char*)primaryName, rtOpts, &primaryInfo, FALSE) || !initHwTs(primaryName, &primaryInfo)) {
	    ret = FALSE;
	}

	if(bi->bonded && !bi->activeBackup) {
	    ret = FALSE;
	    WARNING("%s is a bonded interface but is not running active backup. Cannot enable hardware timestamping.\n",
		    rtOpts->ifaceName);
	} else if (bi->bonded) {

	    for(int i = 0; i < bi->slaveCount; i++) {
		ifaceName = bi->slaves[i].name;
		if(strlen(ifaceName)) {

		    if(bi->slaves[i].id == bi->activeSlave.id) {
			continue;
		    }
		    memset(&info, 0, sizeof(HwTsInfo));

		    if(!getHwTs((const char*)ifaceName, rtOpts, &info, FALSE)) {
			ret = FALSE;
			continue;
		    }
		    if(info.tsMode != primaryInfo.tsMode) {
			WARNING("Interface %s does not support the same timestamping modes as primary %s (%03x vs. %03x)\n",
			ifaceName, primaryName, info.tsMode, primaryInfo.tsMode);
		    }
		    if(!initHwTs(ifaceName, &info)) {
			ret = FALSE;
			continue;
		    }
		}
	    }
	}

	if(setsockopt(netPath->eventSock, SOL_SOCKET, SO_TIMESTAMPING, &primaryInfo.tsMode, sizeof(primaryInfo.tsMode)) < 0) {
	    PERROR("Could not set SO_TIMESTAMPING modes on primary interface %s\n", primaryName);
	    ret = FALSE;
    }

    if(ret) {
	netPath->txTimestamping = TRUE;
	netPath->hwTimestamping = TRUE;
	INFO("Hardware timestamping successfully enabled on %s\n", primaryName);
    }

//    netPath->txLoop = TRUE;

    return ret;

}

/**
 * Initialize timestamping of packets
 *
 * @param netPath
 *
 * @return TRUE if successful
 */
static Boolean
netInitTimestamping(NetPath * netPath, const RunTimeOpts * rtOpts)
{

	int val = 1;
	Boolean result = TRUE;
#if defined(SO_TIMESTAMPING) && defined(SO_TIMESTAMPNS)/* Linux - current API */
	if(rtOpts->hwTimestamping) {
	    DBG("netInitTimestamping: attempting HW timestamp init\n");
	    if(netInitHwTimestamping(netPath, rtOpts)) {
		return TRUE;
	    }
	} else {
	    HwTsInfo info;
	    getHwTs(rtOpts->ifaceName, rtOpts, &info, TRUE);
	    if(info.txTimestamping) {
		val = info.tsMode;
		netPath->hwTimestamping = FALSE;
		netPath->txTimestamping = TRUE;
	    }
	}
#else
	netPath->txTimestamping = FALSE;
	val = 1;
#endif /* SO_TIMESTAMPING */

#if defined(SO_TIMESTAMPING) && defined(SO_TIMESTAMPNS)
	if(val == 1) {
	    if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_TIMESTAMPNS, &val, sizeof(int)) < 0) {
		    PERROR("netInitTimestamping: failed to enable SO_TIMESTAMPNS");
		    result = FALSE;
	    }
	} else {
	    if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_TIMESTAMPING, &val, sizeof(int)) < 0) {
		    PERROR("netInitTimestamping: failed to enable SO_TIMESTAMPING");
		    result = FALSE;
	    }
	}

	if (result == TRUE) {
	    DBG("SO_TIMESTAMP%s initialised\n",(val==1)?"NS":"ING");
	}

#elif defined(SO_TIMESTAMPNS) /* Linux, Apple */
	DBG("netInitTimestamping: trying to use SO_TIMESTAMPNS\n");
	
	if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_TIMESTAMPNS, &val, sizeof(int)) < 0) {
		PERROR("netInitTimestamping: failed to enable SO_TIMESTAMPNS");
		result = FALSE;
	}
#elif defined(SO_BINTIME) /* FreeBSD */
	DBG("netInitTimestamping: trying to use SO_BINTIME\n");
		
	if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_BINTIME, &val, sizeof(int)) < 0) {
		PERROR("netInitTimestamping: failed to enable SO_BINTIME");
		result = FALSE;
	}
#else
	result = FALSE;
#endif
			
/* fallback method */
#if defined(SO_TIMESTAMP) /* Linux, Apple, FreeBSD */
	if (!result) {
		DBG("netInitTimestamping: trying to use SO_TIMESTAMP\n");
		
		if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_TIMESTAMP, &val, sizeof(int)) < 0) {
			PERROR("netInitTimestamping: failed to enable SO_TIMESTAMP");
			result = FALSE;
		}
		result = TRUE;
	}
#endif

	return result;
}

Boolean
hostLookup(const char* hostname, Integer32* addr)
{
	if (hostname[0]) {
		/* Attempt a DNS lookup first. */
		struct hostent *host;
#ifdef HAVE_GETHOSTBYNAME2
		host = gethostbyname2(hostname, AF_INET);
#else
		host = getipnodebyname(hostname, AF_INET, AI_DEFAULT, &errno);
#endif /* HAVE_GETHOSTBYNAME2 */

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

/* parse a list of hosts to a list of IP addresses */
static int parseUnicastConfig(const RunTimeOpts *rtOpts, int maxCount, UnicastDestination * output)
{
    char* token;
    char* stash;
    int found = 0;
    int total = 0;
    char* text_;
    char* text__;
    int tmp = 0;

    if(strlen(rtOpts->unicastDestinations)==0) return 0;

    text_=strdup(rtOpts->unicastDestinations);

    for(text__=text_;found < maxCount; text__=NULL) {

	token=strtok_r(text__,DEFAULT_TOKEN_DELIM,&stash);
	if(token==NULL) break;
	if(hostLookup(token, &output[found].transportAddress)) {
	DBG("hostList %d host: %s addr %08x\n", found, token, output[found]);
	    found++;
	}

    }

    if(text_ != NULL) {
	free(text_);
    }

    if(!found) {
	return 0;
    }
    total = found;

    found = 0;

    text_=strdup(rtOpts->unicastDomains);

    for(text__=text_;found < total; text__=NULL) {

	token=strtok_r(text__,DEFAULT_TOKEN_DELIM,&stash);
	if(token==NULL) break;
	if (sscanf(token,"%d", &tmp)) {
	    DBG("hostList %dth host: domain %d\n", found, tmp);
	    output[found].domainNumber = tmp;
	    found++;
	}

    }

    if(text_ != NULL) {
	free(text_);
    }

    found = 0;

    text_=strdup(rtOpts->unicastLocalPreference);

    for(text__=text_;found < total; text__=NULL) {

	token=strtok_r(text__,DEFAULT_TOKEN_DELIM,&stash);
	tmp = LOWEST_LOCALPREFERENCE;
	if(token!=NULL) {
	    if (sscanf(token,"%d", &tmp) != 1) {
		tmp = LOWEST_LOCALPREFERENCE;
	    }
	}

	DBG("hostList %dth host: preference %d\n", found, tmp);
	output[found].localPreference = tmp;
	found++;

    }

    if(text_ != NULL) {
	free(text_);
    }

    return total;

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
netInit(NetPath * netPath, RunTimeOpts * rtOpts, PtpClock * ptpClock)
{

#ifdef __QNXNTO__
	unsigned char temp;
#else
	int temp;
#endif
	struct sockaddr_in addr;

#ifdef PTPD_PCAP
	struct bpf_program program;
	char errbuf[PCAP_ERRBUF_SIZE];
#endif

	DBG("netInit\n");

#ifdef PTPD_PCAP
	netPath->pcapEvent = NULL;
	netPath->pcapGeneral = NULL;
	netPath->pcapEventSock = -1;
	netPath->pcapGeneralSock = -1;
#endif
	netPath->generalSock = -1;
	netPath->eventSock = -1;

	netPath->hwTimestamping = FALSE;
	netPath->txTimestamping = FALSE;

	if(rtOpts->backupIfaceEnabled &&
		ptpClock->runningBackupInterface) {
		strncpy(rtOpts->ifaceName, rtOpts->backupIfaceName, IFACE_NAME_LENGTH);
	} else {
		strncpy(rtOpts->ifaceName, rtOpts->primaryIfaceName, IFACE_NAME_LENGTH);
	}

#ifdef PTPD_PCAP
	if (rtOpts->transport == IEEE_802_3) {
		netPath->headerOffset = PACKET_BEGIN_ETHER;
#ifdef HAVE_STRUCT_ETHER_ADDR_OCTET
		memcpy(netPath->etherDest.octet, ether_aton(PTP_ETHER_DST), ETHER_ADDR_LEN);
		memcpy(netPath->peerEtherDest.octet, ether_aton(PTP_ETHER_PEER), ETHER_ADDR_LEN);
#else
		memcpy(netPath->etherDest.ether_addr_octet, ether_aton(PTP_ETHER_DST), ETHER_ADDR_LEN);
		memcpy(netPath->peerEtherDest.ether_addr_octet, ether_aton(PTP_ETHER_PEER), ETHER_ADDR_LEN);
#endif /* HAVE_STRUCT_ETHER_ADDR_OCTET */
	} else
#endif
		netPath->headerOffset = PACKET_BEGIN_UDP;

	/* open sockets */
	if ((netPath->eventSock = socket(PF_INET, SOCK_DGRAM,
					 IPPROTO_UDP)) < 0
	    || (netPath->generalSock = socket(PF_INET, SOCK_DGRAM,
					      IPPROTO_UDP)) < 0) {
		PERROR("failed to initialize sockets");
		return FALSE;
	}

	/* let's see if we have another interface left before we die */
	if(!testInterface(rtOpts->ifaceName, rtOpts)) {

		/* backup not enabled - exit */
		if(!rtOpts->backupIfaceEnabled)
		    return FALSE;

		/* backup enabled - try the other interface */
		ptpClock->runningBackupInterface = !ptpClock->runningBackupInterface;

		if(ptpClock->runningBackupInterface) {
		    strncpy(rtOpts->ifaceName, rtOpts->backupIfaceName, IFACE_NAME_LENGTH);
		} else {
		    strncpy(rtOpts->ifaceName, rtOpts->primaryIfaceName, IFACE_NAME_LENGTH);
		}

		NOTICE("Last resort - attempting to switch to %s interface\n", ptpClock->runningBackupInterface ? "backup" : "primary");
		/* if this fails, we have no reason to live */
		if(!testInterface(rtOpts->ifaceName, rtOpts)) {
		    return FALSE;
		}
	}

	netPath->interfaceInfo.addressFamily = AF_INET;

	/* the if is here only to get rid of an unused result warning. */
	if( getInterfaceInfo(rtOpts->ifaceName, &netPath->interfaceInfo)!= 1)
		return FALSE;

	/* No HW address, we'll use the protocol address to form interfaceID -> clockID */
	if( !netPath->interfaceInfo.hasHwAddress && netPath->interfaceInfo.hasAfAddress ) {
		uint32_t addr = ((struct sockaddr_in*)&(netPath->interfaceInfo.afAddress))->sin_addr.s_addr;
		memcpy(netPath->interfaceID, &addr, 2);
		memcpy(netPath->interfaceID + 4, &addr + 2, 2);
	/* Initialise interfaceID with hardware address */
	} else {
		    memcpy(&netPath->interfaceID, &netPath->interfaceInfo.hwAddress,
			    sizeof(netPath->interfaceID) <= sizeof(netPath->interfaceInfo.hwAddress) ?
				    sizeof(netPath->interfaceID) : sizeof(netPath->interfaceInfo.hwAddress)
			    );
	}

	DBG("Listening on IP: %s\n",inet_ntoa(
		((struct sockaddr_in*)&(netPath->interfaceInfo.afAddress))->sin_addr));

#ifdef PTPD_PCAP
	if (rtOpts->pcap == TRUE) {

		netPath->txTimestamping = FALSE;

		int promisc = (rtOpts->transport == IEEE_802_3 ) ? 1 : 0;

		if ((netPath->pcapEvent = pcap_open_live(rtOpts->ifaceName,
							 PACKET_SIZE, promisc,
							 PCAP_TIMEOUT,
							 errbuf)) == NULL) {
			PERROR("failed to open event pcap");
			return FALSE;
		}

/* libpcap - new way - may be required for non-default buffer sizes  */
/*
		netPath->pcapEvent = pcap_create(rtOpts->ifaceName, errbuf);
		pcap_set_promisc(netPath->pcapEvent, promisc);
		pcap_set_snaplen(netPath->pcapEvent, PACKET_SIZE);
		pcap_set_timeout(netPath->pcapEvent, PCAP_TIMEOUT);
		pcap_set_buffer_size(netPath->pcapEvent, 1024 * 2 * UNICAST_MAX_DESTINATIONS);
		pcap_activate(netPath->pcapEvent);
*/
		if (pcap_compile(netPath->pcapEvent, &program,
				 ( rtOpts->transport == IEEE_802_3 ) ?
				    "ether proto 0x88f7":
				( rtOpts->ipMode == IPMODE_UNICAST ) ?
				    "udp port 319 and not multicast" :
				 ( rtOpts->ipMode != IPMODE_MULTICAST ) ?
					 "udp port 319" :
				 "host (224.0.1.129 or 224.0.0.107) and udp port 319" ,
				 1, 0) < 0) {
			PERROR("failed to compile pcap event filter");
			pcap_perror(netPath->pcapEvent, "ptpd");
			return FALSE;
		}
		if (pcap_setfilter(netPath->pcapEvent, &program) < 0) {
			PERROR("failed to set pcap event filter");
			return FALSE;
		}
		pcap_freecode(&program);
		if ((netPath->pcapEventSock =
		     pcap_get_selectable_fd(netPath->pcapEvent)) < 0) {
			PERROR("failed to get pcap event fd");
			return FALSE;
		}		
		if ((netPath->pcapGeneral = pcap_open_live(rtOpts->ifaceName,
							   PACKET_SIZE, promisc,
							   PCAP_TIMEOUT,
							 errbuf)) == NULL) {
			PERROR("failed to open general pcap");
			return FALSE;
		}
		if (rtOpts->transport != IEEE_802_3) {
			if (pcap_compile(netPath->pcapGeneral, &program,
					( rtOpts->ipMode == IPMODE_UNICAST ) ?
					    "udp port 320 and not multicast" :
					 ( rtOpts->ipMode != IPMODE_MULTICAST ) ?
						 "udp port 320" :
					 "host (224.0.1.129 or 224.0.0.107) and udp port 320" ,
					 1, 0) < 0) {
				PERROR("failed to compile pcap general filter");
				pcap_perror(netPath->pcapGeneral, "ptpd");
				return FALSE;
			}
			if (pcap_setfilter(netPath->pcapGeneral, &program) < 0) {
				PERROR("failed to set pcap general filter");
				return FALSE;
			}
			pcap_freecode(&program);
			if ((netPath->pcapGeneralSock =
			     pcap_get_selectable_fd(netPath->pcapGeneral)) < 0) {
				PERROR("failed to get pcap general fd");
				return FALSE;
			}
		}
	}
#endif

#ifdef PTPD_PCAP
	if(rtOpts->transport == IEEE_802_3) {
		close(netPath->eventSock);
		netPath->eventSock = -1;
		close(netPath->generalSock);
		netPath->generalSock = -1;
		/* TX timestamp is not generated for PCAP mode and Ethernet transport */
#ifdef SO_TIMESTAMPING
		netPath->txTimestamping = FALSE;
#endif /* SO_TIMESTAMPING */
	} else {
#endif
		/* save interface address for IGMP refresh */
		{
		    struct sockaddr_in* sin = (struct sockaddr_in*)&(netPath->interfaceInfo.afAddress);
		    netPath->interfaceAddr = sin->sin_addr;
		}

		DBG("Local IP address used : %s \n", inet_ntoa(netPath->interfaceAddr));

		temp = 1;			/* allow address reuse */
		if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_REUSEADDR,
			       &temp, sizeof(int)) < 0
		    || setsockopt(netPath->generalSock, SOL_SOCKET, SO_REUSEADDR,
				  &temp, sizeof(int)) < 0) {
			DBG("failed to set socket reuse\n");
		}

		/* disable UDP checksum validation (Linux) */
#ifdef SO_NO_CHECK
		if(rtOpts->disableUdpChecksums) {
			if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_NO_CHECK , &temp,
			sizeof(int)) < 0
			|| setsockopt(netPath->generalSock, SOL_SOCKET, SO_NO_CHECK , &temp,
			    sizeof(int)) < 0) {
			    WARNING("Could not disable UDP checksum validation\n");
			}
		}
#endif /* SO_NO_CHECK */

		/* bind sockets */
		/*
		 * need INADDR_ANY to receive both unicast and multicast (Linux),
		 * but only need interface address for unicast
		 */

		if(rtOpts->ipMode == IPMODE_UNICAST ||
		   rtOpts->bindToInterface) {
			addr.sin_addr = netPath->interfaceAddr;
		} else {
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
		}

		addr.sin_family = AF_INET;
		addr.sin_port = htons(PTP_EVENT_PORT);
		if (bind(netPath->eventSock, (struct sockaddr *)&addr,
			sizeof(struct sockaddr_in)) < 0) {
			PERROR("failed to bind event socket");
			return FALSE;
		}
		addr.sin_port = htons(PTP_GENERAL_PORT);
		if (bind(netPath->generalSock, (struct sockaddr *)&addr,
			sizeof(struct sockaddr_in)) < 0) {
			PERROR("failed to bind general socket");
			return FALSE;
		}

#ifdef SO_RCVBUF
                /* try increasing receive buffers for unicast Sync processing */
                if(rtOpts->ipMode == IPMODE_UNICAST && !rtOpts->slaveOnly) {
                    uint32_t n = 0;
                    socklen_t nlen = sizeof(n);

                    if (getsockopt(netPath->eventSock, SOL_SOCKET, SO_RCVBUF, &n, &nlen) < 0) {
                        n = 0;
                    }

                    DBG("eventSock rcvbuff : %d\n", n);

                    if(n < (UNICAST_MAX_DESTINATIONS * 1024)) {
                        n = UNICAST_MAX_DESTINATIONS * 1024;
                        if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) < 0) {
                            DBG("Failed to increase event socket receive buffer\n");
                        }
                    }

                    if (getsockopt(netPath->generalSock, SOL_SOCKET, SO_RCVBUF, &n, &nlen) < 0) {
                        n = 0;
                    }

                    DBG("genetalSock rcvbuff : %d\n", n);

                    if(n < (UNICAST_MAX_DESTINATIONS * 1024)) {
                        n = UNICAST_MAX_DESTINATIONS * 1024;
                        if (setsockopt(netPath->generalSock, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) < 0) {
                            DBG("Failed to increase general socket receive buffer\n");
                        }
                    }
                }
#endif /* SO_RCVBUF */

#ifdef USE_BINDTODEVICE
#ifdef linux
		/*
		 * The following code makes sure that the data is only
		 * received on the specified interface.  Without this option,
		 * it's possible to receive PTP from another interface, and
		 * confuse the protocol.  Calling bind() with the IP address
		 * of the device instead of INADDR_ANY does not work.
		 *
		 * More info:
		 *   http://developerweb.net/viewtopic.php?id=6471
		 *   http://stackoverflow.com/questions/1207746/problems-with-so-bindtodevice-linux-socket-option
		 */

		/*
		 * wowczarek: 2.3.1-rc4@jun0215: this breaks the manual packet looping,
		 * so may only be used for multicast-only
		 */
		
		if ( rtOpts->ipMode == IPMODE_MULTICAST ) {
		    if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_BINDTODEVICE,
				rtOpts->ifaceName, strlen(rtOpts->ifaceName)) < 0
			|| setsockopt(netPath->generalSock, SOL_SOCKET, SO_BINDTODEVICE,
				rtOpts->ifaceName, strlen(rtOpts->ifaceName)) < 0){
			PERROR("failed to call SO_BINDTODEVICE on the interface");
			return FALSE;
		    }
		}

#endif
#endif

		/* Set socket dscp */
		if(rtOpts->dscpValue) {

			if (setsockopt(netPath->eventSock, IPPROTO_IP, IP_TOS,
				 &rtOpts->dscpValue, sizeof(int)) < 0
			    || setsockopt(netPath->generalSock, IPPROTO_IP, IP_TOS,
				&rtOpts->dscpValue, sizeof(int)) < 0) {
				    PERROR("Failed to set socket DSCP bits");
				    return FALSE;
				}
		}

		if(rtOpts->unicastDestinationsSet) {

		    ptpClock->unicastDestinationCount = parseUnicastConfig(rtOpts,
			    UNICAST_MAX_DESTINATIONS, ptpClock->unicastDestinations);
			    DBG("configured %d unicast destinations\n",ptpClock->unicastDestinationCount);

		}

		if(rtOpts->delayMechanism==P2P && rtOpts->ipMode==IPMODE_UNICAST) {
			ptpClock->unicastPeerDestination.transportAddress = 0;
		    	if(rtOpts->unicastPeerDestinationSet &&
				rtOpts->delayMechanism==P2P && !hostLookup(rtOpts->unicastPeerDestination,
				&ptpClock->unicastPeerDestination.transportAddress)) {

			    ERROR("Could not parse P2P unicast destination %s:\n",
				    rtOpts->unicastPeerDestination);
			    return FALSE;

			} else if(!rtOpts->unicastPeerDestinationSet) {

			    ERROR("No P2P unicast destination specified\n");
			    return FALSE;

			}

		}

		if(rtOpts->ipMode != IPMODE_UNICAST) {

			/* init UDP Multicast on both Default and Peer addresses */
			if (!netInitMulticast(netPath, rtOpts))
				return FALSE;

			/* set socket time-to-live  */
			if(!netSetMulticastTTL(netPath->eventSock,rtOpts->ttl) ||
			    !netSetMulticastTTL(netPath->generalSock,rtOpts->ttl))
				return FALSE;

			/* start tracking TTL */
			netPath->ttlEvent = rtOpts->ttl;
			netPath->ttlGeneral = rtOpts->ttl;
		}

		/* try enabling the capture of destination address */

#ifdef IP_PKTINFO
		temp = 1;
		setsockopt(netPath->eventSock, IPPROTO_IP, IP_PKTINFO, &temp, sizeof(int));
#endif /* IP_PKTINFO */

#ifdef IP_RECVDSTADDR
		temp = 1;
		setsockopt(netPath->eventSock, IPPROTO_IP, IP_RECVDSTADDR, &temp, sizeof(int));
#endif


#ifdef SO_TIMESTAMPING
			/* Reset the failure indicator when (re)starting network */
			netPath->txTimestamping = FALSE;
			/* for SO_TIMESTAMPING we're receiving transmitted packets via ERRQUEUE */
			temp = 0;
#else
			/* enable loopback */
			temp = 1;
#endif

		/* make timestamps available through recvmsg() */
		if (!netInitTimestamping(netPath,rtOpts)) {
			ERROR("Failed to enable packet time stamping\n");
			return FALSE;
		}

		controlClockDrivers(CD_NOTINUSE);

		if(!prepareClockDrivers(netPath, ptpClock, rtOpts)) {
			ERROR("Cannot start clock drivers - aborting mission\n");
			exit(-1);
		}

		createClockDriversFromString(rtOpts->extraClocks, rtOpts, FALSE);

		/* clean up unused clock drivers */
		controlClockDrivers(CD_CLEANUP);

		reconfigureClockDrivers(rtOpts);

#ifdef SO_TIMESTAMPING
		/* If we failed to initialise SO_TIMESTAMPING, enable mcast loopback */
		if(!netPath->txTimestamping)
			temp = 1;
#endif

			if(!netSetMulticastLoopback(netPath, temp)) {
				return FALSE;
			}

#ifdef PTPD_PCAP
	}
#endif

	/* Compile ACLs */
	if(rtOpts->timingAclEnabled) {
    		freeIpv4AccessList(&netPath->timingAcl);
		netPath->timingAcl=createIpv4AccessList(rtOpts->timingAclPermitText,
			rtOpts->timingAclDenyText, rtOpts->timingAclOrder);
	}
	if(rtOpts->managementAclEnabled) {
		freeIpv4AccessList(&netPath->managementAcl);
		netPath->managementAcl=createIpv4AccessList(rtOpts->managementAclPermitText,
			rtOpts->managementAclDenyText, rtOpts->managementAclOrder);
	}


	return TRUE;
}

/*Check if data has been received*/
int
netSelect(TimeInternal * timeout, NetPath * netPath, fd_set *readfds)
{
	int ret, nfds;
	struct timeval tv, *tv_ptr;


#if defined PTPD_SNMP
	extern const RunTimeOpts rtOpts;
	struct timeval snmp_timer_wait = { 0, 0}; // initialise to avoid unused warnings when SNMP disabled
	int snmpblock = 0;
#endif

	if (timeout) {
		if(isTimeNegative(timeout)) {
			ERROR("Negative timeout attempted for select()\n");
			return -1;
		}
		tv.tv_sec = timeout->seconds;
		tv.tv_usec = timeout->nanoseconds / 1000;
		tv_ptr = &tv;
	} else {
		tv_ptr = NULL;
	}

	FD_ZERO(readfds);
	nfds = 0;
#ifdef PTPD_PCAP
	if (netPath->pcapEventSock >= 0) {
		FD_SET(netPath->pcapEventSock, readfds);
		if (netPath->pcapGeneralSock >= 0)
			FD_SET(netPath->pcapGeneralSock, readfds);

		nfds = netPath->pcapEventSock;
		if (netPath->pcapEventSock < netPath->pcapGeneralSock)
			nfds = netPath->pcapGeneralSock;

	} else if (netPath->eventSock >= 0) {
#endif
		FD_SET(netPath->eventSock, readfds);
		if (netPath->generalSock >= 0)
			FD_SET(netPath->generalSock, readfds);

		nfds = netPath->eventSock;
		if (netPath->eventSock < netPath->generalSock)
			nfds = netPath->generalSock;
#ifdef PTPD_PCAP
	}
#endif
	nfds++;

#if defined PTPD_SNMP
if (rtOpts.snmpEnabled) {
	snmpblock = 1;
	if (tv_ptr) {
		snmpblock = 0;
		memcpy(&snmp_timer_wait, tv_ptr, sizeof(struct timeval));
	}
	snmp_select_info(&nfds, readfds, &snmp_timer_wait, &snmpblock);
	if (snmpblock == 0)
		tv_ptr = &snmp_timer_wait;
}
#endif

	ret = select(nfds, readfds, 0, 0, tv_ptr);

	if (ret < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return 0;
	}
#if defined PTPD_SNMP
if (rtOpts.snmpEnabled) {
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

/**
 * store received data from network to "buf" , get and store the
 * SO_TIMESTAMP value in "time" for an event message
 *
 * @note Should this function be merged with netRecvGeneral(), below?
 * Jan Breuer: I think that netRecvGeneral should be
 * simplified. Timestamp returned by this function is never
 * used. According to this, netInitTimestamping can be also simplified
 * to initialize timestamping only on eventSock.
 *
 * @param buf
 * @param time
 * @param netPath
 *
 * @return
 */

static ssize_t
netr(Octet * buf, TimeInternal * time, NetPath * netPath, int flags)
{
	ssize_t ret = 0;
	struct msghdr msg;
	struct iovec vec; //[1];
	struct sockaddr_in from_addr;

#ifdef PTPD_PCAP
	struct pcap_pkthdr *pkt_header;
	const u_char *pkt_data;
#endif

#if defined(__QNXNTO__) && defined(PTPD_EXPERIMENTAL)
	TimeInternal tmpTime;
	/* get system time interpolated with TSC / clockCycles as soon as we have data on the socket */
	getSystemClock()->getTime(getSystemClock(), &tmpTime);
#endif

	union {
		struct cmsghdr cm;
		char	control[256];
	}     cmsg_un;

	struct cmsghdr *cmsg;

#if defined(SO_TIMESTAMPNS) || defined(SO_TIMESTAMPING)
	struct timespec * ts;
#elif defined(SO_BINTIME)
	struct bintime * bt;
	struct timespec ts;
#endif
	
#if defined(SO_TIMESTAMP)
	struct timeval * tv;
#endif
	Boolean timestampValid = FALSE;
	netPath->lastDestAddr = 0;
#ifdef PTPD_PCAP
	if (netPath->pcapEvent == NULL) { /* Using sockets */
#endif
		vec.iov_base = buf;
		vec.iov_len = PACKET_SIZE;

		memset(&msg, 0, sizeof(msg));
		memset(&from_addr, 0, sizeof(from_addr));
		memset(buf, 0, PACKET_SIZE);
		memset(&cmsg_un, 0, sizeof(cmsg_un));

		msg.msg_name = (caddr_t)&from_addr;
		msg.msg_namelen = sizeof(from_addr);
		msg.msg_iov = &vec;
		msg.msg_iovlen = 1;
		msg.msg_control = cmsg_un.control;
		msg.msg_controllen = sizeof(cmsg_un.control);
		msg.msg_flags = 0;

		ret = recvmsg(netPath->eventSock, &msg, flags | MSG_DONTWAIT);

		if (ret <= 0) {
			/* we may have a TX timestamp stuck in error queue, flush it */
			if((errno == ENOMSG) && netPath->txTimestamping && !netPath->txLoop) {
			    DBG("netRecvEvent: Flushed errqueue\n");
			    ret = recvmsg(netPath->eventSock, &msg, flags | MSG_ERRQUEUE | MSG_DONTWAIT);
			    /* drop the next regular message as well - it can be severely delayed... */
			    ret = recvmsg(netPath->eventSock, &msg, flags | MSG_DONTWAIT);
			    return 0;
			}

			if (errno == EAGAIN || errno == EINTR) {
				return 0;
			} else {
				return ret;
			}
		};

		if (msg.msg_flags & MSG_TRUNC) {
			ERROR("received truncated message\n");
			return 0;
		}
		/* get time stamp of packet */
		if (!time) {
			ERROR("null receive time stamp argument\n");
			return 0;
		}
		if (msg.msg_flags & MSG_CTRUNC) {
			ERROR("received truncated ancillary data\n");
			return 0;
		}

#if defined(HAVE_DECL_MSG_ERRQUEUE) && HAVE_DECL_MSG_ERRQUEUE
		if(!(flags & MSG_ERRQUEUE))
#endif
		netPath->lastSourceAddr = from_addr.sin_addr.s_addr;

		netPath->receivedPacketsTotal++;

		/* do not report "from self" */
		if(!netPath->lastSourceAddr || (netPath->lastSourceAddr != netPath->interfaceAddr.s_addr)) {
			netPath->receivedPackets++;
		}

		if (msg.msg_controllen <= 0) {
			ERROR("received short ancillary data (%ld/%ld)\n",
			      (long)msg.msg_controllen, (long)sizeof(cmsg_un.control));

			return 0;
		}

		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
		     cmsg = CMSG_NXTHDR(&msg, cmsg)) {

#ifdef IP_PKTINFO
			if ((cmsg->cmsg_level == IPPROTO_IP) &&
			    (cmsg->cmsg_type == IP_PKTINFO)) {
				struct in_pktinfo *pi =
				(struct in_pktinfo *) CMSG_DATA(cmsg);
				netPath->lastDestAddr = pi->ipi_addr.s_addr;
				DBG("IP_PKTINFO Dst: %s\n", inet_ntoa(pi->ipi_addr));
			}
#endif

#ifdef IP_RECVDSTADDR
			if ((cmsg->cmsg_level == IPPROTO_IP) &&
			    (cmsg->cmsg_type == IP_RECVDSTADDR)) {
				struct in_addr *pa = (struct in_addr *) CMSG_DATA(cmsg);
				netPath->lastDestAddr = pa->s_addr;
				DBG("IP_RECVDSTADDR Dst: %s\n", inet_ntoa(*pa));
			}
#endif
			if (cmsg->cmsg_level == SOL_SOCKET) {
#if defined(SO_TIMESTAMPING)
				if(cmsg->cmsg_type == SO_TIMESTAMPING) {
					ts = (struct timespec *)CMSG_DATA(cmsg);
					if(netPath->hwTimestamping) {
						if (cmsg->cmsg_len >= sizeof(*ts) * 3) {
						    time->seconds = ts[2].tv_sec;
						    time->nanoseconds = ts[2].tv_nsec;
						} else {
						    ERROR("SO_TIMESTAMPING: HW timestamp control message too short\n");
						    return 0;
						}
					} else {
						    time->seconds = ts[0].tv_sec;
						    time->nanoseconds = ts[0].tv_nsec;
					}
					timestampValid = TRUE;
					if(flags & MSG_ERRQUEUE) {
						char tmpB[PACKET_SIZE];
						memset(tmpB,0,PACKET_SIZE);
						memcpy(tmpB, buf, PACKET_SIZE);
						memset(buf,0,PACKET_SIZE);
						memcpy(buf, tmpB + 42, PACKET_SIZE - 42);
						uint8_t msgtype = *(uint8_t*)(buf);
						msgtype &= 0x0F;
						uint16_t seqno = *(uint16_t*)(buf + 30);
						seqno = ntohs(seqno);
						DBG("tx timestamp for msg type %d seq %d\n", msgtype, seqno);

					}

					DBG("rcvevent: SO_TIMESTAMPING %s time stamp: %us %dns\n", 
					    (flags & MSG_ERRQUEUE) ? "TX" : "RX" ,
					     time->seconds, time->nanoseconds);
					break;
				}
#endif

#if defined(SO_TIMESTAMPNS)
				if(cmsg->cmsg_type == SCM_TIMESTAMPNS) {
					ts = (struct timespec *)CMSG_DATA(cmsg);
					time->seconds = ts->tv_sec;
					time->nanoseconds = ts->tv_nsec;
					timestampValid = TRUE;
					DBGV("kernel NANO recv time stamp %us %dns\n",
					     time->seconds, time->nanoseconds);
					break;
				}
#elif defined(SO_BINTIME)
				if(cmsg->cmsg_type == SCM_BINTIME) {
					bt = (struct bintime *)CMSG_DATA(cmsg);
					bintime2timespec(bt, &ts);
					time->seconds = ts.tv_sec;
					time->nanoseconds = ts.tv_nsec;
					timestampValid = TRUE;
					DBGV("kernel NANO recv time stamp %us %dns\n",
					     time->seconds, time->nanoseconds);
					break;
				}
#endif
			
#if defined(SO_TIMESTAMP)
				if(cmsg->cmsg_type == SCM_TIMESTAMP) {
					tv = (struct timeval *)CMSG_DATA(cmsg);
					time->seconds = tv->tv_sec;
					time->nanoseconds = tv->tv_usec * 1000;
					timestampValid = TRUE;
					DBGV("kernel MICRO recv time stamp %us %dns\n",
					     time->seconds, time->nanoseconds);
				}
#endif
			 }

		}

		if (!timestampValid) {
			/*
			 * do not try to get by with recording the time here, better
			 * to fail because the time recorded could be well after the
			 * message receive, which would put a big spike in the
			 * offset signal sent to the clock servo
			 */
			DBG("netRecvEvent: no receive time stamp\n");
			return 0;
		}
#ifdef PTPD_PCAP
	}
#endif

#ifdef PTPD_PCAP
	else { /* Using PCAP */
		/* Discard packet on socket */
		if (netPath->eventSock >= 0) {
			recv(netPath->eventSock, buf, PACKET_SIZE, MSG_DONTWAIT);
		}
		
		if ((ret = pcap_next_ex(netPath->pcapEvent, &pkt_header,
					&pkt_data)) < 1) {
			if (ret < 0)
				INFO("netRecvEvent: pcap_next_ex failed %s\n",
				     pcap_geterr(netPath->pcapEvent));
			return 0;
		}

	/* Make sure this is IP (could dot1q get here?) */
	if( ntohs(*(u_short *)(pkt_data + 12)) != ETHERTYPE_IP) {
		if( ntohs(*(u_short *)(pkt_data + 12)) != PTP_ETHER_TYPE) {
		DBG("PCAP payload ethertype received not IP or PTP: 0x%04x\n",
		    ntohs(*(u_short *)(pkt_data + 12)));
		/* do not count packets if from self */
		} else if(memcmp(&netPath->interfaceInfo.hwAddress, pkt_data + 6, 6)) {
		    netPath->receivedPackets++;
		}
	} else {
	    /* Retrieve source IP from the payload - 14 eth + 12 IP */
	    netPath->lastSourceAddr = *(Integer32 *)(pkt_data + 26);
	    /* Retrieve destination IP from the payload - 14 eth + 16 IP */
	    netPath->lastDestAddr = *(Integer32 *)(pkt_data + 30);
	    /* do not count packets from self */
	    if(netPath->lastSourceAddr != netPath->interfaceAddr.s_addr) {
		    netPath->receivedPackets++;
	    }
	}

	netPath->receivedPacketsTotal++;

		/* XXX Total cheat */
		memcpy(buf, pkt_data + netPath->headerOffset,
		       pkt_header->caplen - netPath->headerOffset);
		time->seconds = pkt_header->ts.tv_sec;
		time->nanoseconds = pkt_header->ts.tv_usec * 1000;
		timestampValid = TRUE;
		DBGV("netRecvEvent: kernel PCAP recv time stamp %us %dns\n",
		     time->seconds, time->nanoseconds);
		fflush(NULL);
		ret = pkt_header->caplen - netPath->headerOffset;
	}
#endif

#if defined(__QNXNTO__) && defined(PTPD_EXPERIMENTAL)
	*time = tmpTime;
#endif

	return ret;
}

ssize_t
netRecvEvent(Octet * buf, TimeInternal * time, NetPath * netPath, int flags) {

	int ret;

	if(netPath->txTimestamping && netPath->txLoop) {
	/* if we are pushing TX timestamps for message processing, read error queue first */
	    ret = netr(buf, time, netPath, MSG_ERRQUEUE);
	    if(ret > 0) {
		return ret;
	    }
	}

	ret = netr(buf, time, netPath, 0);

	/* no data received - possibly a stuck TX timestamp - drain the error queue */
	if(!ret &&netPath->txTimestamping) {
	    netr(buf, time, netPath, MSG_ERRQUEUE);
		
	}

	return ret;

}



/**
 *
 * store received data from network to "buf" get and store the
 * SO_TIMESTAMP value in "time" for a general message
 *
 * @param buf
 * @param time
 * @param netPath
 *
 * @return
 */

ssize_t
netRecvGeneral(Octet * buf, NetPath * netPath)
{
	ssize_t ret = 0;
	struct sockaddr_in from_addr;

#ifdef PTPD_PCAP
	struct pcap_pkthdr *pkt_header;
	const u_char *pkt_data;
#endif
	socklen_t from_addr_len = sizeof(from_addr);

	netPath->lastSourceAddr = 0;

#ifdef PTPD_PCAP
	if (netPath->pcapGeneral == NULL) {
#endif
		ret=recvfrom(netPath->generalSock, buf, PACKET_SIZE, MSG_DONTWAIT, (struct sockaddr*)&from_addr, &from_addr_len);
		netPath->lastSourceAddr = from_addr.sin_addr.s_addr;

		/* do not report "from self" */
		if(!netPath->lastSourceAddr || (netPath->lastSourceAddr != netPath->interfaceAddr.s_addr)) {
		    netPath->receivedPackets++;
		}
		netPath->receivedPacketsTotal++;
		return ret;
#ifdef PTPD_PCAP
	}
#endif

#ifdef PTPD_PCAP
	else { /* Using PCAP */
		/* Discard packet on socket */
		if (netPath->generalSock >= 0)
			recv(netPath->generalSock, buf, PACKET_SIZE, MSG_DONTWAIT);

		
		if (( ret = pcap_next_ex(netPath->pcapGeneral, &pkt_header,
					 &pkt_data)) < 1) {
			if (ret < 0)
				DBGV("netRecvGeneral: pcap_next_ex failed %d %s\n",
				     ret, pcap_geterr(netPath->pcapGeneral));
			return 0;
		}

	/* Make sure this is IP (could dot1q get here?) */
	if( ntohs(*(u_short *)(pkt_data + 12)) != ETHERTYPE_IP) {
		if( ntohs(*(u_short *)(pkt_data + 12)) != PTP_ETHER_TYPE) {
		DBG("PCAP payload ethertype received not IP or PTP: 0x%04x\n",
		    ntohs(*(u_short *)(pkt_data + 12)));
		/* do not count packets if from self */
		} else if(memcmp(&netPath->interfaceInfo.hwAddress, pkt_data + 6, 6)) {
		    netPath->receivedPackets++;
		}
	} else {
	    /* Retrieve source IP from the payload - 14 eth + 12 IP */
	    netPath->lastSourceAddr = *(Integer32 *)(pkt_data + 26);
	    /* Retrieve destination IP from the payload - 14 eth + 16 IP */
	    netPath->lastDestAddr = *(Integer32 *)(pkt_data + 30);
	    /* do not count packets from self */
	    if(netPath->lastSourceAddr != netPath->interfaceAddr.s_addr) {
		    netPath->receivedPackets++;
	    }
	}

	netPath->receivedPacketsTotal++;

		/* XXX Total cheat */
		memcpy(buf, pkt_data + netPath->headerOffset,
		       pkt_header->caplen - netPath->headerOffset);
		fflush(NULL);
		ret = pkt_header->caplen - netPath->headerOffset;
	}
#endif
	return ret;
}


#ifdef PTPD_PCAP
ssize_t
netSendPcapEther(Octet * buf,  UInteger16 length,
			struct ether_addr * dst, struct ether_addr * src,
			pcap_t * pcap) {
	Octet ether[ETHER_HDR_LEN + PACKET_SIZE];
#ifdef HAVE_STRUCT_ETHER_ADDR_OCTET
	memcpy(ether, dst->octet, ETHER_ADDR_LEN);
	memcpy(ether + ETHER_ADDR_LEN, src->octet, ETHER_ADDR_LEN);
#else
	memcpy(ether, dst->ether_addr_octet, ETHER_ADDR_LEN);
	memcpy(ether + ETHER_ADDR_LEN, src->ether_addr_octet, ETHER_ADDR_LEN);
#endif /* HAVE_STRUCT_ETHER_ADDR_OCTET */
	*((short *)&ether[2 * ETHER_ADDR_LEN]) = htons(PTP_ETHER_TYPE);
	memcpy(ether + ETHER_HDR_LEN, buf, length);

	return pcap_inject(pcap, ether, ETHER_HDR_LEN + length);
}
#endif

//
// destinationAddress: destination:
//   if filled, send to this unicast dest;
//   if zero, sending to multicast.
//
///
/// TODO: merge these 2 functions into one
///
ssize_t
netSendEvent(Octet * buf, UInteger16 length, NetPath * netPath,
	     const RunTimeOpts *rtOpts, Integer32 destinationAddress, TimeInternal * tim)
{
	extern PtpClock *G_ptpClock;
	ssize_t ret;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_EVENT_PORT);

#if defined(__QNXNTO__) && defined(PTPD_EXPERIMENTAL)
	TimeInternal tmpTime;
	/* get system time interpolated with TSC / clockCycles as soon as we have data on the socket */
	getSystemClock()->getTime(getSystemClock(), &tmpTime);
#endif

#ifdef PTPD_PCAP

	/* In PCAP Ethernet mode, we use pcapEvent for receiving all messages
	 * and pcapGeneral for sending all messages
	 */
	if ((netPath->pcapGeneral != NULL) && (rtOpts->transport == IEEE_802_3 )) {
		ret = netSendPcapEther(buf, length,
			&netPath->etherDest,
			(struct ether_addr *)netPath->interfaceID,
			netPath->pcapGeneral);
		if (ret <= 0)
			DBG("Error sending ether multicast event message\n");
		else {
			netPath->sentPackets++;
			netPath->sentPacketsTotal++;
		}
        } else {
#endif
		if (destinationAddress ) {
			addr.sin_addr.s_addr = destinationAddress;
			/*
			 * This function is used for PTP only anyway - for now.
			 * If we're sending to a unicast address, set the UNICAST flag.
			 * Transport API in LibCCK / 2.4 uses a callback for this,
			 * so the client can do something with the payload before it's sent,
			 * depending if it's unicast or multicast.
			 */
			*(char *)(buf + 6) |= PTP_UNICAST;

			ret = sendto(netPath->eventSock, buf, length, 0,
				     (struct sockaddr *)&addr,
				     sizeof(struct sockaddr_in));
			if (ret <= 0)
				DBG("Error sending unicast event message\n");
			else {
				netPath->sentPackets++;
				netPath->sentPacketsTotal++;
			}
#ifndef SO_TIMESTAMPING
#if defined(__QNXNTO__) && defined(PTPD_EXPERIMENTAL)
			*tim = tmpTime;
#else
			/*
			 * Need to forcibly loop back the packet since
			 * we are not using multicast.
			 */
			addr.sin_addr.s_addr = netPath->interfaceAddr.s_addr;
			ret = sendto(netPath->eventSock, buf, length, 0,
				     (struct sockaddr *)&addr,
				     sizeof(struct sockaddr_in));
			if (ret <= 0)
				DBGV("Error looping back unicast event message\n");
#endif

#else

#ifdef PTPD_PCAP
			if((netPath->pcapEvent == NULL) && netPath->txTimestamping && !netPath->txLoop) {
#else
			if(netPath->txTimestamping && !netPath->txLoop) {
#endif /* PTPD_PCAP */

				if(!getTxTimestamp(netPath, tim)) {
					if (tim) {
						clearTime(tim);
					}
				} else {
					    uint8_t msgtypea= (*(uint8_t*)(buf)) & 0x0F;
					    uint16_t seqnoa = ntohs(*(uint16_t*)(buf + 30));
					    uint8_t msgtypeb= (*(uint8_t*)(G_ptpClock->msgIbuf)) & 0x0F;
					    uint16_t seqnob = ntohs(*(uint16_t*)(G_ptpClock->msgIbuf + 30));
					    if((seqnoa != seqnob) || (msgtypea != msgtypeb)) {
						if(tim) {
						    clearTime(tim);
						}
						DBG("netSendEvent: Sent type %d seq %d, got timestamp for type %d seq %d - discarding TX timestamp\n",
						    msgtypea, seqnoa, msgtypeb, seqnob);
					    }
				}


			}

#endif /* SO_TIMESTAMPING */		
		} else {
			addr.sin_addr.s_addr = netPath->multicastAddr;
                        /* Is TTL OK? */
			if(netPath->ttlEvent != rtOpts->ttl) {
				/* Try restoring TTL */
			/* set socket time-to-live  */
			if (netSetMulticastTTL(netPath->eventSock,rtOpts->ttl)) {
				    netPath->ttlEvent = rtOpts->ttl;
				}
            		}
			ret = sendto(netPath->eventSock, buf, length, 0,
				     (struct sockaddr *)&addr,
				     sizeof(struct sockaddr_in));
			if (ret <= 0)
				DBG("Error sending multicast event message\n");
			else {
				netPath->sentPackets++;
				netPath->sentPacketsTotal++;
			}
#ifdef SO_TIMESTAMPING

#ifdef PTPD_PCAP
			if((netPath->pcapEvent == NULL) && netPath->txTimestamping && !netPath->txLoop) {
#else
			if(netPath->txTimestamping && !netPath->txLoop) {
#endif /* PTPD_PCAP */
				if(!getTxTimestamp(netPath, tim)) {
					if (tim) {
						clearTime(tim);
					}
				}
			}
#endif /* SO_TIMESTAMPING */
		}

#ifdef PTPD_PCAP
	}
#endif
	return ret;
}

ssize_t
netSendGeneral(Octet * buf, UInteger16 length, NetPath * netPath,
	       const const RunTimeOpts *rtOpts, Integer32 destinationAddress)
{
	ssize_t ret;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_GENERAL_PORT);

#ifdef PTPD_PCAP
	if ((netPath->pcapGeneral != NULL) && (rtOpts->transport == IEEE_802_3)) {
		ret = netSendPcapEther(buf, length,
			&netPath->etherDest,
			(struct ether_addr *)netPath->interfaceID,
			netPath->pcapGeneral);

		if (ret <= 0)
			DBG("Error sending ether multicast general message\n");
		else {
			netPath->sentPackets++;
			netPath->sentPacketsTotal++;
		}
	} else {
#endif
		if(destinationAddress) {

			addr.sin_addr.s_addr = destinationAddress;
			/*
			 * This function is used for PTP only anyway...
			 * If we're sending to a unicast address, set the UNICAST flag.
			 */
			*(char *)(buf + 6) |= PTP_UNICAST;

			ret = sendto(netPath->generalSock, buf, length, 0,
				     (struct sockaddr *)&addr,
				     sizeof(struct sockaddr_in));
			if (ret <= 0)
				DBG("Error sending unicast general message\n");
			else {
				netPath->sentPackets++;
				netPath->sentPacketsTotal++;
			}
		} else {
			addr.sin_addr.s_addr = netPath->multicastAddr;

                        /* Is TTL OK? */
			if(netPath->ttlGeneral != rtOpts->ttl) {
				/* Try restoring TTL */
				if (netSetMulticastTTL(netPath->generalSock,rtOpts->ttl)) {
				    netPath->ttlGeneral = rtOpts->ttl;
				}
            		}

			ret = sendto(netPath->generalSock, buf, length, 0,
				     (struct sockaddr *)&addr,
				     sizeof(struct sockaddr_in));
			if (ret <= 0)
				DBG("Error sending multicast general message\n");
			else {
				netPath->sentPackets++;
				netPath->sentPacketsTotal++;
			}
		}

#ifdef PTPD_PCAP
	}
#endif
	return ret;
}

ssize_t
netSendPeerGeneral(Octet * buf, UInteger16 length, NetPath * netPath, const RunTimeOpts *rtOpts, Integer32 dst)
{

	ssize_t ret;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_GENERAL_PORT);

#ifdef PTPD_PCAP
	if ((netPath->pcapGeneral != NULL) && (rtOpts->transport == IEEE_802_3)) {
		ret = netSendPcapEther(buf, length,
			&netPath->peerEtherDest,
			(struct ether_addr *)netPath->interfaceID,
			netPath->pcapGeneral);

		if (ret <= 0)
			DBG("error sending ether multicast general message\n");

	} else if (dst)
#else
	if (dst)
#endif
	{
		addr.sin_addr.s_addr = dst;

		/*
		 * This function is used for PTP only anyway...
		 * If we're sending to a unicast address, set the UNICAST flag.
		 */
		*(char *)(buf + 6) |= PTP_UNICAST;

		ret = sendto(netPath->generalSock, buf, length, 0,
			     (struct sockaddr *)&addr,
			     sizeof(struct sockaddr_in));
		if (ret <= 0)
			DBG("Error sending unicast peer general message\n");

	} else {
		addr.sin_addr.s_addr = netPath->peerMulticastAddr;
		
		/* is TTL already 1 ? */
		if(netPath->ttlGeneral != 1) {
			/* Try setting TTL to 1 */
			if (netSetMulticastTTL(netPath->generalSock,1)) {
				netPath->ttlGeneral = 1;
			}
                }
		ret = sendto(netPath->generalSock, buf, length, 0,
			     (struct sockaddr *)&addr,
			     sizeof(struct sockaddr_in));
		if (ret <= 0)
			DBG("Error sending multicast peer general message\n");

	}

	if (ret > 0) {
			netPath->sentPackets++;
			netPath->sentPacketsTotal++;
		}
	
	return ret;

}

ssize_t
netSendPeerEvent(Octet * buf, UInteger16 length, NetPath * netPath, const RunTimeOpts *rtOpts, Integer32 dst, TimeInternal * tim)
{
	ssize_t ret;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_EVENT_PORT);

#ifdef PTPD_PCAP
	if ((netPath->pcapGeneral != NULL) && (rtOpts->transport == IEEE_802_3)) {
		ret = netSendPcapEther(buf, length,
			&netPath->peerEtherDest,
			(struct ether_addr *)netPath->interfaceID,
			netPath->pcapGeneral);

		if (ret <= 0)
			DBG("error sending ether multicast general message\n");
	} else if (dst)
#else
	if (dst)
#endif
	{
		addr.sin_addr.s_addr = dst;

		/*
		 * This function is used for PTP only anyway...
		 * If we're sending to a unicast address, set the UNICAST flag.
		 */
		*(char *)(buf + 6) |= PTP_UNICAST;

		ret = sendto(netPath->eventSock, buf, length, 0,
			     (struct sockaddr *)&addr,
			     sizeof(struct sockaddr_in));
		if (ret <= 0)
			DBG("Error sending unicast peer event message\n");

#ifndef SO_TIMESTAMPING
		/*
		 * Need to forcibly loop back the packet since
		 * we are not using multicast.
		 */
		addr.sin_addr.s_addr = netPath->interfaceAddr.s_addr;

		ret = sendto(netPath->eventSock, buf, length, 0,
			     (struct sockaddr *)&addr,
			     sizeof(struct sockaddr_in));
		if (ret <= 0)
			DBG("Error looping back unicast peer event message\n");
#else

#ifdef PTPD_PCAP
		if((netPath->pcapEvent == NULL) && netPath->txTimestamping && !netPath->txLoop) {
#else
		if(netPath->txTimestamping && !netPath->txLoop) {
#endif /* PTPD_PCAP */
			if(!getTxTimestamp(netPath, tim)) {
				if (tim) {
					clearTime(tim);
				}
			}
		}

#endif /* SO_TIMESTAMPING */

	} else {
		addr.sin_addr.s_addr = netPath->peerMulticastAddr;

		/* is TTL already 1 ? */
		if(netPath->ttlEvent != 1) {
			/* Try setting TTL to 1 */
			if (netSetMulticastTTL(netPath->eventSock,1)) {
			    netPath->ttlEvent = 1;
			}
                }
		ret = sendto(netPath->eventSock, buf, length, 0,
			     (struct sockaddr *)&addr,
			     sizeof(struct sockaddr_in));
		if (ret <= 0)
			DBG("Error sending multicast peer event message\n");
#ifdef SO_TIMESTAMPING
		if(netPath->txTimestamping && !netPath->txLoop) {
			if(!getTxTimestamp(netPath, tim)) {
				if (tim) {
					clearTime(tim);
				}
			}
		}
#endif /* SO_TIMESTAMPING */
	}

	if (ret > 0) {
		netPath->sentPackets++;
		netPath->sentPacketsTotal++;
	}

	return ret;
}



/*
 * refresh IGMP on a timeout
 */
/*
 * @return TRUE if successful
 */
Boolean
netRefreshIGMP(NetPath * netPath, const RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	DBG("netRefreshIGMP\n");

	if(netPath->joinedGeneral) {
		netShutdownMulticastIPv4(netPath, netPath->multicastAddr);
		netPath->multicastAddr = 0;
	}

	if(netPath->joinedPeer) {
		netShutdownMulticastIPv4(netPath, netPath->peerMulticastAddr);
		netPath->peerMulticastAddr = 0;
	}

	/* suspend process 100 milliseconds, to make sure the kernel sends the IGMP_leave properly */
	usleep(100*1000);

	if (!netInitMulticast(netPath, rtOpts)) {
		return FALSE;
	}

	return TRUE;
}

Boolean netIoctlHelper(struct ifreq *ifr, const char* ifaceName, unsigned long request) {

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
	PERROR("ioctlHelper: could not create helper socket");
	return FALSE;
    }

    strncpy(ifr->ifr_name, ifaceName, IFACE_NAME_LENGTH);

    if (ioctl(sockfd, request, ifr) < 0) {
            DBG("ioctlHelper: failed to call ioctl 0x%x on %s: %s\n", request, ifaceName, strerror(errno));
	    close(sockfd);
	    return FALSE;
    }
    close(sockfd);
    return TRUE;

}

static Boolean bondQuery(char *ifaceName, ifbond *ifb)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    memset(ifb, 0, sizeof(ifbond));
    ifr.ifr_data = (caddr_t)ifb;
    return netIoctlHelper(&ifr, ifaceName, SIOCBONDINFOQUERY);

}

static Boolean bondSlaveQuery(char *ifaceName, ifslave *ifs, int member)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    memset(ifs, 0, sizeof(ifslave));
    ifs->slave_id = member;
    ifr.ifr_data = (caddr_t)ifs;
    return netIoctlHelper(&ifr, ifaceName, SIOCBONDSLAVEINFOQUERY);

}

static int getBondSlaves(char * ifaceName, BondInfo *info)
{

    ifbond ifb;
    ifslave ifs;

    memset(&ifb, 0, sizeof(ifb));
    memset(&ifs, 0, sizeof(ifs));


    memset(&info->activeSlave, 0, sizeof(BondSlave));

    info->activeSlave.id = -1;

    for(int i = 0; i < BOND_SLAVES_MAX; i++) {
	memset(&info->slaves[i], 0, sizeof(BondSlave));
	info->slaves[i].id = -1;
    }

    if(bondQuery(ifaceName, &ifb)) {

	if(ifb.num_slaves == 0) return -1;

	for(int i = 0; i < ifb.num_slaves; i++) {
	    if(bondSlaveQuery(ifaceName, &ifs, i) && ifs.state == BOND_STATE_ACTIVE) {
		strncpy(info->activeSlave.name, ifs.slave_name, IFACE_NAME_LENGTH);
		info->activeSlave.id = i;
	    break;
	    }
	}

	for(int i = 0; (i < ifb.num_slaves) && (i < BOND_SLAVES_MAX); i++) {
	    if(bondSlaveQuery(ifaceName, &ifs, i)) {
		strncpy(info->slaves[i].name, ifs.slave_name, IFACE_NAME_LENGTH);
		info->slaves[i].id = i;
	    }
	}

    }

    return ( ifb.num_slaves );

}


static void getBondInfo(char *ifaceName, BondInfo *info)
{

    ifbond ifb;
    BondInfo lastInfo;

    if(!bondQuery(ifaceName, &ifb)) {
	info->bonded = FALSE;
	return;
    }

    memset(&lastInfo, 0, sizeof(BondInfo));

    if(info->updated) {
	memcpy(&lastInfo, info, sizeof(BondInfo));
    }

    info->bonded = TRUE;
    info->activeBackup = (ifb.bond_mode == BOND_MODE_ACTIVEBACKUP);
    info->slaveCount = getBondSlaves(ifaceName, info);

    if(info->activeSlave.id >= 0) {
	info->activeCount = 1;
    } else {
	info->activeCount = 0;
    }

    if(info->updated) {
	if (strncmp(lastInfo.activeSlave.name, info->activeSlave.name, IFACE_NAME_LENGTH)) {
	    info->activeChanged = TRUE;
	} else if (lastInfo.slaveCount != info->slaveCount) {
	    info->countChanged = TRUE;
	} else {
	    info->activeChanged = FALSE;
	    info->countChanged = FALSE;
	}
    }

    info->updated = TRUE;
    return;

}

static void getVlanInfo(char* ifaceName, VlanInfo *info)
{

    struct vlan_ioctl_args args;

    int sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
	PERROR("ioctlHelper: could not create helper socket");
	info->vlan = FALSE;
	return;
    }

    memset(&args, 0, sizeof(struct vlan_ioctl_args));
    strncpy(args.device1, ifaceName, min(sizeof(args.device1), IFACE_NAME_LENGTH));
    args.cmd = GET_VLAN_REALDEV_NAME_CMD;
    if (ioctl(sockfd, SIOCGIFVLAN, &args) < 0) {
            DBG("getVlanInfo: failed to call SIOCGIFVLAN ioctl on %s: %s\n", ifaceName, strerror(errno));
	    close(sockfd);
	    info->vlan = FALSE;
	    return;
    }

    info->vlan = TRUE;

    strncpy(info->realDevice, args.u.device2, min(sizeof(args.device1), IFACE_NAME_LENGTH));
    memset(&args, 0, sizeof(struct vlan_ioctl_args));
    strncpy(args.device1, ifaceName, min(sizeof(args.device1), IFACE_NAME_LENGTH));
    args.cmd = GET_VLAN_VID_CMD;
    if (ioctl(sockfd, SIOCGIFVLAN, &args) < 0) {
            DBG("getVlanInfo: failed to call SIOCGIFVLAN ioctl on %s: %s\n", ifaceName, strerror(errno));
	    close(sockfd);
	    info->vlan = FALSE;
	    return;
    }

    info->vlanId = args.u.VID;


    close(sockfd);
    return;


}

void updateInterfaceInfo(NetPath * netPath, RunTimeOpts * rtOpts, PtpClock * ptpClock)
{

    BondInfo *bondInfo = &netPath->interfaceInfo.bondInfo;
    VlanInfo *vlanInfo = &netPath->interfaceInfo.vlanInfo;
    char * realDevice = rtOpts->ifaceName;
    char * ifaceName = rtOpts->ifaceName;


    getVlanInfo(rtOpts->ifaceName, vlanInfo);

    if(vlanInfo->vlan) {
	    ifaceName = vlanInfo->realDevice;
	    realDevice = ifaceName;
    }

    getBondInfo(ifaceName, bondInfo);


    if(bondInfo->bonded && bondInfo->activeCount > 0) {
	    realDevice = bondInfo->activeSlave.name;
    }

    strncpy(netPath->interfaceInfo.physicalDevice, realDevice, IFACE_NAME_LENGTH);

    if(bondInfo->activeChanged) {
	ptpClock->ignoreOffsetUpdates = 2;
	ptpClock->ignoreDelayUpdates = 2;
		if(bondInfo->activeCount == 0) {
		    WARNING("Bonding: no active slaves on %s! Interface down\n", realDevice);
		} else {
		    WARNING("Bonding: %s active bond member changed to %s\n", realDevice,
			bondInfo->activeSlave.name);
		    if(rtOpts->refreshIgmp) {
			INFO("Re-sending IGMP joins\n");
		    }
		    netInitHwTimestamping(netPath, rtOpts);
		    ptpClock->clockDriver->setReference(ptpClock->clockDriver, NULL);

		    controlClockDrivers(CD_NOTINUSE);
		    prepareClockDrivers(&ptpClock->netPath, ptpClock, rtOpts);
		    createClockDriversFromString(rtOpts->extraClocks, rtOpts, TRUE);
		    /* clean up unused clock drivers */
		    controlClockDrivers(CD_CLEANUP);
		    reconfigureClockDrivers(rtOpts);

		    netRefreshIGMP(netPath, rtOpts, ptpClock);

		    if(rtOpts->calibrationDelay) {
			ptpClock->isCalibrated = FALSE;
			timerStart(&ptpClock->timers[CALIBRATION_DELAY_TIMER], rtOpts->calibrationDelay);
		    }
		}
    } else if(bondInfo->countChanged) {
	WARNING("Bonding: %s slave count changed, checking clocks\n", realDevice);

	ptpClock->clockDriver->setReference(ptpClock->clockDriver, NULL);

	controlClockDrivers(CD_NOTINUSE);
	prepareClockDrivers(&ptpClock->netPath, ptpClock, rtOpts);
	createClockDriversFromString(rtOpts->extraClocks, rtOpts, TRUE);
	/* clean up unused clock drivers */
	controlClockDrivers(CD_CLEANUP);
	reconfigureClockDrivers(rtOpts);
    }
}

Boolean getTsInfo(const char *ifaceName, struct ethtool_ts_info *info) {

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	memset(info, 0, sizeof(struct ethtool_ts_info));
	info->cmd = ETHTOOL_GET_TS_INFO;
	ifr.ifr_data = (char*) info;
	if(!netIoctlHelper(&ifr, ifaceName, SIOCETHTOOL)) {
	    PERROR("Could not get ethtool information from %s", ifaceName);
	    return FALSE;
	}

	return TRUE;

}

Boolean getHwTs(const char *ifaceName, const RunTimeOpts *rtOpts, HwTsInfo *target, Boolean quiet) {

	HwTsInfo output;
	memset(&output, 0, sizeof(HwTsInfo));

	static const OptionName hwtsMode[] = {
	    {SOF_TIMESTAMPING_TX_HARDWARE, "SOF_TIMESTAMPING_TX_HARDWARE"},
	    {SOF_TIMESTAMPING_RX_HARDWARE, "SOF_TIMESTAMPING_RX_HARDWARE"},
	    {SOF_TIMESTAMPING_RAW_HARDWARE, "SOF_TIMESTAMPING_RAW_HARDWARE"},
	    {-1}
	};

	static const OptionName swtsMode[] = {
	    {SOF_TIMESTAMPING_TX_SOFTWARE, "SOF_TIMESTAMPING_TX_SOFTWARE"},
	    {SOF_TIMESTAMPING_RX_SOFTWARE, "SOF_TIMESTAMPING_RX_SOFTWARE"},
	    {SOF_TIMESTAMPING_SOFTWARE, "SOF_TIMESTAMPING_SOFTWARE"},
	    {-1}
	};

	/* "sensible" order - last mode will not support timestamping delay.
	 * Preferring ALL to allow VLAN timestamping etc.
	 */
	static const OptionName rxFilters[] = {
	    { HWTSTAMP_FILTER_PTP_V2_EVENT, "HWTSTAMP_FILTER_PTP_V2_EVENT"},
	    { HWTSTAMP_FILTER_PTP_V2_L4_EVENT, "HWTSTAMP_FILTER_PTP_V2_L4_EVENT"},
	    { HWTSTAMP_FILTER_ALL, "HWTSTAMP_FILTER_ALL"},
	    { HWTSTAMP_FILTER_PTP_V2_L4_SYNC, "HWTSTAMP_FILTER_PTP_V2_L4_SYNC"},
	    {-1}
	};
#define HWTSTAMP_TX_ONESTEP_SYNC 2
	/* "sensible" order - last mode will not support timestamping delay */
	static const OptionName txTypes[] = {
	    { HWTSTAMP_TX_ON, "HWTSTAMP_TX_ON"},
	    { HWTSTAMP_TX_ONESTEP_SYNC, "HWTSTAMP_TX_ONESTEP_SYNC"},
	    {-1}
	};

	struct ethtool_ts_info info;

	if(!getTsInfo(ifaceName, &info)) {
	    return FALSE;
	}

	for(const OptionName *mode = hwtsMode; mode->value != -1; mode++) {
	    if((info.so_timestamping & mode->value) == mode->value) {
		DBG("hwts: Interface %s supports mode %s\n", ifaceName, mode->name);
		output.tsMode |= mode->value;
	    } else {
		output.tsMode = 0;

		if(!quiet && !rtOpts->hwTimestamping) NOTICE("Interface %s does not support required hardware timestamping mode %s\n", ifaceName, mode->name);

		for(const OptionName *mode = swtsMode; mode->value != -1; mode++) {
		    if((info.so_timestamping & mode->value) == mode->value) {
		    DBG("hwts: Interface %s supports mode %s\n", ifaceName, mode->name);
		    output.tsMode |= mode->value;
		    } else {
			if(!quiet) NOTICE("Interface %s does not support required software timextamping mode  %s\n", ifaceName, mode->name);
			output.tsMode = 0;
			return FALSE;
		    }
		}
		output.txTimestamping = TRUE;
		output.hwTimestamping = FALSE;
		if(target != NULL) {
		    *target = output;
		}
		return TRUE;
	    }
	}

	for(const OptionName *mode = rxFilters; mode->value != -1; mode++) {
	    if(info.rx_filters & (1 << mode->value) ) {
		DBG("hwts: Interface %s supports RX filter %s\n", ifaceName, mode->name);
		output.rxFilter = mode->value;
		break;
	    } else {
		DBG("Interface %s does not support RX filter %s\n", ifaceName, mode->name);
	    }
	}

	if(!output.rxFilter) {
		if(!quiet) ERROR("Interface %s does not support suitable hardware timestamping filters \n", ifaceName);
		return FALSE;
	}

	if(info.tx_types & (1 << HWTSTAMP_TX_ON)) {
		DBG("hwts: Interface %s supports HWTSTAMP_TX_ON\n", ifaceName);
		output.txType = HWTSTAMP_TX_ON;
	} else {
		if(!quiet) ERROR("Interface %s does not support hardware transmit timestamps(HWTSTAMP_TX_ON)\n");
		return FALSE;
	}
	output.hwTimestamping = TRUE;
	output.txTimestamping = TRUE;

	for(const OptionName *mode = txTypes; mode->value != -1; mode++) {
	    if(info.tx_types & (1 << mode->value) ) {
		DBG("hwts: Interface %s supports TX type %s\n", ifaceName, mode->name);
	    } else {
		DBG("Interface %s does not support TX  type %s\n", ifaceName, mode->name);
	    }
	}

	if(target != NULL) {
	    *target = output;
	}

	return TRUE;

}

Boolean initHwTs(char *ifaceName, HwTsInfo *info) {

	struct ifreq ifr;
	struct hwtstamp_config config;

	memset(&ifr, 0, sizeof(ifr));
	memset(&config, 0, sizeof(config));

	config.tx_type = info->txType;
	config.rx_filter = info->rxFilter;

	ifr.ifr_data = (void*) &config;

	if(!netIoctlHelper(&ifr, ifaceName, SIOCSHWTSTAMP)) {
	    PERROR("Could not init hardware timestamping on %s", ifaceName);
	    return FALSE;
	}

	if(config.tx_type != info->txType) {
	    ERROR("Interface %s refused to set the required hardware transmit timestamping option\n",
	    ifaceName);
	    return FALSE;
	}

	/* if the driver has changed the RX filter, check if we can use what we got */
	if(config.rx_filter != info->rxFilter) {
	    if((config.rx_filter == HWTSTAMP_FILTER_PTP_V2_EVENT) ||
		(config.rx_filter == HWTSTAMP_FILTER_PTP_V2_L4_EVENT) ||
		(config.rx_filter == HWTSTAMP_FILTER_ALL) ||
		(config.rx_filter == HWTSTAMP_FILTER_PTP_V2_L4_SYNC)) {
		WARNING("Interface %s changed hardware receive filter type from %d to %d - still acceptable\n",
		ifaceName, info->rxFilter, config.rx_filter);
	    /* nope. */
	    } else {
		ERROR("Interface %s changed hardware received filter type from %d to %d - cannot continue\n",
		ifaceName, info->rxFilter, config.rx_filter);
		return FALSE;
	    }
	}

	return TRUE;

}

static ClockDriver *
getClockDriver(const char *ifaceName, RunTimeOpts *rtOpts)
{

    ClockDriver *cd = NULL;
    HwTsInfo info;

    cd = findClockDriver(ifaceName);

    if(cd != NULL) {
	return cd;
    }

    memset(&info, 0, sizeof(HwTsInfo));

    if(!getHwTs(ifaceName, rtOpts, &info, TRUE) || !info.hwTimestamping) {
	cd = getSystemClock();
    } else {
	cd = createClockDriver(CLOCKDRIVER_LINUXPHC, ifaceName);
    }

    if(cd != NULL && !cd->_init) {
	cd->init(cd, ifaceName);
    }

    return cd;

}

Boolean
prepareClockDrivers(NetPath *netPath, PtpClock *ptpClock, RunTimeOpts *rtOpts) {

	BondInfo *bi = &netPath->interfaceInfo.bondInfo;
	ClockDriver *cd;

	getSystemClock()->pushConfig(getSystemClock(), rtOpts);

	if(rtOpts->hwTimestamping && netPath->hwTimestamping) {

	    ptpClock->clockDriver = NULL;
	    ptpClock->clockDriver = getClockDriver(netPath->interfaceInfo.physicalDevice, rtOpts);

	    if ((ptpClock->clockDriver == NULL) || !ptpClock->clockDriver->_init) {
		    ERROR("Could not start clock driver for primary interface %s\n",
			netPath->interfaceInfo.physicalDevice);
		    return FALSE;
	    }

	} else {
	    ptpClock->clockDriver = getSystemClock();
	}

	if(rtOpts->hwTimestamping && bi->bonded) {
	    for(int i = 0; i < bi->slaveCount; i++) {
		cd = getClockDriver(bi->slaves[i].name, rtOpts);
		if((cd != NULL) && !cd->systemClock && strlen(bi->slaves[i].name)) {
		    if(!cd->_init) {
			cd->init(cd, bi->slaves[i].name);
		    }
		    cd->config.required = TRUE;
		    cd->pushConfig(cd, rtOpts);
		    cd->inUse = TRUE;
		}
	    }
	}

	ptpClock->clockDriver->pushConfig(ptpClock->clockDriver, rtOpts);
	ptpClock->clockDriver->inUse = TRUE;
	ptpClock->clockDriver->config.required = TRUE;
	ptpClock->clockDriver->owner = ptpClock;
	ptpClock->clockDriver->callbacks.onStep = clockStepNotify;

	if(ptpClock->portDS.portState == PTP_SLAVE) {
	    ptpClock->clockDriver->setExternalReference(ptpClock->clockDriver, "PTP", RC_PTP);
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

	if((lastMaster != NULL) && (lastMaster != ptpClock->masterClock)) {
	    lastMaster->setReference(lastMaster, NULL);
	    lastMaster->setState(lastMaster, CS_FREERUN);
	}

	if((ptpClock->masterClock != NULL) && (ptpClock->masterClock != ptpClock->clockDriver)) {
	    if( (ptpClock->portDS.portState == PTP_MASTER) || (ptpClock->portDS.portState == PTP_PASSIVE)) {
		ptpClock->masterClock->setExternalReference(ptpClock->masterClock, "PREFMST", RC_EXTERNAL);
	    }
	}

	return TRUE;


}

