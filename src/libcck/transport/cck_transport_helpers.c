/*-
 * libCCK - Clock Construction Kit
 *
 * Copyright (c) 2014 Wojciech Owczarek,
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
 * @file   cck_transport_helpers.c
 * 
 * @brief  LibCCK transport helper functions
 *
 */

#include "cck_transport_helpers.h"

CckBool
cckGetHwAddress (const char* ifaceName, TransportAddress* hwAddr)
{

    int hwAddrSize = 6;
    int ret;
    if(!strlen(ifaceName))
	return 0;

/* BSD* - AF_LINK gives us access to the hw address via struct sockaddr_dl */
#ifdef AF_LINK

    struct ifaddrs *ifaddr, *ifa;

    if(getifaddrs(&ifaddr) == -1) {
	CCK_PERROR("Could not get interface list");
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
			CCK_DBG("Unsupported hardware address family on %s\n", ifaceName);
			ret = 0;
			goto end;
		}
	}

    }

    ret = 0;
    CCK_DBG("Interface not found: %s\n", ifaceName);

end:

    freeifaddrs(ifaddr);
    return ret;

/* Linux, basically */
#else

    int sockfd;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
	CCK_PERROR("Could not open test socket");
	return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));

    strncpy(ifr.ifr_name, ifaceName, IFACE_NAME_LENGTH);

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
            CCK_DBG("failed to request hardware address for %s", ifaceName);
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
	    CCK_DBG("Unsupported hardware address family on %s\n", ifaceName);
	    ret = 0;
	}
end:
    close(sockfd);
    return ret;

#endif /* AF_LINK */

}

int
cckGetInterfaceAddress(const CckTransport* transport, TransportAddress* addr, int addressFamily)
{
    int ret;
    struct ifaddrs *ifaddr, *ifa;

    if(getifaddrs(&ifaddr) == -1) {
	CCK_PERROR("Could not get interface list");
	ret = -1;
	goto end;

    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

	if(!strcmp(transport->transportEndpoint, ifa->ifa_name) && ifa->ifa_addr->sa_family == addressFamily) {

		/* If address was specified on input, find that address, otherwise get the first one */
		if(!transportAddressEmpty(addr) &&
		    !transport->addressEqual(addr, (TransportAddress*)ifa->ifa_addr))
			continue;

		/* If we have no output pointer, we won't copy but we'll still return true */
		if(addr!=NULL)
		    memcpy(addr, ifa->ifa_addr, sizeof(struct sockaddr));

    		ret = 1;
    		goto end;

	}

    }

    ret = 0;

    if(!transportAddressEmpty(addr)) {
	CCK_ERROR("Could not find address %s on interface %s\n.",
			transport->addressToString(addr),
			transport->transportEndpoint);
    } else {
	CCK_ERROR("Interface not found: %s\n", transport->transportEndpoint);
    }

end:

    freeifaddrs(ifaddr);
    return ret;
}

/* add fd to set, increase maxFd id needed */
void
cckAddFd(int fd, CckFdSet* set)
{

    if(set == NULL)
	return;

    FD_SET(fd, &set->readFdSet);

    CCK_DBGV("Added fd %d to cckSet\n", fd);

    if(fd > set->maxFd) {
        set->maxFd = fd;
	CCK_DBGV("New maxFd is %d\n", fd);
    }

}

/* remove fd from set, set new maxFd */
void
cckRemoveFd(int fd, CckFdSet* set)
{

    int i = 0;

    if(set == NULL || !FD_ISSET(fd, &set->readFdSet))
	return;

    FD_CLR(fd, &set->readFdSet);

    CCK_DBGV("Removed fd %d from cckFdSet\n", fd);

    for(i = fd; i >= 0; i--) {

	if(FD_ISSET(i, &set->readFdSet)) {

	    set->maxFd = i;
	    CCK_DBGV("New maxFd is %d\n", i);
	    break;

	}

    }


}

int
cckSelect(CckFdSet* set, struct timeval* timeout)
{

    int ret;

    /* copy the fd set into the working fd set */
    memcpy(&set->workReadFdSet, &set->readFdSet, sizeof(fd_set));

    /* run select () */
    ret = select(set->maxFd + 1, &set->workReadFdSet, NULL, NULL, timeout);

    if(ret < 0 && (errno == EAGAIN || errno == EINTR))
	return 0;

    return ret;

}
