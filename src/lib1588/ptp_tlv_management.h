/*-
 * Copyright (c) 2016-2017              Wojciech Owczarek,
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

#ifndef PTP_TLV_MANAGEMENT_H_
#define PTP_TLV_MANAGEMENT_H_

#include "ptp_tlv.h"

/* management TLV dataField offset from TLV valueField */
#define PTP_MTLV_DATAFIELD_OFFSET	2
/* length of an empty TLV (managementID only) */
#define PTP_MTLVLEN_EMPTY		2

/* begin generated code */

/* Table 40: ManagementId values */

enum {

	PTP_MGMTID_ANNOUNCE_RECEIPT_TIMEOUT = 0x200A,
	PTP_MGMTID_CLOCK_ACCURACY = 0x2010,
	PTP_MGMTID_CLOCK_DESCRIPTION = 0x0001,
	PTP_MGMTID_CURRENT_DATA_SET = 0x2001,
	PTP_MGMTID_DEFAULT_DATA_SET = 0x2000,
	PTP_MGMTID_DELAY_MECHANISM = 0x6000,
	PTP_MGMTID_DISABLE_PORT = 0x200E,
	PTP_MGMTID_DOMAIN = 0x2007,
	PTP_MGMTID_ENABLE_PORT = 0x200D,
	PTP_MGMTID_INITIALIZE = 0x0005,
	PTP_MGMTID_LOG_ANNOUNCE_INTERVAL = 0x2009,
	PTP_MGMTID_LOG_MIN_PDELAY_REQ_INTERVAL = 0x6001,
	PTP_MGMTID_LOG_SYNC_INTERVAL = 0x200B,
	PTP_MGMTID_NULL_MANAGEMENT = 0x0000,
	PTP_MGMTID_PARENT_DATA_SET = 0x2002,
	PTP_MGMTID_PORT_DATA_SET = 0x2004,
	PTP_MGMTID_PRIORITY1 = 0x2005,
	PTP_MGMTID_PRIORITY2 = 0x2006,
	PTP_MGMTID_SLAVE_ONLY = 0x2008,
	PTP_MGMTID_TIME = 0x200F,
	PTP_MGMTID_TIME_PROPERTIES_DATA_SET = 0x2003,
	PTP_MGMTID_TIMESCALE_PROPERTIES = 0x2013,
	PTP_MGMTID_TRACEABILITY_PROPERTIES = 0x2012,
	PTP_MGMTID_UNICAST_NEGOTIATION_ENABLE = 0x2014,
	PTP_MGMTID_USER_DESCRIPTION = 0x0002,
	PTP_MGMTID_UTC_PROPERTIES = 0x2011,
	PTP_MGMTID_VERSION_NUMBER = 0x200C,

};

/* management TLV data types */

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/announceReceiptTimeout.def"
	#undef PROCESS_FIELD
} PtpTlvAnnounceReceiptTimeout;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/clockAccuracy.def"
	#undef PROCESS_FIELD
} PtpTlvClockAccuracy;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/clockDescription.def"
	#undef PROCESS_FIELD
} PtpTlvClockDescription;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/currentDataSet.def"
	#undef PROCESS_FIELD
} PtpTlvCurrentDataSet;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/defaultDataSet.def"
	#undef PROCESS_FIELD
} PtpTlvDefaultDataSet;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/delayMechanism.def"
	#undef PROCESS_FIELD
} PtpTlvDelayMechanism;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/disablePort.def"
	#undef PROCESS_FIELD
} PtpTlvDisablePort;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/domain.def"
	#undef PROCESS_FIELD
} PtpTlvDomain;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/enablePort.def"
	#undef PROCESS_FIELD
} PtpTlvEnablePort;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/initialize.def"
	#undef PROCESS_FIELD
} PtpTlvInitialize;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/logAnnounceInterval.def"
	#undef PROCESS_FIELD
} PtpTlvLogAnnounceInterval;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/logMinPdelayReqInterval.def"
	#undef PROCESS_FIELD
} PtpTlvLogMinPdelayReqInterval;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/logSyncInterval.def"
	#undef PROCESS_FIELD
} PtpTlvLogSyncInterval;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/nullManagement.def"
	#undef PROCESS_FIELD
} PtpTlvNullManagement;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/parentDataSet.def"
	#undef PROCESS_FIELD
} PtpTlvParentDataSet;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/portDataSet.def"
	#undef PROCESS_FIELD
} PtpTlvPortDataSet;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/priority1.def"
	#undef PROCESS_FIELD
} PtpTlvPriority1;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/priority2.def"
	#undef PROCESS_FIELD
} PtpTlvPriority2;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/slaveOnly.def"
	#undef PROCESS_FIELD
} PtpTlvSlaveOnly;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/time.def"
	#undef PROCESS_FIELD
} PtpTlvTime;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/timePropertiesDataSet.def"
	#undef PROCESS_FIELD
} PtpTlvTimePropertiesDataSet;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/timescaleProperties.def"
	#undef PROCESS_FIELD
} PtpTlvTimescaleProperties;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/traceabilityProperties.def"
	#undef PROCESS_FIELD
} PtpTlvTraceabilityProperties;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/unicastNegotiationEnable.def"
	#undef PROCESS_FIELD
} PtpTlvUnicastNegotiationEnable;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/userDescription.def"
	#undef PROCESS_FIELD
} PtpTlvUserDescription;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/utcProperties.def"
	#undef PROCESS_FIELD
} PtpTlvUtcProperties;

typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/versionNumber.def"
	#undef PROCESS_FIELD
} PtpTlvVersionNumber;

/* management TLV bodybag */

typedef union {

		PtpTlvAnnounceReceiptTimeout announceReceiptTimeout;
		PtpTlvClockAccuracy clockAccuracy;
		PtpTlvClockDescription clockDescription;
		PtpTlvCurrentDataSet currentDataSet;
		PtpTlvDefaultDataSet defaultDataSet;
		PtpTlvDelayMechanism delayMechanism;
		PtpTlvDisablePort disablePort;
		PtpTlvDomain domain;
		PtpTlvEnablePort enablePort;
		PtpTlvInitialize initialize;
		PtpTlvLogAnnounceInterval logAnnounceInterval;
		PtpTlvLogMinPdelayReqInterval logMinPdelayReqInterval;
		PtpTlvLogSyncInterval logSyncInterval;
		PtpTlvNullManagement nullManagement;
		PtpTlvParentDataSet parentDataSet;
		PtpTlvPortDataSet portDataSet;
		PtpTlvPriority1 priority1;
		PtpTlvPriority2 priority2;
		PtpTlvSlaveOnly slaveOnly;
		PtpTlvTime time;
		PtpTlvTimePropertiesDataSet timePropertiesDataSet;
		PtpTlvTimescaleProperties timescaleProperties;
		PtpTlvTraceabilityProperties traceabilityProperties;
		PtpTlvUnicastNegotiationEnable unicastNegotiationEnable;
		PtpTlvUserDescription userDescription;
		PtpTlvUtcProperties utcProperties;
		PtpTlvVersionNumber versionNumber;

} PtpManagementTlvBody;

/* Management TLV container */

typedef struct {

    PtpUInteger16 lengthField; /* from parent PtpTlv */

    #include "def/field_declare.h"
    #include "def/managementTlv/management.def"
    #undef PROCESS_FIELD

    PtpManagementTlvBody body;
    PtpBoolean empty;

} PtpTlvManagement;

typedef struct {

    #include "def/field_declare.h"
    #include "def/managementTlv/managementErrorStatus.def"
    #undef PROCESS_FIELD

} PtpTlvManagementErrorStatus;

/* end generated code */

#endif /* PTP_TLV_SIGNALING_H */
