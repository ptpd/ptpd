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
 * @file   cck_transport.h
 * 
 * @brief  libCCK transport component definitions
 *
 */

#ifndef CCK_TRANSPORT_H_
#define CCK_TRANSPORT_H_

#ifndef CCK_H_INSIDE_
#error libCCK component headers should not be uncluded directly - please include cck.h only.
#endif

typedef struct CckTransport CckTransport;

#include "cck.h"

#include "../../config.h"

#include <limits.h>

#ifdef PTPD_PCAP
#ifdef HAVE_PCAP_PCAP_H
#include <pcap/pcap.h>
#else /* !HAVE_PCAP_PCAP_H */
/* Cases like RHEL5 and others where only pcap.h exists */
#ifdef HAVE_PCAP_H
#include <pcap.h>
#endif /* HAVE_PCAP_H */
#endif
#define PCAP_TIMEOUT 1 /* expressed in milliseconds */
#endif


#if !defined(linux) && !defined(__NetBSD__) && !defined(__FreeBSD__) && \
  !defined(__APPLE__) && !defined(__OpenBSD__)
#error PTPD hasn't been ported to this OS - should be possible \
if it's POSIX compatible, if you succeed, report it to ptpd-devel@sourceforge.net
#endif

#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>

#ifdef	linux
#include<netinet/in.h>
#include<net/if.h>
#include<net/if_arp.h>
#include <ifaddrs.h>
#define IFACE_NAME_LENGTH         IF_NAMESIZE
#define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

#define IFCONF_LENGTH 10

#define octet ether_addr_octet
#include<endian.h>
#endif /* linux */



#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <net/if.h>
# include <net/if_dl.h>
# include <net/if_types.h>
#ifdef HAVE_NET_IF_ETHER_H
#  include <net/if_ether.h>
#endif
#ifdef HAVE_SYS_UIO_H
#  include <sys/uio.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#  include <net/ethernet.h>
#endif
#  include <ifaddrs.h>

#endif

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif /* HAVE_NET_ETHERNET_H */

#include <netinet/in.h>
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#include <netinet/ip.h>
#include <netinet/udp.h>
#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif /* HAVE_NETINET_ETHER_H */

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif /* HAVE_NET_IF_ARP_H*/

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif /* HAVE_NET_IF_H*/

#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif /* HAVE_NETINET_IF_ETHER_H */

/* macro to allow registering component implementations */
#define CCK_SETUP_TRANSPORT(type,suffix)\
	if(transport->transportType==type) {\
	    transport->unicastCallback = NULL;\
	    transport->init = cckTransportInit_##suffix;\
	    transport->shutdown = cckTransportShutdown_##suffix;\
	    transport->header.shutdown = cckTransportShutdown_##suffix;\
	    transport->testConfig = cckTransportTestConfig_##suffix;\
	    transport->pushConfig = cckTransportPushConfig_##suffix;\
	    transport->refresh = cckTransportRefresh_##suffix;\
	    transport->hasData = cckTransportHasData_##suffix;\
	    transport->isMulticastAddress = cckTransportIsMulticastAddress_##suffix;\
	    transport->send = cckTransportSend_##suffix;\
	    transport->recv = cckTransportRecv_##suffix;\
	    transport->addressFromString = cckTransportAddressFromString_##suffix;\
	    transport->addressToString = cckTransportAddressToString_##suffix;\
	    transport->addressEqual = cckTransportAddressEqual_##suffix;\
	}

#define CCK_REGISTER_TRANSPORT(type,suffix) \
	if(transportType==type) {\
	    cckTransportSetup_##suffix(transport);\
	}
/* generic transport address descriptor */
typedef union {
        struct sockaddr         inetAddr;
        struct sockaddr_in      inetAddr4;
        struct sockaddr_in6     inetAddr6;
        struct ether_addr       etherAddr;
#if 0
        struct sockaddr_ll      llAdr;
        struct sockaddr_un      unAddr;
#endif
} TransportAddress;

/*
  Configuration data for a transport - some parameters are generic,
  so it's up to the transport implementation how they are used.
  This is easier to use than per-implementation config, although
  this is still possible using configData.
*/
typedef struct {
	/* we probably don't need this, this is passed after create() */
	int transportType;
	/* interface name, unix socket path, file, device, etc. */
	char transportEndpoint[PATH_MAX+1];
	/* additional local endpoint identifier - port for IP transports, ethertype for Ethernet, etc. */
	CckUInt16 sourceId;
	/* destuntion endpoint identifier - port for IP transports, ethertype for Ethernet, etc. */
	CckUInt16 destinationId;
	/* generic int type transport specific parameter #1 - ttl for IP transports */
	int param1;
	/* generic int type transport specific parameter #1 - DSCP bits for IP transports */
	int param2;
	/* software timestamping required - init() / test() will fail if none available */
	CckBool swTimestamping;
	/* hardware timestamping required - init() / test() will fail if none available */
	CckBool hwTimestamping;
	/* Local transport endpoint */
	TransportAddress ownAddress;
	/* Primary destination, used to send data if destination not specified */
	TransportAddress defaultDestination;
	/* Secondary / alternative destination whih can be used to send data to */
	TransportAddress secondaryDestination;
	/* Any implementation-specific config data */
	void* configData;
} CckTransportConfig;

/*
   If a Transport component is assigned a pointer to a CckFdSet,
   it will be maintained as new transport components come and go.
*/
typedef struct {
	/* fds are set and cleared in this set */
	fd_set readFdSet;
	/* this set is copied from readFdSet before the select()call */
	fd_set workReadFdSet;
	/* maxFd tracks the maximum fd number for the select call */
	int maxFd;
} CckFdSet;

/* this type holds socket timestamp capability information */
typedef struct {
	/* socket option set on the socket to enable timestamping */
	int socketOption;
	/* socket control message type to retrieve to get timestamps */
	int scmType;
	/* constant to mark different containers for timestamps - timespec, timeval, bintime */
	CckTimestampType timestampType;
	/* size of the timestamp type */
	int timestampSize;
	/* set to true if transmit timestamps are supported */
	CckBool txTimestamping;
	/* set to true if hardware timestamping is supported */
	CckBool hwTimestamping;
} CckSocketTimestampCaps;

struct CckTransport {
	/* mandatory - this has to be the first member */
	CckComponent header;

	int transportType;
	CckTransportConfig config;
	CckBool txTimestamping;
	CckBool timestamping;

	CckUInt32 receivedMessages;
	CckUInt32 sentMessages;

	/* string descriptor for transport endpoint - interface, unix socket path, etc */
	char transportEndpoint[PATH_MAX+1];
	/* file descriptor for reading / sending data */
	int fd;
	/* container for implementation-specific data */
	void* transportData;
	/* own address, useful for verifying if data is from self, looping data, etc. */
	TransportAddress ownAddress;
	/* decondary source identifier - port for UDP, ethertype for Ethernet, etc. */
	CckUInt16 ownSourceId;
	/* destination data is sent to if not specified in send() */
	TransportAddress defaultDestination;
	/* this is only used for multicast, if additional groups / addresses are needed */
	TransportAddress secondaryDestination;
	/* destination identifier - port for UDP, ethertype for Ethernet, etc. */
	CckUInt16 destinationId;

	/*
	    if populated with an fd_set, transport will make sure
	    that maxFd tracks the highest fd number, and the transport's
	    file descriptor is added to / removed from the fd set
	    when initialising or shutting down. This makes it easier to
	    keep track of multiple transports with select()
	*/
	CckFdSet* watcher;

	/*
	    Callback allowing the transport's user to make changes to data
	    if the destination is unicast (or multicast). If NULL, not used.
	    This is mainly useful for the PTP protocol that has a unicast flag
	    in the message header, but may otherwise be useful as well. Transport
	    user doesn't care if the destination it's sending to is multicast
	    or unicast, but if it has to know or act upon it, it has 
	    isMulticastAddress(), and this callback.
	*/
	void (*unicastCallback) (CckBool, CckOctet*, CckUInt16);

	/* Transport interface */

	/* start up the transport, join multicast groups, etc */
	int (*init) (CckTransport*, const CckTransportConfig*);
	/* leave multicast groups, close FDs, etc */
	int (*shutdown) (void*);
	/* test configuration */
	CckBool (*testConfig) (CckTransport*, const CckTransportConfig*);
	/* Push new configuration */
	int (*pushConfig) (CckTransport*, const CckTransportConfig*);
	/* reload interface address, re-join multicast groups if needed */
	void (*refresh) (CckTransport*);
	/* could be FD_ISSET(watcher->workReadFdSet) */
	CckBool (*hasData) (CckTransport*);
	/* return true if address being checked is multicast */
	CckBool (*isMulticastAddress) (const TransportAddress*);
	/* send x bytes of data from buffer to the specified destination - if enabled and possible, populate tx timestamp */
	ssize_t (*send) (CckTransport*, CckOctet*, CckUInt16, TransportAddress*, CckTimestamp*);
	/* receive up to x bytes of data into buffer, capture source address, and if enabled, populate rx timestamp */
	ssize_t (*recv) (CckTransport*, CckOctet*, CckUInt16, TransportAddress*, CckTimestamp*, int);
	/* parse a string into TransportAddress of the current transport type */
	CckBool (*addressFromString) (const char*, TransportAddress*);
	/* parse a string into TransportAddress of the current transport type */
	char* (*addressToString) (const TransportAddress*);
	/* check if two transport addresses are equal */
	CckBool (*addressEqual) (const TransportAddress*, const TransportAddress*);


};

CckTransport* createCckTransport(int transportType, const char* instanceName);
void freeCckTransport(CckTransport** transport);
void clearTransportAddress(TransportAddress* address);
CckBool transportAddressEmpty(const TransportAddress* address);

/* helper functions - code shared by multiple transport implementations*/
#include "transport/cck_transport_helpers.h"

/* ============ TRANSPORT IMPLEMENTATIONS BEGIN ============ */

/* transport type enum */
enum {
	CCK_TRANSPORT_NULL = 0,
	CCK_TRANSPORT_UDP_IPV4,
	CCK_TRANSPORT_UDP_IPV6,
	CCK_TRANSPORT_ETHERNET
};

/* transport implementation headers */
#include "transport/cck_transport_null.h"
#include "transport/cck_transport_ipv4.h"
#include "transport/cck_transport_ipv6.h"
#include "transport/cck_transport_ethernet.h"

/* ============ TRANSPORT IMPLEMENTATIONS END ============== */

#endif /* CCK_TRANSPORT_H_ */
