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
 * @file   cck_transport_socket_common.h
 *
 * @brief  common libCCK transport helper function implementations
 *
 */

#ifndef CCK_TRANSPORT_SOCKET_COMMON_H_
#define CCK_TRANSPORT_SOCKET_COMMON_H_

#include "../cck.h"

#include <config.h>

#include <limits.h>

#if !defined(linux) && !defined(__NetBSD__) && !defined(__FreeBSD__) && \
  !defined(__APPLE__) && !defined(__OpenBSD__)
#error PTPD hasn't been ported to this OS - should be possible \
if it's POSIX compatible, if you succeed, report it to ptpd-devel@sourceforge.net
#endif

#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>

#ifdef	linux
#include<netinet/in.h>
#include<net/if.h>
#include<net/if_arp.h>
#include <ifaddrs.h>
#define IFACE_NAME_LENGTH         IF_NAMESIZE
#define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

#define IFCONF_LENGTH 10

#define octet ether_addr_octet
#include<endian.h>
#endif /* linux */



#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <net/if.h>
# include <net/if_dl.h>
# include <net/if_types.h>
#ifdef HAVE_NET_IF_ETHER_H
#  include <net/if_ether.h>
#endif
#ifdef HAVE_SYS_UIO_H
#  include <sys/uio.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#  include <net/ethernet.h>
#endif
#  include <ifaddrs.h>

#endif

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif /* HAVE_NET_ETHERNET_H */

#include <netinet/in.h>
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#include <netinet/ip.h>
#include <netinet/udp.h>
#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif /* HAVE_NETINET_ETHER_H */

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif /* HAVE_NET_IF_ARP_H*/

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif /* HAVE_NET_IF_H*/

#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif /* HAVE_NETINET_IF_ETHER_H */

#ifdef SO_TIMESTAMPING
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#endif /* SO_TIMESTAMPING */

#include <sys/ioctl.h>
#include <unistd.h>

CckBool cckGetHwAddress (const char* ifaceName, TransportAddress* addr);
int cckGetInterfaceFlags(const char* ifaceName, unsigned int* flags);
int cckGetInterfaceAddress(const CckTransport* transport, TransportAddress* addr, int addressFamily);
void cckAddFd(int fd, CckFdSet* set);
void cckRemoveFd(int fd, CckFdSet* set);
int cckSelect(CckFdSet* set, struct timeval* timeout);

#endif /* CCK_TRANSPORT_SOCKET_COMMON_H_ */
