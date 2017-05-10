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
 * @file   dlpi_common.c
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  dlpi transport common code
 *
 */

#include <config.h>

#include <errno.h>
#include <sys/stropts.h>

#include <libcck/cck_types.h>
#include <libcck/cck.h>
#include <libcck/cck_logger.h>
#include <libcck/ttransport.h>
#include <libcck/ttransport/dlpi_common.h>
#include <libcck/net_utils.h>

#define THIS_COMPONENT "ttransport.dlpi: "

/* helper function for the I_STR ioctl */
static int strIoctlHelper(int fd, int cmd, int timeout, int len, void *buf);

/* push a word into a packetfilt structure */
void
pfPush(struct pfstruct *pf, const uint16_t word)
{

    if(pf == NULL) {
	return;
    }

    /* keep two last entries free */
    if(pf->Pf_FilterLen > (PFMAXFILT -2)) {
	CCK_DBG(THIS_COMPONENT"pfPush(): cannot push more entries into pf filter\n");
	return;
    }


    pf->Pf_Filter[pf->Pf_FilterLen++] = word;

}

/* match a specific ethertype */
void pfMatchEthertype(struct pfstruct *pf, const uint16_t ethertype)
{

    if(pf == NULL) {
	return;
    }

    PFPUSH(ENF_PUSHWORD + 6);		/* 6 words = 12 bytes past header = ethertype */
    PFPUSH(ENF_PUSHLIT | ENF_EQ );	/* push ethertype, check if equal */
    PFPUSH(htons(ethertype));
}


/* Match specific VLAN ID and skip past VLAN header */
void
pfMatchVlan(struct pfstruct *pf, const uint16_t vid)
{

    if(pf == NULL) {
	return;
    }

    pfMatchEthertype(pf, ETHERTYPE_VLAN);
    pfMatchWord(pf, 14, htons(vid));
    PFPUSH(ENF_AND);
    PFPUSH(ENF_BRFL);
    PFPUSH(2);
    PFPUSH(ENF_LOAD_OFFSET);
    PFPUSH(2);

}

/* skip VLAN header if present */
void pfSkipVlan(struct pfstruct *pf)
{

    if(pf == NULL) {
	return;
    }

    pfMatchEthertype(pf, ETHERTYPE_VLAN);
    PFPUSH(ENF_BRFL);			/* branch if false */
    PFPUSH(3);				/* skip 3 words:  */
    PFPUSH(ENF_LOAD_OFFSET);		/*  1: we have a VLAN tag, skip it */
    PFPUSH(2);				/*  2: 2 words = 4 bytes */
    PFPUSH(ENF_POP);			/*  3: pop last result off the stack */

}


/* match a specific byte at a specific offset */
void pfMatchByte(struct pfstruct *pf, const int offset, const uint8_t byte)
{

    /* odd offset: lower bits */
    if(offset % 2 ) {

	PFPUSH(ENF_PUSHWORD + offset / 2);
	PFPUSH(ENF_PUSHLIT | ENF_AND);
	PFPUSH(htons(0x00FF));
	PFPUSH(ENF_PUSHLIT | ENF_EQ);
	PFPUSH(htons((uint16_t)byte & 0x00FF));

    /* even offset: upper bits*/
    } else {

	PFPUSH(ENF_PUSHWORD + offset / 2);
	PFPUSH(ENF_PUSHLIT | ENF_AND);
	PFPUSH(htons(0xFF00));
	PFPUSH(ENF_PUSHLIT | ENF_EQ);
	PFPUSH(htons((uint16_t)byte << 8));

    }


}

/* match a specific word at a specific offset */
void pfMatchWord(struct pfstruct *pf, const int offset, const uint16_t word)
{

    /* odd: match this byte and the next */
    if(offset % 2) {

	pfMatchByte(pf, offset, (word >> 8) & 0x00FF);
	pfMatchByte(pf, offset + 1, word & 0x00FF);
	PFPUSH(ENF_AND);

    /* even: match this word */
    } else {

	PFPUSH(ENF_PUSHWORD + offset / 2);
	PFPUSH(ENF_PUSHLIT | ENF_EQ);
	PFPUSH(word);

    }

}

/* match the whole contents of a buffer at a specific offset, using whole words when possible */
void pfMatchData(struct pfstruct *pf, int offset, const void *buf, const size_t len)
{

    int left = len;
    bool first = true;
    const char *marker = buf;

    if(len == 0 || buf == NULL) {
	return;
    }

    /* first match on the first byte if need be, to align to a full word */
    if (offset % 2) {

	pfMatchByte(pf, offset++, (uint8_t)*(marker++));
	left--;
	first = false;

    }

    /* then match the rest, word by word, or one byte if it's the last) */
    while (left) {

	if(left < 2) {
	    pfMatchByte(pf, offset++, *((uint8_t*)marker++));
	    left--;
	} else {
	    pfMatchWord(pf, offset, (*((uint16_t*)marker)));
	    offset += 2;
	    left -= 2;
	    marker += 2;
	}

	/* if this is not the first match, push an AND */
	if(!first) {
	    PFPUSH(ENF_AND);
	}

	first = false;

    }

}

/* match transport address at given offset */
void
pfMatchAddress(struct pfstruct *pf, CckTransportAddress *addr, const int offset)
{

    if(pf == NULL || addr == NULL || offset == 0) {
	return;
    }

    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	return;
    }

    pfMatchData(pf, offset, ts->getData(addr), ts->addrSize);

}

/* match transport address of given direction (source, destination, any) */
void
pfMatchAddressDir(struct pfstruct *pf, CckTransportAddress *addr, const int direction)
{

    int srcOffset, dstOffset;

    if(pf == NULL || addr == NULL) {
	return;
    }

    /* establish source and destination address offsets */
    switch(addr->family) {

	case TT_FAMILY_IPV4:

	    srcOffset = TT_HDRLEN_ETHERNET + 12;
	    dstOffset = TT_HDRLEN_ETHERNET + 16;

	    break;

	case TT_FAMILY_IPV6:

	    srcOffset = TT_HDRLEN_ETHERNET + 8;
	    dstOffset = TT_HDRLEN_ETHERNET + 24;

	    break;

	case TT_FAMILY_ETHERNET:

	    srcOffset = 6;
	    dstOffset = 0;

	    break;

	default:
	    CCK_DBG(THIS_COMPONENT"pfMatchAddressDir(): unsupported address family %d\n", addr->family);
	    return;

    }

    /* match the address in given direction */
    switch(direction) {

	case PFD_TO:

	    pfMatchAddress(pf, addr, dstOffset);
	    break;

	case PFD_FROM:

	    pfMatchAddress(pf, addr, srcOffset);
	    break;

	case PFD_ANY:
	default:

	    /* match source OR destination */
	    pfMatchAddress(pf, addr, srcOffset);
	    pfMatchAddress(pf, addr, dstOffset);
	    PFPUSH(ENF_OR);

    }

}

/* match specified IP protocol in IPv4 or IPv6 */
void
pfMatchIpProto(struct pfstruct *pf, const int family, const uint8_t proto)
{

    int offset = (family == TT_FAMILY_IPV4) ? (TT_HDRLEN_ETHERNET + 9) : (family == TT_FAMILY_IPV6) ? (TT_HDRLEN_ETHERNET + 6) : 0;

    if(offset == 0) {

	CCK_DBG(THIS_COMPONENT"pfMatchIpProto(): unsupported address family %d\n", family);
	return;


    }

    pfMatchByte(pf, offset, proto);

}

/* dump pf filter - hex only "for now" */
void pfDump(struct pfstruct *pf)
{

    for(int i = 0; i < pf->Pf_FilterLen; i++) {

	CCK_INFO(THIS_COMPONENT"pf filter: %03d: 0x%04x\n", i, (pf->Pf_Filter[i]));

    }


}

/* I_STR helper function */
static int
strIoctlHelper(int fd, int cmd, int timeout, int len, void *buf)
{

    struct strioctl strio = { cmd, timeout, len, buf };

    int ret;

    ret = ioctl(fd, I_STR, &strio);

    if (ret >= 0) {
	ret = strio.ic_len;
    }

    return ret;

}

/* initialise a DLPI handle and set all options required, return the fd */
int
dlpiInit(dlpi_handle_t *dh, const char *ifName, const bool promisc, struct timeval timeout, uint32_t chunksize, uint32_t snaplen, struct pfstruct *pf, uint_t sap)
{

    int fd = -1;
    int ret;

    if(sap == 0) {
	sap = DLPI_ANY_SAP;
    }

    /* open */
    ret = dlpi_open(ifName, dh, DLPI_PASSIVE | DLPI_RAW);

    if (ret != DLPI_SUCCESS) {

	CCK_ERROR(THIS_COMPONENT"dlpiInit(%s): could not open DLPI handle (%s)\n",
			ifName, (ret == DL_SYSERR) ? strerror(errno) : dlpi_strerror(ret));
	return -1;


    }

    /* bind */
    ret = dlpi_bind(*dh, DLPI_ANY_SAP, NULL);

    if (ret != DLPI_SUCCESS) {

	CCK_ERROR(THIS_COMPONENT"dlpiInit(%s): could not bind DLPI handle (%s)\n",
			ifName, (ret == DL_SYSERR) ? strerror(errno) : dlpi_strerror(ret));
	return -1;


    }

    /* set promisc modes */
    if(promisc) {

	/* unfortunately this is basically required to see our own transmitted packets */
	ret = dlpi_promiscon(*dh, DL_PROMISC_PHYS);

	if (ret != DLPI_SUCCESS) {

	    CCK_ERROR(THIS_COMPONENT"dlpiInit(%s): could not set DLPI _PHYS promiscuous mode (%s)\n",
			ifName, (ret == DL_SYSERR) ? strerror(errno) : dlpi_strerror(ret));
	    return -1;

	}

	ret = dlpi_promiscon(*dh, DL_PROMISC_SAP);

	if (ret != DLPI_SUCCESS) {

	    CCK_ERROR(THIS_COMPONENT"dlpiInit(%s): could not set DLPI _SAP promiscuous mode (%s)\n",
			ifName, (ret == DL_SYSERR) ? strerror(errno) : dlpi_strerror(ret));
	    return -1;

	}

	ret = dlpi_promiscon(*dh, DL_PROMISC_MULTI);

	if (ret != DLPI_SUCCESS) {

	    CCK_ERROR(THIS_COMPONENT"dlpiInit(%s): could not set DLPI _SAP promiscuous mode (%s)\n",
			ifName, (ret == DL_SYSERR) ? strerror(errno) : dlpi_strerror(ret));
	    return -1;

	}


    }

    /* get FD */
    fd = dlpi_fd(*dh);

    if(fd < 0) {
	    CCK_PERROR(THIS_COMPONENT"dlpiInit(%s): could not get file descriptor from DLPI handle",
		ifName);
	    return -1;
    }

    /* push pfmod to STREAMS if we have a filter */
    if (pf && (pf->Pf_FilterLen > 0)) {

	ret = ioctl(fd, I_PUSH, "pfmod");

	if (ret < 0) {
	    CCK_PERROR(THIS_COMPONENT"dlpiInit(%s): could not push pfmod to DLPI handle",
			ifName);
	    return -1;
	}

	ret = strIoctlHelper(fd, PFIOCSETF, -1, sizeof(struct pfstruct), (char*)pf);

	if (ret < 0) {
	    CCK_PERROR(THIS_COMPONENT"dlpiInit(%s): could not set pfmod filter on DLPI handle",
			ifName);
	    return -1;
	}

    }

    /* push bufmod to STREAMS so we get a header with timestamps */
    ret = ioctl(fd, I_PUSH, "bufmod");

    if (ret < 0) {
	CCK_PERROR(THIS_COMPONENT"dlpiInit(%s): could not push bufmod to DLPI handle",
		ifName);
	return -1;
    }

    /* set bufmod options */

    ret = strIoctlHelper(fd, SBIOCSTIME, -1, sizeof(struct timeval), (char*)&timeout);

    if (ret < 0) {
	CCK_PERROR(THIS_COMPONENT"dlpiInit(%s): could not set DLPI timeout\n",
		ifName);
	return -1;
    }

    ret = strIoctlHelper(fd, SBIOCSCHUNK, -1, sizeof(chunksize), (char*)&chunksize);

    if (ret < 0) {
	CCK_PERROR(THIS_COMPONENT"dlpiInit(%s): could not set DLPI chunk size",
		ifName);
	return -1;
    }

    ret = strIoctlHelper(fd, SBIOCSSNAP, -1, sizeof(snaplen), (char*)&snaplen);

    if (ret < 0) {
	CCK_PERROR(THIS_COMPONENT"dlpiInit(%s): could not set DLPI snap length",
		ifName);
	return -1;
    }

    /* flush anything that might have arrived before we completed */
    ret = ioctl(fd, I_FLUSH, FLUSHR);

    if(ret < 0) {
	CCK_WARNING(THIS_COMPONENT"dlpiInit(%s): could not flush DLPI handle",
		ifName);
    }

    return fd;

}
