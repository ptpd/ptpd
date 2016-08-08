# Makefile for ptpd

AUTOMAKE_OPTIONS = subdir-objects
lib_LTLIBRARIES = $(LIBPTPD2_LIBS_LA)
sbin_PROGRAMS = ptpd
man_MANS = ptpd.8 ptpd.conf.5

AM_CFLAGS	= $(SNMP_CFLAGS) $(PCAP_CFLAGS) -Wall -fexceptions
AM_CPPFLAGS	= $(SNMP_CPPFLAGS) $(PCAP_CPPFLAGS)
AM_LDFLAGS	= $(SNMP_LIBS) $(PCAP_LIBS)

if LINUX_KERNEL_HEADERS
AM_CFLAGS += $(LINUX_KERNEL_INCLUDES)
endif
AM_CPPFLAGS    += -DDATADIR='"$(datadir)"' $(PTP_DBL) $(PTP_DAEMON) $(PTP_EXP) $(PTP_SNMP) $(PTP_PCAP) $(PTP_SLAVE_ONLY) $(PTP_PTIMERS) $(PTP_UNICAST_MAX) $(PTP_DISABLE_SOTIMESTAMPING) $(PTP_PROFILING_OUTPUT)

NULL=

BOB=@VERSION_NUMBER@

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
	dep/ipv4_acl.h			\
	dep/ipv4_acl.c			\
	dep/msg.c			\
	dep/net.c			\
	dep/ptpd_dep.h			\
	dep/eventtimer.h		\
	dep/eventtimer.c		\
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
	dep/ntpengine/ntp_isc_md5.c	\
	dep/ntpengine/ntp_isc_md5.h	\
	dep/ntpengine/ntpdcontrol.c	\
	dep/ntpengine/ntpdcontrol.h	\
	timingdomain.h			\
	timingdomain.c			\
	libcck/net_utils.h		\
	libcck/net_utils.c		\
	dep/statistics.h		\
	dep/statistics.c		\
	dep/outlierfilter.h		\
	dep/outlierfilter.c		\
	dep/alarms.h			\
	dep/alarms.c			\
	libcck/cck_types.h		\
	libcck/clockdriver.h		\
	libcck/clockdriver.c		\
	libcck/clockdriver_interface.h	\
	libcck/clockdriver_unix.h		\
	libcck/clockdriver_unix.c		\
	libcck/extensions/clockdriver/linuxphc_vext_intel.h \
	libcck/extensions/clockdriver/linuxphc_vext_intel.c \
	libcck/extensions/clockdriver/efx_ioctl_timesync.h \
	libcck/extensions/clockdriver/linuxphc_vext_solarflare.h \
	libcck/extensions/clockdriver/linuxphc_vext_solarflare.c \
	libcck/extensions/clockdriver/linuxphc_vext.def \
	libcck/clockdriver_linuxphc.h	\
	libcck/clockdriver_linuxphc.c	\
	libcck/linkedlist.h		\
	globalconfig.h			\
	libcck/piservo.h		\
	libcck/piservo.c		\
	lib1588/ptp_primitives.h	\
	lib1588/ptp_primitives.c	\
	lib1588/ptp_derived_types.h	\
	lib1588/ptp_derived_types.c	\
	lib1588/ptp_tlv_signaling.h	\
	lib1588/ptp_tlv_signaling.c	\
	lib1588/ptp_tlv_management.h	\
	lib1588/ptp_tlv_management.c	\
	lib1588/ptp_tlv_other.h	\
	lib1588/ptp_tlv_other.c	\
	lib1588/ptp_tlv.h		\
	lib1588/ptp_tlv.c		\
	lib1588/ptp_message.h		\
	lib1588/ptp_message.c		\
	lib1588/tmp.h			\
	ptpd.c				\
	ptpd.h				\
	$(NULL)

# SNMP
if SNMP
ptpd_SOURCES += dep/snmp.c
endif

# posix timers
if PTIMERS
ptpd_SOURCES +=dep/eventtimer_posix.c
else
ptpd_SOURCES +=dep/eventtimer_itimer.c
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
