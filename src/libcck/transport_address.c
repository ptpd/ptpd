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
#include <string.h>	/* strlen() */

#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/transport_address.h>

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
    return(strncmp(a->addr.local.sun_path, b->addr.local.sun_path, SUN_PATH_MAX));
}

int
transportAddressToString(char *string, const size_t len, const CckTransportAddress *address)
{
    return 1;
}

int
transportAddressFromString(CckTransportAddress *address, const char *string, const int family)
{
    return 1;
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
			min(SUN_PATH_MAX, strlen(address->addr.local.sun_path)), modulo);
}

