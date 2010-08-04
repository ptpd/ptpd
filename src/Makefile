# Makefile for ptpd

RM = rm -f
CFLAGS = -Wall -g -DBSD_INTERFACE_FUNCTIONS 
#CFLAGS += -DPTPD_DBG -DPTPD_NO_DAEMON

PROG = ptpd
SRC  = ptpd.c arith.c bmc.c probe.c protocol.c \
	dep/msg.c dep/net.c dep/servo.c dep/startup.c dep/sys.c dep/timer.c
OBJ  = ptpd.o arith.o bmc.o probe.o protocol.o \
	dep/msg.o dep/net.o dep/servo.o dep/startup.o dep/sys.o dep/timer.o
HDR  = ptpd.h constants.h datatypes.h \
	dep/ptpd_dep.h dep/constants_dep.h dep/datatypes_dep.h


.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

all: $(PROG)

$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

$(OBJ): $(HDR)

clean:
	$(RM) $(PROG) $(OBJ)

cflow:
	cflow -r main -d 4 $(HDR) $(SRC)  > cflow.out
	cflow2dot < cflow.out > cflow.dot
	dot -Grankdir=BT -Tjpg < cflow.dot > cflow.jpg