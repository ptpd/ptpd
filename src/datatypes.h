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
	Integer64 scaledNanoseconds;
} TimeInterval;

/**
* \brief The Timestamp type represents a positive time with respect to the epoch
 */
typedef struct  {
	UInteger48 secondsField;
	UInteger32 nanosecondsField;
} Timestamp;

/**
* \brief The ClockIdentity type identifies a clock
 */
typedef Octet ClockIdentity[CLOCK_IDENTITY_LENGTH];

/**
* \brief The PortIdentity identifies a PTP port.
 */
typedef struct {
	ClockIdentity clockIdentity;
	UInteger16 portNumber;
} PortIdentity;

/**
* \brief The PortAdress type represents the protocol address of a PTP port
 */
typedef struct {
	Enumeration16 networkProtocol;
	UInteger16 adressLength;
	Octet* adressField;
} PortAdress;

/**
* \brief The ClockQuality represents the quality of a clock
 */
typedef struct {
	UInteger8 clockClass;
	Enumeration8 clockAccuracy;
	UInteger16 offsetScaledLogVariance;
} ClockQuality;

/**
* \brief The TLV type represents TLV extension fields
 */
typedef struct {
	Enumeration16 tlvType;
	UInteger16 lengthField;
	Octet* valueField;
} TLV;

/**
* \brief The PTPText data type is used to represent textual material in PTP messages
 */
typedef struct {
	UInteger8 lengthField;
	Octet* textField;
} PTPText;

/**
* \brief The FaultRecord type is used to construct fault logs
 */
typedef struct {
	UInteger16 faultRecordLength;
	Timestamp faultTime;
	Enumeration8 severityCode;
	PTPText faultName;
	PTPText faultValue;
	PTPText faultDescription;
} FaultRecord;


/**
* \brief The common header for all PTP messages (Table 18 of the spec)
 */
/* Message header */
typedef struct {
	Nibble transportSpecific;
	Enumeration4 messageType;
	UInteger4 versionPTP;
	UInteger16 messageLength;
	UInteger8 domainNumber;
	Octet flagField[2];
	Integer64 correctionfield;
	PortIdentity sourcePortIdentity;
	UInteger16 sequenceId;
	UInteger8 controlField;
	Integer8 logMessageInterval;
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
* \brief Management message fields (Table 37 of the spec)
 */
/*management Message*/
typedef struct {
	PortIdentity targetPortIdentity;
	UInteger8 startingBoundaryHops;
	UInteger8 boundaryHops;
	Enumeration4 actionField;
	char* tlv;
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
	
#ifdef PTP_EXPERIMENTAL
	Integer32 MasterAddr;                           // used for hybrid mode, when receiving announces
	Integer32 LastSlaveAddr;                        // used for hybrid mode, when receiving delayreqs
#endif

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
	Boolean	noAdjust;
	Boolean	displayStats;
	Boolean	csvStats;
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
