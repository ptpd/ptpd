#ifndef PTP_DATATYPES_H_
#define PTP_DATATYPES_H_

#include "ptp_primitives.h"

/*Struct defined in spec*/


/**
*\file
* \brief Main structures used in ptpdv2
*
* This header file defines structures defined by the spec,
* main program data structure, and all messages structures
 */

/**
* \brief Time structure to handle timestamps
 */
typedef struct {
	Integer32 seconds;
	Integer32 nanoseconds;
} TimeInternal;

/**
* \brief The TimeInterval type represents time intervals
 */
typedef struct {
	/* see src/def/README for a note on this X-macro */
	#define OPERATE( name, size, type ) type name;
	#include "def/derivedData/timeInterval.def"
} TimeInterval;

/**
* \brief The Timestamp type represents a positive time with respect to the epoch
 */
typedef struct  {
	#define OPERATE( name, size, type ) type name;
	#include "def/derivedData/timestamp.def"
} Timestamp;

/**
* \brief The ClockIdentity type identifies a clock
 */
typedef Octet ClockIdentity[CLOCK_IDENTITY_LENGTH];

/**
* \brief The PortIdentity identifies a PTP port.
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/derivedData/portIdentity.def"
} PortIdentity;

/**
* \brief The PortAdress type represents the protocol address of a PTP port
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/derivedData/portAddress.def"
} PortAddress;

/**
* \brief The ClockQuality represents the quality of a clock
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/derivedData/clockQuality.def"
} ClockQuality;

/**
* \brief The TimePropertiesDS type represent time source and traceability properties of a clock
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/derivedData/timePropertiesDS.def"
} TimePropertiesDS;

/**
* \brief The TLV type represents TLV extension fields
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/derivedData/tlv.def"
} TLV;

/**
* \brief The PTPText data type is used to represent textual material in PTP messages
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/derivedData/ptpText.def"
} PTPText;

/**
* \brief The FaultRecord type is used to construct fault logs
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/derivedData/faultRecord.def"
} FaultRecord;

/**
* \brief The PhysicalAddress type is used to represent a physical address
 */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/derivedData/physicalAddress.def"
} PhysicalAddress;


/**
* \brief The common header for all PTP messages (Table 18 of the spec)
 */
/* Message header */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/message/header.def"
} MsgHeader;

/**
* \brief Announce message fields (Table 25 of the spec)
 */
/*Announce Message */
typedef struct {
	Timestamp originTimestamp;
	Integer16 currentUtcOffset;
	UInteger8 grandmasterPriority1;
	ClockQuality grandmasterClockQuality;
	UInteger8 grandmasterPriority2;
	ClockIdentity grandmasterIdentity;
	UInteger16 stepsRemoved;
	Enumeration8 timeSource;
}MsgAnnounce;


/**
* \brief Sync message fields (Table 26 of the spec)
 */
/*Sync Message */
typedef struct {
	Timestamp originTimestamp;
}MsgSync;

/**
* \brief DelayReq message fields (Table 26 of the spec)
 */
/*DelayReq Message */
typedef struct {
	Timestamp originTimestamp;
}MsgDelayReq;

/**
* \brief DelayResp message fields (Table 30 of the spec)
 */
/*delayResp Message*/
typedef struct {
	Timestamp receiveTimestamp;
	PortIdentity requestingPortIdentity;
}MsgDelayResp;

/**
* \brief FollowUp message fields (Table 27 of the spec)
 */
/*Follow-up Message*/
typedef struct {
	Timestamp preciseOriginTimestamp;
}MsgFollowUp;

/**
* \brief PdelayReq message fields (Table 29 of the spec)
 */
/*PdelayReq Message*/
typedef struct {
	Timestamp originTimestamp;
}MsgPdelayReq;

/**
* \brief PdelayResp message fields (Table 30 of the spec)
 */
/*PdelayResp Message*/
typedef struct {
	Timestamp requestReceiptTimestamp;
	PortIdentity requestingPortIdentity;
}MsgPdelayResp;

/**
* \brief PdelayRespFollowUp message fields (Table 31 of the spec)
 */
/*PdelayRespFollowUp Message*/
typedef struct {
	Timestamp responseOriginTimestamp;
	PortIdentity requestingPortIdentity;
} MsgPdelayRespFollowUp;


/**
 * \brief Management TLV message fields
 */
/* Management TLV Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/managementTLV.def"
	Octet* dataField;
} ManagementTLV;

/**
 * \brief Management TLV Clock Description fields (Table 41 of the spec)
 */
/* Management TLV Clock Description Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/clockDescription.def"
} MMClockDescription;

/**
 * \brief Management TLV User Description fields (Table 43 of the spec)
 */
/* Management TLV User Description Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/userDescription.def"
} MMUserDescription;

/**
 * \brief Management TLV Initialize fields (Table 44 of the spec)
 */
/* Management TLV Initialize Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/initialize.def"
} MMInitialize;

/**
 * \brief Management TLV Default Data Set fields (Table 50 of the spec)
 */
/* Management TLV Default Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/defaultDataSet.def"
} MMDefaultDataSet;

/**
 * \brief Management TLV Current Data Set fields (Table 55 of the spec)
 */
/* Management TLV Current Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/currentDataSet.def"
} MMCurrentDataSet;

/**
 * \brief Management TLV Parent Data Set fields (Table 56 of the spec)
 */
/* Management TLV Parent Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/parentDataSet.def"
} MMParentDataSet;

/**
 * \brief Management TLV Time Properties Data Set fields (Table 57 of the spec)
 */
/* Management TLV Time Properties Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/timePropertiesDataSet.def"
} MMTimePropertiesDataSet;

/**
 * \brief Management TLV Port Data Set fields (Table 61 of the spec)
 */
/* Management TLV Port Data Set Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/portDataSet.def"
} MMPortDataSet;

/**
 * \brief Management TLV Priority1 fields (Table 51 of the spec)
 */
/* Management TLV Priority1 Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/priority1.def"
} MMPriority1;

/**
 * \brief Management TLV Priority2 fields (Table 52 of the spec)
 */
/* Management TLV Priority2 Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/priority2.def"
} MMPriority2;

/**
 * \brief Management TLV Domain fields (Table 53 of the spec)
 */
/* Management TLV Domain Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/domain.def"
} MMDomain;

/**
 * \brief Management TLV Slave Only fields (Table 54 of the spec)
 */
/* Management TLV Slave Only Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/slaveOnly.def"
} MMSlaveOnly;

/**
 * \brief Management TLV Log Announce Interval fields (Table 62 of the spec)
 */
/* Management TLV Log Announce Interval Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/logAnnounceInterval.def"
} MMLogAnnounceInterval;

/**
 * \brief Management TLV Announce Receipt Timeout fields (Table 63 of the spec)
 */
/* Management TLV Announce Receipt Timeout Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/announceReceiptTimeout.def"
} MMAnnounceReceiptTimeout;

/**
 * \brief Management TLV Log Sync Interval fields (Table 64 of the spec)
 */
/* Management TLV Log Sync Interval Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/logSyncInterval.def"
} MMLogSyncInterval;

/**
 * \brief Management TLV Version Number fields (Table 67 of the spec)
 */
/* Management TLV Version Number Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/versionNumber.def"
} MMVersionNumber;

/**
 * \brief Management TLV Time fields (Table 48 of the spec)
 */
/* Management TLV Time Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/time.def"
} MMTime;

/**
 * \brief Management TLV Clock Accuracy fields (Table 49 of the spec)
 */
/* Management TLV Clock Accuracy Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/clockAccuracy.def"
} MMClockAccuracy;

/**
 * \brief Management TLV UTC Properties fields (Table 58 of the spec)
 */
/* Management TLV UTC Properties Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/utcProperties.def"
} MMUtcProperties;

/**
 * \brief Management TLV Traceability Properties fields (Table 59 of the spec)
 */
/* Management TLV Traceability Properties Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/traceabilityProperties.def"
} MMTraceabilityProperties;

/**
 * \brief Management TLV Timescale Properties fields (Table 59 of the spec)
 */
/* Management TLV Timescale Properties Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/timescaleProperties.def"
} MMTimescaleProperties;

/**
 * \brief Management TLV Unicast Negotiation Enable fields (Table 73 of the spec)
 */
/* Management TLV Traceability Properties Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/unicastNegotiationEnable.def"
} MMUnicastNegotiationEnable;

/**
 * \brief Management TLV Delay Mechanism fields (Table 65 of the spec)
 */
/* Management TLV Delay Mechanism Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/delayMechanism.def"
} MMDelayMechanism;

/**
 * \brief Management TLV Log Min Pdelay Req Interval fields (Table 66 of the spec)
 */
/* Management TLV Log Min Pdelay Req Interval Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/logMinPdelayReqInterval.def"
} MMLogMinPdelayReqInterval;

/**
 * \brief Management TLV Error Status fields (Table 71 of the spec)
 */
/* Management TLV Error Status Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/managementTLV/errorStatus.def"
} MMErrorStatus;

/**
* \brief Management message fields (Table 37 of the spec)
 */
/*management Message*/
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/message/management.def"
	ManagementTLV* tlv;
} MsgManagement;

/**
 * \brief Signaling TLV message fields (tab
 */
/* Signaling TLV Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/signalingTLV/signalingTLV.def"
	Octet* valueField;
} SignalingTLV;

/**
 * \brief Signaling TLV Request Unicast Transmission fields (Table 73 of the spec)
 */
/* Signaling TLV Request Unicast Transmission Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/signalingTLV/requestUnicastTransmission.def"
} SMRequestUnicastTransmission;

/**
 * \brief Signaling TLV Grant Unicast Transmission fields (Table 74 of the spec)
 */
/* Signaling TLV Grant Unicast Transmission Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/signalingTLV/grantUnicastTransmission.def"
} SMGrantUnicastTransmission;

/**
 * \brief Signaling TLV Cancel Unicast Transmission fields (Table 75 of the spec)
 */
/* Signaling TLV Cancel Unicast Transmission Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/signalingTLV/cancelUnicastTransmission.def"
} SMCancelUnicastTransmission;

/**
 * \brief Signaling TLV Acknowledge Cancel Unicast Transmission fields (Table 76 of the spec)
 */
/* Signaling TLV Acknowledge Cancel Unicast Transmission Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/signalingTLV/acknowledgeCancelUnicastTransmission.def"
} SMAcknowledgeCancelUnicastTransmission;

/**
* \brief Signaling message fields (Table 33 of the spec)
 */
/*Signaling Message*/
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "def/message/signaling.def"
	SignalingTLV* tlv;
} MsgSignaling;


/**
* \brief ForeignMasterRecord is used to manage foreign masters
 */
typedef struct
{
	PortIdentity foreignMasterPortIdentity;
	UInteger16 foreignMasterAnnounceMessages;

	/* Supplementary data */
	MsgAnnounce  announce;	/* announce message -> all datasets */
	MsgHeader    header;	/* header -> some datasets */
	UInteger8    localPreference; /* local preference - only used by telecom profile */
	UInteger32   sourceAddr; /* source address */
	Boolean	     disqualified; /* if true, this one always loses */
} ForeignMasterRecord;

typedef struct {

	/*Static members*/
	Boolean twoStepFlag;
	ClockIdentity clockIdentity;
	UInteger16 numberPorts;

	/*Dynamic members*/
	ClockQuality clockQuality;

	/*Configurable members*/
	UInteger8 priority1;
	UInteger8 priority2;
	UInteger8 domainNumber;
	Boolean slaveOnly;

} DefaultDS;

typedef struct {

	UInteger16 stepsRemoved;
	TimeInternal offsetFromMaster;
	TimeInternal meanPathDelay;

	/* PTPd additions */
	UInteger32 offsetFromMasterThreshold; /* maximum offset from master */

} CurrentDS;

typedef struct {
	/*Dynamic members*/
	PortIdentity parentPortIdentity;
	Boolean parentStats;
	UInteger16 observedParentOffsetScaledLogVariance;
	Integer32 observedParentClockPhaseChangeRate;
	ClockIdentity grandmasterIdentity;
	ClockQuality grandmasterClockQuality;
	UInteger8 grandmasterPriority1;
	UInteger8 grandmasterPriority2;
} ParentDS;

typedef struct {

	/*Static members*/
	PortIdentity portIdentity;

	/*Dynamic members*/
	Enumeration8 portState;
	Integer8 logMinDelayReqInterval;
	TimeInternal peerMeanPathDelay;

	/*Configurable members*/
	Integer8 logAnnounceInterval;
	UInteger8 announceReceiptTimeout;
	Integer8 logSyncInterval;
	Enumeration8 delayMechanism;
	Integer8 logMinPdelayReqInterval;
	UInteger4 versionNumber;

	/* PTPd additions */
	Enumeration8 lastPortState; 	/* previous state */
	Nibble transportSpecific; 	/* TransportSpecific for 802.1AS */
	Integer16 lastMismatchedDomain; 	/* domain mismatch detection */

} PortDS;

/* structure to hold basic PTP datasets when capturing an event */
typedef struct {
    DefaultDS 		defaultDS;
    CurrentDS 		currentDS;
    TimePropertiesDS 	timePropertiesDS;
    PortDS 		portDS;
    ParentDS 		parentDS;
    ForeignMasterRecord bestMaster;
    Integer32		ofmAlarmThreshold;
} PtpEventData;

#endif /*PTP_DATATYPES_H_*/
