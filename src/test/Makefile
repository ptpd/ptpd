#
# Copyright (c) 2011      George V. Neville-Neil, Steven Kreuzer,
#                         Martin Burnicki, Gael Mace, Alexandre Van Kempen,
#                         National Instruments.
# Copyright (c) 2009-2010 George V. Neville-Neil, Steven Kreuzer,
#                         Martin Burnicki, Gael Mace, Alexandre Van Kempen
# Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
#
# All Rights Reserved
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# Makefile for ptpd2 tests

#
# Compile time defines:
#   -DRUNTIME_DEBUG
#     if defined:    all debug messages are included, and are either selected at startup (-B) and runtime (SIGUSR2) (implies -DPTPD_DBGV)
#                    using syslog, add "*.debug /var/log/debug" to /etc/rsyslog.conf to see debug messages
#     if undefined:  individual defines select which messages will be included and presented (old behaviour)
#
#  -DPTPD_DBG     basic debug messages
#  -DPTPD_DBG2    adds basic protocol messages, +ignored messages reasons
#  -DPTPD_DBGV    adds all debug messages
#
#   -DPTPD_NO_DAEMON             forces option -c
#   -DBSD_INTERFACE_FUNCTIONS    for OpenBSD
#
#   -PTP_EXPERIMENTAL            Allows non-standard compliant experimental options:
#                                   -U: Hybrid mode
#                                   -I: choose multicast group
#
# Mutually exclusive defines (useful for testing):
#   -DDBG_SIGUSR2_CHANGE_DOMAIN: SIGUSR2 cycles the PTP domain number
#   -DDBG_SIGUSR2_CHANGE_DEBUG:  SIGUSR2 cycles the current debug level (if RUNTIME_DEBUG is defined)
#
#######

RM = rm -f
SYMLINK = ln -s

.SUFFIXES: .o1

CFLAGS = -Wall -g -fPIC
#CFLAGS += -DRUNTIME_DEBUG
#CFLAGS += -DPTPD_DBG
#CFLAGS += -DPTPD_DBG2
#CFLAGS += -DPTPD_DBGV

#CFLAGS += -DPTPD_NO_DAEMON
#CFLAGS += -DBSD_INTERFACE_FUNCTIONS
#CFLAGS += -DDBG_SIGUSR2_CHANGE_DOMAIN
#CFLAGS += -DDBG_SIGUSR2_CHANGE_DEBUG

#CFLAGS += -DPTP_EXPERIMENTAL

LDFLAGS = -lm -lrt

TEST = ptpd2test

SRCS = test.c init.c ../arith.c ../bmc.c ../protocol.c ../display.c ../management.c \
	../dep/msg.c ../dep/net.c ../dep/servo.c ../dep/startup.c ../dep/sys.c ../dep/timer.c
TESTSRCS = $(SRCS)

OBJS = $(TESTSRCS:.c=.o1)

HDRS = ../ptpd.h ../constants.h ../datatypes.h \
	../dep/ptpd_dep.h ../dep/constants_dep.h ../dep/datatypes_dep.h \
	init.h helper.h

.c.o1:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

$(TEST): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

all: $(TEST)

$(OBJS): $(HDRS)

clean:
	$(RM) $(TEST) $(OBJS) make.out
