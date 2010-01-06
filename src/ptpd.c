/* ptpd.c */

#include "ptpd.h"

RunTimeOpts rtOpts;  /* statically allocated run-time configuration data */

int main(int argc, char **argv)
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
  rtOpts.slaveOnly = SLAVE_ONLY;
  rtOpts.currentUtcOffset = DEFAULT_UTC_OFFSET;
  rtOpts.noResetClock = DEFAULT_NO_RESET_CLOCK;
  rtOpts.noAdjust = NO_ADJUST;
  rtOpts.inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
  rtOpts.outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
  rtOpts.s = DEFAULT_DELAY_S;
  rtOpts.ap = DEFAULT_AP;
  rtOpts.ai = DEFAULT_AI;
  rtOpts.max_foreign_records = DEFAULT_MAX_FOREIGN_RECORDS;

  /*Initialize run time options with command line arguments*/
  if( !(ptpClock = ptpdStartup(argc, argv, &ret, &rtOpts)) )
    return ret;
 
	/* do the protocol engine */
    protocol(&rtOpts, ptpClock); //forever loop..

  ptpdShutdown();
  
  NOTIFY("self shutdown, probably due to an error\n");

  return 1;
}
