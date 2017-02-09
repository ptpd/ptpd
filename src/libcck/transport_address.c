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
 * @file   transport_address.c
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  transport address management and conversion functions
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/linked_list.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/transport_address.h>

/* constants / data */

static const char *addressFamilyNames[] = {
	[TT_FAMILY_IPV4]	= "IPv4",
	[TT_FAMILY_IPV6]	= "IPv6",
	[TT_FAMILY_ETHERNET]	= "Ethernet",
	[TT_FAMILY_LOCAL]	= "local endpoint",
	[TT_FAMILY_MAX]		= "nosuchtype"
};

static const int addressFamilyMappings[] = {
	[TT_FAMILY_IPV4]	= AF_INET,
	[TT_FAMILY_IPV6]	= AF_INET6,
	[TT_FAMILY_ETHERNET]	= AF_UNSPEC,
	[TT_FAMILY_LOCAL]	= AF_LOCAL,
	[TT_FAMILY_MAX]		= AF_MAX
};

static CckAddressToolset addressFamilyToolsets[] = {

	[TT_FAMILY_IPV4]	= {
		.structSize	= sizeof (struct sockaddr_in),
		.strLen		= TT_STRADDRLEN_IPV4,
		.afType		= AF_INET,
		.addrSize	= TT_ADDRLEN_IPV4,
		.isMulticast	= isAddressMulticast_ipv4,
		.isEmpty	= isAddressEmpty_ipv4,
		.compare	= cmpTransportAddress_ipv4,
		.hash		= transportAddressHash_ipv4,
		.toString	= transportAddressToString_ipv4,
		.fromString	= transportAddressFromString_ipv4,
		.getData	= getAddressData_ipv4,
		.getStruct	= getAddressStruct_ipv4
	},
	[TT_FAMILY_IPV6]	= {
		.structSize	= sizeof (struct sockaddr_in6),
		.strLen		= TT_STRADDRLEN_IPV6,
		.afType		= AF_INET6,
		.addrSize	= TT_ADDRLEN_IPV6,
		.isMulticast	= isAddressMulticast_ipv6,
		.isEmpty	= isAddressEmpty_ipv6,
		.compare	= cmpTransportAddress_ipv6,
		.hash		= transportAddressHash_ipv6,
		.toString	= transportAddressToString_ipv6,
		.fromString	= transportAddressFromString_ipv6,
		.getData	= getAddressData_ipv6,
		.getStruct	= getAddressStruct_ipv6
	},
	[TT_FAMILY_ETHERNET]	= {
		.structSize	= sizeof (struct ether_addr),
		.strLen		= TT_STRADDRLEN_ETHERNET,
		.afType		= AF_UNSPEC,
		.addrSize	= TT_ADDRLEN_ETHERNET,
		.isMulticast	= isAddressMulticast_ethernet,
		.isEmpty	= isAddressEmpty_ethernet,
		.compare	= cmpTransportAddress_ethernet,
		.hash		= transportAddressHash_ethernet,
		.toString	= transportAddressToString_ethernet,
		.fromString	= transportAddressFromString_ethernet,
		.getData	= getAddressData_ethernet,
		.getStruct	= getAddressStruct_ethernet
	},
	[TT_FAMILY_LOCAL]	= {
		.structSize	= sizeof (struct sockaddr_un),
		.strLen		= TT_STRADDRLEN_LOCAL,
		.afType		= AF_LOCAL,
		.addrSize	= TT_ADDRLEN_LOCAL,
		.isMulticast	= isAddressMulticast_local,
		.isEmpty	= isAddressEmpty_local,
		.compare	= cmpTransportAddress_local,
		.hash		= transportAddressHash_local,
		.toString	= transportAddressToString_local,
		.fromString	= transportAddressFromString_local,
		.getData	= getAddressData_local,
		.getStruct	= getAddressStruct_local
	}
};

/* static declarations */

static char* inetAddressToString(char *buf, const size_t len, const CckTransportAddress *address);
static bool inetAddressFromString (CckTransportAddress *out, const int af, const char *address);

static void addrListAdd		(CckTransportAddressList *list, CckTransportAddress *item);
static bool addrListAddString	(CckTransportAddressList *list, const char* string);
static bool addrListRemove	(CckTransportAddressList *list, CckTransportAddress *item);
static void addrListClear	(CckTransportAddressList *list);
static void addrListDump	(CckTransportAddressList *list);

/* public definitions */

CckAddressToolset* getAddressToolset(int family)
{
	if(family < 0 || family >= TT_FAMILY_MAX) {
	    CCK_ERROR("getAddressToolset(): unsupported address family %02x\n", family);
	    return NULL;
	}
	return &addressFamilyToolsets[family];
}

bool
isAddressMulticast_ipv4(const CckTransportAddress *address)
{
    /* first byte is 0x0E - multicast */
    return ((ntohl(address->addr.inet4.sin_addr.s_addr) >> 28) == 0x0E);
}

bool
isAddressMulticast_ipv6(const CckTransportAddress *address)
{
    /* first byte is 0xFF - multicast */
    return ((address->addr.inet6.sin6_addr.s6_addr[0]) == 0xFF);
}

bool
isAddressMulticast_ethernet(const CckTransportAddress *address)
{
    /* last bit of first byte is lit - multicast */
    return((address->addr.ether.ether_addr_octet[0] & 0x01) == 0x01);
}

bool
isAddressMulticast_local(const CckTransportAddress *address)
{
    return false;
}


bool
isAddressEmpty_ipv4(const CckTransportAddress *address)
{
    return (address->addr.inet4.sin_addr.s_addr == 0);
}

bool
isAddressEmpty_ipv6(const CckTransportAddress *address)
{
    struct in6_addr zero;
    memset(&zero, 0, sizeof(zero));
    return (!memcmp(&address->addr.inet6.sin6_addr, &zero, sizeof(zero)));
}

bool
isAddressEmpty_ethernet(const CckTransportAddress *address)
{
    struct ether_addr zero;
    memset(&zero, 0, sizeof(zero));
    return (!memcmp(&address->addr.ether, &zero, sizeof(zero)));
}

bool
isAddressEmpty_local(const CckTransportAddress *address)
{
    return(!strlen(address->addr.local.sun_path));
}

int
cmpTransportAddress_ipv4(const void *va, const void *vb)
{
    const CckTransportAddress *a = va;
    const CckTransportAddress *b = vb;

    uint32_t ia = ntohl(a->addr.inet4.sin_addr.s_addr);
    uint32_t ib = ntohl(b->addr.inet4.sin_addr.s_addr);

    return ((ia == ib) ? 0 : (ia > ib) ? 1 : -1);
}

int
cmpTransportAddress_ipv6(const void *va, const void *vb)
{
    const CckTransportAddress *a = va;
    const CckTransportAddress *b = vb;

    return (memcmp(&a->addr.inet6.sin6_addr, &b->addr.inet6.sin6_addr, sizeof(struct in6_addr)));
}

int
cmpTransportAddress_ethernet(const void *va, const void *vb)
{
    const CckTransportAddress *a = va;
    const CckTransportAddress *b = vb;
    return (memcmp(&a->addr.ether, &b->addr.ether, sizeof(struct ether_addr)));
}

int
cmpTransportAddress_local(const void *va, const void *vb)
{
    const CckTransportAddress *a = va;
    const CckTransportAddress *b = vb;
    return(strncmp(a->addr.local.sun_path, b->addr.local.sun_path, TT_ADDRLEN_LOCAL));
}


uint32_t
transportAddressHash_ipv4(const CckTransportAddress *address, const int modulo)
{
    return getFnvHash(&address->addr.inet4.sin_addr.s_addr, 4, modulo);
}

uint32_t
transportAddressHash_ipv6(const CckTransportAddress *address, const int modulo)
{
    return getFnvHash(&address->addr.inet6.sin6_addr.s6_addr, 16, modulo);
}

uint32_t
transportAddressHash_ethernet(const CckTransportAddress *address, const int modulo)
{
    return getFnvHash(&address->addr.ether.ether_addr_octet, ETHER_ADDR_LEN, modulo);
}

uint32_t
transportAddressHash_local(const CckTransportAddress *address, const int modulo)
{
    /* use a max of 108 bytes (size of sun_path) in case of an unterminated string */
    return getFnvHash(&address->addr.local.sun_path,
			min(TT_ADDRLEN_LOCAL, strlen(address->addr.local.sun_path)), modulo);
}

const char*
getAddressFamilyName(const int family)
{

    if ((family < 0) || (family >= TT_FAMILY_MAX)) {
	return NULL;
    }

    return addressFamilyNames[family];

}

int
getAddressFamilyType(const char* name)
{

    for(int i = 0; i < TT_FAMILY_MAX; i++) {

	if(!strcmp(name, addressFamilyNames[i])) {
	    return i;
	}

    }

    return -1;

}

void
clearTransportAddress(CckTransportAddress *address)
{
    memset(address, 0, sizeof(CckTransportAddress));
}

void*
getAddressData(CckTransportAddress *addr)
{

	switch(addr->family) {
	    case TT_FAMILY_IPV4:
		return &addr->addr.inet4.sin_addr.s_addr;
	    case TT_FAMILY_IPV6:
		return &addr->addr.inet6.sin6_addr.s6_addr;
	    case TT_FAMILY_ETHERNET:
		return &addr->addr.ether;
	    case TT_FAMILY_LOCAL:
		return &addr->addr.local.sun_path;
	    default:
		return NULL;
	}

}

void*
getAddressStruct(CckTransportAddress *addr)
{

	switch(addr->family) {
	    case TT_FAMILY_IPV4:
		return &addr->addr.inet4.sin_addr;
	    case TT_FAMILY_IPV6:
		return &addr->addr.inet6.sin6_addr;
	    case TT_FAMILY_ETHERNET:
		return &addr->addr.ether;
	    case TT_FAMILY_LOCAL:
		return &addr->addr.local.sun_path;
	    default:
		return NULL;
	}

}

void*
getAddressData_ipv4(CckTransportAddress *addr)
{

	return &addr->addr.inet4.sin_addr.s_addr;

}

void*
getAddressData_ipv6(CckTransportAddress *addr)
{

	return &addr->addr.inet6.sin6_addr.s6_addr;

}

void*
getAddressData_ethernet(CckTransportAddress *addr)
{

	return &addr->addr.ether.ether_addr_octet;

}

void*
getAddressData_local(CckTransportAddress *addr)
{

	return &addr->addr.local.sun_path;

}

void*
getAddressStruct_ipv4(CckTransportAddress *addr)
{

	return &addr->addr.inet4.sin_addr;

}

void*
getAddressStruct_ipv6(CckTransportAddress *addr)
{

	return &addr->addr.inet6.sin6_addr;

}

void*
getAddressStruct_ethernet(CckTransportAddress *addr)
{

	return &addr->addr.ether;

}

void*
getAddressStruct_local(CckTransportAddress *addr)
{

	return &addr->addr.local.sun_path;

}


/* string conversion */

char*
transportAddressToString(char *string, const size_t len, const CckTransportAddress *address)
{
    	switch(address->family) {
	    case TT_FAMILY_IPV4:
		return transportAddressToString_ipv4(string, len, address);
	    case TT_FAMILY_IPV6:
		return transportAddressToString_ipv6(string, len, address);
	    case TT_FAMILY_ETHERNET:
		return transportAddressToString_ethernet(string, len, address);
	    case TT_FAMILY_LOCAL:
		return transportAddressToString_local(string, len, address);
	    default:
		CCK_ERROR("transportAddressToString: unsupported address family %d\n",
			    address->family);
		return NULL;
	}

}

bool
transportAddressFromString(CckTransportAddress *address, const int family, const char *string)
{

    	switch(family) {
	    case TT_FAMILY_IPV4:
		return transportAddressFromString_ipv4(address, string);
	    case TT_FAMILY_IPV6:
		return transportAddressFromString_ipv6(address, string);
	    case TT_FAMILY_ETHERNET:
		return transportAddressFromString_ethernet(address, string);
	    case TT_FAMILY_LOCAL:
		return transportAddressFromString_local(address, string);
	    default:
		CCK_ERROR("transportAddressToString(%s): unsupported address family %d\n", string,
			    family);
		return false;
	}

}

CckTransportAddress*
createAddressFromString(const int family, const char* string) 
{
    CckTransportAddress *out;

    CCKCALLOC(out, sizeof(CckTransportAddress));

    if(transportAddressFromString(out, family, string)) {
	return out;
    }

    free(out);

    return NULL;

}

static char*
inetAddressToString(char *buf, const size_t len, const CckTransportAddress *address)
{
    memset(buf, 0, len);

    if((address->family < 0) || (address->family >= TT_FAMILY_MAX)) {
	CCK_INFO("inetAddressToString(): unknown address family %d\n", address->family);
	return NULL;
    }

    if(!inet_ntop(addressFamilyMappings[address->family], getAddressStruct((CckTransportAddress*)address), buf, (int)len)) {
	CCK_DBG("inetAddressToString(%s): could not convert address to string: %s\n",
		addressFamilyNames[address->family], strerror(errno));
	return NULL;
    }

    return buf;

}

static bool
inetAddressFromString (CckTransportAddress *out, const int af, const char *address)
{

    bool ret = false;
    int res = 0;
    struct addrinfo hints, *info;

    clearTransportAddress(out);
    memset(&hints, 0, sizeof(hints));

    if((af < 0) || (af >= TT_FAMILY_MAX)) {
	CCK_DBG("inetAddressFromString(%s): unknown address family %d\n", address, af);
	return false;
    }

    hints.ai_family = addressFamilyMappings[af];
    hints.ai_socktype = SOCK_DGRAM;

    res = getaddrinfo(address, NULL, &hints, &info);

    if(res == 0) {

        ret = true;
	out->populated = true;
	out->family = af;
        memcpy(&out->addr, info->ai_addr,
	    info->ai_addrlen);
	freeaddrinfo(info);

    } else {
        CCK_ERROR("inetAddressFromString(%s): could not parse / resolve %s address: %s\n",
	    address, addressFamilyNames[af], gai_strerror(res));
    }

    return ret;

}

char*
transportAddressToString_ipv4 (char *buf, const size_t len, const CckTransportAddress *address)
{
    return inetAddressToString(buf, len, address);
}

char*
transportAddressToString_ipv6 (char *buf, const size_t len, const CckTransportAddress *address)
{
    return inetAddressToString(buf, len, address);
}

char*
transportAddressToString_ethernet (char *buf, const size_t len, const CckTransportAddress *address)
{

    uint8_t *addr = (uint8_t*)&address->addr.ether;

    if(len < TT_STRADDRLEN_ETHERNET) {
	return NULL;
    }

    memset(buf, 0, len);

    snprintf(buf, len, "%02x:%02x:%02x:%02x:%02x:%02x",
    addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    return buf;

}

char*
transportAddressToString_local (char *buf, const size_t len, const CckTransportAddress *address)
{
    memset(buf, 0, len);

    strncpy(buf, address->addr.local.sun_path, min(TT_ADDRLEN_LOCAL, len));
    return buf;
}

bool
transportAddressFromString_ipv4 (CckTransportAddress *out, const char *address)
{
    return inetAddressFromString(out, TT_FAMILY_IPV4, address);
}

bool
transportAddressFromString_ipv6 (CckTransportAddress *out, const char *address)
{
    return inetAddressFromString(out, TT_FAMILY_IPV6, address);
}

bool
transportAddressFromString_ethernet (CckTransportAddress *out, const char *address)
{

    int i;
    int digit;
    int octet;

    char *addr = (char*)address;

    clearTransportAddress(out);
    out->family = TT_FAMILY_ETHERNET;

    if(!ether_hostton(address, &out->addr.ether)) {
	return true;
    }

    if(strlen(address) > TT_STRADDRLEN_ETHERNET) {
	CCK_ERROR("transportAddressFromString(%s): Ethernet address too long\n", address);
	return false;
    }

    /* ...because ether_aton_r... */
    for(i = 0; i < 6; i++) {

	if((digit = hexDigitToInt(*addr++)) < 0) {
	    break;
	}

	if(!*addr || (*addr == ':')) {
	    octet = digit;
	    out->addr.ether.ether_addr_octet[i] = octet;
	} else {
	    octet = digit << 4;
	    if((digit = hexDigitToInt(*addr++)) < 0) {
		break;
	    }
	    octet += digit;
	    out->addr.ether.ether_addr_octet[i] = octet;
	}

	if(*addr == ':') {
	    addr++;
	}

    }

    if(i == 6 ) return true;

    CCK_ERROR("transportAddressFromString(%s): Could not resolve / encode Ethernet address\n",
		address);

    return false;

}

bool
transportAddressFromString_local (CckTransportAddress *out, const char *address)
{

    if(!address) {
	CCK_ERROR("transportAddressFromString(): NULL string given\n");
	return false;
    }

    out->family = TT_FAMILY_LOCAL;

    memset(out->addr.local.sun_path, 0, TT_ADDRLEN_LOCAL);
    strncpy(out->addr.local.sun_path, address, min(TT_ADDRLEN_LOCAL, strlen(address)));

    return true;

}

int
parseAddressArray(CckTransportAddress *array, const int arraySize, const int family, const char* list )
{

	CckAddressToolset *ts = getAddressToolset(family);
	CckTransportAddress current;
	char strAddr[ts->strLen];
	int found = 0;

	if(!ts) {
	    return -1;
	}

	foreach_token_begin(addrlist, list, item, DEFAULT_TOKEN_DELIM);

	    if (found >= arraySize) {
		CCK_INFO("parseAddressArray(%s): maximum address list size reached (%d)\n", list, arraySize);
		break;
	    }

	    memset(&current, 0, sizeof(current));
	    memset(strAddr, 0, ts->strLen);

	    if(!ts->fromString(&current, item)) {
		continue;
	    }

	    memcpy(&array[found], &current, sizeof(current));

	    found++;

	    CCK_DBG("parseAddressArray() #%03d address parrsed: %s\n", found, ts->toString(strAddr, ts->strLen, &current));

	foreach_token_end(addrlist);

	return found;

}

/* create and free transport address objects */
CckTransportAddress
*createCckTransportAddress()
{
    CckTransportAddress *out;

    CCKCALLOC(out, sizeof(CckTransportAddress));

    return out;
}

void
freeCckTransportAddress(CckTransportAddress **address)
{

    if(*address) {
	free(*address);
    }

    *address = NULL;

}

/* copy / duplicate transport address objects */
void
copyCckTransportAddress(CckTransportAddress *dst, CckTransportAddress *src)
{

    if(src == NULL || dst == NULL) {
	return;
    }

    memcpy(dst, src, sizeof(CckTransportAddress));

}

CckTransportAddress*
duplicateCckTransportAddress(CckTransportAddress *src)
{

    CckTransportAddress *out = NULL;

    if(src == NULL) {
	return NULL;
    }

    out = createCckTransportAddress();

    copyCckTransportAddress(out, src);

    return out;
}

void
setTransportAddressPort(CckTransportAddress *address, const uint16_t port)
{

	switch(address->family) {

	    case TT_FAMILY_IPV4:
		address->port = port;
		address->addr.inet4.sin_port = htons(port);
		break;

	    case TT_FAMILY_IPV6:
		address->port = port;
		address->addr.inet6.sin6_port = htons(port);
		break;

	    default:
		break;

	}

}

void
setAddressScope_ipv6(CckTransportAddress *address, uint8_t scope)
{
	if(!address || (address->family != TT_FAMILY_IPV6)) {
	    return;
	}

	address->addr.inet6.sin6_addr.s6_addr[1] &= 0xF0;
	address->addr.inet6.sin6_addr.s6_addr[1] |= (scope & 0x0F);
}

/* Address list */

void
setupCckTransportAddressList(CckTransportAddressList *list, const int family, const char *name)
{

    if(!list) {
	return;
    }

    memset(list, 0, sizeof(CckTransportAddressList));

    list->family = family;
    strncpy(list->name, name, CCK_COMPONENT_NAME_MAX);

    list->add = addrListAdd;
    list->addString = addrListAddString;
    list->remove = addrListRemove;
    list->clear = addrListClear;
    list->dump = addrListDump;

}


CckTransportAddressList*
createCckTransportAddressList(const int family, const char* name)
{

    CckTransportAddressList *out;

    CCKCALLOC(out, sizeof(CckTransportAddressList));

    if(!out) {
	return out;
    }

    setupCckTransportAddressList(out, family, name);

    return out;

}

void
freeCckTransportAddressList(CckTransportAddressList **list)
{

    CckTransportAddress *item;

    if(!list || !*list) {
	return;
    }

    while((*list)->_first) {
	item = (*list)->_last;
	(*list)->remove(*list, item);
	freeCckTransportAddress(&item);
    }

    free(*list);
    *list = NULL;

}

CckTransportAddressList*
duplicateCckTransportAddressList(const CckTransportAddressList *list)
{

    CckTransportAddress *addr, *copy;

    if(!list) {
	return NULL;
    }

    CckTransportAddressList *dup = createCckTransportAddressList(list->family, list->name);

    LL_FOREACH_DYNAMIC(list, addr) {

	copy = duplicateCckTransportAddress(addr);
	dup->add(dup, copy);

    }

    return dup;

}

static void
addrListAdd(CckTransportAddressList *list, CckTransportAddress *item)
{

    if(!item) {
	return;
    }

    LL_APPEND_DYNAMIC(list, item);
    list->count++;

}

static bool
addrListAddString (CckTransportAddressList *list, const char* string)
{

    CckAddressToolset *ts = getAddressToolset(list->family);
    CckTransportAddress *addr;

    if(!ts) {
	CCK_DBG("addrListAddString(): unsupported address family %d\n", list->family);
	return false;
    }

    addr = createAddressFromString(list->family, string);

    if(addr) {
	list->add(list, addr);
	return true;
    }

    return false;
}

static bool
addrListRemove (CckTransportAddressList *list, CckTransportAddress *item)
{

    if(!item) {
	return false;
    }

    /* item does not belong to the list */
    if(*(item->_first) != list->_first) {
	return false;
    }

    LL_REMOVE_DYNAMIC(list, item);
    list->count--;

    return true;

}

static void
addrListClear (CckTransportAddressList *list)
{

    CckTransportAddress *item;

    LL_FOREACH_DYNAMIC( list, item) {
	list->remove(list, item);
    }

    list->count = 0;

}

static void
addrListDump (CckTransportAddressList *list)
{

    CckAddressToolset *ts = getAddressToolset(list->family);
    CckTransportAddress *address;

    CCK_INFO("=== Transport address list '%s' contents (%d items)  ===\n", list->name, list->count);

    if(!ts) {
	CCK_INFO("[unsupported address family %d]\n", list->family);
	return;
    }

    if(!list->count) {
	CCK_INFO("[empty list]\n");
    }

    tmpstr(strAddr, ts->strLen);

    LL_FOREACH_DYNAMIC(list, address) {

	CCK_INFO("+\t%s\n", ts->toString(strAddr, strAddr_len, address));

	clearstr(strAddr);
    }

}
