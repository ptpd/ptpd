/*-
 * libCCK - Clock Construction Kit
 *
 * Copyright (c) 2014 Wojciech Owczarek,
 *                    Eric Satterness,
 *                    National Instruments,
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
 * @file   cck_transport_swtimestamp.c
 * 
 * @brief  LibCCK software timestamping helper funcion implementations
 *
 */

#include "cck_transport_socket_swtimestamp.h"

#if defined(SO_TIMESTAMPING) && defined(ETHTOOL_GET_TS_INFO)
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
#endif

CckBool
cckInitSwTimestamping(CckTransport* transport, CckSocketTimestampCaps* caps, CckBool testingOnly)
{

    int val = 1;
    /*
     * methodStr produces an "unused variable" warning on release builds because it
     * is only used in debug print statements. Add the unused attribute to tell gcc
     * not to print the warning.
     */
    const char* methodStr __attribute__ ((unused));

    memset(caps, 0, sizeof(CckSocketTimestampCaps));

/*
  Use software SO_TIMESTAMPING only if the NIC supports s/w TX timestamping,
  otherwise stick to SO_TIMESTAMPNS as it has the same effect.

  TODO: if s/w SO_TIMESTAMPING fails, we should fail over to SO_TIMESTAMPNS.
*/
#if defined(SO_TIMESTAMPING) && defined(SO_TIMESTAMPNS) && defined(ETHTOOL_GET_TS_INFO)

    struct ethtool_ts_info tsInfo;

    if(!cckGetTsInfo(transport->transportEndpoint, &tsInfo)) {
	CCK_DBG("Reverting to SO_TIMESTAMPNS\n");
    } else {
	val = 0;
	if(tsInfo.so_timestamping & SOF_TIMESTAMPING_RX_SOFTWARE) val |= SOF_TIMESTAMPING_RX_SOFTWARE;
	if(tsInfo.so_timestamping & SOF_TIMESTAMPING_SOFTWARE) val |= SOF_TIMESTAMPING_SOFTWARE;
	if (val != 0) {
	    caps->socketOption = SO_TIMESTAMPING;
	    caps->scmType = SCM_TIMESTAMPING;
	    caps->timestampType = CCK_TIMESPEC;
	    caps->timestampSize = 3 * sizeof(struct timespec);
	    methodStr = "SO_TIMESTAMPING";
	    if(tsInfo.so_timestamping & SOF_TIMESTAMPING_TX_SOFTWARE) { 
		val |= SOF_TIMESTAMPING_TX_SOFTWARE;
		caps->txTimestamping = CCK_TRUE;
	    } else {
		CCK_DBGV("SO_TIMESTAMPING software TX timestamping not supported\n");
		val = 1;
	    }
	} else {
	    CCK_DBGV("SO_TIMESTAMPING software RX timestamping not supported\n");
	    val = 1;
	}
    }
#endif
#ifdef SO_TIMESTAMPNS
    /* This will be picked up either if SO_TIMESTAMPING is not supported or it didn't work */
    if(val == 1) {
	caps->socketOption = SO_TIMESTAMPNS;
	caps->scmType = SCM_TIMESTAMPNS;
	caps->timestampType = CCK_TIMESPEC;
	caps->timestampSize = sizeof(struct timespec);
	methodStr = "SO_TIMESTAMPNS";
    }
#elif defined(SO_BINTIME)
    caps->socketOption = SO_BINTIME;
    caps->scmType = SCM_BINTIME;
    caps->timestampType = CCK_BINTIME;
    caps->timestampSize = sizeof(struct bintime);
    methodStr = "SO_BINTIME";
#elif defined(SO_TIMESTAMP)
    caps->socketOption = SO_TIMESTAMP;
    caps->scmType = SCM_TIMESTAMP;
    caps->timestampType = CCK_TIMEVAL;
    caps->timestampSize = sizeof(struct timeval);
    methodStr = "SO_TIMESTAMP";
#else
    CCK_ERROR("No usable socket timestamping methods available for transport %s\n",
		transport->header.instanceName);
    return CCK_FALSE;
#endif

    /* we're only probing options - assume setting the SO_ succeeded */
    if(testingOnly) {
	return CCK_TRUE;
    }

    if(setsockopt(transport->fd, SOL_SOCKET, caps->socketOption, &val, sizeof(int)) == 0) {

	CCK_DBG("Successfully enabled %s timestamping on transport %s\n", methodStr,
		    transport->header.instanceName);
	return CCK_TRUE;

    }

    CCK_DBG("Failed to enable %s timestamping on transport %s", methodStr,
		    transport->header.instanceName);

#ifdef SO_TIMESTAMP
    if(setsockopt(transport->fd, SOL_SOCKET, SO_TIMESTAMP, &val, sizeof(int)) == 0) {

	CCK_DBG("Successfully enabled fallback SO_TIMESTAMP timestamping on transport %s\n",
		    transport->header.instanceName);

	caps->socketOption = SO_TIMESTAMP;
	caps->scmType = SCM_TIMESTAMP;
	caps->timestampType = CCK_TIMEVAL;
	caps->timestampSize = sizeof(struct timeval);
	return CCK_TRUE;

    } else {

	CCK_PERROR("Failed to enable fallback SO_TIMESTAMP timestamping on transport %s",
		    transport->header.instanceName);
	return CCK_FALSE;
    }
#endif

    return CCK_FALSE;

}

CckBool
cckGetSwTimestamp(struct cmsghdr* cmsg, CckSocketTimestampCaps* caps, CckTimestamp* timestamp)
{

    typedef struct{
	struct timespec sys;
	struct timespec hwt;
	struct timespec hwr;
    } scmts;

    timestamp->seconds = 0;
    timestamp->nanoseconds = 0;

    if (cmsg->cmsg_type != caps->scmType)
	return CCK_FALSE;

    switch(caps->timestampType) {
	case CCK_SCMTIMESTAMPING:
	    {
		scmts* scm = (scmts*)CMSG_DATA(cmsg);
		timestamp->seconds = scm->hwr.tv_sec;
		timestamp->nanoseconds = scm->hwr.tv_nsec;
	    }
	    goto success;
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
