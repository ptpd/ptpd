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
//#include "cck_transport.h"

CckTransport*
createCckTransport(int transportType, const char* instanceName)
{

    extern CckRegistry* cckRegistry;

    CckTransport* transport;

    CCKCALLOC(transport, sizeof(CckTransport));

    transport->transportType = transportType;

    strncpy(transport->header.instanceName, instanceName, CCK_MAX_INSTANCE_NAME_LEN);

    transport->header.componentType = CCK_COMPONENT_TRANSPORT;

/* 
   if you write a new transport,
   make sure you register it here. Format is:

   CCK_REGISTER_TRANSPORT(TRANSPORT_TYPE, function_name_suffix)

   where suffix is the string all function names are
   suffixed with for your implementation, i.e. you
   need to define cckTransportXXX_bob for your "bob" tansport.
   
*/

/* ============ TRANSPORT IMPLEMENTATIONS BEGIN ============ */

    CCK_REGISTER_TRANSPORT(CCK_TRANSPORT_NULL, null);
    CCK_REGISTER_TRANSPORT(CCK_TRANSPORT_UDP_IPV4, ipv4);
    CCK_REGISTER_TRANSPORT(CCK_TRANSPORT_UDP_IPV6, ipv6);
    CCK_REGISTER_TRANSPORT(CCK_TRANSPORT_ETHERNET, ethernet);

/* ============ TRANSPORT IMPLEMENTATIONS END ============== */

/* we 're done with this macro */
#undef CCK_REGISTER_TRANSPORT

   if(cckRegistry == NULL) {

        CCK_DBG("Will not register component - registry not initialised\n");

    } else {

	transport->header._next = NULL;
	transport->header._prev = NULL;
        cckRegister(transport);

    }

    return transport;
}


void
freeCckTransport(CckTransport** transport)
{

    if(*transport==NULL)
	    return;

    (*transport)->shutdown(*transport);

    cckDeregister(*transport);

    free(*transport);

    *transport = NULL;

}


/* Zero out TransportAddress structure */
void
clearTransportAddress(TransportAddress* address)
{

    memset(address, 0, sizeof(TransportAddress));

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

