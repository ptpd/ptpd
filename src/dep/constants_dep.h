
/* constants_dep.h */

#ifndef CONSTANTS_DEP_H
#define CONSTANTS_DEP_H

/**
*\file
* \brief Plateform-dependent constants definition
*
* This header defines all includes and constants which are plateform-dependent
*
* ptpdv2 is only implemented for linux, NetBSD and FreeBSD
 */

/* platform dependent */

#if !defined(linux) && !defined(__NetBSD__) && !defined(__FreeBSD__) && \
  !defined(__APPLE__) && !defined(__OpenBSD__) && !defined(__sun) && !defined(__QNXNTO__)
#error PTPD hasn't been ported to this OS - should be possible \
if it's POSIX compatible, if you succeed, report it to ptpd-devel@sourceforge.net
#endif

#ifdef	linux
#include<netinet/in.h>
#include<net/if.h>
#include<net/if_arp.h>
#include <ifaddrs.h>
#define IFACE_NAME_LENGTH         IF_NAMESIZE
#define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

#define IFCONF_LENGTH 10

#define octet ether_addr_octet
#endif /* linux */

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__sun) || defined(__QNXNTO__)
# include <sys/types.h>
# include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif /* HAVE_SYS_SOCKIO_H */
# include <netinet/in.h>
# include <net/if.h>
# include <net/if_dl.h>
# include <net/if_types.h>
#ifdef HAVE_NET_IF_ETHER_H
#  include <net/if_ether.h>
#endif
#ifdef HAVE_SYS_UIO_H
#  include <sys/uio.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#  include <net/ethernet.h>
#endif
#include <ifaddrs.h>
# define IFACE_NAME_LENGTH         IF_NAMESIZE
# define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#ifdef __QNXNTO__
#include <sys/neutrino.h>
#include <sys/syspage.h>
#define BSD_INTERFACE_FUNCTIONS
#endif /* __QNXNTO __ */


#if !defined(ETHER_ADDR_LEN) && defined(ETHERADDRL)
#	define ETHER_ADDR_LEN ETHERADDRL
#endif /* ETHER_ADDR_LEN && ETHERADDRL */

#ifndef ETHER_HDR_LEN
#	define ETHER_HDR_LEN sizeof (struct ether_header)
#endif /* ETHER_ADDR_LEN && ETHERADDRL */


# define IFCONF_LENGTH 10

# define adjtimex ntp_adjtime


#endif

#ifdef HAVE_MACHINE_ENDIAN_H
#	include <machine/endian.h>
#endif /* HAVE_MACHINE_ENDIAN_H */

#ifdef HAVE_ENDIAN_H
#	include <endian.h>
#endif /* HAVE_ENDIAN_H */

#ifdef HAVE_SYS_ISA_DEFS_H
#	include <sys/isa_defs.h>
#endif /* HAVE_SYS_ISA_DEFS_H */

# if BYTE_ORDER == LITTLE_ENDIAN || defined(_LITTLE_ENDIAN) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN)
#   define PTPD_LSBF
# elif BYTE_ORDER == BIG_ENDIAN || defined(_BIG_ENDIAN) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
#   define PTPD_MSBF
# endif

#define CLOCK_IDENTITY_LENGTH 8
#define ADJ_FREQ_MAX 500000

/* UDP/IPv4 dependent */
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001UL
#endif

#define SUBDOMAIN_ADDRESS_LENGTH  4
#define PORT_ADDRESS_LENGTH       2
#define PTP_UUID_LENGTH           6
#define CLOCK_IDENTITY_LENGTH	  8
#define FLAG_FIELD_LENGTH         2

#define PACKET_SIZE  300
#define PACKET_BEGIN_UDP (ETHER_HDR_LEN + sizeof(struct ip) + \
	    sizeof(struct udphdr))
#define PACKET_BEGIN_ETHER (ETHER_HDR_LEN)

#define PTP_EVENT_PORT    319
#define PTP_GENERAL_PORT  320

#define DEFAULT_PTP_DOMAIN_ADDRESS     "224.0.1.129"
#define PEER_PTP_DOMAIN_ADDRESS        "224.0.0.107"

/* 802.3 Support */

#define PTP_ETHER_DST "01:1b:19:00:00:00"
#define PTP_ETHER_TYPE 0x88f7
#define PTP_ETHER_PEER "01:80:c2:00:00:0E"

#ifdef PTPD_UNICAST_MAX
#define UNICAST_MAX_DESTINATIONS PTPD_UNICAST_MAX
#else
#define UNICAST_MAX_DESTINATIONS 16
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

/* Difference between Unix time / UTC and NTP time */
#define NTP_EPOCH 2208988800ULL

/* wait a maximum of 10 ms for a late TX timestamp */
#define LATE_TXTIMESTAMP_US 10000

/* drift recovery metod for use with -F */
enum {
	DRIFT_RESET = 0,
	DRIFT_KERNEL,
	DRIFT_FILE
};
/* IP transmission mode */
enum {
	IPMODE_MULTICAST = 0,
	IPMODE_UNICAST,
	IPMODE_HYBRID,
#if 0
	IPMODE_UNICAST_SIGNALING
#endif
};

/* log timestamp mode */
enum {
	TIMESTAMP_DATETIME,
	TIMESTAMP_UNIX,
	TIMESTAMP_BOTH
};

/* servo dT calculation mode */
enum {
	DT_NONE,
	DT_CONSTANT,
	DT_MEASURED
};

/* StatFilter op type */
enum {
	FILTER_NONE,
	FILTER_MEAN,
	FILTER_MIN,
	FILTER_MAX,
	FILTER_ABSMIN,
	FILTER_ABSMAX,
	FILTER_MEDIAN,
	FILTER_MAXVALUE
};

/* StatFilter window type */
enum {
	WINDOW_INTERVAL,
	WINDOW_SLIDING,
//	WINDOW_OVERLAPPING
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
