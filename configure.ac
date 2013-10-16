#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_include([m4/version.m4])
AC_PREREQ(2.61)
AC_INIT(
    [ptpd2],
    [VERSION_NUMBER],
    [ptpd-bugs@nwtime.org],
    [],
    [PTPD_URL]dnl
)
AC_CONFIG_SRCDIR([src/arith.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10 foreign -Wall])

[RELEASE_DATE]="RELEASE_DATE"
AC_SUBST([RELEASE_DATE])
[VERSION_NUMBER]="VERSION_NUMBER"
AC_SUBST([VERSION_NUMBER])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_LIBTOOL

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

# Checks for libraries.
AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([rt], [clock_gettime])
AC_CHECK_LIB(
    [pcap],
    [pcap_inject],
    [],
    [AC_MSG_ERROR([Cannot find libpcap - required for pcap / BPF timestamping!])]
)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h netinet/ether.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h glob.h sched.h utmp.h utmpx.h pcap/pcap.h pcap.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_HEADER_TIME
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_C_VOLATILE

# Check for tick in the timex structure
AC_CHECK_MEMBERS([struct timex.tick], [], [], [[#include <sys/timex.h>]])

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([clock_gettime dup2 ftruncate gettimeofday inet_ntoa memset pow select socket strchr strdup strerror strtol glob pututline utmpxname updwtmpx])

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
	    [--enable-debug-level={basic,medium,all}],
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

AC_MSG_CHECKING([for SIGUSR2 support])
AC_ARG_ENABLE(
    [sigusr2],
    [AS_HELP_STRING(
	[--enable-sigusr2={domain,debug,counters}],
	[Enable SIGUSR2 support to: cycle PTP domain #, debug level or dump PTP engine counters (disabled by default)]
    )],
    [ptp_sigusr2=$enableval],
    [ptp_sigusr2=no]
)
AC_MSG_RESULT([$ptp_sigusr2])
case "$ptp_sigusr2" in
 domain)
    PTP_SIGUSR2="-DDBG_SIGUSR2_CHANGE_DOMAIN"
    ;;
 debug)
    # We could/should check to be sure that RUNTIME_DEBUG is set...
    PTP_SIGUSR2="-DDBG_SIGUSR2_CHANGE_DEBUG"
    ;;
 counters)
    PTP_SIGUSR2="-DDBG_SIGUSR2_DUMP_COUNTERS"
    ;;
esac
AC_SUBST(PTP_SIGUSR2)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/ptpd2.8])
AC_CONFIG_FILES([src/ptpd2.conf.5])
AC_CONFIG_FILES([src/ptpd2.conf.8])

AC_OUTPUT
