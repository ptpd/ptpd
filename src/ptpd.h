/**
 * @file   ptpd.h
 * @mainpage Ptpd Documentation
 * @authors Martin Burnicki, Alexandre van Kempen, Steven Kreuzer, 
 *          George Neville-Neil
 * @version 1.0
 * @date   Tue Jul 20 16:15:04 2010
 * 
 * 
 */

#ifndef PTPD_H
#define PTPD_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <syslog.h>
#include <limits.h>

#include "constants.h"
#include "dep/constants_dep.h"
#include "dep/datatypes_dep.h"
#include "datatypes.h"
#include "dep/ptpd_dep.h"

/* arith.c */
UInteger32 crc_algorithm(Octet *, Integer16);
UInteger32 sum(Octet *, Integer16);
void	fromInternalTime(TimeInternal *, TimeRepresentation *, Boolean);
void	toInternalTime(TimeInternal *, TimeRepresentation *, Boolean *);
void	normalizeTime(TimeInternal *);
void	addTime(TimeInternal *, TimeInternal *, TimeInternal *);
void	subTime(TimeInternal *, TimeInternal *, TimeInternal *);

/* bmc.c */
UInteger8 bmc(ForeignMasterRecord *, RunTimeOpts *, PtpClock *);
void	m1 (PtpClock *);
void	s1 (MsgHeader *, MsgSync *, PtpClock *);
void	initData(RunTimeOpts *, PtpClock *);

/* probe.c */
void	probe(RunTimeOpts *, PtpClock *);

/* protocol.c */
void	protocol(RunTimeOpts *, PtpClock *);


#endif
