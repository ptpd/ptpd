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
 * @file   net_utils_linux.c
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  Linux-specific network utility functions
 *
 */

#include <config.h>

#include <string.h>
#include <errno.h>

#include <libcck/net_utils.h>
#include <libcck/net_utils_linux.h>

/* after net_utils.h to resolve net/if.h vs. linux/if.h conflicts */
#include <linux/if_bonding.h>
#include <linux/if_vlan.h>

#include <libcck/cck_logger.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_types.h>

static bool bondQuery(ifbond *ifb, const char *ifName);
static bool bondSlaveQuery(ifslave *ifs, const char *ifName, int member);
static int getBondSlaves(LinuxBondInfo *info, const char *ifName);

static bool bondQuery(ifbond *ifb, const char *ifName)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    memset(ifb, 0, sizeof(ifbond));
    ifr.ifr_data = (caddr_t)ifb;
    return ioctlHelper(&ifr, ifName, SIOCBONDINFOQUERY);

}

static bool bondSlaveQuery(ifslave *ifs, const char *ifName, int member)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    memset(ifs, 0, sizeof(ifslave));
    ifs->slave_id = member;
    ifr.ifr_data = (caddr_t)ifs;
    return ioctlHelper(&ifr, ifName, SIOCBONDSLAVEINFOQUERY);

}

static int getBondSlaves(LinuxBondInfo *info, const char *ifName)
{

    ifbond ifb;
    ifslave ifs;

    memset(&ifb, 0, sizeof(ifb));
    memset(&ifs, 0, sizeof(ifs));


    memset(&info->activeSlave, 0, sizeof(LinuxBondSlave));

    info->activeSlave.id = -1;

    for(int i = 0; i < BOND_SLAVES_MAX; i++) {
	memset(&info->slaves[i], 0, sizeof(LinuxBondSlave));
	info->slaves[i].id = -1;
    }

    if(bondQuery(&ifb, ifName)) {

	if(ifb.num_slaves == 0) return -1;

	for(int i = 0; (i < ifb.num_slaves) && (i < BOND_SLAVES_MAX); i++) {
	    if(bondSlaveQuery(&ifs, ifName, i)) {
		strncpy(info->slaves[i].name, ifs.slave_name, IFNAMSIZ);
		info->slaves[i].id = i;
		getEthtoolTsInfo(&info->slaves[i].tsInfo, ifs.slave_name);
		if(ifs.state == BOND_STATE_ACTIVE) {
		    memcpy(&info->activeSlave, &info->slaves[i], sizeof(LinuxBondSlave));
		}
	    }
	}

    }

    return ( ifb.num_slaves );

}

#ifdef ETHTOOL_GET_TS_INFO
bool getEthtoolTsInfo(struct ethtool_ts_info *info, const char *ifName) {

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	memset(info, 0, sizeof(struct ethtool_ts_info));
	info->cmd = ETHTOOL_GET_TS_INFO;
	ifr.ifr_data = (char*) info;
	if(!ioctlHelper(&ifr, ifName, SIOCETHTOOL)) {
	    CCK_PERROR("Could not get ethtool timestamping information from %s", ifName);
	    return false;
	}

	return true;

}
#endif /* ETHTOOL_GET_TS_INFO */

void getLinuxBondInfo(LinuxBondInfo *info, const char *ifName)
{

    ifbond ifb;
    LinuxBondInfo lastInfo;

    if(!bondQuery(&ifb, ifName)) {
	info->bonded = false;
	return;
    }

    memset(&lastInfo, 0, sizeof(LinuxBondInfo));

    if(info->updated) {
	memcpy(&lastInfo, info, sizeof(LinuxBondInfo));
    }

    info->bonded = true;
    info->activeBackup = (ifb.bond_mode == BOND_MODE_ACTIVEBACKUP);
    info->slaveCount = getBondSlaves(info, ifName);

    if(info->activeSlave.id >= 0) {
	info->activeCount = 1;
    } else {
	info->activeCount = 0;
    }

    if(info->updated) {

	if (strncmp(lastInfo.activeSlave.name, info->activeSlave.name, IFNAMSIZ)) {
	    info->activeChanged = true;
	} else if (lastInfo.slaveCount != info->slaveCount) {
	    info->countChanged = true;
	} else {
	    info->activeChanged = false;
	    info->countChanged = false;
	}

    }

    info->updated = true;
    return;

}

void getLinuxVlanInfo(LinuxVlanInfo *info, const char* ifName)
{

    struct vlan_ioctl_args args;

    int sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
	CCK_PERROR("ioctlHelper(): could not create helper socket");
	info->vlan = false;
	return;
    }

    memset(&args, 0, sizeof(struct vlan_ioctl_args));
    strncpy(args.device1, ifName, min(sizeof(args.device1), IFNAMSIZ));
    args.cmd = GET_VLAN_REALDEV_NAME_CMD;
    if (ioctl(sockfd, SIOCGIFVLAN, &args) < 0) {
            CCK_DBG("getLinuxVlanInfo(): failed to call SIOCGIFVLAN ioctl on %s: %s\n", ifName, strerror(errno));
	    close(sockfd);
	    info->vlan = false;
	    return;
    }

    info->vlan = true;

    strncpy(info->realDevice, args.u.device2, min(sizeof(args.device1), IFNAMSIZ));
    memset(&args, 0, sizeof(struct vlan_ioctl_args));
    strncpy(args.device1, ifName, min(sizeof(args.device1), IFNAMSIZ));
    args.cmd = GET_VLAN_VID_CMD;
    if (ioctl(sockfd, SIOCGIFVLAN, &args) < 0) {
            CCK_DBG("getLinuxVlanInfo(): failed to call SIOCGIFVLAN ioctl on %s: %s\n", ifName, strerror(errno));
	    close(sockfd);
	    info->vlan = false;
	    return;
    }

    info->vlanId = args.u.VID;

    getEthtoolTsInfo(&info->tsInfo, info->realDevice);

    close(sockfd);
    return;


}

void getLinuxInterfaceInfo(LinuxInterfaceInfo *info, const char *ifName)
{

    strncpy(info->physicalDevice, ifName, IFNAMSIZ);

    LinuxBondInfo *bondInfo = &info->bondInfo;
    LinuxVlanInfo *vlanInfo = &info->vlanInfo;

    if(!interfaceExists(ifName)) {
	return;
    }

    info->valid = true;

    strncpy(info->ifName, ifName, IFNAMSIZ);

    getLinuxVlanInfo(vlanInfo, ifName);

    if(vlanInfo->vlan) {
	    strncpy(info->physicalDevice, vlanInfo->realDevice, IFNAMSIZ);
    }

    getLinuxBondInfo(bondInfo, info->physicalDevice);

    if(bondInfo->bonded && bondInfo->activeCount > 0) {
	    strncpy(info->physicalDevice, bondInfo->activeSlave.name, IFNAMSIZ);
    }

    getEthtoolTsInfo(&info->tsInfo, info->physicalDevice);
    getEthtoolTsInfo(&info->logicalTsInfo, ifName);

    CCK_DBG("getLinuxInterfaceInfo(%s): Underlying physical device: %s\n", ifName, info->physicalDevice);

}


/*
 *  status in the @last structure is only one of OK, DOWN, FAULT, but returned value
 *  provides event status (went up, went down, fault, fault cleared, major change, or no change)
 */


int monitorLinuxInterface(LinuxInterfaceInfo *info, const bool quiet)
{

    int ret = 0;

    if(!info) {
	return -1;
    }

    /* no point monitoring without previous data */
    if(!info->valid) {
	return 0;
    }

    /* we don't care about bonding changes when not running h/w timestamping */
    if(!info->hwTimestamping) {

	return CCK_INTINFO_OK;

    }

    getLinuxInterfaceInfo(info, info->ifName);

    LinuxBondInfo *bi = &info->bondInfo;

    if(bi->bonded) {

	if(!bi->activeBackup) {
	    CCK_QERROR("transport: monitorLinuxInterface('%s'): Bonded interface"
			 " not running Active Backup, expect random timing performance\n", info->ifName);
	    return CCK_INTINFO_FAULT;
	}

	if(bi->countChanged) {
	    CCK_QNOTICE("transport: monitorLinuxInterface('%s'): Bonded interface"
			 " member count has changed\n", info->ifName);
	    ret = CCK_INTINFO_CHANGE;
	}

	if(bi->activeChanged) {
	    CCK_QNOTICE("transport: monitorLinuxInterface('%s'): Bonded interface"
			 " active member changed to %s\n", info->ifName, info->physicalDevice);
	    ret |= CCK_INTINFO_CLOCKCHANGE;

	}

    }

    return ret;

}
