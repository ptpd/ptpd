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
 * @file   cck_transport_pcap_ipv4.c
 *
 * @brief  libCCK ipv4 transport implementation
 *
 */

#include "cck_transport_pcap_ipv4.h"

#define CCK_THIS_TYPE CCK_TRANSPORT_PCAP_UDP_IPV4

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
cckTransportSetup_pcap_ipv4 (CckTransport* transport)
{
	if(transport->transportType == CCK_THIS_TYPE) {

	    transport->aclType = CCK_ACL_IPV4;

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
multicastJoin(CckTransport* transport, int fd, TransportAddress* mcastAddr)
{

    struct ip_mreq imr;

    imr.imr_multiaddr.s_addr = mcastAddr->inetAddr4.sin_addr.s_addr;
    imr.imr_interface.s_addr = transport->ownAddress.inetAddr4.sin_addr.s_addr;

    /* set multicast outbound interface */
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF,
           &imr.imr_interface, sizeof(struct in_addr)) < 0) {
        CCK_PERROR("failed to set multicast outbound interface");
        return CCK_FALSE;
    }

    /* drop multicast group on specified interface first */
    (void)setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
           &imr, sizeof(struct ip_mreq));

    /* join multicast group on specified interface */
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
           &imr, sizeof(struct ip_mreq)) < 0) {
	    CCK_PERROR("failed to join multicast address %s: ", transport->addressToString(mcastAddr));
		    return CCK_FALSE;
    }

    CCK_DBGV("Joined IPV4 multicast group %s on %s\n", transport->addressToString(mcastAddr),
						    transport->transportEndpoint);

    return CCK_TRUE;

}

static CckBool
multicastLeave(CckTransport* transport, int fd, TransportAddress* mcastAddr)
{

    struct ip_mreq imr;

    imr.imr_multiaddr.s_addr = mcastAddr->inetAddr4.sin_addr.s_addr;
    imr.imr_interface.s_addr = transport->ownAddress.inetAddr4.sin_addr.s_addr;

    /* leave multicast group on specified interface */
    if (setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
           &imr, sizeof(struct ip_mreq)) < 0) {
	    CCK_PERROR("failed to leave multicast group %s: ", transport->addressToString(mcastAddr));
		    return CCK_FALSE;
    }

    CCK_DBG("Sent IPv4 multicast leave for %s on %s (transport %s)\n", transport->addressToString(mcastAddr),
								transport->transportEndpoint,
								transport->header.instanceName);

    return CCK_TRUE;

}

static CckBool
setMulticastTtl(CckTransport* transport, int fd, int _value)
{

	CckUInt8 value = _value;

	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
	       &value, sizeof(value)) < 0) {
		CCK_PERROR("Failed to set IPv4 multicast TTL to %d on %s",
			value, transport->transportEndpoint);
		return CCK_FALSE;
	}

	CCK_DBG("Set IPv4 multicast TTL on %s to %d\n", transport->transportEndpoint, value);

	return CCK_TRUE;
}

/* Extract (source) TransportAddress from raw data payload */
static void getPayloadAddress(const CckOctet* buf, CckUInt16 len, int* offset, TransportAddress* out)
{

    /*
     * plenty of "magic" numbers used here but they are commented.
     * This is to avoid using headers like ip.h which may not exist everywhere
     */

    u_short etherType, headerLen;

    /* mah spoon is too small: 14 = 2 x MAC + 1 x EtherType*/
    if((len-*offset) < 14)
	return;

    /* 12 = 2 x MAC */
    etherType = ntohs(*(u_short *)(buf + 12 + *offset));

    /* IP header length - lower 4 bits of first byte of IP payload */
    headerLen = (*(u_short *)(buf + 14 + *offset)) & 0x0F;
    /* header length is number of 4-byte words */
    headerLen = headerLen * 4;

    clearTransportAddress(out);

    /* 0x800 == IP */
    if(etherType != 0x800) {
	CCK_DBG("Could not extract source addess from raw payload - not IP frame\n");
	return;
    }

    /* 30 = 14 Ethernet + 12 offset of source IP + 4 length of source IP */
    if((len-*offset) < 30)
        return;

    out->inetAddr4.sin_family = AF_INET;
    out->inetAddr4.sin_addr.s_addr = *(CckUInt32 *)(buf + 26 + *offset);

    /* 14 Ethernet + IP header length + 3rd-4th byte UDP */
    out->inetAddr4.sin_port = *(CckUInt16 *)(buf + 14 + headerLen + 2 + *offset);

    /* return payload offset: 14 Ethernet + IP header + 8 UDP */
    *offset = 14 + headerLen + 8;

}


static int
cckTransportInit(CckTransport* transport, const CckTransportConfig* config)
{
    int    res = 0;
    struct sockaddr_in* addr;
    struct bpf_program program;
    char   errbuf[PCAP_ERRBUF_SIZE];
    char   filter[PATH_MAX];
    int    len = 0;

    memset(filter, 0, PATH_MAX);

    CckPcapEndpoint* handle = NULL;

    if(transport == NULL) {
	CCK_ERROR("IPv4 pcap transport init called for an empty transport\n");
	return -1;
    }

    if(config == NULL) {
	CCK_ERROR("IPv4 pcap transport init called with empty config\n");
	return -1;
    }

    /* Shutdown will be called on this object anyway, OK to exit without explicitly freeing this */
    CCKCALLOC(transport->transportData, sizeof(CckPcapEndpoint));

    handle = (CckPcapEndpoint*)transport->transportData;

    handle->bufSize = config->param3;

    /* helper socket, needed so that we can listen on it and avoid unreachables */
    handle->socket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(handle->socket < 0) {
	CCK_PERROR("Could not create IPv4 helper socket for pcap transport");
	return -1;
    }

    strncpy(transport->transportEndpoint, config->transportEndpoint, PATH_MAX);

    transport->ownAddress = config->ownAddress;

    /* Get own address first - or find the desired one if config was populated with it*/
    res = cckPcapGetInterfaceAddress(transport, &transport->ownAddress, AF_INET);

    if(res != 1) {
	return res;
    }

    CCK_DBG("own address: %s\n", transport->addressToString(&transport->ownAddress));

    transport->ownAddress.inetAddr4.sin_port = htons(config->sourceId);

    clearTransportAddress(&transport->hardwareAddress);

    if(!cckGetHwAddress(transport->transportEndpoint, &transport->hardwareAddress)) {
	CCK_DBGV("No suitable hardware adddress found on %s\n",
			transport->transportEndpoint);
    } else {
	CCK_DBGV("%s hardware address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			transport->transportEndpoint,
			*((CckUInt8*)&transport->hardwareAddress),
			*((CckUInt8*)&transport->hardwareAddress + 1),
			*((CckUInt8*)&transport->hardwareAddress + 2),
			*((CckUInt8*)&transport->hardwareAddress + 3),
			*((CckUInt8*)&transport->hardwareAddress + 4),
			*((CckUInt8*)&transport->hardwareAddress + 5));
    }

    /* Assign default destinations */
    transport->defaultDestination = config->defaultDestination;
    transport->secondaryDestination = config->secondaryDestination;

    transport->defaultDestination.inetAddr4.sin_port = htons(config->destinationId);
    transport->secondaryDestination.inetAddr4.sin_port = htons(config->destinationId);


    /* One of our destinations is multicast - begin multicast initialisation */
    if(transport->isMulticastAddress(&transport->defaultDestination) ||
	transport->isMulticastAddress(&transport->secondaryDestination)) {

	/* ====== MCAST_INIT_BEGIN ======= */

	struct sockaddr_in sin;
        memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(config->sourceId);
	addr = &sin;

	if(transport->isMulticastAddress(&transport->defaultDestination) &&
	    !multicastJoin(transport, handle->socket, &transport->defaultDestination)) {
	    return -1;
	}

	if(transport->isMulticastAddress(&transport->secondaryDestination) &&
	    !multicastJoin(transport, handle->socket, &transport->secondaryDestination)) {
	    return -1;
	}


#if defined(linux) && defined(SO_BINDTODEVICE)
	if(setsockopt(handle->socket, SOL_SOCKET, SO_BINDTODEVICE,
		transport->transportEndpoint, strlen(transport->transportEndpoint)) < 0) {
	    CCK_DBG("Failed to set SO_BINDTODEVICE\n");
	}
#endif /* linux && SO_BINDTODEVICE */

	/* set multicast TTL */
	if(config->param1) {
	    if (setMulticastTtl(transport, handle->socket, config->param1)) {
	    } else {
		CCK_DBG("Failed to set IPv4 multicast TTL=%d on %s\n",
		    config->param1, transport->transportEndpoint);
	    }
	}

	/* ====== MCAST_INIT_END ======= */

    } else {
	addr = &transport->ownAddress.inetAddr4;
    }

    int tmp = 1;
    if(setsockopt(handle->socket, SOL_SOCKET, SO_REUSEADDR,
	&tmp, sizeof(int)) < 0) {
	    CCK_DBG("Failed to set SO_REUSEADDR\n");
    }

    if(bind(handle->socket, (struct sockaddr*)addr,
		    sizeof(struct sockaddr_in)) < 0) {
        CCK_PERROR("Failed to bind helper socket to %s:%d on %s",
			transport->addressToString(&transport->ownAddress),
			config->sourceId, transport->transportEndpoint);
	return -1;
    }

    CCK_DBG("Successfully bound helper socket to %s:%d on %s\n",
			transport->addressToString(&transport->ownAddress),
			config->sourceId, transport->transportEndpoint);

    /* set DSCP bits */
    if(config->param2 && (setsockopt(handle->socket, IPPROTO_IP, IP_TOS,
		&config->param2, sizeof(int)) < 0)) {
	CCK_DBG("Failed to set DSCP bits on %s\n",
		    transport->transportEndpoint);
    }

    if(config->swTimestamping) {
	/* pcap transport always timestamps on receive */
	transport->timestamping = CCK_TRUE;
    } else if(config->hwTimestamping) {
	CCK_ERROR("libpcap based transport do not directly support hardware timestamping\n");
	return -1;
    }

    /* open the reader pcap handle */
    handle->receiver = pcap_open_live(transport->transportEndpoint, handle->bufSize , 0, 1, errbuf);

    if(handle->receiver == NULL) {
	CCK_PERROR("Could not open pcap reader on %s",
	transport->transportEndpoint);
	return -1;
    } else {
	CCK_DBG("Opened pcap reader endpoint for transport \"%s\"\n",
		    transport->header.instanceName);
    }

    /* open the writer pcap handle */
    handle->sender =  pcap_open_live(transport->transportEndpoint, handle->bufSize , 0, 1, errbuf);

    if(handle->sender == NULL) {
	CCK_PERROR("Could not open pcap writer on %s",
	transport->transportEndpoint);
	return -1;
    } else {
	CCK_DBG("Opened pcap writer endpoint for transport \"%s\"\n",
		    transport->header.instanceName);
    }

    /* format the pcap filter */
    len += snprintf(filter, PATH_MAX, "ip and udp port %d and (host %s ",
		config->destinationId,
		transport->addressToString(&transport->ownAddress));

    if(!transportAddressEmpty(&transport->defaultDestination)) {
    len += snprintf(filter + len, PATH_MAX - len, "or host %s ",
		transport->addressToString(&transport->defaultDestination));
    }

    if(!transportAddressEmpty(&transport->secondaryDestination)) {
    len += snprintf(filter + len, PATH_MAX - len, "or host %s ",
		transport->addressToString(&transport->secondaryDestination));
    }

    len += snprintf(filter + len, PATH_MAX - len, ")");

    CCK_DBGV("pcap IPv4 transport \"%s\" using filter: %s\n",
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

    CCK_DBG("Successfully started IPv4 pcap transport serial %08x (%s) on endpoint %s\n",
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

    if(transport->transportData == NULL) {

	CCK_DBG("shutdown called for IPv4 pcap transport with no transport data present\n");
	return -1;

    }

    handle = (CckPcapEndpoint*)transport->transportData;

    pcap_close(handle->sender);
    pcap_close(handle->receiver);

    if(transport->isMulticastAddress(&transport->defaultDestination) &&
	    !multicastLeave(transport, handle->socket, &transport->defaultDestination)) {
	CCK_DBG("Error while leaving IPv4 multicast group %s\n",
		    transport->addressToString(&transport->defaultDestination));
    }

    if(transport->isMulticastAddress(&transport->secondaryDestination) &&
	    !multicastLeave(transport, handle->socket, &transport->secondaryDestination)) {
	CCK_DBG("Error while leaving IPv4 multicast group %s\n",
		    transport->addressToString(&transport->secondaryDestination));
    }

    /* remove fd from watcher set */
    cckRemoveFd(transport->fd, transport->watcher);

    /* close the helper FD */
    close(handle->socket);

    clearCckTransportCounters(transport);

    free(transport->transportData);

    transport->transportData = NULL;

    return 1;

}

static CckBool
cckTransportTestConfig(CckTransport* transport, const CckTransportConfig* config)
{

    int res = 0;

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
    res = cckPcapGetInterfaceAddress(transport, &transport->ownAddress, AF_INET);

    if(res != 1) {
	return CCK_FALSE;
    }

    CCK_DBG("own address: %s\n", transport->addressToString(&transport->ownAddress));

    if(config->hwTimestamping) {
	CCK_ERROR("libpcap based transport do not directly support hardware timestamping\n");
	return CCK_FALSE;
    }

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

    CckPcapEndpoint* handle = NULL;

    if(transport == NULL) {

	return;

    }

    if(transport->transportData == NULL) {
	CCK_DBG("refresh called for IPv4 pcap transport with no transport data present\n");
	return;
    }

    handle = (CckPcapEndpoint*)transport->transportData;


    if(transport->isMulticastAddress(&transport->defaultDestination)) {
	(void)multicastJoin(transport, handle->socket, &transport->defaultDestination);
    }

    if(transport->isMulticastAddress(&transport->secondaryDestination)) {
	(void)multicastJoin(transport, handle->socket, &transport->secondaryDestination);
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

    /* If last 4 bits of an address are 0xE (1110), it's multicast */
    if((ntohl(address->inetAddr4.sin_addr.s_addr) >> 28) == 0x0E)
	return CCK_TRUE;
    else
	return CCK_FALSE;

}

static ssize_t
cckTransportSend(CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* dst, CckTimestamp* timestamp)
{

    ssize_t ret;
    CckPcapEndpoint* handle = NULL;

    if(transport == NULL)
	return -1;

    if(transport->transportData == NULL) {
	CCK_DBG("send called for IPv4 pcap transport with no transport data present\n");
	return -1;
    }

    handle = (CckPcapEndpoint*)transport->transportData;

    if(dst==NULL)
	dst = &transport->defaultDestination;

    CckBool isMulticast = transport->isMulticastAddress(dst);

    /* If we have a unicast callback, run it first */
    if(transport->unicastCallback != NULL) {
	transport->unicastCallback(!isMulticast, buf, size);
    }

    if( (ret = sendto(handle->socket, buf, size, 0, (struct sockaddr *)dst,
            		sizeof(struct sockaddr_in))) != size) {
       CCK_WARNING("sendto() Error while sending IPv4 message on %s (transport \"%s\"): %s\n",
                      transport->transportEndpoint, transport->header.instanceName, strerror(errno));
    }
    else
	transport->sentMessages++;

    return ret;

}

static ssize_t
cckTransportRecv(CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* src, CckTimestamp* timestamp, int flags)
{
    ssize_t ret = 0;
    CckPcapEndpoint* handle = NULL;
    int payloadOffset = 0;
    struct pcap_pkthdr* header;
    const u_char*       data;

    if(transport == NULL)
	return -1;

    if(transport->transportData == NULL) {
	CCK_DBG("recv called for IPv4 pcap transport with no transport data present\n");
	return -1;
    }

    handle = (CckPcapEndpoint*)transport->transportData;

    clearTransportAddress(src);

    /* I bet you can squeal like a pig */
    (void)recv(handle->socket, buf, size, MSG_DONTWAIT);

    ret = pcap_next_ex(handle->receiver, &header, &data);

    if (ret <= 0 ) {

        /* could we have been interrupted by a signal? */
        if (errno == EAGAIN || errno == EINTR) {
            return 0;
        }

        CCK_DBG("Failed to receive pcap data for transport \"%s\" on %s: %s\n",
                        transport->header.instanceName, transport->transportEndpoint,
                        pcap_geterr(handle->receiver));
    }

    getPayloadAddress(data, header->caplen, &payloadOffset, src);

    ret = header->caplen - payloadOffset;

    /* if we have more data than buffer size, read only so much */
    if(ret > size) {
        ret = size;
    }

    memcpy(buf, data + payloadOffset, ret);

    /* get the timestamp */
    timestamp->seconds = header->ts.tv_sec;
    timestamp->nanoseconds = header->ts.tv_usec * 1000;

    CCK_DBGV("pcap IPv4 transport \"%s\" on %s - got pcap timestamp %us.%dns\n",
                    transport->header.instanceName, transport->transportEndpoint,
                    timestamp->seconds, timestamp->nanoseconds
                    );
    return ret;


}

static CckBool
cckTransportAddressFromString(const char* addrStr, TransportAddress* out)
{

    CckBool ret = CCK_FALSE;
    int res = 0;
    struct addrinfo hints, *info;

    clearTransportAddress(out);
    memset(&hints, 0, sizeof(hints));

    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;

    res = getaddrinfo(addrStr, NULL, &hints, &info);

    if(res == 0) {

        ret = CCK_TRUE;
        memcpy(out, info->ai_addr,
    	    sizeof(struct sockaddr_in));
	freeaddrinfo(info);

    } else {

        CCK_ERROR("Could not encode / resolve IPV4 address %s : %s\n", addrStr, gai_strerror(res));

    }

    return ret;

}

static char*
cckTransportAddressToString(const TransportAddress* address)
{

    /* just like *toa: convenient but not re-entrant */
    static char buf[INET_ADDRSTRLEN + 1];

    memset(buf, 0, INET_ADDRSTRLEN + 1);

    if(inet_ntop(AF_INET, &address->inetAddr4.sin_addr, buf, INET_ADDRSTRLEN) == NULL)
        return "-";

    return buf;

}

static CckBool
cckTransportAddressEqual(const TransportAddress* a, const TransportAddress* b)
{
	if(a->inetAddr4.sin_addr.s_addr == b->inetAddr4.sin_addr.s_addr)
	    return CCK_TRUE;
	else
	    return CCK_FALSE;
}

#undef CCK_THIS_TYPE
