/*-
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

#ifdef HAVE_PCAP_PCAP_H
#include <pcap/pcap.h>
#else /* !HAVE_PCAP_PCAP_H */
/* Cases like RHEL5 and others where only pcap.h exists */
#ifdef HAVE_PCAP_H
#include <pcap.h>
#endif /* HAVE_PCAP_H */
#endif

#define PCAP_TIMEOUT 1 /* expressed in milliseconds */

#if defined PTPD_SNMP
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#endif
#if HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#else /* !HAVE_NETINET_ETHER_H */
#include <net/ethernet.h>
#endif /* HAVE_NETINET_ETHER_H */


/* choose kernel-level nanoseconds or microseconds resolution on the client-side */
#if !defined(SO_TIMESTAMPING) && !defined(SO_TIMESTAMPNS) && !defined(SO_TIMESTAMP) && !defined(SO_BINTIME)
#error No kernel-level support for packet timestamping detected!
#endif

#ifdef SO_TIMESTAMPING
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#endif /* SO_TIMESTAMPING */

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

/* shut down the UDP stuff */
Boolean 
netShutdown(NetPath * netPath)
{
	netShutdownMulticast(netPath);

	netPath->unicastAddr = 0;

	/* Close sockets */
	if (netPath->eventSock > 0)
		close(netPath->eventSock);
	netPath->eventSock = -1;

	if (netPath->generalSock > 0)
		close(netPath->generalSock);
	netPath->generalSock = -1;

	if (netPath->pcapEvent != NULL) {
		pcap_close(netPath->pcapEvent);
		netPath->pcapEventSock = -1;
	}
	if (netPath->pcapGeneral != NULL) {
		pcap_close(netPath->pcapGeneral);
		netPath->pcapGeneralSock = -1;
	}

	freeIpv4AccessList(netPath->timingAcl);
	freeIpv4AccessList(netPath->managementAcl);

	return TRUE;
}


Boolean
chooseMcastGroup(RunTimeOpts * rtOpts, struct in_addr *netAddr)
{

	char *addrStr;

#ifdef PTPD_EXPERIMENTAL
	switch(rtOpts->mcast_group_Number){
	case 0:
		addrStr = DEFAULT_PTP_DOMAIN_ADDRESS;
		break;

	case 1:
		addrStr = ALTERNATE_PTP_DOMAIN1_ADDRESS;
		break;
	case 2:
		addrStr = ALTERNATE_PTP_DOMAIN2_ADDRESS;
		break;
	case 3:
		addrStr = ALTERNATE_PTP_DOMAIN3_ADDRESS;
		break;

	default:
		ERROR("Unk group %d\n", rtOpts->mcast_group_Number);
		exit(3);
		break;
	}
#else
	addrStr = DEFAULT_PTP_DOMAIN_ADDRESS;
#endif

	if (!inet_aton(addrStr, netAddr)) {
		ERROR("failed to encode multicast address: %s\n", addrStr);
		return FALSE;
	}
	return TRUE;
}


/*Test if network layer is OK for PTP*/
UInteger8 
lookupCommunicationTechnology(UInteger8 communicationTechnology)
{
#if defined(linux)
	switch (communicationTechnology) {
	case ARPHRD_ETHER:
	case ARPHRD_EETHER:
	case ARPHRD_IEEE802:
		return PTP_ETHER;
	default:
		break;
	}
#endif  /* defined(linux) */

	return PTP_DEFAULT;
}

Boolean
testInterface(char * ifaceName)
{
	UInteger8 ct;
	NetPath np;
        /* open a test socket */
	if ((np.eventSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		DBG("testInterface: error opening test socket\n");
		return FALSE;
	/* Attempt getting an IP address */
        } else if (!findIface(ifaceName,
		&ct,np.port_uuid_field, &np)) {
				DBG("testInterface: findIface returned FALSE");
		return FALSE;
	}
	close(np.eventSock);
	return TRUE;
}

 /* Find the local network interface */
UInteger32
findIface(Octet * ifaceName, UInteger8 * communicationTechnology,
    Octet * uuid, NetPath * netPath)
{
#if defined(linux)

	/* depends on linux specific ioctls (see 'netdevice' man page) */
	int i, flags;
	struct ifconf data;
	struct ifreq device[IFCONF_LENGTH];

	data.ifc_len = sizeof(device);
	data.ifc_req = device;

	memset(data.ifc_buf, 0, data.ifc_len);

	flags = IFF_UP | IFF_RUNNING | IFF_MULTICAST;

	/* look for an interface if none specified */
	if (ifaceName[0] != '\0') {
		i = 0;
		memcpy(device[i].ifr_name, ifaceName, IFACE_NAME_LENGTH);

		if (ioctl(netPath->eventSock, SIOCGIFHWADDR, &device[i]) < 0)
			DBGV("failed to get hardware address\n");
		else if ((*communicationTechnology = 
			  lookupCommunicationTechnology(
				  device[i].ifr_hwaddr.sa_family)) 
			 == PTP_DEFAULT)
			DBGV("unsupported communication technology (%d)\n", 
			     *communicationTechnology);
		else
			memcpy(uuid, device[i].ifr_hwaddr.sa_data, 
			       PTP_UUID_LENGTH);
	} else {
		/* no iface specified */
		/* get list of network interfaces */
		if (ioctl(netPath->eventSock, SIOCGIFCONF, &data) < 0) {
			PERROR("failed query network interfaces");
			return 0;
		}
		if (data.ifc_len >= sizeof(device))
			DBG("device list may exceed allocated space\n");

		/* search through interfaces */
		for (i = 0; i < data.ifc_len / sizeof(device[0]); ++i) {
			DBGV("%d %s %s\n", i, device[i].ifr_name, 
			     inet_ntoa(((struct sockaddr_in *)
					&device[i].ifr_addr)->sin_addr));

			if (ioctl(netPath->eventSock, SIOCGIFFLAGS, 
				  &device[i]) < 0)
				DBGV("failed to get device flags\n");
			else if ((device[i].ifr_flags & flags) != flags)
				DBGV("does not meet requirements"
				     "(%08x, %08x)\n", device[i].ifr_flags, 
				     flags);
			else if (ioctl(netPath->eventSock, SIOCGIFHWADDR, 
				       &device[i]) < 0)
				DBGV("failed to get hardware address\n");
			else if ((*communicationTechnology = 
				  lookupCommunicationTechnology(
					  device[i].ifr_hwaddr.sa_family)) 
				 == PTP_DEFAULT)
				DBGV("unsupported communication technology"
				     "(%d)\n", *communicationTechnology);
			else {
				DBGV("found interface (%s)\n", 
				     device[i].ifr_name);
				memcpy(uuid, device[i].ifr_hwaddr.sa_data, 
				       PTP_UUID_LENGTH);
				memcpy(ifaceName, device[i].ifr_name, 
				       IFACE_NAME_LENGTH);
				break;
			}
		}
	}

	if (ifaceName[0] == '\0') {
		ERROR("failed to find a usable interface\n");
		return 0;
	}
	if (ioctl(netPath->eventSock, SIOCGIFADDR, &device[i]) < 0) {
		PERROR("failed to get ip address");
		return 0;
	}

	if (*communicationTechnology == PTP_DEFAULT) {
		UInteger32 sourceIPaddr=((struct sockaddr_in *)&device[i].ifr_addr)->sin_addr.s_addr;
		// replace first 4 bytes of 6 byte uuid with interface ip address to ensure unique uuid
		// for non ethernet networks
		memcpy(uuid, &sourceIPaddr, 4);
		DBG("using ip address instead of mac address for uuid\n");
	}

	return ((struct sockaddr_in *)&device[i].ifr_addr)->sin_addr.s_addr;

#else /* usually *BSD */

	struct ifaddrs *if_list, *ifv4, *ifh;

	if (getifaddrs(&if_list) < 0) {
		PERROR("getifaddrs() failed");
		return FALSE;
	}
	/* find an IPv4, multicast, UP interface, right name(if supplied) */
	for (ifv4 = if_list; ifv4 != NULL; ifv4 = ifv4->ifa_next) {
		if ((ifv4->ifa_flags & IFF_UP) == 0)
			continue;
		if ((ifv4->ifa_flags & IFF_RUNNING) == 0)
			continue;
		if ((ifv4->ifa_flags & IFF_LOOPBACK))
			continue;
		if ((ifv4->ifa_flags & IFF_MULTICAST) == 0)
			continue;
                /* must have IPv4 address */
		if (ifv4->ifa_addr->sa_family != AF_INET)
			continue;
		if (ifaceName[0] && strncmp(ifv4->ifa_name, ifaceName, 
					    IF_NAMESIZE) != 0)
			continue;
		break;
	}

	if (ifv4 == NULL) {
		if (ifaceName[0]) {
			ERROR("interface \"%s\" does not exist,"
			      "or is not appropriate\n", ifaceName);
			return FALSE;
		}
		ERROR("no suitable interfaces found!");
		return FALSE;
	}
	/* find the AF_LINK info associated with the chosen interface */
	for (ifh = if_list; ifh != NULL; ifh = ifh->ifa_next) {
		if (ifh->ifa_addr->sa_family != AF_LINK)
			continue;
		if (strncmp(ifv4->ifa_name, ifh->ifa_name, IF_NAMESIZE) == 0)
			break;
	}

	if (ifh == NULL) {
		ERROR("could not get hardware address for interface \"%s\"\n", 
		      ifv4->ifa_name);
		return FALSE;
	}
	/* check that the interface TYPE is OK */
	switch (((struct sockaddr_dl *)ifh->ifa_addr)->sdl_type) {
		case IFT_ETHER:
		case IFT_L2VLAN:
			break;
		default:
			ERROR("\"%s\" is not an ethernet interface!\n", ifh->ifa_name);
			return FALSE;
	}
	DBG("==> %s %s %s\n", ifv4->ifa_name,
	    inet_ntoa(((struct sockaddr_in *)ifv4->ifa_addr)->sin_addr),
	    ether_ntoa((struct ether_addr *)
		       LLADDR((struct sockaddr_dl *)ifh->ifa_addr))
	    );

	*communicationTechnology = PTP_ETHER;
	memcpy(ifaceName, ifh->ifa_name, IFACE_NAME_LENGTH);
	memcpy(uuid, LLADDR((struct sockaddr_dl *)ifh->ifa_addr), 
	       PTP_UUID_LENGTH);

	return ((struct sockaddr_in *)ifv4->ifa_addr)->sin_addr.s_addr;

#endif
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
		       &imr.imr_interface.s_addr, sizeof(struct in_addr)) < 0
	    || setsockopt(netPath->generalSock, IPPROTO_IP, IP_MULTICAST_IF, 
			  &imr.imr_interface.s_addr, sizeof(struct in_addr)) 
	    < 0) {
		PERROR("failed to enable multi-cast on the interface");
		return FALSE;
	}
	/* join multicast group (for receiving) on specified interface */
	if (setsockopt(netPath->eventSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
		       &imr, sizeof(struct ip_mreq)) < 0
	    || setsockopt(netPath->generalSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			  &imr, sizeof(struct ip_mreq)) < 0) {
		PERROR("failed to join the multi-cast group");
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
Boolean
netInitMulticast(NetPath * netPath,  RunTimeOpts * rtOpts)
{
	struct in_addr netAddr;
	char addrStr[NET_ADDRESS_LENGTH];
	
	/* Init General multicast IP address */
	if(!chooseMcastGroup(rtOpts, &netAddr)){
		return FALSE;
	}
	netPath->multicastAddr = netAddr.s_addr;
	if(!netInitMulticastIPv4(netPath, netPath->multicastAddr)) {
		return FALSE;
	}
	/* End of General multicast Ip address init */


	/* Init Peer multicast IP address */
	memcpy(addrStr, PEER_PTP_DOMAIN_ADDRESS, NET_ADDRESS_LENGTH);

	if (!inet_aton(addrStr, &netAddr)) {
		ERROR("failed to encode multi-cast address: %s\n", addrStr);
		return FALSE;
	}
	netPath->peerMulticastAddr = netAddr.s_addr;
	if(!netInitMulticastIPv4(netPath, netPath->peerMulticastAddr)) {
		return FALSE;
	}
	/* End of Peer multicast Ip address init */
	
	return TRUE;
}

Boolean
netSetMulticastLoopback(NetPath * netPath, Boolean value) {
	int temp = value ? 1 : 0;

	DBG("Going to set multicast loopback with %d \n", temp);

	if (setsockopt(netPath->eventSock, IPPROTO_IP, IP_MULTICAST_LOOP, 
	       &temp, sizeof(int)) < 0) {
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
	struct timeval timeOut = {0,0};
	int val = 1;
	if(netPath->txTimestampFailure)
		goto end;

	FD_ZERO(&tmpSet);
	FD_SET(netPath->eventSock, &tmpSet);

	if(select(netPath->eventSock + 1, &tmpSet, NULL, NULL, &timeOut) > 0) {
		if (FD_ISSET(netPath->eventSock, &tmpSet)) {
			length = netRecvEvent(G_ptpClock->msgIbuf, timeStamp, 
			    netPath, MSG_ERRQUEUE);

			if (length > 0) {
				DBG("Grabbed sent msg via errqueue: %d bytes, at %d.%d\n", length, timeStamp->seconds, timeStamp->nanoseconds);
				return TRUE;
			} else if (length < 0) {
				DBG("Failed to poll error queue for SO_TIMESTAMPING transmit time");
				G_ptpClock->counters.messageRecvErrors++;
				goto end;
			} else if (length == 0) {
				DBG("Received no data from TX error queue");
				goto end;
			}
		}
	} else {
		DBG("SO_TIMESTAMPING - t timeout on TX timestamp - will use loop from now on\n");
		return FALSE;
	}
end:
		if(setsockopt(netPath->eventSock, SOL_SOCKET, SO_TIMESTAMPNS, &val, sizeof(int)) < 0) {
			DBG("gettxtimestamp: failed to revert to SO_TIMESTAMPNS");
		}

		return FALSE;
}
#endif /* SO_TIMESTAMPING */


/**
 * Initialize timestamping of packets
 *
 * @param netPath 
 * 
 * @return TRUE if successful
 */
Boolean 
netInitTimestamping(NetPath * netPath, RunTimeOpts * rtOpts)
{

	int val = 1;
	Boolean result = TRUE;
#if defined(SO_TIMESTAMPING) && defined(SO_TIMESTAMPNS)/* Linux - current API */
	DBG("netInitTimestamping: trying to use SO_TIMESTAMPING\n");
	val = SOF_TIMESTAMPING_TX_SOFTWARE |
	    SOF_TIMESTAMPING_RX_SOFTWARE |
	    SOF_TIMESTAMPING_SOFTWARE;

/* unless compiled with PTPD_EXPERIMENTAL, check if we support the desired tstamp capabilities */
#ifndef PTPD_EXPERIMENTAL
#ifdef ETHTOOL_GET_TS_INFO

       struct ethtool_ts_info tsInfo;
	struct ifreq ifRequest;
        int res;

        memset(&tsInfo, 0, sizeof(tsInfo));
        memset(&ifRequest, 0, sizeof(ifRequest));
        tsInfo.cmd = ETHTOOL_GET_TS_INFO;
        strncpy( ifRequest.ifr_name, rtOpts->ifaceName, IFNAMSIZ - 1);
        ifRequest.ifr_data = (char *) &tsInfo;
        res = ioctl(netPath->eventSock, SIOCETHTOOL, &ifRequest);

	if (res < 0) {
		PERROR("Could not retrieve ethtool information for %s",
			    rtOpts->ifaceName);
			    return FALSE;
	}

	if(!(tsInfo.so_timestamping & val)) {
		DBGV("Required SO_TIMESTAMPING flags not supported - reverting to SO_TIMESTAMPNS\n");
		val = 1;
		netPath->txTimestampFailure = FALSE;
	}


#else
	netPath->txTimestampFailure = FALSE;
	val = 1;
#endif /* ETHTOOL_GET_TS_INFO */
#endif /* PTPD_EXPERIMENTAL */

	if((val==1 && (setsockopt(netPath->eventSock, SOL_SOCKET, SO_TIMESTAMPNS, &val, sizeof(int)) < 0)) ||
		(setsockopt(netPath->eventSock, SOL_SOCKET, SO_TIMESTAMPING, &val, sizeof(int)) < 0)) {
		PERROR("netInitTimestamping: failed to enable SO_TIMESTAMPING");
		result = FALSE;
	} else {
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
 * start all of the UDP stuff 
 * must specify 'subdomainName', and optionally 'ifaceName', 
 * if not then pass ifaceName == "" 
 * on socket options, see the 'socket(7)' and 'ip' man pages 
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
	int temp;
	struct in_addr interfaceAddr;
	struct sockaddr_in addr;
	struct bpf_program program;
	char errbuf[PCAP_ERRBUF_SIZE];
	pid_t job = htonl(getpid());
       
	DBG("netInit\n");

	netPath->pcapEvent = NULL;
	netPath->pcapGeneral = NULL;
	netPath->pcapEventSock = -1;
	netPath->pcapGeneralSock = -1;

	if (rtOpts->transport == IEEE_802_3) {
		netPath->headerOffset = PACKET_BEGIN_ETHER;
		memcpy(netPath->etherDest.octet, ether_aton(PTP_ETHER_DST), ETHER_ADDR_LEN);
		memcpy(netPath->peerEtherDest.octet, ether_aton(PTP_ETHER_PEER), ETHER_ADDR_LEN);
	} else
		netPath->headerOffset = PACKET_BEGIN_UDP;

	if (rtOpts->jobid) {
		memset(netPath->port_uuid_field, 0, PTP_UUID_LENGTH);
		memcpy(netPath->port_uuid_field + (PTP_UUID_LENGTH - sizeof(job)),
		    (pid_t *)&job, sizeof(job));
	}

	/* open sockets */
	if ((netPath->eventSock = socket(PF_INET, SOCK_DGRAM, 
					 IPPROTO_UDP)) < 0
	    || (netPath->generalSock = socket(PF_INET, SOCK_DGRAM, 
					      IPPROTO_UDP)) < 0) {
		PERROR("failed to initialize sockets");
		return FALSE;
	}

	/* find a network interface */
	if (!(interfaceAddr.s_addr = 
	      findIface(rtOpts->ifaceName, 
			&ptpClock->port_communication_technology,
			netPath->port_uuid_field, netPath)))
		return FALSE;

	DBG("Listening on IP: %s\n",inet_ntoa(interfaceAddr));

	if (rtOpts->pcap == TRUE) {
		int promisc = (rtOpts->transport == IEEE_802_3 ) ? 1 : 0;
		if ((netPath->pcapEvent = pcap_open_live(rtOpts->ifaceName,
							 PACKET_SIZE, promisc,
							 PCAP_TIMEOUT,
							 errbuf)) == NULL) {
			PERROR("failed to open event pcap");
			return FALSE;
		}
		if (pcap_compile(netPath->pcapEvent, &program, 
				 ( rtOpts->transport == IEEE_802_3 ) ?
				    "ether proto 0x88f7":
				 ( rtOpts->ip_mode != IPMODE_MULTICAST ) ?
					 "udp port 319" :
				 "multicast and host 224.0.1.129 and udp port 319" ,
				 1, 0) < 0) {
			PERROR("failed to compile pcap event filter");
			pcap_perror(netPath->pcapEvent, "ptpd2");
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
					 ( rtOpts->ip_mode != IPMODE_MULTICAST ) ?
						 "udp port 320" :
					 "multicast and host 224.0.1.129 and udp port 320" ,
					 1, 0) < 0) {
				PERROR("failed to compile pcap general filter");
				pcap_perror(netPath->pcapGeneral, "ptpd2");
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
	
	if(rtOpts->transport == IEEE_802_3) {
		close(netPath->eventSock);
		netPath->eventSock = -1;
		close(netPath->generalSock);
		netPath->generalSock = -1;
	} else {
		
		/* save interface address for IGMP refresh */
		netPath->interfaceAddr = interfaceAddr;
		
		DBG("Local IP address used : %s \n", inet_ntoa(interfaceAddr));

		temp = 1;			/* allow address reuse */
		if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_REUSEADDR, 
			       &temp, sizeof(int)) < 0
		    || setsockopt(netPath->generalSock, SOL_SOCKET, SO_REUSEADDR, 
				  &temp, sizeof(int)) < 0) {
			DBG("failed to set socket reuse\n");
		}
		/* bind sockets */
		/*
		 * need INADDR_ANY to allow receipt of multi-cast and uni-cast
		 * messages
		 */
		if (rtOpts->jobid) {
			if (inet_pton(AF_INET, DEFAULT_PTP_DOMAIN_ADDRESS, &addr.sin_addr) < 0) {
				PERROR("failed to convert address");
				return FALSE;
			}
		} else
			addr.sin_addr.s_addr = htonl(INADDR_ANY);

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

		if ( rtOpts->ip_mode != IPMODE_HYBRID )
		if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_BINDTODEVICE,
				rtOpts->ifaceName, strlen(rtOpts->ifaceName)) < 0
			|| setsockopt(netPath->generalSock, SOL_SOCKET, SO_BINDTODEVICE,
				rtOpts->ifaceName, strlen(rtOpts->ifaceName)) < 0){
			PERROR("failed to call SO_BINDTODEVICE on the interface");
			return FALSE;
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

		/* send a uni-cast address if specified (useful for testing) */
		if(!hostLookup(rtOpts->unicastAddress, &netPath->unicastAddr)) {
	                netPath->unicastAddr = 0;
		}


		if(rtOpts->ip_mode != IPMODE_UNICAST)  {

			/* init UDP Multicast on both Default and Peer addresses */
			if (!netInitMulticast(netPath, rtOpts))
				return FALSE;

			/* set socket time-to-live  */
			if (setsockopt(netPath->eventSock, IPPROTO_IP, IP_MULTICAST_TTL,
				       &rtOpts->ttl, sizeof(int)) < 0
			    || setsockopt(netPath->generalSock, IPPROTO_IP, IP_MULTICAST_TTL,
					  &rtOpts->ttl, sizeof(int)) < 0) {
				PERROR("Failed to set socket multicast time-to-live");
				return FALSE;
			}

			/* start tracking TTL */
			netPath->ttlEvent = rtOpts->ttl;
			netPath->ttlGeneral = rtOpts->ttl;

		}


#ifdef SO_TIMESTAMPING
			/* Reset the failure indicator when (re)starting network */
			netPath->txTimestampFailure = FALSE;
			/* for SO_TIMESTAMPING we're receiving transmitted packets via ERRQUEUE */
			temp = 0;
#else
			/* enable loopback */
			temp = 1;
#endif

		/* make timestamps available through recvmsg() */
		if (!netInitTimestamping(netPath,rtOpts)) {
			ERROR("failed to enable receive time stamps");
			return FALSE;
		}

#ifdef SO_TIMESTAMPING
		/* If we failed to initialise SO_TIMESTAMPING, enable mcast loopback */
		if(netPath->txTimestampFailure)
			temp = 1;
#endif

			if(!netSetMulticastLoopback(netPath, temp)) {
				return FALSE;
			}


	}

	/* Compile ACLs */
	if(rtOpts->timingAclEnabled) {
    		freeIpv4AccessList(netPath->timingAcl);
		netPath->timingAcl=createIpv4AccessList(rtOpts->timingAclPermitText,
			rtOpts->timingAclDenyText, rtOpts->timingAclOrder);
	}
	if(rtOpts->managementAclEnabled) {
		freeIpv4AccessList(netPath->managementAcl);
		netPath->managementAcl=createIpv4AccessList(rtOpts->managementAclPermitText,
			rtOpts->managementAclDenyText, rtOpts->managementAclOrder);
	}

	return TRUE;
}

/*Check if data have been received*/
int 
netSelect(TimeInternal * timeout, NetPath * netPath, fd_set *readfds)
{
	int ret, nfds;
	struct timeval tv, *tv_ptr;


#if defined PTPD_SNMP
	extern RunTimeOpts rtOpts;
	struct timeval snmp_timer_wait = { 0, 0}; // initialise to avoid unused warnings when SNMP disabled
	int snmpblock = 0;
#endif

	if (timeout) {
		if(isTimeInternalNegative(timeout)) {
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
	if (netPath->eventSock >=0) 
		FD_SET(netPath->eventSock, readfds);
	if (netPath->generalSock >=0)
		FD_SET(netPath->generalSock, readfds);
	if (netPath->pcapEventSock >= 0)
		FD_SET(netPath->pcapEventSock, readfds);
	if (netPath->pcapGeneralSock >= 0)
		FD_SET(netPath->pcapGeneralSock, readfds);

	if (netPath->pcapGeneralSock > 0)
		nfds = netPath->pcapGeneralSock;
	else 
		if (netPath->pcapEventSock > netPath->pcapGeneralSock)
			nfds = netPath->pcapEventSock;
	else 
		if (netPath->eventSock > netPath->generalSock)
			nfds = netPath->eventSock;
		else
			nfds = netPath->generalSock;
	nfds++;

#if defined PTPD_SNMP
if (rtOpts.snmp_enabled) {
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
if (rtOpts.snmp_enabled) {
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

ssize_t 
netRecvEvent(Octet * buf, TimeInternal * time, NetPath * netPath, int flags)
{
	ssize_t ret = 0;
	struct msghdr msg;
	struct iovec vec[1];
	struct sockaddr_in from_addr;

	struct pcap_pkthdr *pkt_header;
	const u_char *pkt_data;

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

	if (netPath->pcapEvent == NULL) { /* Using sockets */
		vec[0].iov_base = buf;
		vec[0].iov_len = PACKET_SIZE;

		memset(&msg, 0, sizeof(msg));
		memset(&from_addr, 0, sizeof(from_addr));
		memset(buf, 0, PACKET_SIZE);
		memset(&cmsg_un, 0, sizeof(cmsg_un));

		msg.msg_name = (caddr_t)&from_addr;
		msg.msg_namelen = sizeof(from_addr);
		msg.msg_iov = vec;
		msg.msg_iovlen = 1;
		msg.msg_control = cmsg_un.control;
		msg.msg_controllen = sizeof(cmsg_un.control);
		msg.msg_flags = 0;

		ret = recvmsg(netPath->eventSock, &msg, flags | MSG_DONTWAIT);
		if (ret <= 0) {
			if (errno == EAGAIN || errno == EINTR)
				return 0;

			return ret;
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
			netPath->lastRecvAddr = from_addr.sin_addr.s_addr;
		netPath->receivedPackets++;

		if (msg.msg_controllen <= 0) {
			ERROR("received short ancillary data (%ld/%ld)\n",
			      (long)msg.msg_controllen, (long)sizeof(cmsg_un.control));

			return 0;
		}

		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
		     cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			if (cmsg->cmsg_level == SOL_SOCKET) {
#if defined(SO_TIMESTAMPING) && defined(SO_TIMESTAMPNS)
				if(cmsg->cmsg_type == SO_TIMESTAMPING || 
				    cmsg->cmsg_type == SO_TIMESTAMPNS) {
					ts = (struct timespec *)CMSG_DATA(cmsg);
					time->seconds = ts->tv_sec;
					time->nanoseconds = ts->tv_nsec;
					timestampValid = TRUE;
					DBG("rcvevent: SO_TIMESTAMP%s %s time stamp: %us %dns\n", netPath->txTimestampFailure ?
					    "NS" : "ING",
					    (flags & MSG_ERRQUEUE) ? "(TX)" : "(RX)" , time->seconds, time->nanoseconds);
					break;
				}
#elif defined(SO_TIMESTAMPNS)
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
	} else { /* Using PCAP */
		/* Discard packet on socket */
		if (netPath->eventSock >= 0) {
			recv(netPath->eventSock, buf, PACKET_SIZE, MSG_DONTWAIT);
		}
		
		if ((ret = pcap_next_ex(netPath->pcapEvent, &pkt_header, 
					&pkt_data)) < 1) {
			if (ret < 0)
				DBGV("netRecvEvent: pcap_next_ex failed %s\n",
				     pcap_geterr(netPath->pcapEvent));
			return 0;
		}

	/* Make sure this is IP (could dot1q get here?) */
	if( ntohs(*(u_short *)(pkt_data + 12)) != ETHERTYPE_IP)
		DBGV("PCAP payload received is not Ethernet: 0x%04x\n",
		    ntohs(*(u_short *)(pkt_data + 12)));
	/* Retrieve source IP from the payload - 14 eth + 12 IP */
	netPath->lastRecvAddr = *(Integer32 *)(pkt_data + 26);

		netPath->receivedPackets++;
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
	ssize_t ret;
	struct sockaddr_in from_addr;
	struct pcap_pkthdr *pkt_header;
	const u_char *pkt_data;
	socklen_t from_addr_len = sizeof(from_addr);

	if (netPath->pcapGeneral == NULL) {
		ret=recvfrom(netPath->generalSock, buf, PACKET_SIZE, MSG_DONTWAIT, &from_addr, &from_addr_len);
		netPath->lastRecvAddr = from_addr.sin_addr.s_addr;
		return ret;
	} else { /* Using PCAP */
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
	if( ntohs(*(u_short *)(pkt_data + 12)) != ETHERTYPE_IP)
		DBGV("PCAP payload received is not Ethernet: 0x%04x\n",
			ntohs(*(u_short *)(pkt_data + 12)));
	/* Retrieve source IP from the payload - 14 eth + 12 IP src*/
	netPath->lastRecvAddr = *(Integer32 *)(pkt_data + 26);

		netPath->receivedPackets++;
		/* XXX Total cheat */
		memcpy(buf, pkt_data + netPath->headerOffset, 
		       pkt_header->caplen - netPath->headerOffset);
		fflush(NULL);
		ret = pkt_header->caplen - netPath->headerOffset;
	}

	return ret;
}


ssize_t
netSendPcapEther(Octet * buf,  UInteger16 length,
			struct ether_addr * dst, struct ether_addr * src,
			pcap_t * pcap) {
	Octet ether[ETHER_HDR_LEN + PACKET_SIZE];
	memcpy(ether, dst->octet, ETHER_ADDR_LEN);
	memcpy(ether + ETHER_ADDR_LEN, src->octet, ETHER_ADDR_LEN);
	*((short *)&ether[2 * ETHER_ADDR_LEN]) = htons(PTP_ETHER_TYPE);
	memcpy(ether + ETHER_HDR_LEN, buf, length);

	return pcap_inject(pcap, ether, ETHER_HDR_LEN + length);
}


//
// alt_dst: alternative destination.
//   if filled, send to this unicast dest;
//   if zero, do the normal operation (send to unicast with -u, or send to the multcast group)
//
///
/// TODO: merge these 2 functions into one
///
ssize_t 
netSendEvent(Octet * buf, UInteger16 length, NetPath * netPath,
	     RunTimeOpts *rtOpts, Integer32 alt_dst, TimeInternal * tim)
{
	ssize_t ret;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_EVENT_PORT);

	/* In PCAP Ethernet mode, we use pcapEvent for receiving all messages 
	 * and pcapGeneral for sending all messages
	 */
	if ((netPath->pcapGeneral != NULL) && (rtOpts->transport == IEEE_802_3 )) {

		ret = netSendPcapEther(buf, length,
			&netPath->etherDest,
			(struct ether_addr *)netPath->port_uuid_field,
			netPath->pcapGeneral);
		
		if (ret <= 0) 
			DBG("Error sending ether multicast event message\n");
		else
			netPath->sentPackets++;

	} else {
		if (netPath->unicastAddr || alt_dst ) {
			if (netPath->unicastAddr) {
				addr.sin_addr.s_addr = netPath->unicastAddr;
			} else {
				addr.sin_addr.s_addr = alt_dst;
			}

			ret = sendto(netPath->eventSock, buf, length, 0, 
				     (struct sockaddr *)&addr, 
				     sizeof(struct sockaddr_in));
			if (ret <= 0)
				DBG("Error sending unicast event message\n");
			else
				netPath->sentPackets++;
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
				DBG("Error looping back unicast event message\n");
#else
			if(!netPath->txTimestampFailure) {
				if(!getTxTimestamp(netPath, tim)) {
					netPath->txTimestampFailure = TRUE;
					if (tim) {
						clearTime(tim);
					}
				}
			}

			if(netPath->txTimestampFailure)
			{
				/* We've had a TX timestamp receipt timeout - falling back to packet looping */
				addr.sin_addr.s_addr = netPath->interfaceAddr.s_addr;
				ret = sendto(netPath->eventSock, buf, length, 0, 
				     (struct sockaddr *)&addr, 
				     sizeof(struct sockaddr_in));
				if (ret <= 0)
					DBG("Error looping back unicast event message\n");
			}
#endif /* SO_TIMESTAMPING */		
		} else {
			addr.sin_addr.s_addr = netPath->multicastAddr;
                        /* Is TTL OK? */
			if(netPath->ttlEvent != rtOpts->ttl) {
				/* Try restoring TTL */
				if(setsockopt(netPath->eventSock, IPPROTO_IP, IP_MULTICAST_TTL,
					&rtOpts->ttl, sizeof(int)) >= 0) {
				    netPath->ttlEvent = rtOpts->ttl;
				}
            		}
			ret = sendto(netPath->eventSock, buf, length, 0, 
				     (struct sockaddr *)&addr, 
				     sizeof(struct sockaddr_in));
			if (ret <= 0)
				DBG("Error sending multicast event message\n");
			else
				netPath->sentPackets++;
#ifdef SO_TIMESTAMPING
			if(!netPath->txTimestampFailure) {
				if(!getTxTimestamp(netPath, tim)) {
					if (tim) {
						clearTime(tim);
					}
					
					netPath->txTimestampFailure = TRUE;

					/* Try re-enabling MULTICAST_LOOP */
					netSetMulticastLoopback(netPath, TRUE);
				}
			}
#endif /* SO_TIMESTAMPING */
		}
	}
	return ret;
}

ssize_t 
netSendGeneral(Octet * buf, UInteger16 length, NetPath * netPath,
	       RunTimeOpts *rtOpts, Integer32 alt_dst)
{
	ssize_t ret;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_GENERAL_PORT);

	if ((netPath->pcapGeneral != NULL) && (rtOpts->transport == IEEE_802_3)) {
		ret = netSendPcapEther(buf, length,
			&netPath->etherDest,
			(struct ether_addr *)netPath->port_uuid_field,
			netPath->pcapGeneral);

		if (ret <= 0) 
			DBG("Error sending ether multicast general message\n");
		else
			netPath->sentPackets++;

	} else {
		if(netPath->unicastAddr || alt_dst ){
			if (netPath->unicastAddr) {
				addr.sin_addr.s_addr = netPath->unicastAddr;
			} else {
				addr.sin_addr.s_addr = alt_dst;
			}


			ret = sendto(netPath->generalSock, buf, length, 0, 
				     (struct sockaddr *)&addr, 
				     sizeof(struct sockaddr_in));
			if (ret <= 0)
				DBG("Error sending unicast general message\n");
			else
				netPath->sentPackets++;
		} else {
			addr.sin_addr.s_addr = netPath->multicastAddr;

                        /* Is TTL OK? */
			if(netPath->ttlGeneral != rtOpts->ttl) {
				/* Try restoring TTL */
				if(setsockopt(netPath->generalSock, IPPROTO_IP, IP_MULTICAST_TTL,
					&rtOpts->ttl, sizeof(int)) >= 0) {

				    netPath->ttlGeneral = rtOpts->ttl;

				}
            		}

			ret = sendto(netPath->generalSock, buf, length, 0, 
				     (struct sockaddr *)&addr, 
				     sizeof(struct sockaddr_in));
			if (ret <= 0)
				DBG("Error sending multicast general message\n");
			else
				netPath->sentPackets++;
		}
	}
	return ret;
}

ssize_t 
netSendPeerGeneral(Octet * buf, UInteger16 length, NetPath * netPath, RunTimeOpts *rtOpts)
{

	ssize_t ret;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_GENERAL_PORT);

	if ((netPath->pcapGeneral != NULL) && (rtOpts->transport == IEEE_802_3)) {
		ret = netSendPcapEther(buf, length,
			&netPath->peerEtherDest,
			(struct ether_addr *)netPath->port_uuid_field,
			netPath->pcapGeneral);

		if (ret <= 0) 
			DBG("error sending ether multi-cast general message\n");
	} else if (netPath->unicastAddr) {
		addr.sin_addr.s_addr = netPath->unicastAddr;

		ret = sendto(netPath->generalSock, buf, length, 0, 
			     (struct sockaddr *)&addr, 
			     sizeof(struct sockaddr_in));
		if (ret <= 0)
			DBG("Error sending unicast peer general message\n");

	} else {
		addr.sin_addr.s_addr = netPath->peerMulticastAddr;
		
		/* is TTL already 1 ? */
		if(netPath->ttlGeneral != 1) {
			int ttl = 1;
			/* Try setting TTL to 1 */
			if(setsockopt(netPath->generalSock, IPPROTO_IP, IP_MULTICAST_TTL,
				&ttl, sizeof(int)) >= 0) {
				
				netPath->ttlGeneral = 1;

			}
                }
		ret = sendto(netPath->generalSock, buf, length, 0, 
			     (struct sockaddr *)&addr, 
			     sizeof(struct sockaddr_in));
		if (ret <= 0)
			DBG("Error sending multicast peer general message\n");
	}

	if (ret > 0)
		netPath->sentPackets++;
	
	return ret;

}

ssize_t 
netSendPeerEvent(Octet * buf, UInteger16 length, NetPath * netPath, RunTimeOpts *rtOpts, TimeInternal * tim)
{
	ssize_t ret;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_EVENT_PORT);

	if ((netPath->pcapGeneral != NULL) && (rtOpts->transport == IEEE_802_3)) {
		ret = netSendPcapEther(buf, length,
			&netPath->peerEtherDest,
			(struct ether_addr *)netPath->port_uuid_field,
			netPath->pcapGeneral);

		if (ret <= 0) 
			DBG("error sending ether multi-cast general message\n");
	} else if (netPath->unicastAddr) {
		addr.sin_addr.s_addr = netPath->unicastAddr;

		ret = sendto(netPath->eventSock, buf, length, 0, 
			     (struct sockaddr *)&addr, 
			     sizeof(struct sockaddr_in));
		if (ret <= 0)
			DBG("Error sending unicast peer event message\n");
		else
			netPath->sentPackets++;

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
		if(!netPath->txTimestampFailure) {
			if(!getTxTimestamp(netPath, tim)) {
				netPath->txTimestampFailure = TRUE;
				if (tim) {
					clearTime(tim);
				}
			}
		}

		if(netPath->txTimestampFailure) {
			/* We've had a TX timestamp receipt timeout - falling back to packet looping */
			addr.sin_addr.s_addr = netPath->interfaceAddr.s_addr;
			ret = sendto(netPath->eventSock, buf, length, 0, 
				     (struct sockaddr *)&addr, 
				     sizeof(struct sockaddr_in));
			if (ret <= 0)
				DBG("Error looping back unicast event message\n");
		}
#endif /* SO_TIMESTAMPING */

	} else {
		addr.sin_addr.s_addr = netPath->peerMulticastAddr;

		/* is TTL already 1 ? */
		if(netPath->ttlEvent != 1) {
			int ttl = 1;
			/* Try setting TTL to 1 */
			if(setsockopt(netPath->eventSock, IPPROTO_IP, IP_MULTICAST_TTL,
				&ttl, sizeof(int)) >= 0) {

			    netPath->ttlEvent = 1;

			}
                }
		ret = sendto(netPath->eventSock, buf, length, 0, 
			     (struct sockaddr *)&addr, 
			     sizeof(struct sockaddr_in));
		if (ret <= 0)
			DBG("Error sending multicast peer event message\n");
		else
			netPath->sentPackets++;
#ifdef SO_TIMESTAMPING
		if(!netPath->txTimestampFailure) {
			if(!getTxTimestamp(netPath, tim)) {
				if (tim) {
					clearTime(tim);
				}
					
				netPath->txTimestampFailure = TRUE;

				/* Try re-enabling MULTICAST_LOOP */
				netSetMulticastLoopback(netPath, TRUE);
			}
		}
#endif /* SO_TIMESTAMPING */
	}

	if (ret > 0)
		netPath->sentPackets++;

	return ret;
}



/*
 * refresh IGMP on a timeout
 */
/*
 * @return TRUE if successful
 */
Boolean
netRefreshIGMP(NetPath * netPath, RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	DBG("netRefreshIGMP\n");
	
	netShutdownMulticast(netPath);
	
	/* suspend process 100 milliseconds, to make sure the kernel sends the IGMP_leave properly */
	usleep(100*1000);

	if (!netInitMulticast(netPath, rtOpts)) {
		return FALSE;
	}
	return TRUE;
}
