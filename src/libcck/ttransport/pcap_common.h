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
 * @file   pcap_common.h
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  libpcap transport common definitions
 *
 */

#ifndef CCK_TTRANSPORT_PCAP_COMMON_H_
#define CCK_TTRANSPORT_PCAP_COMMON_H_

#ifdef HAVE_PCAP_PCAP_H
#include <pcap/pcap.h>
#else /* !HAVE_PCAP_PCAP_H */
/* Cases like RHEL5 and others where only pcap.h exists */
#ifdef HAVE_PCAP_H
#include <pcap.h>
#endif /* HAVE_PCAP_H */
#endif

#ifdef __sun
#define PCAP_TIMEOUT 10		/* BPF-pcap buffering issues */
#else
#define PCAP_TIMEOUT 1
#endif /* __sun */

#define FILTER_EXPR_LEN 1000	/* maximum PCAP filter expression length */

/* missing on some systems / libpcap versions */
#ifndef PCAP_NETMASK_UNKNOWN
#define PCAP_NETMASK_UNKNOWN 0xffffffff
#endif

#endif /* CCK_TTRANSPORT_PCAP_COMMON_H_ */
