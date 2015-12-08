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
ifdef([AC_PROG_CC_C99], [AC_PROG_CC_C99])
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

# Check for svnversion, cut and tr

AC_PATH_PROG(cutpath, cut)
AC_PATH_PROG(trpath, tr)
AC_PATH_PROG(datepath, date)

if test -n "$datepath"; then
    build_date=`$datepath +%Y%m%d`
    AC_DEFINE_UNQUOTED(BUILD_DATE, ["$build_date"], [build date year-month-day])
fi

# Set CODE_REVISION if svnversion found, version is suffixed with "-svn" and code is versioned
if test -n "$cutpath" && test -n "$trpath"; then
    ver_prefix=`echo VERSION_NUMBER | $cutpath -s -d- -f2`
    AC_PATH_PROG(svnverpath, svnversion)
    if test "$ver_prefix" = "svn" && test -n "$svnverpath"; then
	svn_rev=`$svnverpath 2>/dev/null | $cutpath -s -d: -f2 | $trpath -d MS`
	if test -n "$svn_rev"; then
	    AC_DEFINE_UNQUOTED(CODE_REVISION, ["$svn_rev"], [source code revision])
	else
	svn_rev=`$svnverpath 2>/dev/null | $trpath -d MS`
	    if test -n "$svn_rev"; then
		AC_DEFINE_UNQUOTED(CODE_REVISION, ["$svn_rev"], [source code revision])
	    fi
	fi
    fi
fi

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
AC_SEARCH_LIBS([pow], [m])
AC_SEARCH_LIBS([clock_gettime], [rt])
AC_SEARCH_LIBS([timer_create], [rt])
AC_SEARCH_LIBS([connect], [socket])
AC_SEARCH_LIBS([gethostbyname], [nsl])

# Checks for header files.
AC_HEADER_STDC

AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h net/ethernet.h netinet/in.h netinet/in_systm.h netinet/ether.h sys/uio.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/sockio.h ifaddrs.h sys/time.h syslog.h unistd.h glob.h sched.h utmp.h utmpx.h unix.h linux/rtc.h sys/timex.h getopt.h])

AC_CHECK_HEADERS([endian.h machine/endian.h sys/isa_defs.h])

AC_CHECK_HEADERS([math.h],[],[AC_MSG_ERROR([math.h is required to compile PTPd])],[])

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
AC_CHECK_MEMBERS([struct utmp.ut_time], [], [], [
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
])
AC_CHECK_MEMBERS([struct timex.tick], [], [], [[#include <sys/timex.h>]])
AC_CHECK_MEMBERS([struct timex.tai], [], [], [[#include <sys/timex.h>]])
AC_CHECK_MEMBERS([struct ntptimeval.tai], [], [],
[
#include <sys/time.h>
#include <sys/timex.h>
])


# Check for ifr_hwaddr in the ifreq structure
AC_CHECK_MEMBERS([struct ifreq.ifr_hwaddr], [], [], [[#include <net/if.h>]])

# Check for ifr_index in the ifreq structure
AC_CHECK_MEMBERS([struct ifreq.ifr_index], [], [], [[#include <net/if.h>]])

# Check for ifr_ifindex in the ifreq structure
AC_CHECK_MEMBERS([struct ifreq.ifr_ifindex], [], [], [[#include <net/if.h>]])

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

# Checks for library functions
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([clock_gettime dup2 ftruncate gethostbyname2 gettimeofday inet_ntoa memset pow select socket strchr strdup strerror strtol glob pututline utmpxname updwtmpx setutent endutent signal ntp_gettime getopt_long])

if test -n "$GCC"; then
    AC_MSG_CHECKING(if GCC -fstack-protector is usable)
    OLDCFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -fstack-protector"
    AC_TRY_LINK(,,[
	AC_MSG_RESULT(yes)
    ], [
	AC_MSG_RESULT(no)
	CFLAGS="$OLDCFLAGS"
    ])
fi


AC_MSG_NOTICE([************************************************************])
AC_MSG_NOTICE([* PTPD BUILD FLAG AND LIBRARY DEPENDENCY CHECKS START HERE *])
AC_MSG_NOTICE([************************************************************])

AC_CHECK_DECLS([MSG_ERRQUEUE], [], [], [[#include <sys/socket.h>]])

AC_CHECK_DECLS([POSIX_TIMERS_SUPPORTED], [posix_timers=true], [posix_timers=false], [
#ifdef __sun && !defined(_XPG6)
#define _XPG6
#endif
#include <unistd.h>
#if _POSIX_TIMERS && (_POSIX_TIMERS - 200112L) >= 0
#define POSIX_TIMERS_SUPPORTED
#else
#endif
])

# ALL USER-CONTROLLED KNOBS START HERE

AC_ARG_ENABLE([posix_timers],
	    AS_HELP_STRING( [--disable-posix-timers (enabled by default if supported)],
			    [Disable POSIX timer support and use interval timers even if POSIX timers supported by the OS])
	    )

AS_IF([test "x$enable_posix_timers" == "xno"], [
posix_timers=false
])

AM_CONDITIONAL([PTIMERS], [test x$posix_timers = xtrue ])

AC_MSG_CHECKING([if we want to build POSIX timer support])

case "$posix_timers" in
     "true")
	PTP_PTIMERS="-DPTPD_PTIMERS"
	AC_MSG_RESULT([yes])
	;;
     *) PTP_PTIMERS=""
	AC_MSG_RESULT([no])
	;;
esac

AC_SUBST(PTP_PTIMERS)


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
	[disable support for PCAP (enabled by default / if we find pcap-config)]
    )],
    [try_pcap=$enableval],
    [try_pcap=yes]
)

if test "x$try_pcap" = "xyes"; then

case "$PATH_PCAP_CONFIG" in
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
esac

fi

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
 no)
			PTP_PCAP=""
			PCAP_LIBS=""
			PCAP_CPPFLAGS=""
			PCAP_CFLAGS=""
			AC_MSG_RESULT([no])
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



AC_MSG_CHECKING([for RUNTIME_DEBUG])
AC_ARG_ENABLE(
    [runtime-debug],
    [AS_HELP_STRING(
	[--enable-runtime-debug (disabled by default)],
	[Enable all debug messages]
    )],
    [],
    [enable_runtime_debug=no]
)
AC_MSG_RESULT([$enable_runtime_debug])
case "$enable_runtime_debug" in
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

AC_MSG_CHECKING([for maximum unicast destination table size])
AC_ARG_WITH(
    [max-unicast-destinations],
    [AS_HELP_STRING(
	[--with-max-unicast-destinations = [ 16 .. 2048 (default: 128)]],
	[Change maximum supported number of unicast destinations - 
	    this determines the maximum supported number of slaves 
	    in unicast mode (with and without signaling)]
    )],
    [max_destinations=$with_max_unicast_destinations],
    [max_destinations=128]
)

test $max_destinations -lt 16 && max_destinations=16
test $max_destinations -gt 2048 && max_destinations=2048

PTP_UNICAST_MAX="-DPTPD_UNICAST_MAX=$max_destinations"
AC_SUBST([PTP_UNICAST_MAX])
AC_MSG_RESULT([$max_destinations])

AC_MSG_CHECKING([for daemon mode])
AC_ARG_ENABLE(
    [daemon],
    [AS_HELP_STRING(
	[--disable-daemon (enabled by default)],
	[Disable daemon mode]
    )],
    [],
    [enable_daemon=yes]
)
AC_MSG_RESULT([$enable_daemon])
case "$enable_daemon" in
 no)
    PTP_DAEMON="-DPTPD_NO_DAEMON"
    ;;
esac
AC_SUBST(PTP_DAEMON)

AC_MSG_CHECKING([for experimental options])
AC_ARG_ENABLE(
    [experimental-options],
    [AS_HELP_STRING(
	[--enable-experimental-options (disabled by default)],
	[Enable experimental options]
    )],
    [],
    [enable_experimental_options=no]
)
AC_MSG_RESULT([$enable_experimental_options])
case "$enable_experimental_options" in
 yes)
    PTP_EXP="-DPTPD_EXPERIMENTAL"
    ;;
esac
AC_SUBST(PTP_EXP)
AM_CONDITIONAL([EXP], [test x$enable_experimental_options = xyes])

AC_MSG_CHECKING([for realtime statistics support])
AC_ARG_ENABLE(
    [statistics],
    [AS_HELP_STRING(
	[--disable-statistics (enabled by default)],
	[Disable support for realtime statistics and statistics based filtering]
    )],
    [],
    [enable_statistics=yes]
)
AC_MSG_RESULT([$enable_statistics])
case "$enable_statistics" in
 yes)
    PTP_STATISTICS="-DPTPD_STATISTICS"
    ;;
esac
AC_SUBST(PTP_STATISTICS)

AM_CONDITIONAL([STATISTICS], [test x$enable_statistics = xyes])

AC_MSG_CHECKING([if we want to enable SO_TIMESTAMPING support on Linux if available])
AC_ARG_ENABLE(
    [so-timestamping],
    [AS_HELP_STRING(
	[--disable-so-timestamping (enabled by default)],
	[Disable support for SO_TIMESTAMPING on Linux builds]
    )],
    [],
    [enable_so_timestamping=yes]
)
AC_MSG_RESULT([$enable_so_timestamping])
case "$enable_so_timestamping" in
 no)
    PTP_DISABLE_SOTIMESTAMPING="-DPTPD_DISABLE_SOTIMESTAMPING"
    ;;
esac
AC_SUBST(PTP_DISABLE_SOTIMESTAMPING)

AM_CONDITIONAL([DISABLE_SOTIMESTAMPING], [test x$enable_so_timestamping = xno])

AC_MSG_CHECKING([if we're building a slave-only build])

AC_ARG_ENABLE(
    [slave-only],
    [AS_HELP_STRING(
	[--enable-slave-only (disabled by default)],
	[Build a slave-only version incapable of running as PTP master]
    )],
    [],
    [enable_slave_only=no]
)
AC_MSG_RESULT([$enable_slave_only])
case "$enable_slave_only" in
 yes)
    PTP_SLAVE_ONLY="-DPTPD_SLAVE_ONLY"
    ;;
esac

AC_SUBST(PTP_SLAVE_ONLY)
AM_CONDITIONAL([SLAVE_ONLY], [test x$enable_slave_only = xyes])

AC_MSG_NOTICE([************************************************************])
AC_MSG_NOTICE([*   END OF PTPD BUILD FLAG AND LIBRARY DEPENDENCY CHECKS   *])
AC_MSG_NOTICE([************************************************************])


AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/ptpd2.8])
AC_CONFIG_FILES([src/ptpd2.conf.5])

AC_OUTPUT
