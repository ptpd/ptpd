#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_include([m4/version.m4])
AC_PREREQ(2.59)
AC_INIT(
    [ptpd],
    [VERSION_NUMBER],
    [ptpd-bugs@nwtime.org],
    [],
    [PTPD_URL]dnl
)
AC_CONFIG_SRCDIR([src/arith.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9 foreign -Wall])

[RELEASE_DATE]="RELEASE_DATE"
AC_SUBST([RELEASE_DATE])
[VERSION_NUMBER]="VERSION_NUMBER"
AC_SUBST([VERSION_NUMBER])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_LIBTOOL

# Check for svnversion, cut and tr

AC_PATH_PROG(cutpath, cut)
AC_PATH_PROG(trpath, tr)

# Set CODE_REVISION if svnversion found, version is suffixed with "-svn" and code is versioned
if test -n "$cutpath" && test -n "$trpath"; then
    ver_prefix=`echo VERSION_NUMBER | $cutpath -s -d- -f2`
    AC_PATH_PROG(svnverpath, svnversion)
    if test "$ver_prefix" = "svn" && test -n "$svnverpath"; then
	svn_rev=`$svnverpath 2>/dev/null | $cutpath -s -d: -f2 | $trpath -d MS`
	if test -n "$svn_rev"; then
	    AC_DEFINE_UNQUOTED(CODE_REVISION, ["$svn_rev"], [source code revision])
	fi
    fi
fi

AC_ARG_WITH(
    [pcap-config],
    [AS_HELP_STRING(
	[--with-pcap-config],
	[+ =pcap-config]
    )],
    [ans=$withval],
    [ans=yes]
)
case "$ans" in
 no)
    ;;
 yes)
    ans=pcap-config
    ;;
 /*)
    pcap_config_dirname=`dirname $ans`
    pcap_config_basename=`basename $ans`
    ans=abspath
    ;;
 */*)
    AC_MSG_ERROR([--with-pcap-config takes either a name or an absolute path])
    ;;
 *)
    ;;
esac
PROG_PCAP_CONFIG=$ans
case "$PROG_PCAP_CONFIG" in
 no)
    ;;
 abspath)
    AC_PATH_PROG([PATH_PCAP_CONFIG], [$pcap_config_basename], [], [$pcap_config_dirname])
    AS_UNSET([ac_cv_path_PATH_PCAP_CONFIG])
    ;;
 *)
    AC_PATH_PROG([PATH_PCAP_CONFIG], [$PROG_PCAP_CONFIG])
    AS_UNSET([ac_cv_path_PATH_PCAP_CONFIG])
    ;;
esac

AC_MSG_CHECKING([if we want to build with libpcap support])
AC_ARG_ENABLE(
    [pcap],
    [AS_HELP_STRING(
	[--disable-pcap],
	[disable support for PCAP (enabled if we find pcap-config)]
    )],
    [try_pcap=$enableval; AC_MSG_RESULT([$try_pcap])],
    [case "$PATH_PCAP_CONFIG" in
     /*)
	AC_MSG_RESULT([yes, pcap-config])
	AC_CHECK_HEADERS([pcap/pcap.h pcap.h])
	try_pcap=yes
	;;
     *)
	AC_MSG_RESULT([yes, guessing flags])
	AC_CHECK_HEADERS([pcap/pcap.h pcap.h])
	try_pcap=guess
	;;
    esac]
)

ptpd_pcap_enabled=0

case "$try_pcap" in
 yes)
    case "$PATH_PCAP_CONFIG" in
     /*)
	PCAP_LIBS=`$PATH_PCAP_CONFIG --libs`
	AC_SUBST([PCAP_LIBS])
	# Separate CPPFLAGS and CFLAGS
	foo=`$PATH_PCAP_CONFIG --cflags`
	PCAP_CPPFLAGS=
	PCAP_CFLAGS=
	for i in $foo; do
	    case "$i" in
	     -D*|-F*|-U*|-I*)
		PCAP_CPPFLAGS="$PCAP_CPPFLAGS $i"
		;;
	    *)	PCAP_CFLAGS="$PCAP_CFLAGS $i"
		;;
	    esac
	done
	AC_SUBST([PCAP_CPPFLAGS])
	AC_SUBST([PCAP_CFLAGS])

	save_CFLAGS=$CFLAGS
	save_CPPFLAGS=$CPPFLAGS
	save_LIBS=$LIBS
	CFLAGS=$PCAP_CFLAGS
	CPPFLAGS=$PCAP_CPPFLAGS



	CFLAGS=$save_CFLAGS
	AS_UNSET([save_CFLAGS])
	CPPFLAGS=$save_CPPFLAGS
	AS_UNSET([save_CPPFLAGS])
	LIBS=$save_LIBS
	AS_UNSET([save_LIBS])

	AC_MSG_CHECKING([if we can build with libpcap support])
	case "$ac_cv_header_pcap_pcap_h" in
	 yes)
		ptpd_pcap_enabled=1
		PTP_PCAP="-DPTPD_PCAP"
		AC_MSG_RESULT([yes])
		;;
	 no)
		case "$ac_cv_header_pcap_h" in
		 yes)
			ptpd_pcap_enabled=1
			PTP_PCAP="-DPTPD_PCAP"
			AC_MSG_RESULT([yes])
			;;
		 no)
			PTP_PCAP=""
			PCAP_LIBS=""
			PCAP_CPPFLAGS=""
			PCAP_CFLAGS=""
			AC_MSG_RESULT([no])
			;;
		esac
		;;

	esac
	;;
     *)
	# This case can't be triggered - we won't get here unless we
	# have an absolute path to pcap-config.
	AC_MSG_WARN([Cannot build with libpcap support - pcap-config cannot be found])
	;;
    esac
    ;;
 guess)
	PCAP_LIBS="-lpcap"
	AC_SUBST([PCAP_LIBS])

	AC_MSG_CHECKING([if we can build with PCAP support])
	case "$ac_cv_header_pcap_pcap_h" in
	 yes)
		ptpd_pcap_enabled=1
		PTP_PCAP="-DPTPD_PCAP"
		AC_MSG_RESULT([yes])
		;;
	 no)
		case "$ac_cv_header_pcap_h" in
		 yes)
			ptpd_pcap_enabled=1
			PTP_PCAP="-DPTPD_PCAP"
			AC_MSG_RESULT([yes])
			;;
		 no)
			PTP_PCAP=""
			PCAP_LIBS=""
			PCAP_CPPFLAGS=""
			PCAP_CFLAGS=""
			AC_MSG_RESULT([no])
			;;
		esac
		;;
	esac

;;

esac

AC_SUBST(PTP_PCAP)
AM_CONDITIONAL([PCAP], [test x$ptpd_pcap_enabled = x1])

AC_ARG_WITH(
    [net-snmp-config],
    [AS_HELP_STRING(
	[--with-net-snmp-config],
	[+ =net-snmp-config]
    )],
    [ans=$withval],
    [ans=yes]
)
case "$ans" in
 no)
    ;;
 yes)
    ans=net-snmp-config
    ;;
 /*)
    net_snmp_config_dirname=`dirname $ans`
    net_snmp_config_basename=`basename $ans`
    ans=abspath
    ;;
 */*)
    AC_MSG_ERROR([--with-net-snmp-config takes either a name or an absolute path])
    ;;
 *)
    ;;
esac
PROG_NET_SNMP_CONFIG=$ans
case "$PROG_NET_SNMP_CONFIG" in
 no)
    ;;
 abspath)
    AC_PATH_PROG([PATH_NET_SNMP_CONFIG], [$net_snmp_config_basename], [], [$net_snmp_config_dirname])
    AS_UNSET([ac_cv_path_PATH_NET_SNMP_CONFIG])
    ;;
 *)
    AC_PATH_PROG([PATH_NET_SNMP_CONFIG], [$PROG_NET_SNMP_CONFIG])
    AS_UNSET([ac_cv_path_PATH_NET_SNMP_CONFIG])
    ;;
esac

AC_MSG_CHECKING([if we want to try building SNMP support])
AC_ARG_ENABLE(
    [snmp],
    [AS_HELP_STRING(
	[--disable-snmp],
	[disable support for SNMP (enabled if we find net-snmp-config)]
    )],
    [try_snmp=$enableval],
    [case "$PATH_NET_SNMP_CONFIG" in
     /*)
	try_snmp=yes
	;;
     *)
	try_snmp=no
	;;
    esac]
)
AC_MSG_RESULT([$try_snmp])

ptpd_snmp_enabled=0
case "$try_snmp" in
 yes)
    case "$PATH_NET_SNMP_CONFIG" in
     /*)
	SNMP_LIBS=`$PATH_NET_SNMP_CONFIG --agent-libs`
	AC_SUBST([SNMP_LIBS])
	# HMS: we really want to separate CPPFLAGS and CFLAGS
	foo=`$PATH_NET_SNMP_CONFIG --base-cflags`
	SNMP_CPPFLAGS=
	SNMP_CFLAGS=
	for i in $foo; do
	    case "$i" in
	     -D*|-F*|-U*|-I*)
		SNMP_CPPFLAGS="$SNMP_CPPFLAGS $i"
		;;
	    *)	SNMP_CFLAGS="$SNMP_CFLAGS $i"
		;;
	    esac
	done
	AC_SUBST([SNMP_CPPFLAGS])
	AC_SUBST([SNMP_CFLAGS])

	save_CFLAGS=$CFLAGS
	save_CPPFLAGS=$CPPFLAGS
	save_LIBS=$LIBS
	CFLAGS=$SNMP_CFLAGS
	CPPFLAGS=$SNMP_CPPFLAGS

	AC_CHECK_HEADER([net-snmp/net-snmp-config.h])

	CFLAGS=$save_CFLAGS
	AS_UNSET([save_CFLAGS])
	CPPFLAGS=$save_CPPFLAGS
	AS_UNSET([save_CPPFLAGS])
	LIBS=$save_LIBS
	AS_UNSET([save_LIBS])

	AC_MSG_CHECKING([if we are building SNMP support])
	case "$ac_cv_header_net_snmp_net_snmp_config_h" in
	 yes)
		ptpd_snmp_enabled=1
		PTP_SNMP="-DPTPD_SNMP"
		AC_MSG_RESULT([yes])
		;;
	 no)
		PTP_SNMP=""
		SNMP_LIBS=""
		SNMP_CPPFLAGS=""
		SNMP_CFLAGS=""
		AC_MSG_RESULT([no])
		;;
	esac
	;;
     *) 
	# This case can't be triggered - we won't get here unless we
	# have an absolute path to net-snmp-config.
	AC_MSG_WARN([Cannot build with SNMP support - net-snmp-config cannot be found])
	;;
    esac
    ;;
esac

AC_SUBST(PTP_SNMP)
AM_CONDITIONAL([SNMP], [test x$ptpd_snmp_enabled = x1])

case $host in
*linux*)

    AC_CHECK_HEADERS([linux/net_tstamp.h], [], [

    if [ test -d "/usr/src/kernels/`uname -r`/include" ]; then
	LINUX_KERNEL_INCLUDES="-I/usr/src/kernels/`uname -r`/include"
	LINUX_KERNEL_HEADERS=yes
    fi

    if [ test -d "/usr/src/linux-headers-`uname -r`/include" ]; then
	LINUX_KERNEL_INCLUDES="-I/usr/src/linux-headers-`uname -r`/include"
	LINUX_KERNEL_HEADERS=yes
    fi

    ])

    ;;
esac

AM_CONDITIONAL(LINUX_KERNEL_HEADERS, test x$LINUX_KERNEL_HEADERS = xyes)
AC_SUBST(LINUX_KERNEL_INCLUDES)

# Checks for libraries.
AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([rt], [clock_gettime])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h net/ethernet.h netinet/in.h netinet/in_systm.h netinet/ether.h sys/uio.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h glob.h sched.h utmp.h utmpx.h linux/rtc.h sys/timex.h])

# MUST chck for cpuset AFTER the check for param as the latter needs 
# the former to pass the compile check.
AC_CHECK_HEADERS([sys/cpuset.h], [], [],
[#ifdef HAVE_SYS_PARAM_H
    # include <sys/param.h>
#endif
])

# Similar case for if.h
AC_CHECK_HEADERS([net/if.h], [], [],
[
#ifdef HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif
])

# Similar case for if_arp.h
AC_CHECK_HEADERS([net/if_arp.h], [], [],
[
#ifdef HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif
#ifdef HAVE_NET_IF_H
    #include <net/if.h>
#endif
])

# Similar case for netinet/if_ether.h
AC_CHECK_HEADERS([netinet/if_ether.h], [], [],
[
#ifdef HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
    #include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
    #include <net/if_arp.h>
#endif
#ifdef HAVE_NET_IF_H
    #include <net/if.h>
#endif
])


# ...and for if_ether.h
AC_CHECK_HEADERS([net/if_ether.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif
#ifdef HAVE_NET_IF_H
    #include <net/if.h>
#endif
])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL
AC_HEADER_TIME
AC_C_VOLATILE

# Automake 2.59 has a problem with those on RHEL5
m4_version_prereq(2.60,[
    AC_TYPE_UINT32_T
    AC_TYPE_UINT64_T
    AC_TYPE_UINT8_T
    AC_TYPE_SIZE_T
    AC_TYPE_SSIZE_T
    AC_TYPE_INT64_T
    ],
    [AC_TYPE_SIZE_T]
)

# Check for tick in the timex structure
AC_CHECK_MEMBERS([struct timex.tick], [], [], [[#include <sys/timex.h>]])

# ether_add: octet vs. ether_addr_octet - FreeBSD, any others?
AC_CHECK_MEMBERS([struct ether_addr.octet], [], [],
[
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif
#ifdef HAVE_NETINET_ETHER_H
    #include <netinet/ether.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
    #include <net/ethernet.h>
#endif
#ifdef HAVE_NET_IF_ETHER_H
    #include <net/if.h>
#endif
#ifdef HAVE_NET_IF_ETHER_H
    #include <net/if_ether.h>
#endif
])

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([clock_gettime dup2 ftruncate gettimeofday inet_ntoa memset pow select socket strchr strdup strerror strtol glob pututline utmpxname updwtmpx setutent endutent])

AC_CHECK_DECLS([MSG_ERRQUEUE], [], [], [[#include <sys/socket.h>]])

AC_MSG_CHECKING([for RUNTIME_DEBUG])
AC_ARG_ENABLE(
    [runtime-debug],
    [AS_HELP_STRING(
	[--enable-runtime-debug],
	[Enable all debug messages]
    )],
    [ptp_runtime=$enableval],
    [ptp_runtime=no]
)
AC_MSG_RESULT([$ptp_runtime])
case "$ptp_runtime" in
 yes)
    PTP_DBL="-DRUNTIME_DEBUG"
    ;;
 *) 
    AC_MSG_CHECKING([for (old-style) debug message level])
    AC_ARG_ENABLE(
        [debug-level],
        [AS_HELP_STRING(
	    [[[[--enable-debug-level={basic,medium,all}]]]],
	    [debug message level: basic, medium, all]

        )],
        [ptp_dblevel=$enableval],
        [ptp_dblevel=no]
    )
    AC_MSG_RESULT([$ptp_dblevel])
    case "$ptp_dblevel" in
     "basic")
	PTP_DBL="-DPTPD_DBG"
	;;
     "medium")
	PTP_DBL="-DPTPD_DBG2"
	;;
     "all")
	PTP_DBL="-DPTPD_DBGV"
	;;
     *) PTP_DBL=""
	;;
    esac
    ;;
esac
AC_SUBST(PTP_DBL)

AC_MSG_CHECKING([for daemon mode])
AC_ARG_ENABLE(
    [daemon],
    [AS_HELP_STRING(
	[--disable-daemon],
	[Disable daemon mode (enabled by default)]
    )],
    [ptp_daemon=$enableval],
    [ptp_daemon=yes]
)
AC_MSG_RESULT([$ptp_daemon])
case "$ptp_daemon" in
 no)
    PTP_DAEMON="-DPTPD_NO_DAEMON"
    ;;
esac
AC_SUBST(PTP_DAEMON)

AC_MSG_CHECKING([for experimental options])
AC_ARG_ENABLE(
    [experimental-options],
    [AS_HELP_STRING(
	[--enable-experimental-options],
	[Enable experimental options (disabled by default)]
    )],
    [ptp_exp=$enableval],
    [ptp_exp=no]
)
AC_MSG_RESULT([$ptp_exp])
case "$ptp_exp" in
 yes)
    PTP_EXP="-DPTPD_EXPERIMENTAL"
    ;;
esac
AC_SUBST(PTP_EXP)

AC_MSG_CHECKING([for realtime statistics support])
AC_ARG_ENABLE(
    [statistics],
    [AS_HELP_STRING(
	[--enable-statistics],
	[Enable support for realtime statistics and statistics based filtering]
    )],
    [ptp_statistics=$enableval],
    [ptp_statistics=no]
)
AC_MSG_RESULT([$ptp_statistics])
case "$ptp_statistics" in
 yes)
    PTP_STATISTICS="-DPTPD_STATISTICS"
    ;;
esac
AC_SUBST(PTP_STATISTICS)
AM_CONDITIONAL([STATISTICS], [test x$ptp_statistics = xyes])

AC_MSG_CHECKING([ntpd control / failover support])
AC_ARG_ENABLE(
    [ntpdc],
    [AS_HELP_STRING(
	[--enable-ntpdc],
	[Enable control of local NTP daemon and failover to local NTP]
    )],
    [ptp_ntpdc=$enableval],
    [ptp_ntpdc=no]
)
AC_MSG_RESULT([$ptp_ntpdc])
case "$ptp_ntpdc" in
 yes)
    PTP_NTPDC="-DPTPD_NTPDC"
    ;;
esac
AC_SUBST(PTP_NTPDC)
AM_CONDITIONAL([NTPDC], [test x$ptp_ntpdc = xyes])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/ptpd2.8])
AC_CONFIG_FILES([src/ptpd2.conf.5])

AC_OUTPUT
