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
# Description: A tool to graph the offset vs. the master to slave
# delay vs. the slave to mater delay vs. the calculated delay from a
# PTPd output file generated with the -D flag.
#
# Usage: graph.R input_file [output_file]
#

argv <- commandArgs(TRUE)

file = argv[1]
output = argv[2]
if (is.na(output))
  output = paste(basename(file), ".png", sep="")
  
ptplog = read.table(file, fill=TRUE, sep=",", col.names=c("timestamp", "state", "clockID", "delay", "offset", "master.to.slave", "slave.to.master", "drift", "packet"), blank.lines.skip=TRUE, header=FALSE, skip=100) 
ymin = min(min(ptplog$offset, na.rm=TRUE), min(ptplog$delay, na.rm=TRUE),
  min(ptplog$master.to.slave, na.rm=TRUE),
  min(ptplog$slave.to.master, na.rm=TRUE))
ymax = max(max(ptplog$offset, na.rm=TRUE), max(ptplog$delay, na.rm=TRUE),
  max(ptplog$master.to.slave, na.rm=TRUE),
  max(ptplog$slave.to.master, na.rm=TRUE))
png(filename=output, height=960, width=1280, bg="white")
plot(ptplog$delay, y=NULL, xaxt = "n" ,type="n", ylim=range(ymin, ymax),
     main="PTP Results", xlab="Time", ylab="Nanoseconds")
legend(100, ymax,
       c("Delay", "Offset", "M->S", "S->M"), col=c("orange", "blue", "red", "green"), pch=21:24)
points(ptplog$offset, y=NULL, cex=.1, col="blue", pch=21)
points(ptplog$master.to.slave, y=NULL, cex=.1, col="red", pch=22)
points(ptplog$slave.to.master, y=NULL, cex=.1, col="green", pch=23)
points(ptplog$delay, y=NULL, cex=.1, col="orange", pch=24)
axis(1, at=ptplog$timestamp, labels=ptplog$timestamp, )
