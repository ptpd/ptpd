#!/bin/sh

PWD=`pwd`
BUILDDIR=`mktemp -d $PWD/tmpbuild.XXXXXXXXX`
SPEC=ptpd.spec

rm -rf $BUILDDIR

rpm -q rpm-build --quiet || { echo "Error: rpm-build not installed"; exit 1; }
rpm -q libpcap-devel --quiet || { echo "Error: libpcap-devel not installed"; exit 1; }


for dir in BUILD BUILDROOT RPMS SOURCES SPECS SRPMS; do mkdir -p $BUILDDIR/$dir; done


TARBALL=`cat $SPEC | grep ^Source0 | awk '{ print $2; }'`

( cd ../..; ./configure; make dist; )

mv ../../$TARBALL .

for sourcefile in `grep ^Source $SPEC | awk '{print $2;}'`; do

    cp -f $sourcefile $BUILDDIR/SOURCES

done

cp -f $SPEC $BUILDDIR/SPECS

ARCHIVE=`cat $SPEC | egrep "Name|Version|Release" | awk 'BEGIN {ORS="-";} NR<3 { print $2;} NR>=3 {ORS=""; print $2}'`
RPMFILE=$BUILDDIR/RPMS/`uname -m`/$ARCHIVE.`uname -m`.rpm
SRPMFILE=$BUILDDIR/SRPMS/$ARCHIVE.src.rpm

rm *.rpm

rpmbuild --define "_topdir $BUILDDIR" -ba $BUILDDIR/SPECS/$SPEC && { 
    find $BUILDDIR -name "*.rpm" -exec mv {} $PWD \;
}

rm -rf $BUILDDIR
rm $TARBALL
