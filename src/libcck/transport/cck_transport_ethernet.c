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
 * @file   cck_transport_ethernet.c
 * 
 * @brief  libCCK ethernet transport implementation
 *
 */

#include "cck_transport_ethernet.h"

static int	cckTransportInit_ethernet (CckTransport* transport, const CckTransportConfig* config);
static int	cckTransportShutdown_ethernet (void* _transport);
static CckBool cckTransportTestConfig_ethernet (CckTransport* transport, const CckTransportConfig* config);
static int     cckTransportPushConfig_ethernet (CckTransport* transport, const CckTransportConfig* config);
static void    cckTransportRefresh_ethernet (CckTransport* transport);
static CckBool cckTransportHasData_ethernet (CckTransport* transport);
static CckBool cckTransportIsMulticastAddress_ethernet (const TransportAddress* address);
static ssize_t cckTransportSend_ethernet (CckTransport* transport, CckOctet* buf, CckUInt16 size, TransportAddress* dst, CckTimestamp* timestamp);
static ssize_t cckTransportRecv_ethernet (CckTransport* transport, CckOctet* buf, CckUInt16 size, TransportAddress* src, CckTimestamp* timestamp, int flags);
static CckBool cckTransportAddressFromString_ethernet (const char*, TransportAddress* out);
static char*   cckTransportAddressToString_ethernet (const TransportAddress* address);
static CckBool cckTransportAddressEqual_ethernet (const TransportAddress* a, const TransportAddress* b);

void
cckTransportSetup_ethernet(CckTransport* transport)
{
    CCK_SETUP_TRANSPORT(CCK_TRANSPORT_ETHERNET, ethernet);
}

static int
cckTransportInit_ethernet (CckTransport* transport, const CckTransportConfig* config)
{
    return 1;
}

static int
cckTransportShutdown_ethernet (void* _transport)
{

    CckTransport* transport = (CckTransport*)_transport;

    if(transport->transportData != NULL)
	free(transport->transportData);

    return 0;

}

static CckBool
cckTransportTestConfig_ethernet (CckTransport* transport, const CckTransportConfig* config)
{

    return CCK_TRUE;

}

static int
cckTransportPushConfig_ethernet (CckTransport* transport, const CckTransportConfig* config)
{

    return 1;

}

static void
cckTransportRefresh_ethernet (CckTransport* transport)
{

    return;

}

static CckBool
cckTransportHasData_ethernet (CckTransport* transport)
{

    return CCK_FALSE;

}

static CckBool
cckTransportIsMulticastAddress_ethernet (const TransportAddress* address)
{

    /* last bit of first byte is lit == multicast */
    if( (*(unsigned char*)address & (unsigned char)1) == 1)
	return CCK_TRUE;
    else
	return CCK_FALSE;

}

static ssize_t
cckTransportSend_ethernet (CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* dst, CckTimestamp* timestamp)
{

    return 0;

}

static ssize_t
cckTransportRecv_ethernet (CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* src, CckTimestamp* timestamp, int flags)
{

    return 0;

}

static CckBool
cckTransportAddressFromString_ethernet (const char* addrStr, TransportAddress* out)
{

    struct ether_addr* eth;

    clearTransportAddress(out);

    if(!ether_hostton(addrStr, (struct ether_addr*)out)) {
	return CCK_TRUE;
    }

    if((eth=ether_aton(addrStr)) != NULL) {
	memcpy(out, eth,
	    sizeof(struct ether_addr));
	return CCK_TRUE;
    }

    CCK_PERROR("Could not resolve / encode Ethernet address: %s",
		addrStr);

    return CCK_FALSE;

}

static char*
cckTransportAddressToString_ethernet (const TransportAddress* address)
{

    return ether_ntoa(&address->etherAddr);

}

static CckBool
cckTransportAddressEqual_ethernet (const TransportAddress* a, const TransportAddress* b)
{

	if(!memcmp(&a->etherAddr, &b->etherAddr, ETHER_ADDR_LEN))
	    return CCK_TRUE;
	else
	    return CCK_FALSE;
}
