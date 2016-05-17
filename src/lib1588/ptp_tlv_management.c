/*-
 * Copyright (c) 2016      Wojciech Owczarek,
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

#include "ptp_primitives.h"
#include "ptp_tlv.h"
#include "ptp_message.h"

#include "tmp.h"

#include <stdlib.h>

/* begin generated code */

#define PTP_TYPE_FUNCDEFS( type ) \
static int unpack##type (type *data, char *buf, char *boundary); \
static int pack##type (char *buf, type *data, char *boundary); \
static void display##type (type *data); \
static void free##type (type *var);

PTP_TYPE_FUNCDEFS(PtpTlvAnnounceReceiptTimeout)
PTP_TYPE_FUNCDEFS(PtpTlvClockAccuracy)
PTP_TYPE_FUNCDEFS(PtpTlvClockDescription)
PTP_TYPE_FUNCDEFS(PtpTlvCurrentDataSet)
PTP_TYPE_FUNCDEFS(PtpTlvDefaultDataSet)
PTP_TYPE_FUNCDEFS(PtpTlvDelayMechanism)
PTP_TYPE_FUNCDEFS(PtpTlvDisablePort)
PTP_TYPE_FUNCDEFS(PtpTlvDomain)
PTP_TYPE_FUNCDEFS(PtpTlvEnablePort)
PTP_TYPE_FUNCDEFS(PtpTlvInitialize)
PTP_TYPE_FUNCDEFS(PtpTlvLogAnnounceInterval)
PTP_TYPE_FUNCDEFS(PtpTlvLogMinPdelayReqInterval)
PTP_TYPE_FUNCDEFS(PtpTlvLogSyncInterval)
PTP_TYPE_FUNCDEFS(PtpTlvManagement)
PTP_TYPE_FUNCDEFS(PtpTlvManagementErrorStatus)
PTP_TYPE_FUNCDEFS(PtpTlvNullManagement)
PTP_TYPE_FUNCDEFS(PtpTlvParentDataSet)
PTP_TYPE_FUNCDEFS(PtpTlvPortDataSet)
PTP_TYPE_FUNCDEFS(PtpTlvPriority1)
PTP_TYPE_FUNCDEFS(PtpTlvPriority2)
PTP_TYPE_FUNCDEFS(PtpTlvSlaveOnly)
PTP_TYPE_FUNCDEFS(PtpTlvTime)
PTP_TYPE_FUNCDEFS(PtpTlvTimePropertiesDataSet)
PTP_TYPE_FUNCDEFS(PtpTlvTimescaleProperties)
PTP_TYPE_FUNCDEFS(PtpTlvTraceabilityProperties)
PTP_TYPE_FUNCDEFS(PtpTlvUnicastNegotiationEnable)
PTP_TYPE_FUNCDEFS(PtpTlvUserDescription)
PTP_TYPE_FUNCDEFS(PtpTlvUtcProperties)
PTP_TYPE_FUNCDEFS(PtpTlvVersionNumber)

#undef PTP_TYPE_FUNCDEFS

static int unpackPtpTlvAnnounceReceiptTimeout(PtpTlvAnnounceReceiptTimeout *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/announceReceiptTimeout.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvAnnounceReceiptTimeout(char *buf, PtpTlvAnnounceReceiptTimeout *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/announceReceiptTimeout.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvAnnounceReceiptTimeout(PtpTlvAnnounceReceiptTimeout *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/announceReceiptTimeout.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvAnnounceReceiptTimeout(PtpTlvAnnounceReceiptTimeout *data) {

    PTPINFO("\tPtpTlvAnnounceReceiptTimeout: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/announceReceiptTimeout.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvClockAccuracy(PtpTlvClockAccuracy *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/clockAccuracy.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvClockAccuracy(char *buf, PtpTlvClockAccuracy *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/clockAccuracy.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvClockAccuracy(PtpTlvClockAccuracy *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/clockAccuracy.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvClockAccuracy(PtpTlvClockAccuracy *data) {

    PTPINFO("\tPtpTlvClockAccuracy: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/clockAccuracy.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvClockDescription(PtpTlvClockDescription *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/clockDescription.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvClockDescription(char *buf, PtpTlvClockDescription *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/clockDescription.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvClockDescription(PtpTlvClockDescription *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/clockDescription.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvClockDescription(PtpTlvClockDescription *data) {

    PTPINFO("\tPtpTlvClockDescription: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/clockDescription.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvCurrentDataSet(PtpTlvCurrentDataSet *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/currentDataSet.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvCurrentDataSet(char *buf, PtpTlvCurrentDataSet *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/currentDataSet.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvCurrentDataSet(PtpTlvCurrentDataSet *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/currentDataSet.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvCurrentDataSet(PtpTlvCurrentDataSet *data) {

    PTPINFO("\tPtpTlvCurrentDataSet: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/currentDataSet.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvDefaultDataSet(PtpTlvDefaultDataSet *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/defaultDataSet.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvDefaultDataSet(char *buf, PtpTlvDefaultDataSet *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/defaultDataSet.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvDefaultDataSet(PtpTlvDefaultDataSet *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/defaultDataSet.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvDefaultDataSet(PtpTlvDefaultDataSet *data) {

    PTPINFO("\tPtpTlvDefaultDataSet: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/defaultDataSet.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvDelayMechanism(PtpTlvDelayMechanism *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/delayMechanism.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvDelayMechanism(char *buf, PtpTlvDelayMechanism *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/delayMechanism.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvDelayMechanism(PtpTlvDelayMechanism *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/delayMechanism.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvDelayMechanism(PtpTlvDelayMechanism *data) {

    PTPINFO("\tPtpTlvDelayMechanism: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/delayMechanism.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvDisablePort(PtpTlvDisablePort *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/disablePort.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvDisablePort(char *buf, PtpTlvDisablePort *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/disablePort.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvDisablePort(PtpTlvDisablePort *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/disablePort.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvDisablePort(PtpTlvDisablePort *data) {

    PTPINFO("\tPtpTlvDisablePort: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/disablePort.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvDomain(PtpTlvDomain *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/domain.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvDomain(char *buf, PtpTlvDomain *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/domain.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvDomain(PtpTlvDomain *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/domain.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvDomain(PtpTlvDomain *data) {

    PTPINFO("\tPtpTlvDomain: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/domain.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvEnablePort(PtpTlvEnablePort *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/enablePort.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvEnablePort(char *buf, PtpTlvEnablePort *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/enablePort.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvEnablePort(PtpTlvEnablePort *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/enablePort.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvEnablePort(PtpTlvEnablePort *data) {

    PTPINFO("\tPtpTlvEnablePort: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/enablePort.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvInitialize(PtpTlvInitialize *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/initialize.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvInitialize(char *buf, PtpTlvInitialize *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/initialize.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvInitialize(PtpTlvInitialize *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/initialize.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvInitialize(PtpTlvInitialize *data) {

    PTPINFO("\tPtpTlvInitialize: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/initialize.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvLogAnnounceInterval(PtpTlvLogAnnounceInterval *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/logAnnounceInterval.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvLogAnnounceInterval(char *buf, PtpTlvLogAnnounceInterval *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/logAnnounceInterval.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvLogAnnounceInterval(PtpTlvLogAnnounceInterval *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/logAnnounceInterval.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvLogAnnounceInterval(PtpTlvLogAnnounceInterval *data) {

    PTPINFO("\tPtpTlvLogAnnounceInterval: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/logAnnounceInterval.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvLogMinPdelayReqInterval(PtpTlvLogMinPdelayReqInterval *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/logMinPdelayReqInterval.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvLogMinPdelayReqInterval(char *buf, PtpTlvLogMinPdelayReqInterval *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/logMinPdelayReqInterval.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvLogMinPdelayReqInterval(PtpTlvLogMinPdelayReqInterval *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/logMinPdelayReqInterval.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvLogMinPdelayReqInterval(PtpTlvLogMinPdelayReqInterval *data) {

    PTPINFO("\tPtpTlvLogMinPdelayReqInterval: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/logMinPdelayReqInterval.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvLogSyncInterval(PtpTlvLogSyncInterval *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/logSyncInterval.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvLogSyncInterval(char *buf, PtpTlvLogSyncInterval *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/logSyncInterval.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvLogSyncInterval(PtpTlvLogSyncInterval *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/logSyncInterval.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvLogSyncInterval(PtpTlvLogSyncInterval *data) {

    PTPINFO("\tPtpTlvLogSyncInterval: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/logSyncInterval.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvManagement(PtpTlvManagement *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/management.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvManagement(char *buf, PtpTlvManagement *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/management.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvManagement(PtpTlvManagement *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/management.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvManagement(PtpTlvManagement *data) {

    PTPINFO("\tPtpTlvManagement: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/management.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvManagementErrorStatus(PtpTlvManagementErrorStatus *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/managementErrorStatus.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvManagementErrorStatus(char *buf, PtpTlvManagementErrorStatus *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/managementErrorStatus.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvManagementErrorStatus(PtpTlvManagementErrorStatus *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/managementErrorStatus.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvManagementErrorStatus(PtpTlvManagementErrorStatus *data) {

    PTPINFO("\tPtpTlvManagementErrorStatus: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/managementErrorStatus.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvNullManagement(PtpTlvNullManagement *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/nullManagement.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvNullManagement(char *buf, PtpTlvNullManagement *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/nullManagement.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvNullManagement(PtpTlvNullManagement *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/nullManagement.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvNullManagement(PtpTlvNullManagement *data) {

    PTPINFO("\tPtpTlvNullManagement: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/nullManagement.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvParentDataSet(PtpTlvParentDataSet *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/parentDataSet.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvParentDataSet(char *buf, PtpTlvParentDataSet *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/parentDataSet.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvParentDataSet(PtpTlvParentDataSet *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/parentDataSet.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvParentDataSet(PtpTlvParentDataSet *data) {

    PTPINFO("\tPtpTlvParentDataSet: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/parentDataSet.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvPortDataSet(PtpTlvPortDataSet *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/portDataSet.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvPortDataSet(char *buf, PtpTlvPortDataSet *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/portDataSet.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvPortDataSet(PtpTlvPortDataSet *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/portDataSet.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvPortDataSet(PtpTlvPortDataSet *data) {

    PTPINFO("\tPtpTlvPortDataSet: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/portDataSet.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvPriority1(PtpTlvPriority1 *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/priority1.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvPriority1(char *buf, PtpTlvPriority1 *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/priority1.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvPriority1(PtpTlvPriority1 *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/priority1.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvPriority1(PtpTlvPriority1 *data) {

    PTPINFO("\tPtpTlvPriority1: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/priority1.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvPriority2(PtpTlvPriority2 *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/priority2.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvPriority2(char *buf, PtpTlvPriority2 *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/priority2.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvPriority2(PtpTlvPriority2 *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/priority2.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvPriority2(PtpTlvPriority2 *data) {

    PTPINFO("\tPtpTlvPriority2: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/priority2.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvSlaveOnly(PtpTlvSlaveOnly *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/slaveOnly.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvSlaveOnly(char *buf, PtpTlvSlaveOnly *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/slaveOnly.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvSlaveOnly(PtpTlvSlaveOnly *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/slaveOnly.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvSlaveOnly(PtpTlvSlaveOnly *data) {

    PTPINFO("\tPtpTlvSlaveOnly: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/slaveOnly.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvTime(PtpTlvTime *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/time.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvTime(char *buf, PtpTlvTime *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/time.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvTime(PtpTlvTime *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/time.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvTime(PtpTlvTime *data) {

    PTPINFO("\tPtpTlvTime: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/time.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvTimePropertiesDataSet(PtpTlvTimePropertiesDataSet *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/timePropertiesDataSet.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvTimePropertiesDataSet(char *buf, PtpTlvTimePropertiesDataSet *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/timePropertiesDataSet.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvTimePropertiesDataSet(PtpTlvTimePropertiesDataSet *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/timePropertiesDataSet.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvTimePropertiesDataSet(PtpTlvTimePropertiesDataSet *data) {

    PTPINFO("\tPtpTlvTimePropertiesDataSet: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/timePropertiesDataSet.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvTimescaleProperties(PtpTlvTimescaleProperties *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/timescaleProperties.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvTimescaleProperties(char *buf, PtpTlvTimescaleProperties *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/timescaleProperties.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvTimescaleProperties(PtpTlvTimescaleProperties *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/timescaleProperties.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvTimescaleProperties(PtpTlvTimescaleProperties *data) {

    PTPINFO("\tPtpTlvTimescaleProperties: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/timescaleProperties.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvTraceabilityProperties(PtpTlvTraceabilityProperties *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/traceabilityProperties.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvTraceabilityProperties(char *buf, PtpTlvTraceabilityProperties *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/traceabilityProperties.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvTraceabilityProperties(PtpTlvTraceabilityProperties *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/traceabilityProperties.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvTraceabilityProperties(PtpTlvTraceabilityProperties *data) {

    PTPINFO("\tPtpTlvTraceabilityProperties: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/traceabilityProperties.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvUnicastNegotiationEnable(PtpTlvUnicastNegotiationEnable *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/unicastNegotiationEnable.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvUnicastNegotiationEnable(char *buf, PtpTlvUnicastNegotiationEnable *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/unicastNegotiationEnable.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvUnicastNegotiationEnable(PtpTlvUnicastNegotiationEnable *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/unicastNegotiationEnable.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvUnicastNegotiationEnable(PtpTlvUnicastNegotiationEnable *data) {

    PTPINFO("\tPtpTlvUnicastNegotiationEnable: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/unicastNegotiationEnable.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvUserDescription(PtpTlvUserDescription *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/userDescription.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvUserDescription(char *buf, PtpTlvUserDescription *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/userDescription.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvUserDescription(PtpTlvUserDescription *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/userDescription.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvUserDescription(PtpTlvUserDescription *data) {

    PTPINFO("\tPtpTlvUserDescription: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/userDescription.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvUtcProperties(PtpTlvUtcProperties *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/utcProperties.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvUtcProperties(char *buf, PtpTlvUtcProperties *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/utcProperties.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvUtcProperties(PtpTlvUtcProperties *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/utcProperties.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvUtcProperties(PtpTlvUtcProperties *data) {

    PTPINFO("\tPtpTlvUtcProperties: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/utcProperties.def"
    #undef PROCESS_FIELD
}
static int unpackPtpTlvVersionNumber(PtpTlvVersionNumber *data, char *buf, char *boundary) {

    int offset = 0;

    #include "definitions/field_unpack_bufcheck.h"
    #include "definitions/managementTlv/versionNumber.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlvVersionNumber(char *buf, PtpTlvVersionNumber *data, char *boundary) {

    int offset = 0;

    #include "definitions/field_pack_bufcheck.h"
    #include "definitions/managementTlv/versionNumber.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlvVersionNumber(PtpTlvVersionNumber *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/managementTlv/versionNumber.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlvVersionNumber(PtpTlvVersionNumber *data) {

    PTPINFO("\tPtpTlvVersionNumber: \n");

    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t"#name, size);
    #include "definitions/managementTlv/versionNumber.def"
    #undef PROCESS_FIELD
}
int unpackPtpManagementTlvData(PtpTlv *tlv, char* buf, char* boundary) {

    int ret = 0;

    if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS) {

	ret = unpackPtpTlvManagementErrorStatus( &tlv->body.managementErrorStatus, buf, boundary);

	if(ret != tlv->lengthField) {
	    ret = PTP_MESSAGE_CORRUPT_TLV;
	}

    } else if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT) {

	tlv->body.management.lengthField = tlv->lengthField;

	ret = unpackPtpTlvManagement( &tlv->body.management, buf, boundary);

	if(ret != tlv->lengthField) {
	    ret = PTP_MESSAGE_CORRUPT_TLV;
	} else if (ret == PTP_MTLVLEN_EMPTY) {
	    /* nothing to unpack if this is an empty TLV - and mark this */
	    tlv->body.management.empty = TRUEx;
	} else {
	     switch(tlv->body.management.managementId) {
		case PTP_MGMTID_ANNOUNCE_RECEIPT_TIMEOUT:
			ret = unpackPtpTlvAnnounceReceiptTimeout( &tlv->body.management.body.announceReceiptTimeout, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_CLOCK_ACCURACY:
			ret = unpackPtpTlvClockAccuracy( &tlv->body.management.body.clockAccuracy, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_CLOCK_DESCRIPTION:
			ret = unpackPtpTlvClockDescription( &tlv->body.management.body.clockDescription, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_CURRENT_DATA_SET:
			ret = unpackPtpTlvCurrentDataSet( &tlv->body.management.body.currentDataSet, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_DEFAULT_DATA_SET:
			ret = unpackPtpTlvDefaultDataSet( &tlv->body.management.body.defaultDataSet, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_DELAY_MECHANISM:
			ret = unpackPtpTlvDelayMechanism( &tlv->body.management.body.delayMechanism, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_DISABLE_PORT:
			ret = unpackPtpTlvDisablePort( &tlv->body.management.body.disablePort, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_DOMAIN:
			ret = unpackPtpTlvDomain( &tlv->body.management.body.domain, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_ENABLE_PORT:
			ret = unpackPtpTlvEnablePort( &tlv->body.management.body.enablePort, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_INITIALIZE:
			ret = unpackPtpTlvInitialize( &tlv->body.management.body.initialize, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_LOG_ANNOUNCE_INTERVAL:
			ret = unpackPtpTlvLogAnnounceInterval( &tlv->body.management.body.logAnnounceInterval, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_LOG_MIN_PDELAY_REQ_INTERVAL:
			ret = unpackPtpTlvLogMinPdelayReqInterval( &tlv->body.management.body.logMinPdelayReqInterval, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_LOG_SYNC_INTERVAL:
			ret = unpackPtpTlvLogSyncInterval( &tlv->body.management.body.logSyncInterval, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_NULL_MANAGEMENT:
			ret = unpackPtpTlvNullManagement( &tlv->body.management.body.nullManagement, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_PARENT_DATA_SET:
			ret = unpackPtpTlvParentDataSet( &tlv->body.management.body.parentDataSet, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_PORT_DATA_SET:
			ret = unpackPtpTlvPortDataSet( &tlv->body.management.body.portDataSet, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_PRIORITY1:
			ret = unpackPtpTlvPriority1( &tlv->body.management.body.priority1, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_PRIORITY2:
			ret = unpackPtpTlvPriority2( &tlv->body.management.body.priority2, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_SLAVE_ONLY:
			ret = unpackPtpTlvSlaveOnly( &tlv->body.management.body.slaveOnly, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_TIME:
			ret = unpackPtpTlvTime( &tlv->body.management.body.time, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_TIME_PROPERTIES_DATA_SET:
			ret = unpackPtpTlvTimePropertiesDataSet( &tlv->body.management.body.timePropertiesDataSet, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_TIMESCALE_PROPERTIES:
			ret = unpackPtpTlvTimescaleProperties( &tlv->body.management.body.timescaleProperties, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_TRACEABILITY_PROPERTIES:
			ret = unpackPtpTlvTraceabilityProperties( &tlv->body.management.body.traceabilityProperties, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_UNICAST_NEGOTIATION_ENABLE:
			ret = unpackPtpTlvUnicastNegotiationEnable( &tlv->body.management.body.unicastNegotiationEnable, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_USER_DESCRIPTION:
			ret = unpackPtpTlvUserDescription( &tlv->body.management.body.userDescription, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_UTC_PROPERTIES:
			ret = unpackPtpTlvUtcProperties( &tlv->body.management.body.utcProperties, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
		case PTP_MGMTID_VERSION_NUMBER:
			ret = unpackPtpTlvVersionNumber( &tlv->body.management.body.versionNumber, tlv->body.management.dataField,
				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
			break;
default:
			ret =  PTP_MESSAGE_UNSUPPORTED_TLV;
			PTPDEBUG("ptp_tlv_management.c:unpackPtpManagementTlvData(): Unsupported TLV type %04x\n", tlv->tlvType);
			break;

	    }
	
	    if(ret >= 0) {
		ret += PTP_MTLV_DATAFIELD_OFFSET;
	    }

	}

        if(ret < tlv->lengthField) {
	    ret =  PTP_MESSAGE_CORRUPT_TLV;
	}
	
    } else {

	ret = PTP_MESSAGE_UNSUPPORTED_TLV;

    }
    return ret;

}
int packPtpManagementTlvData(char *buf, PtpTlv *tlv, char* boundary) {

    int ret = 0;

    if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS) {

	ret = packPtpTlvManagementErrorStatus( buf, &tlv->body.managementErrorStatus, boundary);

	if(!((buf == NULL) && (boundary == NULL))) {
	    if(ret != tlv->lengthField) {
		ret = PTP_MESSAGE_CORRUPT_TLV;
	    }
	}

    } else if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT) {

	if(tlv->body.management.empty) {
	    ret = 0;
	} else {
	    if(!((buf == NULL) && (boundary == NULL))) {
		tlv->body.management.dataField = calloc(1, tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET);
		if(tlv->body.management.dataField == NULL) {
		    return PTP_MESSAGE_OTHER_ERROR;
		}
	    }

	    switch(tlv->body.management.managementId) {
			case PTP_MGMTID_ANNOUNCE_RECEIPT_TIMEOUT:
				ret = packPtpTlvAnnounceReceiptTimeout( tlv->body.management.dataField, &tlv->body.management.body.announceReceiptTimeout,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_CLOCK_ACCURACY:
				ret = packPtpTlvClockAccuracy( tlv->body.management.dataField, &tlv->body.management.body.clockAccuracy,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_CLOCK_DESCRIPTION:
				ret = packPtpTlvClockDescription( tlv->body.management.dataField, &tlv->body.management.body.clockDescription,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_CURRENT_DATA_SET:
				ret = packPtpTlvCurrentDataSet( tlv->body.management.dataField, &tlv->body.management.body.currentDataSet,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_DEFAULT_DATA_SET:
				ret = packPtpTlvDefaultDataSet( tlv->body.management.dataField, &tlv->body.management.body.defaultDataSet,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_DELAY_MECHANISM:
				ret = packPtpTlvDelayMechanism( tlv->body.management.dataField, &tlv->body.management.body.delayMechanism,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_DISABLE_PORT:
				ret = packPtpTlvDisablePort( tlv->body.management.dataField, &tlv->body.management.body.disablePort,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_DOMAIN:
				ret = packPtpTlvDomain( tlv->body.management.dataField, &tlv->body.management.body.domain,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_ENABLE_PORT:
				ret = packPtpTlvEnablePort( tlv->body.management.dataField, &tlv->body.management.body.enablePort,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_INITIALIZE:
				ret = packPtpTlvInitialize( tlv->body.management.dataField, &tlv->body.management.body.initialize,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_LOG_ANNOUNCE_INTERVAL:
				ret = packPtpTlvLogAnnounceInterval( tlv->body.management.dataField, &tlv->body.management.body.logAnnounceInterval,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_LOG_MIN_PDELAY_REQ_INTERVAL:
				ret = packPtpTlvLogMinPdelayReqInterval( tlv->body.management.dataField, &tlv->body.management.body.logMinPdelayReqInterval,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_LOG_SYNC_INTERVAL:
				ret = packPtpTlvLogSyncInterval( tlv->body.management.dataField, &tlv->body.management.body.logSyncInterval,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_NULL_MANAGEMENT:
				ret = packPtpTlvNullManagement( tlv->body.management.dataField, &tlv->body.management.body.nullManagement,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_PARENT_DATA_SET:
				ret = packPtpTlvParentDataSet( tlv->body.management.dataField, &tlv->body.management.body.parentDataSet,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_PORT_DATA_SET:
				ret = packPtpTlvPortDataSet( tlv->body.management.dataField, &tlv->body.management.body.portDataSet,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_PRIORITY1:
				ret = packPtpTlvPriority1( tlv->body.management.dataField, &tlv->body.management.body.priority1,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_PRIORITY2:
				ret = packPtpTlvPriority2( tlv->body.management.dataField, &tlv->body.management.body.priority2,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_SLAVE_ONLY:
				ret = packPtpTlvSlaveOnly( tlv->body.management.dataField, &tlv->body.management.body.slaveOnly,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_TIME:
				ret = packPtpTlvTime( tlv->body.management.dataField, &tlv->body.management.body.time,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_TIME_PROPERTIES_DATA_SET:
				ret = packPtpTlvTimePropertiesDataSet( tlv->body.management.dataField, &tlv->body.management.body.timePropertiesDataSet,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_TIMESCALE_PROPERTIES:
				ret = packPtpTlvTimescaleProperties( tlv->body.management.dataField, &tlv->body.management.body.timescaleProperties,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_TRACEABILITY_PROPERTIES:
				ret = packPtpTlvTraceabilityProperties( tlv->body.management.dataField, &tlv->body.management.body.traceabilityProperties,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_UNICAST_NEGOTIATION_ENABLE:
				ret = packPtpTlvUnicastNegotiationEnable( tlv->body.management.dataField, &tlv->body.management.body.unicastNegotiationEnable,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_USER_DESCRIPTION:
				ret = packPtpTlvUserDescription( tlv->body.management.dataField, &tlv->body.management.body.userDescription,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_UTC_PROPERTIES:
				ret = packPtpTlvUtcProperties( tlv->body.management.dataField, &tlv->body.management.body.utcProperties,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
			case PTP_MGMTID_VERSION_NUMBER:
				ret = packPtpTlvVersionNumber( tlv->body.management.dataField, &tlv->body.management.body.versionNumber,
					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );
				break;
default:
			    ret =  PTP_MESSAGE_UNSUPPORTED_TLV;
			    PTPDEBUG("ptp_tlv_management.c:packPtpManagementTlvData(): Unsupported management ID %04x\n",
				tlv->body.management.managementId);
		    	    break;

		}
	    }
	    if(ret >= 0) {
		ret += PTP_MTLV_DATAFIELD_OFFSET;
	    }

	    if(!((buf == NULL) && (boundary == NULL))) {

		if(ret != tlv->lengthField) {
		    ret =  PTP_MESSAGE_CORRUPT_TLV;
		} else if (ret >= 0) {
		    tlv->body.management.lengthField = tlv->lengthField;
		    ret = packPtpTlvManagement( tlv->valueField, &tlv->body.management, tlv->valueField + tlv->lengthField);
		    if(ret != tlv->lengthField) {
			ret = PTP_MESSAGE_CORRUPT_TLV;
		    }
		}

	    }

    } else {

	ret = PTP_MESSAGE_UNSUPPORTED_TLV;
	PTPDEBUG("ptp_tlv_management.c:packPtpManagementTlvData(): Unsupported TLV type %04x\n", tlv->tlvType);

    }
    return ret;

}
void displayPtpManagementTlvData(PtpTlv *tlv) {

    if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS) {

	displayPtpTlvManagementErrorStatus( &tlv->body.managementErrorStatus);

    } else if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT) {

	displayPtpTlvManagement(&tlv->body.management);
	    if(tlv->body.management.empty) {
		PTPINFO("\t(empty management TLV body)\n");
	    } else {
		switch(tlv->body.management.managementId) {
		case PTP_MGMTID_ANNOUNCE_RECEIPT_TIMEOUT:
			displayPtpTlvAnnounceReceiptTimeout( &tlv->body.management.body.announceReceiptTimeout );
			break;
		case PTP_MGMTID_CLOCK_ACCURACY:
			displayPtpTlvClockAccuracy( &tlv->body.management.body.clockAccuracy );
			break;
		case PTP_MGMTID_CLOCK_DESCRIPTION:
			displayPtpTlvClockDescription( &tlv->body.management.body.clockDescription );
			break;
		case PTP_MGMTID_CURRENT_DATA_SET:
			displayPtpTlvCurrentDataSet( &tlv->body.management.body.currentDataSet );
			break;
		case PTP_MGMTID_DEFAULT_DATA_SET:
			displayPtpTlvDefaultDataSet( &tlv->body.management.body.defaultDataSet );
			break;
		case PTP_MGMTID_DELAY_MECHANISM:
			displayPtpTlvDelayMechanism( &tlv->body.management.body.delayMechanism );
			break;
		case PTP_MGMTID_DISABLE_PORT:
			displayPtpTlvDisablePort( &tlv->body.management.body.disablePort );
			break;
		case PTP_MGMTID_DOMAIN:
			displayPtpTlvDomain( &tlv->body.management.body.domain );
			break;
		case PTP_MGMTID_ENABLE_PORT:
			displayPtpTlvEnablePort( &tlv->body.management.body.enablePort );
			break;
		case PTP_MGMTID_INITIALIZE:
			displayPtpTlvInitialize( &tlv->body.management.body.initialize );
			break;
		case PTP_MGMTID_LOG_ANNOUNCE_INTERVAL:
			displayPtpTlvLogAnnounceInterval( &tlv->body.management.body.logAnnounceInterval );
			break;
		case PTP_MGMTID_LOG_MIN_PDELAY_REQ_INTERVAL:
			displayPtpTlvLogMinPdelayReqInterval( &tlv->body.management.body.logMinPdelayReqInterval );
			break;
		case PTP_MGMTID_LOG_SYNC_INTERVAL:
			displayPtpTlvLogSyncInterval( &tlv->body.management.body.logSyncInterval );
			break;
		case PTP_MGMTID_NULL_MANAGEMENT:
			displayPtpTlvNullManagement( &tlv->body.management.body.nullManagement );
			break;
		case PTP_MGMTID_PARENT_DATA_SET:
			displayPtpTlvParentDataSet( &tlv->body.management.body.parentDataSet );
			break;
		case PTP_MGMTID_PORT_DATA_SET:
			displayPtpTlvPortDataSet( &tlv->body.management.body.portDataSet );
			break;
		case PTP_MGMTID_PRIORITY1:
			displayPtpTlvPriority1( &tlv->body.management.body.priority1 );
			break;
		case PTP_MGMTID_PRIORITY2:
			displayPtpTlvPriority2( &tlv->body.management.body.priority2 );
			break;
		case PTP_MGMTID_SLAVE_ONLY:
			displayPtpTlvSlaveOnly( &tlv->body.management.body.slaveOnly );
			break;
		case PTP_MGMTID_TIME:
			displayPtpTlvTime( &tlv->body.management.body.time );
			break;
		case PTP_MGMTID_TIME_PROPERTIES_DATA_SET:
			displayPtpTlvTimePropertiesDataSet( &tlv->body.management.body.timePropertiesDataSet );
			break;
		case PTP_MGMTID_TIMESCALE_PROPERTIES:
			displayPtpTlvTimescaleProperties( &tlv->body.management.body.timescaleProperties );
			break;
		case PTP_MGMTID_TRACEABILITY_PROPERTIES:
			displayPtpTlvTraceabilityProperties( &tlv->body.management.body.traceabilityProperties );
			break;
		case PTP_MGMTID_UNICAST_NEGOTIATION_ENABLE:
			displayPtpTlvUnicastNegotiationEnable( &tlv->body.management.body.unicastNegotiationEnable );
			break;
		case PTP_MGMTID_USER_DESCRIPTION:
			displayPtpTlvUserDescription( &tlv->body.management.body.userDescription );
			break;
		case PTP_MGMTID_UTC_PROPERTIES:
			displayPtpTlvUtcProperties( &tlv->body.management.body.utcProperties );
			break;
		case PTP_MGMTID_VERSION_NUMBER:
			displayPtpTlvVersionNumber( &tlv->body.management.body.versionNumber );
			break;
default:
		    PTPINFO("Unsupported management ID %04x\n", tlv->body.management.managementId);
			break;

	    }
	}
    } else {
	    PTPINFO("Unsupported TLV type %04x\n", tlv->tlvType);
    }

}
void freePtpManagementTlvData(PtpTlv *tlv) {

    if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS) {

	freePtpTlvManagementErrorStatus( &tlv->body.managementErrorStatus);

    } else if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT) {

	    switch(tlv->body.management.managementId) {
		case PTP_MGMTID_ANNOUNCE_RECEIPT_TIMEOUT:
			freePtpTlvAnnounceReceiptTimeout( &tlv->body.management.body.announceReceiptTimeout );
			break;
		case PTP_MGMTID_CLOCK_ACCURACY:
			freePtpTlvClockAccuracy( &tlv->body.management.body.clockAccuracy );
			break;
		case PTP_MGMTID_CLOCK_DESCRIPTION:
			freePtpTlvClockDescription( &tlv->body.management.body.clockDescription );
			break;
		case PTP_MGMTID_CURRENT_DATA_SET:
			freePtpTlvCurrentDataSet( &tlv->body.management.body.currentDataSet );
			break;
		case PTP_MGMTID_DEFAULT_DATA_SET:
			freePtpTlvDefaultDataSet( &tlv->body.management.body.defaultDataSet );
			break;
		case PTP_MGMTID_DELAY_MECHANISM:
			freePtpTlvDelayMechanism( &tlv->body.management.body.delayMechanism );
			break;
		case PTP_MGMTID_DISABLE_PORT:
			freePtpTlvDisablePort( &tlv->body.management.body.disablePort );
			break;
		case PTP_MGMTID_DOMAIN:
			freePtpTlvDomain( &tlv->body.management.body.domain );
			break;
		case PTP_MGMTID_ENABLE_PORT:
			freePtpTlvEnablePort( &tlv->body.management.body.enablePort );
			break;
		case PTP_MGMTID_INITIALIZE:
			freePtpTlvInitialize( &tlv->body.management.body.initialize );
			break;
		case PTP_MGMTID_LOG_ANNOUNCE_INTERVAL:
			freePtpTlvLogAnnounceInterval( &tlv->body.management.body.logAnnounceInterval );
			break;
		case PTP_MGMTID_LOG_MIN_PDELAY_REQ_INTERVAL:
			freePtpTlvLogMinPdelayReqInterval( &tlv->body.management.body.logMinPdelayReqInterval );
			break;
		case PTP_MGMTID_LOG_SYNC_INTERVAL:
			freePtpTlvLogSyncInterval( &tlv->body.management.body.logSyncInterval );
			break;
		case PTP_MGMTID_NULL_MANAGEMENT:
			freePtpTlvNullManagement( &tlv->body.management.body.nullManagement );
			break;
		case PTP_MGMTID_PARENT_DATA_SET:
			freePtpTlvParentDataSet( &tlv->body.management.body.parentDataSet );
			break;
		case PTP_MGMTID_PORT_DATA_SET:
			freePtpTlvPortDataSet( &tlv->body.management.body.portDataSet );
			break;
		case PTP_MGMTID_PRIORITY1:
			freePtpTlvPriority1( &tlv->body.management.body.priority1 );
			break;
		case PTP_MGMTID_PRIORITY2:
			freePtpTlvPriority2( &tlv->body.management.body.priority2 );
			break;
		case PTP_MGMTID_SLAVE_ONLY:
			freePtpTlvSlaveOnly( &tlv->body.management.body.slaveOnly );
			break;
		case PTP_MGMTID_TIME:
			freePtpTlvTime( &tlv->body.management.body.time );
			break;
		case PTP_MGMTID_TIME_PROPERTIES_DATA_SET:
			freePtpTlvTimePropertiesDataSet( &tlv->body.management.body.timePropertiesDataSet );
			break;
		case PTP_MGMTID_TIMESCALE_PROPERTIES:
			freePtpTlvTimescaleProperties( &tlv->body.management.body.timescaleProperties );
			break;
		case PTP_MGMTID_TRACEABILITY_PROPERTIES:
			freePtpTlvTraceabilityProperties( &tlv->body.management.body.traceabilityProperties );
			break;
		case PTP_MGMTID_UNICAST_NEGOTIATION_ENABLE:
			freePtpTlvUnicastNegotiationEnable( &tlv->body.management.body.unicastNegotiationEnable );
			break;
		case PTP_MGMTID_USER_DESCRIPTION:
			freePtpTlvUserDescription( &tlv->body.management.body.userDescription );
			break;
		case PTP_MGMTID_UTC_PROPERTIES:
			freePtpTlvUtcProperties( &tlv->body.management.body.utcProperties );
			break;
		case PTP_MGMTID_VERSION_NUMBER:
			freePtpTlvVersionNumber( &tlv->body.management.body.versionNumber );
			break;
default:
			    PTPDEBUG("ptp_tlv_management.c:freePtpManagementTlvData(): Unsupported management ID %04x\n",
				tlv->body.management.managementId);
			break;

	    }

	freePtpTlvManagement(&tlv->body.management);

    } else {
	PTPDEBUG("ptp_tlv_management.c:freePtpManagementTlvData(): Unsupported TLV type %04x\n", tlv->tlvType);
    }

}
/* end generated code */

