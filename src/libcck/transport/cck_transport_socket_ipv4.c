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
 * @file   cck_transport_socket_ipv4.c
 * 
 * @brief  libCCK ipv4 transport implementation
 *
 */

#include "cck_transport_socket_ipv4.h"

#define CCK_THIS_TYPE CCK_TRANSPORT_SOCKET_UDP_IPV4

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
cckTransportSetup_socket_ipv4 (CckTransport* transport)
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
multicastJoin(CckTransport* transport, TransportAddress* mcastAddr)
{

    struct ip_mreq imr;

    imr.imr_multiaddr.s_addr = mcastAddr->inetAddr4.sin_addr.s_addr;
    imr.imr_interface.s_addr = transport->ownAddress.inetAddr4.sin_addr.s_addr;

    /* set multicast outbound interface */
    if (setsockopt(transport->fd, IPPROTO_IP, IP_MULTICAST_IF,
           &imr.imr_interface, sizeof(struct in_addr)) < 0) {
        CCK_PERROR("failed to set multicast outbound interface");
        return CCK_FALSE;
    }

    /* drop multicast group on specified interface first */
    (void)setsockopt(transport->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
           &imr, sizeof(struct ip_mreq));

    /* join multicast group on specified interface */
    if (setsockopt(transport->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
           &imr, sizeof(struct ip_mreq)) < 0) {
	    CCK_PERROR("failed to join multicast address %s: ", transport->addressToString(mcastAddr));
		    return CCK_FALSE;
    }

    CCK_DBGV("Joined IPV4 multicast group %s on %s\n", transport->addressToString(mcastAddr),
						    transport->transportEndpoint);

    return CCK_TRUE;

}

static CckBool
multicastLeave(CckTransport* transport, TransportAddress* mcastAddr)
{

    struct ip_mreq imr;

    imr.imr_multiaddr.s_addr = mcastAddr->inetAddr4.sin_addr.s_addr;
    imr.imr_interface.s_addr = transport->ownAddress.inetAddr4.sin_addr.s_addr;

    /* leave multicast group on specified interface */
    if (setsockopt(transport->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
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
setMulticastLoopback(CckTransport* transport, CckBool _value)
{

	CckUInt8 value = _value ? 1 : 0;

	if (setsockopt(transport->fd, IPPROTO_IP, IP_MULTICAST_LOOP, 
	       &value, sizeof(value)) < 0) {
		CCK_PERROR("Failed to %s IPv4 multicast loopback on %s",
			    value ? "enable":"disable",
			    transport->transportEndpoint);
		return CCK_FALSE;
	}

	CCK_DBG("Successfully %s IPv4 multicast loopback on %s \n", value ? "enabled":"disabled", 
		    transport->transportEndpoint);

	return CCK_TRUE;
}

static CckBool
setMulticastTtl(CckTransport* transport, int _value)
{

	CckUInt8 value = _value;

	if (setsockopt(transport->fd, IPPROTO_IP, IP_MULTICAST_TTL,
	       &value, sizeof(value)) < 0) {
		CCK_PERROR("Failed to set IPv4 multicast TTL to %d on %s",
			value, transport->transportEndpoint);
		return CCK_FALSE;
	}

	CCK_DBG("Set IPv4 multicast TTL on %s to %d\n", transport->transportEndpoint, value);

	return CCK_TRUE;
}

static CckBool
getTxTimestamp(CckTransport* transport, CckOctet* buf, CckUInt16 size, CckTimestamp* timeStamp) {

    if(!transport->txTimestamping) {
	return CCK_FALSE;
    }

    if(timeStamp == NULL) {
	CCK_DBG("getTxTimestamp() called for a NULL timestamp\n");
	return CCK_FALSE;
    }

#if defined(SO_TIMESTAMPING) && defined(HAVE_DECL_MSG_ERRQUEUE) && HAVE_DECL_MSG_ERRQUEUE
    int i = 0;
    int ret = 0;
    TransportAddress src;
    ssize_t length;
    fd_set tmpSet;
    struct timeval timeOut = {0,0};

    FD_ZERO(&tmpSet);
    FD_SET(transport->fd, &tmpSet);

    /* try up to two times on errors / timeouts */
    for(ret = 0; ret <= 0 && i < 2; i++) {
	/* timeout is zero - should return immediately */
	ret = select(transport->fd + 1, &tmpSet, NULL, NULL, &timeOut);
    }

    if(ret > 0) {

	if (FD_ISSET(transport->fd, &tmpSet)) {

	    length = transport->recv(transport, buf, size, &src, timeStamp, MSG_ERRQUEUE);

	    if(length < 0) {
		CCK_DBG("getTxTimestamp SO_TIMESTAMPING Error while getting TX timestamp via MSG_ERRQUEUE");
		return CCK_FALSE;
	    }

	    if(length == 0) {
		CCK_DBG("getTxTimestamp SO_TIMESTAMPING Received no data while getting TX timestamp via MSG_ERRQUEUE");
		return CCK_FALSE;
	    }

	    if(!timeStamp->seconds && !timeStamp->nanoseconds) {
		CCK_DBG("getTxTimestamp SO_TIMESTAMPING Received no TX timestamp via MSG_ERRQUEUE");
		return CCK_FALSE;
	    }

    	    CCK_DBG("getTxTimestamp SO_TIMESTAMPING TX timestamp %d.%d\n",
		    timeStamp->seconds, timeStamp->nanoseconds);
	    return CCK_TRUE;

	} else {
	    /* This should not happen - only one FD in set */
	    CCK_DBG("getTxTimestamp SO_TIMESTAMPING select() >0 but !FD_ISSET\n");
	    return CCK_FALSE;
	}

    } else if (ret == 0) {
	CCK_DBG("getTxTimestamp SO_TIMESTAMPING select() timeout\n");
    } else {
	CCK_DBG("getTxTimestamp SO_TIMESTAMPING select() exit with %s\n", strerror(errno));
    }

    return CCK_FALSE;

#endif /* SO_TIMESTAMPING */

    return CCK_FALSE;

}

static int
cckTransportInit(CckTransport* transport, const CckTransportConfig* config)
{
    int res = 0;
    struct sockaddr_in* addr;

    CckIpv4TransportData* data = NULL;

    if(transport == NULL) {
	CCK_ERROR("IPv4 transport init called for an empty transport\n");
	return -1;
    }

    if(config == NULL) {
	CCK_ERROR("IPv4 transport init called with empty config\n");
	return -1;
    }

    /* Shutdown will be called on this object anyway, OK to exit without explicitly freeing this */
    CCKCALLOC(transport->transportData, sizeof(CckIpv4TransportData));

    data = (CckIpv4TransportData*)transport->transportData;

    transport->fd = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(transport->fd < 0) {
	CCK_PERROR("Could not create IPv4 socket");
	return -1;
    }

    strncpy(transport->transportEndpoint, config->transportEndpoint, PATH_MAX);

    transport->ownAddress = config->ownAddress;

    /* Get own address first - or find the desired one if config was populated with it*/
    res = cckGetInterfaceAddress(transport, &transport->ownAddress, AF_INET);

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
	    !multicastJoin(transport, &transport->defaultDestination)) {
	    return -1;
	}

	if(transport->isMulticastAddress(&transport->secondaryDestination) &&
	    !multicastJoin(transport, &transport->secondaryDestination)) {
	    return -1;
	}


#if defined(linux) && defined(SO_BINDTODEVICE)
	if(setsockopt(transport->fd, SOL_SOCKET, SO_BINDTODEVICE,
		transport->transportEndpoint, strlen(transport->transportEndpoint)) < 0) {
	    CCK_DBG("Failed to set SO_BINDTODEVICE\n");
	}
#endif /* linux && SO_BINDTODEVICE */

	/* set multicast TTL */
	if(config->param1) {
	    if (setMulticastTtl(transport, config->param1)) {
		data->lastTtl = config->param1;
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
	if(setsockopt(transport->fd, SOL_SOCKET, SO_REUSEADDR,
		&tmp, sizeof(int)) < 0) {
	    CCK_DBG("Failed to set SO_REUSEADDR\n");
	}

    if(bind(transport->fd, (struct sockaddr*)addr,
		    sizeof(struct sockaddr_in)) < 0) {
        CCK_PERROR("Failed to bind to %s:%d on %s",
			transport->addressToString(&transport->ownAddress),
			config->sourceId, transport->transportEndpoint);
	return -1;
    }

    CCK_DBG("Successfully bound to %s:%d on %s\n",
			transport->addressToString(&transport->ownAddress),
			config->sourceId, transport->transportEndpoint);

    /* set DSCP bits */
    if(config->param2 && (setsockopt(transport->fd, IPPROTO_IP, IP_TOS,
		&config->param2, sizeof(int)) < 0)) {
	CCK_DBG("Failed to set DSCP bits on %s\n",
		    transport->transportEndpoint);
    }


    if(config->swTimestamping) {

	if(cckInitSwTimestamping(
	    transport, &data->timestampCaps, CCK_FALSE)) {
		transport->timestamping = CCK_TRUE;
	} else {
	    CCK_ERROR("Failed to enable software timestamping on %s\n",
		    transport->transportEndpoint);
	    return -1;
	}

    }

    /* One of our destinations is multicast - set loopback if we have no TX timestamping */
    if(transport->timestamping && !transport->txTimestamping && (
	transport->isMulticastAddress(&transport->defaultDestination) ||
	transport->isMulticastAddress(&transport->secondaryDestination)) &&
	!setMulticastLoopback(transport, CCK_TRUE)) {
	    CCK_ERROR("Failed to set multicast loopback on socket\n");
	    return -1;
    }

    /* everything succeeded, so we can add the transport's fd to the fd set */
    cckAddFd(transport->fd, transport->watcher);

    CCK_DBG("Successfully started transport serial %08x (%s) on endpoint %s\n",
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

    if(transport->isMulticastAddress(&transport->defaultDestination) &&
	    !multicastLeave(transport, &transport->defaultDestination)) {
	CCK_DBG("Error while leaving IPv4 multicast group %s\n",
		    transport->addressToString(&transport->defaultDestination));
    }

    if(transport->isMulticastAddress(&transport->secondaryDestination) &&
	    !multicastLeave(transport, &transport->secondaryDestination)) {
	CCK_DBG("Error while leaving IPv4 multicast group %s\n",
		    transport->addressToString(&transport->secondaryDestination));
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

    CckBool txFailure = CCK_FALSE;

    if(dst==NULL)

    dst = &transport->defaultDestination;

    CckBool isMulticast = transport->isMulticastAddress(dst);

    /* If we have a unicast callback, run it first */
    if(transport->unicastCallback != NULL) {
	transport->unicastCallback(!isMulticast, buf, size);
    }

    ret = sendto(transport->fd, buf, size, 0,
		(struct sockaddr *)dst,
		sizeof(struct sockaddr_in));


    /* If the transport can timestamp on TX, try doing it - if we failed, loop back the packet */
    if(transport->txTimestamping && !getTxTimestamp(transport, buf, size, timestamp)) {
	txFailure = CCK_TRUE;
    }

    /* destination is unicast and we have no TX timestamping, or TX timestamping failed - loop back the packet */
    if((!isMulticast && !transport->txTimestamping) || txFailure) {
	if(sendto(transport->fd, buf, size, 0,
		     (struct sockaddr *)&transport->ownAddress,
	 sizeof(struct sockaddr_in)) <= 0)
             CCK_DBG("send() Error looping back IPv4 message on %s (transport \"%s\")\n",
                        transport->transportEndpoint, transport->header.instanceName);
    }

    if (ret <= 0)
           CCK_DBG("send() Error while sending IPv6 message on %s (transport \"%s\")\n",
                        transport->transportEndpoint, transport->header.instanceName);
    else
	transport->sentMessages++;

    return ret;

}

static ssize_t
cckTransportRecv(CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* src, CckTimestamp* timestamp, int flags)
{
    ssize_t ret = 0;
    socklen_t srcLen = sizeof(struct sockaddr);

    CckIpv4TransportData* data = (CckIpv4TransportData*)transport->transportData;

    if(data == NULL || transport == NULL)
	return -1;

    clearTransportAddress(src);

    /* === timestamping transport - get timestamp BEGIN ===== */

    if(transport->timestamping) {
	if(timestamp == NULL) {
	    CCK_DBG("recv() called with an empty timestamp\n");
	    return 0;
	}

	struct msghdr msg;
	struct iovec vec[1];

	union {
		struct cmsghdr hdr;
		unsigned char control[CMSG_SPACE(data->timestampCaps.timestampSize)];
	}     cmsg_un;

	struct cmsghdr *cmsg;

	vec[0].iov_base = buf;
	vec[0].iov_len = size;

	memset(&msg, 0, sizeof(msg));

	memset(buf, 0, size);
	memset(&cmsg_un, 0, sizeof(cmsg_un));

	msg.msg_name = (caddr_t)src;
	msg.msg_namelen = sizeof(TransportAddress);
	msg.msg_iov = vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsg_un.control;
	msg.msg_controllen = sizeof(cmsg_un.control);
	msg.msg_flags = 0;

	ret = recvmsg(transport->fd, &msg, MSG_DONTWAIT | flags);

	if (ret <= 0) {
	    if (errno == EAGAIN || errno == EINTR) {
		return 0;
	    } else {
		return ret;
	    }
	};

	if (msg.msg_flags & MSG_TRUNC) {
	    CCK_ERROR("IPv4 - Received truncated message on %s\n",
		transport->transportEndpoint);
	    return 0;
	}

	if (msg.msg_flags & MSG_CTRUNC) {
	    CCK_ERROR("IPv4 - Received truncated control message on %s\n",
		transport->transportEndpoint);
	    return 0;
	}

	if(!flags)
	    transport->receivedMessages++;

	if (msg.msg_controllen <= 0) {
	    CCK_ERROR("IPv4 - Received short control message on %s: (%lu/%lu)\n",
			transport->transportEndpoint,
		        (long)msg.msg_controllen, 
			(long)sizeof(cmsg_un.control));
			return 0;
	}

	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
	    if (cmsg->cmsg_level == SOL_SOCKET && cckGetSwTimestamp(cmsg, &data->timestampCaps, timestamp)) {
		break;
	    }
	}

	/* TODO: cckTimestampEmpty() */
	if (!timestamp->seconds && !timestamp->nanoseconds) {
	    CCK_DBG("recv() IPv4 - no timestamp received on %s\n",
		transport->transportEndpoint);
			return 0;
	}

	return ret;

    /* === timestamping transport - get RX timestamp END ===== */

    } else {

    /* === no timestamp needed - standard recvfrom */

	ret = recvfrom(transport->fd, buf, size, MSG_DONTWAIT, (struct sockaddr*)src, &srcLen);
	if (ret <= 0 ) {
	    if (errno == EAGAIN || errno == EINTR)
		return 0;
	}
	return ret;

    }
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
