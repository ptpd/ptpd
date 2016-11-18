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
 * @file   transport_address.c
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  transport address management and conversion functions
 *
 */

#include <stdio.h>	/* stddef.h -> size_t */
#include <string.h>	/* strlen(), strerror() */
#include <errno.h>	/* well, errno */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/transport_address.h>

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
	[TT_FAMILY_IPV4]	= { isAddressMulticast_ipv4, isAddressEmpty_ipv4, cmpTransportAddress_ipv4, transportAddressHash_ipv4, transportAddressToString_ipv4, transportAddressFromString_ipv4 },
	[TT_FAMILY_IPV6]	= { isAddressMulticast_ipv6, isAddressEmpty_ipv6, cmpTransportAddress_ipv6, transportAddressHash_ipv6, transportAddressToString_ipv6, transportAddressFromString_ipv6 },
	[TT_FAMILY_ETHERNET]	= { isAddressMulticast_ethernet, isAddressEmpty_ethernet, cmpTransportAddress_ethernet, transportAddressHash_ethernet, transportAddressToString_ethernet, transportAddressFromString_ethernet },
	[TT_FAMILY_LOCAL]	= { isAddressMulticast_local, isAddressEmpty_local, cmpTransportAddress_local, transportAddressHash_local, transportAddressToString_ethernet, transportAddressFromString_local },
};

static char* inetAddressToString(char *buf, const size_t len, const CckTransportAddress *address);
static CckBool inetAddressFromString (CckTransportAddress *out, const int af, const char *address);
static void * getAddressData(CckTransportAddress *addr);

CckAddressToolset* getAddressToolset(int family)
{
	if(family < 0 || family >= TT_FAMILY_MAX) {
	    return NULL;
	}
	return &addressFamilyToolsets[family];
}

CckBool
isAddressMulticast_ipv4(const CckTransportAddress *address)
{
    /* first byte is 0x0E - multicast */
    return ((ntohl(address->addr.inet4.sin_addr.s_addr) >> 28) == 0x0E);
}

CckBool
isAddressMulticast_ipv6(const CckTransportAddress *address)
{
    /* first byte is 0xFF - multicast */
    return ((address->addr.inet6.sin6_addr.s6_addr[0]) == 0xFF);
}

CckBool
isAddressMulticast_ethernet(const CckTransportAddress *address)
{
    /* last bit of first byte is lit - multicast */
    return((address->addr.ether.ether_addr_octet[0] & 0x01) == 0x01);
}

CckBool
isAddressMulticast_local(const CckTransportAddress *address)
{
    return CCK_FALSE;
}


CckBool
isAddressEmpty_ipv4(const CckTransportAddress *address)
{
    return (address->addr.inet4.sin_addr.s_addr == 0);
}

CckBool
isAddressEmpty_ipv6(const CckTransportAddress *address)
{
    struct in6_addr zero;
    memset(&zero, 0, sizeof(zero));
    return (!memcmp(&address->addr.inet6.sin6_addr, &zero, sizeof(zero)));
}

CckBool
isAddressEmpty_ethernet(const CckTransportAddress *address)
{
    struct ether_addr zero;
    memset(&zero, 0, sizeof(zero));
    return (!memcmp(&address->addr.ether, &zero, sizeof(zero)));
}

CckBool
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


CckU32
transportAddressHash_ipv4(const CckTransportAddress *address, const int modulo)
{
    return getFnvHash(&address->addr.inet4.sin_addr.s_addr, 4, modulo);
}

CckU32
transportAddressHash_ipv6(const CckTransportAddress *address, const int modulo)
{
    return getFnvHash(&address->addr.inet6.sin6_addr.s6_addr, 16, modulo);
}

CckU32
transportAddressHash_ethernet(const CckTransportAddress *address, const int modulo)
{
    return getFnvHash(&address->addr.ether.ether_addr_octet, ETHER_ADDR_LEN, modulo);
}

CckU32
transportAddressHash_local(const CckTransportAddress *address, const int modulo)
{
    /* use a max of 108 bytes (size of sun_path) in case of an unterminated string */
    return getFnvHash(&address->addr.local.sun_path,
			min(TT_ADDRLEN_LOCAL, strlen(address->addr.local.sun_path)), modulo);
}

const char*
getAddressFamilyName(int type)
{

    if ((type < 0) || (type >= TT_FAMILY_MAX)) {
	return NULL;
    }

    return addressFamilyNames[type];

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

static void*
getAddressData(CckTransportAddress *addr)
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

CckBool
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
		return CCK_FALSE;
	}

}



static char*
inetAddressToString(char *buf, const size_t len, const CckTransportAddress *address)
{
    memset(buf, 0, len);

    if((address->family < 0) || (address->family >= TT_FAMILY_MAX)) {
	CCK_DBG("inetAddressToString: unknown address family %d\n", address->family);
	return NULL;
    }

    if(!inet_ntop(addressFamilyMappings[address->family], getAddressData((CckTransportAddress*)address), buf, (int)len)) {
	CCK_DBG("inetAddressToString: could not convert %s address to string: %s\n",
		addressFamilyNames[address->family], strerror(errno));
	return NULL;
    }

    return buf;

}

static CckBool
inetAddressFromString (CckTransportAddress *out, const int af, const char *address)
{

    CckBool ret = CCK_FALSE;
    int res = 0;
    struct addrinfo hints, *info;

    clearTransportAddress(out);
    memset(&hints, 0, sizeof(hints));

    if((af < 0) || (af >= TT_FAMILY_MAX)) {
	CCK_DBG("inetAddressFromString(%s): unknown address family %d\n", address, af);
	return CCK_FALSE;
    }

    hints.ai_family = addressFamilyMappings[af];
    hints.ai_socktype = SOCK_DGRAM;

    res = getaddrinfo(address, NULL, &hints, &info);

    if(res == 0) {

        ret = CCK_TRUE;
	out->populated = CCK_TRUE;
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

    memset(buf, 0, len);

    uint8_t *addr = (uint8_t*)&address->addr.ether;

    if(len < (3 * TT_ADDRLEN_ETHERNET)) {
	return NULL;
    }

    snprintf(buf, len, "%02x:%02x:%02x:%02x:%02x:%02x",
    *(addr), *(addr+1), *(addr+2), *(addr+3), *(addr+4), *(addr+5));

    return buf;

}

char*
transportAddressToString_local (char *buf, const size_t len, const CckTransportAddress *address)
{
    memset(buf, 0, len);

    strncpy(buf, address->addr.local.sun_path, min(TT_ADDRLEN_LOCAL, len));
    return buf;
}

CckBool
transportAddressFromString_ipv4 (CckTransportAddress *out, const char *address)
{
    return inetAddressFromString(out, TT_FAMILY_IPV4, address);
}

CckBool
transportAddressFromString_ipv6 (CckTransportAddress *out, const char *address)
{
    return inetAddressFromString(out, TT_FAMILY_IPV6, address);
}

CckBool
transportAddressFromString_ethernet (CckTransportAddress *out, const char *address)
{

    int i;
    int digit;
    int octet;
    unsigned char *addr = (unsigned char*) address;

    clearTransportAddress(out);
    out->family = TT_FAMILY_ETHERNET;

    if(!ether_hostton(address, &out->addr.ether)) {
	return CCK_TRUE;
    }

    if(strlen(address) > TT_STRADDRLEN_ETHERNET) {
	CCK_ERROR("transportAddressFromString: Ethernet address too long: %s\n", address);
	return CCK_FALSE;
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

    if(i == 6 ) return CCK_TRUE;

    CCK_ERROR("Could not resolve / encode Ethernet address: %s\n",
		address);

    return CCK_FALSE;

}

CckBool
transportAddressFromString_local (CckTransportAddress *out, const char *address)
{

    if(!address) {
	CCK_ERROR("transportAddressFromString_local: NULL string given\n");
	return CCK_FALSE;
    }

    out->family = TT_FAMILY_LOCAL;

    memset(out->addr.local.sun_path, 0, TT_ADDRLEN_LOCAL);
    strncpy(out->addr.local.sun_path, address, min(TT_ADDRLEN_LOCAL, strlen(address)));

    return CCK_TRUE;

}
