#!/bin/sh

if [ ! -z "$DEBUG" ]; then
    set -x
fi

# ptpd RPM building script
# (c) 2013-2015: Wojciech Owczarek, PTPd project

while [[ $# -gt 0 ]]; do
    opt="$1";
    shift;              #expose next argument
    case "$opt" in
        "--tag" )
            # Add git-based tag
            ADD_TAG=1;;
        "--service2" )
            # Create ptpd2 service
            SERVICESUFFIX='2';;
        *) cat >&2 <<EOF
Invalid option: $opt (remainder $@)
Supported options:
    --tag: add a git-based tag to the rpm release
    --service2: generate the service as ptpd2 (instead of default ptpd)
EOF
           exit 1;;
    esac
done


SPEC=ptpd.spec
PWD=`pwd`
if [ `basename $PWD` != 'rpm-rh' ]; then
    # deleting rpm is dangerous
    echo "Can only be run from packagebuild/rpm-rh subdir in repo"
    exit 1
fi
rm *.rpm

for dep in rpm-build `grep BuildRequires ptpd.spec | sed "s/^.*: //" |grep -v systemd | tr '\n' ' '`; do
    rpm -q $dep --quiet || { echo "Error: $dep not installed"; exit 1; }
done

# build both versions: normal and slave-only
for slaveonly in 0 1; do

    cd $PWD
    BUILDDIR=`mktemp -d /tmp/tmpbuild.XXXXXXXXX`

    rm -rf $BUILDDIR


    for dir in BUILD BUILDROOT RPMS SOURCES SPECS SRPMS; do
        mkdir -p $BUILDDIR/$dir;
    done


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

    if [ -z "$ADD_TAG" ]; then
        TAG_ARG='__gittag notag' # cannot be empty
    else
        tag=`git log --format=.%ct.%h -1` # based on last commit, timestamp for increasing versions
        TAG_ARG="gittag $tag"
    fi

    if [ -z "$SERVICESUFFIX" ]; then
        SERVICESUFFIX_ARG='__servicesuffix nosuffix' # cannot be empty
    else
        SERVICESUFFIX_ARG="servicesuffix $SERVICESUFFIX"
    fi


    rpmbuild --define "$TAG_ARG" --define "$SERVICESUFFIX_ARG" --define "build_slaveonly $slaveonly" --define "_topdir $BUILDDIR" -ba $BUILDDIR/SPECS/$SPEC && {
        find $BUILDDIR -name "*.rpm" -exec mv {} $PWD \;
    }

    rm -rf $BUILDDIR
    rm $TARBALL

    echo "Moved rpms to $PWD"
done
