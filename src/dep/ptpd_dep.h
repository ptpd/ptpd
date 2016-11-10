/**
 * @file   ptpd_dep.h
 *
 * @brief  External definitions for inclusion elsewhere.
 *
 *
 */

#ifndef PTPD_DEP_H_
#define PTPD_DEP_H_

#ifdef RUNTIME_DEBUG
#undef PTPD_DBGV
#define PTPD_DBGV
#endif

 /** \name System messages*/
 /**\{*/


// Syslog ordering. We define extra debug levels above LOG_DEBUG for internal use - but message() doesn't pass these to SysLog

// extended from <sys/syslog.h>
#define LOG_DEBUG1   7
#define LOG_DEBUG2   8
#define LOG_DEBUG3   9
#define LOG_DEBUGV   9


#define EMERGENCY(x, ...) logMessage(LOG_EMERG, x, ##__VA_ARGS__)
#define ALERT(x, ...)     logMessage(LOG_ALERT, x, ##__VA_ARGS__)
#define CRITICAL(x, ...)  logMessage(LOG_CRIT, x, ##__VA_ARGS__)
#define ERROR(x, ...)  logMessage(LOG_ERR, x, ##__VA_ARGS__)
#define PERROR(x, ...)    logMessage(LOG_ERR, x "      (strerror: %m)\n", ##__VA_ARGS__)
#define WARNING(x, ...)   logMessage(LOG_WARNING, x, ##__VA_ARGS__)
#define NOTIFY(x, ...) logMessage(LOG_NOTICE, x, ##__VA_ARGS__)
#define NOTICE(x, ...)    logMessage(LOG_NOTICE, x, ##__VA_ARGS__)
#define INFO(x, ...)   logMessage(LOG_INFO, x, ##__VA_ARGS__)

#define EMERGENCY_LOCAL(x, ...)	EMERGENCY(LOCAL_PREFIX": " x,##__VA_ARGS__)
#define ALERT_LOCAL(x, ...)	ALERT(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define CRITICAL_LOCAL(x, ...)	CRITICAL(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define ERROR_LOCAL(x, ...)	ERROR(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define PERROR_LOCAL(x, ...)	PERROR(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define WARNING_LOCAL(x, ...)	WARNING(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define NOTIFY_LOCAL(x, ...)	NOTIFY(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define NOTIC_LOCALE(x, ...)	NOTICE(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define INFO_LOCAL(x, ...)	INFO(LOCAL_PREFIX": " x, ##__VA_ARGS__)

#define EMERGENCY_LOCAL_ID(o,x, ...)	EMERGENCY(LOCAL_PREFIX".%s: "x,o->id,##__VA_ARGS__)
#define ALERT_LOCAL_ID(o,x, ...)	ALERT(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define CRITICAL_LOCAL_ID(o,x, ...)	CRITICAL(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define ERROR_LOCAL_ID(o,x, ...)	ERROR(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define PERROR_LOCAL_ID(o,x, ...)	PERROR(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define WARNING_LOCAL_ID(o,x, ...)	WARNING(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define NOTIFY_LOCAL_ID(o,x, ...)	NOTIFY(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define NOTICE_LOCAL_ID(o,x, ...)	NOTICE(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define INFO_LOCAL_ID(o,x, ...)		INFO(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)

#if defined(__FILE__) && defined(__LINE__)
#define MARKER INFO("Marker: %s:%d\n", __FILE__, __LINE__)
#else
#define MARKER INFO("Marker\n")
#endif

#include <assert.h>


/*
  list of per-module defines:

./dep/sys.c:#define PRINT_MAC_ADDRESSES
./dep/timer.c:#define US_TIMER_INTERVAL 125000
*/
#define USE_BINDTODEVICE



// enable this line to show debug numbers in nanoseconds instead of microseconds
// #define DEBUG_IN_NS

#define DBG_UNIT_US (1000)
#define DBG_UNIT_NS (1)

#ifdef DEBUG_IN_NS
#define DBG_UNIT DBG_UNIT_NS
#else
#define DBG_UNIT DBG_UNIT_US
#endif




/** \}*/

/** \name Debug messages*/
 /**\{*/

#ifdef PTPD_DBGV
#undef PTPD_DBG
#undef PTPD_DBG2
#define PTPD_DBG
#define PTPD_DBG2

#define DBGV(x, ...) logMessage(LOG_DEBUGV, x, ##__VA_ARGS__)
#define DBGV_LOCAL(x, ...) DBGV(LOCAL_PREFIX": " x,##__VA_ARGS__)
#define DBGV_LOCAL_ID(o,x, ...)	DBGV(LOCAL_PREFIX".%s:"x,o->id,##__VA_ARGS__)
#else
#define DBGV(x, ...)
#define DBGV_LOCAL(x, ...)
#define DBGV_LOCAL_ID(x, ...)
#endif

/*
 * new debug level DBG2:
 * this is above DBG(), but below DBGV() (to avoid changing hundreds of lines)
 */


#ifdef PTPD_DBG2
#undef PTPD_DBG
#define PTPD_DBG
#define DBG2(x, ...) logMessage(LOG_DEBUG2, x, ##__VA_ARGS__)
#define DBG2_LOCAL(x, ...) DBG2(LOCAL_PREFIX": " x,##__VA_ARGS__)
#define DBG2_LOCAL_ID(o,x, ...)	DBG2(LOCAL_PREFIX".%s:"x,o->id,##__VA_ARGS__)

#else

#define DBG2(x, ...)
#define DBG2_LOCAL(x, ...)
#define DBG2_LOCAL_ID(x, ...)
#endif

#ifdef PTPD_DBG
#define DBG(x, ...) logMessage(LOG_DEBUG, x, ##__VA_ARGS__)
#define DBG_LOCAL(x, ...) DBG(LOCAL_PREFIX": " x,##__VA_ARGS__)
#define DBG_LOCAL_ID(o,x, ...)	DBG(LOCAL_PREFIX".%s:"x,o->id,##__VA_ARGS__)
#else
#define DBG(x, ...)
#define DBG_LOCAL(x, ...)
#define DBG_LOCAL_ID(x, ...)
#endif

/** \}*/

/** \name Endian corrections*/
 /**\{*/

#if defined(PTPD_MSBF)
#define shift8(x,y)   ( (x) << ((3-y)<<3) )
#define shift16(x,y)  ( (x) << ((1-y)<<4) )
#elif defined(PTPD_LSBF)
#define shift8(x,y)   ( (x) << ((y)<<3) )
#define shift16(x,y)  ( (x) << ((y)<<4) )
#endif

#define flip16(x) htons(x)
#define flip32(x) htonl(x)

/* i don't know any target platforms that do not have htons and htonl,
   but here are generic funtions just in case */
/*
#if defined(PTPD_MSBF)
#define flip16(x) (x)
#define flip32(x) (x)
#elif defined(PTPD_LSBF)
static inline Integer16 flip16(Integer16 x)
{
   return (((x) >> 8) & 0x00ff) | (((x) << 8) & 0xff00);
}

static inline Integer32 flip32(x)
{
  return (((x) >> 24) & 0x000000ff) | (((x) >> 8 ) & 0x0000ff00) |
         (((x) << 8 ) & 0x00ff0000) | (((x) << 24) & 0xff000000);
}
#endif
*/

/** \}*/


/** \name Bit array manipulations*/
 /**\{*/

#define getFlag(x,y)  !!( *(UInteger8*)((x)+((y)<8?1:0)) &   (1<<((y)<8?(y):(y)-8)) )
#define setFlag(x,y)    ( *(UInteger8*)((x)+((y)<8?1:0)) |=   1<<((y)<8?(y):(y)-8)  )
#define clearFlag(x,y)  ( *(UInteger8*)((x)+((y)<8?1:0)) &= ~(1<<((y)<8?(y):(y)-8)) )
/** \}*/

#define DEFAULT_TOKEN_DELIM ", ;\t"

/*
 * foreach loop across substrings from var, delimited by delim, placing
 * each token in targetvar on iteration, using id variable name prefix
 * to allow nesting (each loop uses an individual set of variables)
 */
#define foreach_token_begin(id, var, targetvar, delim) {\
    char* id_stash; \
    char* id_text_; \
    char* id_text__; \
    char* targetvar; \
    id_text_=strdup(var); \
    for(id_text__ = id_text_;; id_text__=NULL) { \
	targetvar = strtok_r(id_text__, delim, &id_stash); \
	if(targetvar==NULL) break;

#define foreach_token_end(id) } \
    if(id_text_ != NULL) { \
	free(id_text_); \
    }\
}

/** \name msg.c
 *-Pack and unpack PTP messages */
 /**\{*/

void msgUnpackHeader(Octet * buf,MsgHeader*);
void msgUnpackAnnounce (Octet * buf,MsgAnnounce*);
void msgUnpackSync(Octet * buf,MsgSync*);
void msgUnpackFollowUp(Octet * buf,MsgFollowUp*);
void msgUnpackDelayReq(Octet * buf, MsgDelayReq * delayreq);
void msgUnpackDelayResp(Octet * buf,MsgDelayResp *);
void msgUnpackPdelayReq(Octet * buf,MsgPdelayReq*);
void msgUnpackPdelayResp(Octet * buf,MsgPdelayResp*);
void msgUnpackPdelayRespFollowUp(Octet * buf,MsgPdelayRespFollowUp*);
Boolean msgUnpackManagement(Octet * buf,MsgManagement*, MsgHeader*, PtpClock *ptpClock, const int tlvOffset);
Boolean msgUnpackSignaling(Octet * buf,MsgSignaling*, MsgHeader*, PtpClock *ptpClock, const int tlvOffset);
void msgPackHeader(Octet * buf,PtpClock*);
#ifndef PTPD_SLAVE_ONLY
void msgPackAnnounce(Octet * buf, UInteger16, Timestamp*, PtpClock*);
void msgPackSync(Octet * buf, UInteger16, Timestamp*, PtpClock*);
#endif /* PTPD_SLAVE_ONLY */
void msgPackFollowUp(Octet * buf,Timestamp*,PtpClock*, const UInteger16);
void msgPackDelayReq(Octet * buf,Timestamp *,PtpClock *);
void msgPackDelayResp(Octet * buf,MsgHeader *,Timestamp *,PtpClock *);
void msgPackPdelayReq(Octet * buf,Timestamp*,PtpClock*);
void msgPackPdelayResp(Octet * buf,MsgHeader*,Timestamp*,PtpClock*);
void msgPackPdelayRespFollowUp(Octet * buf,MsgHeader*,Timestamp*,PtpClock*, const UInteger16);
void msgPackManagement(Octet * buf,MsgManagement*,PtpClock*);
void msgPackSignaling(Octet * buf,MsgSignaling*,PtpClock*);
void msgPackManagementRespAck(Octet *,MsgManagement*,PtpClock*);
void msgPackManagementTLV(Octet *,MsgManagement*, PtpClock*);
void msgPackSignalingTLV(Octet *,MsgSignaling*, PtpClock*);
void msgPackManagementErrorStatusTLV(Octet *,MsgManagement*,PtpClock*);

void freeMMErrorStatusTLV(ManagementTLV*);
void freeMMTLV(ManagementTLV*);

void msgDump(PtpClock *ptpClock);
void msgDebugHeader(MsgHeader *header);
void msgDebugSync(MsgSync *sync);
void msgDebugAnnounce(MsgAnnounce *announce);
void msgDebugDelayReq(MsgDelayReq *req);
void msgDebugFollowUp(MsgFollowUp *follow);
void msgDebugDelayResp(MsgDelayResp *resp);
void msgDebugManagement(MsgManagement *manage);

void copyClockIdentity( ClockIdentity dest, ClockIdentity src);
void copyPortIdentity( PortIdentity * dest, PortIdentity * src);

void unpackMsgSignaling(Octet *, MsgSignaling*, PtpClock*);
void packMsgSignaling(MsgSignaling*, Octet *);
void unpackSignalingTLV(Octet*, MsgSignaling*, PtpClock*);
void packSignalingTLV(SignalingTLV*, Octet*);
void freeSignalingTLV(MsgSignaling*);

void unpackMsgManagement(Octet *, MsgManagement*, PtpClock*);
void packMsgManagement(MsgManagement*, Octet *);
void unpackManagementTLV(Octet*, int, MsgManagement*, PtpClock*);
void packManagementTLV(ManagementTLV*, Octet*);
void freeManagementTLV(MsgManagement*);

int unpackMMClockDescription( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMClockDescription( MsgManagement*, Octet*);
void freeMMClockDescription( MMClockDescription*);
int unpackMMUserDescription( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMUserDescription( MsgManagement*, Octet*);
void freeMMUserDescription( MMUserDescription*);
int unpackMMErrorStatus( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMErrorStatus( MsgManagement*, Octet*);
void freeMMErrorStatus( MMErrorStatus*);
int unpackMMInitialize( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMInitialize( MsgManagement*, Octet*);
int unpackMMDefaultDataSet( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMDefaultDataSet( MsgManagement*, Octet*);
int unpackMMCurrentDataSet( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMCurrentDataSet( MsgManagement*, Octet*);
int unpackMMParentDataSet( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMParentDataSet( MsgManagement*, Octet*);
int unpackMMTimePropertiesDataSet( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMTimePropertiesDataSet( MsgManagement*, Octet*);
int unpackMMPortDataSet( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMPortDataSet( MsgManagement*, Octet*);
int unpackMMPriority1( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMPriority1( MsgManagement*, Octet*);
int unpackMMPriority2( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMPriority2( MsgManagement*, Octet*);
int unpackMMDomain( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMDomain( MsgManagement*, Octet*);
int unpackMMSlaveOnly( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMSlaveOnly( MsgManagement*, Octet* );
int unpackMMLogAnnounceInterval( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMLogAnnounceInterval( MsgManagement*, Octet*);
int unpackMMAnnounceReceiptTimeout( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMAnnounceReceiptTimeout( MsgManagement*, Octet*);
int unpackMMLogSyncInterval( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMLogSyncInterval( MsgManagement*, Octet*);
int unpackMMVersionNumber( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMVersionNumber( MsgManagement*, Octet*);
int unpackMMTime( Octet* buf, int, MsgManagement*, PtpClock * );
UInteger16 packMMTime( MsgManagement*, Octet*);
int unpackMMClockAccuracy( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMClockAccuracy( MsgManagement*, Octet*);
int unpackMMUtcProperties( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMUtcProperties( MsgManagement*, Octet*);
int unpackMMTraceabilityProperties( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMTraceabilityProperties( MsgManagement*, Octet*);
int unpackMMTimescaleProperties( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMTimescaleProperties( MsgManagement*, Octet*);
int unpackMMUnicastNegotiationEnable( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMUnicastNegotiationEnable( MsgManagement*, Octet*);
int unpackMMDelayMechanism( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMDelayMechanism( MsgManagement*, Octet*);
int unpackMMLogMinPdelayReqInterval( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMLogMinPdelayReqInterval( MsgManagement*, Octet*);

/* Signaling TLV packing / unpacking functions */
void unpackSMRequestUnicastTransmission( Octet* buf, MsgSignaling*, PtpClock* );
UInteger16 packSMRequestUnicastTransmission( MsgSignaling*, Octet*);
void unpackSMGrantUnicastTransmission( Octet* buf, MsgSignaling*, PtpClock* );
UInteger16 packSMGrantUnicastTransmission( MsgSignaling*, Octet*);
void unpackSMCancelUnicastTransmission( Octet* buf, MsgSignaling*, PtpClock* );
UInteger16 packSMCancelUnicastTransmission( MsgSignaling*, Octet*);
void unpackSMAcknowledgeCancelUnicastTransmission( Octet* buf, MsgSignaling*, PtpClock* );
UInteger16 packSMAcknowledgeCancelUnicastTransmission( MsgSignaling*, Octet*);

void unpackPortAddress( Octet* buf, PortAddress*, PtpClock*);
void packPortAddress( PortAddress*, Octet*);
void freePortAddress( PortAddress*);
void unpackPTPText( Octet* buf, PTPText*, PtpClock*);
void packPTPText( PTPText*, Octet*);
void freePTPText( PTPText*);
void unpackPhysicalAddress( Octet* buf, PhysicalAddress*, PtpClock*);
void packPhysicalAddress( PhysicalAddress*, Octet*);
void freePhysicalAddress( PhysicalAddress*);
void unpackClockIdentity( Octet* buf, ClockIdentity *c, PtpClock*);
void packClockIdentity( ClockIdentity *c, Octet* buf);
void freeClockIdentity( ClockIdentity *c);
void unpackClockQuality( Octet* buf, ClockQuality *c, PtpClock*);
void packClockQuality( ClockQuality *c, Octet* buf);
void freeClockQuality( ClockQuality *c);
void unpackTimeInterval( Octet* buf, TimeInterval *t, PtpClock*);
void packTimeInterval( TimeInterval *t, Octet* buf);
void freeTimeInterval( TimeInterval *t);
void unpackPortIdentity( Octet* buf, PortIdentity *p, PtpClock*);
void packPortIdentity( PortIdentity *p, Octet*  buf);
void freePortIdentity( PortIdentity *p);
void unpackTimestamp( Octet* buf, Timestamp *t, PtpClock*);
void packTimestamp( Timestamp *t, Octet* buf);
void freeTimestamp( Timestamp *t);
UInteger16 msgPackManagementResponse(Octet * buf,MsgHeader*,MsgManagement*,PtpClock*);
/** \}*/

/** \name net.c (Unix API dependent)
 * -Init network stuff, send and receive datas*/
 /**\{*/

Boolean testInterface(char* ifaceName, const RunTimeOpts* rtOpts);
Boolean netInit(NetPath*,RunTimeOpts*,PtpClock*);
Boolean netShutdown(NetPath*);
int netSelect(TimeInternal*,NetPath*,fd_set*);
ssize_t netRecvEvent(Octet*,TimeInternal*,NetPath*,int);
ssize_t netRecvGeneral(Octet*,NetPath*);
ssize_t netSendEvent(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32,TimeInternal*);
ssize_t netSendGeneral(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32 );
ssize_t netSendPeerGeneral(Octet*,UInteger16,NetPath*,const RunTimeOpts*, Integer32);
ssize_t netSendPeerEvent(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32,TimeInternal*);
Boolean netRefreshIGMP(NetPath *, const RunTimeOpts *, PtpClock *);
Boolean hostLookup(const char* hostname, Integer32* addr);

/** \}*/

#if defined PTPD_SNMP
/** \name snmp.c (SNMP subsystem)
 * -Handle SNMP subsystem*/
 /**\{*/

void snmpInit(RunTimeOpts *, PtpClock *);
void snmpShutdown();
void eventHandler_snmp(AlarmEntry *alarm);
void alarmHandler_snmp(AlarmEntry *alarm);

//void sendNotif(int eventType, PtpEventData *eventData);


/** \}*/
#endif

/** \name servo.c
 * -Clock servo*/
 /**\{*/

void initClock(const RunTimeOpts*,PtpClock*);
void updatePeerDelay (one_way_delay_filter*, const RunTimeOpts*,PtpClock*,TimeInternal*,Boolean);
void updateDelay (one_way_delay_filter*, const RunTimeOpts*, PtpClock*,TimeInternal*);
void updateOffset(TimeInternal*,TimeInternal*,
  offset_from_master_filter*,const RunTimeOpts*,PtpClock*,TimeInternal*);
void checkOffset(const RunTimeOpts*, PtpClock*);
void updateClock(const RunTimeOpts*,PtpClock*);
void stepClock(const RunTimeOpts * rtOpts, PtpClock * ptpClock);

/** \}*/

/** \name startup.c (Unix API dependent)
 * -Handle with runtime options*/
 /**\{*/
int setCpuAffinity(int cpu);
int logToFile(RunTimeOpts * rtOpts);
int recordToFile(RunTimeOpts * rtOpts);
PtpClock * ptpdStartup(int,char**,Integer16*,RunTimeOpts*);

void ptpdShutdown(PtpClock * ptpClock);
void checkSignals(RunTimeOpts * rtOpts, PtpClock * ptpClock);
void restartSubsystems(RunTimeOpts *rtOpts, PtpClock *ptpClock);
void applyConfig(dictionary *baseConfig, RunTimeOpts *rtOpts, PtpClock *ptpClock);

void enable_runtime_debug(void );
void disable_runtime_debug(void );

void ntpSetup(RunTimeOpts *rtOpts, PtpClock *ptpClock);

#define D_ON      do { enable_runtime_debug();  } while (0);
#define D_OFF     do { disable_runtime_debug( ); } while (0);


/** \}*/

/** \name sys.c (Unix API dependent)
 * -Manage timing system API*/
 /**\{*/

/* new debug methods to debug time variables */
char *time2st(const TimeInternal * p);
void DBG_time(const char *name, const TimeInternal  p);


void logMessage(int priority, const char *format, ...);
void updateLogSize(LogFileHandler* handler);
Boolean maintainLogSize(LogFileHandler* handler);
int restartLog(LogFileHandler* handler, Boolean quiet);
void restartLogging(RunTimeOpts* rtOpts);
void stopLogging(RunTimeOpts* rtOpts);
void logStatistics(PtpClock *ptpClock);
void periodicUpdate(const RunTimeOpts *rtOpts, PtpClock *ptpClock);
void displayStatus(PtpClock *ptpClock, const char *prefixMessage);
void displayPortIdentity(PortIdentity *port, const char *prefixMessage);
int snprint_PortIdentity(char *s, int max_len, const PortIdentity *id);
Boolean nanoSleep(TimeInternal*);
void getTime(TimeInternal*);
void getTimeMonotonic(TimeInternal*);
void setTime(TimeInternal*);
#ifdef linux
void setRtc(TimeInternal *);
#endif /* linux */
double getRand(void);
int lockFile(int fd);
int checkLockStatus(int fd, short lockType, int *lockPid);
int checkFileLockable(const char *fileName, int *lockPid);
Boolean checkOtherLocks(RunTimeOpts *rtOpts);

void recordSync(UInteger16 sequenceId, TimeInternal * time);

void adjFreq_wrapper(const RunTimeOpts * rtOpts, PtpClock * ptpClock, double adj);

Boolean adjFreq(double);
double getAdjFreq(void);

#ifdef HAVE_SYS_TIMEX_H
void informClockSource(PtpClock* ptpClock);

/* Helper function to manage ntpadjtime / adjtimex flags */
void setTimexFlags(int flags, Boolean quiet);
void unsetTimexFlags(int flags, Boolean quiet);
int getTimexFlags(void);
Boolean checkTimexFlags(int flags);

#if defined(MOD_TAI) &&  NTP_API == 4
void setKernelUtcOffset(int utc_offset);
Boolean getKernelUtcOffset(int *utc_offset);
#endif /* MOD_TAI */

#endif /* HAVE_SYS_TIMEX_H */

/* Observed drift save / recovery functions */
void restoreDrift(PtpClock * ptpClock, const RunTimeOpts * rtOpts, Boolean quiet);
void saveDrift(PtpClock * ptpClock, const RunTimeOpts * rtOpts, Boolean quiet);

int parseLeapFile(char * path, LeapSecondInfo *info);

void
resetWarnings(const RunTimeOpts * rtOpts, PtpClock * ptpClock);

void setupPIservo(PIservo* servo, const RunTimeOpts* rtOpts);
void resetPIservo(PIservo* servo);
double runPIservo(PIservo* servo, const Integer32 input);

#ifdef PTPD_STATISTICS
void updatePtpEngineStats (PtpClock* ptpClock, const RunTimeOpts* rtOpts);
#endif /* PTPD_STATISTICS */

void writeStatusFile(PtpClock *ptpClock, const RunTimeOpts *rtOpts, Boolean quiet);
void updateXtmp (TimeInternal oldTime, TimeInternal newTime);


#endif /*PTPD_DEP_H_*/
