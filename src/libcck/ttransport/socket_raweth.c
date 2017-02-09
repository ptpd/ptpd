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
 * @file   socket_raweth.c
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  Raw (Ethernet) socket (software) timestamping transport implementation
 *
 */

#include <config.h>

#include <errno.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/ttransport.h>
#include <libcck/ttransport_interface.h>
#include <libcck/net_utils.h>
#include <libcck/clockdriver.h>

#define THIS_COMPONENT "ttransport.socket_raweth: "

/* tracking the number of instances */
static int _instanceCount = 0;

/* initialise timestamping and populate @config */
static bool initTimestamping(TTransport *self, TTsocketTimestampConfig *config);

bool
_setupTTransport_socket_raweth(TTransport *self)
{

    INIT_INTERFACE(self);

    CCK_INIT_PDATA(TTransport, socket_raweth, self);
    CCK_INIT_PCONFIG(TTransport, socket_raweth, self);

    self->_instanceCount = &_instanceCount;

    self->headerLen = TT_HDRLEN_ETHERNET;

    _instanceCount++;

    return true;

}

bool
_probeTTransport_socket_raweth(const char *path, const int flags) {

    return true;

}

void
_initTTransportConfig_socket_raweth(TTransportConfig_socket_raweth *myConfig, const int family)
{
    myConfig->multicastStreams = createCckTransportAddressList(family, "tmp");
}

void
_freeTTransportConfig_socket_raweth(TTransportConfig_socket_raweth *myConfig)
{
    freeCckTransportAddressList(&myConfig->multicastStreams);
}

static int
tTransport_init(TTransport* self, const TTransportConfig *config, CckFdSet *fdSet) {

    int *fd = &self->myFd.fd;
    int val = 1;
    int valsize = sizeof(val);
    int ret;
    struct sockaddr_ll bindAddr;

    tmpstr(strAddr, self->tools->strLen);

    *fd = -1;

    CCK_GET_PDATA(TTransport, socket_raweth, self, myData);
    CCK_GET_PCONFIG(TTransport, socket_raweth, self, myConfig);
    CCK_GET_PCONFIG(TTransport, socket_raweth, config, yourConfig);

    if(!getInterfaceInfo(&myData->intInfo, yourConfig->interface, self->family, NULL, false)) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): Interface %s not usable, cannot continue\n",
		self->name, yourConfig->interface);
	return -1;
    }

    copyCckTransportAddress(&self->ownAddress, &myData->intInfo.afAddress);
    copyCckTransportAddress(&self->hwAddress, &myData->intInfo.hwAddress);

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

    /* open the socket */
    *fd = socket(PF_PACKET, SOCK_RAW, htons(myConfig->etherType));

    if(*fd < 0) {
	CCK_PERROR(THIS_COMPONENT"tTransportInit(%s): Failed to open socket!", self->name);
	return -1;
    }

    /* allow address reuse */
    ret = setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &val, valsize);

    if(ret < 0) {
	CCK_DBG(THIS_COMPONENT"tTransportInit(%s): Failed to set SO_REUSEADDR: %s\n", self->name, strerror(errno));
    }

#ifdef SO_BINDTODEVICE
	ret = setsockopt(*fd, SOL_SOCKET, SO_BINDTODEVICE, myConfig->interface, strlen(myConfig->interface));
	if(ret < 0) {
	    CCK_PERROR(THIS_COMPONENT"tTransport_init(%s): Failed to call SO_BINDTODEVICE on %s",
			self->name, myConfig->interface);
	}
#endif /* SO_BINDTODEVICE */

    memcpy(&self->ownAddress, &self->hwAddress, sizeof(CckTransportAddress));

    memset(&bindAddr, 0, sizeof(bindAddr));
    memcpy(&bindAddr.sll_addr, &self->ownAddress.addr.ether, ETHER_ADDR_LEN);
    bindAddr.sll_family = AF_PACKET;
    bindAddr.sll_protocol = htons(myConfig->etherType);
    bindAddr.sll_halen = ETHER_ADDR_LEN;

    ret = getInterfaceIndex(myConfig->interface);

    if(ret < 0) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): Failed to retrieve interface infex. Cannot bind to inteface.\n",
				myConfig->interface);
	goto cleanup;
    }

    bindAddr.sll_ifindex = ret;

    ret = bind(*fd, (struct sockaddr*)&bindAddr, sizeof(bindAddr));

    clearstr(strAddr);
    self->tools->toString(strAddr, strAddr_len, &self->ownAddress);

    if (ret < 0) {
	CCK_PERROR(THIS_COMPONENT"tTransportInit(%s): Failed to bind raw socket on %s (%s, etherType 0x%04x)",
	    self->name, myConfig->interface,strAddr, myConfig->etherType);
	goto cleanup;
    } else {
	CCK_DBG(THIS_COMPONENT"tTransportInit(%s): Bound raw socket on %s (%s, etherType 0x%04x)\n",
	    self->name, myConfig->interface,strAddr, myConfig->etherType);
    }
/*
	mreq.mr_ifindex = bindAddr.sll_ifindex;
	mreq.mr_type = PACKET_MR_ALLMULTI;
	mreq.mr_alen = 0;
	if (!setsockopt(*fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq))) {
		return 0;
	}

	mreq.mr_ifindex = bindAddr.sll_ifindex;
	mreq.mr_type = PACKET_MR_MULTICAST;
	mreq.mr_alen = 0;
	if (!setsockopt(*fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq))) {
		return 0;
	}

	mreq.mr_ifindex = bindAddr.sll_ifindex;
	mreq.mr_type = PACKET_MR_PROMISC;
	mreq.mr_alen = 0;
	if (!setsockopt(*fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq))) {
		return 0;
	}
*/

    /* interface flags */
    ret = setInterfaceFlags(myConfig->interface, IFF_PROMISC);
    if(ret < 0) {
	CCK_ERROR(THIS_COMPONENT"tTransportInit(%s): Could not set interface flags (IFF_PROMISC | IFF_ALLMULTI)\n",
		self->name);
	goto cleanup;
    }

    /* try enabling timestamping if we need it */
    if(self->config.timestamping && !initTimestamping(self, &myData->tsConfig)) {
	CCK_ERROR(THIS_COMPONENT"tTransport_init(%s): Failed to initialise software timestamping!\n",
		self->name);
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

    CCK_NOTICE(THIS_COMPONENT"Transport '%s' (%s) started successfully.\n",
		self->name, myConfig->interface);

    return 1;

cleanup:

    self->shutdown(self);
    return -1;

}

static int
tTransport_shutdown(TTransport *self) {

    CCK_GET_PCONFIG(TTransport, socket_raweth, self, myConfig);

    /* remove our fd from fd set if we have one */
    if(self->fdSet) {
	cckRemoveFd(self->fdSet, &self->myFd);
    }

    /* leave multicast groups if we have them */
    if(self->config.flags | TT_CAPS_MCAST) {

	if(myConfig->multicastStreams) {
	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {
		joinMulticast_ethernet(mcAddr, myConfig->interface, false);
	    }
	}

    }

    clearInterfaceFlags(myConfig->interface, IFF_PROMISC | IFF_ALLMULTI);

    /* close the socket */
    if(self->myFd.fd > -1) {
	close(self->myFd.fd);
    }

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
	CCK_GET_PCONFIG(TTransport, socket_raweth, self, myConfig);

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

    CCK_GET_PDATA(TTransport, socket_raweth, self, myData);
    CCK_GET_PCONFIG(TTransport, socket_raweth, self, myConfig);

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

struct sockaddr_ll dad;
memset(&dad, 0, sizeof(dad));

    memcpy(&dad.sll_addr, &message->destination->addr.ether, ETHER_ADDR_LEN);
    dad.sll_family = AF_PACKET;
//    bindAddr.sll_protocol = htons(myConfig->etherType);
    dad.sll_halen = ETHER_ADDR_LEN;
    dad.sll_ifindex = if_nametoindex(myConfig->interface);

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

#if defined(__QNXNTO__) && defined(PTPD_EXPERIMENTAL)
	getSystemClock()->getTime(getSystemClock(), &message->timestamp);
	message->hasTimestamp = true;
#else
    if(myData->tsConfig.naive) {
	getSystemClock()->getTime(getSystemClock(), &message->timestamp);
	message->hasTimestamp = true;
    }
#endif

/*    ret = send(
		    self->myFd.fd,
		    buf,
		    message->length + self->headerLen,
		    0
		);
*/
    ret = sendto(
		    self->myFd.fd,
		    buf,
		    message->length + self->headerLen,
		    0,
		    (struct sockaddr*)&dad,
		    sizeof(dad)
		);


    sent = ret;

    if(ret <= 0) {
#ifdef CCK_DEBUG
	    tmpstr(strAddr, self->tools->strLen);
	    CCK_PERROR(THIS_COMPONENT"sendMessage(%s): error sending message to %s\n",
		    self->name,
		    self->tools->toString(strAddr, strAddr_len, message->destination));
#endif /* CCK_DEBUG */
	    self->counters.txErrors++;
	    return ret;
    }

    /* loop packet to self if we have no naive timestamp */
//    if(!mc && !message->hasTimestamp) {
	    memcpy(&ethh->ether_dhost, &self->ownAddress.addr.ether, TT_ADDRLEN_ETHERNET);
	    ret = send(
		    self->myFd.fd,
		    buf,
		    message->length + self->headerLen,
		    0
		);

	    if(ret < 0) {
		CCK_DBG(THIS_COMPONENT"sendMessage(%s): error looping packet to self\n",
			self->name);
		self->counters.txTimestampErrors++;
		self->counters.txErrors++;
		return ret;
	    }

//    }

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

    struct sockaddr_ll from;
    struct ether_header *ethh;
    CCK_GET_PDATA(TTransport, socket_raweth, self, myData);
    ssize_t ret = 0;
    char buf[message->capacity + self->headerLen];

    struct msghdr msg;
    struct iovec vec;
    struct cmsghdr *cmsg;

    union {
	    struct cmsghdr cm;
	    char control[256];
    } cun_t;

    if(self->config.disabled) {
	return 0;
    }

    memset(buf, 0, sizeof(buf));
    memset(&from, 0, sizeof(from));
    clearTTransportMessage(message);

    ethh = (struct ether_header*) buf;

#if defined(__QNXNTO__) && defined(PTPD_EXPERIMENTAL)
/* on QNX if EXPERIMENTAL (timer interrupt interpolation), always use system time */
	getSystemClock()->getTime(getSystemClock(), &message->timestamp);
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): QNX: got  system timestamp %d.%d\n",
		self->name, message->timestamp.seconds, message->timestamp.nanoseconds);
	message->hasTimestamp = true;
#else
/* get system timestamp as early as possible */
    if(myData->tsConfig.naive) {
	getSystemClock()->getTime(getSystemClock(), &message->timestamp);
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): got naive system timestamp %d.%d\n",
		self->name, message->timestamp.seconds, message->timestamp.nanoseconds);
	message->hasTimestamp = true;
    }
#endif /* __QNXNTO__ */

    prepareMsgHdr(&msg,
		  &vec,
		  buf,
		  message->capacity + self->headerLen,
		  cun_t.control,
		  sizeof(cun_t.control),
		  (struct sockaddr *)&from,
		  sizeof(from));

    ret = recvmsg(self->myFd.fd, &msg, 0);

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

    if(ret <= 0) {
	if (errno == EAGAIN || errno == EINTR) {
	    CCK_DBG(THIS_COMPONENT"receiveMessage(%s): EAGAIN / EINTR: continuing as normal\n",
		    self->name);
	    return 0;
	}

	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): Error while receiving message: %s\n",
		self->name, strerror(errno));
	self->counters.rxErrors++;
	return ret;
    }

    if (msg.msg_flags & MSG_TRUNC) {
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): Received truncated message (check buffer size - is %d bytes, received %d)\n",
		    self->name, message->capacity, ret);
	return 0;
    }

    if ((msg.msg_flags & MSG_CTRUNC) || (msg.msg_controllen <= 0)) {
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): Received truncated control data (check control buffer size - is %d bytes, received %d, controllen %d)\n",
		    self->name, sizeof(cun_t.control), ret, msg.msg_controllen);
	return 0;
    }

    /* loop through control messages */
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
	 cmsg = CMSG_NXTHDR(&msg, cmsg)) {

	/* grab the timestamp */
	if(self->config.timestamping && !message->hasTimestamp) {
	    if(getCmsgTimestamp (&message->timestamp, cmsg, &myData->tsConfig)) {
		CCK_DBG(THIS_COMPONENT"receiveMessage(%s): got timestamp %d.%d\n",
		    self->name, message->timestamp.seconds, message->timestamp.nanoseconds);
		message->hasTimestamp = true;
	    }
	}

    }

    memcpy(message->data, buf + self->headerLen, ret - self->headerLen);

    message->length = ret - self->headerLen;

    if(self->config.timestamping && !message->hasTimestamp) {
	CCK_DBG(THIS_COMPONENT"receiveMessage(%s): Error: no timestamp received!\n",
		    self->name);
	self->counters.rxTimestampErrors++;
	/* message with no timestamp considered harmful */
	ret = 0;
    }

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
	self->counters.rxBytes += ret;
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

    return 0;

}

static int
refresh(TTransport *self) {

    CCK_GET_PCONFIG(TTransport, socket_raweth, self, myConfig);

    /* join multicast groups if we have them */
    if(self->config.flags | TT_CAPS_MCAST) {

	if(myConfig->multicastStreams) {
	    CckTransportAddress *mcAddr;
	    LL_FOREACH_DYNAMIC(myConfig->multicastStreams, mcAddr) {
		joinMulticast_ethernet(mcAddr, myConfig->interface, true);
	    }
	}

    }

    return 1;

}

static void
loadVendorExt(TTransport *self) {

}

/*
static void
loadVendorExt(TTransport *self) {

	bool found = false;
	char vendorName[100];
	uint32_t oui;
	int (*loader)(TTransport*, const char*);

	int ret = getHwAddrData(ifname, (unsigned char*)&oui, 4);

	oui = ntohl(oui) >> 8;

	if(ret != 1) {
	    CCK_WARNING(THIS_COMPONENT"%s: could not retrieve hardware address for vendor extension check\n",
		    self->name);
	} else {

	    memset(vendorName, 0, 100);
	    CCK_INFO(THIS_COMPONENT"%s: probing for vendor extensions (OUI %06x)\n", self->name,
				oui);

	    #define LOAD_VENDOR_EXT(voui,vendor,name) \
		case voui: \
		    found = true; \
		    strncpy(vendorName, name, 100); \
		    loader = loadCdVendorExt_##vendor; \
		    break;

	    switch(oui) {

		#include "vext/socket_raweth_vext.def"

		default:
		    CCK_INFO(THIS_COMPONENT"%s: no vendor extensions available\n", self->name);
	    }

	    #undef LOAD_VENDOR_EXT

	    if(found) {
		if(loader(self, ifname) < 0) {
		    CCK_ERROR(THIS_COMPONENT"%s: vendor: %s, failed loading extensions, using Linux PHC\n", self->name, vendorName);
		} else {
		    CCK_INFO(THIS_COMPONENT"%s: vendor: %s, extensions loaded successfully\n", self->name, vendorName);
		}
	    }

	}

}
*/

static bool
initTimestamping(TTransport *self,  TTsocketTimestampConfig *config) {

	int level = SOL_SOCKET;
	int option;
	int val = 1;

	config->arrayIndex = 0;
	config->cmsgLevel = SOL_SOCKET;

/* find which timestamp options are available */

#if defined(SO_TIMESTAMPNS) /* begin ifdef block: select available SO_XXXX option */

	CCK_DBG(THIS_COMPONENT"initTimestamping(%s): using SO_TIMESTAMPNS\n", self->name);
	option = SO_TIMESTAMPNS;
	config->cmsgType = SCM_TIMESTAMPNS;
	config->elemSize = sizeof(struct timespec);
	config->convertTs = convertTimestamp_timespec;

#elif defined(SO_BINTIME)

	CCK_DBG(THIS_COMPONENT"initTimestamping(%s): using SO_BINTIME\n", self->name);
	option = SO_BINTIME;
	config->cmsgType = SCM_BINTIME;
	config->elemSize = sizeof(struct bintime);
	config->convertTs = convertTimestamp_bintime;

#elif defined(SO_TIMESTAMP)

	CCK_DBG(THIS_COMPONENT"initTimestamping(%s): using SO_TIMESTAMP\n", self->name);
	option = SO_BINTIME;
	config->cmsgType = SCM_TIMESTAMP;
	config->elemSize = sizeof(struct timeval);
	config->convertTs = convertTimestamp_timeval;

#else /* no SO_XXXX option available */

#if defined(__QNXNTO__) && defined (CCK_EXPERIMENTAL) /* interpolation on QNX is OK, no need for a warning */

	CCK_DBG(THIS_COMPONENT"initTimestamping(%s): using QNX timestamp interpolation\n", self->name);

#else /* not QNX or not CCK_EXPERIMENTAL: warn about poor precision */

	CCK_WARNING("initTimestamping(%s): no timestamp API available, timestamps will be imprecise!\n",
		    self->name);

#endif

	/* we are using naive timestamps, that is it */
	confif->naive = true;
	return true;

#endif /* end ifdef block */


	/* we have an option to try */

	if (setsockopt(self->myFd.fd, level, option, &val, sizeof(int)) < 0) {
		CCK_PERROR("initTimestamping(%s): failed to enable socket timestamps", self->name);
		return false;
	}

	return true;

}
