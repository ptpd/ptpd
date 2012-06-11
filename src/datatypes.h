#ifndef DATATYPES_H_
#define DATATYPES_H_

#include <stdio.h> 

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
* \brief PDelayReq message fields (Table 29 of the spec)
 */
/*PdelayReq Message*/
typedef struct {
	Timestamp originTimestamp;
}MsgPDelayReq;

/**
* \brief PDelayResp message fields (Table 30 of the spec)
 */
/*PdelayResp Message*/
typedef struct {
	Timestamp requestReceiptTimestamp;
	PortIdentity requestingPortIdentity;
}MsgPDelayResp;

/**
* \brief PDelayRespFollowUp message fields (Table 31 of the spec)
 */
/*PdelayRespFollowUp Message*/
typedef struct {
	Timestamp responseOriginTimestamp;
	PortIdentity requestingPortIdentity;
}MsgPDelayRespFollowUp;

/**
* \brief Signaling message fields (Table 33 of the spec)
 */
/*Signaling Message*/
typedef struct {
	PortIdentity targetPortIdentity;
	char* tlv;
}MsgSignaling;


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
}MsgManagement;

/**
* \brief Time structure to handle Linux time information
 */
typedef struct {
	Integer32 seconds;
	Integer32 nanoseconds;
} TimeInternal;

/**
* \brief Structure used as a timer
 */
typedef struct {
	Integer32 interval;
	Integer32 left;
	Boolean expire;
} IntervalTimer;

/**
* \brief ForeignMasterRecord is used to manage foreign masters
 */
typedef struct
{
	PortIdentity foreignMasterPortIdentity;
	UInteger16 foreignMasterAnnounceMessages;

	//This one is not in the spec
	MsgAnnounce  announce;
	MsgHeader    header;
} ForeignMasterRecord;

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

	/*Dynamic members*/
	Integer16 currentUtcOffset;
	Boolean currentUtcOffsetValid;
	Boolean leap59;
	Boolean leap61;
	Boolean timeTraceable;
	Boolean frequencyTraceable;
	Boolean ptpTimescale;
        Boolean leapSecondInProgress;
        Boolean leapSecondPending;
	Enumeration8 timeSource;

	/* Port configuration data set */

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


	/* Foreign master data set */
	ForeignMasterRecord *foreign;

	/* Other things we need for the protocol */
	UInteger16 number_foreign_records;
	Integer16  max_foreign_records;
	Integer16  foreign_record_i;
	Integer16  foreign_record_best;
	UInteger32 random_seed;
	Boolean  record_update;    /* should we run bmc() after receiving an announce message? */


	MsgHeader msgTmpHeader;

	union {
		MsgSync  sync;
		MsgFollowUp  follow;
		MsgDelayReq  req;
		MsgDelayResp resp;
		MsgPDelayReq  preq;
		MsgPDelayResp  presp;
		MsgPDelayRespFollowUp  prespfollow;
		MsgManagement  manage;
		MsgAnnounce  announce;
		MsgSignaling signaling;
	} msgTmp;

	MsgManagement outgoingManageTmp;

	Octet msgObuf[PACKET_SIZE];
	Octet msgIbuf[PACKET_SIZE];

/*
	20110630: These variables were deprecated in favor of the ones that appear in the stats log (delayMS and delaySM)
	
	TimeInternal  master_to_slave_delay;
	TimeInternal  slave_to_master_delay;

	*/
	Integer32 	observed_drift;

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
	TimeInternal  delayMS;
	TimeInternal	delaySM;
	TimeInternal  lastSyncCorrectionField;
	TimeInternal  lastPdelayRespCorrectionField;



	Boolean  sentPDelayReq;
	UInteger16  sentPDelayReqSequenceId;
	UInteger16  sentDelayReqSequenceId;
	UInteger16  sentSyncSequenceId;
	UInteger16  sentAnnounceSequenceId;
	UInteger16  recvPDelayReqSequenceId;
	UInteger16  recvSyncSequenceId;
	UInteger16  recvPDelayRespSequenceId;
	Boolean  waitingForFollow;
	Boolean  waitingForDelayResp;
	
	offset_from_master_filter  ofm_filt;
	one_way_delay_filter  owd_filt;

	Boolean message_activity;

	IntervalTimer  itimer[TIMER_ARRAY_SIZE];

	NetPath netPath;

	/*Usefull to init network stuff*/
	UInteger8 port_communication_technology;
	Octet port_uuid_field[PTP_UUID_LENGTH];

	int reset_count;
	int current_init_clock;
	int can_step_clock;
	int warned_operator_slow_slewing;
	int warned_operator_fast_slewing;

	char char_last_msg;                             /* representation of last message processed by servo */
	Boolean last_packet_was_sync;                   /* used to limit logging of Sync messages */
	
	int waiting_for_first_sync;                     /* we'll only start the delayReq timer after the first sync */
	int waiting_for_first_delayresp;                /* Just for information purposes */
	Boolean startup_in_progress;
	
	/* user description is max size + 1 to leave space for a null terminator */
	Octet user_description[USER_DESCRIPTION_MAX + 1];

#ifdef PTP_EXPERIMENTAL
	Integer32 MasterAddr;                           // used for hybrid mode, when receiving announces
	Integer32 LastSlaveAddr;                        // used for hybrid mode, when receiving delayreqs
#endif
	Integer32 discardedPacketCount;                 // used for autotuning the maxDelay threshold

} PtpClock;

/**
 * \struct RunTimeOpts
 * \brief Program options set at run-time
 */
/* program options set at run-time */
typedef struct {
	Integer8 announceInterval;
	Integer8 announceReceiptTimeout;
	
	Integer8 syncInterval;
	ClockQuality clockQuality;
	UInteger8 priority1;
	UInteger8 priority2;
	UInteger8 domainNumber;
#ifdef PTP_EXPERIMENTAL
	UInteger8 mcast_group_Number;
#endif

	Boolean	slaveOnly;
	Integer16 currentUtcOffset;
	Octet ifaceName[IFACE_NAME_LENGTH];
	Boolean	noResetClock;
	Integer32 maxReset; /* Maximum number of nanoseconds to reset */
	Integer32 maxDelay; /* Maximum number of nanoseconds of delay */
	Integer32 origMaxDelay; /* Lower bound of nanoseconds of delay */
	Boolean maxDelayAutoTune;
	Integer32 discardedPacketThreshold;
	Boolean	noAdjust;
	Boolean	displayStats;
	Boolean displayPackets;
	Octet unicastAddress[MAXHOSTNAMELEN];
	Integer32 ap, ai;
	Integer16 s;
	TimeInternal inboundLatency, outboundLatency;
	Integer16 max_foreign_records;
	Boolean ethernet_mode;
	Enumeration8 delayMechanism;
	Boolean	offset_first_updated;
	char file[PATH_MAX];
	int logFd;
	Boolean useSysLog;
	int ttl;
	char recordFile[PATH_MAX];
	FILE *recordFP;

	int log_seconds_between_message;

	Boolean ignore_daemon_lock;
	Boolean do_IGMP_refresh;
	int nonDaemon;
	Boolean do_log_to_file;
	Boolean do_record_quality_file;
	
	int initial_delayreq;
	int subsequent_delayreq;
	Boolean ignore_delayreq_interval_master;
	Boolean syslog_startup_messages_also_to_stdout;
	
#ifdef PTP_EXPERIMENTAL
	int do_hybrid_mode;
#endif
	int do_unicast_mode;

#ifdef RUNTIME_DEBUG
	int debug_level;
#endif

} RunTimeOpts;

#endif /*DATATYPES_H_*/
