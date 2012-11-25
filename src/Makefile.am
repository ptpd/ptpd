# Makefile for ptpd2

lib_LTLIBRARIES = $(LIBPTPD2_LIBS_LA)
bin_PROGRAMS = ptpd2

AM_CFLAGS	= $SNMP_CFLAGS
AM_CPPFLAGS	= $SNMP_CPPFLAGS
AM_LDFLAGS	= $SNMP_LIBS

AM_CPPFLAGS	+= $PTP_DBL $PTP_DAEMON $PTP_EXP $PTP_SNMP $PTP_SIGUSR2

NULL=

#VERSION = 2.3.0
#SHAREDLIBVER = $(SHAREDLIB).$(VERSION)

ptpd2_SOURCES =			\
	arith.c			\
	bmc.c			\
	constants.h		\
	datatypes.h		\
	dep/constants_dep.h	\
	dep/datatypes_dep.h	\
	dep/msg.c		\
	dep/net.c		\
	dep/ptpd_dep.h		\
	dep/servo.c		\
	dep/startup.c		\
	dep/sys.c		\
	dep/timer.c		\
	display.c		\
	management.c		\
	protocol.c		\
	ptpd.c			\
	ptpd.h			\
	$(NULL)

# SNMP
if SNMP
ptpd2_SOURCES += dep/snmp.c
endif

CSCOPE = cscope
GTAGS = gtags
DOXYGEN = doxygen

TAGFILES = GPATH GRTAGS GSYMS GTAGS cscope.in.out cscope.out cscope.po.out

#tags:
#	$(CSCOPE) -R -q -b
#	$(GTAGS)
#	$(DOXYGEN) Doxyfile
