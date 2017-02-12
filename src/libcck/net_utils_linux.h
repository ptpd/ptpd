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
 * @file   net_utils_linux.h
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  Definitions for Linux-specific network utility functions
 *
 */

#ifndef CCK_NET_UTILS_LINUX_H_
#define CCK_NET_UTILS_LINUX_H_

#include <libcck/cck_types.h>
#include <linux/ethtool.h>

/*
 * Some Linux distributions come with kernels which support ETHTOOL_GET_TS_INFO,
 * but headers do not reflect this. If this fails in runtime,
 * the system is not fully usable for PTP anyway.
 */
#if defined(HAVE_DECL_ETHTOOL_GET_TS_INFO) && !HAVE_DECL_ETHTOOL_GET_TS_INFO

#define ETHTOOL_GET_TS_INFO	0x00000041 /* Get time stamping and PHC info */
struct ethtool_ts_info {
    __u32	cmd;
    __u32	so_timestamping;
    __s32	phc_index;
    __u32	tx_types;
    __u32	tx_reserved[3];
    __u32	rx_filters;
    __u32	rx_reserved[3];
};

#endif /* HAVE_DECL_ETHTOOL_GET_TS_INFO */

#define BOND_SLAVES_MAX 20

typedef struct {
    char name[IFNAMSIZ + 1];
    struct ethtool_ts_info tsInfo;
    int id;
} LinuxBondSlave;

typedef struct {
	LinuxBondSlave activeSlave;
	LinuxBondSlave slaves[BOND_SLAVES_MAX];
	bool updated;
	bool bonded;
	bool activeBackup;
	int slaveCount;
	int activeCount;
	bool activeChanged;
	bool countChanged;
} LinuxBondInfo;

typedef struct {
	char realDevice[IFNAMSIZ + 1];
	struct ethtool_ts_info tsInfo;
	bool vlan;
	int vlanId;
} LinuxVlanInfo;

typedef struct {
	char ifName[IFNAMSIZ + 1];
	char physicalDevice[IFNAMSIZ + 1];
	struct ethtool_ts_info tsInfo;	/* physical device */
	struct ethtool_ts_info logicalTsInfo; /* logical device */
	bool hwTimestamping;
	bool valid;
	LinuxBondInfo bondInfo;
	LinuxVlanInfo vlanInfo;
} LinuxInterfaceInfo;

bool getEthtoolTsInfo(struct ethtool_ts_info *info, const char *ifName);
void getLinuxBondInfo(LinuxBondInfo *info, const char *ifName);
void getLinuxVlanInfo(LinuxVlanInfo *info, const char* ifName);
void getLinuxInterfaceInfo(LinuxInterfaceInfo *info, const char *ifName);

/* check for interface changes */
int monitorLinuxInterface(LinuxInterfaceInfo *info, const bool quiet);

#endif /* CCK_NET_UTILS_LINUX_H_ */
