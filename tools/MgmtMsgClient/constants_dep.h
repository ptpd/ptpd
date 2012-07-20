/** 
 * @file        constants_dep.h
 * @author      Tomasz Kleinschmidt
 *
 * @brief       Constants defined by the specification.
 */

#ifndef CONSTANTS_DEP_H
#define	CONSTANTS_DEP_H

#define CLOCK_IDENTITY_LENGTH 8

#define PTP_EVENT_PORT    "319"
#define PTP_GENERAL_PORT  "320"

/* features, only change to refelect changes in implementation */
#define VERSION_PTP 2

/** 
 * @name Packet length
 Minimal length values for each message.
 If TLV used length could be higher.*/
 /**\{*/
#define HEADER_LENGTH           34
#define MANAGEMENT_LENGTH       48
#define TLV_LENGTH              6
/** \}*/

/**
 * @brief Domain Number (Table 2 in the spec)
 */

enum {
	DFLT_DOMAIN_NUMBER = 0, ALT1_DOMAIN_NUMBER, ALT2_DOMAIN_NUMBER, ALT3_DOMAIN_NUMBER
};

/**
 * @brief PTP Management Message managementId values (Table 40 in the spec)
 */
enum {
	/* Applicable to all node types */
	MM_NULL_MANAGEMENT=0x0000,
	MM_CLOCK_DESCRIPTION=0x0001,
	MM_USER_DESCRIPTION=0x0002,
	MM_SAVE_IN_NON_VOLATILE_STORAGE=0x0003,
	MM_RESET_NON_VOLATILE_STORAGE=0x0004,
	MM_INITIALIZE=0x0005,
	MM_FAULT_LOG=0x0006,
	MM_FAULT_LOG_RESET=0x0007,

	/* Reserved: 0x0008 - 0x1FFF */

	/* Applicable to ordinary and boundary clocks */
	MM_DEFAULT_DATA_SET=0x2000,
	MM_CURRENT_DATA_SET=0x2001,
	MM_PARENT_DATA_SET=0x2002,
	MM_TIME_PROPERTIES_DATA_SET=0x2003,
	MM_PORT_DATA_SET=0x2004,
	MM_PRIORITY1=0x2005,
	MM_PRIORITY2=0x2006,
	MM_DOMAIN=0x2007,
	MM_SLAVE_ONLY=0x2008,
	MM_LOG_ANNOUNCE_INTERVAL=0x2009,
	MM_ANNOUNCE_RECEIPT_TIMEOUT=0x200A,
	MM_LOG_SYNC_INTERVAL=0x200B,
	MM_VERSION_NUMBER=0x200C,
	MM_ENABLE_PORT=0x200D,
	MM_DISABLE_PORT=0x200E,
	MM_TIME=0x200F,
	MM_CLOCK_ACCURACY=0x2010,
	MM_UTC_PROPERTIES=0x2011,
	MM_TRACEABILITY_PROPERTIES=0x2012,
	MM_TIMESCALE_PROPERTIES=0x2013,
	MM_UNICAST_NEGOTIATION_ENABLE=0x2014,
	MM_PATH_TRACE_LIST=0x2015,
	MM_PATH_TRACE_ENABLE=0x2016,
	MM_GRANDMASTER_CLUSTER_TABLE=0x2017,
	MM_UNICAST_MASTER_TABLE=0x2018,
	MM_UNICAST_MASTER_MAX_TABLE_SIZE=0x2019,
	MM_ACCEPTABLE_MASTER_TABLE=0x201A,
	MM_ACCEPTABLE_MASTER_TABLE_ENABLED=0x201B,
	MM_ACCEPTABLE_MASTER_MAX_TABLE_SIZE=0x201C,
	MM_ALTERNATE_MASTER=0x201D,
	MM_ALTERNATE_TIME_OFFSET_ENABLE=0x201E,
	MM_ALTERNATE_TIME_OFFSET_NAME=0x201F,
	MM_ALTERNATE_TIME_OFFSET_MAX_KEY=0x2020,
	MM_ALTERNATE_TIME_OFFSET_PROPERTIES=0x2021,

	/* Reserved: 0x2022 - 0x3FFF */

	/* Applicable to transparent clocks */
	MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET=0x4000,
	MM_TRANSPARENT_CLOCK_PORT_DATA_SET=0x4001,
	MM_PRIMARY_DOMAIN=0x4002,

	/* Reserved: 0x4003 - 0x5FFF */

	/* Applicable to ordinary, boundary, and transparent clocks */
	MM_DELAY_MECHANISM=0x6000,
	MM_LOG_MIN_PDELAY_REQ_INTERVAL=0x6001,

	/* Reserved: 0x6002 - 0xBFFF */
	/* Implementation-specific identifiers: 0xC000 - 0xDFFF */
	/* Assigned by alternate PTP profile: 0xE000 - 0xFFFE */
	/* Reserved: 0xFFFF */
};

/**
 * @brief MANAGEMENT MESSAGE INITIALIZE (Table 44 in the spec)
 */
#define INITIALIZE_EVENT	0x0

/**
 * @brief MANAGEMENT ERROR STATUS managementErrorId (Table 72 in the spec)
 */
enum {
	RESPONSE_TOO_BIG=0x0001,
	NO_SUCH_ID=0x0002,
	WRONG_LENGTH=0x0003,
	WRONG_VALUE=0x0004,
	NOT_SETABLE=0x0005,
	NOT_SUPPORTED=0x0006,
	GENERAL_ERROR=0xFFFE
};

/**
 * @brief PTP tlvType values (Table 34 in the spec) (management only)
 */
enum {
	/* Standard TLVs */
	TLV_MANAGEMENT=0x0001,
	TLV_MANAGEMENT_ERROR_STATUS=0x0002,
};

/**
 * \brief Management Message actions (Table 38 in the spec)
 */
enum {
	GET=0,
	SET,
	RESPONSE,
	COMMAND,
	ACKNOWLEDGE
};

/**
 * \brief MANAGEMENT MESSAGE
 */
#define MANAGEMENT      0x0D

#endif	/* CONSTANTS_DEP_H */

