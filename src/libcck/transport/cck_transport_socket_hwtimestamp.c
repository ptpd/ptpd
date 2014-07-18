/*-
 * libCCK - Clock Construction Kit
 *
 * Copyright (c) 2014 Wojciech Owczarek,
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
 * @file   cck_transport_hwtimestamp.c
 *
 * @brief  LibCCK software timestamping helper funcion implementations
 *
 */

#include "cck_transport_socket_hwtimestamp.h"

#if defined(SO_TIMESTAMPING) && defined(ETHTOOL_GET_TS_INFO)

/*
CckBool
cckGetTsInfo(const char* ifName, struct ethtool_ts_info* tsInfo)
{
    struct ifreq ifRequest;
    CckBool ret = CCK_TRUE;

    int sockFd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockFd < 0) {
	CCK_PERROR("Could not create test socket");
	return CCK_FALSE;
    }

    memset(tsInfo, 0, sizeof(struct ethtool_ts_info));
    memset(&ifRequest, 0, sizeof(ifRequest));
    tsInfo->cmd = ETHTOOL_GET_TS_INFO;
    strncpy( ifRequest.ifr_name, ifName, IFNAMSIZ );
    ifRequest.ifr_data = (char *) tsInfo;
    if (ioctl(sockFd, SIOCETHTOOL, &ifRequest) < 0) {
	CCK_PERROR("Could not rertrieve ethtool information for %s",
		ifName);
	memset(tsInfo, 0, sizeof(struct ethtool_ts_info));
	ret = CCK_FALSE;
    }

    close(sockFd);

    return ret;
}
*/
CckBool
cckSetTsConfig(int sockFd, const char* ifName, struct hwtstamp_config* tsConfig)
{
    struct ifreq ifRequest;
    CckBool ret = CCK_TRUE;

/*
    int sockFd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockFd < 0) {
	CCK_PERROR("Could not create test socket");
	return CCK_FALSE;
    }
*/
    memset(&ifRequest, 0, sizeof(ifRequest));
    strncpy( ifRequest.ifr_name, ifName, IFNAMSIZ );
    ifRequest.ifr_data = (void *) tsConfig;
    if (ioctl(sockFd, SIOCSHWTSTAMP, &ifRequest) < 0) {
	CCK_PERROR("Could not set hardware timestamping options on %s",
		ifName);
	ret = CCK_FALSE;
    }

//    close(sockFd);

    return ret;
}
#endif

CckBool
cckInitHwTimestamping(CckTransport* transport, CckSocketTimestampCaps* caps, CckBool testingOnly)
{
    int val = 1;
    const char* methodStr;

    memset(caps, 0, sizeof(CckSocketTimestampCaps));

#if defined(SO_TIMESTAMPING) && defined(SO_TIMESTAMPNS) && defined(ETHTOOL_GET_TS_INFO)

    struct ethtool_ts_info tsInfo;
    struct hwtstamp_config tsConfig;

    tsConfig.flags = 0;
    tsConfig.tx_type = 0;
    tsConfig.rx_filter = 0;

    if(!cckGetTsInfo(transport->transportEndpoint, &tsInfo)) {
	CCK_ERROR("Could not retrieve timestamping capabilities for %s\n",
			transport->header.instanceName);
	return CCK_FALSE;
    } else {
	val = 0;
	if(tsInfo.so_timestamping & SOF_TIMESTAMPING_RX_HARDWARE) val |= SOF_TIMESTAMPING_RX_HARDWARE;
	if(tsInfo.so_timestamping & SOF_TIMESTAMPING_RAW_HARDWARE) val |= SOF_TIMESTAMPING_RAW_HARDWARE;
	if (val != 0) {
	    caps->socketOption = SO_TIMESTAMPING;
	    caps->scmType = SCM_TIMESTAMPING;
	    caps->timestampType = CCK_SCMTIMESTAMPING;
	    caps->timestampSize = 3 * sizeof(struct timespec);
	    methodStr = "SO_TIMESTAMPING";
	    if(tsInfo.so_timestamping & SOF_TIMESTAMPING_TX_HARDWARE) {
		val |= SOF_TIMESTAMPING_TX_HARDWARE;
		caps->txTimestamping = CCK_TRUE;
	    } else {
		CCK_ERROR("SO_TIMESTAMPING hardware TX timestamping not supported on %s\n",
			    transport->transportEndpoint);
		return CCK_FALSE;
	    }
	} else {
	    CCK_ERROR("required SO_TIMESTAMPING hardware timestamping options not supported on %s\n",
			    transport->transportEndpoint);
	    return CCK_FALSE;;
	}
    }

    /* SO_TIMESTAMPING flags OK - trying the RX filters now */
    if(tsInfo.rx_filters & 1 << HWTSTAMP_FILTER_ALL) {
	tsConfig.rx_filter = HWTSTAMP_FILTER_ALL;
	CCK_DBGV("HWTSTAMP_FILTER_ALL supported on %s\n",
		    transport->transportEndpoint);
    } else {
	CCK_DBGV("Trying to enable PTP-specific timestamping filters on %s, may not be useful with other protocols\n",
		    transport->transportEndpoint);
	if(tsInfo.rx_filters & 1 << HWTSTAMP_FILTER_PTP_V2_L4_EVENT) {
		tsConfig.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_EVENT;
	}
    }

    if(tsConfig.rx_filter == 0) {
	CCK_ERROR("No suitable hardware RX filters available on %s\n",
			transport->transportEndpoint);
	return CCK_FALSE;
    }

    /* RX filters OK - trying TX timestamp options */

    if(tsInfo.tx_types & HWTSTAMP_TX_ON) {
	tsConfig.tx_type |= HWTSTAMP_TX_ON;
    } else {
	CCK_ERROR("HW TX timestamping not available on %s\n",
		    transport->transportEndpoint);
	return CCK_FALSE;
    }

    /* we're only probing options - assume setting the SO_ succeeded */
    if(testingOnly) {
	return CCK_TRUE;
    }

    if(!cckSetTsConfig(transport->fd, transport->transportEndpoint, &tsConfig)) {
	CCK_ERROR("Failed to set hardware timestamping configuration on %s\n",
		    transport->transportEndpoint);
	return CCK_FALSE;
    }

    if(setsockopt(transport->fd, SOL_SOCKET, caps->socketOption, &val, sizeof(int)) != 0) {

	CCK_ERROR("Failed to enable %s hardware timestamping on transport %s", methodStr,
			    transport->header.instanceName);
	return CCK_FALSE;

    }


	CCK_DBG("Successfully enabled %s hardware timestamping on transport %s\n", methodStr,
		    transport->header.instanceName);
	caps->hwTimestamping = CCK_TRUE;
	return CCK_TRUE;

#else

CCK_ERROR("Hardware timestamping not supported on this platform\n");


#endif

 return CCK_FALSE;
}

CckBool
cckGetHwtimestamp(struct cmsghdr* cmsg, CckSocketTimestampCaps* caps, CckTimestamp* timestamp)
{

    timestamp->seconds = 0;
    timestamp->nanoseconds = 0;

    if (cmsg->cmsg_type != caps->scmType)
	return CCK_FALSE;

    switch(caps->timestampType) {
	case CCK_TIMESPEC:
	    {
		struct timespec* ts = (struct timespec*)CMSG_DATA(cmsg);
		timestamp->seconds = ts->tv_sec;
		timestamp->nanoseconds = ts->tv_nsec;
	    }
	    goto success;
	case CCK_TIMEVAL:
	    {
		struct timeval* tv = (struct timeval*)CMSG_DATA(cmsg);
		timestamp->seconds = tv->tv_sec;
		timestamp->nanoseconds = tv->tv_usec * 1000;
	    }
	    goto success;
#ifdef SO_BINTIME
	case CCK_BINTIME:
	    {
		struct bintime* bt = (struct bintime*)CMSG_DATA(cmsg);
		struct timespec ts;
		bintime2timespec(bt, &ts);
		timestamp->seconds = ts.tv_sec;
		timestamp->nanoseconds = ts.tv_nsec;
	    }
	    goto success;
#endif
	default:
    	    return CCK_FALSE;

    }

    success:
	CCK_DBGV("Got packet timestamp %d.%d\n",
		    timestamp->seconds,
		    timestamp->nanoseconds);
	return CCK_TRUE;
}
