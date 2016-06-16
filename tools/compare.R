#!/usr/bin/env Rscript
# Copyright (c) 2013, Neville-Neil Consulting
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
# Description: A tool to graph and compare the offset vs. the master to slave
# delay vs. the slave to mater delay vs. the calculated delay from a
# PTPd output file generated with the -D flag.
#
# Usage: compare.R input_file input_file2 [value] [output_file]
#
# Value is one of: offset [default value], delay, master.to.slave, slave.to.master
#

source("ptplib.R")

argv <- commandArgs(TRUE)

file1 = argv[1]
file2 = argv[2]
value = argv[3]
outputFile = argv[4]

if (is.na(outputFile))
    outputFile = paste(basename(file2), ".png", sep="")
  
logA = ptpLogRead(file1)
logB = ptpLogRead(file2)

if (is.na(value)) {
    ptpCompare(logA, logB, output=outputFile)
} else {
    ptpCompare(logA, logB, value, outputFile)
}
