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

    struct ifreq ifr;
    int fd;

    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	CCK_DBG("Failed to open test socket when adding ethernet multicast membership %s on %s: %s\n",
		transport->addressToString(mcastAddr), transport->transportEndpoint, strerror(errno));
        return CCK_FALSE;
    }

    memset(&ifr, 0, sizeof(struct ifreq));

    strncpy(ifr.ifr_name, transport->transportEndpoint, IFNAMSIZ -1);
    memcpy(&ifr.ifr_hwaddr.sa_data, mcastAddr, ETHER_ADDR_LEN);

    /* join multicast group on specified interface */
    if ((ioctl(fd, SIOCADDMULTI, &ifr)) < 0 ) {
	CCK_DBG("Failed to add ethernet multicast membership %s on  %s: %s\n",
		transport->addressToString(mcastAddr), transport->transportEndpoint, strerror(errno));
	close(fd);
        return CCK_FALSE;
    }

    CCK_DBG("Added ethernet membership %s on %s\n", transport->addressToString(mcastAddr),
						    transport->transportEndpoint);
    close(fd);
    return CCK_TRUE;

}

static CckBool
multicastLeave(CckTransport* transport, TransportAddress* mcastAddr)
{

    struct ifreq ifr;
    int fd;

    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	CCK_DBG("Failed to open test socket when adding ethernet multicast membership %s on %s: %s\n",
		transport->addressToString(mcastAddr), transport->transportEndpoint, strerror(errno));
        return CCK_FALSE;
    }

    memset(&ifr, 0, sizeof(struct ifreq));

    strncpy(ifr.ifr_name, transport->transportEndpoint, IFNAMSIZ -1);
    memcpy(&ifr.ifr_hwaddr.sa_data, mcastAddr, ETHER_ADDR_LEN);

    /* leave multicast group on specified interface */
    if ((ioctl(fd, SIOCDELMULTI, &ifr)) < 0 ) {
	CCK_DBG("Failed to drop ethernet multicast membership %s on  %s: %s\n",
		transport->addressToString(mcastAddr), transport->transportEndpoint, strerror(errno));
	close(fd);
        return CCK_FALSE;
    }

    CCK_DBG("Dropped ethernet membership %s on %s\n", transport->addressToString(mcastAddr),
						    transport->transportEndpoint);
    close(fd);
    return CCK_TRUE;

}

static int
cckTransportInit(CckTransport* transport, const CckTransportConfig* config)
{

    struct bpf_program program;
    char   errbuf[PCAP_ERRBUF_SIZE];
    char   filter[PATH_MAX];
    int    promisc = 0;
    int	   len = 0;

    memset(filter, 0, PATH_MAX);

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

    handle->bufSize = config->param3;

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

    /*
       One of our destinations is multicast - try adding memberships,
       go promisc if failed.
    */

    if(transport->isMulticastAddress(&transport->defaultDestination) &&
        !multicastJoin(transport, &transport->defaultDestination)) {
	CCK_DBGV("Could not add multicast membership - going promiscuous\n");
        promisc = 1;
    }

    if(transport->isMulticastAddress(&transport->secondaryDestination) &&
        !multicastJoin(transport, &transport->secondaryDestination)) {
	CCK_DBGV("Could not add multicast membership - going promiscuous\n");
        promisc = 1;
    }

    /* open the reader pcap handle */
    handle->receiver = pcap_open_live(transport->transportEndpoint, handle->bufSize , promisc, 1, errbuf);

    if(handle->receiver == NULL) {
	CCK_PERROR("Could not open pcap reader on %s",
	transport->transportEndpoint);
	return -1;
    } else {
	CCK_DBG("Opened pcap reader endpoint for transport \"%s\"\n",
		    transport->header.instanceName);
    }

    /* open the writer pcap handle */
    handle->sender =  pcap_open_live(transport->transportEndpoint, handle->bufSize , promisc, 1, errbuf);

    if(handle->sender == NULL) {
	CCK_PERROR("Could not open pcap writer on %s",
	transport->transportEndpoint);
	return -1;
    } else {
	CCK_DBG("Opened pcap writer endpoint for transport \"%s\"\n",
		    transport->header.instanceName);
    }

    /* format the pcap filter */
    len += snprintf(filter, PATH_MAX, "ether proto 0x%04x and (ether host %s ",
		transport->destinationId,
		transport->addressToString(&transport->ownAddress));

    if(!transportAddressEmpty(&transport->defaultDestination)) {
    len += snprintf(filter + len, PATH_MAX - len, "or ether host %s ",
		transport->addressToString(&transport->defaultDestination));
    }

    if(!transportAddressEmpty(&transport->secondaryDestination)) {
    len += snprintf(filter + len, PATH_MAX - len, "or ether host %s ",
		transport->addressToString(&transport->secondaryDestination));
    }

    len += snprintf(filter + len, PATH_MAX - len, ")");

    CCK_DBGV("pcap ethernet transport \"%s\" using filter: %s\n",
		transport->header.instanceName, filter);

    if (pcap_compile(handle->receiver, &program, filter, 1, 0) < 0) {
	    CCK_PERROR("Could not compile pcap filter for %s transport on %s",
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

    /* everything succeeded, so we can add the transport's fd to the fd set */
    cckAddFd(transport->fd, transport->watcher);

    if(config->hwTimestamping) {
	CCK_ERROR("libpcap based transport do not directly support hardware timestamping\n");
	return -1;
    }

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

    struct bpf_program program;
    char   errbuf[PCAP_ERRBUF_SIZE];
    char   filter[PATH_MAX];
    int    promisc = 0;
    int	   len = 0;

    memset(filter, 0, PATH_MAX);

    CckPcapEndpoint handle;

    if(transport == NULL) {
	CCK_ERROR("Ethernet transport test called for an empty transport\n");
	return CCK_FALSE;
    }

    if(config == NULL) {
	CCK_ERROR("Ethernet transport test called with empty config\n");
	return CCK_FALSE;
    }

    handle.bufSize = config->param3;

    transport->destinationId = config->destinationId;
    strncpy(transport->transportEndpoint, config->transportEndpoint, PATH_MAX);

    /* Get own address first - or find the desired one if config was populated with it*/
    if(!cckGetHwAddress(transport->transportEndpoint, &transport->ownAddress)) {
	    CCK_ERROR("No suitable Ethernet address found on %s - transport not usable\n",
			    transport->transportEndpoint);
	    CCK_FALSE;
    }

    CCK_DBG("own address: %s\n", transport->addressToString(&transport->ownAddress));

    memcpy(&transport->hardwareAddress, &transport->ownAddress, ETHER_ADDR_LEN);


    /* format the pcap filter */
    len += snprintf(filter, PATH_MAX, "ether proto 0x%04x and (ether host %s ",
		transport->destinationId,
		transport->addressToString(&transport->ownAddress));

    if(!transportAddressEmpty(&config->defaultDestination)) {
    len += snprintf(filter + len, PATH_MAX - len, "or ether host %s ",
		transport->addressToString(&config->defaultDestination));
    }

    if(!transportAddressEmpty(&config->secondaryDestination)) {
    len += snprintf(filter + len, PATH_MAX - len, "or ether host %s ",
		transport->addressToString(&config->secondaryDestination));
    }

    len += snprintf(filter + len, PATH_MAX - len, ")");

    CCK_DBGV("pcap ethernet transport \"%s\" test using filter: %s\n",
		transport->header.instanceName, filter);

    /* open the reader pcap handle */
    handle.receiver = pcap_open_live(transport->transportEndpoint, handle.bufSize , promisc, 1, errbuf);

    if(handle.receiver == NULL) {
	CCK_PERROR("Could not open test pcap reader on %s",
	transport->transportEndpoint);
	return CCK_FALSE;
    } else {
	CCK_DBG("pcap reader endpoint for transport \"%s\" OK\n",
		    transport->header.instanceName);
    }

    if (pcap_compile(handle.receiver, &program, filter, 1, 0) < 0) {
	    CCK_PERROR("Could not compile pcap test filter for %s transport on %s",
			    transport->header.instanceName,
			    transport->transportEndpoint);
	    pcap_perror(handle.receiver, transport->transportEndpoint);
	    pcap_close(handle.receiver);
	    pcap_freecode(&program);
	    return CCK_FALSE;
    }

    pcap_freecode(&program);

    /* open the writer pcap handle */
    handle.sender =  pcap_open_live(transport->transportEndpoint, handle.bufSize , promisc, 1, errbuf);

    if(handle.sender == NULL) {
	CCK_PERROR("Could not open pcap writer on %s",
	transport->transportEndpoint);
	return CCK_FALSE;
    } else {
	CCK_DBG("pcap writer endpoint for transport \"%s\" OK\n",
		    transport->header.instanceName);
	pcap_close(handle.sender);
    }

    if(config->hwTimestamping) {
	CCK_ERROR("libpcap based transport do not directly support hardware timestamping\n");
	return -1;
    }

    return 1;

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

    ssize_t ret = 0;
    CckOctet etherData[ETHER_HDR_LEN + size];
    CckPcapEndpoint* handle = NULL;

    if(dst == NULL)
	dst = &transport->defaultDestination;

    /* first 6 bytes - ethernet destination */
    memcpy(etherData, &dst->etherAddr, ETHER_ADDR_LEN);
    /* next 6 bytes - my own ethernet address */
    memcpy(etherData + ETHER_ADDR_LEN, &transport->ownAddress.etherAddr, ETHER_ADDR_LEN);
    /* next 2 bytes - ethertype */
    *((short *)&etherData[2 * ETHER_ADDR_LEN]) = htons(transport->destinationId);

    /* rest - data */
    memcpy(etherData + ETHER_HDR_LEN, buf, size);

    /* retrieve the pcap sender handle and send data */
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

    /* clear the timestamp, so it's empty if we didn't get one */
    timestamp->seconds = 0;
    timestamp->nanoseconds = 0;

    if(transport->transportData != NULL) {

        handle = (CckPcapEndpoint*)transport->transportData;
	ret = pcap_next_ex(handle->receiver, &header, &data);

    }

    if (ret <= 0 ) {

	/* could we have been interrupted by a signal? */
	if (errno == EAGAIN || errno == EINTR) {
	    return 0;
	}

	CCK_DBG("Failed to receive pcap data for transport \"%s\" on %s: %s\n",
			transport->header.instanceName, transport->transportEndpoint,
			pcap_geterr(handle->receiver));
    }

    etherType = *(CckUInt16*)(data + 2 * ETHER_ADDR_LEN);

    /* should not happen! */
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

    /* retrieve source address */
    memcpy(src, data, ETHER_ADDR_LEN);
    /* retrieve data */
    memcpy(buf, data + ETHER_HDR_LEN, ret);

    /* get the timestamp */
    timestamp->seconds = header->ts.tv_sec;
    timestamp->nanoseconds = header->ts.tv_usec * 1000;

    CCK_DBGV("pcap ethernet transport \"%s\" on %s - got pcap timestamp %us.%dns\n",
		    transport->header.instanceName, transport->transportEndpoint,
		    timestamp->seconds, timestamp->nanoseconds
		    );
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

/* nicer looking than ether_ntoa, but also non-reentrant */
static char*
cckTransportAddressToString(const TransportAddress* address)
{

    /* 'XX:' times six - last ':' is null termination */
    static char buf[3 * ETHER_ADDR_LEN];
    CckUInt8* addr = (CckUInt8*)&address->etherAddr;

    snprintf(buf, 3 * ETHER_ADDR_LEN, "%02x:%02x:%02x:%02x:%02x:%02x",
    *(addr), *(addr+1), *(addr+2), *(addr+3), *(addr+4), *(addr+5));

    return buf;

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
