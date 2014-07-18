# Makefile for ptpd

#lib_LTLIBRARIES = $(LIBptpd_LIBS_LA)
sbin_PROGRAMS = ptpd
man_MANS = ptpd.8 ptpd.conf.5

AM_CFLAGS	= $(SNMP_CFLAGS) $(PCAP_CFLAGS) -Wall
AM_CPPFLAGS	= $(SNMP_CPPFLAGS) $(PCAP_CPPFLAGS)
AM_LDFLAGS	= $(SNMP_LIBS) $(PCAP_LIBS) -lpthread


if LINUX_KERNEL_HEADERS
AM_CFLAGS += $(LINUX_KERNEL_INCLUDES)
endif

AM_CPPFLAGS    += $(PTP_DBL) $(PTP_DAEMON) $(PTP_EXP) $(PTP_SNMP) $(PTP_PCAP) $(PTP_STATISTICS) $(PTP_NTPDC)

NULL=

#VERSION = 2.3.0
#SHAREDLIBVER = $(SHAREDLIB).$(VERSION)

EXTRA_DIST = def

libptp_SOURCES =			\
	arith.c				\
	bmc.c				\
	constants.h			\
	datatypes.h			\
	dep/constants_dep.h		\
	dep/datatypes_dep.h		\
	dep/msg.c			\
	dep/net.c			\
	dep/ptpd_dep.h			\
	dep/servo.c			\
	dep/iniparser/dictionary.h	\
	dep/iniparser/iniparser.h	\
	dep/iniparser/dictionary.c	\
	dep/iniparser/iniparser.c	\
	dep/daemonconfig.h		\
	dep/daemonconfig.c		\
	dep/startup.c			\
	dep/sys.c			\
	dep/timer.c			\
	display.c			\
	management.c			\
	protocol.c			\
	ptp_api.c                       \
	$(NULL)

# SNMP
if SNMP
libptp_SOURCES += dep/snmp.c
endif

# STATISTICS
if STATISTICS
libptp_SOURCES += dep/statistics.h
libptp_SOURCES += dep/statistics.c
endif

# NTP control
if NTPDC
libptp_SOURCES += dep/ntpengine/ntp_isc_md5.c
libptp_SOURCES += dep/ntpengine/ntp_isc_md5.h
libptp_SOURCES += dep/ntpengine/ntpdcontrol.c
libptp_SOURCES += dep/ntpengine/ntpdcontrol.h
endif

#
# FIXME: in order to create a single ptp lib, add the libcck sources as well.
# Is this better than just exposing libcck and making clients link to 2 libs?
# Why does libcck need to be a separate lib anyway?
#
libcck_la_SOURCES = 			\
	libcck/cck_types.h		\
	libcck/cck.h			\
	libcck/cck_loghandler.h		\
	libcck/cck_component.h		\
	libcck/cck_component.c		\
	libcck/cck_transport.h		\
	libcck/cck_transport.c		\
	libcck/cck_clockdriver.h	\
	libcck/cck_clockdriver.c	\
	libcck/clockdriver/cck_clockdriver_null.h	\
	libcck/clockdriver/cck_clockdriver_null.c	\
	libcck/clockdriver/cck_clockdriver_unix.h	\
	libcck/clockdriver/cck_clockdriver_unix.c	\
	libcck/cck_dummy.h		\
	libcck/cck_dummy.c		\
	libcck/dummy/cck_dummy_null.h	\
	libcck/dummy/cck_dummy_null.c	\
	libcck/dummy/cck_dummy_dummy.h	\
	libcck/dummy/cck_dummy_dummy.c	\
	libcck/cck_acl.h		\
	libcck/cck_acl.c		\
	libcck/acl/cck_acl_null.h	\
	libcck/acl/cck_acl_null.c	\
	libcck/acl/cck_acl_ipv4.h	\
	libcck/acl/cck_acl_ipv4.c	\
	libcck/acl/cck_acl_ipv6.h	\
	libcck/acl/cck_acl_ipv6.c	\
	libcck/acl/cck_acl_ethernet.h	\
	libcck/acl/cck_acl_ethernet.c	\
	libcck/transport/cck_transport_socket_common.h	\
	libcck/transport/cck_transport_socket_common.c	\
	libcck/transport/cck_transport_socket_swtimestamp.h	\
	libcck/transport/cck_transport_socket_swtimestamp.c	\
	libcck/transport/cck_transport_socket_hwtimestamp.h	\
	libcck/transport/cck_transport_socket_hwtimestamp.c	\
	libcck/transport/cck_transport_socket_ipv4.h	\
	libcck/transport/cck_transport_socket_ipv4.c	\
	libcck/transport/cck_transport_socket_ipv6.h	\
	libcck/transport/cck_transport_socket_ipv6.c	\
	libcck/transport/cck_transport_null.h	\
	libcck/transport/cck_transport_null.c

if PCAP
libcck_la_SOURCES += \
	libcck/transport/cck_transport_pcap_common.h	\
	libcck/transport/cck_transport_pcap_common.c	\
	libcck/transport/cck_transport_pcap_ethernet.h	\
	libcck/transport/cck_transport_pcap_ethernet.c \
	libcck/transport/cck_transport_pcap_ipv4.h	\
	libcck/transport/cck_transport_pcap_ipv4.c	\
	libcck/transport/cck_transport_pcap_ipv6.h	\
	libcck/transport/cck_transport_pcap_ipv6.c
endif



ptpd_SOURCES =                          \
	ptpd.c				\
	ptpd.h				\
	$(NULL)


lib_LTLIBRARIES = libptp.la

libptp_la_SOURCES = $(libptp_SOURCES) $(libcck_la_SOURCES)

libptp_la_LDFLAGS = -static
libptp_SHORTNAME = ptp


CSCOPE = cscope
GTAGS = gtags
DOXYGEN = doxygen

TAGFILES = GPATH GRTAGS GSYMS GTAGS cscope.in.out cscope.out cscope.po.out

ptpd_LDADD = libptp.la

#tags:
#	$(CSCOPE) -R -q -b
#	$(GTAGS)
#	$(DOXYGEN) Doxyfile
