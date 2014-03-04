# Makefile for libcck

AM_CFLAGS	= $(SNMP_CFLAGS) $(PCAP_CFLAGS) -Wall
AM_CPPFLAGS	= $(SNMP_CPPFLAGS) $(PCAP_CPPFLAGS)
AM_LDFLAGS	= $(SNMP_LIBS) $(PCAP_LIBS)

if LINUX_KERNEL_HEADERS
AM_CFLAGS += $(LINUX_KERNEL_INCLUDES)
endif

AM_CPPFLAGS    += $(PTP_DBL) $(PTP_DAEMON) $(PTP_EXP) $(PTP_SNMP) $(PTP_PCAP) $(PTP_STATISTICS) $(PTP_NTPDC)


lib_LTLIBRARIES = libcck.la

libcck_la_SOURCES = 			\
	cck_types.h		\
	cck.h			\
	cck_loghandler.h		\
	cck_component.h		\
	cck_component.c		\
	cck_transport.h		\
	cck_transport.c		\
	cck_acl.h		\
	cck_acl.c		\
	acl/cck_acl_null.h	\
	acl/cck_acl_null.c	\
	acl/cck_acl_ipv4.h	\
	acl/cck_acl_ipv4.c	\
	acl/cck_acl_ipv6.h	\
	acl/cck_acl_ipv6.c	\
	acl/cck_acl_ethernet.h	\
	acl/cck_acl_ethernet.c	\
	transport/cck_transport_helpers.h	\
	transport/cck_transport_helpers.c	\
	transport/cck_transport_swtimestamp.h	\
	transport/cck_transport_swtimestamp.c	\
	transport/cck_transport_null.h	\
	transport/cck_transport_null.c	\
	transport/cck_transport_ipv4.h	\
	transport/cck_transport_ipv4.c	\
	transport/cck_transport_ipv6.h	\
	transport/cck_transport_ipv6.c	\
	transport/cck_transport_ethernet.h	\
	transport/cck_transport_ethernet.c

libcck_la_LDFLAGS = -version-info 1:0:1 -static
libcck_SHORTNAME = cck

NULL=

#VERSION = 2.3.0
#SHAREDLIBVER = $(SHAREDLIB).$(VERSION)

CSCOPE = cscope
GTAGS = gtags
DOXYGEN = doxygen

TAGFILES = GPATH GRTAGS GSYMS GTAGS cscope.in.out cscope.out cscope.po.out

