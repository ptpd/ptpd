/*-
 * Copyright (c) 2011      George V. Neville-Neil, Steven Kreuzer,
 *                         Martin Burnicki, Gael Mace, Alexandre Van Kempen,
 *                         National Instruments.
 * Copyright (c) 2009-2010 George V. Neville-Neil, Steven Kreuzer,
 *                         Martin Burnicki, Gael Mace, Alexandre Van Kempen
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
 *
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

#ifdef DBG_SIGUSR2_CHANGE_DOMAIN
#ifdef DBG_SIGUSR2_CHANGE_DEBUG

#error "Cannot compile with both DBG_SIGUSR2_CHANGE_DOMAIN and DBG_SIGUSR2_CHANGE_DEBUG"

#endif
#endif




 /** \name System messages*/
 /**\{*/


// Syslog ordering. We define extra debug levels above LOG_DEBUG for internal use - but message() doesn't pass these to SysLog

// extended from <sys/syslog.h>
#define LOG_DEBUG1   7
#define LOG_DEBUG2   8
#define LOG_DEBUG3   9
#define LOG_DEBUGV   9


#define EMERGENCY(x, ...) message(LOG_EMERG, x, ##__VA_ARGS__)
#define ALERT(x, ...)     message(LOG_ALERT, x, ##__VA_ARGS__)
#define CRITICAL(x, ...)  message(LOG_CRIT, x, ##__VA_ARGS__)
#define ERROR(x, ...)  message(LOG_ERR, x, ##__VA_ARGS__)
#define PERROR(x, ...)    message(LOG_ERR, x "      (strerror: %m)\n", ##__VA_ARGS__)
#define WARNING(x, ...)   message(LOG_WARNING, x, ##__VA_ARGS__)
#define NOTIFY(x, ...) message(LOG_NOTICE, x, ##__VA_ARGS__)
#define NOTICE(x, ...)    message(LOG_NOTICE, x, ##__VA_ARGS__)
#define INFO(x, ...)   message(LOG_INFO, x, ##__VA_ARGS__)



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

#define DBGV(x, ...) message(LOG_DEBUGV, x, ##__VA_ARGS__)
#else
#define DBGV(x, ...)
#endif

/*
 * new debug level DBG2:
 * this is above DBG(), but below DBGV() (to avoid changing hundreds of lines)
 */


#ifdef PTPD_DBG2
#undef PTPD_DBG
#define PTPD_DBG
#define DBG2(x, ...) message(LOG_DEBUG2, x, ##__VA_ARGS__)
#else
#define DBG2(x, ...)
#endif

#ifdef PTPD_DBG
#define DBG(x, ...) message(LOG_DEBUG, x, ##__VA_ARGS__)
#else
#define DBG(x, ...)
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

/** \name msg.c
 *-Pack and unpack PTP messages */
 /**\{*/

void msgUnpackHeader(Octet * buf,MsgHeader*);
void msgUnpackAnnounce (Octet * buf,MsgAnnounce*);
void msgUnpackSync(Octet * buf,MsgSync*);
void msgUnpackFollowUp(Octet * buf,MsgFollowUp*);
void msgUnpackDelayReq(Octet * buf, MsgDelayReq * delayreq);
void msgUnpackDelayResp(Octet * buf,MsgDelayResp *);
void msgUnpackPDelayReq(Octet * buf,MsgPDelayReq*);
void msgUnpackPDelayResp(Octet * buf,MsgPDelayResp*);
void msgUnpackPDelayRespFollowUp(Octet * buf,MsgPDelayRespFollowUp*);
void msgUnpackManagement(Octet * buf,MsgManagement*, MsgHeader*);
void msgPackHeader(Octet * buf,PtpClock*);
void msgPackAnnounce(Octet * buf,PtpClock*);
void msgPackSync(Octet * buf,Timestamp*,PtpClock*);
void msgPackFollowUp(Octet * buf,Timestamp*,PtpClock*);
void msgPackDelayReq(Octet * buf,Timestamp *,PtpClock *);
void msgPackDelayResp(Octet * buf,MsgHeader *,Timestamp *,PtpClock *);
void msgPackPDelayReq(Octet * buf,Timestamp*,PtpClock*);
void msgPackPDelayResp(Octet * buf,MsgHeader*,Timestamp*,PtpClock*);
void msgPackPDelayRespFollowUp(Octet * buf,MsgHeader*,Timestamp*,PtpClock*);
void msgPackManagementRespAck(Octet *,MsgManagement*,PtpClock*);
void msgPackManagementTLV(Octet *,MsgManagement*, PtpClock*);
void msgPackManagement(Octet *,MsgManagement*,PtpClock*);
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

void unpackMsgManagement(Octet *, MsgManagement*);
void packMsgManagement(MsgManagement*, Octet *);
void unpackManagementTLV(Octet*, MsgManagement*);
void packManagementTLV(ManagementTLV*, Octet*);
void freeManagementTLV(MsgManagement*);
void unpackMMClockDescription( Octet* buf, MsgManagement* );
UInteger16 packMMClockDescription( MsgManagement*, Octet*);
void freeMMClockDescription( MMClockDescription*);
void unpackMMUserDescription( Octet* buf, MsgManagement* );
UInteger16 packMMUserDescription( MsgManagement*, Octet*);
void freeMMUserDescription( MMUserDescription*);
void unpackMMErrorStatus( Octet* buf, MsgManagement* );
UInteger16 packMMErrorStatus( MsgManagement*, Octet*);
void freeMMErrorStatus( MMErrorStatus*);
void unpackMMInitialize( Octet* buf, MsgManagement* );
UInteger16 packMMInitialize( MsgManagement*, Octet*);
void unpackMMDefaultDataSet( Octet* buf, MsgManagement* );
UInteger16 packMMDefaultDataSet( MsgManagement*, Octet*);
void unpackMMCurrentDataSet( Octet* buf, MsgManagement* );
UInteger16 packMMCurrentDataSet( MsgManagement*, Octet*);
void unpackMMParentDataSet( Octet* buf, MsgManagement* );
UInteger16 packMMParentDataSet( MsgManagement*, Octet*);
void unpackMMTimePropertiesDataSet( Octet* buf, MsgManagement* );
UInteger16 packMMTimePropertiesDataSet( MsgManagement*, Octet*);
void unpackMMPortDataSet( Octet* buf, MsgManagement* );
UInteger16 packMMPortDataSet( MsgManagement*, Octet*);
void unpackMMPriority1( Octet* buf, MsgManagement* );
UInteger16 packMMPriority1( MsgManagement*, Octet*);
void unpackMMPriority2( Octet* buf, MsgManagement* );
UInteger16 packMMPriority2( MsgManagement*, Octet*);
void unpackMMDomain( Octet* buf, MsgManagement* );
UInteger16 packMMDomain( MsgManagement*, Octet*);
void unpackMMSlaveOnly( Octet* buf, MsgManagement* );
UInteger16 packMMSlaveOnly( MsgManagement*, Octet* );
void unpackMMLogAnnounceInterval( Octet* buf, MsgManagement* );
UInteger16 packMMLogAnnounceInterval( MsgManagement*, Octet*);
void unpackMMAnnounceReceiptTimeout( Octet* buf, MsgManagement* );
UInteger16 packMMAnnounceReceiptTimeout( MsgManagement*, Octet*);
void unpackMMLogSyncInterval( Octet* buf, MsgManagement* );
UInteger16 packMMLogSyncInterval( MsgManagement*, Octet*);
void unpackMMVersionNumber( Octet* buf, MsgManagement* );
UInteger16 packMMVersionNumber( MsgManagement*, Octet*);
void unpackMMTime( Octet* buf, MsgManagement* );
UInteger16 packMMTime( MsgManagement*, Octet*);
void unpackMMClockAccuracy( Octet* buf, MsgManagement* );
UInteger16 packMMClockAccuracy( MsgManagement*, Octet*);
void unpackMMUtcProperties( Octet* buf, MsgManagement* );
UInteger16 packMMUtcProperties( MsgManagement*, Octet*);
void unpackMMTraceabilityProperties( Octet* buf, MsgManagement* );
UInteger16 packMMTraceabilityProperties( MsgManagement*, Octet*);
void unpackMMDelayMechanism( Octet* buf, MsgManagement* );
UInteger16 packMMDelayMechanism( MsgManagement*, Octet*);
void unpackMMLogMinPdelayReqInterval( Octet* buf, MsgManagement* );
UInteger16 packMMLogMinPdelayReqInterval( MsgManagement*, Octet*);
void unpackMMErrorStatus( Octet* buf, MsgManagement* );
UInteger16 packMMErrorStatus( MsgManagement*, Octet*);


void unpackPortAddress( Octet* buf, PortAddress*);
void packPortAddress( PortAddress*, Octet*);
void freePortAddress( PortAddress*);
void unpackPTPText( Octet* buf, PTPText*);
void packPTPText( PTPText*, Octet*);
void freePTPText( PTPText*);
void unpackPhysicalAddress( Octet* buf, PhysicalAddress*);
void packPhysicalAddress( PhysicalAddress*, Octet*);
void freePhysicalAddress( PhysicalAddress*);
void unpackClockIdentity( Octet* buf, ClockIdentity *c);
void packClockIdentity( ClockIdentity *c, Octet* buf);
void freeClockIdentity( ClockIdentity *c);
void unpackClockQuality( Octet* buf, ClockQuality *c);
void packClockQuality( ClockQuality *c, Octet* buf);
void freeClockQuality( ClockQuality *c);
void unpackTimeInterval( Octet* buf, TimeInterval *t);
void packTimeInterval( TimeInterval *t, Octet* buf);
void freeTimeInterval( TimeInterval *t);
void unpackPortIdentity( Octet* buf, PortIdentity *p);
void packPortIdentity( PortIdentity *p, Octet*  buf);
void freePortIdentity( PortIdentity *p);
void unpackTimestamp( Octet* buf, Timestamp *t);
void packTimestamp( Timestamp *t, Octet* buf);
void freeTimestamp( Timestamp *t);
/** \}*/

/** \name net.c (Unix API dependent)
 * -Init network stuff, send and receive datas*/
 /**\{*/

Boolean netInit(NetPath*,RunTimeOpts*,PtpClock*);
Boolean netShutdown(NetPath*);
int netSelect(TimeInternal*,NetPath*);
ssize_t netRecvEvent(Octet*,TimeInternal*,NetPath*);
ssize_t netRecvGeneral(Octet*,TimeInternal*,NetPath*);
ssize_t netSendEvent(Octet*,UInteger16,NetPath*, Integer32 );
ssize_t netSendGeneral(Octet*,UInteger16,NetPath*, Integer32 );
ssize_t netSendPeerGeneral(Octet*,UInteger16,NetPath*);
ssize_t netSendPeerEvent(Octet*,UInteger16,NetPath*);

Boolean netRefreshIGMP(NetPath *, RunTimeOpts *, PtpClock *);


/** \}*/

/** \name servo.c
 * -Clock servo*/
 /**\{*/

void initClock(RunTimeOpts*,PtpClock*);
void updatePeerDelay (one_way_delay_filter*, RunTimeOpts*,PtpClock*,TimeInternal*,Boolean);
void updateDelay (one_way_delay_filter*, RunTimeOpts*, PtpClock*,TimeInternal*);
void updateOffset(TimeInternal*,TimeInternal*,
  offset_from_master_filter*,RunTimeOpts*,PtpClock*,TimeInternal*);
void updateClock(RunTimeOpts*,PtpClock*);

void servo_perform_clock_step(RunTimeOpts * rtOpts, PtpClock * ptpClock);

/** \}*/

/** \name startup.c (Unix API dependent)
 * -Handle with runtime options*/
 /**\{*/
int logToFile(RunTimeOpts * rtOpts);
int recordToFile(RunTimeOpts * rtOpts);
PtpClock * ptpdStartup(int,char**,Integer16*,RunTimeOpts*);
void ptpdShutdown();

void check_signals(RunTimeOpts * rtOpts, PtpClock * ptpClock);

void enable_runtime_debug(void );
void disable_runtime_debug(void );

#define D_ON      do { enable_runtime_debug();  } while (0);
#define D_OFF     do { disable_runtime_debug( ); } while (0);


/** \}*/

/** \name sys.c (Unix API dependent)
 * -Manage timing system API*/
 /**\{*/

/* new debug methods to debug time variables */
char *time2st(const TimeInternal * p);
void DBG_time(const char *name, const TimeInternal  p);


void message(int priority, const char *format, ...);
void displayStats(RunTimeOpts *rtOpts, PtpClock *ptpClock);
Boolean nanoSleep(TimeInternal*);
void getTime(TimeInternal*);
void setTime(TimeInternal*);
double getRand(void);
Boolean adjFreq(Integer32);

void recordSync(RunTimeOpts * rtOpts, UInteger16 sequenceId, TimeInternal * time);

#if defined(__APPLE__)
void 	adjTime(Integer32);
#endif /* __APPLE__ */

/** \}*/

/** \name timer.c (Unix API dependent)
 * -Handle with timers*/
 /**\{*/
void initTimer(void);
void timerUpdate(IntervalTimer*);
void timerStop(UInteger16,IntervalTimer*);
//void timerStart(UInteger16,UInteger16,IntervalTimer*);

/* R135 patch: we went back to floating point periods (for less than 1s )*/
void timerStart(UInteger16 index, float interval, IntervalTimer * itimer);

/* Version with randomized backoff */
void timerStart_random(UInteger16 index, float interval, IntervalTimer * itimer);

Boolean timerExpired(UInteger16,IntervalTimer*);
/** \}*/


/*Test functions*/
void
reset_operator_messages(RunTimeOpts * rtOpts, PtpClock * ptpClock);



#endif /*PTPD_DEP_H_*/
