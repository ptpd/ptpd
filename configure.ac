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

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lrt':
AC_CHECK_LIB([rt], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h])

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

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([clock_gettime dup2 ftruncate gettimeofday inet_ntoa memset pow select socket strchr strdup strerror strtol])

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

AC_MSG_CHECKING([for SNMP agent support])
AC_ARG_ENABLE(
    [snmp],
    [AS_HELP_STRING(
	[--enable-snmp],
	[Enable SNMP agent support (disabled by default)]
    )],
    [ptp_snmp=$enableval],
    [ptp_snmp=no]
)
AC_MSG_RESULT([$ptp_snmp])
case "$ptp_snmp" in
 yes)
    PTP_SNMP="-DPTPD_SNMP"
    ;;
esac

AC_MSG_CHECKING([for SIGUSR2 support])
AC_ARG_ENABLE(
    [sigusr2],
    [AS_HELP_STRING(
	[--enable-sigusr2={domain,debug}],
	[Enable SIGUSR2 support, cycle PTP domain #, or debug level (disabled by default)]
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
esac

AM_CPPFLAGS="$PTP_DBL $PTP_DAEMON $PTP_EXP $PTP_SNMP $PTP_SIGUSR2"
AC_SUBST(AM_CPPFLAGS)

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
