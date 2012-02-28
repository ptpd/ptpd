#!/usr/local/bin/Rscript --slave
# Copyright (c) 2012, Neville-Neil Consulting
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# Neither the name of Neville-Neil Consulting nor the names of its 
# contributors may be used to endorse or promote products derived from 
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Author: George V. Neville-Neil
#
# Description: A tool, written in R, that will process a PTPd log and
# give various statistics about PTP performance.  Statistics are given
# for the offset from master (Offset), one way delay (Delay), Master
# to Slave and Slave to Master timings.  An argument is required which
# is a raw ptpd log produced by the -D flag to ptpd2.

argv <- commandArgs(TRUE)

file = argv[1]

ptplog = read.table(file, fill=TRUE, sep=",", col.names=c("timestamp", "state", "clockID", "delay", "offset", "master.to.slave", "slave.to.master", "drift", "packet"), blank.lines.skip=TRUE, header=FALSE, skip=100)
cat("Offset",
    "\nmin:", min(ptplog$offset, na.rm=TRUE),
    " max: ", max(ptplog$offset, na.rm=TRUE),
    " median: ", median(ptplog$offset, na.rm=TRUE),
    " mean: ", mean(ptplog$offset, na.rm=TRUE),
    "\nstd dev: ", sd(ptplog$offset, na.rm=TRUE),
    " variance: ", var(ptplog$offset, na.rm=TRUE), "\n")
cat("Delay",
    "\nmin:", min(ptplog$delay, na.rm=TRUE),
    " max: ", max(ptplog$delay, na.rm=TRUE),
    " median: ", median(ptplog$delay, na.rm=TRUE),
    " mean: ", mean(ptplog$delay, na.rm=TRUE),
    "\nstd dev: ", sd(ptplog$delay, na.rm=TRUE),
    " variance: ", var(ptplog$delay, na.rm=TRUE), "\n")
cat("Master -> Slave",
    "\nmin:", min(ptplog$master.to.slave, na.rm=TRUE),
    " max: ", max(ptplog$master.to.slave, na.rm=TRUE),
    " median: ", median(ptplog$master.to.slave, na.rm=TRUE),
    " mean: ", mean(ptplog$master.to.slave, na.rm=TRUE),
    "\nstd dev: ", sd(ptplog$master.to.slave, na.rm=TRUE),
    " variance: ", var(ptplog$master.to.slave, na.rm=TRUE), "\n")
cat("Slave -> Master",
    "\nmin:", min(ptplog$slave.to.master, na.rm=TRUE),
    " max: ", max(ptplog$slave.to.master, na.rm=TRUE),
    " median: ", median(ptplog$slave.to.master, na.rm=TRUE),
    " mean: ", mean(ptplog$slave.to.master, na.rm=TRUE),
    "\nstd dev: ", sd(ptplog$slave.to.master, na.rm=TRUE),
    " variance: ", var(ptplog$slave.to.master, na.rm=TRUE), "\n")
