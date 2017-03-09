#ifndef DATATYPES_H_
#define DATATYPES_H_

#include "ptp_primitives.h"
#include "ptp_datatypes.h"

#include <stdio.h>
#include <dep/iniparser/dictionary.h>
#include <dep/statistics.h>
#include "dep/alarms.h"

#include "libcck/clockdriver.h"
#include "globalconfig.h"

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

	/* todo: replace the below */
	uint32_t rxMessages[PTP_MAX_MESSAGE_INDEXED];
	uint32_t txMessages[PTP_MAX_MESSAGE_INDEXED];

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

	/* ptpmon counters */
	uint32_t ptpMonReqReceived;
	uint32_t ptpMonMtieReqReceived;

/* not implemented yet */

	/* FMR counters */
	uint32_t foreignAdded; // implement me! /* number of insertions to FMR */
	uint32_t foreignCount; // implement me! /* number of foreign masters seen */
	uint32_t foreignRemoved; // implement me! /* number of FMR records deleted */
	uint32_t foreignOverflows; // implement me! /* how many times the FMR was full */

	/* protocol engine counters */

	uint32_t stateTransitions;	  /* number of state changes */
	uint32_t bestMasterChanges;		  /* number of BM changes as result of BMC */
	uint32_t announceTimeouts;	  /* number of announce receipt timeouts */

	/* discarded / uknown / ignored */
	uint32_t discardedMessages;	  /* only messages we shouldn't be receiving - ignored from self don't count */
	uint32_t unknownMessages;	  /* unknown type - also increments discarded */
	uint32_t ignoredAnnounce;	  /* ignored Announce messages: acl / security / preference */

	/* error counters */
	uint32_t messageRecvErrors;	  /* message receive errors */
	uint32_t messageSendErrors;	  /* message send errors */
	uint32_t messageFormatErrors;	  /* headers or messages too short etc. */
	uint32_t protocolErrors;	  /* conditions that shouldn't happen */
	uint32_t versionMismatchErrors;	  /* V1 received, V2 expected - also increments discarded */
	uint32_t domainMismatchErrors;	  /* different domain than configured - also increments discarded */
	uint32_t sequenceMismatchErrors;  /* mismatched sequence IDs - also increments discarded */
	uint32_t delayMechanismMismatchErrors; /* P2P received, E2E expected or vice versa - incremets discarded */
	uint32_t consecutiveSequenceErrors;    /* number of consecutive sequence mismatch errors */

	/* not in SNMP yet */
	uint32_t txTimestampFailures;		/* number of transmit timestamp delivery failures */

	/* unicast sgnaling counters */
	uint32_t unicastGrantsRequested;  /* slave: how many we requested, master: how many requests we received */
	uint32_t unicastGrantsGranted;	  /* slave: how many we got granted, master: how many we granted */
	uint32_t unicastGrantsDenied;	 /* slave: how many we got denied, master: how many we denied */
	uint32_t unicastGrantsCancelSent;   /* how many we canceled */
	uint32_t unicastGrantsCancelReceived; /* how many cancels we received */
	uint32_t unicastGrantsCancelAckSent; /* how many cancel ack we sent */
	uint32_t unicastGrantsCancelAckReceived; /* how many cancel ack we received */

	uint32_t delayMSOutliers;	  /* Number of outliers found by the delayMS filter */
	uint32_t delaySMOutliers;	  /* Number of outliers found by the delaySM filter */
	uint32_t maxDelayDrops;		  /* number of samples dropped due to maxDelay threshold */

	uint32_t messageSendRate;	/* TX message rate per sec */
	uint32_t messageReceiveRate;	/* RX message rate per sec */
	uint32_t bytesSendRate;		/* TX Bps */
	uint32_t bytesReceiveRate;	/* RX Bps */

} PtpdCounters;

typedef struct {
	Boolean activity; 		/* periodic check, updateClock sets this to let the watchdog know we're holding clock control */
	Boolean	available; 	/* flags that we can control the clock */
	Boolean granted; 	/* upstream watchdog will grant this when we're the best provider */
	Boolean updateOK;		/* if not, updateClock() will not run */
	Boolean stepRequired;		/* if set, we need to step the clock */
	Boolean stepFailed;		/* clock driver refused to step clock, we're locked until it's forcefully stepped */
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
	void*			protocolAddress;	/* user-supplied address object - we are agnostic to this - address  of slave / or master*/
	UInteger8		domainNumber;		/* domain of the master - as used by Telecom Profile */
	UInteger8		localPreference;		/* local preference - as used by Telecom profile */
	PortIdentity    	portIdentity;		/* master: port ID of grantee, slave: portID of grantor */
	UnicastGrantData	grantData[PTP_MAX_MESSAGE_INDEXED];/* master: grantee's grants, slave: grantor's grant status */
	UInteger32		timeLeft;		/* time until expiry of last grant (max[grants.timeLeft]. when runs out and no renewal, entry can be re-used */
	Boolean			isPeer;			/* this entry is peer only */
	TimeInternal		lastSyncTimestamp;		/* last Sync message timestamp sent */
};

/* Unicast index holder: data + port mask */
typedef struct {
	UnicastGrantTable* data[UNICAST_MAX_DESTINATIONS];
	UInteger16 portMask;
} UnicastGrantIndex;

/* Unicast destination configuration: Address, domain, preference, last Sync timestamp sent */
typedef struct {
    void*		protocolAddress;		/* user-supplied address object - we are agnostic to this - destination address */
    UInteger8 		domainNumber;			/* domain number - for slaves with masters in multiple domains */
    UInteger8 		localPreference;		/* local preference to influence BMC */
    TimeInternal 	lastSyncTimestamp;			/* last Sync timestamp sent */
} UnicastDestination;

typedef struct {
    void	*protocolAddress;
} SyncDestEntry;

/* PTP data for transmission / received PTP data */
typedef struct {
    void *src;			/* source address */
    void *dst;			/* destination address */
    char *data;			/* data buffer */
    bool hasTimestamp;		/* message contains an RX / TX timestamp */
    TimeInternal timestamp;	/* RX / TX timestamp */
} PtpData;

/**
 * \struct PtpClock
 * \brief Main program data structure
 */
/* main program data structure */

typedef struct PtpClock PtpClock;

struct PtpClock {

	GlobalConfig *global;

	/* PTP datsets */
	DefaultDS defaultDS; 			/* Default data set */
	CurrentDS currentDS; 			/* Current data set */
	TimePropertiesDS timePropertiesDS; 	/* Global time properties data set */
	PortDS portDS; 				/* Port data set */
	ParentDS parentDS; 			/* Parent data set */

	/* Leap second related flags */
        Boolean leapSecondInProgress;
        Boolean leapSecondPending;

	/* Foreign master data set */
	ForeignMasterRecord *foreign;
	/* Current best master (unless it's us) */
	ForeignMasterRecord *bestMaster;

	/* Other things we need for the protocol */
	UInteger16 number_foreign_records;
	Integer16  fmrCapacity;
	Integer16  foreign_record_i;
	Integer16  foreign_record_best;
	Boolean  record_update;    /* should we run bmc() after receiving an announce message? */

	Boolean disabled;	/* port is permanently disabled */

	/* unicast grant table - our own grants or our slaves' grants or grants to peers */
	UnicastGrantTable unicastGrants[UNICAST_MAX_DESTINATIONS];
	/* our trivial index table to speed up lookups */
	UnicastGrantIndex grantIndex;
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
	TimeInternal	lastOriginTimestamp;

	/* todo: replace the below */
	UInteger16 txId[PTP_MAX_MESSAGE_INDEXED];

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
	IIRfilter  mpdIirFilter;

	Boolean message_activity;

	PtpTimer	timers[PTP_MAX_TIMER];
	AlarmEntry	alarms[ALRM_MAX];
	int alarmDelay;

	/* user-supplied objects go here */

	void *clockDriver;		/* some clock driver interface */
	void *eventTransport;		/* some transport to send/receive event messages */
	void *generalTransport;		/* some transport to send/receive general messages */
	void *eventDestination;		/* some default address object to send event messages to */
	void *generalDestination;	/* some default address object to send general messages to */
	void *peerEventDestination;	/* some default address object to send P2P event messages to */
	void *peerGeneralDestination;   /* some default address object to send P2P general messages to */
	void *userData;			/* any other data we want attached to the PTP clock */
	void *owner;			/* some arbitrary object owning or controlling this PTP clock */

	struct {
	    void (*preInit) (PtpClock *ptpClock);
	    void (*postInit) (PtpClock *ptpClock);
//	    void (*preShutdown) (PtpClock *ptpClock);
//	    void (*postShutdown) (PtpClock *ptpClock);
//	    int (*addrStrLen)	(void *addr);
//	    char (*addrToString) (char *buf, const int len, void *addr);
	    bool (*onStateChange) (PtpClock *ptpClock, const uint8_t from, const uint8_t to);
	} callbacks;

	/*Stats header will be re-printed when set to true*/
	Boolean resetStatisticsLog;

	int listenCount; // number of consecutive resets to listening
	int resetCount;
	int announceTimeouts;
	Boolean warnedUnicastCapacity;
	int maxDelayRejected;

	Boolean runningBackupInterface;

	char char_last_msg;                             /* representation of last message processed by servo */

	int syncWaiting;                     /* we'll only start the delayReq timer after the first sync */
	int delayRespWaiting;                /* Just for information purposes */
	Boolean startup_in_progress;

	Boolean pastmrStartup;				/* we've set the clock already, at least once */

	Boolean	offsetFirstUpdated;

	/* user description is max size + 1 to leave space for a null terminator */
	Octet userDescription[USER_DESCRIPTION_MAX + 1];
	Octet profileIdentity[6];

	void *lastSyncDst;		/* destination address for last sync, so we know where to send the followUp - last resort: we should capture the dst address ourselves */
	void *lastPdelayRespDst;	/* captures the destination address of last pdelayResp so we know where to send the pdelayRespfollowUp */

	/*
	 * counters - useful for debugging and monitoring,
	 * should be exposed through management messages
	 * and SNMP eventually
	 */
	PtpdCounters counters;

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

	Integer32 acceptedUpdates;
	Integer32 offsetUpdates;

	Boolean isCalibrated;
	double dT;

	NTPcontrol ntpControl;

	/* accumulating offset correction added when smearing leap second */
	double leapSmearFudge;

	/* configuration applied by management messages */
	dictionary *managementConfig;

	/* testing only - used to add a 1ms offset */
#if 1
	Boolean addOffset;
#endif

	bool clockLocked;

	/* tell the protocol engine to silently ignore the next n offset/delay updates */
	int ignoreDelayUpdates;
	int ignoreOffsetUpdates;

};


#endif /*DATATYPES_H_*/
