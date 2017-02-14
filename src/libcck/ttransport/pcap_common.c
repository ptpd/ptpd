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
 * @file   pcap_common.c
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  libpcap transport common code
 *
 */

#include <config.h>

#include <libcck/cck_types.h>

#include <errno.h>

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>	/* struct ether_addr */
#endif /* HAVE_NET_ETHERNET_H */

#ifdef HAVE_LINUX_IF_PACKET_H
#include <linux/if_packet.h>
#endif /* HAVE_LINUX_IF_PACKET_H */

#include <libcck/cck.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/ttransport.h>
#include <libcck/ttransport/pcap_common.h>
#include <libcck/net_utils.h>
#include <libcck/clockdriver.h>

#define THIS_COMPONENT "ttransport.pcap: "

