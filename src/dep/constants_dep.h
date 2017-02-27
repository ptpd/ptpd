
#include <config.h>

/* constants_dep.h */

#ifndef CONSTANTS_DEP_H
#define CONSTANTS_DEP_H

#ifdef HAVE_MACHINE_ENDIAN_H
#	include <machine/endian.h>
#endif /* HAVE_MACHINE_ENDIAN_H */

#ifdef HAVE_ENDIAN_H
#	include <endian.h>
#endif /* HAVE_ENDIAN_H */

#ifdef HAVE_SYS_ISA_DEFS_H
#	include <sys/isa_defs.h>
#endif /* HAVE_SYS_ISA_DEFS_H */

#ifndef HAVE_ADJTIMEX
#ifdef HAVE_NTP_ADJTIME
#define adjtimex ntp_adjtime
#endif /* HAVE_NTP_ADJTIME */
#endif /* HAVE_ADJTIMEX */

# if BYTE_ORDER == LITTLE_ENDIAN || defined(_LITTLE_ENDIAN) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN)
#   define PTPD_LSBF
# elif BYTE_ORDER == BIG_ENDIAN || defined(_BIG_ENDIAN) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
#   define PTPD_MSBF
# endif

#define CLOCK_IDENTITY_LENGTH 8
#define ADJ_FREQ_MAX 500000

#define PACKET_SIZE  300
#define PACKET_BEGIN_UDP (ETHER_HDR_LEN + sizeof(struct ip) + \
	    sizeof(struct udphdr))
#define PACKET_BEGIN_ETHER (ETHER_HDR_LEN)


#define PTP_EVENT_PORT    319
#define PTP_GENERAL_PORT  320

#define PTP_MCAST_GROUP_IPV4		"224.0.1.129"
#define PTP_MCAST_PEER_GROUP_IPV4	"224.0.0.107"

#define PTP_MCAST_GROUP_IPV6     	"FF03:0:0:0:0:0:0:181"
#define PTP_MCAST_PEER_GROUP_IPV6       "FF02:0:0:0:0:0:0:6B"

/* 802.3 Support */

#define PTP_ETHER_DST "01:1b:19:00:00:00"
#define PTP_ETHER_TYPE 0x88f7
#define PTP_ETHER_PEER "01:80:c2:00:00:0E"

#ifdef PTPD_UNICAST_MAX
#define UNICAST_MAX_DESTINATIONS PTPD_UNICAST_MAX
#else
#define UNICAST_MAX_DESTINATIONS 128
#endif /* PTPD_UNICAST_MAX */

/* dummy clock driver designation in preparation for generic clock driver API */
#define DEFAULT_CLOCKDRIVER "kernelclock"
/* default lock file location and mode */
#define DEFAULT_LOCKMODE F_WRLCK
#define DEFAULT_LOCKDIR "/var/run"
#define DEFAULT_LOCKFILE_NAME PTPD_PROGNAME".lock"
//define DEFAULT_LOCKFILE_PATH  DEFAULT_LOCKDIR"/"DEFAULT_LOCKFILE_NAME
#define DEFAULT_FILE_PERMS (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

/* default drift file location */
#define DEFAULT_DRIFTFILE "/etc/"PTPD_PROGNAME"_"DEFAULT_CLOCKDRIVER".drift"

/* default status file location */
#define DEFAULT_STATUSFILE DEFAULT_LOCKDIR"/"PTPD_PROGNAME".status"

/* Highest log level (default) catches all */
#define LOG_ALL LOG_DEBUGV

#define DEFAULT_TOKEN_DELIM ", ;\t"

/*
 * foreach loop across substrings from var, delimited by delim, placing
 * each token in targetvar on iteration, using id variable name prefix
 * to allow nesting (each loop uses an individual set of variables)
 */
#define foreach_token_begin(id, var, targetvar, delim) \
    int counter_##id = -1; \
    char* stash_##id = NULL; \
    char* text_##id; \
    char* text__##id; \
    char* targetvar; \
    text_##id=strdup(var); \
    for(text__##id = text_##id;; text__##id=NULL) { \
	targetvar = strtok_r(text__##id, delim, &stash_##id); \
	if(targetvar==NULL) break; \
	counter_##id++;

#define foreach_token_end(id) } \
    if(text_##id != NULL) { \
	free(text_##id); \
    } \
    counter_##id++;

/* drift recovery metod for use with -F */
enum {
	DRIFT_RESET = 0,
	DRIFT_KERNEL,
	DRIFT_FILE
};
/* IP transmission mode */
enum {
	TMODE_MC = 0,
	TMODE_UC,
	TMODE_MIXED,
#if 0
	TMODE_UC_SIGNALING
#endif
};

/* log timestamp mode */
enum {
	TIMESTAMP_DATETIME,
	TIMESTAMP_UNIX,
	TIMESTAMP_BOTH
};

/* Leap second handling */
enum {
	LEAP_ACCEPT,
	LEAP_IGNORE,
	LEAP_STEP,
	LEAP_SMEAR
};

/* Alarm codes */
enum {
	ALARM_PORTSTATE,
	ALARM_OFFSET_THRESHOLD,
	ALARM_CLOCK_STEP,
	ALARM_OFFSET_1SEC
};

#define MM_STARTING_BOUNDARY_HOPS  0x7fff

/* others */

/* bigger screen size constants */
#define SCREEN_BUFSZ  228
#define SCREEN_MAXSZ  180

/* default size for string buffers */
#define BUF_SIZE  1000

#define NANOSECONDS_MAX 999999999

// limit operator messages to once every X seconds
#define OPERATOR_MESSAGES_INTERVAL 300.0

#define MAX_SEQ_ERRORS 50

#define MAXTIMESTR 32

#endif /*CONSTANTS_DEP_H_*/
