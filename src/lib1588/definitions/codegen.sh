#!/bin/bash

function dodir() {

    pushd .
    cd $1
    pwd
    ./codegen.sh
    popd

}

dodir managementTlv
dodir signalingTlv
dodir otherTlv
