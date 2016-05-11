#!/bin/bash

function dodir() {

    pushd .
    cd $1
    pwd
    ../codegen_standardtlv.sh
    popd

}

dodir signalingTlv
dodir otherTlv

cd managementTlv
./codegen.sh
cd ..


echo "typedef struct {"
grep -Rl @tlvtype@ --include *.def | sort | { while read file; do base=`basename $file | awk 'BEGIN{ FS=".";} { printf("PtpTlv%s%s\t\t\t%s,", toupper(substr($1,1,1)), substr($1,2 ), $1);}'`; echo $base; done } | sort
echo "} PtpTlvBody;"
