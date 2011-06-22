# Makefile for ptpd2

#
# Compile time defines:
#  -DPTPD_DBG     basic debug messages
#  -DPTPD_DBG2    adds basic protocol messages, +ignored messages reasons
#  -DPTPD_DBGV    adds all debug messages
#
#  -DPTPD_NO_DAEMON : forces option -c
#  -DBSD_INTERFACE_FUNCTIONS: for OpenBSD
#  -DDBG_SIGUSR2_CHANGE_DOMAIN: Allow SIGUSR2 to cycle the domain, useful for testing
#
#######

RM = rm -f

CFLAGS = -Wall -g
#CFLAGS += -DPTPD_DBG
#CFLAGS += -DPTPD_DBG2
#CFLAGS += -DPTPD_DBGV

#CFLAGS += -DPTPD_NO_DAEMON
#CFLAGS += -DBSD_INTERFACE_FUNCTIONS
#CFLAGS += -DDBG_SIGUSR2_CHANGE_DOMAIN





LDFLAGS+= -lm -lrt


PROG = ptpd2
SRCS = ptpd.c arith.c bmc.c protocol.c display.c\
	dep/msg.c dep/net.c dep/servo.c dep/startup.c dep/sys.c dep/timer.c

OBJS = $(SRCS:.c=.o)

HDRS = ptpd.h constants.h datatypes.h \
	dep/ptpd_dep.h dep/constants_dep.h dep/datatypes_dep.h

CSCOPE = cscope
GTAGS = gtags
DOXYGEN = doxygen

TAGFILES = GPATH GRTAGS GSYMS GTAGS cscope.in.out cscope.out cscope.po.out

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(OBJS): $(HDRS)

tags:
	$(CSCOPE) -R -q -b
	$(GTAGS)
	$(DOXYGEN) Doxyfile

clean:
	$(RM) $(PROG) $(OBJS) $(TAGFILES) make.out
