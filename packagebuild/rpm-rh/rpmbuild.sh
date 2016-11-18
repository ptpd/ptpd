#!/bin/sh

# ptpd RPM building script
# (c) 2013-2015: Wojciech Owczarek, PTPd project

# build both versions: normal and slave-only
SPEC=ptpd.spec
rm *.rpm


PWD=`pwd`
BUILDDIR=`mktemp -d /tmp/tmpbuild.XXXXXXXXX`


rm -rf $BUILDDIR

rpm -q rpm-build --quiet || { echo "Error: rpm-build not installed"; exit 1; }
rpm -q libpcap-devel --quiet || { echo "Error: libpcap-devel not installed"; exit 1; }

GITTAG="gittag .git.`git log --format=%h.%ct -1 2>/dev/null`" || GITTAG="__gittag notag"

for dir in BUILD BUILDROOT RPMS SOURCES SPECS SRPMS; do mkdir -p $BUILDDIR/$dir; done



TARBALL=`cat $SPEC | grep ^Source0 | awk '{ print $2; }'`

# hack: dist hook now removes rpms from this dir before dist packaging:
# this is how we preserve them...
mv *.rpm ../..
( cd ../..; autoreconf -vi; ./configure; make dist; )
# and in they go again...
mv ../../*.rpm .

mv ../../$TARBALL .

for sourcefile in `grep ^Source $SPEC | awk '{print $2;}'`; do

    cp -f $sourcefile $BUILDDIR/SOURCES

done

cp -f $SPEC $BUILDDIR/SPECS

ARCHIVE=`cat $SPEC | egrep "Name|Version|Release" | awk 'BEGIN {ORS="-";} NR<3 { print $2;} NR>=3 {ORS=""; print $2}'`
RPMFILE=$BUILDDIR/RPMS/`uname -m`/$ARCHIVE.`uname -m`.rpm
SRPMFILE=$BUILDDIR/SRPMS/$ARCHIVE.src.rpm

for slaveonly in 0 1; do
    rpmbuild  --define "$GITTAG" --define "build_slaveonly $slaveonly" --define "_topdir $BUILDDIR" -ba $BUILDDIR/SPECS/$SPEC && {
    find $BUILDDIR -name "*.rpm" -exec mv {} $PWD \;
    }
done

rm -rf $BUILDDIR
rm $TARBALL

done
