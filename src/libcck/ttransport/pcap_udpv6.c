/* Copyright (c) 2016 Wojciech Owczarek,
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
 * @file   pcap_udpv6.c
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  Libpcap-based IPv6 software timestamping transport implementation
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
#include <libcck/ttransport/pcap_common.h>
#include <libcck/net_utils.h>
#include <libcck/clockdriver.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>	/* struct ether_addr */
#endif /* HAVE_NET_ETHERNET_H */

#include <netinet/ip6.h>
#include <netinet/udp.h>


#define THIS_COMPONENT "ttransport.pcap_udpv6: "

/* tracking the number of instances */
static int _instanceCount = 0;

static char* createFilterExpr(TTransport *self, char *buf, int size);

/* short status line - interface up/down, etc */
static char* getStatusLine(TTransport *self, char *buf, size_t len);

bool
_setupTTransport_pcap_udpv6(TTransport *self)
{

    INIT_INTERFACE(self);
    self->getStatusLine = getStatusLine;

    CCK_INIT_PDATA(TTransport, pcap_udpv6, self);
    CCK_INIT_PCONFIG(TTransport, pcap_udpv6, self);

    self->_instanceCount = &_instanceCount;

    self->headerLen = TT_HDRLEN_UDPV6;

    _instanceCount++;

    return true;

}

bool
_probeTTransport_pcap_udpv6(const char *path, const int flags) {

    return true;

}

static int
tTransport_init(TTransport* self, const TTransportConfig *config, CckFdSet *fdSet) {

    int *fd;
    int *pcapFd = &self->myFd.fd;
    int val = 1;
    int valsize = sizeof(val);
    int ret;

    bool bindToAddress = true;
    CckTransportAddress bindAddr;
    tmpstr(filterExpr, FILTER_EXPR_LEN);
    tmpstr(strAddr, self->tools->strLen);

    const struct in6_addr anyv6  = IN6ADDR_ANY_INIT;

    int promisc = 0;
    struct bpf_program program;
    char errbuf[PCAP_ERRBUF_SIZE];

    CCK_GET_PDATA(TTransport, pcap_udpv6, self, myData);
    CCK_GET_PCONFIG(TTransport, pcap_udpv6, self, myConfig);
    CCK_GET_PCONFIG(TTransport, pcap_udpv6, config, yourConfig);

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
    *pcapFd = -1;

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
    *fd = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

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
	bindAddr.addr.inet6.sin6_addr = anyv6;
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

#ifdef linux /* SO_BINDTODEVICE */
    /* multicast only */
    if( (self->config.flags & TT_CAPS_MCAST) && !(self->config.flags & TT_CAPS_UCAST)) {
	ret = setsockopt(*fd, SOL_SOCKET, SO_BINDTODEVICE, myConfig->interface, strlen(myConfig->interface));
	if(ret < 0) {
	    CCK_PERROR(THIS_COMPONENT"tTransport_init(%s): Failed to call SO_BINDTODEVICE on %s\n",
			self->name, myConfig->interface);
	}
    }
#endif /* linux, SO_BINDTODEVICE */

    /* set various options */
    if(self->config.flags & TT_CAPS_MCAST) {

	/* no need for multicast loopback - libpcap receives everything */
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

    /* ===== part 2: set up libpcap reader */

    /* build the filter expression before we set up libpcap so we can quit early on error */
    createFilterExpr(self, filterExpr, sizeof(filterExpr));

    if(!strlen(filterExpr)) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): could not build PCAP filter expression for %s\n",
	self->name, myConfig->interface);
	return -1;
    }

    /* set up PCAP reader */

#ifdef PCAP_TSTAMP_PRECISION_NANO /* use the elaborate PCAP activation to set ns timestamp precision */

    myData->readerHandle = pcap_create(myConfig->interface, errbuf);

    if(myData->readerHandle == NULL) {
	CCK_PERROR(THIS_COMPONENT"tTransportInit(%s): failed to open PCAP reader device on %s",
	self->name, myConfig->interface);
	goto cleanup;
    }

    /* these only fail if the handle has already been activated, and it hasn't */
    pcap_set_promisc(myData->readerHandle, promisc);
    pcap_set_snaplen(myData->readerHandle, self->inputBufferSize + self->headerLen);
    pcap_set_timeout(myData->readerHandle, PCAP_TIMEOUT);

    myData->nanoPrecision = false;

    if (pcap_set_tstamp_precision(myData->readerHandle, PCAP_TSTAMP_PRECISION_NANO) == 0) {
	CCK_DBG(THIS_COMPONENT"tTransportInit(%s): using PCAP_TSTAMP_PRECISION_NANO on %s\n",
	self->name, myConfig->interface);
	myData->nanoPrecision = true;
    }

    if(pcap_activate(myData->readerHandle) != 0) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): reader: could not activate PCAP device for %s :%s \n",
	self->name, myConfig->interface, pcap_geterr(myData->readerHandle));
	goto cleanup;
    }

#else /* use pcap_open_live */

    myData->readerHandle = pcap_open_live(myConfig->interface, self->inputBufferSize + self->headerLen,
					promisc, PCAP_TIMEOUT, errbuf);

    if(myData->readerHandle == NULL) {
	CCK_PERROR(THIS_COMPONENT"tTransportInit(%s): failed to open PCAP reader device on %s",
	self->name, myConfig->interface);
	return -1;
    }

#endif /* PCAP_TSTAMP_PRECISION_NANO */

    if(pcap_compile(myData->readerHandle, &program, filterExpr, 1, PCAP_NETMASK_UNKNOWN) < 0) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): reader: could not compile PCAP filter expression for %s: %s \n",
	self->name, myConfig->interface, pcap_geterr(myData->readerHandle));
	goto cleanup;
    }

    if(pcap_setfilter(myData->readerHandle, &program) < 0) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): reader: could not set PCAP filter for %s: %s \n",
	self->name, myConfig->interface, pcap_geterr(myData->readerHandle));
	goto cleanup;
    }

    pcap_freecode(&program);

    *pcapFd = pcap_get_selectable_fd(myData->readerHandle);

    if(*pcapFd < 0) {
	CCK_PERROR(THIS_COMPONENT"tTransportInit(%s): Failed to get PCAP file descriptor", self->name);
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

    CCK_GET_PDATA(TTransport, pcap_udpv6, self, myData);
    CCK_GET_PCONFIG(TTransport, pcap_udpv6, self, myConfig);

    /* remove our fd from fd set if we have one */
    if(self->fdSet) {
	cckRemoveFd(self->fdSet, &self->myFd);
    }

    /* leave multicast groups if we have them */
    if(self->config.flags | TT_CAPS_MCAST) {

	if(myConfig->multicastStreams) {
	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {
		joinMulticast_ipv6(myData->sockFd, mcAddr, myConfig->interface, &self->ownAddress, false);
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

    /* close the pcap handle */
    pcap_close(myData->readerHandle);

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
	CCK_GET_PCONFIG(TTransport, pcap_udpv6, self, myConfig);

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

    CCK_GET_PDATA(TTransport, pcap_udpv6, self, myData);

    if(self->config.disabled) {
	return 0;
    }

    if(!message->destination) {
	CCK_DBG(THIS_COMPONENT"sendMessage(%s): no destination provided, dropping message\n");
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
		    (struct sockaddr*)&message->destination->addr.inet6,
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

    CCK_GET_PDATA(TTransport, pcap_udpv6, self, myData);
    ssize_t ret = 0;

    struct ip6_hdr *ip6;
    struct udphdr *udp;

    struct pcap_pkthdr *pkt_header;
    const u_char *buf = NULL;

    if(self->config.disabled) {
	return 0;
    }

    ret = pcap_next_ex(myData->readerHandle, &pkt_header, &buf);

    /* drop socket data on the floor */
    recv(myData->sockFd, message->data, message->capacity, MSG_DONTWAIT);

    clearTTransportMessage(message);

    /* skipping n-- next messages */
    if(self->_skipMessages) {
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): _skipMessages = %d, dropping message\n",
		    self->name, self->_skipMessages);
	self->_skipMessages--;
	return 0;
    }

    /* drain the socket, but ignore what we received */
    if(self->config.discarding) {
	return 0;
    }

    if(buf == NULL) {
        CCK_DBG(THIS_COMPONENT"receiveMessage(%s): libcap returned empty buffer\n", self->name);
        return 0;
    }

    if(ret < 0) {
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): Error while receiving message via libpcap: %s\n",
	    self->name, pcap_geterr(myData->readerHandle));
	    self->counters.rxErrors++;
	return ret;
    }

    ret = pkt_header->caplen;

    if(ret <= self->headerLen) {
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): Received truncated data (%d <= %d)", self->name,
		ret, self->headerLen);
	return ret;
    }

    message->length = ret - self->headerLen;

    if(self->config.timestamping) {

	message->timestamp.seconds = pkt_header->ts.tv_sec;
	message->timestamp.nanoseconds = pkt_header->ts.tv_usec;

	if (!myData->nanoPrecision) {
	    message->timestamp.nanoseconds *= 1000;
	}

	if(!tsOps()->isZero(&message->timestamp)) {
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
    ip6 = (struct ip6_hdr*) (buf + sizeof(struct ether_header));
    udp = (struct udphdr*) (buf + sizeof(struct ether_header) + sizeof(struct ip6_hdr));

    message->from.addr.inet6.sin6_family = self->tools->afType;
    message->to.addr.inet6.sin6_family = self->tools->afType;
    memcpy(&message->from.addr.inet6.sin6_addr, &ip6->ip6_src, sizeof(struct in6_addr));
    memcpy(&message->to.addr.inet6.sin6_addr, &ip6->ip6_dst, sizeof(struct in6_addr));
#ifdef HAVE_STRUCT_UDPHDR_SOURCE
    message->from.addr.inet6.sin6_port = udp->source;
    message->to.addr.inet6.sin6_port = udp->dest;
    message->from.port = ntohs(udp->source);
    message->to.port = ntohs(udp->dest);
#else
    message->from.addr.inet6.sin6_port = udp->uh_sport;
    message->to.addr.inet6.sin6_port = udp->uh_dport;
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
	CCK_DBG(THIS_COMPONENT"receiveMesage(%s): received %d bytes from %s to %s\n",
		self->name, ret, self->tools->toString(strAddrf, strAddrf_len, &message->from),
				self->tools->toString(strAddrt, strAddrt_len, &message->to));
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

    CCK_GET_PCONFIG(TTransport, pcap_udpv6, self, myConfig);
    CCK_GET_PDATA(TTransport, pcap_udpv6, self, myData);

    if(!myData->intInfo.valid) {
	getInterfaceInfo(&myData->intInfo, myConfig->interface,
	self->family, &myConfig->sourceAddress, CCK_QUIET);
    }

    return monitorInterface(&myData->intInfo, &myConfig->sourceAddress, quiet);

}

static int
refresh(TTransport *self) {

    CCK_GET_PDATA(TTransport, pcap_udpv6, self, myData);
    CCK_GET_PCONFIG(TTransport, pcap_udpv6, self, myConfig);

    /* join multicast groups if we have them */
    if(self->config.flags | TT_CAPS_MCAST) {

	if(myConfig->multicastStreams) {
	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {
		joinMulticast_ipv6(myData->sockFd, mcAddr, myConfig->interface, &self->ownAddress, true);
	    }
	}

    }

    return 1;

}

static void
loadVendorExt(TTransport *self) {

}

static char*
createFilterExpr(TTransport *self, char *buf, int size) {

    tmpstr(strAddr, self->tools->strLen);
    char *marker = buf;
    int left = size;
    int ret;

    CCK_GET_PCONFIG(TTransport, pcap_udpv6, self, myConfig);

    /* port */
    ret = snprintf(marker, left, "ip6 and udp port %d and (", myConfig->listenPort);
    if(!maintainStrBuf(ret, &marker, &left)) {
	return buf;
    }

    if(TT_UC(self->config.flags)) {
	/* own address */
	ret = snprintf(marker, left, "ip6 host %s", self->tools->toString(strAddr, sizeof(strAddr), &self->ownAddress));
	if(!maintainStrBuf(ret, &marker, &left)) {
	    return buf;
	}
    }

    if(TT_UC_MC(self->config.flags) && myConfig->multicastStreams->count) {
	/* own address */
	ret = snprintf(marker, left, " or ");
	if(!maintainStrBuf(ret, &marker, &left)) {
	    return buf;
	}
    }

    /* build multicast address list */
    if(TT_MC(self->config.flags)) {
	bool first = true;
	if(myConfig->multicastStreams) {
	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {
		ret = snprintf(marker, left, "%sip6 dst %s", first ? "" : " or ",
			self->tools->toString(strAddr, sizeof(strAddr), mcAddr));
		if(!maintainStrBuf(ret, &marker, &left)) {
		    return buf;
		}
		first = false;
	    }
	}
    }

    /* the closing bracket... */
    ret = snprintf(marker, left, ")");
    if(!maintainStrBuf(ret, &marker, &left)) {
	return buf;
    }

    CCK_DBG(THIS_COMPONENT"createFilterExpr(%s): PCAP filter expression: %s\n",
	    self->name, buf);

    return buf;

}

static char*
getStatusLine(TTransport *self, char *buf, size_t len) {

	CCK_GET_PDATA(TTransport, pcap_udpv6, self, myData);

	return getIntStatusLine(&myData->intInfo, buf, len);

}
