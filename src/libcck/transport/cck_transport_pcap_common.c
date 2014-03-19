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
 * @file   cck_transport_pcap_common.c
 * 
 * @brief  LibCCK transport helper functions
 *
 */

#include "cck_transport_pcap_common.h"

int
cckPcapGetInterfaceAddress(const CckTransport* transport, TransportAddress* addr, int addressFamily)
{
    int ret;
    char errbuf[PCAP_ERRBUF_SIZE];
    CckBool found = CCK_FALSE;

    pcap_if_t *pall, *pdev;

    pcap_addr_t *paddr;

    if(pcap_findalldevs(&pall, errbuf) == -1) {
	CCK_PERROR("pcap_findalldevs: could not get interface list: %s",
	errbuf);
	ret = -1;
	goto end;

    }

    for (pdev = pall; pdev != NULL; pdev = pdev->next) {

	CCK_DBGV("Interface: %s desc: %s\n",
		    pdev->name, pdev->description);

	if(!strcmp(transport->transportEndpoint, pdev->name))  {

	    /* interface may exist but not have the address type we're looking for */
	    found = CCK_TRUE;

	    for(paddr = pdev->addresses; paddr != NULL; paddr = paddr->next) {

		if(paddr->addr->sa_family == addressFamily) {

		    /* If address was specified on input, find that address, otherwise get the first one */
		    if(!transportAddressEmpty(addr) &&
			!transport->addressEqual(addr, (TransportAddress*)paddr->addr))
			    continue;

		    /* If we have no output pointer, we won't copy but we'll still return true */
		    if(addr!=NULL)
			memcpy(addr, paddr->addr, (addressFamily == AF_INET6) ? 
						sizeof(struct sockaddr_in6) :
						sizeof(struct sockaddr_in));
    		    ret = 1;
    		    goto end;
		}

	    }
	}

    }

    ret = 0;

    if(!transportAddressEmpty(addr)) {
	CCK_ERROR("Could not find address %s on interface %s\n",
			transport->addressToString(addr),
			transport->transportEndpoint);
    } else if(found) {
	CCK_ERROR("Could not find %saddress on interface %s\n",
		    addressFamily == AF_INET ? "IPv4 " :
		    addressFamily == AF_INET6 ? "IPv6 " : "",
		    transport->transportEndpoint);
    } else {
	CCK_ERROR("Interface not found: %s\n", transport->transportEndpoint);
    }

end:

    pcap_freealldevs(pall);
    return ret;
}

