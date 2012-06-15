/** 
 * @file datatypes.h
 * 
 * @brief Main structures used in ptpdv2
 *
 * This header file defines structures defined by the spec 
 * and needed messages structures
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
 * @brief The PortIdentity identifies a PTP port.
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
        #include "../../src/def/derivedData/portIdentity.def"
} PortIdentity;

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

#endif	/* DATATYPES_H */

