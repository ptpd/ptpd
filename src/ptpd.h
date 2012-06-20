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

#include "constants.h"
#include "limits.h"
#include "dep/constants_dep.h"
#include "dep/datatypes_dep.h"
#include "datatypes.h"
#include "dep/ptpd_dep.h"

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
void fromInternalTime(TimeInternal*,Timestamp*);

/**
 * \brief Convert Timestamp to TimeInternal structure (defined by the spec)
 */
void toInternalTime(TimeInternal*,Timestamp*);

void ts_to_InternalTime(struct timespec *, TimeInternal *);
void tv_to_InternalTime(struct timeval  *, TimeInternal *);




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
UInteger8 bmc(ForeignMasterRecord*,RunTimeOpts*,PtpClock*);

/**
 * \brief When recommended state is Master, copy local data into parent and grandmaster dataset
 */
void m1(RunTimeOpts *, PtpClock*);

/**
 * \brief When recommended state is Slave, copy dataset of master into parent and grandmaster dataset
 */
void s1(MsgHeader*,MsgAnnounce*,PtpClock*, RunTimeOpts *);


void p1(PtpClock *ptpClock, RunTimeOpts *rtOpts);

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
void handleMMTime(MsgManagement*, MsgManagement*, PtpClock*);
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
void displayRunTimeOpts(RunTimeOpts*);
void displayDefault (PtpClock*);
void displayCurrent (PtpClock*);
void displayParent (PtpClock*);
void displayGlobal (PtpClock*);
void displayPort (PtpClock*);
void displayForeignMaster (PtpClock*);
void displayOthers (PtpClock*);
void displayBuffer (PtpClock*);
void displayPtpClock (PtpClock*);
void timeInternal_display(TimeInternal*);
void clockIdentity_display(ClockIdentity);
void netPath_display(NetPath*);
void intervalTimer_display(IntervalTimer*);
void integer64_display (Integer64*);
void timeInterval_display(TimeInterval*);
void portIdentity_display(PortIdentity*);
void clockQuality_display (ClockQuality*);
void PTPText_display(PTPText*, PtpClock*);
void iFaceName_display(Octet*);
void unicast_display(Octet*);

void msgHeader_display(MsgHeader*);
void msgAnnounce_display(MsgAnnounce*);
void msgSync_display(MsgSync *sync);
void msgFollowUp_display(MsgFollowUp*);
void msgPDelayReq_display(MsgPDelayReq*);
void msgDelayReq_display(MsgDelayReq * req);
void msgDelayResp_display(MsgDelayResp * resp);
void msgPDelayResp_display(MsgPDelayResp * presp);
void msgPDelayRespFollowUp_display(MsgPDelayRespFollowUp * prespfollow);
void msgManagement_display(MsgManagement * manage);


void mMSlaveOnly_display(MMSlaveOnly*, PtpClock*);
void mMClockDescription_display(MMClockDescription*, PtpClock*);
void mMUserDescription_display(MMUserDescription*, PtpClock*);
void mMInitialize_display(MMInitialize*, PtpClock*);
void mMDefaultDataSet_display(MMDefaultDataSet*, PtpClock*);
void mMCurrentDataSet_display(MMCurrentDataSet*, PtpClock*);
void mMParentDataSet_display(MMParentDataSet*, PtpClock*);
void mMTimePropertiesDataSet_display(MMTimePropertiesDataSet*, PtpClock*);
void mMPortDataSet_display(MMPortDataSet*, PtpClock*);
void mMPriority1_display(MMPriority1*, PtpClock*);
void mMPriority2_display(MMPriority2*, PtpClock*);
void mMDomain_display(MMDomain*, PtpClock*);
void mMLogAnnounceInterval_display(MMLogAnnounceInterval*, PtpClock*);
void mMAnnounceReceiptTimeout_display(MMAnnounceReceiptTimeout*, PtpClock*);
void mMLogSyncInterval_display(MMLogSyncInterval*, PtpClock*);
void mMVersionNumber_display(MMVersionNumber*, PtpClock*);
void mMTime_display(MMTime*, PtpClock*);
void mMClockAccuracy_display(MMClockAccuracy*, PtpClock*);
void mMUtcProperties_display(MMUtcProperties*, PtpClock*);
void mMTraceabilityProperties_display(MMTraceabilityProperties*, PtpClock*);
void mMDelayMechanism_display(MMDelayMechanism*, PtpClock*);
void mMLogMinPdelayReqInterval_display(MMLogMinPdelayReqInterval*, PtpClock*);
void mMErrorStatus_display(MMErrorStatus*, PtpClock*);

void clearTime(TimeInternal *time);
int isTimeInternalNegative(const TimeInternal * p);

char *dump_TimeInternal(const TimeInternal * p);
char *dump_TimeInternal2(const char *st1, const TimeInternal * p1, const char *st2, const TimeInternal * p2);



int snprint_TimeInternal(char *s, int max_len, const TimeInternal * p);


void nano_to_Time(TimeInternal *time, int nano);
int gtTime(TimeInternal *x, TimeInternal *b);
void absTime(TimeInternal *time);
int is_Time_close(TimeInternal *x, TimeInternal *b, int nanos);
int isTimeInternalNegative(const TimeInternal * p);



int check_timestamp_is_fresh2(TimeInternal * timeA, TimeInternal * timeB);
int check_timestamp_is_fresh(TimeInternal * timeA);


void toState(UInteger8,RunTimeOpts*,PtpClock*);

/* helper functions for leap second handling */
float secondsToMidnight(void);
float getPauseAfterMidnight(Integer8 announceInterval);

#endif /*PTPD_H_*/
