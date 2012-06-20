#!/bin/bash

##
## use this quick and dirty shell script to manage the flags in dep/startup.c
##
## outputs are:
##  - which flags are duplicated
##  - which flags are still available to use (set is: a..z, A..Z, ?)
##  - the getopt string to put in the beginning of ptpdStartup()
## 
## limitations: only the internal help text is parsed. Explanations are assumed to start at position 19.
##

TMP_BASE="/tmp/tmp_ptp_startup"
FILE="dep/startup.c"
TMP1="${TMP_BASE}.1"
TMP2="${TMP_BASE}.2"
TMP3="${TMP_BASE}.3"

L=`cat "$FILE" | awk '/GETOPT_START_OF_OPTIONS/{A=1} /GETOPT_END_OF_OPTIONS/{A=0;} {if(A){print}}' | cut -b1-23 `
A=`echo "$L" | awk -F"\"" '{print $2}' | awk '{print $1}'| awk '/^-/' | awk '{print $1}' | tr -d "-" | sort`
B=`echo "?" {a..z} {A..Z} | sed 's/ /\n/g' | sort `

echo "$A" > "$TMP1"
echo "$A" | sort -u > "$TMP2"
echo "$B" > "$TMP3"

C=` echo "$L" | awk -F"\"" '{print $2 $3}' | awk '/^[ ]*-/' | cut -b1-18 `
CC=`echo "$C" | awk 'BEGIN{print ""}; {print $1; if(NF>1){print ":"}}' | paste -s -d "" | sed 's/\-//g'`

echo "Possible Duplicated:   `cat "$TMP1" | uniq -d | paste -s `"
echo "Available flags:       `diff  -d "$TMP2" "$TMP3"  | grep ">" | awk '{print $2}'| paste -s`"
echo "GetOpt String:                const char *getopt_string = \"$CC\";"




exit 0

