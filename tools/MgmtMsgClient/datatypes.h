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
 * @brief Management TLV Error Status fields (Table 71 of the spec)
 */
/* Management TLV Error Status Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/errorStatus.def"
} MMErrorStatus;

#endif	/* DATATYPES_H */

