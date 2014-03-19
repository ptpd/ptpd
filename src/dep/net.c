/*-
 * Copyright (c) 2014	   Wojciech Owczarek,
 *			   George V. Neville-Neil
 * Copyright (c) 2012-2013 George V. Neville-Neil,
 *                         Wojciech Owczarek.
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments.
 * Copyright (c) 2009-2010 George V. Neville-Neil, 
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen
 *
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
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
 * @file   net.c
 * @date   Tue Jul 20 16:17:49 2010
 *
 * @brief  Functions to interact with the network sockets and NIC driver.
 *
 *
 */

#include "../ptpd.h"

#if defined PTPD_SNMP
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#endif

/* choose kernel-level nanoseconds or microseconds resolution on the client-side */
#if !defined(SO_TIMESTAMPING) && !defined(SO_TIMESTAMPNS) && !defined(SO_TIMESTAMP) && !defined(SO_BINTIME)
#error No kernel-level support for packet timestamping detected!
#endif


/* shut down network transports and ACLs - simple. no errors are reported here */
Boolean 
netShutdown(RunTimeOpts* rtOpts, PtpClock* ptpClock)
{

	/* libCCK free** functions call shutdown() and de-register components for us */

	/* TODO: walk-and-shutdown from registry by component type, don't do it with individual instances! */

	freeCckTransport(&ptpClock->eventTransport);

	/* when running Ethernet transport, this is a pointer to the above */
	if(rtOpts->transport != IEEE_802_3) {
	    freeCckTransport(&ptpClock->generalTransport);
	}

	if(rtOpts->delayMechanism == P2P) {
	    freeCckTransport(&ptpClock->peerEventTransport);
	    /* when running Ethernet transport, this is a pointer to the above */
	    if(rtOpts->transport != IEEE_802_3) {
		freeCckTransport(&ptpClock->peerGeneralTransport);
	    }
	}

	freeCckAcl(&ptpClock->timingAcl);
	freeCckAcl(&ptpClock->managementAcl);

	return TRUE;
}

/* prepare start up network transports, return false on error */
Boolean
netInit(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{

	char *defDest = NULL;
	char *pDest = NULL;

	CckBool *tsMethod;

	/* glorified constructor argument list I suppose */
	CckTransportConfig config;
	memset(&config, 0, sizeof(CckTransportConfig));

	tsMethod = (rtOpts->hwTimestamping) ? &config.hwTimestamping : &config.swTimestamping;

	/* init protocol addresses depending on transport type */
	switch(rtOpts->transport) {

		case UDP_IPV4:
		    defDest = DEFAULT_PTP_IPV4_ADDRESS;
		    pDest = PEER_PTP_IPV4_ADDRESS;
		    if(rtOpts->pcap)
			rtOpts->transportType = CCK_TRANSPORT_PCAP_UDP_IPV4;
		    else
			rtOpts->transportType = CCK_TRANSPORT_SOCKET_UDP_IPV4;
		    break;
		case UDP_IPV6:
		    defDest = DEFAULT_PTP_IPV6_ADDRESS;
		    pDest = PEER_PTP_IPV6_ADDRESS;
		    if(rtOpts->pcap)
			rtOpts->transportType = CCK_TRANSPORT_PCAP_UDP_IPV6;
		    else
			rtOpts->transportType = CCK_TRANSPORT_SOCKET_UDP_IPV6;
		    break;
		case IEEE_802_3:
		    defDest = DEFAULT_PTP_ETHERNET_ADDRESS;
		    pDest = PEER_PTP_ETHERNET_ADDRESS;
		    rtOpts->transportType = CCK_TRANSPORT_PCAP_ETHERNET;
		    break;
		default:
		    ERROR("Unsupported transport: %02x\n",
			    rtOpts->transport);
	}

	config.param3 = PACKET_SIZE;

	config.transportType = rtOpts->transportType;
	strncpy(config.transportEndpoint, rtOpts->ifaceName, IFACE_NAME_LENGTH);

	/* create standard transports */

	ptpClock->eventTransport = createCckTransport(rtOpts->transportType, "event");
	ptpClock->eventTransport->unicastCallback = setPtpUnicastFlags;
	ptpClock->eventTransport->watcher = &ptpClock->watcher;

	/* PTP over Ethernet effectively uses one none-peer transport */
	if(rtOpts->transport != IEEE_802_3) {
	    ptpClock->generalTransport = createCckTransport(rtOpts->transportType, "general");
	    ptpClock->generalTransport->unicastCallback = setPtpUnicastFlags;
	    ptpClock->generalTransport->watcher = &ptpClock->watcher;
	} else {
	    ptpClock->generalTransport = ptpClock->eventTransport;
	}


	/* prepare event transport */

	/* libCCK Ethernet transport uses source/destid to set ethertype */
	if(rtOpts->transport == IEEE_802_3) {
	    config.sourceId = PTP_ETHER_TYPE;
	    config.destinationId = PTP_ETHER_TYPE;
	} else {
	    config.sourceId = PTP_EVENT_PORT;
	    config.destinationId = PTP_EVENT_PORT;
	    config.param1 = rtOpts->ttl;
	}

	*tsMethod = CCK_TRUE;
	config.param2 = rtOpts->dscpValue;

	/* populate config with source address if set - transport will try to bind to this source */
	if(strlen(rtOpts->sourceAddress)>0) {
		if (!(ptpClock->eventTransport->addressFromString(rtOpts->sourceAddress,
					    &config.ownAddress)))
		    return FALSE;
	}

	/* set unicast destination if provided */
	if(strlen(rtOpts->unicastAddress)>0) {
		if (!(ptpClock->eventTransport->addressFromString(rtOpts->unicastAddress,
				    &config.defaultDestination)))
		    return FALSE;
	/* otherwise use default destination for both transports, but set scope if IPv6 */
	} else {
		if (!(ptpClock->eventTransport->addressFromString(defDest,
				    &config.defaultDestination)))
		return FALSE;
		if(rtOpts->transport == UDP_IPV6)
		    config.defaultDestination.inetAddr6.sin6_addr.s6_addr[1] = rtOpts->ipv6Scope;
	}

	/* showtime - start up event transport */
	if((ptpClock->eventTransport->init(ptpClock->eventTransport, &config)) < 1)
	    return FALSE;

	/* PTP over Ethernet not use separate event / general transports, just one non-peer and one peer */
	if(rtOpts->transport != IEEE_802_3) {

		/* prepare general transport - it shares most settings with event transport */
		config.sourceId = PTP_GENERAL_PORT;
		config.destinationId = PTP_GENERAL_PORT;
		*tsMethod = CCK_FALSE;

		/* showtime - start up general transport */
		if((ptpClock->generalTransport->init(ptpClock->generalTransport, &config)) < 1)
			return FALSE;
	}

	/* init P2P transport(s)*/

	if(rtOpts->delayMechanism == P2P) {

	    /* create peer transports */
	    ptpClock->peerEventTransport = createCckTransport(rtOpts->transportType, "peerEvent");
	    ptpClock->peerEventTransport->unicastCallback = setPtpUnicastFlags;
	    ptpClock->peerEventTransport->watcher = &ptpClock->watcher;

	    /* PTP over Ethernet effectively uses one peer transport */
	    if(rtOpts->transport != IEEE_802_3) {
		ptpClock->peerGeneralTransport = createCckTransport(rtOpts->transportType, "peerGeneral");
		ptpClock->peerGeneralTransport->unicastCallback = setPtpUnicastFlags;
		ptpClock->peerGeneralTransport->watcher = &ptpClock->watcher;
	    } else {
		ptpClock->peerGeneralTransport = ptpClock->peerEventTransport;
	    }

	    /* prepare peer event transport */

	    *tsMethod = CCK_TRUE;

	    /* libCCK Ethernet transport uses source/destid to set ethertype */
	    if(rtOpts->transport == IEEE_802_3) {
		config.sourceId = PTP_ETHER_TYPE;
		config.destinationId = PTP_ETHER_TYPE;
	    } else {
		config.sourceId = PTP_EVENT_PORT;
		config.destinationId = PTP_EVENT_PORT;
		/* peer transport config - ttl 1 */
		config.param1 = 1;
	    }

	    /* only the peer destination is used for p2p - correct? */
	    if (!(ptpClock->peerEventTransport->addressFromString(pDest,
				    &config.defaultDestination)))
		return FALSE;

	    /* showtime - start up peer event transport */
	    if((ptpClock->peerEventTransport->init(ptpClock->peerEventTransport, &config)) < 1)
		return FALSE;

	    /* PTP over Ethernet not use separate event / general transports, just one non-peer and one peer */
	    if(rtOpts->transport != IEEE_802_3) {

		    /* prepare peer general transport */

		    config.sourceId = PTP_GENERAL_PORT;
		    config.destinationId = PTP_GENERAL_PORT;
		    *tsMethod = CCK_FALSE;

	    /* only the peer destination is used for p2p - correct? */
		    if (!(ptpClock->peerGeneralTransport->addressFromString(pDest,
				    &config.defaultDestination)))
			return FALSE;

		    /* showtime - start up peer general transport */
		    if((ptpClock->peerGeneralTransport->init(ptpClock->peerGeneralTransport, &config)) < 1)
			return FALSE;
	    }
	}

	/* Compile ACLs corresponding to transport types*/

	if(rtOpts->timingAclEnabled) {
    		freeCckAcl(&ptpClock->timingAcl);
		ptpClock->timingAcl = createCckAcl(ptpClock->eventTransport->aclType,
						    rtOpts->timingAclOrder, "timingAcl");
		ptpClock->timingAcl->compileAcl(ptpClock->timingAcl,
						rtOpts->timingAclPermitText,
						rtOpts->timingAclDenyText);
	}

	if(rtOpts->managementAclEnabled) {
    		freeCckAcl(&ptpClock->managementAcl);
		ptpClock->managementAcl = createCckAcl(ptpClock->eventTransport->aclType,
						    rtOpts->managementAclOrder, "managementAcl");
		ptpClock->managementAcl->compileAcl(ptpClock->managementAcl,
						rtOpts->managementAclPermitText,
						rtOpts->managementAclDenyText);
	}


	/* Try to init transport ID - our transports should have collected the hardware address already */

        if(transportAddressEmpty(&ptpClock->eventTransport->hardwareAddress)) {

    	    /* No HW address, we'll use the protocol address to form interfaceID -> clockID. Not 1588 compliant, but allows to use any interface */

	    Octet* addr = NULL;
	    /* stick the outer octets of the L3 address into the outer octers of the "fake MAC" address */
	    switch(rtOpts->transport) {
		case UDP_IPV4:
		    addr=(Octet*)&(ptpClock->eventTransport->ownAddress.inetAddr4.sin_addr.s_addr);
		    memcpy(ptpClock->transportID, addr, 2);
		    memcpy(ptpClock->transportID + 4, addr + 2, 2);
		    break;
		case UDP_IPV6:
		    addr=(Octet*)&(ptpClock->eventTransport->ownAddress.inetAddr6.sin6_addr.s6_addr);
		    memcpy(ptpClock->transportID , addr + 9, 2);
		    memcpy(ptpClock->transportID + 3, addr + 12, 2);
		    break;
		/*
		    Other transport types supported by PTP are all Ethernet based,
		    so we shouldn't be here if we don't have a MAC address
		*/
		default:
		    ERROR("Cannot run layer 2 transport with no hardware address \n");
		    return FALSE;
	    }

	    /* meh.... */
	    CCK_DBGV("Transport ID generated for %s: %02x:%02x:%02x:%02x:%02x:%02x\n",
                        rtOpts->ifaceName,
                        *((CckUInt8*)&ptpClock->transportID),
                        *((CckUInt8*)&ptpClock->transportID + 1),
                        *((CckUInt8*)&ptpClock->transportID + 2),
                        *((CckUInt8*)&ptpClock->transportID + 3),
                        *((CckUInt8*)&ptpClock->transportID + 4),
                        *((CckUInt8*)&ptpClock->transportID + 5));

        /* Otherwise initialise interfaceID with hardware address */
        } else {
                memcpy(ptpClock->transportID, &ptpClock->eventTransport->hardwareAddress, 6);
        }

	return TRUE;
}

/* test network configuration - like a netInit dry run, minus actual transport startup. return false on error */
Boolean testNetworkConfig(RunTimeOpts* rtOpts) {

	INFO("Testing network configuration\n");

	char* defDest = NULL;
	char* pDest = NULL;

	Boolean ret = FALSE;

	CckBool *tsMethod;

	CckTransportConfig config;

	memset(&config, 0, sizeof(CckTransportConfig));

	tsMethod = (rtOpts->hwTimestamping) ? &config.hwTimestamping : &config.swTimestamping;

	switch(rtOpts->transport) {

		case UDP_IPV4:
		    defDest = DEFAULT_PTP_IPV4_ADDRESS;
		    pDest = PEER_PTP_IPV4_ADDRESS;
		    if(rtOpts->pcap)
			rtOpts->transportType = CCK_TRANSPORT_PCAP_UDP_IPV4;
		    else
			rtOpts->transportType = CCK_TRANSPORT_SOCKET_UDP_IPV4;
		    break;
		case UDP_IPV6:
		    defDest = DEFAULT_PTP_IPV6_ADDRESS;
		    pDest = PEER_PTP_IPV6_ADDRESS;
		    if(rtOpts->pcap)
			rtOpts->transportType = CCK_TRANSPORT_PCAP_UDP_IPV6;
		    else
			rtOpts->transportType = CCK_TRANSPORT_SOCKET_UDP_IPV6;
		    break;
		case IEEE_802_3:
		    defDest = DEFAULT_PTP_ETHERNET_ADDRESS;
		    pDest = PEER_PTP_ETHERNET_ADDRESS;
		    rtOpts->transportType = CCK_TRANSPORT_PCAP_ETHERNET;
		    break;
		default:
		    ERROR("Unsupported transport: %02x\n",
			    rtOpts->transport);
	}

	config.param3 = PACKET_SIZE;

	/* create a test transport */
	CckTransport* transport = createCckTransport(rtOpts->transportType, "testNetworkConfig");

	config.transportType = rtOpts->transportType;

	strncpy(config.transportEndpoint, rtOpts->ifaceName, IFACE_NAME_LENGTH);

	/* set up test transport with event transport settings */

	/* libCCK Ethernet transport uses source/destid to set ethertype */
	if(rtOpts->transport == IEEE_802_3) {
	    config.sourceId = PTP_ETHER_TYPE;
	    config.destinationId = PTP_ETHER_TYPE;
	} else {
	    config.sourceId = PTP_EVENT_PORT;
	    config.destinationId = PTP_EVENT_PORT;
	}

	*tsMethod  = CCK_TRUE;
	config.param1 = rtOpts->ttl;
	config.param2 = rtOpts->dscpValue;

	/* init source address if provided */
	if(strlen(rtOpts->sourceAddress)>0) {
		if (!(transport->addressFromString(rtOpts->sourceAddress, &config.ownAddress)))
		    goto end;
	}

	/*  init unicast destination - do not accept a multicast address as unicast dest - those are restricted by IEEE 1588 */
	if(strlen(rtOpts->unicastAddress)>0) {
		if (!(transport->addressFromString(rtOpts->unicastAddress, &config.defaultDestination)))
		    goto end;
		if(transport->isMulticastAddress(&config.defaultDestination)) {
			ERROR("Configured unicast destination is a multicast address\n");
			goto end;
		}
	} else {
		/* default destination */
		if (!(transport->addressFromString(defDest, &config.defaultDestination)))
		goto end;

		/* ipv6 multicast scope - luckily the PTP multicast IPv6 addresses have zeros in third nibble */
		if(rtOpts->transport == UDP_IPV6)
		    config.defaultDestination.inetAddr6.sin6_addr.s6_addr[1] = rtOpts->ipv6Scope;
	}

	/* showtime - test event transport settings */
	if((transport->testConfig(transport, &config)) < 1)
	    goto end;

	/* PTP over Ethernet not use separate event / general transports, just one non-peer and one peer */
	if(rtOpts->transport != IEEE_802_3) {

	    /* test general transport */
	    config.sourceId = PTP_GENERAL_PORT;
	    config.destinationId = PTP_GENERAL_PORT;
	    *tsMethod = CCK_FALSE;

	    if((transport->testConfig(transport, &config)) < 1)
		goto end;

	}

	/* test P2P transports */
	if(rtOpts->delayMechanism == P2P) {

	    /* set up test peer event transport config */

	    /* ttl 1 for the transports that respect this*/
	    config.param1 = 1;

	    /* libCCK Ethernet transport uses source/destid to set ethertype */
	    if(rtOpts->transport == IEEE_802_3) {
		config.sourceId = PTP_ETHER_TYPE;
		config.destinationId = PTP_ETHER_TYPE;
	    } else {
		config.sourceId = PTP_EVENT_PORT;
		config.destinationId = PTP_EVENT_PORT;
	    }

	    *tsMethod = CCK_TRUE;

	    if (!(transport->addressFromString(pDest,
			    &config.defaultDestination)))
		goto end;

	    /* showtime - test peer event transport settings */
	    if((transport->testConfig(transport, &config)) < 1)
		goto end;

	    /* PTP over Ethernet not use separate event / general transports, just one non-peer and one peer */
	    if(rtOpts->transport != IEEE_802_3) {

		/* set up test peer general transport config */
		config.sourceId = PTP_GENERAL_PORT;
		config.destinationId = PTP_GENERAL_PORT;
		*tsMethod = CCK_FALSE;

		/* showtime - test peer general transport settings */
		if((transport->testConfig(transport, &config)) < 1)
		    goto end;

	    }

	}

	/* note that we don't test ACLs here, should we ? this is done separately during config test stage - good, bad? */

	ret = TRUE;

	INFO("Network configuration OK\n");

    end:

    freeCckTransport(&transport);
    return ret;

}

/*Check if data has been received*/
int 
netSelect(TimeInternal * timeout, fd_set *readfds)
{
	int ret = 0;
#if defined PTPD_SNMP
	int nfds;
	struct timeval tv, *tv_ptr;

	extern RunTimeOpts rtOpts;
	struct timeval snmp_timer_wait = { 0, 0}; // initialise to avoid unused warnings when SNMP disabled
	int snmpblock = 0;

if (rtOpts.snmp_enabled) {
	snmpblock = 1;
	if (tv_ptr) {
		snmpblock = 0;
		memcpy(&snmp_timer_wait, tv_ptr, sizeof(struct timeval));
	}
	snmp_select_info(&nfds, readfds, &snmp_timer_wait, &snmpblock);
	if (snmpblock == 0)
		tv_ptr = &snmp_timer_wait;

	ret = select(nfds, readfds, 0, 0, tv_ptr);

	if (ret < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return 0;
	}

	/* Maybe we have received SNMP related data */
	if (ret > 0) {
		snmp_read(readfds);
	} else if (ret == 0) {
		snmp_timeout();
		run_alarms();
	}
	netsnmp_check_outstanding_agent_requests();

}
#endif
	return ret;
}

/* re-join multicast groups. don't leve groups so as not to generate unnecessary PIM / IGMP snooping / MLD churn */
Boolean
netRefresh(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	DBG("netRefresh\n");

	ptpClock->eventTransport->refresh(ptpClock->eventTransport);
	ptpClock->generalTransport->refresh(ptpClock->generalTransport);

	if(rtOpts->delayMechanism == P2P) {
	    ptpClock->peerEventTransport->refresh(ptpClock->peerEventTransport);
	    ptpClock->peerGeneralTransport->refresh(ptpClock->peerGeneralTransport);
	}

	return TRUE;
}
