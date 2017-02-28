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
 * @file   extensions/clockdriver/linuxphc_vext_solarflare.c
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  Linux PHC clock driver Solarflare vendor extensions
 *
 */

#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include "linuxphc_vext_solarflare.h"
#include "efx_ioctl_timesync.h"
#include <libcck/cck.h>

typedef struct {
	struct ifreq ifr;
	struct efx_sock_ioctl sfioctl;
	int fd;
	char ifName[IFNAMSIZ + 1];
} ClockDriverExtData_solarflare;

static bool getSystemClockOffset(ClockDriver *driver, CckTimestamp *output);
static int vendorInit(ClockDriver *driver);
static int vendorShutdown(ClockDriver *driver);

int
loadCdVendorExt_solarflare(ClockDriver *driver, const char *ifname) {

    int ret = 0;
    CckTimestamp delta = {0,0};

    CCK_INIT_EXTDATA(ClockDriver, solarflare, driver);
    CCK_GET_EXTDATA(ClockDriver, solarflare, driver, sfData);

    strncpy(sfData->ifName, ifname, IFNAMSIZ);

//todo: do not call vendorInit here, just allocate, just overload it and driver will call it

    ret = vendorInit(driver);

    if(ret < 0) {
	vendorShutdown(driver);
	return ret;
    }


    /* test if the vendor extension is operational */
    if(getSystemClockOffset(driver, &delta)) {
	driver->getSystemClockOffset = getSystemClockOffset;
	ret = 1;
    }

    if(ret < 1)  {
	vendorShutdown(driver);
	return -1;

    }

    driver->_vendorShutdown = vendorShutdown;

    return 1;

}

static bool
getSystemClockOffset(ClockDriver *driver, CckTimestamp *output)
{

    int ret;

    CCK_GET_EXTDATA(ClockDriver, solarflare, driver, sfData);

    memset(&sfData->sfioctl, 0, sizeof(struct efx_sock_ioctl));

    sfData->sfioctl.cmd = EFX_TS_SYNC;

    ret = ioctl(sfData->fd, SIOCEFX, &sfData->ifr);

    if(ret < 0) {
	return false;
    }

    output->seconds = sfData->sfioctl.u.ts_sync.ts.tv_sec;
    output->nanoseconds = sfData->sfioctl.u.ts_sync.ts.tv_nsec;

    if(output->seconds < 0) {
	output->seconds += 1;
	output->nanoseconds = output->nanoseconds - 1E9;
    }

    output->seconds = -output->seconds;
    output->nanoseconds = -output->nanoseconds;

    return true;
}

static int
vendorInit(ClockDriver *driver) {

    CCK_GET_EXTDATA(ClockDriver, solarflare, driver, sfData);

    sfData->fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sfData->fd < 0) {
	return -1;
    }

    strncpy(sfData->ifr.ifr_name, sfData->ifName, IFNAMSIZ);
    sfData->ifr.ifr_data = (caddr_t) & sfData->sfioctl;

    return 1;

}

static int
vendorShutdown(ClockDriver *driver) {

    CCK_GET_EXTDATA(ClockDriver, solarflare, driver, sfData);

    close(sfData->fd);
    free(driver->_extData);

    return 1;

}
