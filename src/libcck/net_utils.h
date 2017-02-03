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
 * @file   net_utils.h
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  Definitions for network utility functions
 *
 */

#ifndef CCK_NET_UTILS_H_
#define CCK_NET_UTILS_H_

#include <config.h>

#include <stdio.h> /* -> sttdef.h, size_t */

#ifdef HAVE_SYS_UIO_H
#  include <sys/uio.h> /* iovec, ifreq etc. */
#endif

#include <arpa/inet.h>		/* struct sockaddr and friends, ntoh/hton */
#include <netinet/in.h>		/* struct sockaddr and friends */

#ifdef HAVE_LINUX_IF_H
#include <linux/if.h>		/* struct ifaddr, ifreq, ifconf, ifmap, IF_NAMESIZE etc. */
#elif defined(HAVE_NET_IF_H)
#include <net/if.h>		/* struct ifaddr, ifreq, ifconf, ifmap, IF_NAMESIZE etc. */
#endif /* HAVE_LINUX_IF_H*/

#ifndef IF_NAMESIZE
#define IF_NAMESIZE IFNAMSIZ
#endif /* IF_NAMESIZE */

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif /* HAVE_SYS_SOCKIO_H */

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif /* HAVE_LINUX_SOCKIO_H */

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif /* HAVE_NET_IF_ARP_H */

#include <unistd.h>

#include <libcck/cck_types.h>
#include <libcck/transport_address.h>

void cckVersion();

typedef struct {

    bool naive;
    int cmsgLevel;
    int cmsgType;
    size_t elemSize;
    int arrayIndex;
    void (*convertTs) (CckTimestamp *timestamp, const void *data);

} TTsocketTimestampConfig;

typedef struct {
    CckTransportAddress afAddress;
    CckTransportAddress hwAddress;
    int hwStatus;
    int afStatus;
    bool found;
    bool sourceFound;
    int flags;
    int index;
} CckInterfaceInfo;

#ifdef HAVE_LINUX_IF_H
/* missing from linux/if.h */
extern unsigned int if_nametoindex (const char *__ifname);
#endif /* HAVE_LINUX_IF_H*/

/*
 * Try getting hwAddrSize bytes of ifaceName hardware address,
 * and place them in hwAddr. Return 1 on success, 0 when no suitable
 * hw address available, -1 on failure.
 */
int getHwAddrData(unsigned char* hwAddr, const char* ifaceName, const int hwAddrSize);

/* get interface address of the given family, or check if interface has the desired @wanted address */
int getIfAddr(CckTransportAddress *addr, const char *ifaceName, const int family, const CckTransportAddress *wanted);

/* wrapper for ioctl() to run on an interface */
bool ioctlHelper(struct ifreq *ifr, const char* ifaceName, unsigned long request);

/* check if interface exists */
int interfaceExists(const char* ifaceName);
/* return interface index */
int getInterfaceIndex(const char *ifaceName);
/* set / get interface flags */
int getInterfaceFlags(const char *ifaceName);
int setInterfaceFlags(const char *ifaceName, const int flags);
int clearInterfaceFlags(const char *ifaceName, const int flags);
/* get interface information + test if operational */
bool getInterfaceInfo(CckInterfaceInfo *info, const char* ifaceName, const int family, const CckTransportAddress *sourceHint);
bool testInterface(const char* ifaceName, const int family, const char* sourceHint);

/* join or leave a multicast address */
bool joinMulticast_ipv4(int fd, const CckTransportAddress *addr, const char *ifaceName, const CckTransportAddress *ownAddr, const bool join);
bool joinMulticast_ipv6(int fd, const CckTransportAddress *addr, const char *ifaceName, const CckTransportAddress *ownAddr, const bool join);
bool joinMulticast_ethernet(const CckTransportAddress *addr, const char *ifaceName, const bool join);

/* various multicast operations - inet and inet6 */
bool setMulticastLoopback(int fd,  const int family, const bool _value);
bool setMulticastTtl(int fd,  const int family, const int _value);
bool setSocketDscp(int fd, const int family, const int _value);

/* populate and prepare msghdr (assuming a single message) */
void prepareMsgHdr(struct msghdr *msg, struct iovec *vec, char *dataBuf, size_t dataBufSize, char *controlBuf, size_t controlLen, struct sockaddr *addr, int addrLen);
/* get control message data of minimum size len from control message header. Return 1 if found, 0 if not found, -1 if data too short */
int getCmsgData(void **data, struct cmsghdr *cmsg, const int level, const int type, const size_t len);

/* get control message data of n-th array member of size elemSize from control message header. Return 1 if found, 0 if not found, -1 if data too short */
int getCmsgItem(void **data, struct cmsghdr *cmsg, const int level, const int type, const size_t elemSize, const int arrayIndex);

/* get timestamp from control message header based on timestamp type described by tstampConfig. Return 1 if found and non-zero, 0 if not found or zero, -1 if data too short */
int getCmsgTimestamp(CckTimestamp *timestamp, struct cmsghdr *cmsg, const TTsocketTimestampConfig *config);

/* get timestamp from a timestamp struct of the given type */
void convertTimestamp_timespec(CckTimestamp *timestamp, const void *data);
void convertTimestamp_timeval(CckTimestamp *timestamp, const void *data);
#ifdef SO_BINTIME
void convertTimestamp_bintime(CckTimestamp *timestamp, const void *data);
#endif /* SO_BINTIME */



#endif /* CCK_NET_UTILS_H_ */