# Makefile for ptpd2

AUTOMAKE_OPTIONS = subdir-objects
lib_LTLIBRARIES = $(LIBPTPD2_LIBS_LA)
sbin_PROGRAMS = ptpd2
man_MANS = ptpd2.8 ptpd2.conf.5

AM_CFLAGS	= $(SNMP_CFLAGS) $(PCAP_CFLAGS) -Wall
AM_CPPFLAGS	= $(SNMP_CPPFLAGS) $(PCAP_CPPFLAGS)
AM_LDFLAGS	= $(SNMP_LIBS) $(PCAP_LIBS)

if LINUX_KERNEL_HEADERS
AM_CFLAGS += $(LINUX_KERNEL_INCLUDES)
endif

AM_CPPFLAGS    += $(PTP_DBL) $(PTP_DAEMON) $(PTP_EXP) $(PTP_SNMP) $(PTP_PCAP) $(PTP_STATISTICS) $(PTP_NTPDC)

NULL=

#VERSION = 2.3.1
#SHAREDLIBVER = $(SHAREDLIB).$(VERSION)

EXTRA_DIST = def

ptpd2_SOURCES =				\
	arith.c				\
	bmc.c				\
	constants.h			\
	datatypes.h			\
	dep/constants_dep.h		\
	dep/datatypes_dep.h		\
	dep/ipv4_acl.h			\
	dep/ipv4_acl.c			\
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
	ptpd.c				\
	ptpd.h				\
	$(NULL)

# SNMP
if SNMP
ptpd2_SOURCES += dep/snmp.c
endif

# STATISTICS
if STATISTICS
ptpd2_SOURCES += dep/statistics.h
ptpd2_SOURCES += dep/statistics.c
ptpd2_SOURCES += dep/outlierfilter.h
ptpd2_SOURCES += dep/outlierfilter.c
endif

# NTP control
if NTPDC
ptpd2_SOURCES += dep/ntpengine/ntp_isc_md5.c
ptpd2_SOURCES += dep/ntpengine/ntp_isc_md5.h
ptpd2_SOURCES += dep/ntpengine/ntpdcontrol.c
ptpd2_SOURCES += dep/ntpengine/ntpdcontrol.h
endif

CSCOPE = cscope
GTAGS = gtags
DOXYGEN = doxygen

TAGFILES = GPATH GRTAGS GSYMS GTAGS cscope.in.out cscope.out cscope.po.out

#tags:
#	$(CSCOPE) -R -q -b
#	$(GTAGS)
#	$(DOXYGEN) Doxyfile
