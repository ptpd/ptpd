#ifndef DATATYPES_H_
#define DATATYPES_H_

#include <stdio.h>
#include <dep/iniparser/dictionary.h>
#ifdef PTPD_STATISTICS
#include <dep/statistics.h>
#endif /* PTPD_STATISTICS */

/*Struct defined in spec*/


/**
*\file
* \brief Main structures used in ptpdv2
*
* This header file defines structures defined by the spec,
* main program data structure, and all messages structures
 */


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
* \brief Time structure to handle timestamps
 */
typedef struct {
	Integer32 seconds;
	Integer32 nanoseconds;
} TimeInternal;


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
	
} ForeignMasterRecord;

/**
 * \struct PtpdCounters
 * \brief Ptpd engine counters per port
 */
typedef struct
{

	/*
	 * message sent/received counters:
	 * - sent only incremented on success,
	 * - received only incremented when message valid and accepted,
	 * - looped messages to self don't increment received,
	 */
	uint32_t announceMessagesSent;
	uint32_t announceMessagesReceived;
	uint32_t syncMessagesSent;
	uint32_t syncMessagesReceived;
	uint32_t followUpMessagesSent;
	uint32_t followUpMessagesReceived;
	uint32_t delayReqMessagesSent;
	uint32_t delayReqMessagesReceived;
	uint32_t delayRespMessagesSent;
	uint32_t delayRespMessagesReceived;
	uint32_t pdelayReqMessagesSent;
	uint32_t pdelayReqMessagesReceived;
	uint32_t pdelayRespMessagesSent;
	uint32_t pdelayRespMessagesReceived;
	uint32_t pdelayRespFollowUpMessagesSent;
	uint32_t pdelayRespFollowUpMessagesReceived;
	uint32_t signalingMessagesSent;
	uint32_t signalingMessagesReceived;
	uint32_t managementMessagesSent;
	uint32_t managementMessagesReceived;

/* not implemented yet */
#if 0
	/* FMR counters */
	uint32_t foreignAdded; // implement me! /* number of insertions to FMR */
	uint32_t foreignMax; // implement me! /* maximum foreign masters seen */
	uint32_t foreignRemoved; // implement me! /* number of FMR records deleted */
	uint32_t foreignOverflow; // implement me! /* how many times the FMR was full */
#endif /* 0 */

	/* protocol engine counters */

	uint32_t stateTransitions;	  /* number of state changes */
	uint32_t masterChanges;		  /* number of BM changes as result of BMC */
	uint32_t announceTimeouts;	  /* number of announce receipt timeouts */

	/* discarded / uknown / ignored */
	uint32_t discardedMessages;	  /* only messages we shouldn't be receiving - ignored from self don't count */
	uint32_t unknownMessages;	  /* unknown type - also increments discarded */
	uint32_t ignoredAnnounce;	  /* ignored Announce messages: acl / security / preference */
	uint32_t aclTimingDiscardedMessages;	  /* Timing messages discarded by access lists */
	uint32_t aclManagementDiscardedMessages;	  /* Timing messages discarded by access lists */

	/* error counters */
	uint32_t messageRecvErrors;	  /* message receive errors */
	uint32_t messageSendErrors;	  /* message send errors */
	uint32_t messageFormatErrors;	  /* headers or messages too short etc. */
	uint32_t protocolErrors;	  /* conditions that shouldn't happen */
	uint32_t versionMismatchErrors;	  /* V1 received, V2 expected - also increments discarded */
	uint32_t domainMismatchErrors;	  /* different domain than configured - also increments discarded */
	uint32_t sequenceMismatchErrors;  /* mismatched sequence IDs - also increments discarded */
	uint32_t delayModeMismatchErrors; /* P2P received, E2E expected or vice versa - incremets discarded */
	uint32_t consecutiveSequenceErrors;    /* number of consecutive sequence mismatch errors */

	/* unicast sgnaling counters */
	uint32_t unicastGrantsRequested;  /* slave: how many we requested, master: how many requests we received */
	uint32_t unicastGrantsGranted;	  /* slave: how many we got granted, master: how many we granted */
	uint32_t unicastGrantsDenied;	 /* slave: how many we got denied, master: how many we denied */
	uint32_t unicastGrantsCancelSent;   /* how many we canceled */
	uint32_t unicastGrantsCancelReceived; /* how many cancels we received */
	uint32_t unicastGrantsCancelAckReceived; /* how many cancel ack we received */

#ifdef PTPD_STATISTICS
	uint32_t delayMSOutliersFound;	  /* Number of outliers found by the delayMS filter */
	uint32_t delaySMOutliersFound;	  /* Number of outliers found by the delaySM filter */
#endif /* PTPD_STATISTICS */
	uint32_t maxDelayDrops; /* number of samples dropped due to maxDelay threshold */

	uint32_t sentMessageRate;	/* RX message rate per sec */
	uint32_t receivedMessageRate;	/* TX message rate per sec */

} PtpdCounters;

/**
 * \struct PIservo
 * \brief PI controller model structure
 */

typedef struct{
    int maxOutput;
    Integer32 input;
    double output;
    double observedDrift;
    double kP, kI;
    TimeInternal lastUpdate;
    Boolean runningMaxOutput;
    int dTmethod;
    double dT;
    int maxdT;
#ifdef PTPD_STATISTICS
    int updateCount;
    int stableCount;
    Boolean statsCalculated;
    Boolean isStable;
    double stabilityThreshold;
    int stabilityPeriod;
    int stabilityTimeout;
    double driftMean;
    double driftStdDev;
    DoublePermanentStdDev driftStats;
#endif /* PTPD_STATISTICS */
} PIservo;

typedef struct {
	Boolean activity; 		/* periodic check, updateClock sets this to let the watchdog know we're holding clock control */
	Boolean	available; 	/* flags that we can control the clock */
	Boolean granted; 	/* upstream watchdog will grant this when we're the best provider */
	Boolean updateOK;		/* if not, updateClock() will not run */
	Boolean stepRequired;		/* if set, we need to step the clock */
	Boolean offsetOK;		/* if set, updateOffset accepted oFm */
} ClockControlInfo;

typedef struct {
	Boolean inSync;
	Boolean leapInsert;
	Boolean leapDelete;
	Boolean majorChange;
	Boolean update;
	Boolean override; /* this tells the client that this info takes priority
			   * over whatever the client itself would like to set
			   * prime example: leap second file
			   */
	int utcOffset;
	long clockOffset;
} ClockStatusInfo;

typedef struct UnicastGrantTable UnicastGrantTable;

typedef struct {
	UInteger32      duration;		/* grant duration */
	Boolean		requestable;		/* is this mesage type even requestable? */
	Boolean		requested;		/* slave: we have requested this */
	Boolean		canceled;		/* this has been canceled (awaiting ack) */
	Boolean		cancelCount;		/* how many times we sent the cancel message while waiting for ack */
	Integer8	logInterval;		/* interval we granted or got granted */
	Integer8	logMinInterval;		/* minimum interval we're going to request */
	Integer8	logMaxInterval;		/* maximum interval we're going to request */
	UInteger16	sentSeqId;		/* used by masters: last sent sequence id */
	UInteger32	intervalCounter;	/* used as a modulo counter to allow different message rates for different slaves */
	Boolean		expired;		/* TRUE -> grant has expired */
	Boolean         granted;		/* master: we have granted this, slave: we have been granted this */
	UInteger32      timeLeft;		/* countdown timer for aging out grants */
	UInteger16      messageType;		/* message type this grant is for */
	UnicastGrantTable *parent;		/* parent entry (that has transportAddress and portIdentity */
	Boolean		receiving;		/* keepalive: used to detect if message of this type is being received */
} UnicastGrantData;

struct UnicastGrantTable {
	Integer32		transportAddress;	/* IP address of slave (or master) */
	UInteger8		domainNumber;		/* domain of the master - as used by Telecom Profile */
	UInteger8		localPreference;		/* local preference - as used by Telecom profile */
	PortIdentity    	portIdentity;		/* master: port ID of grantee, slave: portID of grantor */
	UnicastGrantData	grantData[PTP_MAX_MESSAGE];/* master: grantee's grants, slave: grantor's grant status */
	UInteger32		timeLeft;		/* time until expiry of last grant (max[grants.timeLeft]. when runs out and no renewal, entry can be re-used */
	Boolean			isPeer;			/* this entry is peer only */
	TimeInternal		lastSyncTimestamp;		/* last Sync message timestamp sent */
};

/* Unicast destination configuration: Address, domain, preference, last Sync timestamp sent */
typedef struct {
    Integer32 		transportAddress;		/* destination address */
    UInteger8 		domainNumber;			/* domain number - for slaves with masters in multiple domains */
    UInteger8 		localPreference;		/* local preference to influence BMC */
    TimeInternal 	lastSyncTimestamp;			/* last Sync timestamp sent */
} UnicastDestination;

typedef struct {
    Integer32 transportAddress;
} SyncDestEntry;

/**
 * \struct PtpClock
 * \brief Main program data structure
 */
/* main program data structure */
typedef struct {
	/* Default data set */

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


	/* Current data set */

	/*Dynamic members*/
	UInteger16 stepsRemoved;
	TimeInternal offsetFromMaster;
	TimeInternal meanPathDelay;

	/* Parent data set */

	/*Dynamic members*/
	PortIdentity parentPortIdentity;
	Boolean parentStats;
	UInteger16 observedParentOffsetScaledLogVariance;
	Integer32 observedParentClockPhaseChangeRate;
	ClockIdentity grandmasterIdentity;
	ClockQuality grandmasterClockQuality;
	UInteger8 grandmasterPriority1;
	UInteger8 grandmasterPriority2;

	/* Global time properties data set */
	TimePropertiesDS timePropertiesDS;

	/* Leap second related flags */
        Boolean leapSecondInProgress;
        Boolean leapSecondPending;

	/* Port configuration data set */

	/*Static members*/
	PortIdentity portIdentity;

	/*Dynamic members*/
	Enumeration8 portState;
	Integer8 logMinDelayReqInterval;
	Integer8 logMinPdelayReqInterval;
	TimeInternal peerMeanPathDelay;
 
	/*Configurable members*/
	Integer8 logAnnounceInterval;
	UInteger8 announceReceiptTimeout;
	Integer8 logSyncInterval;
	Enumeration8 delayMechanism;
	UInteger4 versionNumber;

	/* TransportSpecific for 802.1AS */
	Nibble transportSpecific;

	/* Foreign master data set */
	ForeignMasterRecord *foreign;

	/* Other things we need for the protocol */
	UInteger16 number_foreign_records;
	Integer16  max_foreign_records;
	Integer16  foreign_record_i;
	Integer16  foreign_record_best;
	UInteger32 random_seed;
	Boolean  record_update;    /* should we run bmc() after receiving an announce message? */


	/* unicast grant table - our own grants or our slaves' grants or grants to peers */
	UnicastGrantTable unicastGrants[UNICAST_MAX_DESTINATIONS];
	/* our trivial index table to speed up lookups */
	UnicastGrantTable* unicastGrantIndex[UNICAST_MAX_DESTINATIONS];
	/* current parent from the above table */
	UnicastGrantTable *parentGrants;
	/* previous parent's grants when changing parents: if not null, this is what should be canceled */
	UnicastGrantTable *previousGrants;
	/* another index to match unicast Sync with FollowUp when we can't capture the destination address of Sync */
	SyncDestEntry syncDestIndex[UNICAST_MAX_DESTINATIONS];

	/* unicast destinations parsed from config */
	UnicastDestination unicastDestinations[UNICAST_MAX_DESTINATIONS];
	int unicastDestinationCount;

	/* number of slaves we have granted Announce to */
	int slaveCount;

	/* unicast destination for pdelay */
	UnicastDestination  unicastPeerDestination;

	/* support for unicast negotiation in P2P mode */
	UnicastGrantTable peerGrants;

	MsgHeader msgTmpHeader;

	union {
		MsgSync  sync;
		MsgFollowUp  follow;
		MsgDelayReq  req;
		MsgDelayResp resp;
		MsgPdelayReq  preq;
		MsgPdelayResp  presp;
		MsgPdelayRespFollowUp  prespfollow;
		MsgManagement  manage;
		MsgAnnounce  announce;
		MsgSignaling signaling;
	} msgTmp;

	MsgManagement outgoingManageTmp;
	MsgSignaling outgoingSignalingTmp;

	Octet msgObuf[PACKET_SIZE];
	Octet msgIbuf[PACKET_SIZE];

	int followUpGap;

/*
	20110630: These variables were deprecated in favor of the ones that appear in the stats log (delayMS and delaySM)
	
	TimeInternal  master_to_slave_delay;
	TimeInternal  slave_to_master_delay;

	*/

	TimeInternal  pdelay_req_receive_time;
	TimeInternal  pdelay_req_send_time;
	TimeInternal  pdelay_resp_receive_time;
	TimeInternal  pdelay_resp_send_time;
	TimeInternal  sync_receive_time;
	TimeInternal  delay_req_send_time;
	TimeInternal  delay_req_receive_time;
	MsgHeader		PdelayReqHeader;
	MsgHeader		delayReqHeader;
	TimeInternal	pdelayMS;
	TimeInternal	pdelaySM;
	TimeInternal	delayMS;
	TimeInternal	delaySM;
	TimeInternal  lastSyncCorrectionField;
	TimeInternal  lastPdelayRespCorrectionField;

	Boolean  sentPdelayReq;
	UInteger16  sentPdelayReqSequenceId;
	UInteger16  sentDelayReqSequenceId;
	UInteger16  sentSyncSequenceId;
	UInteger16  sentAnnounceSequenceId;
	UInteger16  sentSignalingSequenceId;
	UInteger16  recvPdelayReqSequenceId;
	UInteger16  recvSyncSequenceId;
	UInteger16  recvPdelayRespSequenceId;
	Boolean  waitingForFollow;
	Boolean  waitingForDelayResp;
	
	offset_from_master_filter  ofm_filt;
	one_way_delay_filter  owd_filt;

	Boolean message_activity;

	IntervalTimer  timers[PTP_MAX_TIMER];

	NetPath netPath;

	/*Usefull to init network stuff*/
	UInteger8 port_communication_technology;

	/*Stats header will be re-printed when set to true*/
	Boolean resetStatisticsLog;

	int listenCount; // number of consecutive resets to listening
	int resetCount;
	int announceTimeouts; 
	int current_init_clock;
	int can_step_clock;
	int warned_operator_slow_slewing;
	int warned_operator_fast_slewing;
	Boolean warnedUnicastCapacity;
	int maxDelayRejected;

	Boolean runningBackupInterface;


	char char_last_msg;                             /* representation of last message processed by servo */

	int syncWaiting;                     /* we'll only start the delayReq timer after the first sync */
	int delayRespWaiting;                /* Just for information purposes */
	Boolean startup_in_progress;

	Boolean pastStartup;				/* we've set the clock already, at least once */

	Boolean	offsetFirstUpdated;

	uint32_t init_timestamp;                        /* When the clock was last initialised */
	Integer32 stabilisation_time;                   /* How long (seconds) it took to stabilise the clock */
	double last_saved_drift;                     /* Last observed drift value written to file */
	Boolean drift_saved;                            /* Did we save a drift value already? */

	/* user description is max size + 1 to leave space for a null terminator */
	Octet user_description[USER_DESCRIPTION_MAX + 1];

	Integer32 	masterAddr;                           // used for hybrid mode, when receiving announces
	Integer32 	LastSlaveAddr;                        // used for hybrid mode, when receiving delayreqs
	Integer32	lastSyncDst;		/* captures the destination address for last sync, so we know where to send the followUp */
	Integer32	lastPdelayRespDst;	/* captures the destination address of last pdelayResp so we know where to send the pdelayRespfollowUp */

	/*
	 * counters - useful for debugging and monitoring,
	 * should be exposed through management messages
	 * and SNMP eventually
	 */
	PtpdCounters counters;

	/* PI servo model */
	PIservo servo;

	/* "panic mode" support */
	Boolean panicMode; /* in panic mode - do not update clock or calculate offsets */
	Boolean panicOver; /* panic mode is over, we can reset the clock */
	int panicModeTimeLeft; /* How many 30-second periods left in panic mode */

	/* used to wait on failure while allowing timers to tick */
	Boolean initFailure;
	Integer32 initFailureTimeout;

	/* 
	 * used to inform TimingService about our status, but so 
	 * that PTP code is independent from LibCCK and glue code can poll this
	 */
	ClockControlInfo clockControl;
	ClockStatusInfo clockStatus;


	TimeInternal	rawDelayMS;
	TimeInternal	rawDelaySM;
	TimeInternal	rawPdelayMS;
	TimeInternal	rawPdelaySM;

#ifdef	PTPD_STATISTICS
	/*
	 * basic clock statistics information, useful
	 * for monitoring servo performance and estimating
	 * clock stability - should be exposed through
	 * management messages and SNMP eventually
	*/
	PtpEngineSlaveStats slaveStats;

	OutlierFilter 	oFilterMS;
	OutlierFilter	oFilterSM;

	DoubleMovingStatFilter *filterMS;
	DoubleMovingStatFilter *filterSM;

#endif

	Integer32 acceptedUpdates;
	Integer32 offsetUpdates;

	Boolean isCalibrated;

	NTPcontrol ntpControl;

	/* the interface to TimingDomain */
	TimingService timingService;

	/* accumulating offset correction added when smearing leap second */
	double leapSmearFudge;

	/* testing only - used to add a 1ms offset */
#if 0
	Boolean addOffset;
#endif

} PtpClock;

/**
 * \struct RunTimeOpts
 * \brief Program options set at run-time
 */
/* program options set at run-time */
typedef struct {
	Integer8 logAnnounceInterval;
	Integer8 announceReceiptTimeout;
	Integer8 logSyncInterval;
	Integer8 logMinDelayReqInterval;
	Integer8 logMinPdelayReqInterval;

	Boolean slaveOnly;

	ClockQuality clockQuality;
	TimePropertiesDS timeProperties;

	UInteger8 priority1;
	UInteger8 priority2;
	UInteger8 domainNumber;
	UInteger16 portNumber;


	/* Max intervals for unicast negotiation */
	Integer8 logMaxPdelayReqInterval;
	Integer8 logMaxDelayReqInterval;
	Integer8 logMaxSyncInterval;
	Integer8 logMaxAnnounceInterval;

	/* optional BMC extension: accept any domain, prefer configured domain, prefer lower domain */
	Boolean anyDomain;
//	UInteger8 timeSource;

	/*
	 * For slave state, grace period of n * announceReceiptTimeout
	 * before going into LISTENING again - we disqualify the best GM
	 * and keep waiting for BMC to act - if a new GM picks up,
	 * we don't lose the calculated offsets etc. Allows seamless GM
	 * failover with little time variation
	 */

	int announceTimeoutGracePeriod;
//	Integer16 currentUtcOffset;

	Octet* ifaceName;
	Octet primaryIfaceName[IFACE_NAME_LENGTH];
	Octet backupIfaceName[IFACE_NAME_LENGTH];
	Boolean backupIfaceEnabled;

	Boolean	noResetClock; // don't step the clock if offset > 1s
	Boolean stepForce; // force clock step on first sync after startup
	Boolean stepOnce; // only step clock on first sync after startup
#ifdef linux
	Boolean setRtc;
#endif /* linux */


	Boolean clearCounters;

	Integer8 masterRefreshInterval;

	Integer32 maxOffset; /* Maximum number of nanoseconds of offset */
	Integer32 maxDelay; /* Maximum number of nanoseconds of delay */

	Boolean	noAdjust;

	Boolean displayPackets;
	Integer16 s;
	TimeInternal inboundLatency, outboundLatency, ofmShift;
	Integer16 max_foreign_records;
	Enumeration8 delayMechanism;

	int ttl;
	int dscpValue;
#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H)
	int cpuNumber;
#endif /* linux && HAVE_SCHED_H || HAVE_SYS_CPUSET_H*/

	Boolean alwaysRespectUtcOffset;
	Boolean preferUtcValid;
	Boolean requireUtcValid;
	Boolean useSysLog;
	Boolean checkConfigOnly;
	Boolean printLockFile;

	char configFile[PATH_MAX];

	LogFileHandler statisticsLog;
	LogFileHandler recordLog;
	LogFileHandler eventLog;
	LogFileHandler statusLog;

	int leapSecondPausePeriod;
	int leapSecondHandling;
	Integer32 leapSecondSmearPeriod;
	int leapSecondNoticePeriod;

	Boolean periodicUpdates;
	Boolean logStatistics;
	int statisticsTimestamp;

	int logLevel;
	int statisticsLogInterval;

	int statusFileUpdateInterval;

	Boolean ignore_daemon_lock;
	Boolean do_IGMP_refresh;
	Boolean  nonDaemon;

	int initial_delayreq;

	Boolean ignore_delayreq_interval_master;
	Boolean autoDelayReqInterval;

	Boolean autoLockFile; /* mode and interface specific lock files are used
				    * when set to TRUE */
	char lockDirectory[PATH_MAX]; /* Directory to store lock files 
				       * When automatic lock files used */
	char lockFile[PATH_MAX]; /* lock file location */
	char driftFile[PATH_MAX]; /* drift file location */
	char leapFile[PATH_MAX]; /* leap seconds file location */
	int drift_recovery_method; /* how the observed drift is managed 
				      between restarts */

	LeapSecondInfo	leapInfo;

	Boolean snmp_enabled; /* SNMP subsystem enabled / disabled even if 
				 compiled in */

	Boolean pcap; /* Receive and send packets using libpcap, bypassing the
			 network stack. */
	int transport; /* transport type */
	int ipMode; /* IP transmission mode */
	Boolean dot2AS; /* 801.2AS support -> transportSpecific field */

	/* list of unicast destinations for use with unicast with or without signaling */
	char unicastDestinations[MAXHOSTNAMELEN * UNICAST_MAX_DESTINATIONS];
	char unicastDomains[MAXHOSTNAMELEN * UNICAST_MAX_DESTINATIONS];
	char unicastLocalPreference[MAXHOSTNAMELEN * UNICAST_MAX_DESTINATIONS];

	Boolean		unicastDestinationsSet;
	char unicastPeerDestination[MAXHOSTNAMELEN];
	Boolean		unicastPeerDestinationSet;

	UInteger32	unicastGrantDuration;

	Boolean unicastNegotiation; /* Enable unicast negotiation support */
	Boolean	unicastNegotiationListening; /* Master: Reply to signaling messages when in LISTENING */
	Boolean disableBMCA; /* used to achieve master-only for unicast */

#ifdef RUNTIME_DEBUG
	int debug_level;
#endif
	Boolean pidAsClockId;

	/**
	 * This field holds the flags denoting which subsystems
	 * have to be re-initialised as a result of config reload.
	 * Flags are defined in daemonconfig.h
	 * 0 = no change
	 */
	UInteger32 restartSubsystems;
	/* config dictionary containers - current, candidate and from CLI*/
	dictionary *currentConfig, *candidateConfig, *cliConfig;

	int selectedPreset;

	int servoMaxPpb;
	double servoKP;
	double servoKI;
	int servoDtMethod;
	double servoMaxdT;

	/**
	 *  When enabled, ptpd ensures that Sync message sequence numbers
	 *  are increasing (consecutive sync is not lower than last).
	 *  This can prevent reordered sequences, but can also lock the slave
         *  up if, say, GM restarted and reset sequencing.
	 */
	Boolean syncSequenceChecking;

	/**
	  * (seconds) - if set to non-zero, slave will reset if no clock updates
	  * after this amount of time. Used to "unclog" slave stuck on offset filter
	  * threshold or after GM reset the Sync sequence number
          */
	int clockUpdateTimeout;

#ifdef	PTPD_STATISTICS
	OutlierFilterConfig oFilterMSConfig;
	OutlierFilterConfig oFilterSMConfig;

	StatFilterOptions filterMSOpts;
	StatFilterOptions filterSMOpts;


	Boolean servoStabilityDetection;
	double servoStabilityThreshold;
	int servoStabilityTimeout;
	int servoStabilityPeriod;

	Boolean maxDelayStableOnly;
#endif
	/* also used by the periodic message ticker */
	int statsUpdateInterval;

	int calibrationDelay;
	Boolean enablePanicMode;
	Boolean panicModeReleaseClock;
	int panicModeDuration;
	UInteger32 panicModeExitThreshold;
	int idleTimeout;


	NTPoptions ntpOptions;
	Boolean preferNTP;

	int electionDelay; /* timing domain election delay to prevent flapping */

	int maxDelayMaxRejected;

	/* max reset cycles in LISTENING before full network restart */
	int maxListen;

	Boolean managementEnabled;
	Boolean managementSetEnable;

	/* Access list settings */
	Boolean timingAclEnabled;
	Boolean managementAclEnabled;
	char timingAclPermitText[PATH_MAX];
	char timingAclDenyText[PATH_MAX];
	char managementAclPermitText[PATH_MAX];
	char managementAclDenyText[PATH_MAX];
	int timingAclOrder;
	int managementAclOrder;

} RunTimeOpts;



#endif /*DATATYPES_H_*/
