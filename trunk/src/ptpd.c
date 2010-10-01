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
