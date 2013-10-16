/**
 * @file   ptpd.h
 * @mainpage Ptpd v2 Documentation
 * @authors Martin Burnicki, Alexandre van Kempen, Steven Kreuzer, 
 *          George Neville-Neil
 * @version 2.0
 * @date   Fri Aug 27 10:22:19 2010
 * 
 * @section implementation Implementation
 * PTTdV2 is not a full implementation of 1588 - 2008 standard.
 * It is implemented only with use of Transparent Clock and Peer delay
 * mechanism, according to 802.1AS requierements.
 * 
 * This header file includes all others headers.
 * It defines functions which are not dependant of the operating system.
 */

#ifndef PTPD_H_
#define PTPD_H_

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef linux
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#endif /* linux */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <limits.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/resource.h>
#ifndef __APPLE__
#include <sys/timex.h>
#endif
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <syslog.h>
#include <limits.h>
#include <getopt.h>
#include <ctype.h>
#include <glob.h>
#include <stddef.h>
#include <stdint.h>
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#else
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif /* HAVE_UTMP_H */
#endif /* HAVE_UTMPX_H */
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#ifdef HAVE_PCAP_PCAP_H
#include <pcap/pcap.h>
#else
#include <pcap.h>
#endif
#ifdef linux
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif /* HAVE_SCHED_H */
#endif /* linux */

#include "constants.h"
#include "limits.h"

#include "dep/ipv4_acl.h"

#include "dep/constants_dep.h"
#include "dep/datatypes_dep.h"

#ifdef PTPD_NTPDC
#include "dep/ntpengine/ntpdcontrol.h"
#include "dep/ntpengine/ntp_isc_md5.h"
#endif /* PTPDC_NTPDC */

#include "datatypes.h"

#ifdef PTPD_STATISTICS
#include "dep/statistics.h"
#endif

#include "dep/ptpd_dep.h"
#include "dep/iniparser/dictionary.h"
#include "dep/iniparser/iniparser.h"
#include "dep/daemonconfig.h"

/* NOTE: this macro can be refactored into a function */
#define XMALLOC(ptr,size) \
	if(!((ptr)=malloc(size))) { \
		PERROR("failed to allocate memory"); \
		ptpdShutdown(ptpClock); \
		exit(1); \
	}

#define IS_SET(data, bitpos) \
	((data & ( 0x1 << bitpos )) == (0x1 << bitpos))

#define SET_FIELD(data, bitpos) \
	data << bitpos

#define min(a,b)     (((a)<(b))?(a):(b))
#define max(a,b)     (((a)>(b))?(a):(b))


/** \name arith.c
 * -Timing management and arithmetic*/
 /**\{*/
/* arith.c */

/**
 * \brief Convert Integer64 into TimeInternal structure
 */
void integer64_to_internalTime(Integer64,TimeInternal*);
/**
 * \brief Convert TimeInternal structure to Integer64
 */
void internalTime_to_integer64(TimeInternal, Integer64*);
/**
 * \brief Convert TimeInternal into Timestamp structure (defined by the spec)
 */
void fromInternalTime(const TimeInternal*,Timestamp*);

/**
 * \brief Convert Timestamp to TimeInternal structure (defined by the spec)
 */
void toInternalTime(TimeInternal*, const Timestamp*);

void ts_to_InternalTime(const struct timespec *, TimeInternal *);
void tv_to_InternalTime(const struct timeval  *, TimeInternal *);




/**
 * \brief Use to normalize a TimeInternal structure
 *
 * The nanosecondsField member must always be less than 10⁹
 * This function is used after adding or substracting TimeInternal
 */
void normalizeTime(TimeInternal*);

/**
 * \brief Add two InternalTime structure and normalize
 */
void addTime(TimeInternal*,const TimeInternal*,const TimeInternal*);

/**
 * \brief Substract two InternalTime structure and normalize
 */
void subTime(TimeInternal*,const TimeInternal*,const TimeInternal*);
/** \}*/

/**
 * \brief Divied an InternalTime by 2
 */
void div2Time(TimeInternal *);

/** \name bmc.c
 * -Best Master Clock Algorithm functions*/
 /**\{*/
/* bmc.c */
/**
 * \brief Compare data set of foreign masters and local data set
 * \return The recommended state for the port
 */
UInteger8 bmc(ForeignMasterRecord*, const RunTimeOpts*,PtpClock*);

/**
 * \brief When recommended state is Master, copy local data into parent and grandmaster dataset
 */
void m1(const RunTimeOpts *, PtpClock*);

/**
 * \brief When recommended state is Slave, copy dataset of master into parent and grandmaster dataset
 */
void s1(MsgHeader*,MsgAnnounce*,PtpClock*, const RunTimeOpts *);


void p1(PtpClock *ptpClock, const RunTimeOpts *rtOpts);

/**
 * \brief Initialize datas
 */
void initData(RunTimeOpts*,PtpClock*);
/** \}*/


/** \name protocol.c
 * -Execute the protocol engine*/
 /**\{*/
/**
 * \brief Protocol engine
 */
/* protocol.c */
void protocol(RunTimeOpts*,PtpClock*);
/** \}*/

/** \name management.c
 * -Management message support*/
 /**\{*/
/* management.c */
/**
 * \brief Management message support
 */
void handleMMNullManagement(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMClockDescription(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMSlaveOnly(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMUserDescription(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMSaveInNonVolatileStorage(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMResetNonVolatileStorage(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMInitialize(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMDefaultDataSet(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMCurrentDataSet(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMParentDataSet(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMTimePropertiesDataSet(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMPortDataSet(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMPriority1(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMPriority2(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMDomain(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMLogAnnounceInterval(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMAnnounceReceiptTimeout(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMLogSyncInterval(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMVersionNumber(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMEnablePort(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMDisablePort(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMTime(MsgManagement*, MsgManagement*, PtpClock*, RunTimeOpts*);
void handleMMClockAccuracy(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMUtcProperties(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMTraceabilityProperties(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMDelayMechanism(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMLogMinPdelayReqInterval(MsgManagement*, MsgManagement*, PtpClock*);
void handleMMErrorStatus(MsgManagement*);
void handleErrorManagementMessage(MsgManagement *incoming, MsgManagement *outgoing,
                                PtpClock *ptpClock, Enumeration16 mgmtId,
                                Enumeration16 errorId);
/** \}*/

/*
 * \brief Packing and Unpacking macros
 */
#define DECLARE_PACK( type ) void pack##type( void*, void* );

DECLARE_PACK( NibbleUpper )
DECLARE_PACK( Enumeration4Lower )
DECLARE_PACK( UInteger4Lower )
DECLARE_PACK( UInteger16 )
DECLARE_PACK( UInteger8 )
DECLARE_PACK( Octet )
DECLARE_PACK( Integer8 )
DECLARE_PACK( UInteger48 )
DECLARE_PACK( Integer64 )

#define DECLARE_UNPACK( type ) void unpack##type( void*, void*, PtpClock *ptpClock );

DECLARE_UNPACK( Boolean )
DECLARE_UNPACK( Enumeration4Lower )
DECLARE_UNPACK( Octet )
DECLARE_UNPACK( UInteger48 )
DECLARE_UNPACK( Integer64 )

//Diplay functions usefull to debug
void displayRunTimeOpts(const RunTimeOpts*);
void displayDefault (const PtpClock*);
void displayCurrent (const PtpClock*);
void displayParent (const PtpClock*);
void displayGlobal (const PtpClock*);
void displayPort (const PtpClock*);
void displayForeignMaster (const PtpClock*);
void displayOthers (const PtpClock*);
void displayBuffer (const PtpClock*);
void displayPtpClock (const PtpClock*);
void timeInternal_display(const TimeInternal*);
void clockIdentity_display(const ClockIdentity);
void netPath_display(const NetPath*);
void intervalTimer_display(const IntervalTimer*);
void integer64_display (const Integer64*);
void timeInterval_display(const TimeInterval*);
void portIdentity_display(const PortIdentity*);
void clockQuality_display (const ClockQuality*);
void PTPText_display(const PTPText*, const PtpClock*);
void iFaceName_display(const Octet*);
void unicast_display(const Octet*);
const char *portState_getName(Enumeration8 portState);
void timestamp_display(const Timestamp * timestamp);

void displayCounters(const PtpClock*);
void displayStatistics(const PtpClock*);

void msgHeader_display(const MsgHeader*);
void msgAnnounce_display(const MsgAnnounce*);
void msgSync_display(const MsgSync *sync);
void msgFollowUp_display(const MsgFollowUp*);
void msgPDelayReq_display(const MsgPDelayReq*);
void msgDelayReq_display(const MsgDelayReq * req);
void msgDelayResp_display(const MsgDelayResp * resp);
void msgPDelayResp_display(const MsgPDelayResp * presp);
void msgPDelayRespFollowUp_display(const MsgPDelayRespFollowUp * prespfollow);
void msgManagement_display(const MsgManagement * manage);


void mMSlaveOnly_display(const MMSlaveOnly*, const PtpClock*);
void mMClockDescription_display(const MMClockDescription*, const PtpClock*);
void mMUserDescription_display(const MMUserDescription*, const PtpClock*);
void mMInitialize_display(const MMInitialize*, const PtpClock*);
void mMDefaultDataSet_display(const MMDefaultDataSet*, const PtpClock*);
void mMCurrentDataSet_display(const MMCurrentDataSet*, const PtpClock*);
void mMParentDataSet_display(const MMParentDataSet*, const PtpClock*);
void mMTimePropertiesDataSet_display(const MMTimePropertiesDataSet*, const PtpClock*);
void mMPortDataSet_display(const MMPortDataSet*, const PtpClock*);
void mMPriority1_display(const MMPriority1*, const PtpClock*);
void mMPriority2_display(const MMPriority2*, const PtpClock*);
void mMDomain_display(const MMDomain*, const PtpClock*);
void mMLogAnnounceInterval_display(const MMLogAnnounceInterval*, const PtpClock*);
void mMAnnounceReceiptTimeout_display(const MMAnnounceReceiptTimeout*, const PtpClock*);
void mMLogSyncInterval_display(const MMLogSyncInterval*, const PtpClock*);
void mMVersionNumber_display(const MMVersionNumber*, const PtpClock*);
void mMTime_display(const MMTime*, const PtpClock*);
void mMClockAccuracy_display(const MMClockAccuracy*, const PtpClock*);
void mMUtcProperties_display(const MMUtcProperties*, const PtpClock*);
void mMTraceabilityProperties_display(const MMTraceabilityProperties*, const PtpClock*);
void mMDelayMechanism_display(const MMDelayMechanism*, const PtpClock*);
void mMLogMinPdelayReqInterval_display(const MMLogMinPdelayReqInterval*, const PtpClock*);
void mMErrorStatus_display(const MMErrorStatus*, const PtpClock*);

void clearTime(TimeInternal *time);

char *dump_TimeInternal(const TimeInternal * p);
char *dump_TimeInternal2(const char *st1, const TimeInternal * p1, const char *st2, const TimeInternal * p2);



int snprint_TimeInternal(char *s, int max_len, const TimeInternal * p);


void nano_to_Time(TimeInternal *time, int nano);
int gtTime(const TimeInternal *x, const TimeInternal *b);
void absTime(TimeInternal *time);
int is_Time_close(const TimeInternal *x, const TimeInternal *b, int nanos);
int isTimeInternalNegative(const TimeInternal * p);
double timeInternalToDouble(const TimeInternal * p);
TimeInternal doubleToTimeInternal(const double d);

int check_timestamp_is_fresh2(const TimeInternal * timeA, const TimeInternal * timeB);
int check_timestamp_is_fresh(const TimeInternal * timeA);


void toState(UInteger8,RunTimeOpts*,PtpClock*);

/* helper functions for leap second handling */
float secondsToMidnight(void);
float getPauseAfterMidnight(Integer8 announceInterval);

Boolean respectUtcOffset(RunTimeOpts * rtOpts, PtpClock * ptpClock);

#endif /*PTPD_H_*/
