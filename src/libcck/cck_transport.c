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
 * @file   cck_transport.c
 *
 * @brief  constructor and destructor for transport component, additional helper functions
 *
 */

#include "cck.h"

/* Quick shortcut to call the setup function for different implementations */
#define CCK_REGISTER_TRANSPORT(type,suffix) \
	if(transportType==type) {\
	    cckTransportSetup_##suffix(transport);\
	    done = CCK_TRUE;\
	}


CckTransport*
createCckTransport(int transportType, const char* instanceName)
{

    CckTransport* transport;

    CCKCALLOC(transport, sizeof(CckTransport));

    transport->header._next = NULL;
    transport->header._prev = NULL;
    transport->header._dynamic = CCK_TRUE;

    setupCckTransport(transport, transportType, instanceName);

    return transport;
}

void
setupCckTransport(CckTransport* transport, int transportType, const char* instanceName)
{

    CckBool done = CCK_FALSE;

    transport->transportType = transportType;

    strncpy(transport->header.instanceName, instanceName, CCK_MAX_INSTANCE_NAME_LEN);

    transport->header.componentType = CCK_COMPONENT_TRANSPORT;

/*
   if you write a new transport,
   make sure you register it here. Format is:

   CCK_REGISTER_TRANSPORT(TRANSPORT_TYPE, function_name_suffix)

   where suffix is the string the setup() function name is
   suffixed with for your implementation, i.e. you
   need to define cckTransportSetup_bob for your "bob" tansport.

*/

/* ============ TRANSPORT IMPLEMENTATIONS BEGIN ============ */

    CCK_REGISTER_TRANSPORT(CCK_TRANSPORT_NULL, null);
    CCK_REGISTER_TRANSPORT(CCK_TRANSPORT_SOCKET_UDP_IPV4, socket_ipv4);
    CCK_REGISTER_TRANSPORT(CCK_TRANSPORT_SOCKET_UDP_IPV6, socket_ipv6);
#ifdef PTPD_PCAP
    CCK_REGISTER_TRANSPORT(CCK_TRANSPORT_PCAP_UDP_IPV4, pcap_ipv4);
    CCK_REGISTER_TRANSPORT(CCK_TRANSPORT_PCAP_UDP_IPV6, pcap_ipv6);
    CCK_REGISTER_TRANSPORT(CCK_TRANSPORT_PCAP_ETHERNET, pcap_ethernet);
#endif

/* ============ TRANSPORT IMPLEMENTATIONS END ============== */


    if(!done) {
	CCK_REGISTER_TRANSPORT(transportType, null);
	CCK_ERROR("Attempt to create unknown transport %02x - aborting\n",
		    transportType);
	abort();
    }

/* we 're done with this macro */
#undef CCK_REGISTER_TRANSPORT


    transport->header._next = NULL;
    transport->header._prev = NULL;
    cckRegister(transport);

}


void
freeCckTransport(CckTransport** transport)
{

    if(transport==NULL)
	    return;

    if(*transport==NULL)
	    return;

    (*transport)->shutdown(*transport);

    cckDeregister(*transport);

    if((*transport)->header._dynamic) {

	free(*transport);
	*transport = NULL;

    }

}


/* Zero out TransportAddress structure */
void
clearTransportAddress(TransportAddress* address)
{

    memset(address, 0, sizeof(TransportAddress));

}


/* Zero out TransportAddress structure */
void
clearCckTransportCounters(CckTransport* transport)
{
    transport->sentMessages = 0;
    transport->receivedMessages = 0;
}

/* Check if TransportAddress is empty */
CckBool
transportAddressEmpty(const TransportAddress* addr)
{

    CckUInt16 addr_ = *(CckUInt16*)addr;

    if(!addr_)
        return CCK_TRUE;

    return CCK_FALSE;

}
