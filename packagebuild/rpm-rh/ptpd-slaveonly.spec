# (c) 2014-2015:  Wojciech Owczarek, PTPd project

%define _use_internal_dependency_generator 0

# RHEL5.5 and older don't have the /etc/rpm/macros.dist macros
%if %{?dist:1}%{!?dist:0}
%define distver %{dist}
%else
%define distver .el%(/usr/lib/rpm/redhat/dist.sh --distnum)
%endif

Summary: Synchronises system time using the Precision Time Protocol (PTP) implementing the IEEE 1588-2008 (PTP v 2) standard
Name: ptpd-slaveonly
Version: 2.3.1
Release: 0.99.rc5%{distver}
License: distributable
Group: System Environment/Daemons
Vendor: PTPd project team
Source0: ptpd-2.3.1.tar.gz
Source1: ptpd.init
Source2: ptpd.sysconfig
Source3: ptpd.conf

URL: http://ptpd.sf.net

Conflicts: ptpd

Requires(pre): /sbin/chkconfig
Requires(pre): /bin/awk sed grep

BuildRequires: libpcap-devel net-snmp-devel openssl-devel zlib-devel redhat-rpm-config
Requires: libpcap net-snmp openssl zlib

BuildRoot: %{_tmppath}/%{name}-root

%description
The PTP daemon (PTPd) implements the Precision Time
protocol (PTP) as defined by the IEEE 1588 standard.
PTP was developed to provide very precise time
coordination of LAN connected computers.

Install the ptpd package if you need tools for keeping your system's
time synchronised via the PTP protocol. This version is a slave-only
build - it is not possible to run as PTP master using ptpd-slave-only.

%prep 

%setup -n ptpd-2.3.1

%build

./configure --enable-slave-only --with-max-unicast-destinations=512

make

find . -type f | xargs chmod 644
find . -type d | xargs chmod 755

%install
rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT%{_mandir}/man{5,8}
mkdir -p $RPM_BUILD_ROOT%{_sbindir}
mkdir -p $RPM_BUILD_ROOT%{_datadir}/ptpd
mkdir -p $RPM_BUILD_ROOT%{_datadir}/snmp/mibs

install -m 755 src/ptpd2 $RPM_BUILD_ROOT%{_sbindir}
install -m 644 src/ptpd2.8 $RPM_BUILD_ROOT%{_mandir}/man8/ptpd2.8
install -m 644 src/ptpd2.conf.5 $RPM_BUILD_ROOT%{_mandir}/man5/ptpd2.conf.5
install -m 644 doc/PTPBASE-MIB.txt $RPM_BUILD_ROOT%{_datadir}/snmp/mibs/PTPBASE-MIB.txt
install -m 644 src/leap-seconds.list.28dec2015 $RPM_BUILD_ROOT%{_datadir}/ptpd/leap-seconds.list.28dec2015
install -m 644 src/ptpd2.conf.minimal $RPM_BUILD_ROOT%{_datadir}/ptpd/ptpd2.conf.minimal
install -m 644 src/ptpd2.conf.default-full $RPM_BUILD_ROOT%{_datadir}/ptpd/ptpd2.conf.default-full

{ cd $RPM_BUILD_ROOT

  mkdir -p .%{_initrddir}
  install -m755 $RPM_SOURCE_DIR/ptpd.init .%{_initrddir}/ptpd

  mkdir -p .%{_sysconfdir}/sysconfig
  install -m644 %{SOURCE2} .%{_sysconfdir}/sysconfig/ptpd
  install -m644 %{SOURCE3} .%{_sysconfdir}/ptpd2.conf

}


%clean
rm -rf $RPM_BUILD_ROOT

%pre

%post

/sbin/chkconfig --add ptpd


echo -e "\n===============\n"
echo -e "**** Doing post-install checks...\n"

grep -q : /etc/ethers >/dev/null 2>&1 || echo -e "Consider populating /etc/ethers with the MAC to host mappings of the GMs:\nexample:\t aa:bb:cc:dd:ee:ff gm-1.my.domain.net\n"

echo -e "\n*** Checking NTPd status...\n"
rpm -q ntp >/dev/null 2>&1
if [ $? -ne 0 ]; then

    echo "** NTPd not installed."
    echo -e "\n===============\n"
    exit 0
else
    echo -e "** NTPd is installed.\n"
fi

chkconfig --level `runlevel | awk '{print $2;}'` ntpd >/dev/null 2>&1

if [ $? == 0 ]; then
    echo "** NTPd enabled in current runlevel - consider disabling unless:"
    echo "- you're running a PTP master with NTP reference"
    echo -e "- you configure NTP integration in the PTPd configuration file\n";
fi

service ntpd status > /dev/null 2>&1
ret=$?
if [ $ret == 3 ]; then
    echo -e "** NTPd not running - good.\n";
elif [ $ret == 0 ]; then
    echo "** NTPd running - consider stopping before running ptpd unless:"
    echo "- you're running a PTP master with NTP reference"
    echo -e "- you configure NTP integration in the PTPd configuration file\n";
fi

echo -e "\n===============\n"

:

%preun
if [ $1 = 0 ]; then
    service ptpd stop > /dev/null 2>&1
    /sbin/chkconfig --del ptpd
fi
:

%postun
if [ "$1" -ge "1" ]; then
  service ptpd condrestart > /dev/null 2>&1
fi
:

%files
%defattr(-,root,root)
%{_sbindir}/ptpd2
%config			%{_initrddir}/ptpd
%config(noreplace)	%{_sysconfdir}/sysconfig/ptpd
%config(noreplace)	%{_sysconfdir}/ptpd2.conf
%{_mandir}/man8/*
%{_mandir}/man5/*
%{_datadir}/snmp/mibs/*
%{_datadir}/ptpd/*

%changelog
* Wed Jun 24 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-1
* Mon Jun 15 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc5
* Mon Jun 01 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc4
- rc4 release, adds leap seconds file
* Wed Apr 15 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc4.pre2
- Added the slaveonly spec
* Mon Apr 13 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc4.pre1
* Tue Oct 07 2014 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc3
* Fri Jul 25 2014 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc2
* Thu Jun 26 2014 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc1
* Thu Nov 21 2013 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.0-1
- Added the PTPBASE SNMP MIB
* Wed Nov 14 2013 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.0-0.99-rc2
- Initial public spec file and scripts for RHEL
