# Makefile for ptpd

RM = rm -f
CFLAGS+= -Wall # -DBSD_INTERFACE_FUNCTIONS
#CPPFLAGS =   -DPTPD_DBGV -DPTPD_NO_DAEMON
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
