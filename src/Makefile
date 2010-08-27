# Makefile for ptpd

RM = rm -f
CFLAGS = -Wall # -g -DBSD_INTERFACE_FUNCTIONS
#CPPFLAGS =   -DPTPD_DBGV -DPTPD_NO_DAEMON
LDFLAGS = -lm -lrt

PROG = ptpv2d
OBJ  = ptpd.o arith.o bmc.o protocol.o display.o\
	dep/msg.o dep/net.o dep/servo.o dep/startup.o dep/sys.o dep/timer.o
HDR  = ptpd.h constants.h datatypes.h \
	dep/ptpd_dep.h dep/constants_dep.h dep/datatypes_dep.h

CSCOPE = cscope
GTAGS = gtags
DOXYGEN = doxygen

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

all: $(PROG)

$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

$(OBJ): $(HDR)

tags:
	$(CSCOPE) -R -q -b
	$(GTAGS)
	$(DOXYGEN) Doxyfile

clean:
	$(RM) $(PROG) $(OBJ)
