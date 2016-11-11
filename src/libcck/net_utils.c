/* Copyright (c) 2016 Wojciech Owczarek,
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

#include "net_utils.h"


/* Try getting hwAddrSize bytes of ifaceName hardware address,
   and place them in hwAddr. Return 1 on success, 0 when no suitable
   hw address available, -1 on failure.
 */
int
getHwAddr (const char* ifaceName, unsigned char* hwAddr, const int hwAddrSize)
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

/* pre-populate control message header */
void
prepareMsgHdr(struct msghdr *msg, struct iovec *vec, CckOctet *dataBuf, size_t dataBufSize, char *controlBuf, size_t controlLen, struct sockaddr *fromAddr, int fromAddrLen)
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
getCmsgData(void *data, struct cmsghdr *cmsg, const int level, const int type, const size_t len)
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

    data = CMSG_DATA(cmsg);
    return 1;
}

/* get control message data of n-th array member of size elemSize from control message header */
int
getCmsgItem(void *data, struct cmsghdr *cmsg, const int level, const int type, const size_t elemSize, const int arrayIndex)
{

    int ret = getCmsgData(data, cmsg, level, type, elemSize * (arrayIndex + 1));

    if(ret > 0) {
	data += elemSize * arrayIndex;
    }

    return ret;

}

/* get timestamp from control message header based on timestamp type described by tstampConfig. Return 1 if found and non-zero, 0 if not found or zero, -1 if data too short */
int
getCmsgTimestamp(CckTimestamp *timestamp, struct cmsghdr *cmsg, const TTSocketTstampConfig *config)
{

    void *data = NULL;

    int ret = getCmsgItem(data, cmsg, config->cmsgLevel, config->cmsgType, config->elemSize, config->arrayIndex);

    if (ret <= 0) {
	return ret;
    }

    config->convertTimestamp(timestamp, data);

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
