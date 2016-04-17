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

#VERSION = 2.3.1
#SHAREDLIBVER = $(SHAREDLIB).$(VERSION)
#	ptp_timers.c			

EXTRA_DIST = def

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
	dep/statistics.h		\
	dep/statistics.c		\
	dep/outlierfilter.h		\
	dep/outlierfilter.c		\
	dep/alarms.h			\
	dep/alarms.c			\
	dep/clockdriver.h		\
	dep/clockdriver.c		\
	dep/clockdriver_interface.h	\
	dep/clockdriver_unix.h		\
	dep/clockdriver_unix.c		\
	dep/clockdriver_linuxphc.h	\
	dep/clockdriver_linuxphc.c	\
	dep/linkedlist.h		\
	globalconfig.h			\
	dep/piservo.h			\
	dep/piservo.c			\
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
