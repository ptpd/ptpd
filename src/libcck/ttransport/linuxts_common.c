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
 * @file   linuxts_common.c
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  Linux SO_TIMESTAMPING common transport code
 *
 */

#include <config.h>

#include <errno.h>

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/ttransport.h>
#include <libcck/net_utils.h>
#include <libcck/clockdriver.h>

#define THIS_COMPONENT "ttransport.linuxts: "

#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

#ifndef HWTSTAMP_TX_ONESTEP_SYNC
#define HWTSTAMP_TX_ONESTEP_SYNC 2
#endif /* HWTSTAMP_TX_ONESTEP_SYNC */

/* we attempt to set all three */
static const OptionName hwtsModes[] = {
    {SOF_TIMESTAMPING_TX_HARDWARE, "SOF_TIMESTAMPING_TX_HARDWARE"},
    {SOF_TIMESTAMPING_RX_HARDWARE, "SOF_TIMESTAMPING_RX_HARDWARE"},
    {SOF_TIMESTAMPING_RAW_HARDWARE, "SOF_TIMESTAMPING_RAW_HARDWARE"},
    {-1}
};

/* we attempt to set all three */
static const OptionName swTsModes[] = {
    {SOF_TIMESTAMPING_TX_SOFTWARE, "SOF_TIMESTAMPING_TX_SOFTWARE"},
    {SOF_TIMESTAMPING_RX_SOFTWARE, "SOF_TIMESTAMPING_RX_SOFTWARE"},
    {SOF_TIMESTAMPING_SOFTWARE, "SOF_TIMESTAMPING_SOFTWARE"},
    {-1}
};

/* "sensible" order - last mode will not support timestamping unicast.
 * Preferring ALL to allow VLAN timestamping etc.
 * We attempt to set the first one that matches.
 */
static const OptionName rxFilters_udp[] = {
    { HWTSTAMP_FILTER_PTP_V2_EVENT, "HWTSTAMP_FILTER_PTP_V2_EVENT"},
    { HWTSTAMP_FILTER_PTP_V2_L4_EVENT, "HWTSTAMP_FILTER_PTP_V2_L4_EVENT"},
    { HWTSTAMP_FILTER_ALL, "HWTSTAMP_FILTER_ALL"},
    { HWTSTAMP_FILTER_PTP_V2_L4_SYNC, "HWTSTAMP_FILTER_PTP_V2_L4_SYNC"},
    {-1}
};

static const OptionName rxFilters_ethernet[] = {
    { HWTSTAMP_FILTER_PTP_V2_EVENT, "HWTSTAMP_FILTER_PTP_V2_EVENT"},
    { HWTSTAMP_FILTER_PTP_V2_L2_EVENT, "HWTSTAMP_FILTER_PTP_V2_L2_EVENT"},
    { HWTSTAMP_FILTER_ALL, "HWTSTAMP_FILTER_ALL"},
    { HWTSTAMP_FILTER_PTP_V2_L2_SYNC, "HWTSTAMP_FILTER_PTP_V2_L2_SYNC"},
    {-1}
};

static const OptionName txTypes[] = {
    { HWTSTAMP_TX_ON, "HWTSTAMP_TX_ON"},
    { HWTSTAMP_TX_ONESTEP_SYNC, "HWTSTAMP_TX_ONESTEP_SYNC"},
    {-1}
};

static bool checkRxFilter(uint32_t filter, int family);
static bool testLinuxTimestamping(LinuxTsInfo *info, const char *ifName, int family, const bool preferHw, const bool quiet);
static bool initHwTimestamping(LinuxTsInfo *info, const char *ifName, const int family, const bool quiet);

void
getLinuxTsInfo(LinuxTsInfo *output, const struct ethtool_ts_info *source, const char *ifName, const int family, const bool preferHw) {

	const OptionName *mode = NULL;
	memset(output, 0, sizeof(LinuxTsInfo));

	/* check if we support all required s/w timestamping modes */
	output->swTimestamping = true;
	for(const OptionName *mode = swTsModes; mode->value != -1; mode++) {
	    if((source->so_timestamping & mode->value) == mode->value) {
		CCK_DBG("getLinuxTsInfo(%s): Interface supports s/w mode %s\n", ifName, mode->name);
		output->swTsModes |= mode->value;
	    } else {
		CCK_DBG("getLinuxTsInfo(%s): No support for mode %s, s/w timestamping not usable\n", ifName, mode->name);
		output->swTimestamping = false;
	    }
	}

	if(!output->swTimestamping) {
	    output->swTsModes = 0;
	}

	/* check if we support all required h/w timestamping modes */
	output->hwTimestamping = true;
	for(const OptionName *mode = hwtsModes; mode->value != -1; mode++) {
	    if((source->so_timestamping & mode->value) == mode->value) {
		CCK_DBG("getLinuxTsInfo(%s): Interface supports h/w mode %s\n", ifName, mode->name);
		output->hwTsModes |= mode->value;
	    } else {
		CCK_DBG("getLinuxTsInfo(%s): No support for %s mode %s, h/w timestamping not usable\n", ifName, mode->name);
		output->hwTimestamping = false;
	    }
	}

	if(!output->hwTimestamping) {
	    output->hwTsModes = 0;
	}


	/* no need to check further */
	if(!output->hwTimestamping ) {
	    return;
	}

	switch(family) {
	    case TT_FAMILY_IPV4:
	    case TT_FAMILY_IPV6:
		mode = rxFilters_udp;
		break;
	    case TT_FAMILY_ETHERNET:
		mode = rxFilters_ethernet;
		break;
	    default:
		CCK_DBG("getLinuxTsInfo(%s): No h/w timestamping support for address family %s\n",
		    ifName, getAddressFamilyName(family));
		output->hwTimestamping = false;
		return;
	}

	output->hwTimestamping = false;
	for( ; mode->value != -1; mode++) {
	    bool found = false;
	    if(source->rx_filters & (1 << mode->value) ) {
		CCK_DBG("getLinuxTsInfo(%s): Interface supports RX filter %s\n", ifName, mode->name);
		if(!found) {
		    output->rxFilter = mode->value;
		    found = true;
		    output->hwTimestamping = true;
		}
	    } else {
		CCK_DBG("getLinuxTsInfo(%s): No support for RX filter %s\n", ifName, mode->name);
	    }
	}

	if(source->tx_types & (1 << HWTSTAMP_TX_ON)) {
		CCK_DBG("getLinuxTsInfo(%s): Interface supports HWTSTAMP_TX_ON\n", ifName);
		output->txType = HWTSTAMP_TX_ON;
		output->txTimestamping = true;
	} else {
		CCK_DBG("getLinuxTsInfo(%s): No support for HWTSTAMP_TX_ON\n", ifName);
		output->hwTimestamping = false;
	}

	if(source->tx_types & (1 << HWTSTAMP_TX_ONESTEP_SYNC)) {
		CCK_DBG("getLinuxTsInfo(%s): Interface supports HWTSTAMP_TX_ONESTEP_SYNC\n", ifName);
		output->oneStepTxType = HWTSTAMP_TX_ONESTEP_SYNC;
		output->oneStep = true;
	} else {
		CCK_DBG("getLinuxTsInfo(%s): No support for HWTSTAMP_TX_ONESTEP_SYNC\n", ifName);
	}

	if(output->swTimestamping) {
	    output->tsModes = output->swTsModes;
	}

	if(output->hwTimestamping && preferHw) {
	    output->tsModes = output->hwTsModes;
	} else {
	    output->hwTimestamping = false;
	}

}

static bool
checkRxFilter(uint32_t filter, int family) {

	const OptionName *mode = NULL;

	switch(family) {
	    case TT_FAMILY_IPV4:
	    case TT_FAMILY_IPV6:
		mode = rxFilters_udp;
		break;
	    case TT_FAMILY_ETHERNET:
		mode = rxFilters_ethernet;
		break;
	    default:
		return false;
	}

	for( ; mode->value != -1; mode++) {
	    if(filter ==  mode->value)  {
		return true;
	    }
	}

	return false;

}

static bool testLinuxTimestamping(LinuxTsInfo *info, const char *ifName, int family, const bool preferHw, const bool quiet) {

    /* no adequate h/w timestamping support, let's see why */
    if(!info->hwTimestamping && preferHw) {
	if(!quiet) {
	    if(!info->rxFilter) {
		CCK_INFO("testLinuxTimestamping(%s): No support for required h/w RX timestamp filters for %s timestamping\n", ifName,
		getAddressFamilyName(family));
	    }
	    if(!info->txTimestamping) {
		CCK_INFO("testLinuxTimestamping(%s): No support for required h/w TX timestamp mode\n", ifName);
	    }
	}
	if(info->swTimestamping) {
	    if(!quiet) {
		CCK_INFO("testLinuxTimestamping(%s): Interface supports software timestamping only\n", ifName);
	    }
	    return true;
	}
	if(!quiet) {
	    CCK_ERROR("testLinuxTimestamping(%s): No Linux SO_TIMESTAMPING support\n", ifName);
	}
	return false;
    } else if (!preferHw && !info->swTimestamping) {
	if(!quiet) {
	    CCK_ERROR("testLinuxTimestamping(%s): No full SO_TIMESTAMPING software timestamp support\n", ifName);
	}
	return false;
    }

    return true;

}

static bool
initHwTimestamping(LinuxTsInfo *info, const char *ifName, const int family, const bool quiet) {

	struct ifreq ifr;
	struct hwtstamp_config config;

	/* no hardware or software timestamping support: interface not usable */
	if(!info->hwTimestamping && !info->swTimestamping) {
	    return false;
	}

	/* no hardware timestamping support: no need to init h/w timestamping */
	if(info->swTimestamping && !info->hwTimestamping) {
	    return true;
	}

	memset(&ifr, 0, sizeof(ifr));
	memset(&config, 0, sizeof(config));

	config.tx_type = info->txType;
	config.rx_filter = info->rxFilter;

	ifr.ifr_data = (void*) &config;

	if(!ioctlHelper(&ifr, ifName, SIOCSHWTSTAMP)) {
	    CCK_PERROR("initHwTimestamping(%s): Could not init hardware timestamping", ifName);
	    return false;
	}

	if(config.tx_type != info->txType) {
	    CCK_ERROR("initHwTimestamping(%s): Interface refused to set the required hardware transmit timestamping option\n",
	    ifName);
	    return false;
	}

	/* if the driver has changed the RX filter, check if we can use what we got */
	if(config.rx_filter != info->rxFilter) {
	    /* Some drivers return HWTSTAMP_FILTER_SOME - apparently "what we requested plus more". */
	    if(checkRxFilter(config.rx_filter, family) || config.rx_filter == HWTSTAMP_FILTER_SOME ) {
		CCK_WARNING("initHwTimestamping(%s): Interface changed hardware receive filter type from %d to %d - still acceptable\n",
		ifName, info->rxFilter, config.rx_filter);
	    /* nope. */
	    } else {
		CCK_ERROR("initHwTimestamping(%s): Interface changed hardware received filter type from %d to %d - cannot continue\n",
		ifName, info->rxFilter, config.rx_filter);
		return false;
	    }
	}

	if(!quiet) {
	    CCK_INFO("initHwTimestamping(%s): Hardware timestamping enabled\n", ifName);
	}

	return true;

}

bool
initTimestamping_linuxts_common(TTransport *transport, TTsocketTimestampConfig *tsConfig, LinuxInterfaceInfo *intInfo, const char *ifName, const bool quiet) {


	const char *realDevice;
	LinuxBondInfo *bi = &intInfo->bondInfo;
	LinuxVlanInfo *vi = &intInfo->vlanInfo;

	LinuxTsInfo pti, ti;
	bool ret = true;
	bool preferHw = !(transport->config.flags & TT_CAPS_PREFER_SW);
	struct ethtool_ts_info *tsInfo;

	/* if we are not interested in hardware timestamping, look at the logical interface */
	if(preferHw) {
	    realDevice = intInfo->physicalDevice;
	    tsInfo = &intInfo->tsInfo;
	} else {
	    realDevice = ifName;
	    tsInfo = &intInfo->logicalTsInfo;
	}

	/* socket timestamp properties */
	tsConfig->arrayIndex = 2;
	tsConfig->cmsgLevel = SOL_SOCKET;
	tsConfig->cmsgType = SCM_TIMESTAMPING;
	tsConfig->elemSize = sizeof(struct timespec);
	tsConfig->convertTs = convertTimestamp_timespec;

	if(vi->vlan) {
	    CCK_INFO("initTimestamping(%s): VLAN interface, VLAN %d, underlying interface: %s\n",
			ifName, vi->vlanId, vi->realDevice);
	}

	if(bi->bonded) {
	    tmpstr(bondMembers, (IFNAMSIZ + 2) * bi->slaveCount);
	    char *marker = bondMembers;
	    for(int i = 0; i < bi->slaveCount; i++) {
		char *slaveName = bi->slaves[i].name;
		if(strlen(slaveName)) {
		    marker += snprintf(marker, IFNAMSIZ+2,"%s%s", slaveName,
		    (i+1) < bi->slaveCount ? ", " : "");
		}
	    }

	    CCK_INFO("initTimestamping(%s): Bonded interface%s%s, slaves: %s\n",
			vi->vlan ? vi->realDevice : ifName, bi->activeBackup ? ", active: ": "",
			bi->activeBackup ? realDevice : "",
			bondMembers);

	}

	getLinuxTsInfo(&pti, tsInfo, realDevice, transport->family, preferHw);
	if(!testLinuxTimestamping(&pti, realDevice, transport->family, preferHw, quiet)
	    || !initHwTimestamping(&pti, realDevice, transport->family, quiet)) {
	    ret = false;
	}

	/* nothing more to do */
	if(!preferHw) {
	    goto finalise;
	}

	if(bi->bonded && !bi->activeBackup) {
	    ret = false;
	    CCK_WARNING("initTimestamping(%s): Bonded interface but not running active backup. Cannot enable hardware timestamping.\n",
		    ifName);
	} else if (bi->bonded) {

	    for(int i = 0; i < bi->slaveCount; i++) {
		char *slaveName = bi->slaves[i].name;
		if(strlen(slaveName)) {

		    if(bi->slaves[i].id == bi->activeSlave.id) {
			continue;
		    }

		    getLinuxTsInfo(&ti, &bi->slaves[i].tsInfo, slaveName, transport->family, preferHw);
		    if(!testLinuxTimestamping(&ti, slaveName, transport->family, preferHw, quiet)) {
			ret = false;
			continue;
		    }

		    if(ti.tsModes != pti.tsModes) {
			CCK_WARNING("initTimestamping(%s): Interface does not support the same timestamping modes as primary %s (%03x vs. %03x)\n",
			slaveName, realDevice, ti.tsModes, pti.tsModes);
		    }

		    if(!initHwTimestamping(&ti, slaveName, transport->family, quiet)) {
			ret = false;
			continue;
		    }
		}
	    }
	}

finalise:

	if (setsockopt(transport->myFd.fd, SOL_SOCKET, SO_TIMESTAMPING, &pti.tsModes, sizeof(pti.tsModes)) < 0) {
		CCK_PERROR("initTimestamping(%s): failed to enable socket timestamps", transport->name);
		ret = false;
	}

	if(!quiet && ret) {
	    CCK_INFO("initTimestamping(%s): Linux %s timestamping enabled\n", ifName,
			pti.hwTimestamping ? "hardware" : "software");
	}

	intInfo->hwTimestamping = pti.hwTimestamping;

	return ret;

}

ClockDriver * getLinuxClockDriver(const TTransport *transport, const LinuxInterfaceInfo *intInfo) {

	ClockDriver *ret = NULL;
	const char *realDevice = intInfo->physicalDevice;
	const LinuxBondInfo *bi = &intInfo->bondInfo;
	LinuxTsInfo pti, ti;

	/* if we are not interested in hardware, we have nothing more to do */
	if(transport->config.flags & TT_CAPS_PREFER_SW) {
	    ret = getSystemClock();
	    goto finalise;
	}

	getLinuxTsInfo(&pti, &intInfo->tsInfo, realDevice, transport->family, !(transport->config.flags & TT_CAPS_PREFER_SW));
	/* get the clock driver for physical device */
	if(pti.hwTimestamping) {
	    ret = findClockDriver(realDevice);
	    if(ret == NULL) {
		ret = createClockDriver(CLOCKDRIVER_LINUXPHC, realDevice);
	    }
	} else {
	    ret = getSystemClock();
	}

	/* try to start clock drivers for any other hardware clocks in a bond */
	if(bi->bonded && bi->activeBackup) {
	    for(int i = 0; i < bi->slaveCount; i++) {
		ClockDriver *cd = NULL;
		const char *slaveName = bi->slaves[i].name;
		if(strlen(slaveName)) {

		    if(bi->slaves[i].id == bi->activeSlave.id) {
			continue;
		    }

		    getLinuxTsInfo(&ti, &bi->slaves[i].tsInfo, slaveName, transport->family,
		     !(transport->config.flags & TT_CAPS_PREFER_SW));

		    if(ti.hwTimestamping) {
			cd = findClockDriver(slaveName);
			if(cd == NULL) {
			    cd = createClockDriver(CLOCKDRIVER_LINUXPHC, slaveName);
			}
			if(cd != NULL) {
			    if(!cd->_init) {
				cd->init(cd, slaveName);
			    }
			    cd->config.required = true;
			    cd->inUse = true;
			}
		    }
		}
	    }
	}

finalise:

	if(ret != NULL) {
	    if(!ret->_init) {
		ret->init(ret, realDevice);
	    }
	    ret->config.required = true;
	    ret->inUse = true;
	}

	return ret;

}

void
getLinuxTxTimestamp(TTransport *transport, TTransportMessage *txMessage) {

	char buf[txMessage->capacity];
	TTransportMessage tsMessage;
	ssize_t ret;
	fd_set tmpSet;
	struct timeval timeOut = {0,1500};
	int i = 0;
	int backoff = 10;
	int fd = transport->myFd.fd;

	/* prepare the temporary message to receive TX packet's payload */
	tsMessage.data = buf;
	tsMessage.capacity = txMessage->capacity;
	clearTTransportMessage(&tsMessage);
	tsMessage._flags |= TT_MSGFLAGS_TXTIMESTAMP;

	FD_ZERO(&tmpSet);
	FD_SET(fd, &tmpSet);

	if(select(fd + 1, &tmpSet, NULL, NULL, &timeOut) > 0) {
		if (FD_ISSET(fd, &tmpSet)) {
			
			ret = transport->receiveMessage(transport, &tsMessage);
			if (ret > 0) {
				CCK_DBG(THIS_COMPONENT"getTxTimestamp(%s): Grabbed TX sent msg via errqueue: %d bytes, at %d.%d\n",
					transport->name, ret, tsMessage.timestamp.seconds, tsMessage.timestamp.nanoseconds);
				goto gameover;
			} else if (ret < 0) {
				CCK_ERROR(THIS_COMPONENT"getTxTimestamp(%s): Failed to poll error queue for SO_TIMESTAMPING transmit time: %s\n",
					    transport->name, strerror(errno));
				goto gameover;
			} else if (ret == 0) {
				CCK_DBG(THIS_COMPONENT"getTxTimestamp(%s): Received no data from TX error queue, retrying\n",
				transport->name);
			}
		}
	}

	/* we're desperate here, aren't we... */

	for(i = 0; i <= LATE_TXTIMESTAMP_RETRIES; i++) {
	    CCK_DBG(THIS_COMPONENT"getTxTimestamp(%s) backoff: %d\n", transport->name, backoff);
	    ret = transport->receiveMessage(transport, &tsMessage);
	    if(ret > 0) {
		CCK_DBG(THIS_COMPONENT"getTxTimestamp(%s): delayed TX timestamp caught after %d retries\n",
			transport->name, i+1);
		goto gameover;
	    }
	    usleep(backoff);
	    backoff *= 2;
	}

	CCK_DBG(THIS_COMPONENT"getTxTimestamp(%s): NIC failed to deliver TX timestamp in time\n", transport->name);

gameover:

	if(tsMessage.hasTimestamp && transport->callbacks.matchData) {
	    if(transport->callbacks.matchData(transport->owner, transport,
						txMessage->data, txMessage->length,
						tsMessage.data, tsMessage.length)) {
		txMessage->timestamp = tsMessage.timestamp;
		txMessage->hasTimestamp = true;
		return;
	    } else {
		    CCK_DBG(THIS_COMPONENT"getTxTimestamp(%s) matchData: TX timestamp does not match transmitted messsage\n",
					    transport->name);
	    }
	}
	txMessage->hasTimestamp = false;
	tsOps.clear(&txMessage->timestamp);
	
	transport->counters.txTimestampErrors++;

}

bool
probeLinuxTs(const char *path, const int family, const int caps) {

	const char *realDevice;
	struct ethtool_ts_info *tsInfo;
	LinuxTsInfo ti;
	LinuxInterfaceInfo ii;
	bool preferHw = !(caps & TT_CAPS_PREFER_SW);

	memset(&ii, 0, sizeof(ii)); /* humble yourself at the fence of the fallen ones */

	getLinuxInterfaceInfo(&ii, path);

	if(preferHw) {
	    realDevice = ii.physicalDevice;
	    tsInfo = &ii.tsInfo;
	} else {
	    realDevice = path;
	    tsInfo = &ii.logicalTsInfo;
	}

	getLinuxTsInfo(&ti, tsInfo, realDevice, family, preferHw);

	return(testLinuxTimestamping(&ti, realDevice, family, preferHw, false));

}

char*
getStatusLine_linuxts(const TTransport *transport, const CckInterfaceInfo *info, const LinuxInterfaceInfo *linfo, char *buf, const size_t len)
{

	snprintf(buf, len, "%s%s%s%s%s, status: %s",
		    info->ifName,
		    linfo->vlanInfo.vlan ? ", VLAN" : "",
		    linfo->bondInfo.bonded ? ", bond" : "",
		    (linfo->bondInfo.bonded || linfo->vlanInfo.vlan) ? ", physical ": "",
		    (linfo->bondInfo.bonded || linfo->vlanInfo.vlan) ? linfo->physicalDevice: "",
		    getIntStatusName(info->status)
		);

	return buf;

}

char*
getInfoLine_linuxts(const TTransport *transport, const CckInterfaceInfo *info, const LinuxInterfaceInfo *linfo, char *buf, const size_t len)
{
	snprintf(buf, len, "%s, %s, %s", getTTransportTypeName(transport->type),
		    linfo->hwTimestamping ? "H/W tstamp" : "S/W tstamp",
		    TT_UC_MC(transport->config.flags) ? "hybrid UC/MC" :
		    TT_MC(transport->config.flags) ? "multicast" : "unicast");

	return buf;
}
