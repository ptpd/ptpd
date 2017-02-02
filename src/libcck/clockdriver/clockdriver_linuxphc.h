/* Copyright (c) 2016-2017 Wojciech Owczarek,
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
 * @file   clockdriver_linuxphc.h
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  structure definitions for the Linux PHC clock driver
 *
 */

#ifndef CCK_CLOCKDRIVER_LINUXPHC_H_
#define CCK_CLOCKDRIVER_LINUXPHC_H_

#include <config.h>

#include <sys/types.h>
#include <sys/socket.h>

#ifdef HAVE_LINUX_IF_H
#include <linux/if.h>		/* struct ifaddr, ifreq, ifconf, ifmap, IF_NAMESIZE etc. */
#elif defined(HAVE_NET_IF_H)
#include <net/if.h>		/* struct ifaddr, ifreq, ifconf, ifmap, IF_NAMESIZE etc. */
#endif /* HAVE_LINUX_IF_H*/

#include <libcck/clockdriver.h>

typedef struct {
    int clockFd;
    int helperFd;
    int phcIndex;
    clockid_t clockId;
} ClockDriverData_linuxphc;

typedef struct {
    char networkDevice[IFNAMSIZ];
    char characterDevice[PATH_MAX];
    bool lockDevice;
} ClockDriverConfig_linuxphc;

bool _setupClockDriver_linuxphc(ClockDriver* clockDriver);


#endif /* CCK_CLOCKDRIVER_LINUXPHC_H_ */
