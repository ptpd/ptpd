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

%define servicename ptpd%{?servicesuffix}

%if %{slaveonly_build} == 1
Name: ptpd-slaveonly
Summary: Synchronises system time using the Precision Time Protocol (PTP) implementing the IEEE 1588-2008 (PTP v 2) standard. Slave-only version.
%else
Name: ptpd
Summary: Synchronises system time using the Precision Time Protocol (PTP) implementing the IEEE 1588-2008 (PTP v 2) standard. Full version with master and slave support.
%endif
Version: 2.3.2
Release: 1%{distver}%{?gittag}
License: distributable
Group: System Environment/Daemons
Vendor: PTPd project team
Source0: ptpd-2.3.2.tar.gz

Source2: ptpd.sysconfig
Source3: ptpd.conf

URL: http://ptpd.sf.net

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

%setup -n ptpd-2.3.2

%build

%if %{slaveonly_build} == 1
./configure --enable-slave-only --with-max-unicast-destinations=512
%else
./configure --with-max-unicast-destinations=512
%endif

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
install -m 644 src/leap-seconds.list $RPM_BUILD_ROOT%{_datadir}/ptpd/leap-seconds.list
install -m 644 src/ptpd2.conf.minimal $RPM_BUILD_ROOT%{_datadir}/ptpd/ptpd2.conf.minimal
install -m 644 src/ptpd2.conf.default-full $RPM_BUILD_ROOT%{_datadir}/ptpd/ptpd2.conf.default-full
install -m 644 src/templates.conf $RPM_BUILD_ROOT%{_datadir}/ptpd/templates.conf

{ cd $RPM_BUILD_ROOT

%if %{?_unitdir:1}%{!?_unitdir:0}
  mkdir -p .%{_unitdir}
  install -m644 $RPM_SOURCE_DIR/ptpd.service .%{_unitdir}/%{servicename}.service
  sed -i 's#/etc/sysconfig/__SERVICENAME__#/etc/sysconfig/%{servicename}#' .%{_unitdir}/%{servicename}.service
%else
  mkdir -p .%{_initrddir}
  install -m755 $RPM_SOURCE_DIR/ptpd.init .%{_initrddir}/%{servicename}
  sed -i 's#/etc/sysconfig/__SERVICENAME__#/etc/sysconfig/%{servicename}#' .%{_initrddir}/%{servicename}
%endif

  mkdir -p .%{_sysconfdir}/sysconfig
  install -m644 %{SOURCE2} .%{_sysconfdir}/sysconfig/%{servicename}
  install -m644 %{SOURCE3} .%{_sysconfdir}/ptpd2.conf

}


%clean
rm -rf $RPM_BUILD_ROOT

%pre

%post

%if %{?_unitdir:1}%{!?_unitdir:0}
/usr/bin/systemctl enable %{servicename} >/dev/null 2>&1
%else
/sbin/chkconfig --add %{servicename}
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
systemctl stop %{servicename} > /dev/null 2>&1
systemctl disable %{servicename}  >/dev/null 2>&1
%else
    service %{servicename} stop > /dev/null 2>&1
    /sbin/chkconfig --del %{servicename}
%endif
fi
:

%postun
if [ "$1" -ge "1" ]; then
  service %{servicename} condrestart > /dev/null 2>&1
fi
:

%files
%defattr(-,root,root)
%{_sbindir}/ptpd2
%if %{?_unitdir:1}%{!?_unitdir:0}
%{_unitdir}/%{servicename}.service
%else
%config			%{_initrddir}/%{servicename}
%endif
%config(noreplace)	%{_sysconfdir}/sysconfig/%{servicename}
%config(noreplace)	%{_sysconfdir}/ptpd2.conf
%config(noreplace)	%{_datadir}/ptpd/templates.conf
%{_mandir}/man8/*
%{_mandir}/man5/*
%{_datadir}/snmp/mibs/*
%{_datadir}/ptpd/*

%changelog
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
