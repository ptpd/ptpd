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
 * @file   dlpi_common.h
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  libdlpi transport common definitions
 *
 */

#ifndef CCK_TTRANSPORT_DLPI_COMMON_H_
#define CCK_TTRANSPORT_DLPI_COMMON_H_

#ifdef HAVE_SYS_DLPI_H
#include <sys/dlpi.h>
#endif /* HAVE_SYS_DLPI_H */

#ifdef HAVE_LIBDLPI_H
#include <libdlpi.h>
#endif /* HAVE_LIBDLPI_H */

#ifdef HAVE_SYS_BUFMOD_H
#include <sys/bufmod.h>
#endif /* HAVE_SYS_BUFMOD_H */

#ifdef HAVE_SYS_PFMOD_H
#include <sys/pfmod.h>
#endif /* HAVE_SYS_PFMOD_H */

/* see if we can use Pf_ext_packetfilt or plain packetfilt */
#ifdef HAVE_STRUCT_PF_EXT_PACKETFILT_PF_PRIORITY
#define pfstruct Pf_ext_packetfilt
#define PFMAXFILT PF_MAXFILTERS
#else
#define pfstruct packetfilt
#define PFMAXFILT ENMAXFILTERS
#endif /* HAVE_STRUCT_PF_EXT_PACKETFILT_PF_PRIORITY */

#define PFPUSH(word) pfPush(pf, word)

/* direction of address to match */
enum {
	PFD_ANY,
	PFD_TO,
	PFD_FROM
};

/* packet filter helper functions */
void pfPush(struct pfstruct *pf, const uint16_t word); /* push a word onto the filter stack */
void pfMatchEthertype(struct pfstruct *pf, const uint16_t ethertype); /* match a specific ethertype */
void pfMatchVlan(struct pfstruct *pf, const uint16_t vid); /* Match specific VLAN ID */
void pfSkipVlan(struct pfstruct *pf); /* skip VLAN header if present */
void pfMatchByte(struct pfstruct *pf, const int offset, const uint8_t byte); /* match a byte at specific offset */
void pfMatchWord(struct pfstruct *pf, const int offset, const uint16_t word); /* match a word at a specific offset */
void pfMatchData(struct pfstruct *pf, int offset, const void *buf, const size_t len); /* match the contents of a buffer at a specific offset */
void pfMatchAddress(struct pfstruct *pf, CckTransportAddress *addr, const int offset); /* match a transport address at a specific offset */
void pfMatchAddressDir(struct pfstruct *pf, CckTransportAddress *addr, const int direction); /* match a transport address in specified direction (to/from/any) */
void pfMatchIpProto(struct pfstruct *pf, const int family, const uint8_t proto); /* match IP protocol number */
void pfDump(struct pfstruct *pf); /* dump PF code */


/* DLPI setup functions */

/* open, bind and prepare a DLPI handle, return the fd */
int dlpiInit(dlpi_handle_t *dh, const char *ifName, const bool promisc, struct timeval timeout, uint32_t chunksize, uint32_t snaplen, struct pfstruct *pf, uint_t sap);


#endif /* CCK_TTRANSPORT_DLPI_COMMON_H_ */
