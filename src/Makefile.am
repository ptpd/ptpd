# Makefile for ptpd

AUTOMAKE_OPTIONS = subdir-objects
lib_LTLIBRARIES = $(LIBPTPD2_LIBS_LA)
sbin_PROGRAMS = ptpd
man_MANS = ptpd.8 ptpd.conf.5

AM_CFLAGS	= $(SNMP_CFLAGS) $(PCAP_CFLAGS) -Wall -fexceptions
AM_CPPFLAGS	= $(SNMP_CPPFLAGS) $(PCAP_CPPFLAGS)
AM_LDFLAGS	= $(SNMP_LIBS) $(PCAP_LIBS)

AM_CPPFLAGS    += -DDATADIR='"$(datadir)"' $(PTP_DBL) $(PTP_DAEMON) $(PTP_EXP) $(PTP_SLAVE_ONLY) $(PTP_PROFILING_OUTPUT)

NULL=

#VERSION = 2.3.1
#SHAREDLIBVER = $(SHAREDLIB).$(VERSION)
#	ptp_timers.c			

EXTRA_DIST = 			\
	def			\
	lib1588/def	

ptpd_SOURCES =				\
	arith.c				\
	bmc.c				\
	constants.h			\
	ptp_primitives.h		\
	ptp_datatypes.h			\
	datatypes.h			\
	dep/constants_dep.h		\
	dep/datatypes_dep.h		\
	dep/msg.c			\
	dep/cck_glue.h			\
	dep/cck_glue.c			\
	dep/transport.h			\
	dep/transport.c			\
	dep/ptpd_dep.h			\
	ptp_timers.h			\
	ptp_timers.c			\
	dep/servo.c			\
	dep/iniparser/dictionary.h	\
	dep/iniparser/iniparser.h	\
	dep/iniparser/dictionary.c	\
	dep/iniparser/iniparser.c	\
	dep/configdefaults.h		\
	dep/configdefaults.c		\
	dep/daemonconfig.h		\
	dep/daemonconfig.c		\
	dep/startup.c			\
	dep/sys.c			\
	display.c			\
	management.c			\
	signaling.c			\
	protocol.c			\
	globalconfig.h			\
	dep/ntpengine/ntpoptions.h	\
	dep/ntpengine/ntp_isc_md5.c	\
	dep/ntpengine/ntp_isc_md5.h	\
	dep/ntpengine/ntpdcontrol.c	\
	dep/ntpengine/ntpdcontrol.h	\
	libcck/cck_logger.h		\
	libcck/cck_logger.c		\
	libcck/net_utils.h		\
	libcck/net_utils.c		\
	libcck/statistics.h		\
	libcck/statistics.c		\
	libcck/cck.h			\
	libcck/cck_types.h		\
	libcck/log_handler.h		\
	libcck/linked_list.h		\
	libcck/cck_utils.h		\
	libcck/cck_utils.c		\
	libcck/cck_timestamp.h		\
	libcck/cck_timestamp.c		\
	libcck/fd_set.h			\
	libcck/fd_set.c			\
	libcck/timer.def		\
	libcck/timer.h			\
	libcck/timer.c			\
	libcck/timer_interface.h	\
	libcck/transport_address.h	\
	libcck/transport_address.c	\
	libcck/acl.def			\
	libcck/acl.h			\
	libcck/acl.c			\
	libcck/acl_interface.h		\
	libcck/acl/acl_dummy.h		\
	libcck/acl/acl_dummy.c		\
	libcck/acl/acl_ipv4.h		\
	libcck/acl/acl_ipv4.c		\
	libcck/acl/acl_ipv6.h		\
	libcck/acl/acl_ipv6.c		\
	libcck/acl/acl_ethernet.h	\
	libcck/acl/acl_ethernet.c	\
	libcck/ttransport.def		\
	libcck/ttransport.h		\
	libcck/ttransport.c		\
	libcck/ttransport_interface.h	\
	libcck/ttransport/udp_common.h\
	libcck/ttransport/udp_common.c\
	libcck/ttransport/socket_common.h\
	libcck/ttransport/socket_common.c\
	libcck/ttransport/socket_udpv4.h\
	libcck/ttransport/socket_udpv4.c\
	libcck/ttransport/socket_udpv6.h\
	libcck/ttransport/socket_udpv6.c\
	libcck/clockdriver.def		\
	libcck/clockdriver.h		\
	libcck/clockdriver.c		\
	libcck/clockdriver_interface.h	\
	libcck/clockdriver/unix_common.h \
	libcck/clockdriver/unix_common.c \
	libcck/clockdriver/clockdriver_unix.h		\
	libcck/clockdriver/clockdriver_unix.c		\
	libcck/piservo.h		\
	libcck/piservo.c		\
	libcck/libcck.h			\
	libcck/libcck.c			\
	lib1588/ptp_primitives.h	\
	lib1588/ptp_primitives.c	\
	lib1588/ptp_derived_types.h	\
	lib1588/ptp_derived_types.c	\
	lib1588/ptp_tlv_signaling.h	\
	lib1588/ptp_tlv_signaling.c	\
	lib1588/ptp_tlv_management.h	\
	lib1588/ptp_tlv_management.c	\
	lib1588/ptp_tlv_other.h		\
	lib1588/ptp_tlv_other.c		\
	lib1588/ptp_tlv.h		\
	lib1588/ptp_tlv.c		\
	lib1588/ptp_message.h		\
	lib1588/ptp_message.c		\
	lib1588/tmp.h			\
	lib1588_glue.h			\
	lib1588_glue.c			\
	dep/statistics.h		\
	dep/statistics.c		\
	dep/outlierfilter.h		\
	dep/outlierfilter.c		\
	dep/alarms.h			\
	dep/alarms.c			\
	ptpd.c				\
	ptpd.h				\
	$(NULL)

# LibCCK conditional component builds follow
if LINUX
ptpd_SOURCES += libcck/net_utils_linux.h\
		libcck/net_utils_linux.c
endif

if CCK_BUILD_TTRANSPORT_SOCK_RAWETH
ptpd_SOURCES += libcck/ttransport/socket_raweth.h\
		libcck/ttransport/socket_raweth.c
endif


if CCK_BUILD_TTRANSPORT_LINUXTS
ptpd_SOURCES += libcck/ttransport/linuxts_common.h\
		libcck/ttransport/linuxts_common.c\
		libcck/ttransport/linuxts_udpv4.h\
		libcck/ttransport/linuxts_udpv4.c\
		libcck/ttransport/linuxts_udpv6.h\
		libcck/ttransport/linuxts_udpv6.c\
		libcck/ttransport/linuxts_raweth.h\
		libcck/ttransport/linuxts_raweth.c
endif

if CCK_BUILD_TTRANSPORT_PCAP
ptpd_SOURCES += libcck/ttransport/pcap_common.h\
		libcck/ttransport/pcap_common.c\
		libcck/ttransport/pcap_ethernet.h\
		libcck/ttransport/pcap_ethernet.c\
		libcck/ttransport/pcap_udpv4.h\
		libcck/ttransport/pcap_udpv4.c\
		libcck/ttransport/pcap_udpv6.h\
		libcck/ttransport/pcap_udpv6.c
endif

if CCK_BUILD_CLOCKDRIVER_LINUXPHC
ptpd_SOURCES += libcck/clockdriver/vext/linuxphc_vext_intel.h\
		libcck/clockdriver/vext/linuxphc_vext_intel.c\
		libcck/clockdriver/vext/efx_ioctl_timesync.h\
		libcck/clockdriver/vext/linuxphc_vext.def\
		libcck/clockdriver/vext/linuxphc_vext_solarflare.h\
		libcck/clockdriver/vext/linuxphc_vext_solarflare.c\
		libcck/clockdriver/clockdriver_linuxphc.h\
		libcck/clockdriver/clockdriver_linuxphc.c
endif

if CCK_BUILD_TIMER_ITIMER
ptpd_SOURCES += libcck/timer/itimer.h\
		libcck/timer/itimer.c
endif

if CCK_BUILD_TIMER_POSIX
ptpd_SOURCES += libcck/timer/posix.h\
		libcck/timer/posix.c
endif

if CCK_BUILD_TIMER_TIMERFD
ptpd_SOURCES += libcck/timer/timerfd.h\
		libcck/timer/timerfd.c
endif

#if CCK_BUILD_
#ptpd_SOURCES += 
#endif


# SNMP
if PTPD_SNMP
ptpd_SOURCES += dep/snmp.c
endif

CSCOPE = cscope
GTAGS = gtags
DOXYGEN = doxygen

TAGFILES = GPATH GRTAGS GSYMS GTAGS cscope.in.out cscope.out cscope.po.out

dist:
	cd .. && make dist

#tags:
#	$(CSCOPE) -R -q -b
#	$(GTAGS)
#	$(DOXYGEN) Doxyfile
