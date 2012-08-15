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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <limits.h>

#include "constants.h"

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
Boolean receivedFlag;
Octet *outmessage;
Octet *inmessage;
UInteger32 timeout;
Integer32 generalSock;

enum {
	PTP_ETHER, PTP_DEFAULT
};

typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/portIdentity.def"
} PortIdentity;

typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/message/header.def"
} MsgHeader;

typedef struct {
  UInteger8 lengthField;
  Octet* textField;
} PTPText;

/**
* \brief The ClockQuality represents the quality of a clock
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/clockQuality.def"
} ClockQuality;

/**
* \brief The TimeInterval type represents time intervals
 */
typedef struct {
	/* see src/def/README for a note on this X-macro */
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/timeInterval.def"
} TimeInterval;

/**
* \brief The PortAdress type represents the protocol address of a PTP port
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/portAddress.def"
} PortAddress;

/**
* \brief The PhysicalAddress type is used to represent a physical address
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/derivedData/physicalAddress.def"
} PhysicalAddress;

/**
 * \brief Management TLV message fields
 */
/* Management TLV Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/managementTLV.def"
	Octet* dataField;
} ManagementTLV;

/**
 * \brief Management TLV Clock Description fields (Table 41 of the spec)
 */
/* Management TLV Clock Description Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/clockDescription.def"
} MMClockDescription;

/**
 * \brief Management TLV User Description fields (Table 43 of the spec)
 */
/* Management TLV User Description Message */
typedef struct {
	PTPText userDescription;
} MMUserDescription;

/**
 * \brief Management TLV Parent Data Set fields (Table 56 of the spec)
 */
/* Management TLV Parent Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/defaultDataSet.def"
} MMDefaultDataSet;

/**
 * \brief Management TLV Parent Data Set fields (Table 56 of the spec)
 */
/* Management TLV Parent Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/parentDataSet.def"
} MMParentDataSet;

/**
 * \brief Management TLV Priority1 fields (Table 51 of the spec)
 */
/* Management TLV Priority1 Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/priority1.def"
} MMPriority1;

/**
 * \brief Management TLV Priority2 fields (Table 52 of the spec)
 */
/* Management TLV Priority2 Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/priority2.def"
} MMPriority2;

/**
 * \brief Management TLV Domain fields (Table 53 of the spec)
 */
/* Management TLV Domain Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/domain.def"
} MMDomain;

/**
 * \brief Management TLV Slave Only fields (Table 54 of the spec)
 */
/* Management TLV Slave Only Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/slaveOnly.def"
} MMSlaveOnly;

/**
 * \brief Management TLV Log Announce Interval fields (Table 62 of the spec)
 */
/* Management TLV Log Announce Interval Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/logAnnounceInterval.def"
} MMLogAnnounceInterval;

/**
 * \brief Management TLV Announce Receipt Timeout fields (Table 63 of the spec)
 */
/* Management TLV Announce Receipt Timeout Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/announceReceiptTimeout.def"
} MMAnnounceReceiptTimeout;

/**
 * \brief Management TLV Log Sync Interval fields (Table 64 of the spec)
 */
/* Management TLV Log Sync Interval Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/logSyncInterval.def"
} MMLogSyncInterval;

/**
 * \brief Management TLV Version Number fields (Table 67 of the spec)
 */
/* Management TLV Version Number Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/versionNumber.def"
} MMVersionNumber;

/**
 * \brief Management TLV Clock Accuracy fields (Table 49 of the spec)
 */
/* Management TLV Clock Accuracy Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/clockAccuracy.def"
} MMClockAccuracy;


/**
 * \brief Management TLV Current Data Set fields (Table 55 of the spec)
 */
/* Management TLV Current Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/currentDataSet.def"
} MMCurrentDataSet;

/**
 * \brief Management TLV Time Properties Data Set fields (Table 57 of the spec)
 */
/* Management TLV Time Properties Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/timePropertiesDataSet.def"
} MMTimePropertiesDataSet;

/**
 * \brief Management TLV Port Data Set fields (Table 61 of the spec)
 */
/* Management TLV Port Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/portDataSet.def"
} MMPortDataSet;

/**
 * \brief Management TLV UTC Properties fields (Table 58 of the spec)
 */
/* Management TLV UTC Properties Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/utcProperties.def"
} MMUtcProperties;

/**
 * \brief Management TLV Traceability Properties fields (Table 59 of the spec)
 */
/* Management TLV Traceability Properties Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/traceabilityProperties.def"
} MMTraceabilityProperties;

/**
 * \brief Management TLV Delay Mechanism fields (Table 65 of the spec)
 */
/* Management TLV Delay Mechanism Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/delayMechanism.def"
} MMDelayMechanism;

/**
 * \brief Management TLV Log Min Pdelay Req Interval fields (Table 66 of the spec)
 */
/* Management TLV Log Min Pdelay Req Interval Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/managementTLV/logMinPdelayReqInterval.def"
} MMLogMinPdelayReqInterval;

/**
* \brief Management message fields (Table 37 of the spec)
 */
/*management Message*/
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "../../src/def/message/management.def"
	ManagementTLV* tlv;
}MsgManagement;

#endif /*MANAGER_H_*/
