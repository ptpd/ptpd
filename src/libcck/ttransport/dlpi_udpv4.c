/* Copyright (c) 2017 Wojciech Owczarek,
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
 * @file   dlpi_udpv4.c
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  dlpi-based IPv4 software timestamping transport implementation
 *
 */

#include <config.h>

#include <errno.h>

#include <libcck/cck_types.h>

#include <libcck/cck.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/ttransport.h>
#include <libcck/ttransport_interface.h>
#include <libcck/ttransport/dlpi_common.h>
#include <libcck/net_utils.h>
#include <libcck/clockdriver.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>	/* struct ether_addr */
#endif /* HAVE_NET_ETHERNET_H */

#include <netinet/ip.h>
#include <netinet/udp.h>


#define THIS_COMPONENT "ttransport.dlpi_udpv4: "

/* tracking the number of instances */
static int _instanceCount = 0;

static void createFilterExpr(TTransport *self, struct pfstruct *pf);

/* short status line - interface up/down, etc */
static char* getStatusLine(TTransport *self, char *buf, size_t len);

bool
_setupTTransport_dlpi_udpv4(TTransport *self)
{

    INIT_INTERFACE(self);
    self->getStatusLine = getStatusLine;

    CCK_INIT_PDATA(TTransport, dlpi_udpv4, self);
    CCK_INIT_PCONFIG(TTransport, dlpi_udpv4, self);

    self->_instanceCount = &_instanceCount;

    self->headerLen = TT_HDRLEN_UDPV4;

    _instanceCount++;

    return true;

}

bool
_probeTTransport_dlpi_udpv4(const char *path, const int flags) {

    return true;

}

static int
tTransport_init(TTransport* self, const TTransportConfig *config, CckFdSet *fdSet) {

    int *fd;
    int *dlpiFd = &self->myFd.fd;
    int val = 1;
    int valsize = sizeof(val);
    int ret;

    bool bindToAddress = true;
    CckTransportAddress bindAddr;

    tmpstr(strAddr, self->tools->strLen);

    int promisc = 1;

    struct pfstruct pf;

    memset(&pf, 0, sizeof(struct pfstruct));

    CCK_GET_PDATA(TTransport, dlpi_udpv4, self, myData);
    CCK_GET_PCONFIG(TTransport, dlpi_udpv4, self, myConfig);
    CCK_GET_PCONFIG(TTransport, dlpi_udpv4, config, yourConfig);

    if(!getInterfaceInfo(&myData->intInfo, yourConfig->interface, self->family, &yourConfig->sourceAddress, false)) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): Interface %s not usable, cannot continue\n",
		self->name, yourConfig->interface);
	return -1;
    }

    copyCckTransportAddress(&self->ownAddress, &myData->intInfo.afAddress);
    copyCckTransportAddress(&self->hwAddress, &myData->intInfo.hwAddress);

    if(!self->hwAddress.populated) {
	CCK_NOTICE(THIS_COMPONENT"tTransportInit(%s): No hardware address available, using protocol address for EUID\n");
    }

    fd = &myData->sockFd;

    *fd = -1;
    *dlpiFd = -1;

    /* if initialising with existing config, it's a restart */
    if(config != &self->config) {
	/* copy the configuration into the transport */
	copyTTransportConfig(&self->config, config);

	/* make a duplicate of the address list */
	if(myConfig->multicastStreams) {
	    CckTransportAddressList *duplicate = duplicateCckTransportAddressList(myConfig->multicastStreams);
	    myConfig->multicastStreams = duplicate;
	}
    }

    /* set up ports */
    setTransportAddressPort(&myConfig->sourceAddress, myConfig->listenPort);
    setTransportAddressPort(&self->ownAddress, myConfig->listenPort);

    /* ===== part 1: prepare the socket - we need to have it open so we don't send unreachables etc. */

    /* open the socket */
    *fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(*fd < 0) {
	CCK_PERROR(THIS_COMPONENT"tTransportInit(%s): Failed to open socket!", self->name);
	return -1;
    }

    /* allow address reuse */
    ret = setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &val, valsize);

    if(ret < 0) {
	CCK_DBG(THIS_COMPONENT"tTransportInit(%s): Failed to set SO_REUSEADDR: %s\n", self->name, strerror(errno));
    }

    /* prepare bind address */
    memcpy(&bindAddr, &self->ownAddress, sizeof(CckTransportAddress));

    /* if using multicast or mixed, bind to ANY */
    if(self->config.flags & TT_CAPS_MCAST) {
	bindToAddress = false;
    }

    /* ...unless it is forced to bind to source */
    if(myConfig->bindToAddress) {
	bindToAddress = true;
    }

    if(!bindToAddress) {
	bindAddr.addr.inet4.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    /* bind socket */
    ret = bind(*fd, (struct sockaddr *)&bindAddr.addr, self->tools->structSize);

    clearstr(strAddr);
    self->tools->toString(strAddr, strAddr_len, &bindAddr);

    if(ret < 0) {
	CCK_PERROR(THIS_COMPONENT"tTransportInit(%s): could not bind to %s:%s on %s",
	self->name, strAddr, myConfig->listenPort, myConfig->interface);
	goto cleanup;
    } else {
	CCK_DBG(THIS_COMPONENT"tTransportInit(%s): successfully bound to  %s:%d on %s\n",
	self->name, strAddr, myConfig->listenPort, myConfig->interface);
    }

    /* set various options */
    if(self->config.flags & TT_CAPS_MCAST) {

	/* no need for multicast loopback - dlpi receives everything */
	if (!setMulticastLoopback(*fd, self->family, false)) {
	    goto cleanup;
	}

	if(myConfig->multicastTtl) {
	    setMulticastTtl(*fd, self->family, myConfig->multicastTtl);
	}

    }

    if(myConfig->dscpValue) {
	setSocketDscp(*fd, self->family, myConfig->dscpValue);
    }

    /* ===== part 2: set up DLPI reader */

    /* build the DLPI filter */
    createFilterExpr(self, &pf);

    /* set up DLPI reader */

    struct timeval timeout = { 0,0};

    *dlpiFd = dlpiInit(&myData->readerHandle, myConfig->interface, promisc, timeout, 0, self->inputBufferSize + self->headerLen, &pf, (uint_t)ETHERTYPE_IP);

    if(*dlpiFd < 0) {
	CCK_PERROR(THIS_COMPONENT"tTransportInit(%s): Failed to get DLPI file descriptor", self->name);
	goto cleanup;
    }

    /* join multicast groups */
    self->refresh(self);

    /* assign FD set */
    self->fdSet = fdSet;

    /* mark us as the owner of this fd */
    self->myFd.owner = self;

    /* add ourselves to the FD set */
    if(self->fdSet != NULL) {
	cckAddFd(self->fdSet, &self->myFd);
    }

    /* set our UUID based on our addresses */
    setTTransportUid(self);

    self->_vendorInit(self);
    self->_init = true;

    CCK_NOTICE(THIS_COMPONENT"Transport '%s' (%s) started\n",
		self->name, myConfig->interface);

    return 1;

cleanup:

    self->shutdown(self);
    return -1;

}

static int
tTransport_shutdown(TTransport *self) {

    CCK_GET_PDATA(TTransport, dlpi_udpv4, self, myData);
    CCK_GET_PCONFIG(TTransport, dlpi_udpv4, self, myConfig);

    /* remove our fd from fd set if we have one */
    if(self->fdSet) {
	cckRemoveFd(self->fdSet, &self->myFd);
    }

    /* leave multicast groups if we have them */
    if(self->config.flags | TT_CAPS_MCAST) {

	if(myConfig->multicastStreams) {
	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {
		joinMulticast_ipv4(myData->sockFd, mcAddr, myConfig->interface, &self->ownAddress, false);
	    }
	}

    }

    /* close the sockets */
    if(myData->sockFd > -1) {
	close(myData->sockFd);
    }
    myData->sockFd = -1;

    if(self->myFd.fd > -1) {
	close(self->myFd.fd);
    }
    self->myFd.fd = -1;

    /* close the dlpi handle */
    dlpi_close(myData->readerHandle);

    if(self->_init) {
	CCK_INFO(THIS_COMPONENT"Transport '%s' (%s) shutting down\n", self->name, myConfig->interface);
    }

    /* run any vendor-specific shutdown code */
    self->_vendorShutdown(self);

    if(_instanceCount > 0) {
	_instanceCount--;
    }

    self->_init = false;

    return 1;

}

static bool
isThisMe(TTransport *self, const char* search)
{
	CCK_GET_PCONFIG(TTransport, dlpi_udpv4, self, myConfig);

	tmpstr(strAddr, self->tools->strLen);

	/* are we looking for my address? */
	if(!strncmp(search, self->tools->toString(strAddr, strAddr_len, &self->ownAddress), strAddr_len)) {
		return true;
	}

	/* are we looking for my interface? */
	if(!strncmp(search, myConfig->interface, IFNAMSIZ)) {
		return true;
	}

	return false;

}

static bool
testConfig(TTransport *self, const TTransportConfig *config) {

	return true;

}

static bool
privateHealthCheck(TTransport *self)
{

    bool ret = true;

    if(self->config.disabled) {
	return 0;
    }


    if(self == NULL) {
	return false;
    }

    CCK_DBG(THIS_COMPONENT"transport '%s' private health check...\n", self->name);

    return ret;

}

static ssize_t
sendMessage(TTransport *self, TTransportMessage *message) {

    ssize_t ret, sent;
    bool mc;

    CCK_GET_PDATA(TTransport, dlpi_udpv4, self, myData);

    if(self->config.disabled) {
	return 0;
    }

    if(!message->destination) {
	CCK_DBG(THIS_COMPONENT"sendMessage(%s): no destination provided, dropping message\n");
	return 0;
    }

    /* skipping n-- next messages */
    if(self->_skipTxMessages) {
	CCK_DBG(THIS_COMPONENT"sendMessage(%s): _skipTxMessages = %d, dropping message\n",
		    self->name, self->_skipTxMessages);
	self->_skipTxMessages--;
	return 0;
    }

    mc = self->tools->isMulticast(message->destination);

    /* run user callback before sending */
    if(self->callbacks.preTx) {

	ret = self->callbacks.preTx(self, self->owner, message->data, message->length, mc);

	if(ret < 0) {
#ifdef CCK_DEBUG
	    tmpstr(strAddr, self->tools->strLen);
	    CCK_DBG(THIS_COMPONENT"sendMessage(%s): user callback prevented us from sending a message to %s\n",
		    self->name,
		    self->tools->toString(strAddr, strAddr_len, message->destination));
#endif /* CCK_DEBUG */
	    self->counters.txDiscards++;
	    return ret;
	}
    }

    ret = sendto(
		    myData->sockFd,
		    message->data,
		    message->length,
		    MSG_DONTWAIT,
		    (struct sockaddr*)&message->destination->addr.inet4,
		    self->tools->structSize
		);

    sent = ret;

    if(ret <= 0) {
#ifdef CCK_DEBUG
	    tmpstr(strAddr, self->tools->strLen);
	    CCK_PERROR(THIS_COMPONENT"sendMessage(%s): error sending message to %s: %s\n",
		    self->name,
		    self->tools->toString(strAddr, strAddr_len, message->destination),
		    strerror(errno));
#endif /* CCK_DEBUG */
	    self->counters.txErrors++;
	    return ret;
    }

#ifdef CCK_DEBUG
	    tmpstr(strAddr, self->tools->strLen);
	    CCK_DBG(THIS_COMPONENT"sendMessage(%s): sent %d bytes to %s\n",
		    self->name,
		    message->length,
		    self->tools->toString(strAddr, strAddr_len, message->destination));
#endif /* CCK_DEBUG */

    self->counters.txMsg++;
    self->counters.txBytes += sent + self->headerLen;

    return sent;

}

static ssize_t
receiveMessage(TTransport *self, TTransportMessage *message) {


    CCK_GET_PDATA(TTransport, dlpi_udpv4, self, myData);

    ssize_t ret = 0;
    size_t len = self->headerLen + message->capacity;

    struct {
	struct sb_hdr hdr;
	char data[len];
    } mbuf;

    char *buf = mbuf.data;
    struct sb_hdr *hdr = &mbuf.hdr;

    struct ip *ip;
    struct udphdr *udp;

    if(self->config.disabled) {
	return 0;
    }

    ret = dlpi_recv(myData->readerHandle, NULL, NULL, &mbuf, &len, 0, NULL);

    /* drop socket data on the floor */
    recv(myData->sockFd, message->data, message->capacity, MSG_DONTWAIT);

    clearTTransportMessage(message);

    /* skipping n-- next messages */
    if(self->_skipRxMessages) {
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): _skipRxMessages = %d, dropping message\n",
		    self->name, self->_skipRxMessages);
	self->_skipRxMessages--;
	return 0;
    }

    /* drain the socket, but ignore what we received */
    if(self->config.discarding) {
	return 0;
    }

    if(hdr->sbh_msglen == 0) {
        CCK_DBG(THIS_COMPONENT"receiveMessage(%s): DLPI returned no data\n", self->name);
        return 0;
    }

    if(ret < 0) {
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): Error while receiving message via DLPI: %s\n",
	    self->name, strerror(errno));
	    self->counters.rxErrors++;
	return ret;
    }

    ret = hdr->sbh_msglen;

    if(ret <= self->headerLen) {
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): Received truncated data (%d <= %d)", self->name,
		ret, self->headerLen);
	return ret;
    }

    message->length = ret - self->headerLen;

    if(self->config.timestamping) {

	message->timestamp.seconds = hdr->sbh_timestamp.tv_sec;
	message->timestamp.nanoseconds = hdr->sbh_timestamp.tv_usec * 1000;

	if(!tsOps.isZero(&message->timestamp)) {
	    message->hasTimestamp = true;
	} else {
	    CCK_DBG(THIS_COMPONENT"receiveMessage(%s): Error: no timestamp received!\n",
		    self->name);
	    self->counters.rxTimestampErrors++;
	    /* message with no timestamp considered harmful */
	    ret = 0;
	}
    }

    memcpy(message->data, buf + self->headerLen, message->length);

    /* grab source and destination */

    ip = (struct ip*) (buf + sizeof(struct ether_header));
    udp = (struct udphdr*) (buf + sizeof(struct ether_header) + sizeof(struct ip));

    message->from.addr.inet4.sin_family = self->tools->afType;
    message->to.addr.inet4.sin_family = self->tools->afType;
    memcpy(&message->from.addr.inet4.sin_addr, &ip->ip_src, sizeof(struct in_addr));
    memcpy(&message->to.addr.inet4.sin_addr, &ip->ip_dst, sizeof(struct in_addr));
#ifdef HAVE_STRUCT_UDPHDR_SOURCE
    message->from.addr.inet4.sin_port = udp->source;
    message->to.addr.inet4.sin_port = udp->dest;
    message->from.port = ntohs(udp->source);
    message->to.port = ntohs(udp->dest);
#else
    message->from.addr.inet4.sin_port = udp->uh_sport;
    message->to.addr.inet4.sin_port = udp->uh_dport;
    message->from.port = ntohs(udp->uh_sport);
    message->to.port = ntohs(udp->uh_dport);
#endif

    if(!self->tools->isEmpty(&message->from)) {
	message->from.populated = true;
	message->from.family = self->family;
    }

    if(!self->tools->isEmpty(&message->to)) {
	message->to.populated = true;
	message->to.family = self->family;
    }

    /* see if message is from my address - do not increment counters if it is */
    if(message->from.populated && !self->tools->compare(&message->from, &self->ownAddress)) {
	message->fromSelf = true;
    } else {
	self->counters.rxMsg++;
	self->counters.rxBytes += ret + self->headerLen;
    }

    message->toSelf = message->to.populated && !self->tools->compare(&message->to, &self->ownAddress);
    message->isMulticast = message->to.populated && self->tools->isMulticast(&message->to);

    message->_family = self->family;

    /* check message against ACLs */
    if(!self->messagePermitted(self, message)) {
	return 0;
    }

#ifdef CCK_DEBUG
{
	tmpstr(strAddrt, self->tools->strLen);
	tmpstr(strAddrf, self->tools->strLen);
	CCK_DBG(THIS_COMPONENT"receiveMesage(%s): received %d bytes from %s:%d to %s:%d\n",
		self->name, ret, self->tools->toString(strAddrf, strAddrf_len, &message->from), message->from.port,
				self->tools->toString(strAddrt, strAddrt_len, &message->to), message->to.port);
}
#endif

    return ret;

}

static ClockDriver*
getClockDriver(TTransport *self) {

    return getSystemClock();

}

static int
monitor(TTransport *self, const int interval, const bool quiet) {

    CCK_GET_PCONFIG(TTransport, dlpi_udpv4, self, myConfig);
    CCK_GET_PDATA(TTransport, dlpi_udpv4, self, myData);

    if(!myData->intInfo.valid) {
	getInterfaceInfo(&myData->intInfo, myConfig->interface,
	self->family, &myConfig->sourceAddress, true);
    }

    return monitorInterface(&myData->intInfo, &myConfig->sourceAddress, quiet);

}

static int
refresh(TTransport *self) {

    CCK_GET_PDATA(TTransport, dlpi_udpv4, self, myData);
    CCK_GET_PCONFIG(TTransport, dlpi_udpv4, self, myConfig);

    /* join multicast groups if we have them */
    if(self->config.flags | TT_CAPS_MCAST) {

	if(myConfig->multicastStreams) {
	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {
		joinMulticast_ipv4(myData->sockFd, mcAddr, myConfig->interface, &self->ownAddress, true);
	    }
	}

    }

    return 1;

}

static void
loadVendorExt(TTransport *self) {

}

static void
createFilterExpr(TTransport *self, struct pfstruct *pf) {

    CCK_GET_PCONFIG(TTransport, dlpi_udpv4, self, myConfig);

    /* skip VLAN tag if present */
    pfSkipVlan(pf);

    /* IP ethertype */
    pfMatchEthertype(pf, ETHERTYPE_IP);

    /* match my own source or my own destination */
    if(TT_UC(self->config.flags)) {
	pfMatchAddressDir(pf, &self->ownAddress, PFD_ANY);
    }

    /* OR on any multicast destinations */
    if(TT_MC(self->config.flags)) {
	bool first = true;
	if(myConfig->multicastStreams) {

	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {

		pfMatchAddressDir(pf, mcAddr, PFD_TO);

		/* if we have no unicast, no need for OR after first mcast group, otherwise an OR */
		if(!first || TT_UC_MC(self->config.flags)) {
		    PFPUSH(ENF_OR);
		}
		first = false;
	    }
	}
    }

    /* AND previous */
    PFPUSH(ENF_AND);

    /* UDP */
    pfMatchIpProto(pf, self->family, IPPROTO_UDP);

    /* AND previous */
    PFPUSH(ENF_AND);

    /* match UDP destination port: Ethernet, assumed 20 bytes of IPv4 header, 2 bytes into UDP */
    pfMatchWord(pf, TT_HDRLEN_ETHERNET + 20 + 2, htons(myConfig->listenPort));

    /* AND previous */
    PFPUSH(ENF_AND);

}

static char*
getStatusLine(TTransport *self, char *buf, size_t len) {

	CCK_GET_PDATA(TTransport, dlpi_udpv4, self, myData);

	return getIntStatusLine(&myData->intInfo, buf, len);

}
