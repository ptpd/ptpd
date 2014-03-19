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
 * @file   cck_transport_socket_ipv6.c
 * 
 * @brief  libCCK ipv6 transport implementation
 *
 */

#include "cck_transport_socket_ipv6.h"

#define CCK_THIS_TYPE CCK_TRANSPORT_SOCKET_UDP_IPV6

/* interface (public) method definitions */
static int     cckTransportInit (CckTransport* transport, const CckTransportConfig* config);
static int     cckTransportShutdown (void* _transport);
static int     cckTransportPushConfig (CckTransport* transport, const CckTransportConfig* config);
static CckBool cckTransportTestConfig (CckTransport* transport, const CckTransportConfig* config);
static void    cckTransportRefresh (CckTransport* transport);
static CckBool cckTransportHasData (CckTransport* transport);
static CckBool cckTransportIsMulticastAddress (const TransportAddress* address);
static ssize_t cckTransportSend (CckTransport* transport, CckOctet* buf, CckUInt16 size, TransportAddress* dst, CckTimestamp* timestamp);
static ssize_t cckTransportRecv (CckTransport* transport, CckOctet* buf, CckUInt16 size, TransportAddress* src, CckTimestamp* timestamp, int flags);
static CckBool cckTransportAddressFromString (const char*, TransportAddress* out);
static char*   cckTransportAddressToString (const TransportAddress* address);
static CckBool cckTransportAddressEqual (const TransportAddress* a, const TransportAddress* b);

/* private method definitions (if any) */

/* implementations follow */

void
cckTransportSetup_socket_ipv6 (CckTransport* transport)
{
	if(transport->transportType == CCK_THIS_TYPE) {

            transport->aclType = CCK_ACL_IPV6;

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
	    CCK_WARNING("setup() called for incorrect transport: %02x, expected %02x\n",
			transport->transportType, CCK_THIS_TYPE);
	}
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

    /* drop multicast group on specified interface first */
    (void)setsockopt(transport->fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
           &imr, sizeof(struct ipv6_mreq));

    /* join multicast group on specified interface */
    if (setsockopt(transport->fd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
           &imr, sizeof(struct ipv6_mreq)) < 0) {
	    CCK_PERROR("failed to join multicast group %s: ", transport->addressToString(mcastAddr));
		    return CCK_FALSE;
    }

   CCK_DBGV("Joined IPV6 multicast group %s on %s\n", transport->addressToString(mcastAddr),
                                                    transport->transportEndpoint);

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

   CCK_DBG("Sent IPv6 multicast leave for %s on %s (transport %s)\n", transport->addressToString(mcastAddr),
                                                                transport->transportEndpoint,
                                                                transport->header.instanceName);

    return CCK_TRUE;

}

static CckBool
setMulticastLoopback(CckTransport* transport, CckBool _value)
{

//	CckUInt8

	u_int value = _value ? 1 : 0;

	if (setsockopt(transport->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
	       &value, sizeof(value)) < 0) {
                CCK_PERROR("Failed to %s IPv6 multicast loopback on %s",
                            value ? "enable":"disable",
                            transport->transportEndpoint);
		return CCK_FALSE;
	}

        CCK_DBG("Successfully %s IPv6 multicast loopback on %s \n", value ? "enabled":"disabled",
                    transport->transportEndpoint);

	return CCK_TRUE;
}

static CckBool
setMulticastTtl(CckTransport* transport, int _value)
{

//        CckUInt8 

int value = _value;

        if (setsockopt(transport->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
               &value, sizeof(value)) < 0) {
               CCK_PERROR("Failed to set IPv6 multicast hop count to %d on %s",
                        value, transport->transportEndpoint);
                return CCK_FALSE;
        }

        CCK_DBG("Set IPv6 multicast hop count on %s to %d\n", transport->transportEndpoint, value);

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

    int ret = 0;
    TransportAddress src;
    ssize_t length;
    fd_set tmpSet;
    struct timeval timeOut = {0,100};

    FD_ZERO(&tmpSet);
    FD_SET(transport->fd, &tmpSet);

    ret = select(transport->fd + 1, &tmpSet, NULL, NULL, &timeOut);


    if(ret > 0) {

	if (FD_ISSET(transport->fd, &tmpSet)) {

	    length = transport->recv(transport, buf, size, &src, timeStamp, MSG_ERRQUEUE);

	    if(length < 0) {
		CCK_DBG("getTxTimestamp SO_TIMESTAMPING Error while getting TX timestamp via MSG_ERRQUEUE\n");
		return CCK_FALSE;
	    }

	    if(length == 0) {
		CCK_DBG("getTxTimestamp SO_TIMESTAMPING Received no data while getting TX timestamp via MSG_ERRQUEUE\n");
		return CCK_FALSE;
	    }

	    if(!timeStamp->seconds && !timeStamp->nanoseconds) {
		CCK_DBG("getTxTimestamp SO_TIMESTAMPING Received no TX timestamp via MSG_ERRQUEUE\n");
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


int
cckTransportInit (CckTransport* transport, const CckTransportConfig* config)
{
    int res = 0;
    struct sockaddr_in6* addr;

    CckIpv6TransportData* data = NULL;

    if(transport == NULL) {
	CCK_ERROR("IPv6 transport init called for an empty transport\n");
	return -1;
    }

    if(config == NULL) {
	CCK_ERROR("IPv6 transport init called with empty config\n");
	return -1;
    }

    /* Shutdown will be called on this object anyway, OK to exit without explicitly freeing this */
    CCKCALLOC(transport->transportData, sizeof(CckIpv6TransportData));

    data = (CckIpv6TransportData*)transport->transportData;

    transport->fd = socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(transport->fd < 0) {
	CCK_PERROR("Could not create IPv6 socket");
	return -1;
    }

    strncpy(transport->transportEndpoint, config->transportEndpoint, PATH_MAX);

    transport->ownAddress = config->ownAddress;

    /* Get own address first - or find the desired one if config was populated with it*/
    res = cckGetInterfaceAddress(transport, &transport->ownAddress, AF_INET6);

    if(res != 1) {
	return res;
    }

    CCK_DBG("own address: %s\n", transport->addressToString(&transport->ownAddress));

    transport->ownAddress.inetAddr6.sin6_port = htons(config->sourceId);

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

    transport->defaultDestination.inetAddr6.sin6_port = htons(config->destinationId);
    transport->secondaryDestination.inetAddr6.sin6_port = htons(config->destinationId);

    /* One of our destinations is multicast - begin multicast initialisation */
    if(transport->isMulticastAddress(&transport->defaultDestination) ||
	transport->isMulticastAddress(&transport->secondaryDestination)) {

	/* ====== MCAST_INIT_BEGIN ======= */

	struct sockaddr_in6 sin;
	memset(&sin, 0, sizeof(struct sockaddr_in6));
	sin.sin6_family = AF_INET6;
	sin.sin6_addr = in6addr_any;
	sin.sin6_port = htons(config->sourceId);
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
		CCK_DBG("Failed to set IPv6 multicast hops=%d on %s\n",
		    config->param1, transport->transportEndpoint);
	    }
	}

	/* ====== MCAST_INIT_END ======= */

    } else {
	addr = &transport->ownAddress.inetAddr6;
    }

	int tmp = 1;
	if(setsockopt(transport->fd, SOL_SOCKET, SO_REUSEADDR,
		&tmp, sizeof(int)) < 0) {
	    CCK_DBG("Failed to set SO_REUSEADDR\n");
	}

    if(bind(transport->fd, (struct sockaddr*)addr,
		    sizeof(struct sockaddr_in6)) < 0) {
        CCK_PERROR("Failed to bind to %s:%d on %s",
			transport->addressToString(&transport->ownAddress),
			config->sourceId, transport->transportEndpoint);
	return -1;
    }

    CCK_DBG("Successfully bound to %s:%d on %s\n",
			transport->addressToString(&transport->ownAddress),
			config->sourceId, transport->transportEndpoint);

    /* TODO: IPV6_TCLASS */

    /* set DSCP bits */
/*
    if(config->param2 && (setsockopt(transport->fd, IPPROTO_IP, IP_TOS,
		&config->param2, sizeof(int)) < 0)) {
	CCK_DBG("Failed to set DSCP bits on %s\n",
		    transport->transportEndpoint);
    }
*/

    if(config->swTimestamping) {

	if(cckInitSwTimestamping(
	    transport, &data->timestampCaps, CCK_FALSE)) {
		transport->timestamping = CCK_TRUE;
		transport->txTimestamping = data->timestampCaps.txTimestamping;
	} else {
	    CCK_ERROR("Failed to enable software timestamping on %s\n",
		    transport->transportEndpoint);
	    return -1;
	}

    } else if(config->hwTimestamping) {
	if(cckInitHwTimestamping(
	    transport, &data->timestampCaps, CCK_FALSE)) {
		transport->timestamping = CCK_TRUE;
		transport->txTimestamping = data->timestampCaps.txTimestamping;
	} else {
	    CCK_ERROR("Failed to enable hardware timestamping on %s\n",
		    transport->transportEndpoint);
	    return -1;
	}
    }

    /*
       One of our destinations is multicast - enable loopback if we have no TX timestamping,
       AND DISABLE IT if we don't - some OSes default to multicast loopback.
     */
    CckBool loopback = transport->timestamping && !transport->txTimestamping && 
			( transport->isMulticastAddress(&transport->defaultDestination) ||
			transport->isMulticastAddress(&transport->secondaryDestination));

    if(!setMulticastLoopback(transport, loopback)) {
	    CCK_ERROR("Failed to %s multicast loopback on socket\n",
			loopback ? "enable" : "disable");
	    return -1;
    }

    /* everything succeeded, so we can add the transport's fd to the fd set */
    cckAddFd(transport->fd, transport->watcher);

    CCK_DBG("Successfully started transport serial %08x (%s) on endpoint %s\n",
		transport->header.serial, transport->header.instanceName,
		transport->transportEndpoint);

    return 1;

}

int
cckTransportShutdown (void* _transport)
{

    if(_transport == NULL)
	return -1;

    CckTransport* transport = (CckTransport*)_transport;

    if(transport->isMulticastAddress(&transport->defaultDestination) &&
	    !multicastLeave(transport, &transport->defaultDestination)) {
	CCK_DBG("Error while leaving IPv6 multicast group %s\n",
		    transport->addressToString(&transport->defaultDestination));
    }

    if(transport->isMulticastAddress(&transport->secondaryDestination) &&
	    !multicastLeave(transport, &transport->secondaryDestination)) {
	CCK_DBG("Error while leaving IPv6 multicast group %s\n",
		    transport->addressToString(&transport->secondaryDestination));
    }

    /* remove fd from watcher set */
    cckRemoveFd(transport->fd, transport->watcher);

    /* close the fd */
    close(transport->fd);

    clearCckTransportCounters(transport);

    if(transport->transportData != NULL) {
	free(transport->transportData);
	transport->transportData = NULL;
    }

    return 1;

}

static CckBool
cckTransportTestConfig (CckTransport* transport, const CckTransportConfig* config)
{

    int res = 0;
    unsigned int flags;

    CckIpv6TransportData* data = NULL;

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
    res = cckGetInterfaceAddress(transport, &transport->ownAddress, AF_INET6);

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

    CCKCALLOC(data, sizeof(CckIpv6TransportData));

    if(config->swTimestamping) {
	if(!cckInitSwTimestamping(
	    transport, &data->timestampCaps, CCK_TRUE)) {
	    free(data);
	    return CCK_FALSE;
	}

    } else if(config->hwTimestamping) {
	if(!cckInitHwTimestamping(
	    transport, &data->timestampCaps, CCK_TRUE)) {
	    free(data);
	    return CCK_FALSE;
	}

    }

    free(data);
    return CCK_TRUE;

}

static int
cckTransportPushConfig (CckTransport* transport, const CckTransportConfig* config)
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

static void
cckTransportRefresh (CckTransport* transport)
{

    if(transport->isMulticastAddress(&transport->defaultDestination)) {
	(void)multicastJoin(transport, &transport->defaultDestination);
    }

    if(transport->isMulticastAddress(&transport->secondaryDestination)) {
	(void)multicastJoin(transport, &transport->secondaryDestination);
    }

}

static CckBool
cckTransportHasData (CckTransport* transport)
{

    if(transport->watcher == NULL)
	return CCK_FALSE;

    if(FD_ISSET(transport->fd, &transport->watcher->workReadFdSet))
	return CCK_TRUE;

    return CCK_FALSE;

}

static CckBool
cckTransportIsMulticastAddress (const TransportAddress* address)
{

    /* First byte is FF - multicast */
    if((unsigned char)(address->inetAddr6.sin6_addr.s6_addr[0]) == 0xFF)
	return CCK_TRUE;
    else
	return CCK_FALSE;

}

static ssize_t
cckTransportSend (CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* dst, CckTimestamp* timestamp)
{

    ssize_t ret;

    if(dst==NULL) {
	dst = &transport->defaultDestination;
    }

    CckBool isMulticast = transport->isMulticastAddress(dst);

    /* If we have a unicast callback, run it first */
    if(transport->unicastCallback != NULL) {
	transport->unicastCallback(!isMulticast, buf, size);
    }

    ret = sendto(transport->fd, buf, size, 0,
		(struct sockaddr *)dst,
		sizeof(struct sockaddr_in6));


    /* If the transport can timestamp on TX, try doing it - if we failed, loop back the packet */
    if(transport->txTimestamping) {
	getTxTimestamp(transport, buf, size, timestamp);
    }

    /* destination is unicast and we have no TX timestamping, - loop back the packet */
    if(!isMulticast && !transport->txTimestamping) {
	if(sendto(transport->fd, buf, size, 0,
		     (struct sockaddr *)&transport->ownAddress,
	 sizeof(struct sockaddr_in6)) <= 0)
             CCK_DBG("send() Error looping back IPv6 message on %s (transport \"%s\")\n",
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
cckTransportRecv (CckTransport* transport, CckOctet* buf, CckUInt16 size,
			TransportAddress* src, CckTimestamp* timestamp, int flags)
{

    ssize_t ret = 0;
    socklen_t srcLen = sizeof(struct sockaddr);

    CckIpv6TransportData* data = (CckIpv6TransportData*)transport->transportData;

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

//      if(!flags) flags = MSG_DONTWAIT;

	ret = recvmsg(transport->fd, &msg, flags | MSG_DONTWAIT);

	if (!flags && ret <= 0) {
	    if (errno == EAGAIN || errno == EINTR) {
		return 0;
	    } else {
		return ret;
	    }
	};

	if(flags != MSG_ERRQUEUE ) {

	    if (msg.msg_flags & MSG_TRUNC) {
		CCK_ERROR("IPv6 - Received truncated message on %s\n",
		    transport->transportEndpoint);
		return 0;
	    }

	    if (msg.msg_flags & MSG_CTRUNC) {
		CCK_ERROR("IPv6 - Received truncated control message on %s\n",
		    transport->transportEndpoint);
		return 0;
	    }

	    if (msg.msg_controllen <= 0) {
		CCK_ERROR("IPv6 - Received short control message on %s (transport %s): (%lu/%lu)\n",
			transport->transportEndpoint,
			transport->header.instanceName,
		        (long)msg.msg_controllen, 
			(long)sizeof(cmsg_un.control));
			return 0;
	    }

	    transport->receivedMessages++;

	}

	/* control message looks OK - let's try to grab the timestamp */
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
	    if ((cmsg->cmsg_level == SOL_SOCKET) && cckGetSwTimestamp(cmsg, &data->timestampCaps, timestamp)) {
		break;
	    }
	}

	/* TODO: cckTimestampEmpty() */
	if (!timestamp->seconds && !timestamp->nanoseconds) {
	    CCK_DBG("recv() IPv6 - no timestamp received on %s\n",
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
cckTransportAddressFromString (const char* addrStr, TransportAddress* out)
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
    	    sizeof(struct sockaddr_in6));
	freeaddrinfo(info);
    } else {

        CCK_ERROR("Could not encode / resolve IPV6 address %s : %s\n", addrStr, gai_strerror(res));

    }

    return ret;
}

static char*
cckTransportAddressToString (const TransportAddress* address)
{
    /* just like *toa: convenient but not re-entrant */
    static char buf[INET6_ADDRSTRLEN + 1];

    memset(buf, 0, INET6_ADDRSTRLEN + 1);

    if(inet_ntop(AF_INET6, &address->inetAddr6.sin6_addr, buf, INET6_ADDRSTRLEN) == NULL)
        return "-";

    return buf;

}


static CckBool
cckTransportAddressEqual (const TransportAddress* a, const TransportAddress* b)
{
	if(!memcmp(&a->inetAddr6.sin6_addr.s6_addr, &b->inetAddr6.sin6_addr.s6_addr, 16))
	    return CCK_TRUE;
	else
	    return CCK_FALSE;
}

#undef CCK_THIS_TYPE
