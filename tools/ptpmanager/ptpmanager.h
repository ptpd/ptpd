#ifndef MANAGER_H
#define MANAGER_H

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <net/if.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <limits.h>

#define MANAGEMENT_LENGTH 48
#define CLOCK_IDENTITY_LENGTH 8
#define HEADER_LENGTH 34
#define PACKET_SIZE  300
#define MANAGEMENT 13

#define flip16(x) htons(x)
#define flip32(x) htonl(x)

typedef enum {FALSE=0, TRUE} Boolean;
typedef char Octet;
typedef signed char Integer8;
typedef signed short Integer16;
typedef signed int Integer32;
typedef unsigned char UInteger8;
typedef unsigned short UInteger16;
typedef unsigned int UInteger32;
typedef unsigned short Enumeration16;
typedef unsigned char Enumeration8;
typedef unsigned char Enumeration4;
typedef unsigned char Enumeration4Upper;
typedef unsigned char Enumeration4Lower;
typedef unsigned char UInteger4;
typedef unsigned char UInteger4Upper;
typedef unsigned char UInteger4Lower;
typedef unsigned char Nibble;
typedef unsigned char NibbleUpper;
typedef unsigned char NibbleLower;
typedef Octet ClockIdentity[CLOCK_IDENTITY_LENGTH];

typedef struct {
	unsigned int lsb;     /* FIXME: shouldn't uint32_t and int32_t be used here? */
	int msb;
} Integer64;

int out_length;
UInteger16 out_sequence;
UInteger16 in_sequence;


/**
 * \brief PTP Management Message managementId values (Table 40 in the spec)
 */
/* SLAVE_ONLY conflicts with another constant, so scope with MM_ */
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

/*
 * \brief PTP tlvType values for management messages (Table 34 in the spec)
 */
enum {
	/* Standard TLVs */
	TLV_MANAGEMENT=0x0001,
	TLV_MANAGEMENT_ERROR_STATUS=0x0002
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

Boolean receivedFlag;
/**
* \brief Struct used to store network datas
 */
typedef struct {
  Integer32 eventSock, generalSock;

} NetPath;

enum {
	PTP_ETHER, PTP_DEFAULT
};

typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "portIdentity.def"
} PortIdentity;

typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "header.def"
} MsgHeader;


/**
 * \brief Management TLV message fields
 */
/* Management TLV Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "managementTLV.def"
	Octet* dataField;
} ManagementTLV;

/**
* \brief Management message fields (Table 37 of the spec)
 */
/*management Message*/
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "management.def"
	ManagementTLV* tlv;
}MsgManagement;

#endif /*MANAGER_H_*/
