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
 * @file   getifaddrs_solaris.c
 * @date   Sat Jan 9 16:14:10 2017
 *
 * @brief  getifaddrs() implementation for Solaris systems
 *
 */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/sockio.h>
#include <sys/socket.h>
#include <net/if.h>

#include "ifaddrs.h"

#define FREENOTNULL(var) if(var != NULL) { free(var); }

static int populate_lifreq(struct lifreq **out, int fd);
static int populate_ifaddr(struct ifaddrs *ifa, int fd, struct lifreq *lifrp);
static void freeifaddr(struct ifaddrs *ifa);

/* get lifreq data */
static int
populate_lifreq(struct lifreq **out, int fd)
{
	struct lifnum lifn;
	struct lifconf lifc;
	int ifcount;
	char *buf;

	size_t bufsize;

	/* AF_UNSPEC - look at any address family */
	lifn.lifn_family = AF_UNSPEC;
	lifn.lifn_flags = 0;

again:

	if (ioctl(fd, SIOCGLIFNUM, &lifn) == -1) {
		return -1;
	}

	/* add 4 for newly plumbed interfaces, who knows what evil lurks */
	bufsize = (lifn.lifn_count + 4) * sizeof (struct lifreq);

	if((buf = malloc(bufsize)) == NULL) {
	    return -1;
	}

	lifc.lifc_len = bufsize;
	/* AF_UNSPEC - look at any address family */
	lifc.lifc_family = AF_UNSPEC;
	lifc.lifc_flags = 0;
	lifc.lifc_buf = buf;

	if (ioctl(fd, SIOCGLIFCONF, &lifc) <0 ) {
	    free(buf);
	    return -1;
	}

	ifcount = lifc.lifc_len / sizeof(struct lifreq);

	/* more interfaces might have appeared */
	if(ifcount >= lifn.lifn_count + 4 ) {
		free(buf);
		goto again;
	}

	*out = (struct lifreq*)buf;

	return ifcount;
}

/* populate an ifaddr struct from a lifreq struct */
static int
populate_ifaddr(struct ifaddrs *ifa, int fd, struct lifreq *lifr)
{

	size_t slen = (lifr->lifr_addr.ss_family == AF_INET6) ? 
		sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);

	ifa->ifa_name=strdup(lifr->lifr_name);

	if (ioctl(fd, SIOCGLIFFLAGS, lifr) < 0) {

	    ifa->ifa_flags = 0;
	    return -1;

	} else {
	    ifa->ifa_flags = lifr->lifr_flags;
	}

	if (ioctl(fd, SIOCGLIFADDR, lifr) < 0) {

	    ifa->ifa_addr = NULL;
	    return -1;

	} else {

	    ifa->ifa_addr = calloc(1, slen);

	    if(ifa->ifa_addr == NULL) {
		return -1;
	    }

	    memcpy(ifa->ifa_addr, &lifr->lifr_addr, slen);

	
	}

	if (ioctl(fd, SIOCGLIFNETMASK, lifr) < 0) {

	    ifa->ifa_netmask = NULL;
	    return -1;

	} else {

	    ifa->ifa_netmask = calloc(1, slen);

	    if(ifa->ifa_netmask == NULL) {
		return -1;
	    }

	    memcpy(ifa->ifa_netmask, &lifr->lifr_addr, slen);
	
	}


	if (ifa->ifa_flags & IFF_POINTOPOINT) {
	
	    if (ioctl(fd, SIOCGLIFDSTADDR, lifr) < 0) {

		ifa->ifa_dstaddr = NULL;
		return -1;

	    } else {

		ifa->ifa_dstaddr = calloc(1, slen);

		if(ifa->ifa_dstaddr == NULL) {
		    return -1;
		}

		memcpy(ifa->ifa_dstaddr, &lifr->lifr_dstaddr, slen);
	    }
	
	}
	
	if (ifa->ifa_flags & IFF_BROADCAST) {
	
	    if (ioctl(fd, SIOCGLIFBRDADDR, lifr) < 0) {

		ifa->ifa_broadaddr = NULL;
		return -1;

	    } else {

		ifa->ifa_broadaddr = calloc(1, slen);

		if(ifa->ifa_broadaddr == NULL) {
		    return -1;
		}

		memcpy(ifa->ifa_broadaddr, &lifr->lifr_broadaddr, slen);
	    }
	
	}

	return 0;

}

int
getifaddrs(struct ifaddrs **out)
{

	int ret = -1;

	int helper4, helper6;
	int ifcount = 0;

	struct lifreq *ifr = NULL;
	struct ifaddrs *ifa = NULL;
	struct ifaddrs *prev = NULL;

	if ((helper4 = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return (-1);

	if ((helper6 = socket(AF_INET6, SOCK_DGRAM, 0)) == -1 &&
	    errno != EAFNOSUPPORT) {
		close(helper4);
		return (-1);
	}

	/* populate lifreq data: INET socket is fine, we use AF_UNSPEC */
	if ((ifcount = populate_lifreq(&ifr, helper4)) < 0) {
		goto finalise;
	}

	/* nahim' */
	if (ifcount == 0) {
		*out = NULL;
		return (0);
	}

	/* allocate memory for n ifaddrs */
	*out = calloc(ifcount, sizeof(struct ifaddrs));
	
	if(*out == NULL) {
	    goto finalise;
	}

	ifa = *out;

	/* populate inet4 / inet6 entries and link them */
	for (int i = 0; i < ifcount; i++) {

	    int fd = -1;

	    /* pass inet4 or inet6 helper depending on af */
	    switch(ifr[i].lifr_addr.ss_family) {
		case AF_INET:
		    fd = helper4;
		    break;
		case AF_INET6:
		    fd = helper6;
		    break;
	    }

	    if(fd == -1) {
		continue;
	    }

	    if(prev != NULL) {
		prev->ifa_next = ifa;
	    }

	    if(populate_ifaddr(ifa, fd, &ifr[i]) < 0) {
		goto finalise;
	    }

	    prev = ifa;
	    ifa++;

	}

	ret = 0;

finalise:

	close(helper4);

	if (helper6 != -1) {
	    close(helper6);
	}

	free(ifr);

	if(ret == -1 && *out != NULL) {
	    free(*out);
	}

	return ret;
}

/* free allocated data in a single ifaddrs entry */
static void freeifaddr(struct ifaddrs *ifa) {

    FREENOTNULL(ifa->ifa_name);
    FREENOTNULL(ifa->ifa_addr);
    FREENOTNULL(ifa->ifa_netmask);
    FREENOTNULL(ifa->ifa_dstaddr);

}

void
freeifaddrs(struct ifaddrs *ifa)
{

    struct ifaddrs *marker;

    for (marker = ifa; marker != NULL; marker = marker->ifa_next) {
	freeifaddr(marker);
    }

    if(ifa != NULL) {
	free(ifa);
    }

}
