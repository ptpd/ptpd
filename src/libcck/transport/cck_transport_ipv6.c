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
 * @file   cck_transport_ipv6.c
 * 
 * @brief  libCCK ipv6 transport implementation
 *
 */

#include "cck_transport_ipv6.h"

static int	cckTransportInit_ipv6 (CckTransport* transport, const CckTransportConfig* config);
static int	cckTransportShutdown_ipv6 (void* _transport);
static int     cckTransportPushConfig_ipv6 (CckTransport* transport, const CckTransportConfig* config);
static CckBool cckTransportTestConfig_ipv6 (CckTransport* transport, const CckTransportConfig* config);
static void    cckTransportRefresh_ipv6 (CckTransport* transport);
static CckBool cckTransportHasData_ipv6 (CckTransport* transport);
static CckBool cckTransportIsMulticastAddress_ipv6 (const TransportAddress* address);
static ssize_t cckTransportSend_ipv6 (CckTransport* transport, CckOctet* buf, CckUInt16 size, TransportAddress* dst, CckTimestamp* timestamp);
static ssize_t cckTransportRecv_ipv6 (CckTransport* transport, CckOctet* buf, CckUInt16 size, TransportAddress* src, CckTimestamp* timestamp, int flags);
static CckBool cckTransportAddressFromString_ipv6 (const char*, TransportAddress* out);
static char*   cckTransportAddressToString_ipv6 (const TransportAddress* address);
static CckBool cckTransportAddressEqual_ipv6 (const TransportAddress* a, const TransportAddress* b);

void
cckTransportSetup_ipv6(CckTransport* transport)
{
    CCK_SETUP_TRANSPORT(CCK_TRANSPORT_UDP_IPV6, ipv6);
}

static CckBool
multicastJoin(CckTransport* transport, TransportAddress* mcastAddr)
{

    struct ipv6_mreq imr;

    memcpy(imr.ipv6mr_multiaddr.s6_addr, mcastAddr->inetAddr6.sin6_addr.s6_addr, 16);
    imr.ipv6mr_interface = if_nametoindex(transport->transportEndpoint);

    /* multicast send only on specified interface */
    if (setsockopt(transport->fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
           &imr.ipv6mr_interface, sizeof(int)) < 0) {
        CCK_PERROR("failed to set multicast outbound interface");
        return CCK_FALSE;
    }

    /* join multicast group on specified interface */
    if (setsockopt(transport->fd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
           &imr, sizeof(struct ipv6_mreq)) < 0) {
	    CCK_PERROR("failed to join multicast group %s: ", transport->addressToString(mcastAddr));
		    return CCK_FALSE;
    }

    return CCK_TRUE;

}

static CckBool
multicastLeave(CckTransport* transport, TransportAddress* mcastAddr)
{

    struct ipv6_mreq imr;

    memcpy(imr.ipv6mr_multiaddr.s6_addr, mcastAddr->inetAddr6.sin6_addr.s6_addr, 16);
    imr.ipv6mr_interface = if_nametoindex(transport->transportEndpoint);

    /* leave multicast group on specified interface */
    if (setsockopt(transport->fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
           &imr, sizeof(struct ipv6_mreq)) < 0) {
	    CCK_PERROR("failed to leave multicast group %s: ", transport->addressToString(mcastAddr));
		    return CCK_FALSE;
    }

    return CCK_TRUE;

}

static CckBool
setMulticastLoopback(CckTransport* transport, CckBool _value)
{

	CckUInt8 value = _value ? 1 : 0;

	CCK_DBG("Going to %s IPv6 multicast loopback on %s \n", value ? "enable":"disable", transport->transportEndpoint);

	if (setsockopt(transport->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, 
	       &value, sizeof(value)) < 0) {
		CCK_PERROR("Failed to set IPv6 multicast loopback");
		return CCK_FALSE;
	}

	return CCK_TRUE;
}

static CckBool
setMulticastTtl(CckTransport* transport, int value)
{

        CCK_DBG("Going to set IPv6 multicast hop count on %s to %d\n", transport->transportEndpoint, value);

        if (setsockopt(transport->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
               &value, sizeof(value)) < 0) {
                CCK_PERROR("Failed to set IPv6 multicast hop count");
                return CCK_FALSE;
        }

        return CCK_TRUE;
}

int
cckTransportInit_ipv6 (CckTransport* transport, const CckTransportConfig* config)
{
    return 1;
}

int
cckTransportShutdown_ipv6 (void* _transport)
{

    CckTransport* transport = (CckTransport*)_transport;

    if(transport->transportData != NULL)
	free(transport->transportData);

    return 0;

}

static CckBool
cckTransportTestConfig_ipv6 (CckTransport* transport, const CckTransportConfig* config)
{

    return CCK_TRUE;

}

static int
cckTransportPushConfig_ipv6 (CckTransport* transport, const CckTransportConfig* config)
{

    return 1;

}

static void
cckTransportRefresh_ipv6 (CckTransport* transport)
{

    return;

}

static CckBool
cckTransportHasData_ipv6 (CckTransport* transport)
{

    return CCK_FALSE;

}

static CckBool
cckTransportIsMulticastAddress_ipv6 (const TransportAddress* address)
{

    /* First byte is FF - multicast */
    if((unsigned char)(address->inetAddr6.sin6_addr.s6_addr[0]) == 0xFF)
	return CCK_TRUE;
    else
	return CCK_FALSE;

}

static ssize_t
cckTransportSend_ipv6 (CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* dst, CckTimestamp* timestamp)
{

    return 0;

}

static ssize_t
cckTransportRecv_ipv6 (CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* src, CckTimestamp* timestamp, int flags)
{

    return 0;

}

static CckBool
cckTransportAddressFromString_ipv6 (const char* addrStr, TransportAddress* out)
{

    CckBool ret = CCK_FALSE;
    int res = 0;
    struct addrinfo hints, *info;

    clearTransportAddress(out);
    memset(&hints, 0, sizeof(hints));

    hints.ai_family=AF_INET6;
    hints.ai_socktype=SOCK_DGRAM;

    res = getaddrinfo(addrStr, NULL, &hints, &info);

    if(res == 0) {

        ret = CCK_TRUE;
        memcpy(out, info->ai_addr,
    	    sizeof(struct sockaddr));

    } else {

        CCK_ERROR("Could not encode / resolve IPV6 address %s : %s\n", addrStr, gai_strerror(res));

    }

    freeaddrinfo(info);
    return ret;


}

static char*
cckTransportAddressToString_ipv6 (const TransportAddress* address)
{
    /* just like *toa: convenient but not re-entrant */
    static char buf[INET6_ADDRSTRLEN + 1];

    memset(buf, 0, INET6_ADDRSTRLEN + 1);

    if(inet_ntop(AF_INET6, &address->inetAddr6.sin6_addr, buf, INET6_ADDRSTRLEN) == NULL)
        return "-";

    return buf;

}


static CckBool
cckTransportAddressEqual_ipv6 (const TransportAddress* a, const TransportAddress* b)
{

	if(!memcmp(&a->inetAddr6.sin6_addr.s6_addr, &b->inetAddr6.sin6_addr.s6_addr, 16))
	    return CCK_TRUE;
	else
	    return CCK_FALSE;

}
