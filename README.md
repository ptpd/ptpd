PTPd
===

PTP daemon (PTPd) is an implementation the Precision Time Protocol (PTP) version
2 as defined by 'IEEE Std 1588-2008'. PTP provides precise time coordination of
Ethernet LAN connected computers. It was designed primarily for instrumentation
and control systems.

Use
---

PTPd can coordinate the clocks of a group of LAN connected computers with each
other. It has been shown to achieve microsecond level coordination, even on
limited platforms.

The 'ptpd' program can be built from the included source code.  To use the
program, run 'ptpd' on a group of LAN connected computers. Compile with
'PTPD_DBG' defined and run with the '-C' or -V argument to watch what's going on.

If you are just looking for software to update the time on your desktop, you
probably want something that implements the Network Time Protocol. It can
coordinate computer clocks with an absolute time reference such as UTC.

Please refer to the
[INSTALL](https://raw.githubusercontent.com/ptpd/ptpd/master/INSTALL) file
for build instructions and configuration options. Please refer to the
[README.repocheckout](https://github.com/ptpd/ptpd/blob/master/README.repocheckout)
file for information on how to build from source code repositories.

Legal notice
---

PTPd was written by using only information contained within 'IEEE Std
1588-2008'. IEEE 1588 may contain patented technology, the use of which is not
under the control of the authors of PTPd. Users of IEEE 1588 may need to obtain
a license for the patented technology in the protocol. Contact the IEEE for
licensing information.

PTPd is licensed under a 2 Clause BSD Open Source License. Please refer to the
[COPYRIGHT](https://github.com/ptpd/ptpd/blob/master/COPYRIGHT) file for
additional information.

PTPd comes with absolutely no warranty.
