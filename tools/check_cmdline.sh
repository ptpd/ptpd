#!/bin/bash

#
# This simple script helps manage the command line options defined in startup.c:
#
# - getopt string
# - case statements (handling options)
# - help output
#
# This tool is meant to complement / replace the src/check_startup.sh script
#
# The sript will show which options are used, which aren't and where,
# prints out a visual comparison and detects duplicates, to minimise
# false matches, only the ptpdStartup() function is analysed, under
# the assumption that it's the last function in the file.
#
# Todo: reconstruct / suggest getopt string
#
# Wojciech Owczarek, 2012, ptpd.sf.net
#
export LC_ALL=C

SOURCEFILE="../src/dep/startup.c"

START_LINE=`grep -n "ptpdStartup(int"  $SOURCEFILE  | cut -d ":" -f 1`
TOTAL_LINES=`wc -l < $SOURCEFILE`

TMPFILE=`mktemp ptpd_check_cmdlineXXXXXXXX`
tail -n $[ $TOTAL_LINES - $START_LINE ] $SOURCEFILE > $TMPFILE

echo -e "\n=======\n"
ALPHABET=`echo ? {A..Z} {a..z}`
GETOPT_HANDLED=`grep -e "case '[a-zA-Z]'" $TMPFILE | cut -d "'" -s -f 2 | sort | tr "\n\r" " "`
GETOPT_DEFINED=`grep "const char \*getopt_string" $TMPFILE | cut -d '"' -f 2 | tr -d ":" | grep -o . | sort | tr "\n\r" " "`
HELP_HANDLED=`grep -e "^[[:space:]]*\"-[a-zA-Z] " $TMPFILE | cut -d '"' -f 2 | tr -d "-" | cut -d " " -f 1 | sort | tr "\n\r" " "`

echo -e "getopt not defined:\t " `echo $ALPHABET | tr -d "$GETOPT_DEFINED" | sed 's/[ ]*/ /g'`
echo -e "getopt not handled:\t " `echo $ALPHABET | tr -d "$GETOPT_HANDLED" | sed 's/[ ]*/ /g'`
echo -e "  help not handled:\t " `echo $ALPHABET | tr -d "$HELP_HANDLED" | sed 's/[ ]*/ /g'`
echo -e "\n=======\n"
echo -e -n  "options usable:\t | "
echo $ALPHABET
echo -e -n    "getopt defined:\t | "
echo $ALPHABET | tr -c `echo $GETOPT_DEFINED | tr -d " "` " "
echo -e -n  "\ngetopt handled:\t | "
echo $ALPHABET | tr -c `echo $GETOPT_HANDLED | tr -d " "` " "
echo -e -n  "\nhelp  handled: \t | "
echo $ALPHABET | tr -c `echo $HELP_HANDLED | tr -d " "` " "
echo -e "\n\n=======\n"
echo -e "getopt defined duplicates:\t "`echo $GETOPT_DEFINED | tr -d " " | grep -o . | sort | uniq -d | tr "\r\n" " "`
echo -e "getopt handled duplicates:\t "`echo $GETOPT_HANDLED | tr -d " " | grep -o . | sort | uniq -d | tr "\r\n" " "`
echo -e "  help handled duplicates:\t "`echo $HELP_HANDLED | tr -d " " | grep -o . | sort | uniq -d | tr "\r\n" " "`
echo

rm $TMPFILE

exit 0
