# Makefile for ptpd

CFLAGS = -Wall
#CPPFLAGS = -DPTPD_DBG

executable = ptpd
objects = ptpd.o arith.o bmc.o probe.o protocol.o
headers = ptpd.h constants.h datatypes.h

dep_dir = dep/
dep_objects = msg.o net.o servo.o startup.o sys.o timer.o
dep_headers = ptpd_dep.h constants_dep.h datatypes_dep.h

objects += $(addprefix $(dep_dir),$(dep_objects))
headers += $(addprefix $(dep_dir),$(dep_headers))

all: $(executable)

$(executable): $(objects)

$(objects): $(headers)

clean:
	-@$(RM) $(objects) $(executable)
	
.PHONY: clean
