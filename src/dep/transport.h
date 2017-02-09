/*-
 * Copyright (c) 2016      Wojciech Owczarek
 *
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   transport.h
 * @date   Thu Nov 24 23:19:00 2016
 *
 * @brief  PTPd transport code
 *
 *
 */

#ifndef _PTPD_TRANSPORT_H
#define _PTPD_TRANSPORT_H

#include "../ptpd.h"

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/fd_set.h>
#include <libcck/ttransport.h>

#include "../globalconfig.h"

/* Populate network protocol and address information inside PTP clock */
//void setPtpNetworkInfo(PtpClock *ptpClock);

/* configure ACLs */
void configureAcls(PtpClock *ptpClock, const GlobalConfig *global);
/* Create, probe, configure and start network transports for a PTP port */
bool initPtpTransports(PtpClock *ptpClock, CckFdSet *fdSet, const GlobalConfig *global);
/* Shut down and free transports for a PTP port */
void shutdownPtpTransports(PtpClock *ptpClock);
/* get preferred address family we have configured */
int getConfiguredFamily(const GlobalConfig *global);
/* test if desired transport configuration is usable */
bool testTransportConfig(const GlobalConfig *global);
/* get preferred address family based on config */
int getConfiguredFamily(const GlobalConfig *global);

#endif /* _PTPD_TRANSPORT_H */

