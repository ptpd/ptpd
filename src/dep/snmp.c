/*-
 * Copyright (c) 2012 The IMS Company
 *                    Vincent Bernat
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
 * @file   snmp.c
 * @author Vincent Bernat <bernat@luffy.cx>
 * @date   Sat Jun 23 23:08:05 2012
 *
 * @brief  SNMP related functions
 */

#include "../ptpd.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

//  snmp_shutdown("example-demon");


/* Hard to get header... */
extern int header_generic(struct variable *, oid *, size_t *, int,
			  size_t *, WriteMethod **);

#define PTPBASE_SYSTEM_PROFILE           1
#define PTPBASE_DOMAIN_CLOCK_PORTS_TOTAL 2
#define PTPBASE_SYSTEM_DOMAIN_TOTALS     3
#define PTPBASE_CLOCK_CURRENT_DS_STEPS_REMOVED      4
#define PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER 5
#define PTPBASE_CLOCK_CURRENT_DS_MEAN_PATH_DELAY    6
#define PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ID      7
#define PTPBASE_CLOCK_PARENT_DS_PARENT_STATS        8
#define PTPBASE_CLOCK_PARENT_DS_OFFSET              9
#define PTPBASE_CLOCK_PARENT_DS_CLOCK_PH_CH_RATE    10
#define PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_IDENTITY   11
#define PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO1      12
#define PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO2      13
#define PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_CLASS 14
#define PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_ACCURACY 15
#define PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_OFFSET 16
#define PTPBASE_CLOCK_DEFAULT_DS_TWO_STEP_FLAG  17
#define PTPBASE_CLOCK_DEFAULT_DS_CLOCK_IDENTITY 18
#define PTPBASE_CLOCK_DEFAULT_DS_PRIO1          19
#define PTPBASE_CLOCK_DEFAULT_DS_PRIO2          20
#define PTPBASE_CLOCK_DEFAULT_DS_SLAVE_ONLY     21
#define PTPBASE_CLOCK_DEFAULT_DS_QUALITY_CLASS  22
#define PTPBASE_CLOCK_DEFAULT_DS_QUALITY_ACCURACY 23
#define PTPBASE_CLOCK_DEFAULT_DS_QUALITY_OFFSET 24
#define PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET_VALID 25
#define PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET       26
#define PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP59                   27
#define PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP61                   28
#define PTPBASE_CLOCK_TIME_PROPERTIES_DS_TIME_TRACEABLE           29
#define PTPBASE_CLOCK_TIME_PROPERTIES_DS_FREQ_TRACEABLE           30
#define PTPBASE_CLOCK_TIME_PROPERTIES_DS_PTP_TIMESCALE            31
#define PTPBASE_CLOCK_TIME_PROPERTIES_DS_SOURCE                   32
#define PTPBASE_CLOCK_PORT_NAME          33
#define PTPBASE_CLOCK_PORT_ROLE          34
#define PTPBASE_CLOCK_PORT_SYNC_ONE_STEP 35
#define PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS_TYPE 36
#define PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS      37
#define PTPBASE_CLOCK_PORT_NUM_ASSOCIATED_PORTS      38
#define PTPBASE_CLOCK_PORT_DS_PORT_NAME              39
#define PTPBASE_CLOCK_PORT_DS_PORT_IDENTITY          40
#define PTPBASE_CLOCK_PORT_DS_ANNOUNCEMENT_INTERVAL  41
#define PTPBASE_CLOCK_PORT_DS_ANNOUNCE_RCT_TIMEOUT   42
#define PTPBASE_CLOCK_PORT_DS_SYNC_INTERVAL          43
#define PTPBASE_CLOCK_PORT_DS_MIN_DELAY_REQ_INTERVAL 44
#define PTPBASE_CLOCK_PORT_DS_PEER_DELAY_REQ_INTERVAL 45
#define PTPBASE_CLOCK_PORT_DS_DELAY_MECH             46
#define PTPBASE_CLOCK_PORT_DS_PEER_MEAN_PATH_DELAY   47
#define PTPBASE_CLOCK_PORT_DS_GRANT_DURATION         48
#define PTPBASE_CLOCK_PORT_DS_PTP_VERSION            49
#define PTPBASE_CLOCK_PORT_RUNNING_NAME              50
#define PTPBASE_CLOCK_PORT_RUNNING_STATE             51
#define PTPBASE_CLOCK_PORT_RUNNING_ROLE              52
#define PTPBASE_CLOCK_PORT_RUNNING_INTERFACE_INDEX   53
#define PTPBASE_CLOCK_PORT_RUNNING_IPVERSION         54
#define PTPBASE_CLOCK_PORT_RUNNING_ENCAPSULATION_TYPE 55
#define PTPBASE_CLOCK_PORT_RUNNING_TX_MODE           56
#define PTPBASE_CLOCK_PORT_RUNNING_RX_MODE           57
#define PTPBASE_CLOCK_PORT_RUNNING_PACKETS_RECEIVED  58
#define PTPBASE_CLOCK_PORT_RUNNING_PACKETS_SENT      59

#define SNMP_PTP_ORDINARY_CLOCK 1
#define SNMP_PTP_CLOCK_INSTANCE 1	/* Only one instance */
#define SNMP_PTP_PORT_MASTER 1
#define SNMP_PTP_PORT_SLAVE  2
#define SNMP_IPv4 1
#define SNMP_PTP_TX_UNICAST 1
#define SNMP_PTP_TX_MULTICAST 2
#define SNMP_PTP_TX_MULTICAST_MIX 3

static oid ptp_oid[] = {1, 3, 6, 1, 4, 1, 39178, 100, 2};

static PtpClock *snmpPtpClock;
static RunTimeOpts *snmpRtOpts;

/* Helper functions to build header_*indexed_table() functions.  Those
   functions keep an internal state. They are not reentrant!
*/
/* {{{ */
struct snmpHeaderIndex {
	struct variable *vp;
	oid             *name;	 /* Requested/returned OID */
	size_t          *length; /* Length of above OID */
	int              exact;
	oid              best[MAX_OID_LEN]; /* Best OID */
	size_t           best_len;	    /* Best OID length */
	void            *entity;	    /* Best entity */
};

static int
snmpHeaderInit(struct snmpHeaderIndex *idx,
	       struct variable *vp, oid *name, size_t *length,
	       int exact, size_t *var_len, WriteMethod **write_method)
{
	/* If the requested OID name is less than OID prefix we
	   handle, adjust it to our prefix. */
        if ((snmp_oid_compare(name, *length, vp->name, vp->namelen)) < 0) {
                memcpy(name, vp->name, sizeof(oid) * vp->namelen);
                *length = vp->namelen;
        }
	/* Now, we can only handle OID matching our prefix. Those two
	   tests are not really necessary since NetSNMP won't give us
	   OID "above" our prefix. But this makes unit tests
	   easier.  */
	if (*length < vp->namelen) return 0;
	if (memcmp(name, vp->name, vp->namelen * sizeof(oid))) return 0;

	if(write_method != NULL) *write_method = 0;
	*var_len = sizeof(long);

	/* Initialize our header index structure */
	idx->vp = vp;
	idx->name = name;
	idx->length = length;
	idx->exact = exact;
	idx->best_len = 0;
	idx->entity = NULL;
	return 1;
}

static void
snmpHeaderIndexAdd(struct snmpHeaderIndex *idx,
		   oid *index, size_t len, void *entity)
{
	int      result;
	oid     *target;
	size_t   target_len;

        target = idx->name + idx->vp->namelen;
        target_len = *idx->length - idx->vp->namelen;
	if ((result = snmp_oid_compare(index, len, target, target_len)) < 0)
		return;		/* Too small. */
	if (idx->exact) {
		if (result == 0) {
			memcpy(idx->best, index, sizeof(oid) * len);
			idx->best_len = len;
			idx->entity = entity;
			return;
		}
		return;
	}
	if (result == 0)
		return;		/* Too small. */
	if (idx->best_len == 0 ||
	    (snmp_oid_compare(index, len,
			      idx->best,
			      idx->best_len) < 0)) {
		memcpy(idx->best, index, sizeof(oid) * len);
		idx->best_len = len;
		idx->entity = entity;
	}
}

static void*
snmpHeaderIndexBest(struct snmpHeaderIndex *idx)
{
	if (idx->entity == NULL)
		return NULL;
	if (idx->exact) {
		if (snmp_oid_compare(idx->name + idx->vp->namelen,
				     *idx->length - idx->vp->namelen,
				     idx->best, idx->best_len) == 0)
			return idx->entity;
		return NULL;
	}
	memcpy(idx->name + idx->vp->namelen,
	       idx->best, sizeof(oid) * idx->best_len);
	*idx->length = idx->vp->namelen + idx->best_len;
	return idx->entity;
}
/* }}} */


#define SNMP_LOCAL_VARIABLES			\
	static unsigned long long_ret;		\
	static U64 counter64_ret;		\
	static uint32_t ipaddr;			\
	Integer64 bigint;			\
	struct snmpHeaderIndex idx;		\
	(void)long_ret;				\
	(void)counter64_ret;			\
	(void)ipaddr;				\
	(void)bigint;				\
	(void)idx
#define SNMP_INDEXED_TABLE			\
	if (!snmpHeaderInit(&idx, vp, name,	\
			    length, exact,	\
			    var_len,		\
			    write_method))	\
		return NULL
#define SNMP_ADD_INDEX(index, len, w)		\
	snmpHeaderIndexAdd(&idx, index, len, w)
#define SNMP_BEST_MATCH				\
	snmpHeaderIndexBest(&idx)
#define SNMP_OCTETSTR(V, L)			\
	( *var_len = L,				\
	  (u_char *)V )
#define SNMP_COUNTER64(V)			\
	( counter64_ret.low = (V) & 0xffffffff,	\
	  counter64_ret.high = (V) >> 32,	\
	  *var_len = sizeof (counter64_ret),	\
	  (u_char*)&counter64_ret )
#define SNMP_TIMEINTERNAL(V)				\
	( *var_len = sizeof(counter64_ret),		\
	  internalTime_to_integer64(V, &bigint),	\
	  counter64_ret.low = htonl(bigint.lsb),	\
	  counter64_ret.high = htonl(bigint.msb),      	\
	  (u_char *)&counter64_ret )
#define SNMP_INTEGER(V)		    \
	( long_ret = (V),	    \
	  *var_len = sizeof (long_ret),		\
	  (u_char*)&long_ret )
#define SNMP_IPADDR(A)				\
	( ipaddr = A,				\
	  *var_len = sizeof (ipaddr),		\
	  (u_char*)&ipaddr )
#define SNMP_GAUGE SNMP_INTEGER
#define SNMP_UNSIGNED SNMP_INTEGER
#define SNMP_TRUE SNMP_INTEGER(1)
#define SNMP_FALSE SNMP_INTEGER(2)
#define SNMP_BOOLEAN(V)				\
	(V == TRUE)?SNMP_TRUE:SNMP_FALSE
#define SNMP_SIGNATURE struct variable *vp,	\
		oid *name,			\
		size_t *length,			\
		int exact,			\
		size_t *var_len,		\
		WriteMethod **write_method

/**
 * Handle SNMP scalar values.
 */
static u_char*
snmpScalars(SNMP_SIGNATURE) {
    SNMP_LOCAL_VARIABLES;

    if (header_generic(vp, name, length, exact, var_len, write_method))
	    return NULL;

    switch (vp->magic) {
    case PTPBASE_SYSTEM_PROFILE:
	    return SNMP_INTEGER(1);
    }

    return NULL;
}

/**
 * Handle ptpbaseSystemTable
 */
static u_char*
snmpSystemTable(SNMP_SIGNATURE) {
	oid index[2];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one index: one domain, one instance */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_CLOCK_INSTANCE;
	SNMP_ADD_INDEX(index, 2, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
	case PTPBASE_DOMAIN_CLOCK_PORTS_TOTAL:
		return SNMP_GAUGE(snmpPtpClock->numberPorts);
	}

	return NULL;
}

/**
 * Handle ptpbaseSystemDomainTable
 */
static u_char*
snmpSystemDomainTable(SNMP_SIGNATURE) {
	oid index[1];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one index: ordinary clock */
	index[0] = SNMP_PTP_ORDINARY_CLOCK;
	SNMP_ADD_INDEX(index, 1, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
	case PTPBASE_SYSTEM_DOMAIN_TOTALS:
		/* We only handle one domain... */
		return SNMP_UNSIGNED(1);
	}

	return NULL;
}

/**
 * Handle various ptpbaseClock*DSTable
 */
static u_char*
snmpClockDSTable(SNMP_SIGNATURE) {
	oid index[3];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	SNMP_ADD_INDEX(index, 3, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
	/* ptpbaseClockCurrentDSTable */
	case PTPBASE_CLOCK_CURRENT_DS_STEPS_REMOVED:
		return SNMP_UNSIGNED(snmpPtpClock->stepsRemoved);
	case PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER:
		return SNMP_TIMEINTERNAL(snmpPtpClock->offsetFromMaster);
	case PTPBASE_CLOCK_CURRENT_DS_MEAN_PATH_DELAY:
		return SNMP_TIMEINTERNAL(snmpPtpClock->meanPathDelay);
	/* ptpbaseClockParentDSTable */
	case PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ID:
		return SNMP_OCTETSTR(&snmpPtpClock->parentPortIdentity,
				     sizeof(PortIdentity));
	case PTPBASE_CLOCK_PARENT_DS_PARENT_STATS:
		return SNMP_BOOLEAN(snmpPtpClock->parentStats);
	case PTPBASE_CLOCK_PARENT_DS_OFFSET:
		return SNMP_INTEGER(snmpPtpClock->observedParentOffsetScaledLogVariance);
	case PTPBASE_CLOCK_PARENT_DS_CLOCK_PH_CH_RATE:
		return SNMP_INTEGER(snmpPtpClock->observedParentClockPhaseChangeRate);
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_IDENTITY:
		return SNMP_OCTETSTR(&snmpPtpClock->grandmasterIdentity,
				     sizeof(ClockIdentity));
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO1:
		return SNMP_UNSIGNED(snmpPtpClock->grandmasterPriority1);
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO2:
		return SNMP_UNSIGNED(snmpPtpClock->grandmasterPriority2);
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_CLASS:
		return SNMP_UNSIGNED(snmpPtpClock->grandmasterClockQuality.clockClass);
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_ACCURACY:
		return SNMP_INTEGER(snmpPtpClock->grandmasterClockQuality.clockAccuracy);
	case PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_OFFSET:
		return SNMP_INTEGER(snmpPtpClock->grandmasterClockQuality.offsetScaledLogVariance);
	/* ptpbaseClockDefaultDSTable */
	case PTPBASE_CLOCK_DEFAULT_DS_TWO_STEP_FLAG:
		return SNMP_BOOLEAN(snmpPtpClock->twoStepFlag);
	case PTPBASE_CLOCK_DEFAULT_DS_CLOCK_IDENTITY:
		return SNMP_OCTETSTR(&snmpPtpClock->clockIdentity,
				     sizeof(ClockIdentity));
	case PTPBASE_CLOCK_DEFAULT_DS_PRIO1:
		return SNMP_UNSIGNED(snmpPtpClock->priority1);
	case PTPBASE_CLOCK_DEFAULT_DS_PRIO2:
		return SNMP_UNSIGNED(snmpPtpClock->priority2);
	case PTPBASE_CLOCK_DEFAULT_DS_SLAVE_ONLY:
		return SNMP_BOOLEAN(snmpPtpClock->slaveOnly);
	case PTPBASE_CLOCK_DEFAULT_DS_QUALITY_CLASS:
		return SNMP_UNSIGNED(snmpPtpClock->clockQuality.clockClass);
	case PTPBASE_CLOCK_DEFAULT_DS_QUALITY_ACCURACY:
		return SNMP_INTEGER(snmpPtpClock->clockQuality.clockAccuracy);
	case PTPBASE_CLOCK_DEFAULT_DS_QUALITY_OFFSET:
		return SNMP_INTEGER(snmpPtpClock->clockQuality.offsetScaledLogVariance);
	/* ptpbaseClockTimePropertiesDSTable */
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET_VALID:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.currentUtcOffsetValid);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET:
		return SNMP_INTEGER(snmpPtpClock->timePropertiesDS.currentUtcOffset);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP59:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.leap59);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP61:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.leap61);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_TIME_TRACEABLE:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.timeTraceable);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_FREQ_TRACEABLE:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.frequencyTraceable);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_PTP_TIMESCALE:
		return SNMP_BOOLEAN(snmpPtpClock->timePropertiesDS.ptpTimescale);
	case PTPBASE_CLOCK_TIME_PROPERTIES_DS_SOURCE:
		return SNMP_INTEGER(snmpPtpClock->timePropertiesDS.timeSource);
	}

	return NULL;
}

/**
 * Handle ptpbaseClockPort*Table
 */
static u_char*
snmpClockPortTable(SNMP_SIGNATURE) {
	oid index[4];
	SNMP_LOCAL_VARIABLES;
	SNMP_INDEXED_TABLE;

	/* We only have one valid index */
	index[0] = snmpPtpClock->domainNumber;
	index[1] = SNMP_PTP_ORDINARY_CLOCK;
	index[2] = SNMP_PTP_CLOCK_INSTANCE;
	index[3] = snmpPtpClock->portIdentity.portNumber;
	SNMP_ADD_INDEX(index, 4, snmpPtpClock);

	if (!SNMP_BEST_MATCH) return NULL;

	switch (vp->magic) {
	/* ptpbaseClockPortTable */
	case PTPBASE_CLOCK_PORT_NAME:
	case PTPBASE_CLOCK_PORT_DS_PORT_NAME:
	case PTPBASE_CLOCK_PORT_RUNNING_NAME:
		return SNMP_OCTETSTR(snmpPtpClock->user_description,
				     strlen(snmpPtpClock->user_description));
	case PTPBASE_CLOCK_PORT_ROLE:
		return SNMP_INTEGER((snmpPtpClock->portState == PTP_MASTER)?
				    SNMP_PTP_PORT_MASTER:SNMP_PTP_PORT_SLAVE);
	case PTPBASE_CLOCK_PORT_SYNC_ONE_STEP:
		return (snmpPtpClock->twoStepFlag == TRUE)?SNMP_FALSE:SNMP_TRUE;
	case PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS_TYPE:
		/* Only supports IPv4 */
		return SNMP_INTEGER(SNMP_IPv4);
	case PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS:
		if(snmpRtOpts->transport != UDP_IPV4)
		    return SNMP_IPADDR(0);
		if (snmpPtpClock->masterAddr)
		return SNMP_IPADDR(snmpPtpClock->masterAddr);
		if (snmpPtpClock->unicastDestinations[1].transportAddress)
			return SNMP_IPADDR(snmpPtpClock->unicastDestinations[1].transportAddress);
		if(snmpRtOpts->ipMode != IPMODE_MULTICAST) {
		    return(SNMP_IPADDR(snmpPtpClock->netPath.interfaceAddr.s_addr));
		} else {
		    return SNMP_IPADDR(snmpPtpClock->netPath.multicastAddr);
		}
	case PTPBASE_CLOCK_PORT_NUM_ASSOCIATED_PORTS:
		/* Either we are master and we use multicast and we
		 * consider we have a session or we are slave and we
		 * have only one master. */
		return SNMP_INTEGER(1);

	/* ptpbaseClockPortDSTable */
	case PTPBASE_CLOCK_PORT_DS_PORT_IDENTITY:
		return SNMP_OCTETSTR(&snmpPtpClock->portIdentity,
				     sizeof(PortIdentity));
	case PTPBASE_CLOCK_PORT_DS_ANNOUNCEMENT_INTERVAL:
		/* TODO: is it really logAnnounceInterval? */
		return SNMP_INTEGER(snmpPtpClock->logAnnounceInterval);
	case PTPBASE_CLOCK_PORT_DS_ANNOUNCE_RCT_TIMEOUT:
		return SNMP_INTEGER(snmpPtpClock->announceReceiptTimeout);
	case PTPBASE_CLOCK_PORT_DS_SYNC_INTERVAL:
		/* TODO: is it really logSyncInterval? */
		return SNMP_INTEGER(snmpPtpClock->logSyncInterval);
	case PTPBASE_CLOCK_PORT_DS_MIN_DELAY_REQ_INTERVAL:
		/* TODO: is it really logMinDelayReqInterval? */
		return SNMP_INTEGER(snmpPtpClock->logMinDelayReqInterval);
	case PTPBASE_CLOCK_PORT_DS_PEER_DELAY_REQ_INTERVAL:
		/* TODO: is it really logMinPdelayReqInterval? */
		return SNMP_INTEGER(snmpPtpClock->logMinPdelayReqInterval);
	case PTPBASE_CLOCK_PORT_DS_DELAY_MECH:
		return SNMP_INTEGER(snmpPtpClock->delayMechanism);
	case PTPBASE_CLOCK_PORT_DS_PEER_MEAN_PATH_DELAY:
		return SNMP_TIMEINTERNAL(snmpPtpClock->peerMeanPathDelay);
	case PTPBASE_CLOCK_PORT_DS_GRANT_DURATION:
		/* TODO: what is it? */
		return SNMP_UNSIGNED(0);
	case PTPBASE_CLOCK_PORT_DS_PTP_VERSION:
		return SNMP_INTEGER(snmpPtpClock->versionNumber);

	/* ptpbaseClockPortRunningTable */
	case PTPBASE_CLOCK_PORT_RUNNING_STATE:
		return SNMP_INTEGER(snmpPtpClock->portState);
	case PTPBASE_CLOCK_PORT_RUNNING_ROLE:
		return SNMP_INTEGER((snmpPtpClock->portState == PTP_MASTER)?
				    SNMP_PTP_PORT_MASTER:SNMP_PTP_PORT_SLAVE);
	case PTPBASE_CLOCK_PORT_RUNNING_INTERFACE_INDEX:
		/* TODO: maybe we can have it from the general configuration? */
		return SNMP_INTEGER(0);
	case PTPBASE_CLOCK_PORT_RUNNING_IPVERSION:
		/* IPv4 only */
		/* TODO: shouldn't we return SNMP_IPv4??? */
		return SNMP_INTEGER(4);
	case PTPBASE_CLOCK_PORT_RUNNING_ENCAPSULATION_TYPE:
		/* None. Moreover, the format is not really described in the MIB... */
		return SNMP_INTEGER(0);
	case PTPBASE_CLOCK_PORT_RUNNING_TX_MODE:
	case PTPBASE_CLOCK_PORT_RUNNING_RX_MODE:	
		if (snmpRtOpts->ipMode == IPMODE_UNICAST)
			return SNMP_INTEGER(SNMP_PTP_TX_UNICAST);
		if (snmpRtOpts->ipMode == IPMODE_HYBRID)
			return SNMP_INTEGER(SNMP_PTP_TX_MULTICAST_MIX);
		return SNMP_INTEGER(SNMP_PTP_TX_MULTICAST);
	case PTPBASE_CLOCK_PORT_RUNNING_PACKETS_RECEIVED:
		return SNMP_COUNTER64(snmpPtpClock->netPath.receivedPacketsTotal);
	case PTPBASE_CLOCK_PORT_RUNNING_PACKETS_SENT:
		return SNMP_COUNTER64(snmpPtpClock->netPath.sentPacketsTotal);
	}


	return NULL;
}

/**
 * MIB definition
 */
static struct variable7 snmpVariables[] = {
	/* ptpbaseSystemTable */
	{ PTPBASE_DOMAIN_CLOCK_PORTS_TOTAL, ASN_GAUGE, HANDLER_CAN_RONLY,
	  snmpSystemTable, 5, {1, 1, 1, 1, 3}},
	/* ptpbaseSystemDomainTable */
	{ PTPBASE_SYSTEM_DOMAIN_TOTALS, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpSystemDomainTable, 5, {1, 1, 2, 1, 2}},
	/* Scalars */
	{ PTPBASE_SYSTEM_PROFILE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpScalars, 3, {1, 1, 3}},
	/* ptpbaseClockCurrentDSTable */
	{ PTPBASE_CLOCK_CURRENT_DS_STEPS_REMOVED, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 1, 1, 4}},
	{ PTPBASE_CLOCK_CURRENT_DS_OFFSET_FROM_MASTER, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 1, 1, 5}},
	{ PTPBASE_CLOCK_CURRENT_DS_MEAN_PATH_DELAY, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 1, 1, 6}},
	/* ptpbaseClockParentDSTable */
	{ PTPBASE_CLOCK_PARENT_DS_PARENT_PORT_ID, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 4}},
	{ PTPBASE_CLOCK_PARENT_DS_PARENT_STATS, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 5}},
	{ PTPBASE_CLOCK_PARENT_DS_OFFSET, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 6}},
	{ PTPBASE_CLOCK_PARENT_DS_CLOCK_PH_CH_RATE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 7}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_IDENTITY, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 8}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO1, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 9}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_PRIO2, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 10}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_CLASS, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 11}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_ACCURACY, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 12}},
	{ PTPBASE_CLOCK_PARENT_DS_GM_CLOCK_QUALITY_OFFSET, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 2, 1, 13}},
	/* ptpbaseClockDefaultDSTable */
	{ PTPBASE_CLOCK_DEFAULT_DS_TWO_STEP_FLAG, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 4}},
	{ PTPBASE_CLOCK_DEFAULT_DS_CLOCK_IDENTITY, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 5}},
	{ PTPBASE_CLOCK_DEFAULT_DS_PRIO1, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 6}},
	{ PTPBASE_CLOCK_DEFAULT_DS_PRIO2, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 7}},
	{ PTPBASE_CLOCK_DEFAULT_DS_SLAVE_ONLY, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 8}},
	{ PTPBASE_CLOCK_DEFAULT_DS_QUALITY_CLASS, ASN_UNSIGNED, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 9}},
	{ PTPBASE_CLOCK_DEFAULT_DS_QUALITY_ACCURACY, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 10}},
	{ PTPBASE_CLOCK_DEFAULT_DS_QUALITY_OFFSET, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 3, 1, 11}},
	/* ptpbaseClockRunningTable: no data available for this table? */
	/* ptpbaseClockTimePropertiesDSTable */
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET_VALID, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 4}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 5}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP59, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 6}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_LEAP61, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 7}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_TIME_TRACEABLE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 8}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_FREQ_TRACEABLE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 9}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_PTP_TIMESCALE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 10}},
	{ PTPBASE_CLOCK_TIME_PROPERTIES_DS_SOURCE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockDSTable, 5, {1, 2, 5, 1, 11}},
	/* ptpbaseClockPortTable */
	{ PTPBASE_CLOCK_PORT_NAME, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 5}},
	{ PTPBASE_CLOCK_PORT_ROLE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 6}},
	{ PTPBASE_CLOCK_PORT_SYNC_ONE_STEP, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 7}},
	{ PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS_TYPE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 8}},
	{ PTPBASE_CLOCK_PORT_CURRENT_PEER_ADDRESS, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 9}},
	{ PTPBASE_CLOCK_PORT_NUM_ASSOCIATED_PORTS, ASN_GAUGE, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 7, 1, 10}},
	/* ptpbaseClockPortDSTable */
	{ PTPBASE_CLOCK_PORT_DS_PORT_NAME, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 5}},
	{ PTPBASE_CLOCK_PORT_DS_PORT_IDENTITY, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 6}},
	{ PTPBASE_CLOCK_PORT_DS_ANNOUNCEMENT_INTERVAL, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 7}},
	{ PTPBASE_CLOCK_PORT_DS_ANNOUNCE_RCT_TIMEOUT, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 8}},
	{ PTPBASE_CLOCK_PORT_DS_SYNC_INTERVAL, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 9}},
	{ PTPBASE_CLOCK_PORT_DS_MIN_DELAY_REQ_INTERVAL, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 10}},
	{ PTPBASE_CLOCK_PORT_DS_PEER_DELAY_REQ_INTERVAL, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 11}},
	{ PTPBASE_CLOCK_PORT_DS_DELAY_MECH, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 12}},
	{ PTPBASE_CLOCK_PORT_DS_PEER_MEAN_PATH_DELAY, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 13}},
	/* { PTPBASE_CLOCK_PORT_DS_GRANT_DURATION, ASN_UNSIGNED, HANDLER_CAN_RONLY, */
	/*   snmpClockPortTable, 5, {1, 2, 8, 1, 14}}, */
	{ PTPBASE_CLOCK_PORT_DS_PTP_VERSION, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 8, 1, 15}},
	/* ptpbaseClockPortRunningTable */
	{ PTPBASE_CLOCK_PORT_RUNNING_NAME, ASN_OCTET_STR, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 5}},
	{ PTPBASE_CLOCK_PORT_RUNNING_STATE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 6}},
	{ PTPBASE_CLOCK_PORT_RUNNING_ROLE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 7}},
	/* { PTPBASE_CLOCK_PORT_RUNNING_INTERFACE_INDEX, ASN_INTEGER, HANDLER_CAN_RONLY, */
	/*   snmpClockPortTable, 5, {1, 2, 9, 1, 8}}, */
	{ PTPBASE_CLOCK_PORT_RUNNING_IPVERSION, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 9}},
	/* { PTPBASE_CLOCK_PORT_RUNNING_ENCAPSULATION_TYPE, ASN_INTEGER, HANDLER_CAN_RONLY, */
	/*   snmpClockPortTable, 5, {1, 2, 9, 1, 10}}, */
	{ PTPBASE_CLOCK_PORT_RUNNING_TX_MODE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 11}},
	{ PTPBASE_CLOCK_PORT_RUNNING_RX_MODE, ASN_INTEGER, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 12}},
	{ PTPBASE_CLOCK_PORT_RUNNING_PACKETS_RECEIVED, ASN_COUNTER64, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 13}},
	{ PTPBASE_CLOCK_PORT_RUNNING_PACKETS_SENT, ASN_COUNTER64, HANDLER_CAN_RONLY,
	  snmpClockPortTable, 5, {1, 2, 9, 1, 14}},

};

/**
 * Log messages from NetSNMP subsystem.
 */
static int
snmpLogCallback(int major, int minor,
		void *serverarg, void *clientarg)
{
	struct snmp_log_message *slm = (struct snmp_log_message *)serverarg;
	char *msg = strdup (slm->msg);
	if (msg) msg[strlen(msg)-1] = '\0';

	switch (slm->priority)
	{
	case LOG_EMERG:   EMERGENCY("snmp[emerg]: %s\n",   msg?msg:slm->msg); break;
	case LOG_ALERT:   ALERT    ("snmp[alert]: %s\n",   msg?msg:slm->msg); break;
	case LOG_CRIT:    CRITICAL ("snmp[crit]: %s\n",    msg?msg:slm->msg); break;
	case LOG_ERR:     ERROR    ("snmp[err]: %s\n",     msg?msg:slm->msg); break;
	case LOG_WARNING: WARNING  ("snmp[warning]: %s\n", msg?msg:slm->msg); break;
	case LOG_NOTICE:  NOTICE   ("snmp[notice]: %s\n",  msg?msg:slm->msg); break;
	case LOG_INFO:    INFO     ("snmp[info]: %s\n",    msg?msg:slm->msg); break;
	case LOG_DEBUG:   DBGV     ("snmp[debug]: %s\n",   msg?msg:slm->msg); break;
	}
	free(msg);
	return SNMP_ERR_NOERROR;
}

/**
 * Initialisation of SNMP subsystem.
 */
void
snmpInit(RunTimeOpts *rtOpts, PtpClock *ptpClock) {

	netsnmp_enable_subagent();
	snmp_disable_log();
	snmp_enable_calllog();
	snmp_register_callback(SNMP_CALLBACK_LIBRARY,
			       SNMP_CALLBACK_LOGGING,
			       snmpLogCallback,
			       NULL);
	init_agent("ptpAgent");
	REGISTER_MIB("ptpMib", snmpVariables, variable7, ptp_oid);
	init_snmp("ptpAgent");

	/* Currently, ptpd only handle one clock. We put it in a
	 * global variable for the need of our subsystem. */
	snmpPtpClock = ptpClock;
	snmpRtOpts = rtOpts;
}

/**
 * Clean up and shut down the SNMP subagent
 */

void
snmpShutdown() {

	unregister_mib(ptp_oid, sizeof(ptp_oid) / sizeof(oid));
	snmp_shutdown("ptpMib");
	SOCK_CLEANUP;

}
