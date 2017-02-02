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
 * @file   pcap_ethernet.h
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  structure definitions for the Ethernet libpcap timestamping transport
 *
 */

#ifndef CCK_TTRANSPORT_PCAP_ETHERNET_H_
#define CCK_TTRANSPORT_PCAP_ETHERNET_H_

#ifdef HAVE_PCAP_PCAP_H
#include <pcap/pcap.h>
#else /* !HAVE_PCAP_PCAP_H */
/* Cases like RHEL5 and others where only pcap.h exists */
#ifdef HAVE_PCAP_H
#include <pcap.h>
#endif /* HAVE_PCAP_H */
#endif

#include <libcck/ttransport.h>
#include <libcck/net_utils.h>
#include <libcck/transport_address.h>

typedef struct {
    pcap_t *readerHandle;	/* libpcap handle for reading data */
    pcap_t *writerHandle;	/* libpcap handle for writing data */
    bool nanoPrecision;
    CckInterfaceInfo intInfo;
} TTransportData_pcap_ethernet;

typedef struct {
	char interface [IFNAMSIZ];
	CckTransportAddressList *multicastStreams;
	uint16_t etherType;
	uint16_t vlanNumber;
} TTransportConfig_pcap_ethernet;

/* private initialisation, method assignment etc. */
bool _setupTTransport_pcap_ethernet(TTransport *self);

/*
 * initialisation / destruction of any extra data in our private config object
 * when we create a global config object outside of the transport
 */
void _initTTransportConfig_pcap_ethernet(TTransportConfig_pcap_ethernet *myConfig, const int family);
void _freeTTransportConfig_pcap_ethernet(TTransportConfig_pcap_ethernet *myConfig);

/* probe if interface @path supports @flags */
bool _probeTTransport_pcap_ethernet(const char *path, const int flags);

#endif /* CCK_TTRANSPORT_PCAP_ETHERNET_H_ */
