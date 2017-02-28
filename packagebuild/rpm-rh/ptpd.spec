# (c) 2014-2015:  Wojciech Owczarek, PTPd project

%if %{?build_slaveonly:1}%{!?build_slaveonly:0}
%define slaveonly_build %{build_slaveonly}
%else
%define slaveonly_build 0
%endif

%define _use_internal_dependency_generator 0

# RHEL5.5 and older don't have the /etc/rpm/macros.dist macros
%if %{?dist:1}%{!?dist:0}
%define distver %{dist}
%else
%define distver %(/usr/lib/rpm/redhat/dist.sh --dist)
%endif

%if %{slaveonly_build} == 1
Name: ptpd-libcck-slaveonly
Summary: Synchronises system time using the Precision Time Protocol (PTP) implementing the IEEE 1588-2008 (PTP v 2) standard. Slave-only version.
%else
Name: ptpd-libcck
Summary: Synchronises system time using the Precision Time Protocol (PTP) implementing the IEEE 1588-2008 (PTP v 2) standard. Full version with master and slave support.
%endif
Version: 2.3.2
Release: 6%{distver}%{?gittag}
License: distributable
Group: System Environment/Daemons
Vendor: PTPd project team
Source0: ptpd-2.3.2-libcck-git.tar.gz

Source2: ptpd.sysconfig
Source3: ptpd.conf

URL: http://github.com/wowczarek/ptpd/tree/wowczarek-2.3.2-libcck

%if %{slaveonly_build} == 1
Conflicts: ptpd
%else
Conflicts: ptpd-slaveonly
%endif

Requires(pre): /sbin/chkconfig
Requires(pre): /bin/awk sed grep

BuildRequires: libpcap-devel net-snmp-devel openssl-devel zlib-devel redhat-rpm-config
Requires: libpcap net-snmp openssl zlib

# systemd
%if %{?_unitdir:1}%{!?_unitdir:0}
Source1: ptpd.service
Requires: systemd
BuildRequires: systemd
%else
Source1: ptpd.init
%endif

BuildRoot: %{_tmppath}/%{name}-root

%description
The PTP daemon (PTPd) implements the Precision Time
protocol (PTP) as defined by the IEEE 1588 standard.
PTP was developed to provide very precise time
coordination of LAN connected computers.

Install the ptpd package if you need tools for keeping your system's
time synchronised via the PTP protocol or serving PTP time.

%prep

%setup -n ptpd-2.3.2-libcck-git

%build

%if %{slaveonly_build} == 1
./configure --enable-slave-only --with-max-unicast-destinations=512
%else
./configure --with-max-unicast-destinations=512
%endif

make -j8

find . -type f | xargs chmod 644
find . -type d | xargs chmod 755

%install
rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT%{_mandir}/man{5,8}
mkdir -p $RPM_BUILD_ROOT%{_sbindir}
mkdir -p $RPM_BUILD_ROOT%{_datadir}/ptpd
mkdir -p $RPM_BUILD_ROOT%{_datadir}/snmp/mibs

install -m 755 src/ptpd $RPM_BUILD_ROOT%{_sbindir}
install -m 644 src/ptpd.8 $RPM_BUILD_ROOT%{_mandir}/man8/ptpd.8
install -m 644 src/ptpd.conf.5 $RPM_BUILD_ROOT%{_mandir}/man5/ptpd.conf.5
install -m 644 doc/PTPBASE-MIB.txt $RPM_BUILD_ROOT%{_datadir}/snmp/mibs/PTPBASE-MIB.txt
install -m 644 src/leap-seconds.list $RPM_BUILD_ROOT%{_datadir}/ptpd/leap-seconds.list
install -m 644 src/ptpd.conf.minimal $RPM_BUILD_ROOT%{_datadir}/ptpd/ptpd.conf.minimal
install -m 644 src/ptpd.conf.default-full $RPM_BUILD_ROOT%{_datadir}/ptpd/ptpd.conf.default-full
install -m 644 src/templates.conf $RPM_BUILD_ROOT%{_datadir}/ptpd/templates.conf

{ cd $RPM_BUILD_ROOT

%if %{?_unitdir:1}%{!?_unitdir:0}
  mkdir -p .%{_unitdir}
  install -m644 $RPM_SOURCE_DIR/ptpd.service .%{_unitdir}/ptpd.service
%else
  mkdir -p .%{_initrddir}
  install -m755 $RPM_SOURCE_DIR/ptpd.init .%{_initrddir}/ptpd
%endif

  mkdir -p .%{_sysconfdir}/sysconfig
  install -m644 %{SOURCE2} .%{_sysconfdir}/sysconfig/ptpd
  install -m644 %{SOURCE3} .%{_sysconfdir}/ptpd.conf

}


%clean
rm -rf $RPM_BUILD_ROOT

%pre

%post

%if %{?_unitdir:1}%{!?_unitdir:0}
/usr/bin/systemctl enable ptpd  >/dev/null 2>&1
%else
/sbin/chkconfig --add ptpd
%endif
echo
echo -e "**** PTPd - Running post-install checks...\n"

grep -q : /etc/ethers >/dev/null 2>&1 || echo -e "Consider populating /etc/ethers with the MAC to host mappings of the GMs:\nexample:\t aa:bb:cc:dd:ee:ff gm-1.my.domain.net\n"

for candidate in ntp chrony; do

echo -e "\n*** Checking ${candidate}d status...\n"
rpm -q $candidate >/dev/null 2>&1
if [ $? -ne 0 ]; then

    echo "** ${candidate}d not installed."
else
    echo -e "** ${candidate}d is installed.\n"

%if %{?_unitdir:1}%{!?_unitdir:0}
/usr/bin/systemctl status ${candidate}d | grep Loaded | grep enabled >/dev/null 2>&1
%else
chkconfig --level `runlevel | awk '{print $2;}'` ${candidate}d >/dev/null 2>&1
%endif

if [ $? == 0 ]; then
    echo "** ${candidate}d enabled in current runlevel - consider disabling unless:"
    echo "- you're running a PTP master with NTP reference"
    if [ "$candidate" == "ntp" ]; then
	echo -e "- you configure NTP integration in the PTPd configuration file\n";
    fi
fi
%if %{?_unitdir:1}%{!?_unitdir:0}
systemctl status ${candidate}d > /dev/null 2>&1
%else
service ${candidate}d status > /dev/null 2>&1
%endif
ret=$?
if [ $ret == 3 ]; then
    echo -e "** ${candidate}d not running - good.\n";
elif [ $ret == 0 ]; then
    echo "** ${candidate}d running - consider stopping before running ptpd unless:"
    echo "- you're running a PTP master with NTP reference"
    if [ "$candidate" == "ntp" ]; then
	echo -e "- you configure NTP integration in the PTPd configuration file\n";
    fi

fi

fi

echo

done

:

%preun
if [ $1 = 0 ]; then
%if %{?_unitdir:1}%{!?_unitdir:0}
systemctl stop ptpd > /dev/null 2>&1
systemctl disable ptpd  >/dev/null 2>&1
%else
    service ptpd stop > /dev/null 2>&1
    /sbin/chkconfig --del ptpd
%endif
fi
:

%postun
if [ "$1" -ge "1" ]; then
  service ptpd condrestart > /dev/null 2>&1
fi
:

%files
%defattr(-,root,root)
%{_sbindir}/ptpd
%if %{?_unitdir:1}%{!?_unitdir:0}
%{_unitdir}/ptpd.service
%else
%config			%{_initrddir}/ptpd
%endif
%config(noreplace)	%{_sysconfdir}/sysconfig/ptpd
%config(noreplace)	%{_sysconfdir}/ptpd.conf
%config(noreplace)	%{_datadir}/ptpd/templates.conf
%{_mandir}/man8/*
%{_mandir}/man5/*
%{_datadir}/snmp/mibs/*
%{_datadir}/ptpd/*

%changelog
* Mon Feb 27 2017 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.2-6
- libcck progress import (clockdriver, transport, timer)
* Fri Nov 18 2016 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.2-5
- further clock selection work,
- libcck progress (transport),
- rewritten PTP message parsers (beginnings of lib1588)
- PTP monitoring TLV extension support
- improved robustness to TX timestamp failures,
- many more interim fixes
* Fri Feb 05 2016 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.2-4
- third build, further improvements to clock selection and clock control options
* Mon Jan 25 2016 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.2-3
- second linuxphc build, clock election and TX timestamp fixes
* Mon Jan 18 2016 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.2-2
- first linuxphc build
* Thu Oct 23 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.2-1
- version 2.3.2
* Wed Jul 1 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-2
- spec updated for OSes with systemd
- chrony detection added to postinstall checks
* Wed Jun 24 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-1
* Mon Jun 15 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc5
* Mon Jun 01 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc4
- rc5 release, adds leap seconds file and config files
* Wed Apr 15 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc4.pre2
* Mon Apr 13 2015 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc4.pre1
* Tue Oct 07 2014 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc3
* Fri Jul 25 2014 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc2
* Thu Jun 26 2014 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.1-0.99.rc1
* Thu Nov 21 2013 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.0-1
- Added the PTPBASE SNMP MIB
* Wed Nov 13 2013 Wojciech Owczarek <wojciech@owczarek.co.uk> 2.3.0-0.99-rc2
- Initial public spec file and scripts for RHEL
