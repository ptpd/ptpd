
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
  !defined(__APPLE__)
#error Not ported to this architecture, please update.
#endif

#ifdef	linux
#include<netinet/in.h>
#include<net/if.h>
#include<net/if_arp.h>
#define IFACE_NAME_LENGTH         IF_NAMESIZE
#define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

#define IFCONF_LENGTH 10

#define octet ether_addr_octet
#include<endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define PTPD_LSBF
#elif __BYTE_ORDER == __BIG_ENDIAN
#define PTPD_MSBF
#endif
#endif /* linux */


#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__)
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <net/if.h>
# include <net/if_dl.h>
# include <net/if_types.h>
# if defined(__FreeBSD__) || defined(__APPLE__)
#  include <net/ethernet.h>
#  include <sys/uio.h>
# else
#  include <net/if_ether.h>
# endif
# include <ifaddrs.h>
# define IFACE_NAME_LENGTH         IF_NAMESIZE
# define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

# define IFCONF_LENGTH 10

# define adjtimex ntp_adjtime

# include <machine/endian.h>
# if BYTE_ORDER == LITTLE_ENDIAN
#   define PTPD_LSBF
# elif BYTE_ORDER == BIG_ENDIAN
#   define PTPD_MSBF
# endif
#endif

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

#define PACKET_SIZE  300 //ptpdv1 value kept because of use of TLV...
#define PACKET_BEGIN_UDP (ETHER_HDR_LEN + sizeof(struct ip) + \
	    sizeof(struct udphdr))
#define PACKET_BEGIN_ETHER (ETHER_HDR_LEN)

#define PTP_EVENT_PORT    319
#define PTP_GENERAL_PORT  320

#define DEFAULT_PTP_DOMAIN_ADDRESS     "224.0.1.129"
#define PEER_PTP_DOMAIN_ADDRESS        "224.0.0.107"

/* used for -I option */
#define ALTERNATE_PTP_DOMAIN1_ADDRESS  "224.0.1.130"
#define ALTERNATE_PTP_DOMAIN2_ADDRESS  "224.0.1.131"
#define ALTERNATE_PTP_DOMAIN3_ADDRESS  "224.0.1.132"

/* 802.3 Support */

#define PTP_ETHER_DST "01:1b:19:00:00:00"
#define PTP_ETHER_TYPE 0x88f7
#define PTP_ETHER_PEER "01:80:c2:00:00:0E"

/* dummy clock driver designation in preparation for generic clock driver API */
#define DEFAULT_CLOCKDRIVER "kernelclock"
/* default lock file location and mode */
#define DEFAULT_LOCKMODE F_WRLCK
#define DEFAULT_LOCKDIR "/var/run"
#define DEFAULT_LOCKFILE_NAME PTPD_PROGNAME".lock"
//define DEFAULT_LOCKFILE_PATH  DEFAULT_LOCKDIR"/"DEFAULT_LOCKFILE_NAME
#define DEFAULT_UMASK (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

/* default drift file location */
#define DEFAULT_DRIFTFILE "/etc/"PTPD_PROGNAME"_"DEFAULT_CLOCKDRIVER".drift"

/* default status file location */
#define DEFAULT_STATUSFILE DEFAULT_LOCKDIR"/"PTPD_PROGNAME".status"

/* Highest log level (default) catches all */
#define LOG_ALL LOG_DEBUGV

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


#define MAXTIMESTR 32

#endif /*CONSTANTS_DEP_H_*/
