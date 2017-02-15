/* Copyright (c) 2016-2017 Wojciech Owczarek,
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
 * @file   net_utils.c
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  A cross-platform collection of network utility functions
 *
 */

#include <config.h>

#include <libcck/cck.h>

#include <sys/types.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#include <ifaddrs.h>

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif /* HAVE_NET_IF_DL_H */

#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif /* HAVE_NET_IF_TYPES_H */

#include <libcck/net_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/cck_utils.h>
#include <libcck/transport_address.h>

#define THIS_COMPONENT "netUtils."

/* Try getting hwAddrSize bytes of ifName hardware address,
   and place them in hwAddr. Return 1 on success, 0 when no suitable
   hw address available, -1 on failure.
 */
int
getHwAddrData (unsigned char* hwAddr, const char* ifName, const int hwAddrSize)
{

    int ret;
    if(!strlen(ifName))
	return 0;

/* BSD* - AF_LINK gives us access to the hw address via struct sockaddr_dl */
#if defined(AF_LINK) && !defined(__sun)

    struct ifaddrs *ifaddr, *ifa;

    if(getifaddrs(&ifaddr) == -1) {
	CCK_PERROR("Could not get interface list");
	ret = -1;
	goto end;

    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
	if(ifa->ifa_name == NULL) continue;
	if(ifa->ifa_addr == NULL) continue;
	if(!strcmp(ifName, ifa->ifa_name) && ifa->ifa_addr->sa_family == AF_LINK) {

		struct sockaddr_dl* sdl = (struct sockaddr_dl *)ifa->ifa_addr;
		if(sdl->sdl_type == IFT_ETHER || sdl->sdl_type == IFT_L2VLAN) {

			memcpy(hwAddr, LLADDR(sdl),
			hwAddrSize <= sizeof(sdl->sdl_data) ?
			hwAddrSize : sizeof(sdl->sdl_data));
			ret = 1;
			goto end;
		} else {
			CCK_DBGV("Unsupported hardware address family on %s\n", ifName);
			ret = 0;
			goto end;
		}
	}

    }

    ret = 0;
    CCK_DBG("Interface not found: %s\n", ifName);

end:

    freeifaddrs(ifaddr);
    return ret;


#else
/* Linux and Solaris family which also have SIOCGIFHWADDR/SIOCGLIFHWADDR */
    int sockfd;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
	CCK_PERROR("Could not open test socket");
	return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, ifName, IF_NAMESIZE);

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
            CCK_DBGV("failed to request hardware address for %s", ifName);
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
	    CCK_DBGV("Unsupported hardware address family on %s\n", ifName);
	    ret = 0;
	}
end:
    close(sockfd);
    return ret;

#endif /* AF_LINK */

}

/* get interface address of given family, return -1 on failure, 0 if not found or 1 if found. Wanted is used when we want a specific address */
int
getIfAddr(CckTransportAddress *addr, const char *ifName, const int family, const CckTransportAddress *wanted)
{

	int ret = 0;

	if(family == TT_FAMILY_ETHERNET) {
		ret = getHwAddrData(addr->addr.ether.ether_addr_octet, ifName, TT_ADDRLEN_ETHERNET);
		if (ret != 1) {
		    return ret;
		}
		addr->family = TT_FAMILY_ETHERNET;
		addr->populated = true;
	} else 	if( family == TT_FAMILY_IPV4 || family == TT_FAMILY_IPV6 ) {

		CckAddressToolset *ts = getAddressToolset(family);

		struct ifaddrs *ifaddr, *ifa;

		if(getifaddrs(&ifaddr) == -1) {
		    CCK_PERROR(THIS_COMPONENT"getIfAddr(): Could not get interface list");
		    return -1;
		}

		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
			if(ifa->ifa_name == NULL) continue;
			/* ifa_addr not always present - link layer may come first */
			if(ifa->ifa_addr == NULL) continue;
				/* interface found */
				if(!strcmp(ifName, ifa->ifa_name) && ifa->ifa_addr->sa_family == ts->afType) {
				    if(wanted && wanted->populated) {
					CckTransportAddress src;
					memset(&src, 0, sizeof(CckTransportAddress));
					src.family = family;
					memcpy(&src.addr.inet, ifa->ifa_addr, ts->structSize);
					/* specific address found */
					if(!ts->compare(&src, wanted)) {
					    memcpy(&addr->addr.inet, &src.addr.inet, ts->structSize);
					    addr->populated = true;
					    addr->family = family;
					    ret = 1;
					    break;
					}
				    } else {
					addr->populated = true;
					addr->family = family;
					memcpy(&addr->addr.inet, ifa->ifa_addr, ts->structSize);
					ret = 1;
					break;
				    }
				}
		}

		if(!ret) {
		    CCK_DBG(THIS_COMPONENT"getIfAddr(): Interface not found: %s\n", ifName);
		}
		freeifaddrs(ifaddr);
	}
#ifdef CCK_DEBUG
    if(addr->populated) {
	CckAddressToolset *ts = getAddressToolset(addr->family);
	tmpstr(strAddr, ts->strLen);
	CCK_DBG(THIS_COMPONENT"getIfAddr: got %s address %s of interface %s\n",
		getAddressFamilyName(addr->family), ts->toString(strAddr, strAddr_len, addr), ifName);
    }
#endif
    return ret;
}

/* join or leave a multicast address */
bool
joinMulticast_ipv4(int fd, const CckTransportAddress *addr, const char *ifName, const CckTransportAddress *ownAddr, const bool join)
{

    int op = join ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
    struct ip_mreq imr;

    tmpstr(addrStr, TT_STRADDRLEN_IPV4);
    transportAddressToString(addrStr, addrStr_len, addr);

    if(!isAddressMulticast_ipv4(addr)) {
	CCK_DBG("joinMulticast_ipv4(%s): not a multicast address, will not send %s\n", addrStr, join ?  "join" : "leave");
	return false;
    }

    imr.imr_multiaddr.s_addr = addr->addr.inet4.sin_addr.s_addr;
    imr.imr_interface.s_addr = ownAddr->addr.inet4.sin_addr.s_addr;

    if (join) {
	    /* set multicast outbound interface */
	    if(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF,
		&imr.imr_interface, sizeof(struct in_addr)) < 0) {
		CCK_DBG(THIS_COMPONENT"failed to set multicast outbound interface on %s: (%s)\n", ifName, strerror(errno));
		return false;
		}
	/* drop multicast group on specified interface first, to actually re-send a join */
	(void)setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
           &imr, sizeof(imr));
    }

    /* join or leave multicast group on specified interface */
    if (setsockopt(fd, IPPROTO_IP, op,
           &imr, sizeof(imr)) < 0 ) {
	    if(join) {
		CCK_PERROR(THIS_COMPONENT"failed to %s multicast group %s on %s: ", join ? "join" : "leave", addrStr, ifName);
		    return false;
	    }
    }

    CCK_DBGV("%s multicast group %s on %s\n", join ? "Joined" : "Left", addrStr,
						    ifName);

    return true;

}

bool
joinMulticast_ipv6(int fd, const CckTransportAddress *addr, const char *ifName, const CckTransportAddress *ownAddr, const bool join)
{

    int op = join ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP;
    struct ipv6_mreq imr;

    tmpstr(addrStr, TT_STRADDRLEN_IPV6);
    transportAddressToString(addrStr, addrStr_len, addr);

    if(!isAddressMulticast_ipv6(addr)) {
	CCK_DBG("joinMulticast_ipv6(%s): not a multicast address, will not send %s\n", addrStr, join ?  "join" : "leave");
	return false;
    }

    memcpy(imr.ipv6mr_multiaddr.s6_addr, addr->addr.inet6.sin6_addr.s6_addr, TT_ADDRLEN_IPV6);
    imr.ipv6mr_interface = getInterfaceIndex(ifName);

    if (join) {
	/* set multicast outbound interface */
	if(setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		&imr.ipv6mr_interface, sizeof(int)) < 0) {
		CCK_DBG(THIS_COMPONENT"failed to set multicast outbound interface on %s: (%s)\n", ifName, strerror(errno));
		return false;
		}
	/* drop multicast group on specified interface first, to actually re-send a join */
	(void)setsockopt(fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
           &imr, sizeof(struct ipv6_mreq));
    }

    /* join or leave multicast group on specified interface */
    if (setsockopt(fd, IPPROTO_IPV6, op,
           &imr, sizeof(struct ipv6_mreq)) < 0) {
	    if (join) {
		CCK_PERROR(THIS_COMPONENT"failed to %s multicast group %s on %s: ", join ? "join" : "leave", addrStr, ifName);
		    return false;
	    }
    }

    CCK_DBGV(THIS_COMPONENT"%s multicast group %s on %s\n", join ? "Joined" : "Left", addrStr,
						    ifName);

    return true;

}

bool
joinMulticast_ethernet(const CckTransportAddress *addr, const char *ifName, const bool join)
{
    struct ifreq ifr;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    int op = join ? SIOCADDMULTI : SIOCDELMULTI;

    tmpstr(addrStr, TT_STRADDRLEN_ETHERNET);
    transportAddressToString(addrStr, addrStr_len, addr);

    if(!isAddressMulticast_ethernet(addr)) {
	CCK_DBG("joinMulticast_ethernet(%s): not a multicast address, will not send %s\n", addrStr, join ?  "join" : "leave");
	return false;
    }

    if( fd == -1) {
	CCK_DBG("failed to open helper socket when adding ethernet multicast membership %s on %s: %s\n",
		addrStr, ifName, strerror(errno));
        return false;
    }

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, ifName, strlen(ifName));
#ifdef HAVE_STRUCT_IFREQ_IFR_HWADDR
    ifr.ifr_hwaddr.sa_family = AF_UNSPEC;
    memcpy(&ifr.ifr_hwaddr.sa_data, addr->addr.ether.ether_addr_octet, TT_ADDRLEN_ETHERNET);
#else
    ifr.ifr_addr.sa_family = AF_UNSPEC;
    memcpy(&ifr.ifr_addr.sa_data, addr->addr.ether.ether_addr_octet, TT_ADDRLEN_ETHERNET);
#endif

    /* join multicast group on specified interface */
    if ((ioctl(fd, op, &ifr)) < 0 ) {
	if(join) {
	    CCK_ERROR(THIS_COMPONENT"Failed to %s ethernet multicast membership %s on %s: %s\n",
		join ? "add" : "delete", addrStr, ifName, strerror(errno));
	}
	close(fd);
        return false;
    }

    CCK_DBGV("%s ethernet membership %s on %s\n", join ? "Added" : "Deleted",
						addrStr, ifName);
    close(fd);

    return true;

}

bool
setMulticastLoopback(int fd,  const int family, const bool _value)
{

#if defined(__OpenBSD__) || defined(__sun) || defined(__NetBSD__)
	u_char value;
#else
	int value;
#endif

	value = _value ? 1 : 0;

	int proto, option;

	if(family == TT_FAMILY_IPV4) {
	    proto = IPPROTO_IP;
	    option = IP_MULTICAST_LOOP;
	} else if (family == TT_FAMILY_IPV6) {
	    proto = IPPROTO_IPV6;
	    option = IPV6_MULTICAST_LOOP;
	} else {
	    CCK_DBG("setMulticastLoopback(): unsupported address family %02x, ignoring request\n", family);
	    return true;
	}

	if (setsockopt(fd, proto, option,
	       &value, sizeof(value)) < 0) {
		CCK_PERROR(THIS_COMPONENT"Failed to %s IPv%s multicast loopback",
			    value ? "enable":"disable",
			    (family == TT_FAMILY_IPV4) ? "4" : "6");
		return false;
	}

	CCK_DBG("Successfully %s IPv%s multicast loopback \n", value ? "enabled":"disabled",
		    (family == TT_FAMILY_IPV4) ? "4" : "6");

	return true;
}

bool
setMulticastTtl(int fd,  const int family, const int _value)
{

#if defined(__OpenBSD__) || defined(__sun) || defined(__NetBSD__)
	u_char value;
#else
	int value;
#endif

	value = _value;

	int proto, option;

	if(family == TT_FAMILY_IPV4) {
	    proto = IPPROTO_IP;
	    option = IP_MULTICAST_TTL;
	} else if (family == TT_FAMILY_IPV6) {
	    proto = IPPROTO_IPV6;
	    option = IPV6_MULTICAST_HOPS;
	} else {
	    CCK_DBG("setMulticastTtl(): unsupported address family %02x, ignoring request\n", family);
	    return true;
	}

	if (setsockopt(fd, proto, option,
	       &value, sizeof(value)) < 0) {
		CCK_PERROR(THIS_COMPONENT"setMulticastTtl(): Failed to set IPv%s to %d",
			    (family == TT_FAMILY_IPV4) ? "4 multicast TTL" : "6 multicast hops",
			    value);
		return false;
	}

	CCK_DBG("setMulticastTtl(): Successfully set IPv%s to %d\n",
		    (family == TT_FAMILY_IPV4) ? "4 multicast TTL" : "6 multicast hops",
		    value);

	return true;
}

bool
setSocketDscp(int fd,  const int family, const int _value)
{

	uint8_t value = _value << 2;

	int proto, option;

	if(family == TT_FAMILY_IPV4) {
	    proto = IPPROTO_IP;
	    option = IP_TOS;
	} else if (family == TT_FAMILY_IPV6) {
	    proto = IPPROTO_IPV6;
	    option = IPV6_TCLASS;
	} else {
	    CCK_DBG("setSocketDscp(): unsupported address family %02x, ignoring request\n", family);
	    return true;
	}

	if (setsockopt(fd, proto, option,
	       &value, sizeof(value)) < 0) {
		CCK_PERROR(THIS_COMPONENT"setSocketDscp(): Failed to set IPv%s to %d",
			    (family == TT_FAMILY_IPV4) ? "4 DSCP value" : "DSCP value",
			    _value);
		return false;
	}

	CCK_DBG("setSocketDscp(): Successfully set IPv%s to %d\n",
		    (family == TT_FAMILY_IPV4) ? "4 DSCP value" : "6 DSCP value",
		    _value);

	return true;
}


/* pre-populate control message header */
void
prepareMsgHdr(struct msghdr *msg, struct iovec *vec, char *dataBuf, size_t dataBufSize, char *controlBuf, size_t controlLen, struct sockaddr *fromAddr, int fromAddrLen)
{

    memset(vec, 0, sizeof(struct iovec));
    memset(msg, 0, sizeof(struct msghdr));
    memset(dataBuf, 0, dataBufSize);
    memset(controlBuf, 0, controlLen);

    vec->iov_base = dataBuf;
    vec->iov_len = dataBufSize;

    msg->msg_name = (caddr_t)fromAddr;
    msg->msg_namelen = fromAddrLen;
    msg->msg_iov = vec;
    msg->msg_iovlen = 1;
    msg->msg_control = controlBuf;
    msg->msg_controllen = controlLen;
    msg->msg_flags = 0;

}

/* get control message data of minimum size len from control message header */
int
getCmsgData(void **data, struct cmsghdr *cmsg, const int level, const int type, const size_t len)
{

    /* initial checks */
    if(cmsg->cmsg_level != level) {
	return 0;
    }
    if(cmsg->cmsg_type != type) {
	return 0;
    }
    if(cmsg->cmsg_len < len) {
	return -1;
    }

    *data = CMSG_DATA(cmsg);
    return 1;
}

/* get control message data of n-th array member of size elemSize from control message header */
int
getCmsgItem(void **data, struct cmsghdr *cmsg, const int level, const int type, const size_t elemSize, const int arrayIndex)
{

    int ret = getCmsgData(data, cmsg, level, type, elemSize * (arrayIndex + 1));

    if(ret > 0) {
	*data = *data + elemSize * arrayIndex;
    }

    return ret;

}

/* get timestamp from control message header based on timestamp type described by tstampConfig. Return 1 if found and non-zero, 0 if not found or zero, -1 if data too short */
int
getCmsgTimestamp(CckTimestamp *timestamp, struct cmsghdr *cmsg, const TTsocketTimestampConfig *config)
{

    void *data = NULL;

    int ret = getCmsgItem(&data, cmsg, config->cmsgLevel, config->cmsgType, config->elemSize, config->arrayIndex);

    if (ret <= 0) {
	return ret;
    }

    config->convertTs(timestamp, data);

    if((timestamp->seconds) == 0 && (timestamp->nanoseconds == 0)) {
	return 0;
    }

    return 1;

}

/* get timestamp from a timestamp struct of the given type */

void
convertTimestamp_timespec(CckTimestamp *timestamp, const void *data)
{
    const struct timespec *ts = data;
    timestamp->seconds = ts->tv_sec;
    timestamp->nanoseconds = ts->tv_nsec;
}

void
convertTimestamp_timeval(CckTimestamp *timestamp, const void *data)
{
    const struct timeval *tv = data;
    timestamp->seconds = tv->tv_sec;
    timestamp->nanoseconds = tv->tv_usec * 1000;
}


#ifdef SO_BINTIME
void
convertTimestamp_bintime(CckTimestamp *timestamp, const void *data)
{
    const struct bintime *bt = data;
    struct timespec ts;

    bintime2timespec(bt, &ts);
    timestamp->seconds = ts.tv_sec;
    timestamp->nanoseconds = ts.tv_nsec;
}
#endif

bool ioctlHelper(struct ifreq *ifr, const char* ifName, unsigned long request) {

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
	CCK_PERROR("ioctlHelper(%s): could not create helper socket", ifName);
	return false;
    }

    strncpy(ifr->ifr_name, ifName, IFNAMSIZ);

    if (ioctl(sockfd, request, ifr) < 0) {
            CCK_DBG("ioctlHelper: failed to call ioctl 0x%x on %s: %s\n", request, ifName, strerror(errno));
	    close(sockfd);
	    return false;
    }
    close(sockfd);
    return true;

}

/* Check if interface ifName exists. Return 1 on success, 0 when interface doesn't exists, -1 on failure.
 */

int
interfaceExists(const char* ifName)
{

    int ret;
    struct ifaddrs *ifaddr, *ifa;

    if(!strlen(ifName)) {
	CCK_DBG(THIS_COMPONENT"interfaceExists(): Called for an empty interface name\n");
	return 0;
    }

    if(getifaddrs(&ifaddr) == -1) {
	CCK_PERROR(THIS_COMPONENT"interfaceExists(): Could not get interface list");
	ret = -1;
	goto end;

    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
	if(ifa->ifa_name == NULL) continue;
	if(!strcmp(ifName, ifa->ifa_name)) {
	    ret = 1;
	    goto end;
	}

    }

    ret = 0;
    CCK_DBG("Interface not found: %s\n", ifName);

end:
   freeifaddrs(ifaddr);
    return ret;
}

int getInterfaceIndex(const char *ifName)
{

#ifdef HAVE_IF_NAMETOINDEX

return if_nametoindex(ifName);

#else /* no if_nametoindex() */

#ifndef SIOCGIFINDEX

    return -1;

#else
    int sockfd;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
	PERROR("Could not retrieve interface index for %s",ifName);
	return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, ifName, IFACE_NAME_LENGTH);

    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
            DBGV("failed to request hardware address for %s", ifName);
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

#endif /* !HAVE_IF_NAMETOINDEX */

}

int
getInterfaceFlags(const char *ifName, int *flags)
{

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    if (ioctlHelper(&ifr, ifName, SIOCGIFFLAGS)) {
	CCK_DBG(THIS_COMPONENT"getInterfaceFlags(%s): success: %d\n", ifName, ifr.ifr_flags);
	*flags = ifr.ifr_flags;
	return 1;
    }

    CCK_DBG(THIS_COMPONENT"getInterfaceFlags(%s): could not get interface flags\n", ifName);
    return -1;

}

int
setInterfaceFlags(const char *ifName, const int flags)
{

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    int intFlags;

    if (getInterfaceFlags(ifName, &intFlags) < 0) {
	return -1;
    }

    ifr.ifr_flags = flags | intFlags;

    if(ioctlHelper(&ifr, ifName, SIOCGIFFLAGS)) {
	CCK_DBG(THIS_COMPONENT"setInterfaceFlags(%s:%04x): success\n", ifName, ifr.ifr_flags);
	return 1;
    }

    CCK_DBG(THIS_COMPONENT"setInterfaceFlags(%s:%04x): could not set interface flags\n", ifName, flags);

    return -1;
}

int
clearInterfaceFlags(const char *ifName, const int flags)
{

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    int intFlags;

    if (getInterfaceFlags(ifName, &intFlags) < 0) {
	return -1;
    }

    ifr.ifr_flags = ~flags & intFlags;

    if(ioctlHelper(&ifr, ifName, SIOCGIFFLAGS)) {
	CCK_DBG(THIS_COMPONENT"clearInterfaceFlags(%s:%04x): success\n", ifName, ifr.ifr_flags);
	return 1;
    }

    CCK_DBG(THIS_COMPONENT"clearInterfaceFlags(%s:%04x): could not clear interface flags\n", ifName, flags);

    return -1;
}

/* get interface information */
bool
getInterfaceInfo(CckInterfaceInfo *info, const char* ifName, const int family, const CckTransportAddress *sourceHint, const bool quiet)
{

    CckAddressToolset *ts = getAddressToolset(family);

    if(!ts) {
	return false;
    }

    memset(info, 0, sizeof(CckInterfaceInfo));

    info->valid = true;
    info->family = family;

    if(!strlen(ifName)) {
	CCK_QERROR(THIS_COMPONENT"getInferfaceInfo(): no interface name given\n");
	return false;
    }

    info->found = interfaceExists(ifName);

    if(!info->found) {
	CCK_QERROR(THIS_COMPONENT"getInferfaceInfo(%s): interface not found\n", ifName);
	return false;
    }

    strncpy(info->ifName, ifName, IFNAMSIZ);

    /* get h/w address */
    info->hwStatus = getIfAddr(&info->hwAddress, ifName, TT_FAMILY_ETHERNET, NULL);

    if(info->hwStatus == -1) {
	CCK_QERROR(THIS_COMPONENT"getInterfaceInfo(%s): could not get hardware address\n",
	ifName);
    }

    if(family == TT_FAMILY_ETHERNET && info->hwStatus != 1) {
	return false;
    }

    /* try getting protocol address */
    info->afStatus = getIfAddr(&info->afAddress, ifName, family, sourceHint);

    /* failure */
    if(info->afStatus == -1) {
	CCK_QERROR(THIS_COMPONENT"getInferfaceInfo(%s): could not get %s address\n",
	ifName, getAddressFamilyName(family));
	return false;
    }

    info->sourceFound = true;

    if(info->afStatus == 0) {
	/* no requested address found */
	if(sourceHint && sourceHint->populated) {
	    tmpstr(strAddr, ts->strLen);
	    CCK_QERROR(THIS_COMPONENT"getInterfaceInfo(%s): requested %s address \"%s\" not found\n",
		ifName, getAddressFamilyName(family),
		ts->toString(strAddr, strAddr_len, sourceHint));
	    info->sourceFound = false;
	/* no address found */
	} else {
	    CCK_QERROR(THIS_COMPONENT"getInferfaceInfo(%s): interface has no %s address\n",
	    ifName, getAddressFamilyName(family), ifName);

	}
	return false;
    }


    if(getInterfaceFlags(ifName, &info->flags) < 0) {
	CCK_QWARNING(THIS_COMPONENT"getInferfaceInfo(%s): could not query interface flags\n",
	ifName);
    }

    info->index = getInterfaceIndex(ifName);
    if(info->index < 0) {
	CCK_DBG(THIS_COMPONENT"getInferfaceInfo(%s): could not get interface index\n",
	ifName);
    }

    return true;

}

bool
testInterface(const char* ifName, const int family, const char* sourceHint)
{
    bool ret = true;
    CckInterfaceInfo info;
    CckTransportAddress *src = NULL;

    if(sourceHint && strlen(sourceHint)) {

	src = createAddressFromString(family, sourceHint);
	if(src == NULL) {
	    CCK_ERROR(THIS_COMPONENT"testInterface(%s): invalid source address specified: %s\n",
			ifName, sourceHint);
	    ret = false;
	}

    }

    ret &= getInterfaceInfo(&info, ifName, family, src, false);

    if(src) {
	freeCckTransportAddress(&src);
    }

    return ret;

}

/*
 *  status in the @last structure is only one of OK, DOWN, FAULT, but returned value
 *  provides event status (went up, went down, fault, fault cleared, major change, or no change)
 */

int
monitorInterface(CckInterfaceInfo *last, const CckTransportAddress *sourceHint, const bool quiet)
{

    CckInterfaceInfo current;

    int ret = 0;

    if(!last) {
	return -1;
    }

    /* no point monitoring without previous data */
    if(!last->valid) {
	return 0;
    }

    CckAddressToolset *ts = getAddressToolset(last->family);

    /* interface seems OK - check for changes and previous faults */
    if(getInterfaceInfo(&current, last->ifName, last->family, sourceHint, quiet)) {

	/* if not else, or else */
	current.status = CCK_INTINFO_OK;

	/* interface down */
	if(!(current.flags & IFF_RUNNING)) {
	    current.status = CCK_INTINFO_DOWN;
	    if((last->status != CCK_INTINFO_DOWN)) {
		ret |= CCK_INTINFO_DOWN;
	    }
	    goto gameover;
	}

	/* interface returns to good state */
	switch (last->status) {
	    case CCK_INTINFO_FAULT: /* check for fault clear before down/up */
		ret |= CCK_INTINFO_CLEAR;
		goto gameover;
	    case CCK_INTINFO_DOWN:
		ret |= CCK_INTINFO_UP;
		goto gameover;
	}

	/* check for address changes */
	if(ts->compare(&last->hwAddress, &current.hwAddress)) {
	    ret |= CCK_INTINFO_CHANGE;
	    goto gameover;
	}
	if(ts->compare(&last->afAddress, &current.afAddress)) {
	    ret |= CCK_INTINFO_CHANGE;
	    goto gameover;
	}

	ret |= CCK_INTINFO_OK;

    /* interface not OK */
    } else {
	current.status = CCK_INTINFO_FAULT;
	ret |= CCK_INTINFO_FAULT;
    }

gameover:

	if(current.status == last->status) {
	    ret |= CCK_INTINFO_NOCHANGE;
	}

    /* update current info */
    memcpy(last, &current, sizeof(current));

    CCK_DBG(THIS_COMPONENT"monitorInterface('%s'): status %s\n",
		current.ifName, getIntStatusName(current.status));

    return ret;

}

#define CCK_INTINFO_OK		1 << 0		/* A-OK */
#define CCK_INTINFO_DOWN	1 << 1		/* Down (was up) */
#define CCK_INTINFO_FAULT	1 << 2		/* Fault (was OK) */

/* monitor result / events */
#define CCK_INTINFO_NOCHANGE	1 << 3		/* Same status as previously */
#define CCK_INTINFO_CHANGE	1 << 4		/* Change occurred (mostly address change) */
#define CCK_INTINFO_UP		1 << 5		/* Up (was down) */
#define CCK_INTINFO_CLEAR	1 << 6		/* Fault cleared (was fault) */
#define CCK_INTINFO_CLOCKCHANGE 1 << 7		/* minor topology change */

/* get a string representing the statuses above */
const char*
getIntStatusName(int status)
{
    switch(status) {
	case CCK_INTINFO_OK:
	    return "OK";
	case CCK_INTINFO_DOWN:
	    return "DOWN";
	case CCK_INTINFO_FAULT:
	    return "FAULT";
	case CCK_INTINFO_NOCHANGE:
	    return "NOCHANGE";
	case CCK_INTINFO_CHANGE:
	    return "CHANGE";
	case CCK_INTINFO_UP:
	    return "UP";
	case CCK_INTINFO_CLEAR:
	    return "CLEAR";
	case CCK_INTINFO_CLOCKCHANGE:
	    return "CLOCKCHANGE";
	default:
	    return "UNKNOWN";
    }
}

/* get a one-line interface status line */
char*
getIntStatusLine(const CckInterfaceInfo *info, char *buf, const size_t len)
{

    snprintf(buf, len, "%s, status: %s",
	info->ifName, getIntStatusName(info->status));

    return buf;

}
