/*-
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments.
 * Copyright (c) 2009-2010 George V. Neville-Neil, 
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen
 *
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

/*
 * Global variable with the main PTP port. This is used to show the current state in DBG()/message()
 * without having to pass the pointer everytime.
 *
 * if ptpd is extended to handle multiple ports (eg, to instantiate a Boundary Clock),
 * then DBG()/message() needs a per-port pointer argument
 */
PtpClock *G_ptpClock = NULL;

int
main(int argc, char **argv)
{
	PtpClock *ptpClock;
	Integer16 ret;

	/* initialize run-time options to default values */
	rtOpts.announceInterval = DEFAULT_ANNOUNCE_INTERVAL;
	rtOpts.syncInterval = DEFAULT_SYNC_INTERVAL;
	rtOpts.clockQuality.clockAccuracy = DEFAULT_CLOCK_ACCURACY;
	rtOpts.clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
	rtOpts.clockQuality.offsetScaledLogVariance = DEFAULT_CLOCK_VARIANCE;
	rtOpts.priority1 = DEFAULT_PRIORITY1;
	rtOpts.priority2 = DEFAULT_PRIORITY2;
	rtOpts.domainNumber = DEFAULT_DOMAIN_NUMBER;
#ifdef PTP_EXPERIMENTAL
	rtOpts.mcast_group_Number = 0;
	rtOpts.do_hybrid_mode = 0;
#endif
	
	// rtOpts.slaveOnly = FALSE;
	rtOpts.currentUtcOffset = DEFAULT_UTC_OFFSET;
	rtOpts.ifaceName[0] = '\0';
	rtOpts.do_unicast_mode = 0;

	rtOpts.noAdjust = NO_ADJUST;  // false
	// rtOpts.displayStats = FALSE;
	/* Deep display of all packets seen by the daemon */
	rtOpts.displayPackets = FALSE;
	// rtOpts.unicastAddress
	rtOpts.ap = DEFAULT_AP;
	rtOpts.ai = DEFAULT_AI;
	rtOpts.s = DEFAULT_DELAY_S;
	rtOpts.inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
	rtOpts.outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
	rtOpts.max_foreign_records = DEFAULT_MAX_FOREIGN_RECORDS;
	// rtOpts.ethernet_mode = FALSE;
	// rtOpts.offset_first_updated = FALSE;
	// rtOpts.file[0] = 0;
	rtOpts.maxDelayAutoTune = FALSE;
	rtOpts.discardedPacketThreshold = 60;
	rtOpts.logFd = -1;
	rtOpts.recordFP = NULL;
	rtOpts.do_log_to_file = FALSE;
	rtOpts.do_record_quality_file = FALSE;
	rtOpts.nonDaemon = FALSE;

	/*
	 * defaults for new options
	 */
	rtOpts.slaveOnly = TRUE;
	rtOpts.ignore_delayreq_interval_master = FALSE;
	rtOpts.do_IGMP_refresh = TRUE;
	rtOpts.useSysLog       = TRUE;
	rtOpts.syslog_startup_messages_also_to_stdout = TRUE;		/* used to print inital messages both to syslog and screen */
	rtOpts.announceReceiptTimeout  = DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
#ifdef RUNTIME_DEBUG
	rtOpts.debug_level = LOG_INFO;			/* by default debug messages as disabled, but INFO messages and below are printed */
#endif

	rtOpts.ttl = 1;
	rtOpts.delayMechanism   = DEFAULT_DELAY_MECHANISM;
	rtOpts.noResetClock     = DEFAULT_NO_RESET_CLOCK;
	rtOpts.log_seconds_between_message = 0;

	rtOpts.initial_delayreq = DEFAULT_DELAYREQ_INTERVAL;
	rtOpts.subsequent_delayreq = DEFAULT_DELAYREQ_INTERVAL;      // this will be updated if -g is given

	/* Initialize run time options with command line arguments */
	if (!(ptpClock = ptpdStartup(argc, argv, &ret, &rtOpts)))
		return ret;

	/* global variable for message(), please see comment on top of this file */
	G_ptpClock = ptpClock;

	/* do the protocol engine */
	protocol(&rtOpts, ptpClock);
	/* forever loop.. */

	ptpdShutdown(ptpClock);

	NOTIFY("self shutdown, probably due to an error\n");

	return 1;
}
