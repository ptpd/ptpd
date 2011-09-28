
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
#define ADJ_FREQ_MAX  512000

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

#define PTP_EVENT_PORT    319
#define PTP_GENERAL_PORT  320

#define DEFAULT_PTP_DOMAIN_ADDRESS     "224.0.1.129"
#define PEER_PTP_DOMAIN_ADDRESS        "224.0.0.107"

/* used for -I option */
#define ALTERNATE_PTP_DOMAIN1_ADDRESS  "224.0.1.130"
#define ALTERNATE_PTP_DOMAIN2_ADDRESS  "224.0.1.131"
#define ALTERNATE_PTP_DOMAIN3_ADDRESS  "224.0.1.132"



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
