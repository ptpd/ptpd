/*-
 * Copyright (c) 2009-2011 George V. Neville-Neil, Steven Kreuzer, 
 *                         Martin Burnicki
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
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
 * @file   ptpd.c
 * @date   Wed Jun 23 10:13:38 2010
 * 
 * @brief  The main() function for the PTP daemon
 * 
 * This file contains very little code, as should be obvious,
 * and only serves to tie together the rest of the daemon.
 * All of the default options are set here, but command line
 * arguments are processed in the ptpdStartup() routine called
 * below.
 */

#include "ptpd.h"

RunTimeOpts rtOpts;			/* statically allocated run-time
					 * configuration data */

int
main(int argc, char **argv)
{
	PtpClock *ptpClock;
	Integer16 ret;

	/* initialize run-time options to reasonable values */
	rtOpts.syncInterval = DEFAULT_SYNC_INTERVAL;
	memcpy(rtOpts.subdomainName, DEFAULT_PTP_DOMAIN_NAME, PTP_SUBDOMAIN_NAME_LENGTH);
	memcpy(rtOpts.clockIdentifier, IDENTIFIER_DFLT, PTP_CODE_STRING_LENGTH);
	rtOpts.clockVariance = DEFAULT_CLOCK_VARIANCE;
	rtOpts.clockStratum = DEFAULT_CLOCK_STRATUM;
	rtOpts.unicastAddress[0] = 0;
	rtOpts.inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
	rtOpts.outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
	rtOpts.noResetClock = DEFAULT_NO_RESET_CLOCK;
	rtOpts.s = DEFAULT_DELAY_S;
	rtOpts.ap = DEFAULT_AP;
	rtOpts.ai = DEFAULT_AI;
	rtOpts.max_foreign_records = DEFAULT_MAX_FOREIGN_RECORDS;
	rtOpts.currentUtcOffset = DEFAULT_UTC_OFFSET;
	rtOpts.logFd = -1;
	rtOpts.recordFP = NULL;
	rtOpts.useSysLog = FALSE;
	rtOpts.ttl = 1;
	rtOpts.displayPackets = FALSE;
	
	if (!(ptpClock = ptpdStartup(argc, argv, &ret, &rtOpts)))
		return ret;

	if (rtOpts.probe) {
		probe(&rtOpts, ptpClock);
	} else {
		/* do the protocol engine */
		protocol(&rtOpts, ptpClock);
	}

	ptpdShutdown();

	NOTIFY("self shutdown, probably due to an error\n");
	return 1;
}
