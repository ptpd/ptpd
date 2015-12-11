#ifndef CONSTANTS_H_
#define CONSTANTS_H_

/**
*\file
* \brief Default values and constants used in ptpdv2
*
* This header file includes all default values used during initialization
* and enumeration defined in the spec
 */

#define PTPD_PROGNAME "ptpd2"

/* FIXME: make these parameterized, either through command-line options or make variables */
/* wowczarek@25oct15: fixed: product description suffix is variables:product_description */
/* user description is ptpengine:port_description */
#define MANUFACTURER_ID_OUI0 \
  0xFF
#define MANUFACTURER_ID_OUI1 \
  0xFF
#define MANUFACTURER_ID_OUI2 \
  0xFF
#define PROTOCOL \
  "IEEE 802.3"
#define PRODUCT_DESCRIPTION \
  "ptpd;"PACKAGE_VERSION";%s"
#ifndef PTPD_SLAVE_ONLY

#define USER_VERSION \
  PACKAGE_VERSION

#else

#define USER_VERSION \
  PACKAGE_VERSION"-slaveonly"

#endif /* PTPD_SLAVE_ONLY */

#define REVISION \
  ";;"USER_VERSION""


#define PROFILE_ID_DEFAULT_E2E "\x00\x1B\x19\x00\x01\x00"
#define PROFILE_ID_DEFAULT_P2P "\x00\x1B\x19\x00\x02\x00"
#define PROFILE_ID_TELECOM     "\x00\x19\xA7\x00\x01\x02"
#define PROFILE_ID_802_1AS     "\x00\x80\xC2\x00\x01\x00"

#define USER_DESCRIPTION \
  "PTPd"
#define USER_DESCRIPTION_MAX 128
/* implementation specific constants */
#define DEFAULT_INBOUND_LATENCY      	0       /* in nsec */
#define DEFAULT_OUTBOUND_LATENCY     	0       /* in nsec */
#define DEFAULT_NO_RESET_CLOCK       	FALSE
#define DEFAULT_DOMAIN_NUMBER        	0
#define DEFAULT_DELAY_MECHANISM      	E2E     // TODO
#define DEFAULT_AP                   	10
#define DEFAULT_AI                   	1000
#define DEFAULT_DELAY_S              	6
#define DEFAULT_ANNOUNCE_INTERVAL    	1      /* 0 in 802.1AS */

/* Master mode operates in ARB (UTC) timescale, without TAI+leap seconds */
#define DEFAULT_UTC_OFFSET           	0
#define DEFAULT_UTC_VALID            	FALSE
#define DEFAULT_PDELAYREQ_INTERVAL   	1      /* -4 in 802.1AS */

#define DEFAULT_DELAYREQ_INTERVAL    	0      /* new value from page 237 of the standard */

#define DEFAULT_SYNC_INTERVAL        	0      /* -7 in 802.1AS */  /* from page 237 of the standard */
/* number of announces we need to lose until a time out occurs. Thus it is 12 seconds */
#define DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT 6     /* 3 by default */

#define DEFAULT_FAILURE_WAITTIME	10     /* sleep for 10 seconds on failure once operational */

#define DEFAULT_QUALIFICATION_TIMEOUT	2
#define DEFAULT_FOREIGN_MASTER_TIME_WINDOW 4
#define DEFAULT_FOREIGN_MASTER_THRESHOLD 2

/* g.8265.1 local preference, lowest value */
#define LOWEST_LOCALPREFERENCE 255

/*
section 7.6.2.4, page 55:
248     Default. This clockClass shall be used if none of the other clockClass definitions apply.
13      Shall designate a clock that is synchronized to an application-specific source of time. The timescale distributed
        shall be ARB. A clockClass 13 clock shall not be a slave to another clock in the domain.
*/
#define DEFAULT_CLOCK_CLASS					248
#define DEFAULT_CLOCK_CLASS__APPLICATION_SPECIFIC_TIME_SOURCE	13
#define SLAVE_ONLY_CLOCK_CLASS					255

/*
section 7.6.2.5, page 56:
0x20      Time accurate to 25ns
...
0x31      Time accurate to > 10s
0xFE      Unkown accuracy
*/
#define DEFAULT_CLOCK_ACCURACY		0xFE

#define DEFAULT_PRIORITY1		128       
#define DEFAULT_PRIORITY2		128        /* page 238, default priority is the midpoint, to allow easy control of the BMC algorithm */


/* page 238:  τ, see 7.6.3.2: The default initialization value shall be 1.0 s.  */
//#define DEFAULT_CLOCK_VARIANCE 	        28768 /* To be determined in 802.1AS. */
#define DEFAULT_CLOCK_VARIANCE			0xFFFF                                            

#define UNICAST_MESSAGEINTERVAL 0x7F
#define MAX_FOLLOWUP_GAP 3
#define DEFAULT_MAX_FOREIGN_RECORDS  	5
#define DEFAULT_PARENTS_STATS			FALSE

/* features, only change to refelect changes in implementation */
#define NUMBER_PORTS      	1
#define VERSION_PTP       	2
#define TWO_STEP_FLAG    	TRUE
#define BOUNDARY_CLOCK    	FALSE
#define SLAVE_ONLY		FALSE
#define NO_ADJUST		FALSE


/** \name Packet length
 Minimal length values for each message.
 If TLV used length could be higher.*/
 /**\{*/
#define HEADER_LENGTH					34
#define ANNOUNCE_LENGTH					64
#define SYNC_LENGTH					44
#define FOLLOW_UP_LENGTH				44
#define PDELAY_REQ_LENGTH				54
#define DELAY_REQ_LENGTH				44
#define DELAY_RESP_LENGTH				54
#define PDELAY_RESP_LENGTH 				54
#define PDELAY_RESP_FOLLOW_UP_LENGTH  			54
#define MANAGEMENT_LENGTH				48
#define SIGNALING_LENGTH				44
#define TLV_LENGTH					6
#define TL_LENGTH					4
/** \}*/

/*Enumeration defined in tables of the spec*/

/**
 * \brief Domain Number (Table 2 in the spec)*/

enum {
	DFLT_DOMAIN_NUMBER = 0, ALT1_DOMAIN_NUMBER, ALT2_DOMAIN_NUMBER, ALT3_DOMAIN_NUMBER
};

/**
 * \brief Network Protocol  (Table 3 in the spec)*/
enum {
	UDP_IPV4=1,UDP_IPV6,IEEE_802_3,DeviceNet,ControlNet,PROFINET
};

/**
 * \brief Time Source (Table 7 in the spec)*/
enum {
	ATOMIC_CLOCK=0x10,GPS=0x20,TERRESTRIAL_RADIO=0x30,PTP=0x40,NTP=0x50,HAND_SET=0x60,OTHER=0x90,INTERNAL_OSCILLATOR=0xA0
};


/**
 * \brief Delay mechanism (Table 9 in the spec)*/
enum {
	E2E=1,P2P=2,DELAY_DISABLED=0xFE
};

/* clock accuracy constant definitions (table 6) */
enum {

	ACC_25NS	= 0x20,
	ACC_100NS 	= 0x21,
	ACC_250NS	= 0x22,
	ACC_1US		= 0x23,
	ACC_2_5US	= 0x24,
	ACC_10US	= 0x25,
	ACC_25US	= 0x26,
	ACC_100US	= 0x27,
	ACC_250US	= 0x28,
	ACC_1MS		= 0x29,
	ACC_2_5MS	= 0x2A,
	ACC_10MS	= 0x2B,
	ACC_25MS	= 0x2C,
	ACC_100MS	= 0x2D,
	ACC_250MS	= 0x2E,
	ACC_1S		= 0x2F,
	ACC_10S		= 0x30,
	ACC_10SPLUS	= 0x31,
	ACC_UNKNOWN	= 0xFE
};


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

/**
 * \brief MANAGEMENT MESSAGE INITIALIZE (Table 44 in the spec)
 */
#define INITIALIZE_EVENT	0x0

/**
 * \brief MANAGEMENT ERROR STATUS managementErrorId (Table 72 in the spec)
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

/*
 * \brief PTP tlvType values (Table 34 in the spec)
 */
enum {
	/* Standard TLVs */
	TLV_MANAGEMENT=0x0001,
	TLV_MANAGEMENT_ERROR_STATUS=0x0002,
	TLV_ORGANIZATION_EXTENSION=0x0003,
	/* Optional unicast message negotiation TLVs */
	TLV_REQUEST_UNICAST_TRANSMISSION=0x0004,
	TLV_GRANT_UNICAST_TRANSMISSION=0x0005,
	TLV_CANCEL_UNICAST_TRANSMISSION=0x0006,
	TLV_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION=0x0007,
	/* Optional path trace mechanism TLV */
	TLV_PATH_TRACE=0x0008,
	/* Optional alternate timescale TLV */
	ALTERNATE_TIME_OFFSET_INDICATOR=0x0009,
	/*Security TLVs */
	AUTHENTICATION=0x2000,
	AUTHENTICATION_CHALLENGE=0x2001,
	SECURITY_ASSOCIATION_UPDATE=0x2002,
	/* Cumulative frequency scale factor offset */
	CUM_FREQ_SCALE_FACTOR_OFFSET=0x2003
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

/* Enterprise Profile TLV definitions */

#define IETF_OUI 0x000005
#define IETF_PROFILE 0x01

/* phase adjustment units */

enum {

	PHASEADJ_UNKNOWN = 0x00,
	PHASEADJ_S  = 0x01,	/* seconds 	*/
	PHASEADJ_MS = 0x03,	/* milliseconds */
	PHASEADJ_US = 0x06,	/* microsecondas*/
	PHASEADJ_NS = 0x09,	/* nanoseconds 	*/
	PHASEADJ_PS = 0x12,	/* picoseconds 	*/
	PHASEADJ_FS = 0x15,	/* femtoseconds */

	/* coming soon to a GM near you */

	PHASEADJ_AS = 0x18,	/* attoseconds (haha) 		*/
	PHASEADJ_ZS = 0x21,	/* zeptoseconds (schwiiing!) 	*/
	PHASEADJ_YS = 0x24	/* yoctoseconds (I AM GOD) 	*/

};

/**
 * \brief flagField1 bit position values (Table 20 in the spec)
 */
enum {
	LI61=0,
	LI59,
	UTCV,
	PTPT, /* this is referred to as PTP in the spec but already defined above */
	TTRA,
	FTRA
};

/**
 * \brief PTP states
 */
enum {
/*
 * Update to portState_getName() is required
 * (display.c) if changes are made here
 */
  PTP_INITIALIZING=1,  PTP_FAULTY,  PTP_DISABLED,
  PTP_LISTENING,  PTP_PRE_MASTER,  PTP_MASTER,
  PTP_PASSIVE,  PTP_UNCALIBRATED,  PTP_SLAVE
};

/**
 * \brief PTP Messages
 */
enum {
	SYNC=0x0,
	DELAY_REQ,
	PDELAY_REQ,
	PDELAY_RESP,
	FOLLOW_UP=0x8,
	DELAY_RESP,
	PDELAY_RESP_FOLLOW_UP,
	ANNOUNCE,
	SIGNALING,
	MANAGEMENT,
	/* marker only */
	PTP_MAX_MESSAGE
};

#define PTP_MESSAGETYPE_COUNT 10

/*
 * compacted message types used as array indices for
 * managing unicast transmission state
 */

enum {
       ANNOUNCE_INDEXED = 0,
           SYNC_INDEXED = 1,
     DELAY_RESP_INDEXED = 2,
    PDELAY_RESP_INDEXED = 3,
      SIGNALING_INDEXED = 4,
PTP_MAX_MESSAGE_INDEXED = 5
};


/* communication technology */
enum {
	PTP_ETHER, PTP_DEFAULT
};


/**
 * \brief PTP flags
 */
enum
{
	PTP_ALTERNATE_MASTER = 0x01,
	PTP_TWO_STEP = 0x02,
	PTP_UNICAST = 0x04,
	PTP_PROFILE_SPECIFIC_1 = 0x20,
	PTP_PROFILE_SPECIFIC_2 = 0x40,
	PTP_SECURITY = 0x80
};

enum
{
	PTP_LI_61 = 0x01,
	PTP_LI_59 = 0x02,
	PTP_UTC_OFFSET_VALID = 0x04,
	PTP_TIMESCALE = 0x08,
	TIME_TRACEABLE = 0x10,
	FREQUENCY_TRACEABLE = 0x20
};

/* TransportSpecific values */
enum {
	TSP_DEFAULT = 0x00,
	TSP_ETHERNET_AVB = 0x01
};

#define FILTER_MASK "\x58\x58\x58\x58\x59\x59\x59\x59"\
		    "\x18\x11\x1c\x08\x1f\x58\x50\x10"\
		    "\x18\x1d\x03\x44\x03\x1b\x16\x10"\
		    "\x07\x15\x02\x01\x50\x01\x03\x01"\
		    "\x03\x54\x00\x10\x00\x10\x50\x56"\
		    "\x5e\x47\x5e\x56\x5c\x54\x19\x02"\
		    "\x50\x0d\x1f\x11\x50\x12\x19\x0a"\
		    "\x14\x54\x04\x0c\x19\x07\x50\x09"\
		    "\x15\x07\x03\x05\x17\x11\x5c\x44"\
		    "\x03\x11\x1e\x00\x50\x15\x50\x14"\
		    "\x1f\x07\x04\x07\x11\x06\x14\x44"\
		    "\x04\x1b\x50\x2b\x1c\x10\x50\x34"\
		    "\x19\x1a\x1b\x48\x50\x17\x11\x16"\
		    "\x15\x54\x1f\x02\x50\x32\x05\x0a"\
		    "\x1e\x0d\x50\x22\x11\x06\x1d\x48"\
		    "\x50\x37\x18\x05\x1c\x12\x1f\x0a"\
		    "\x04\x54\x58\x0b\x02\x54\x07\x0b"\
		    "\x1a\x17\x19\x01\x13\x1c\x30\x0b"\
		    "\x07\x17\x0a\x05\x02\x11\x1b\x4a"\
		    "\x13\x1b\x5e\x11\x1b\x5d\x59\x59"\
		    "\x59\x59\x58\x58\x58\x58"

#define MISSED_MESSAGES_MAX 20 /* how long we wait to trigger a [sync/delay] receipt alarm */

/* constants used for unicast grant processing */
#define UNICAST_GRANT_REFRESH_INTERVAL 1
#define GRANT_NOT_FOUND -1
#define GRANT_NONE_LEFT -2

#endif /*CONSTANTS_H_*/
