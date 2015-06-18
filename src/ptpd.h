/**
 * @file   ptpd.h
 * @mainpage Ptpd v2 Documentation
 * @authors Martin Burnicki, Alexandre van Kempen, Steven Kreuzer, 
 *          George Neville-Neil
 * @version 2.0
 * @date   Fri Aug 27 10:22:19 2010
 * 
 * @section implementation Implementation
 * PTPdV2 is not a full implementation of 1588 - 2008 standard.
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
#	ifndef _GNU_SOURCE
#		define _GNU_SOURCE
	#endif /* _GNU_SOURCE */
#endif

#ifdef __sun
#	ifndef _XPG6
#		define _XPG6
#	endif /* _XPG6 */
#	ifndef _XOPEN_SOURCE
#		define _XOPEN_SOURCE 500
#	endif /* _XOPEN_SOURCE */
#	ifndef __EXTENSIONS__
#		define __EXTENSIONS__
#	endif /* __EXTENSIONS */
#endif /* __sun */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */
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
#ifdef HAVE_SYS_TIMEX_H
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

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif /* HAVE_NET_ETHERNET_H */

#include <netinet/in.h>
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#include <netinet/ip.h>
#include <netinet/udp.h>
#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif /* HAVE_NETINET_ETHER_H */

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif /* HAVE_NET_IF_ARP_H*/

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif /* HAVE_NET_IF_H*/

#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif /* HAVE_NETINET_IF_ETHER_H */


#ifdef PTPD_PCAP
#ifdef HAVE_PCAP_PCAP_H
#include <pcap/pcap.h>
#else
/* Cases like RHEL5 and others where only pcap.h exists */
#ifdef HAVE_PCAP_H
#include <pcap.h>
#endif /* HAVE_PCAP_H */
#endif
#endif
#if defined(linux) && defined(HAVE_SCHED_H)
#include <sched.h>
#endif /* linux && HAVE_SCHED_H */

#ifdef HAVE_SYS_CPUSET_H
#include <sys/cpuset.h>
#endif /* HAVE_SYS_CPUSET_H */

#include "constants.h"
#include "limits.h"

/* Disable SO_TIMESTAMPING if configured to do so */
#ifdef PTPD_DISABLE_SOTIMESTAMPING

#ifdef 	SO_TIMESTAMPING

#undef 	SO_TIMESTAMPING

#endif 	/* SO_TIMESTAMPING */

#endif /* PTPD_DISABLE_SOTIMESTAMPING */

#include "dep/ipv4_acl.h"

#include "dep/constants_dep.h"
#include "dep/datatypes_dep.h"

#include "ptp_timers.h"
#include "dep/eventtimer.h"

#include "dep/ntpengine/ntpdcontrol.h"
#include "dep/ntpengine/ntp_isc_md5.h"

#include "timingdomain.h"

#ifdef PTPD_STATISTICS
#include "dep/outlierfilter.h"
#endif

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

#ifdef HAVE_LINUX_RTC_H
#include <linux/rtc.h>
#endif /* HAVE_LINUX_RTC_H */

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
 * This function is used after adding or subtracting TimeInternal
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

/* compare two portIdentTitties */
int cmpPortIdentity(const PortIdentity *a, const PortIdentity *b);
/* check if portIdentity is all zero */
Boolean portIdentityEmpty(PortIdentity *portIdentity);
/* check if portIdentity is all ones */
Boolean portIdentityAllOnes(PortIdentity *portIdentity);

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
void updateDatasets(PtpClock* ptpClock, const RunTimeOpts* rtOpts);
Boolean acceptPortIdentity(PortIdentity thisPort, PortIdentity targetPort);

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
void handleMMTime(MsgManagement*, MsgManagement*, PtpClock*, const RunTimeOpts*);
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

/** \name signaling.c
 * -Signaling message support*/
 /**\{*/
/* signaling.c */
/**
 * \brief Signaling message support
 */
UnicastGrantTable* findUnicastGrants(const PortIdentity* portIdentity, Integer32 TransportAddress, UnicastGrantTable *grantTable, UnicastGrantTable **index, int nodeCount, Boolean update);
void 	initUnicastGrantTable(UnicastGrantTable *grantTable, Enumeration8 delayMechanism, int nodeCount, UnicastDestination *destinations, const RunTimeOpts *rtOpts, PtpClock *ptpClock);
void 	handleSMRequestUnicastTransmission(MsgSignaling*, MsgSignaling*, Integer32, const RunTimeOpts*, PtpClock*);
Boolean handleSMCancelUnicastTransmission(MsgSignaling*, MsgSignaling*, Integer32, PtpClock*);
void 	handleSMAcknowledgeCancelUnicastTransmission(MsgSignaling*, Integer32, PtpClock*);
void 	handleSMGrantUnicastTransmission(MsgSignaling*, Integer32 , UnicastGrantTable*, int, PtpClock*);
Boolean prepareSMRequestUnicastTransmission(MsgSignaling*,UnicastGrantData*,PtpClock*);
Boolean prepareSMCancelUnicastTransmission(MsgSignaling*,UnicastGrantData*,PtpClock*);

void 	requestUnicastTransmission(UnicastGrantData*, UInteger32, const RunTimeOpts*, PtpClock*);
void 	cancelUnicastTransmission(UnicastGrantData*, const RunTimeOpts*, PtpClock*);
void 	cancelNodeGrants(UnicastGrantTable *nodeTable, const RunTimeOpts *rtOpts, PtpClock *ptpClock);
void 	cancelAllGrants(UnicastGrantTable *grantTable, int nodeCount, const RunTimeOpts *rtOpts, PtpClock *ptpClock);

void 	issueSignaling(MsgSignaling *outgoing, Integer32 destination, const RunTimeOpts *rtOpts, PtpClock *ptpClock);
void 	handleSignaling(MsgHeader*, Boolean, Integer32, const RunTimeOpts*,PtpClock*);

void 	refreshUnicastGrants(UnicastGrantTable *grantTable, int nodeCount, const RunTimeOpts *rtOpts, PtpClock *ptpClock);
void 	updateUnicastGrantTable(UnicastGrantTable *grantTable, int nodeCount, const RunTimeOpts *rtOpts);

/*
 * \brief Packing and Unpacking macros
 */
#define DECLARE_PACK( type ) void pack##type( void*, void* );

DECLARE_PACK( NibbleUpper )
DECLARE_PACK( Enumeration4Lower )
DECLARE_PACK( UInteger4Lower )
DECLARE_PACK( UInteger4Upper )
DECLARE_PACK( UInteger16 )
DECLARE_PACK( UInteger8 )
DECLARE_PACK( Octet )
DECLARE_PACK( Integer8 )
DECLARE_PACK( UInteger48 )
DECLARE_PACK( Integer64 )

#define DECLARE_UNPACK( type ) void unpack##type( void*, void*, PtpClock *ptpClock );

DECLARE_UNPACK( Boolean )
DECLARE_UNPACK( Enumeration4Lower )
DECLARE_UNPACK( Enumeration4Upper )
DECLARE_UNPACK( Octet )
DECLARE_UNPACK( UInteger48 )
DECLARE_UNPACK( Integer64 )

/* display.c */
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
const char *getMessageTypeName(Enumeration8 messageType);
void timestamp_display(const Timestamp * timestamp);

void displayCounters(const PtpClock*);
void displayStatistics(const PtpClock*);
void clearCounters(PtpClock *);

void msgHeader_display(const MsgHeader*);
void msgAnnounce_display(const MsgAnnounce*);
void msgSync_display(const MsgSync *sync);
void msgFollowUp_display(const MsgFollowUp*);
void msgPdelayReq_display(const MsgPdelayReq*);
void msgDelayReq_display(const MsgDelayReq * req);
void msgDelayResp_display(const MsgDelayResp * resp);
void msgPdelayResp_display(const MsgPdelayResp * presp);
void msgPdelayRespFollowUp_display(const MsgPdelayRespFollowUp * prespfollow);
void msgManagement_display(const MsgManagement * manage);
void msgSignaling_display(const MsgSignaling * signaling);

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

void sMRequestUnicastTransmission_display(const SMRequestUnicastTransmission*, const PtpClock*);
void sMGrantUnicastTransmission_display(const SMGrantUnicastTransmission*, const PtpClock*);
void sMCancelUnicastTransmission_display(const SMCancelUnicastTransmission*, const PtpClock*);
void sMAcknowledgeCancelUnicastTransmission_display(const SMAcknowledgeCancelUnicastTransmission*, const PtpClock*);

void clearTime(TimeInternal *time);

char *dump_TimeInternal(const TimeInternal * p);
char *dump_TimeInternal2(const char *st1, const TimeInternal * p1, const char *st2, const TimeInternal * p2);
const char * getTimeSourceName(Enumeration8 timeSource);

int snprint_TimeInternal(char *s, int max_len, const TimeInternal * p);

void nano_to_Time(TimeInternal *time, int nano);
int gtTime(const TimeInternal *x, const TimeInternal *b);
void absTime(TimeInternal *time);
int is_Time_close(const TimeInternal *x, const TimeInternal *b, int nanos);
int isTimeInternalNegative(const TimeInternal * p);
double timeInternalToDouble(const TimeInternal * p);
TimeInternal doubleToTimeInternal(const double d);

uint32_t fnvHash(void *input, size_t len, int modulo);

int check_timestamp_is_fresh2(const TimeInternal * timeA, const TimeInternal * timeB);
int check_timestamp_is_fresh(const TimeInternal * timeA);


void toState(UInteger8,const RunTimeOpts*,PtpClock*);

/* helper functions for leap second handling */
double secondsToMidnight(void);
double getPauseAfterMidnight(Integer8 announceInterval, int pausePeriod);

Boolean respectUtcOffset(const RunTimeOpts * rtOpts, PtpClock * ptpClock);

#endif /*PTPD_H_*/
