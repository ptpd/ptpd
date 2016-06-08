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
 * @file   clockdriver_linuxphc_vext_solarflare.h
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  Linux PHC clock driver Solarflare extension definitions
 *
 */

#ifndef PTPD_CLOCKDRIVER_LINUXPHC_VEXT_SOLARFLARE_H_
#define PTPD_CLOCKDRIVER_LINUXPHC_VEXT_SOLARFLARE_H_

#include "clockdriver.h"

/* ========= SF code BEGIN - original licencing, to be clarified with SF ========= */

/*
** Copyright 2005-2016  Solarflare Communications Inc.
**                      7505 Irvine Center Drive, Irvine, CA 92618, USA
** Copyright 2002-2005  Level 5 Networks Inc.
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of version 2 of the GNU General Public License as
** published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/****************************************************************************
 * Driver for Solarflare network controllers
 *           (including support for SFE4001 10GBT NIC)
 *
 * Copyright 2005-2006: Fen Systems Ltd.
 * Copyright 2006-2012: Solarflare Communications Inc,
 *                      9501 Jeronimo Road, Suite 250,
 *                      Irvine, CA 92618, USA
 *
 * Initially developed by Michael Brown <mbrown@fensystems.co.uk>
 * Maintained by Solarflare Communications <linux-net-drivers@solarflare.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, incorporated herein by reference.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 ****************************************************************************
 */

#define SIOCSF (SIOCDEVPRIVATE + 3)

/* PTP support for NIC time disciplining ************************************/
struct sf_timespec {
	int64_t		tv_sec;
	int32_t		tv_nsec;
};

/* Get the NIC-system time skew *********************************************/
#define SF_TS_SYNC 0xef16
struct sf_ts_sync {
	struct sf_timespec ts;
};

/* Set the NIC-system synchronization status ********************************/
#define SF_TS_SET_SYNC_STATUS 0xef27
struct sf_ts_set_sync_status {
	uint32_t in_sync;		/* 0 == not in sync, 1 == in sync */
	uint32_t timeout;		/* Seconds until no longer in sync */
};

/* Return a PPS timestamp ***************************************************/
#define SF_TS_GET_PPS 0xef1c
struct sf_ts_get_pps {
	uint32_t sequence;				/* seq. num. of assert event */
	uint32_t timeout;
	struct sf_timespec sys_assert;		/* time of assert in system time */
	struct sf_timespec nic_assert;		/* time of assert in nic time */
	struct sf_timespec delta;		/* delta between NIC and system time */
};

#define SF_TS_ENABLE_HW_PPS 0xef1d
struct sf_ts_hw_pps {
	uint32_t enable;
};

/* Efx private ioctl command structures *************************************/

union sf_ioctl_data {
	struct sf_ts_sync ts_sync;
	struct sf_ts_set_sync_status ts_set_sync_status;
	struct sf_ts_get_pps pps_event;
	struct sf_ts_hw_pps pps_enable;
};

struct sf_sock_ioctl {
	/* Command to run */
	uint16_t cmd;
	uint16_t reserved;
	/* Parameters */
	union sf_ioctl_data u;
};

/* ========= SF code END - original licencing, to be clarified with SF ========= */

int loadCdVendorExt_solarflare(ClockDriver *driver, const char *ifname);


#endif /* PTPD_CLOCKDRIVER_LINUXPHC_VEXT_SOLARFLARE_H_ */
