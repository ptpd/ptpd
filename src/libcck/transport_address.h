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
 * @file   transport_address.h
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  transport address definitions
 *
 */

#ifndef CCK_TTRANSPORT_ADDRESS_H_
#define CCK_TTRANSPORT_ADDRESS_H_

#include <config.h>

#include <stdio.h>		/* stddef.h -> size_t */

#include <arpa/inet.h>		/* struct sockaddr and friends, ntoh/hton */
#include <netinet/in.h>		/* struct sockaddr and friends */

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#ifdef HAVE_LINUX_IF_H
#include <linux/if.h>		/* struct ifaddr, ifreq, ifconf, ifmap, IF_NAMESIZE etc. */
#elif defined(HAVE_NET_IF_H)
#include <net/if.h>		/* struct ifaddr, ifreq, ifconf, ifmap, IF_NAMESIZE etc. */
#endif /* HAVE_LINUX_IF_H*/

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif /* HAVE_NET_IF_ARP_H */

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>	/* struct ether_addr */
#endif /* HAVE_NET_ETHERNET_H */

#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>	/* struct ether_addr */
#endif /* HAVE_NETINET_ETHER_H */

#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>	/* struct ether_addr */
#endif /* HAVE_NETINET_IF_ETHER_H */

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>		/* struct sockadr_un on Linux, some don't have sys/un.h */
#elif defined(HAVE_LINUX_UN_H)
#include <linux/un.h>		/* elsewhere (mostly) */
#endif /* HAVE_SYS_UN_H */

#ifdef HAVE_STRUCT_ETHER_ADDR_OCTET
#ifndef ether_addr_octet
#define ether_addr_octet octet
#endif /* ether_addr_octet */
#endif /* HAVE_STRUCT_ETHER_ADDR_OCTET */

#if !defined(ETHER_ADDR_LEN) && defined(ETHERADDRL)
#define ETHER_ADDR_LEN ETHERADDRL
#endif /* !ETHER_ADDR_LEN && ETHERADDRL */

/* transport address related constants */
#define TT_ADDRLEN_IPV4 4
#define TT_STRADDRLEN_IPV4 INET_ADDRSTRLEN

#define TT_ADDRLEN_IPV6 16
#define TT_STRADDRLEN_IPV6 INET6_ADDRSTRLEN

#define TT_ADDRLEN_ETHERNET ETHER_ADDR_LEN
			
#define TT_STRADDRLEN_ETHERNET (3 * TT_ADDRLEN_ETHERNET)

#define TT_ADDRLEN_LOCAL sizeof(((struct sockaddr_un*)0)->sun_path)
#define TT_STRADDRLEN_LOCAL TT_ADDRLEN_LOCAL

#define IPV6_SCOPE_INT_LOCAL	0x01
#define IPV6_SCOPE_LINK_LOCAL	0x02
#define IPV6_SCOPE_REALM_LOCAL	0x03
#define IPV6_SCOPE_ADMIN_LOCAL	0x04
#define IPV6_SCOPE_SITE_LOCAL	0x05
#define IPV6_SCOPE_ORG_LOCAL	0x08
#define IPV6_SCOPE_GLOBAL	0x0E

/* some IN6ADDR_ANY_INIT lack outer braces and produce unnecessary warnings */
#define CCK_IN6_ANY {{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }}}

#include <libcck/linked_list.h>
#include <libcck/cck.h>
#include <libcck/libcck.h>
#include <libcck/cck_types.h>

/* ========= address family ========= */

/* transport address families provided by timestamping transports */
enum {
	TT_FAMILY_IPV4,		/* UDPv4 */
	TT_FAMILY_IPV6,		/* UDPv6 */
	TT_FAMILY_ETHERNET,	/* Ethernet L2 */
	TT_FAMILY_LOCAL,	/* local transport, implementation-specific (UDS / named pipes) */
	TT_FAMILY_MAX		/* max or unknown */
};

/* display and type helpers */
const char*	getAddressFamilyName(const int);
int	getAddressFamilyType(const char*);


/* ========= transport address ========= */

typedef struct CckTransportAddress CckTransportAddress;

struct CckTransportAddress {
	/* so that this can be used in linked lists */
	LL_MEMBER(CckTransportAddress);

	/* address container */
	union {
		struct sockaddr		inet;
		struct sockaddr_in	inet4;
		struct sockaddr_in6	inet6;
		struct ether_addr	ether;
		struct sockaddr_un	local;
	} addr;
	int family;
	bool populated;
	int port;	/* for those address families which use port numbers */
};

/* create and free transport address objects */
CckTransportAddress	*createCckTransportAddress();
void			freeCckTransportAddress(CckTransportAddress **address);

/* copy and duplicate addresses */
void			copyCckTransportAddress(CckTransportAddress *dst, CckTransportAddress *src);
CckTransportAddress	*duplicateCckTransportAddress(CckTransportAddress *src);

/* just a memset */
void clearTransportAddress(CckTransportAddress *address);

/* string conversion */
char*			transportAddressToString(char *string, const size_t len, const CckTransportAddress *address);
bool 			transportAddressFromString(CckTransportAddress *address, const int family, const char *string);
CckTransportAddress* 	createAddressFromString(const int family, const char* string);

/* parse a list of addresses of the given family into an array, returning the number of entries populated and -1 on error*/
int	parseAddressArray(CckTransportAddress *array, const int arraySize, const int family, const char* list );

/* set the port of an address in the structure and in the underlying socket address structure */
void	setTransportAddressPort(CckTransportAddress *address, const uint16_t port);

/* set the IPv6 scope of an address */
void	setAddressScope_ipv6(CckTransportAddress *address, uint8_t scope);

/* ========= transport address list ========= */

typedef struct CckTransportAddressList CckTransportAddressList;

struct CckTransportAddressList {
	LL_HOLDER(CckTransportAddress);
	void (*add)	(CckTransportAddressList *list, CckTransportAddress *item);
	bool (*addString) (CckTransportAddressList *list, const char* string);
	bool (*remove)	(CckTransportAddressList *list, CckTransportAddress *item);
	void (*clear)	(CckTransportAddressList *list);
	void (*dump)	(CckTransportAddressList *list);
	char name[CCK_COMPONENT_NAME_MAX+1];
	int count;
	int family;
};

void 			 	setupCckTransportAddressList(CckTransportAddressList *list, const int family, const char *name);
CckTransportAddressList* 	createCckTransportAddressList(const int family, const char *name);
void				freeCckTransportAddressList(CckTransportAddressList **list);
CckTransportAddressList* 	duplicateCckTransportAddressList(const CckTransportAddressList *list);

/* ========= transport address toolset ========= */

/* address function toolset container */
typedef struct {
    int		structSize;		/* struct size */
    int		strLen;			/* address string length for conversion */
    int		afType;			/* OS address family type if exists */
    int		addrSize;		/* address data size */
    bool	(*isMulticast)	(const CckTransportAddress *);
    bool	(*isEmpty)	(const CckTransportAddress *);
    int		(*compare)	(const void*, const void*);
    uint32_t	(*hash)		(const CckTransportAddress *, int);
    char*	(*toString)	(char *, const size_t, const CckTransportAddress *);
    bool	(*fromString)	(CckTransportAddress *, const char *);
    void*	(*getData)	(CckTransportAddress *);
    void*	(*getStruct)	(CckTransportAddress *);
} CckAddressToolset;

CckAddressToolset* getAddressToolset(int family);

bool isAddressMulticast_ipv4(const CckTransportAddress *address);
bool isAddressMulticast_ipv6(const CckTransportAddress *address);
bool isAddressMulticast_ethernet(const CckTransportAddress *address);
bool isAddressMulticast_local(const CckTransportAddress *address);

bool isAddressEmpty_ipv4(const CckTransportAddress *address);
bool isAddressEmpty_ipv6(const CckTransportAddress *address);
bool isAddressEmpty_ethernet(const CckTransportAddress *address);
bool isAddressEmpty_local(const CckTransportAddress *address);

/* compare functions suitable for sort callbacks */
int cmpTransportAddress_ipv4(const void *va, const void *vb);
int cmpTransportAddress_ipv6(const void *va, const void *vb);
int cmpTransportAddress_ethernet(const void *va, const void *vb);
int cmpTransportAddress_local(const void *va, const void *vb);

/* hashing functions useful for indexing */
uint32_t transportAddressHash_ipv4(const CckTransportAddress *address, const int modulo);
uint32_t transportAddressHash_ipv6(const CckTransportAddress *address, const int modulo);
uint32_t transportAddressHash_ethernet(const CckTransportAddress *address, const int modulo);
uint32_t transportAddressHash_local(const CckTransportAddress *address, const int modulo);


/* return a pointer to the address data */
void * getAddressData(CckTransportAddress *addr);
void * getAddressData_ipv4(CckTransportAddress *addr);
void * getAddressData_ipv6(CckTransportAddress *addr);
void * getAddressData_ethernet(CckTransportAddress *addr);
void * getAddressData_local(CckTransportAddress *addr);

/* return a pointer to the address structure (in_addr not sockaddr) address data */
void * getAddressStruct(CckTransportAddress *addr);
void * getAddressStruct_ipv4(CckTransportAddress *addr);
void * getAddressStruct_ipv6(CckTransportAddress *addr);
void * getAddressStruct_ethernet(CckTransportAddress *addr);
void * getAddressStruct_local(CckTransportAddress *addr);

/* to string */
char* transportAddressToString_ipv4 (char *buf, const size_t len, const CckTransportAddress *address);
char* transportAddressToString_ipv6 (char *buf, const size_t len, const CckTransportAddress *address);
char* transportAddressToString_ethernet (char *buf, const size_t len, const CckTransportAddress *address);
char* transportAddressToString_local (char *buf, const size_t len, const CckTransportAddress *address);

/* from string */
bool transportAddressFromString_ipv4 (CckTransportAddress *out, const char *address);
bool transportAddressFromString_ipv6 (CckTransportAddress *out, const char *address);
bool transportAddressFromString_ethernet (CckTransportAddress *out, const char *address);
bool transportAddressFromString_local (CckTransportAddress *out, const char *address);

/* ========= transport address toolset end ========= */


#endif /* CCK_TTRANSPORT_ADDRESS_H_ */
