# Makefile for ptpd2

#
# Compile time defines:
#   -DRUNTIME_DEBUG
#     if defined:    all debug messages are included, and are either selected at startup (-B)
#                    and runtime (SIGUSR2) (implies -DPTPD_DBGV) using syslog, 
#                    add "*.debug /var/log/debug" to /etc/rsyslog.conf to see debug messages
#     if undefined:  individual defines select which messages will be included and presented (old behaviour)
#
#  -DPTPD_DBG     basic debug messages
#  -DPTPD_DBG2    adds basic protocol messages, +ignored messages reasons
#  -DPTPD_DBGV    adds all debug messages
#
#   -DPTPD_NO_DAEMON             forces option -c
#
#   -DPTPD_EXPERIMENTAL           Allows non-standard compliant experimental options:
#                                   -U: Hybrid mode
#                                   -I: choose multicast group
#   -DPTPD_SNMP                   Enables SNMP agent support (PTP-MIB)
#
# Mutually exclusive defines (useful for testing):
#   -DDBG_SIGUSR2_CHANGE_DOMAIN: SIGUSR2 cycles the PTP domain number
#   -DDBG_SIGUSR2_CHANGE_DEBUG:  SIGUSR2 cycles the current debug level (if RUNTIME_DEBUG is defined)
#
#######

VERSION = 2.3.0

RM = rm -f
SYMLINK = ln -s

# SNMP support?
SNMP = yes

# start with CFLAGS += ..., so additional CFLAGs can be specified e.g. on the make command line
CFLAGS += -Wall -g -fPIC

CFLAGS += -DRUNTIME_DEBUG
#CFLAGS += -DPTPD_DBG
#CFLAGS += -DPTPD_DBG2
#CFLAGS += -DPTPD_DBGV

#CFLAGS += -DPTPD_NO_DAEMON
#CFLAGS += -DDBG_SIGUSR2_CHANGE_DOMAIN
CFLAGS += -DDBG_SIGUSR2_CHANGE_DEBUG

CFLAGS += -DPTPD_EXPERIMENTAL

LDFLAGS = -lm -lrt
STATICLIBFLAGS = rcs
SHAREDLIBFLAGS = -shared -Wl,-soname

PROG = ptpd2
STATICLIB = libptpd2.a
SHAREDLIB = libptpd2.so
SHAREDLIBVER = $(SHAREDLIB).$(VERSION)

SRCS = ptpd.c arith.c bmc.c protocol.c display.c management.c \
	dep/msg.c dep/net.c dep/servo.c dep/startup.c dep/sys.c dep/timer.c

# SNMP
ifeq ($(SNMP),yes)
LDFLAGS += `net-snmp-config --agent-libs`
CFLAGS  += `net-snmp-config --base-cflags`
CFLAGS  += -DPTPD_SNMP
SRCS    += dep/snmp.c
endif

OBJS = $(SRCS:.c=.o)

HDRS = ptpd.h constants.h datatypes.h \
	dep/ptpd_dep.h dep/constants_dep.h dep/datatypes_dep.h

CSCOPE = cscope
GTAGS = gtags
DOXYGEN = doxygen

TAGFILES = GPATH GRTAGS GSYMS GTAGS cscope.in.out cscope.out cscope.po.out

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<


$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(STATICLIB): $(OBJS)
	$(AR) $(STATICLIBFLAGS) $(STATICLIB) $(OBJS)

$(SHAREDLIB): $(OBJS)
	$(CC) $(SHAREDLIBFLAGS),$(SHAREDLIB) -o $(SHAREDLIB) $(OBJS)
        # create shared lib symlink with ptpd version extension
	$(SYMLINK) $(SHAREDLIB) $(SHAREDLIBVER)

all: $(PROG) $(STATICLIB) $(SHAREDLIB)

$(OBJS): $(HDRS)

tags:
	$(CSCOPE) -R -q -b
	$(GTAGS)
	$(DOXYGEN) Doxyfile

clean:
	$(RM) $(PROG) $(STATICLIB) $(SHAREDLIB) $(SHAREDLIBVER) $(OBJS) $(TAGFILES) make.out

realclean: clean
	$(RM) $(TAGFILES)
