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
 * @file   transport_address.h
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  transport address definitions
 *
 */

#ifndef CCK_TTRANSPORT_ADDRESS_H_
#define CCK_TTRANSPORT_ADDRESS_H_

#define SUN_PATH_MAX 108

#include <config.h>

#include <stdio.h>		/* stddef.h -> size_t */

#include <arpa/inet.h>		/* struct sockaddr and friends, ntoh/hton */
#include <netinet/in.h>		/* struct sockaddr and friends */

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>	/* struct ether_addr */
#endif /* HAVE_NET_ETHERNET_H */

#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>	/* struct ether_addr */
#endif /* HAVE_NETINET_ETHER_H */

#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>	/* struct ether_addr */
#endif /* HAVE_NETINET_IF_ETHER_H */

#ifdef HAVE_LINUX_UN_H
#include <linux/un.h>		/* struct sockadr_un on Linux, some don't have sys/un.h */
#elif defined(HAVE_SYS_UN_H)
#include <sys/un.h>		/* elsewhere (mostly) */
#endif /* HAVE_SYS_UN_H */

#ifdef HAVE_STRUCT_ETHER_ADDR_OCTET
#ifndef ether_addr_octet
#define ether_addr_octet octet
#endif /* ether_addr_octet */
#endif /* HAVE_STRUCT_ETHER_ADDR_OCTET */

#if !defined(ETHER_ADDR_LEN) && defined(ETHERADDRL)
#define ETHER_ADDR_LEN ETHERADDRL
#endif /* !ETHER_ADDR_LEN && ETHERADDRL */

#include <libcck/cck_types.h>

/* transport address families provided by timestamping transports */
enum {
	TT_FAMILY_UDP_IPV4,		/* UDPv4 */
	TT_FAMILY_UDP_IPV6,		/* UDPv6 */
	TT_FAMILY_ETHERNET,		/* Ethernet L2 */
	TT_FAMILY_LOCAL			/* local transport, implementation-specific (UDS / named pipes) */
};

typedef struct {
	int family;
	CckBool empty;
	union {
		struct sockaddr		inet;
		struct sockaddr_in	inet4;
		struct sockaddr_in6	inet6;
		struct ether_addr	ether;
		struct sockaddr_un	local;
	} addr;
} CckTransportAddress;

typedef struct {
	CckOctet *buf;
	int	len;
	CckBool fromSelf;
	CckBool toSelf;
	CckTransportAddress destination;
	CckTransportAddress source;
} CckMessage;

CckBool isAddressMulticast_ipv4(const CckTransportAddress *address);
CckBool isAddressMulticast_ipv6(const CckTransportAddress *address);
CckBool isAddressMulticast_ethernet(const CckTransportAddress *address);
CckBool isAddressMulticast_local(const CckTransportAddress *address);

CckBool isAddressEmpty_ipv4(const CckTransportAddress *address);
CckBool isAddressEmpty_ipv6(const CckTransportAddress *address);
CckBool isAddressEmpty_ethernet(const CckTransportAddress *address);
CckBool isAddressEmpty_local(const CckTransportAddress *address);

/* compare functions suitable for sort callbacks */
int cmpTransportAddress_ipv4(const void *va, const void *vb);
int cmpTransportAddress_ipv6(const void *va, const void *vb);
int cmpTransportAddress_ethernet(const void *va, const void *vb);
int cmpTransportAddress_local(const void *va, const void *vb);

int transportAddressToString(char *string, const size_t len, const CckTransportAddress *address);
int transportAddressFromString(CckTransportAddress *address, const char *string, const int family);

CckU32 transportAddressHash_ipv4(const CckTransportAddress *address, const int modulo);
CckU32 transportAddressHash_ipv6(const CckTransportAddress *address, const int modulo);
CckU32 transportAddressHash_ethernet(const CckTransportAddress *address, const int modulo);
CckU32 transportAddressHash_local(const CckTransportAddress *address, const int modulo);


#endif /* CCK_TTRANSPORT_ADDRESS_H_ */
