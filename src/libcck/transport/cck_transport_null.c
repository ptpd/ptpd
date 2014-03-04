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
 * @file   cck_transport_null.c
 * 
 * @brief  libCCK null transport implementation
 *
 */

#include "cck_transport_null.h"

static int      cckTransportInit_null (CckTransport* transport, const CckTransportConfig* config);
static int      cckTransportShutdown_null (void* component);
static CckBool cckTransportTestConfig_null (CckTransport* transport, const CckTransportConfig* config);
static int     cckTransportPushConfig_null (CckTransport* transport, const CckTransportConfig* config);
static void    cckTransportRefresh_null (CckTransport* transport);
static CckBool cckTransportHasData_null (CckTransport* transport);
static CckBool cckTransportIsMulticastAddress_null (const TransportAddress* address);
static ssize_t cckTransportSend_null (CckTransport* transport, CckOctet* buf, CckUInt16 size, TransportAddress* dst, CckTimestamp* timestamp);
static ssize_t cckTransportRecv_null (CckTransport* transport, CckOctet* buf, CckUInt16 size, TransportAddress* src, CckTimestamp* timestamp, int flags);
static CckBool cckTransportAddressFromString_null (const char*, TransportAddress* out);
static char*    cckTransportAddressToString_null (const TransportAddress* address);
static CckBool cckTransportAddressEqual_null (const TransportAddress* a, const TransportAddress* b);

void
cckTransportSetup_null(CckTransport* transport)
{
    CCK_SETUP_TRANSPORT(CCK_TRANSPORT_NULL, null);
}

static int
cckTransportInit_null (CckTransport* transport, const CckTransportConfig* config)
{
    return 1;
}

static int
cckTransportShutdown_null (void* transport)
{

    return 0;

}

static CckBool
cckTransportTestConfig_null (CckTransport* transport, const CckTransportConfig* config)
{

    return CCK_TRUE;

}

static int
cckTransportPushConfig_null (CckTransport* transport, const CckTransportConfig* config)
{

    return 1;

}

static void
cckTransportRefresh_null (CckTransport* transport)
{

    return;

}

static CckBool
cckTransportHasData_null (CckTransport* transport)
{

    return CCK_FALSE;

}

static CckBool
cckTransportIsMulticastAddress_null (const TransportAddress* address)
{

    return CCK_FALSE;

}

static ssize_t
cckTransportSend_null (CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* dst, CckTimestamp* timestamp)
{

    return 0;

}

static ssize_t
cckTransportRecv_null (CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* src, CckTimestamp* timestamp, int flags)
{

    return 0;

}

static CckBool
cckTransportAddressFromString_null (const char* addrStr, TransportAddress* out)
{
    return CCK_TRUE;
}

static char*
cckTransportAddressToString_null (const TransportAddress* address)
{
    return "-";
}

static CckBool
cckTransportAddressEqual_null (const TransportAddress* a, const TransportAddress* b)
{

    return CCK_TRUE;

}
