/*-
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments,
 *                         Tomasz Kleinschmidt
 * Copyright (c) 2009-2010 George V. Neville-Neil, 
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen
 *
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
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
 * @file        datatypes.h
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       Main structures used in ptpdv2.
 *
 * This header file defines structures defined by the spec 
 * and needed messages structures.
 */

#ifndef DATATYPES_H
#define	DATATYPES_H

#include "constants_dep.h"
#include "datatypes_dep.h"

/**
 * @brief The ClockIdentity type identifies a clock
 */
typedef Octet ClockIdentity[CLOCK_IDENTITY_LENGTH];

/**
 * @brief The TimeInterval type represents time intervals
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/timeInterval.def"
} TimeInterval;

/**
 * @brief The Timestamp type represents a positive time with respect to the epoch
 */
typedef struct  {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/timestamp.def"
} Timestamp;

/**
 * @brief The ClockQuality represents the quality of a clock
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/clockQuality.def"
} ClockQuality;

/**
 * @brief The PTPText data type is used to represent textual material in PTP messages
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/ptpText.def"
} PTPText;

/**
 * @brief The PortIdentity identifies a PTP port.
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
        #include "../../src/def/derivedData/portIdentity.def"
} PortIdentity;

/**
 * @brief The PortAdress type represents the protocol address of a PTP port
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/portAddress.def"
} PortAddress;

/**
 * @brief The PhysicalAddress type is used to represent a physical address
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/physicalAddress.def"
} PhysicalAddress;

/**
 * @brief The common header for all PTP messages (Table 18 of the spec)
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/message/header.def"
} MsgHeader;

/**
 * @brief Management TLV message fields
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/managementTLV.def"
	Octet* dataField;
} ManagementTLV;

/**
 * @brief Management message fields (Table 37 of the spec)
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/message/management.def"
	ManagementTLV* tlv;
}MsgManagement;

/**
 * @brief Management TLV Clock Description fields (Table 41 of the spec)
 */
/* Management TLV Clock Description Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/clockDescription.def"
} MMClockDescription;

/**
 * @brief Management TLV User Description fields (Table 43 of the spec)
 */
/* Management TLV User Description Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/userDescription.def"
} MMUserDescription;

/**
 * @brief Management TLV Initialize fields (Table 44 of the spec)
 */
/* Management TLV Initialize Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/initialize.def"
} MMInitialize;

/**
 * @brief Management TLV Default Data Set fields (Table 50 of the spec)
 */
/* Management TLV Default Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/defaultDataSet.def"
} MMDefaultDataSet;

/**
 * @brief Management TLV Current Data Set fields (Table 55 of the spec)
 */
/* Management TLV Current Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/currentDataSet.def"
} MMCurrentDataSet;

/**
 * @brief Management TLV Parent Data Set fields (Table 56 of the spec)
 */
/* Management TLV Parent Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/parentDataSet.def"
} MMParentDataSet;

/**
 * @brief Management TLV Time Properties Data Set fields (Table 57 of the spec)
 */
/* Management TLV Time Properties Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/timePropertiesDataSet.def"
} MMTimePropertiesDataSet;

/**
 * @brief Management TLV Port Data Set fields (Table 61 of the spec)
 */
/* Management TLV Port Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/portDataSet.def"
} MMPortDataSet;

/**
 * @brief Management TLV Priority1 fields (Table 51 of the spec)
 */
/* Management TLV Priority1 Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/priority1.def"
} MMPriority1;

/**
 * @brief Management TLV Priority2 fields (Table 52 of the spec)
 */
/* Management TLV Priority2 Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/priority2.def"
} MMPriority2;

/**
 * @brief Management TLV Domain fields (Table 53 of the spec)
 */
/* Management TLV Domain Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/domain.def"
} MMDomain;

/**
 * @brief Management TLV Slave Only fields (Table 54 of the spec)
 */
/* Management TLV Slave Only Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/slaveOnly.def"
} MMSlaveOnly;

/**
 * @brief Management TLV Log Announce Interval fields (Table 62 of the spec)
 */
/* Management TLV Log Announce Interval Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/logAnnounceInterval.def"
} MMLogAnnounceInterval;

/**
 * @brief Management TLV Announce Receipt Timeout fields (Table 63 of the spec)
 */
/* Management TLV Announce Receipt Timeout Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/announceReceiptTimeout.def"
} MMAnnounceReceiptTimeout;

/**
 * @brief Management TLV Log Sync Interval fields (Table 64 of the spec)
 */
/* Management TLV Log Sync Interval Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/logSyncInterval.def"
} MMLogSyncInterval;

/**
 * @brief Management TLV Version Number fields (Table 67 of the spec)
 */
/* Management TLV Version Number Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/versionNumber.def"
} MMVersionNumber;

/**
 * @brief Management TLV Time fields (Table 48 of the spec)
 */
/* Management TLV Time Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/time.def"
} MMTime;

/**
 * @brief Management TLV Clock Accuracy fields (Table 49 of the spec)
 */
/* Management TLV Clock Accuracy Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/clockAccuracy.def"
} MMClockAccuracy;

/**
 * @brief Management TLV UTC Properties fields (Table 58 of the spec)
 */
/* Management TLV UTC Properties Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/utcProperties.def"
} MMUtcProperties;

/**
 * @brief Management TLV Traceability Properties fields (Table 59 of the spec)
 */
/* Management TLV Traceability Properties Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/traceabilityProperties.def"
} MMTraceabilityProperties;

/**
 * @brief Management TLV Delay Mechanism fields (Table 65 of the spec)
 */
/* Management TLV Delay Mechanism Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/delayMechanism.def"
} MMDelayMechanism;

/**
 * @brief Management TLV Log Min Pdelay Req Interval fields (Table 66 of the spec)
 */
/* Management TLV Log Min Pdelay Req Interval Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/logMinPdelayReqInterval.def"
} MMLogMinPdelayReqInterval;

/**
 * @brief Management TLV Error Status fields (Table 71 of the spec)
 */
/* Management TLV Error Status Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/errorStatus.def"
} MMErrorStatus;

#endif	/* DATATYPES_H */

