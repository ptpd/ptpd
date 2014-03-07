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
 * @file   cck_transport_pcap_ethernet.c
 * 
 * @brief  libCCK ethernet transport implementation
 *
 */

#include "cck_transport_pcap_ethernet.h"

#define CCK_THIS_TYPE CCK_TRANSPORT_PCAP_ETHERNET

/* interface (public) method definitions */
static int     cckTransportInit(CckTransport* transport, const CckTransportConfig* config);
static int     cckTransportShutdown(void* _transport);
static int     cckTransportPushConfig(CckTransport* transport, const CckTransportConfig* config);
static CckBool cckTransportTestConfig(CckTransport* transport, const CckTransportConfig* config);
static void    cckTransportRefresh(CckTransport* transport);
static CckBool cckTransportHasData(CckTransport* transport);
static CckBool cckTransportIsMulticastAddress(const TransportAddress* address);
static ssize_t cckTransportSend(CckTransport* transport, CckOctet* buf, CckUInt16 size, TransportAddress* dst, CckTimestamp* timestamp);
static ssize_t cckTransportRecv(CckTransport* transport, CckOctet* buf, CckUInt16 size, TransportAddress* src, CckTimestamp* timestamp, int flags);
static CckBool cckTransportAddressFromString(const char*, TransportAddress* out);
static char*   cckTransportAddressToString(const TransportAddress* address);
static CckBool cckTransportAddressEqual(const TransportAddress* a, const TransportAddress* b);

/* private method definitions (if any) */

/* implementations follow */

void
cckTransportSetup_pcap_ethernet (CckTransport* transport)
{
	if(transport->transportType == CCK_THIS_TYPE) {

	    transport->aclType = CCK_ACL_ETHERNET;

	    transport->unicastCallback = NULL;
	    transport->init = cckTransportInit;
	    transport->shutdown = cckTransportShutdown;
	    transport->header.shutdown = cckTransportShutdown;
	    transport->testConfig = cckTransportTestConfig;
	    transport->pushConfig = cckTransportPushConfig;
	    transport->refresh = cckTransportRefresh;
	    transport->hasData = cckTransportHasData;
	    transport->isMulticastAddress = cckTransportIsMulticastAddress;
	    transport->send = cckTransportSend;
	    transport->recv = cckTransportRecv;
	    transport->addressFromString = cckTransportAddressFromString;
	    transport->addressToString = cckTransportAddressToString;
	    transport->addressEqual = cckTransportAddressEqual;
	} else {
	    CCK_ERROR("setup() called for incorrect transport: %02x, expected %02x\n",
			transport->transportType, CCK_THIS_TYPE);
	}
}

static CckBool
multicastJoin(CckTransport* transport, TransportAddress* mcastAddr)
{

    struct packet_mreq pmr;

    pmr.mr_ifindex = if_nametoindex(transport->transportEndpoint);
    pmr.mr_type = PACKET_MR_MULTICAST;
    pmr.mr_alen = ETHER_ADDR_LEN;
    memcpy(&pmr.mr_address, &mcastAddr->etherAddr, ETHER_ADDR_LEN);

    /* join multicast group on specified interface */
    if (setsockopt(transport->fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
           &pmr, sizeof(struct packet_mreq)) < 0) {
	CCK_PERROR("failed to join multicast address %s: ", transport->addressToString(mcastAddr));
        return CCK_FALSE;
    }

    CCK_DBGV("Joined ethernet multicast group %s on %s\n", transport->addressToString(mcastAddr),
						    transport->transportEndpoint);

    return CCK_TRUE;

}

static CckBool
multicastLeave(CckTransport* transport, TransportAddress* mcastAddr)
{

    struct packet_mreq pmr;

    pmr.mr_ifindex = if_nametoindex(transport->transportEndpoint);
    pmr.mr_type = PACKET_MR_MULTICAST;
    pmr.mr_alen = ETHER_ADDR_LEN;
    memcpy(&pmr.mr_address, &mcastAddr->etherAddr, ETHER_ADDR_LEN);

    /* leave multicast group on specified interface */
    if (setsockopt(transport->fd, SOL_PACKET, PACKET_DROP_MEMBERSHIP,
           &pmr, sizeof(struct packet_mreq)) < 0) {
	CCK_PERROR("failed to leave multicast address %s: ", transport->addressToString(mcastAddr));
        return CCK_FALSE;
    }

    CCK_DBGV("Left ethernet multicast group %s on %s\n", transport->addressToString(mcastAddr),
						    transport->transportEndpoint);

    return CCK_TRUE;

}

static int
cckTransportInit(CckTransport* transport, const CckTransportConfig* config)
{

    struct bpf_program program;
    char errbuf[PCAP_ERRBUF_SIZE];

    CckPcapEndpoint* handle = NULL;

    if(transport == NULL) {
	CCK_ERROR("Ethernet transport init called for an empty transport\n");
	return -1;
    }

    if(config == NULL) {
	CCK_ERROR("Ethernet transport init called with empty config\n");
	return -1;
    }

    /* Shutdown will be called on this object anyway, OK to exit without explicitly freeing this */
    CCKCALLOC(transport->transportData, sizeof(CckPcapEndpoint));
    handle = (CckPcapEndpoint*)transport->transportData;

    handle->bufSize = config->param1;

    transport->destinationId = config->destinationId;
    strncpy(transport->transportEndpoint, config->transportEndpoint, PATH_MAX);

    /* Get own address first - or find the desired one if config was populated with it*/
    if(!cckGetHwAddress(transport->transportEndpoint, &transport->ownAddress)) {
	    CCK_ERROR("No suitable Ethernet address found on %s - cannot start ethernet transport\n",
			    transport->transportEndpoint);
	    return -1;
    }

    CCK_DBG("own address: %s\n", transport->addressToString(&transport->ownAddress));

    memcpy(&transport->hardwareAddress, &transport->ownAddress, ETHER_ADDR_LEN);

    /* Assign default destinations */
    transport->defaultDestination = config->defaultDestination;
    transport->secondaryDestination = config->secondaryDestination;

    handle->receiver = pcap_open_live(transport->transportEndpoint, handle->bufSize , 1, 1, errbuf);

    if(handle->receiver == NULL) {
	CCK_PERROR("Could not open pcap reader on %s",
	transport->transportEndpoint);
	return -1;
    } else {
	CCK_DBG("Opened pcap reader endpoint for transport \"%s\"\n",
		    transport->header.instanceName);
    }

    handle->sender =  pcap_open_live(transport->transportEndpoint, handle->bufSize , 1, 1, errbuf);

    if(handle->sender == NULL) {
	CCK_PERROR("Could not open pcap writer on %s",
	transport->transportEndpoint);
	return -1;
    } else {
	CCK_DBG("Opened pcap writer endpoint for transport \"%s\"\n",
		    transport->header.instanceName);
    }

    if (pcap_compile(handle->receiver, &program, "ether proto 0x88f7", 1, 0) < 0) {
	    CCK_PERROR("Could not compile pcap event filter for %s transport on %s",
			    transport->header.instanceName,
			    transport->transportEndpoint);
	    pcap_perror(handle->receiver, transport->transportEndpoint);
	    return -1;
    }

    if (pcap_setfilter(handle->receiver, &program) < 0) {
	    CCK_PERROR("Could not set pcap filter for %s transport on %s",
			    transport->header.instanceName,
			    transport->transportEndpoint);
	    pcap_perror(handle->receiver, transport->transportEndpoint);
	    return -1;
    }

    pcap_freecode(&program);

    if ((transport->fd = pcap_get_selectable_fd(handle->receiver)) < 0) {
	    CCK_PERROR("Could not get pcap file descriptor for %s transport on %s",
			    transport->header.instanceName,
			    transport->transportEndpoint);
	    pcap_perror(handle->receiver, transport->transportEndpoint);
	    return -1;
    }


    /* One of our destinations is multicast - begin multicast initialisation */
    if(transport->isMulticastAddress(&transport->defaultDestination) ||
	transport->isMulticastAddress(&transport->secondaryDestination)) {

	/* ====== MCAST_INIT_BEGIN ======= */
/*
	if(transport->isMulticastAddress(&transport->defaultDestination) &&
	    !multicastJoin(transport, &transport->defaultDestination)) {
	    return -1;
	}

	if(transport->isMulticastAddress(&transport->secondaryDestination) &&
	    !multicastJoin(transport, &transport->secondaryDestination)) {
	    return -1;
	}
*/
	/* ====== MCAST_INIT_END ======= */

    } 

    /* everything succeeded, so we can add the transport's fd to the fd set */
    cckAddFd(transport->fd, transport->watcher);

    CCK_DBG("Successfully started pcap transport serial %08x (%s) on endpoint %s\n",
		transport->header.serial, transport->header.instanceName,
		transport->transportEndpoint);

    return 1;

}

static int
cckTransportShutdown(void* _transport)
{

    if(_transport == NULL)
	return -1;

    CckTransport* transport = (CckTransport*)_transport;

    CckPcapEndpoint* handle = NULL;

    if(transport->isMulticastAddress(&transport->defaultDestination) &&
	    !multicastLeave(transport, &transport->defaultDestination)) {
	CCK_DBG("Error while leaving ethernet multicast group %s\n",
		    transport->addressToString(&transport->defaultDestination));
    }

    if(transport->isMulticastAddress(&transport->secondaryDestination) &&
	    !multicastLeave(transport, &transport->secondaryDestination)) {
	CCK_DBG("Error while leaving ethernet multicast group %s\n",
		    transport->addressToString(&transport->secondaryDestination));
    }

    if(transport->transportData != NULL) {
	handle = (CckPcapEndpoint*)transport->transportData;
	pcap_close(handle->sender);
	pcap_close(handle->receiver);
    }

    /* remove fd from watcher set */
    cckRemoveFd(transport->fd, transport->watcher);

    /* close the FD */
    close(transport->fd);

    clearCckTransportCounters(transport);

    if(transport->transportData != NULL) {
	free(transport->transportData);
	transport->transportData = NULL;
    }

    return 1;

}

static CckBool
cckTransportTestConfig(CckTransport* transport, const CckTransportConfig* config)
{

    int res = 0;
    unsigned int flags;
return CCK_TRUE;
    CckIpv4TransportData* data = NULL;

    if(transport == NULL) {
	CCK_ERROR("transport testConfig called for an empty transport\n");
	return CCK_FALSE;
    }

    if(config == NULL) {
	CCK_ERROR("transport testCpmfog called with empty config\n");
	return CCK_FALSE;
    }

    strncpy(transport->transportEndpoint, config->transportEndpoint, PATH_MAX);

    /* Get own address first */
    res = cckGetInterfaceAddress(transport, &transport->ownAddress, AF_INET);

    if(res != 1) {
	return CCK_FALSE;
    }

    CCK_DBG("own address: %s\n", transport->addressToString(&transport->ownAddress));

    res = cckGetInterfaceFlags(transport->transportEndpoint, &flags);

    if(res != 1) {
	CCK_ERROR("Could not query interface flags for %s\n",
		    transport->transportEndpoint);
	return CCK_FALSE;
    }

     if(!(flags & IFF_UP) || !(flags & IFF_RUNNING))
            CCK_WARNING("Interface %s seems to be down. Transport may not operate correctly until it's up.\n", 
		    transport->transportEndpoint);

    if(flags & IFF_LOOPBACK)
            CCK_WARNING("Interface %s is a loopback interface.\n", 
			transport->transportEndpoint);

    /* One of our destinations is multicast - test multicast operation */
    if(transport->isMulticastAddress(&config->defaultDestination) ||
	transport->isMulticastAddress(&config->secondaryDestination)) {
	    if(!(flags & IFF_MULTICAST)) {
        	CCK_WARNING("Interface %s is not multicast capable.\n",
		transport->transportEndpoint);
	    }
    }

    CCKCALLOC(data, sizeof(CckIpv4TransportData));

    if(config->swTimestamping) {
	if(!cckInitSwTimestamping(
	    transport, &data->timestampCaps, CCK_TRUE)) {
	    free(data);
	    return CCK_FALSE;
	}

    }

    free(data);
    return CCK_TRUE;

}

static int
cckTransportPushConfig(CckTransport* transport, const CckTransportConfig* config)
{

    CckTransport* testTransport = createCckTransport(transport->transportType, "pushConfigTest");

    if(!testTransport->testConfig(testTransport, config)) {
	CCK_ERROR("Could not apply new config to transport 0x%08x (\"%s\")\n",
		    transport->header.serial, transport->header.instanceName);
	return -1;
    }

    freeCckTransport(&testTransport);

    clearCckTransportCounters(transport);

    return transport->init(transport, config);

}

/*
    re-join multicast groups. don't leave, just join, so no unnecessary
    PIM / IGMP snooping movement is generated.
*/
static void
cckTransportRefresh(CckTransport* transport)
{

return;

    if(transport->isMulticastAddress(&transport->defaultDestination)) {
	(void)multicastJoin(transport, &transport->defaultDestination);
    }

    if(transport->isMulticastAddress(&transport->secondaryDestination)) {
	(void)multicastJoin(transport, &transport->secondaryDestination);
    }

}

static CckBool
cckTransportHasData(CckTransport* transport)
{

    if(transport->watcher == NULL)
	return CCK_FALSE;

    if(FD_ISSET(transport->fd, &transport->watcher->workReadFdSet))
	return CCK_TRUE;

    return CCK_FALSE;

}

static CckBool
cckTransportIsMulticastAddress(const TransportAddress* address)
{
    /* last bit of first byte is lit == multicast */
    if( (*(unsigned char*)address & (unsigned char)1) == 1)
        return CCK_TRUE;
    else
        return CCK_FALSE;
}

static ssize_t
cckTransportSend(CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* dst, CckTimestamp* timestamp)
{

    ssize_t ret;
    CckOctet etherData[ETHER_HDR_LEN + size];
    CckPcapEndpoint* handle = NULL;

    if(dst == NULL)
	dst = &transport->defaultDestination;

    memcpy(etherData, &dst->etherAddr, ETHER_ADDR_LEN);
    memcpy(etherData + ETHER_ADDR_LEN, &transport->ownAddress.etherAddr, ETHER_ADDR_LEN);
    *((short *)&etherData[2 * ETHER_ADDR_LEN]) = htons(transport->destinationId);
    memcpy(etherData + ETHER_HDR_LEN, buf, size);

    if(transport->transportData != NULL) {
        handle = (CckPcapEndpoint*)transport->transportData;
	ret = pcap_inject(handle->sender, etherData, ETHER_HDR_LEN + size);
    }

    fflush(NULL);

    if (ret <= 0)
           CCK_DBG("send() Error while sending pcap ethernet message on %s (transport \"%s\"): %s\n",
                        transport->transportEndpoint, transport->header.instanceName, strerror(errno));
    else
	transport->sentMessages++;

    return ret;

}

static ssize_t
cckTransportRecv(CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* src, CckTimestamp* timestamp, int flags)
{
    ssize_t ret = 0;

    CckUInt16 etherType = 0;
    CckPcapEndpoint* handle = NULL;
    struct pcap_pkthdr* header;
    const u_char* 	data;

    timestamp->seconds = 0;
    timestamp->nanoseconds = 0;

    if(transport->transportData != NULL) {

        handle = (CckPcapEndpoint*)transport->transportData;
	ret = pcap_next_ex(handle->receiver, &header, &data);

    }

    if (ret <= 0 ) {

	if (errno == EAGAIN || errno == EINTR) {
	    return 0;
	}

	CCK_DBG("Failed to receive pcap data for transport \"%s\" on %s: %s\n",
			transport->header.instanceName, transport->transportEndpoint,
			pcap_geterr(handle->receiver));
    }

    etherType = *(CckUInt16*)(data + 2 * ETHER_ADDR_LEN);

    if(ntohs(etherType) != transport->destinationId) {
	CCK_DBG("transport \"%s\" on %s Received Ethernet data with incorrect etherType: %04x - expected %04x\n",
		    transport->header.instanceName, transport->transportEndpoint, etherType, transport->destinationId);
	return 0;
    }

    ret = header->caplen - ETHER_HDR_LEN;

    /* if we have more data than buffer size, read only so much */
    if(ret > size) {

	ret = size;

    }

    memcpy(src, data, ETHER_ADDR_LEN);
    memcpy(buf, data + ETHER_HDR_LEN, ret);

    timestamp->seconds = header->ts.tv_sec;
    timestamp->nanoseconds = header->ts.tv_usec * 1000;

    CCK_DBGV("pcap ethernet transport \"%s\" on %s - got pcap timestamp %us.%dns\n",
		    transport->header.instanceName, transport->transportEndpoint,
		    timestamp->seconds, timestamp->nanoseconds
		    );

    fflush(NULL);

    return ret;

}

static CckBool
cckTransportAddressFromString(const char* addrStr, TransportAddress* out)
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
cckTransportAddressToString(const TransportAddress* address)
{

    return ether_ntoa(&address->etherAddr);

}

static CckBool
cckTransportAddressEqual(const TransportAddress* a, const TransportAddress* b)
{
    if(!memcmp(&a->etherAddr, &b->etherAddr, ETHER_ADDR_LEN))
        return CCK_TRUE;
    else
        return CCK_FALSE;
}

#undef CCK_THIS_TYPE
