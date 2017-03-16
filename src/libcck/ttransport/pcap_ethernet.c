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
 * @file   pcap_ethernet.c
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  Ethernet timestamping transport implementation using libpcap
 *
 */

#include <config.h>

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
#include <errno.h>

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>	/* struct ether_addr */
#endif /* HAVE_NET_ETHERNET_H */


#define THIS_COMPONENT "ttransport.pcap_ethernet: "

/* tracking the number of instances */
static int _instanceCount = 0;

static char* createFilterExpr(TTransport *self, char *buf, int size);

/* short status line - interface up/down, etc */
static char* getStatusLine(TTransport *self, char *buf, size_t len);

bool
_setupTTransport_pcap_ethernet(TTransport *self)
{

    INIT_INTERFACE(self);
    self->getStatusLine = getStatusLine;

    CCK_INIT_PDATA(TTransport, pcap_ethernet, self);
    CCK_INIT_PCONFIG(TTransport, pcap_ethernet, self);

    self->_instanceCount = &_instanceCount;

    self->headerLen = TT_HDRLEN_ETHERNET;

    _instanceCount++;

    return true;

}

bool
_probeTTransport_pcap_ethernet(const char *path, const int flags) {

    return true;

}

void
_initTTransportConfig_pcap_ethernet(TTransportConfig_pcap_ethernet *myConfig, const int family)
{
    myConfig->multicastStreams = createCckTransportAddressList(family, "tmp");
}

void
_freeTTransportConfig_pcap_ethernet(TTransportConfig_pcap_ethernet *myConfig)
{
    freeCckTransportAddressList(&myConfig->multicastStreams);
}

static int
tTransport_init(TTransport* self, const TTransportConfig *config, CckFdSet *fdSet) {

    int *fd = &self->myFd.fd;
    int promisc = 0;
    tmpstr(strAddr, self->tools->strLen);
    struct bpf_program program;
    char errbuf[PCAP_ERRBUF_SIZE];
    tmpstr(filterExpr, FILTER_EXPR_LEN);

    CCK_GET_PDATA(TTransport, pcap_ethernet, self, myData);
    CCK_GET_PCONFIG(TTransport, pcap_ethernet, self, myConfig);
    CCK_GET_PCONFIG(TTransport, pcap_ethernet, config, yourConfig);

    if(!getInterfaceInfo(&myData->intInfo, yourConfig->interface, self->family, NULL, false)) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): Interface %s not usable, cannot continue\n",
		self->name, yourConfig->interface);
	return -1;
    }

    copyCckTransportAddress(&self->ownAddress, &myData->intInfo.afAddress);
    copyCckTransportAddress(&self->hwAddress, &myData->intInfo.hwAddress);

    *fd = -1;

    memset(&program, 0, sizeof(program));

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

    /* same */
    memcpy(&self->ownAddress, &self->hwAddress, sizeof(CckTransportAddress));

    /* build the filter expression before we set up libpcap so we can quit early on error */
    createFilterExpr(self, filterExpr, sizeof(filterExpr));

    if(!strlen(filterExpr)) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): could not build PCAP filter expression for %s\n",
	self->name, myConfig->interface);
	return -1;
    }

    /*
     * We use one PCAP handle for reading and one for writing. Why?
     * With libpcap transports we rely on capturing our own packets / frames
     * to estimate transmission time. A pcap device cannot capture packets
     * we have injected into it using pcap_inject(), therefore we use one
     * pap device for injection only (and set a dummy bpf filter on it which
     * is meant to capture nothing), and another to receive all required
     * packets / framesmm, including our own.
     */

    /* set up PCAP writer */

    myData->writerHandle = pcap_open_live(myConfig->interface, self->inputBufferSize + self->headerLen,
					0, PCAP_TIMEOUT, errbuf);
    if(myData->writerHandle == NULL) {
	CCK_PERROR(THIS_COMPONENT"tTransportInit(%s): failed to open PCAP writer device on %s",
	self->name, myConfig->interface);
	return -1;
    }

    /* the writer pcap device does not need to receive anything, set dummy filter */
    if(pcap_compile(myData->writerHandle, &program, 
	"ether src 00:00:00:00:00:00 and ether dst 00:00:00:00:00:00", 1, PCAP_NETMASK_UNKNOWN) < 0) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): writer: could not compile PCAP filter expression for %s:%s \n",
	self->name, myConfig->interface, pcap_geterr(myData->writerHandle));
	goto cleanup;
    }

    if(pcap_setfilter(myData->writerHandle, &program) < 0) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): writer: could not set PCAP filter for %s:%s \n",
	self->name, myConfig->interface, pcap_geterr(myData->writerHandle));
	goto cleanup;
    }

    pcap_freecode(&program);

    /* join Ethernet multicast, use promiscuous mode if failed */
    if(!self->refresh(self)) {
	CCK_DBG(THIS_COMPONENT"tTransportInit(%s):could not add Ethernet multicast membership, using promiscuous mode\n",
	    self->name, myConfig->interface);
	promisc = 1;
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

    *fd = pcap_get_selectable_fd(myData->readerHandle);

    if(*fd < 0) {
	CCK_PERROR(THIS_COMPONENT"tTransportInit(%s): Failed to get PCAP file descriptor", self->name);
	goto cleanup;
    }

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

    CCK_GET_PDATA(TTransport, pcap_ethernet, self, myData);
    CCK_GET_PCONFIG(TTransport, pcap_ethernet, self, myConfig);

    /* remove our fd from fd set if we have one */
    if(self->fdSet) {
	cckRemoveFd(self->fdSet, &self->myFd);
    }

    /* free multicast group list and leave groups if we have them */
    if(myConfig->multicastStreams) {
	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {
		joinMulticast_ethernet(mcAddr, myConfig->interface, false);
	    }
    }

    /* close the socket */
    if(self->myFd.fd > -1) {
	close(self->myFd.fd);
    }

    /* close the pcap handles */
    pcap_close(myData->readerHandle);
    pcap_close(myData->writerHandle);

    self->myFd.fd = -1;

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
	CCK_GET_PCONFIG(TTransport, pcap_ethernet, self, myConfig);

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
    struct ether_header *ethh;
    char buf[message->length + self->headerLen];

    CCK_GET_PDATA(TTransport, pcap_ethernet, self, myData);
    CCK_GET_PCONFIG(TTransport, pcap_ethernet, self, myConfig);

    if(self->config.disabled) {
	return 0;
    }

    if(!message->destination) {
	CCK_DBG(THIS_COMPONENT"sendMessage(%s): no destination provided, dropping message\n");
	return 0;
    }

    ethh = (struct ether_header*) buf;

    memcpy(buf + self->headerLen, message->data, message->length);
    memcpy(&ethh->ether_dhost, &message->destination->addr.ether, TT_ADDRLEN_ETHERNET);
    memcpy(&ethh->ether_shost, &self->ownAddress.addr.ether, TT_ADDRLEN_ETHERNET);
    ethh->ether_type = htons(myConfig->etherType);

    mc = self->tools->isMulticast(message->destination);

    /* run user callback before sending */
    if(self->callbacks.preTx) {

	ret = self->callbacks.preTx(self, self->owner, buf + self->headerLen, message->length, mc);

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

    ret = pcap_inject(myData->writerHandle, buf, message->length + self->headerLen);
    sent = ret - self->headerLen;

    if(ret <= 0) {
#ifdef CCK_DEBUG
	    tmpstr(strAddr, self->tools->strLen);
	    CCK_ERROR(THIS_COMPONENT"sendMessage(%s): error sending message to %s: %s\n",
		    self->name,
		    self->tools->toString(strAddr, strAddr_len, message->destination),
		    pcap_geterr(myData->readerHandle));
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
    self->counters.txBytes += sent;

    return sent;

}

static ssize_t
receiveMessage(TTransport *self, TTransportMessage *message) {

    struct ether_header *ethh;
    CCK_GET_PDATA(TTransport, pcap_ethernet, self, myData);
    ssize_t ret = 0;

    struct pcap_pkthdr *pkt_header;
    const u_char *buf = NULL;

    if(self->config.disabled) {
	return 0;
    }

    clearTTransportMessage(message);

    ret = pcap_next_ex(myData->readerHandle, &pkt_header, &buf);

    /* skipping n-- next messages */
    if(self->_skipMessages) {
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): _skipMessages = %d, dropping message\n",
		    self->name, self->_skipMessages);
	self->_skipMessages--;
	return 0;
    }

    /* drain the buffer, but ignore what we received */
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

    ethh = (struct ether_header*) buf;

    /* grab source and destination */
    memcpy(&message->from.addr.ether, &ethh->ether_shost, TT_ADDRLEN_ETHERNET);
    memcpy(&message->to.addr.ether, &ethh->ether_dhost, TT_ADDRLEN_ETHERNET);

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
	self->counters.rxBytes += message->length;
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

    return message->length;

}

static ClockDriver*
getClockDriver(TTransport *self) {

    return getSystemClock();

}

static int
monitor(TTransport *self, const int interval, const bool quiet) {

    CCK_GET_PCONFIG(TTransport, pcap_ethernet, self, myConfig);
    CCK_GET_PDATA(TTransport, pcap_ethernet, self, myData);

    if(!myData->intInfo.valid) {
	getInterfaceInfo(&myData->intInfo, myConfig->interface,
	self->family, NULL, CCK_QUIET);
    }

    return monitorInterface(&myData->intInfo, NULL, quiet);

}

static int
refresh(TTransport *self) {

    int ret = 1;

    CCK_GET_PCONFIG(TTransport, pcap_ethernet, self, myConfig);

    /* join multicast groups if we have them */
    if(self->config.flags | TT_CAPS_MCAST) {

	if(myConfig->multicastStreams) {
	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {
		ret &= joinMulticast_ethernet(mcAddr, myConfig->interface, true);
	    }
	}

    }

    return ret;

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

    CCK_GET_PCONFIG(TTransport, pcap_ethernet, self, myConfig);

    /* ethertype */
    ret = snprintf(marker, left, "ether proto 0x%04x and (", myConfig->etherType);
    if(!maintainStrBuf(ret, &marker, &left)) {
	return buf;
    }

    /* own address */
    ret = snprintf(marker, left, "ether host %s", self->tools->toString(strAddr, sizeof(strAddr), &self->hwAddress));
    if(!maintainStrBuf(ret, &marker, &left)) {
	return buf;
    }

    /* build multicast address list */
    if(self->config.flags | TT_CAPS_MCAST) {
	if(myConfig->multicastStreams) {
	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {
		ret = snprintf(marker, left, " or ether dst %s", self->tools->toString(strAddr, sizeof(strAddr), mcAddr));
		if(!maintainStrBuf(ret, &marker, &left)) {
		    return buf;
		}
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

	CCK_GET_PDATA(TTransport, pcap_ethernet, self, myData);

	return getIntStatusLine(&myData->intInfo, buf, len);

}
