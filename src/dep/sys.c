/*-
 * Copyright (c) 2012-2015 Wojciech Owczarek,
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
 * @file   sys.c
 * @date   Tue Jul 20 16:19:46 2010
 *
 * @brief  Code to call kernel time routines and also display server statistics.
 *
 *
 */

#include "../ptpd.h"

#ifdef HAVE_NETINET_ETHER_H
#  include <netinet/ether.h>
#endif

#ifdef HAVE_NET_ETHERNET_H
#  include <net/ethernet.h>
#endif

#ifdef HAVE_NET_IF_H
#  include <net/if.h>
#endif

#ifdef HAVE_NET_IF_ETHER_H
#  include <net/if_ether.h>
#endif

/* only C99 has the round function built-in */
double round (double __x);

static int closeLog(LogFileHandler* handler);

/*
 returns a static char * for the representation of time, for debug purposes
 DO NOT call this twice in the same printf!
*/
char *dump_TimeInternal(const TimeInternal * p)
{
	static char buf[100];

	snprint_TimeInternal(buf, 100, p);
	return buf;
}


/*
 displays 2 timestamps and their strings in sequence, and the difference between then
 DO NOT call this twice in the same printf!
*/
char *dump_TimeInternal2(const char *st1, const TimeInternal * p1, const char *st2, const TimeInternal * p2)
{
	static char buf[BUF_SIZE];
	int n = 0;

	/* display Timestamps */
	if (st1) {
		n += snprintf(buf + n, BUF_SIZE - n, "%s ", st1);
	}
	n += snprint_TimeInternal(buf + n, BUF_SIZE - n, p1);
	n += snprintf(buf + n, BUF_SIZE - n, "    ");

	if (st2) {
		n += snprintf(buf + n, BUF_SIZE - n, "%s ", st2);
	}
	n += snprint_TimeInternal(buf + n, BUF_SIZE - n, p2);
	n += snprintf(buf + n, BUF_SIZE - n, " ");

	/* display difference */
	TimeInternal r;
	subTime(&r, p1, p2);
	n += snprintf(buf + n, BUF_SIZE - n, "   (diff: ");
	n += snprint_TimeInternal(buf + n, BUF_SIZE - n, &r);
	n += snprintf(buf + n, BUF_SIZE - n, ") ");

	return buf;
}



int 
snprint_TimeInternal(char *s, int max_len, const TimeInternal * p)
{
	int len = 0;

	/* always print either a space, or the leading "-". This makes the stat files columns-aligned */
	len += snprintf(&s[len], max_len - len, "%c",
		isTimeInternalNegative(p)? '-':' ');

	len += snprintf(&s[len], max_len - len, "%d.%09d",
	    abs(p->seconds), abs(p->nanoseconds));

	return len;
}


/* debug aid: convert a time variable into a static char */
char *time2st(const TimeInternal * p)
{
	static char buf[1000];

	snprint_TimeInternal(buf, sizeof(buf), p);
	return buf;
}



void DBG_time(const char *name, const TimeInternal  p)
{
	DBG("             %s:   %s\n", name, time2st(&p));

}


char *
translatePortState(PtpClock *ptpClock)
{
	char *s;
	switch(ptpClock->portState) {
	    case PTP_INITIALIZING:  s = "init";  break;
	    case PTP_FAULTY:        s = "flt";   break;
	    case PTP_LISTENING:
		    /* seperate init-reset from real resets */
		    if(ptpClock->resetCount == 1){
		    	s = "lstn_init";
		    } else {
		    	s = "lstn_reset";
		    }
		    break;
	    case PTP_PASSIVE:       s = "pass";  break;
	    case PTP_UNCALIBRATED:  s = "uncl";  break;
	    case PTP_SLAVE:         s = "slv";   break;
	    case PTP_PRE_MASTER:    s = "pmst";  break;
	    case PTP_MASTER:        s = "mst";   break;
	    case PTP_DISABLED:      s = "dsbl";  break;
	    default:                s = "?";     break;
	}
	return s;
}


int 
snprint_ClockIdentity(char *s, int max_len, const ClockIdentity id)
{
	int len = 0;
	int i;

	for (i = 0; ;) {
		len += snprintf(&s[len], max_len - len, "%02x", (unsigned char) id[i]);

		if (++i >= CLOCK_IDENTITY_LENGTH)
			break;
	}

	return len;
}


/* show the mac address in an easy way */
int
snprint_ClockIdentity_mac(char *s, int max_len, const ClockIdentity id)
{
	int len = 0;
	int i;

	for (i = 0; ;) {
		/* skip bytes 3 and 4 */
		if(!((i==3) || (i==4))){
			len += snprintf(&s[len], max_len - len, "%02x", (unsigned char) id[i]);

			if (++i >= CLOCK_IDENTITY_LENGTH)
				break;

			/* print a separator after each byte except the last one */
			len += snprintf(&s[len], max_len - len, "%s", ":");
		} else {

			i++;
		}
	}

	return len;
}


/*
 * wrapper that caches the latest value of ether_ntohost
 * this function will NOT check the last accces time of /etc/ethers,
 * so it only have different output on a failover or at restart
 *
 */
int ether_ntohost_cache(char *hostname, struct ether_addr *addr)
{
	static int valid = 0;
	static struct ether_addr prev_addr;
	static char buf[BUF_SIZE];

#ifdef HAVE_STRUCT_ETHER_ADDR_OCTET
	if (memcmp(addr->octet, &prev_addr, 
		  sizeof(struct ether_addr )) != 0) {
		valid = 0;
	}
#else
	if (memcmp(addr->ether_addr_octet, &prev_addr, 
		  sizeof(struct ether_addr )) != 0) {
		valid = 0;
	}
#endif
	if (!valid) {
		if(ether_ntohost(buf, addr)){
			snprintf(buf, BUF_SIZE,"%s", "unknown");
		}

		/* clean possible commas from the string */
		while (strchr(buf, ',') != NULL) {
			*(strchr(buf, ',')) = '_';
		}

		prev_addr = *addr;
	}

	valid = 1;
	strncpy(hostname, buf, 100);
	return 0;
}


/* Show the hostname configured in /etc/ethers */
int
snprint_ClockIdentity_ntohost(char *s, int max_len, const ClockIdentity id)
{
	int len = 0;
	int i,j;
	char  buf[100];
	struct ether_addr e;

	/* extract mac address */
	for (i = 0, j = 0; i< CLOCK_IDENTITY_LENGTH ; i++ ){
		/* skip bytes 3 and 4 */
		if(!((i==3) || (i==4))){
#ifdef HAVE_STRUCT_ETHER_ADDR_OCTET
			e.octet[j] = (uint8_t) id[i];
#else
			e.ether_addr_octet[j] = (uint8_t) id[i];
#endif
			j++;
		}
	}

	/* convert and print hostname */
	ether_ntohost_cache(buf, &e);
	len += snprintf(&s[len], max_len - len, "(%s)", buf);

	return len;
}


int 
snprint_PortIdentity(char *s, int max_len, const PortIdentity *id)
{
	int len = 0;

#ifdef PRINT_MAC_ADDRESSES
	len += snprint_ClockIdentity_mac(&s[len], max_len - len, id->clockIdentity);
#else	
	len += snprint_ClockIdentity(&s[len], max_len - len, id->clockIdentity);
#endif

	len += snprint_ClockIdentity_ntohost(&s[len], max_len - len, id->clockIdentity);

	len += snprintf(&s[len], max_len - len, "/%d", (unsigned) id->portNumber);
	return len;
}

/* Write a formatted string to file pointer */
int writeMessage(FILE* destination, int priority, const char * format, va_list ap) {

	extern RunTimeOpts rtOpts;
	extern Boolean startupInProgress;

	int written;
	char time_str[MAXTIMESTR];
	struct timeval now;

	extern char *translatePortState(PtpClock *ptpClock);
	extern PtpClock *G_ptpClock;

	if(destination == NULL)
		return -1;

	/* If we're starting up as daemon, only print <= WARN */
	if ((destination == stderr) && 
		!rtOpts.nonDaemon && startupInProgress &&
		(priority > LOG_WARNING)){
		    return 1;
		}
	/* Print timestamps and prefixes only if we're running in foreground or logging to file*/
	if( rtOpts.nonDaemon || destination != stderr) {

		/*
		 * select debug tagged with timestamps. This will slow down PTP itself if you send a lot of messages!
		 * it also can cause problems in nested debug statements (which are solved by turning the signal
		 *  handling synchronous, and not calling this function inside asycnhronous signal processing)
		 */
		gettimeofday(&now, 0);
		strftime(time_str, MAXTIMESTR, "%F %X", localtime((time_t*)&now.tv_sec));
		fprintf(destination, "%s.%06d ", time_str, (int)now.tv_usec  );
		fprintf(destination,PTPD_PROGNAME"[%d].%s (%-9s ",
		(int)getpid(), startupInProgress ? "startup" : rtOpts.ifaceName,
		priority == LOG_EMERG   ? "emergency)" :
		priority == LOG_ALERT   ? "alert)" :
		priority == LOG_CRIT    ? "critical)" :
		priority == LOG_ERR     ? "error)" :
		priority == LOG_WARNING ? "warning)" :
		priority == LOG_NOTICE  ? "notice)" :
		priority == LOG_INFO    ? "info)" :
		priority == LOG_DEBUG   ? "debug1)" :
		priority == LOG_DEBUG2  ? "debug2)" :
		priority == LOG_DEBUGV  ? "debug3)" :
		"unk)");


		fprintf(destination, " (%s) ", G_ptpClock ?
		       translatePortState(G_ptpClock) : "___");
	}
	written = vfprintf(destination, format, ap);
	return written;

}

void
updateLogSize(LogFileHandler* handler)
{

	struct stat st;
	if(handler->logFP == NULL)
		return;
	if (fstat(fileno(handler->logFP), &st) != -1) {
		handler->fileSize = st.st_size;
	} else {
#ifdef RUNTIME_DEBUG
/* 2.3.1: do not print to stderr or log file */
//		fprintf(stderr, "fstat on %s file failed!\n", handler->logID);
#endif /* RUNTIME_DEBUG */
	}

}


/*
 * Prints a message, randing from critical to debug.
 * This either prints the message to syslog, or with timestamp+state to stderr
 */
void
logMessage(int priority, const char * format, ...)
{
	extern RunTimeOpts rtOpts;
	extern Boolean startupInProgress;
	va_list ap;
	va_start(ap, format);

#ifdef RUNTIME_DEBUG
	if ((priority >= LOG_DEBUG) && (priority > rtOpts.debug_level)) {
		goto end;
	}
#endif

	/* log level filter */
	if(priority > rtOpts.logLevel)
	    goto end;

	/* If we're using a log file and the message has been written OK, we're done*/
	if(rtOpts.eventLog.logEnabled && rtOpts.eventLog.logFP != NULL) {
	    if(writeMessage(rtOpts.eventLog.logFP, priority, format, ap) > 0) {
		maintainLogSize(&rtOpts.eventLog);
		if(!startupInProgress)
		    goto end;
		else
		    goto std_err;
	    }
	}

	/*
	 * Otherwise we try syslog - if we're here, we didn't write to log file.
	 * If we're running in background and we're starting up, also log first
	 * messages to syslog to at least leave a trace.
	 */
	if (rtOpts.useSysLog ||
	    (!rtOpts.nonDaemon && startupInProgress)) {
		static Boolean syslogOpened;
#ifdef RUNTIME_DEBUG
		/*
		 *  Syslog only has 8 message levels (3 bits)
		 *  important: messages will only appear if "*.debug /var/log/debug" is on /etc/rsyslog.conf
		 */
		if(priority > LOG_DEBUG){
			priority = LOG_DEBUG;
		}
#endif

		if (!syslogOpened) {
			openlog(PTPD_PROGNAME, LOG_PID, LOG_DAEMON);
			syslogOpened = TRUE;
		}
		vsyslog(priority, format, ap);
		if (!startupInProgress)
			goto end;
		else
			goto std_err;
	}
std_err:
	va_start(ap, format);
	/* Either all else failed or we're running in foreground - or we also log to stderr */
	writeMessage(stderr, priority, format, ap);

end:
	va_end(ap);
	return;
}


/* Restart a file log target based on its settings  */
int
restartLog(LogFileHandler* handler, Boolean quiet)
{

        /* The FP is open - close it */
        if(handler->logFP != NULL) {
                fclose(handler->logFP);
		/*
		 * fclose doesn't do this at least on Linux - changes the underlying FD to -1,
		 * but not the FP to NULL - with this we can tell if the FP is closed
		 */
		handler->logFP=NULL;
                /* If we're not logging to file (any more), call it quits */
                if (!handler->logEnabled) {
                    if(!quiet) INFO("Logging to %s file disabled. Closing file.\n", handler->logID);
		    if(handler->unlinkOnClose)
			unlink(handler->logPath);
                    return 1;
                }
        }
        /* FP is not open and we're not logging */
        if (!handler->logEnabled)
                return 1;

	/* Open the file */
        if ( (handler->logFP = fopen(handler->logPath, handler->openMode)) == NULL) {
                if(!quiet) PERROR("Could not open %s file", handler->logID);
        } else {
		if(handler->truncateOnReopen) {
			if(!ftruncate(fileno(handler->logFP), 0)) {
				if(!quiet) INFO("Truncated %s file\n", handler->logID);
			} else
				DBG("Could not truncate % file: %s\n", handler->logID, handler->logPath);
		}
		/* \n flushes output for us, no need for fflush() - if you want something different, set it later */
                setlinebuf(handler->logFP);

	}
        return (handler->logFP != NULL);
}

/* Close a file log target */
static int
closeLog(LogFileHandler* handler)
{
        if(handler->logFP != NULL) {
                fclose(handler->logFP);
		handler->logFP=NULL;
		return 1;
        }

	return 0;
}


/* Return TRUE only if the log file had to be rotated / truncated - FALSE does not mean error */
/* Mini-logrotate: truncate file if exceeds preset size, also rotate up to n number of files if configured */
Boolean
maintainLogSize(LogFileHandler* handler)
{

	if(handler->maxSize) {
		if(handler->logFP == NULL)
		    return FALSE;
		updateLogSize(handler);
#ifdef RUNTIME_DEBUG
/* 2.3.1: do not print to stderr or log file */
//		fprintf(stderr, "%s logsize: %d\n", handler->logID, handler->fileSize);
#endif /* RUNTIME_DEBUG */
		if(handler->fileSize > (handler->maxSize * 1024)) {

		    /* Rotate the log file */
		    if (handler->maxFiles) {
			int i = 0;
			int logFileNumber = 0;
			time_t maxMtime = 0;
			struct stat st;
			char fname[PATH_MAX];
			/* Find the last modified file of the series */
			while(++i <= handler->maxFiles) {
				memset(fname, 0, PATH_MAX);
				snprintf(fname, PATH_MAX,"%s.%d", handler->logPath, i);
				if((stat(fname,&st) != -1) && S_ISREG(st.st_mode)) {
					if(st.st_mtime > maxMtime) {
						maxMtime = st.st_mtime;
						logFileNumber = i;
					}
				}
			}
			/* Use next file in line or first one if rolled over */
			if(++logFileNumber > handler->maxFiles)
				logFileNumber = 1;
			memset(fname, 0, PATH_MAX);
			snprintf(fname, PATH_MAX,"%s.%d", handler->logPath, logFileNumber);
			/* Move current file to new location */
			rename(handler->logPath, fname);
			/* Reopen to reactivate the original path */
			if(restartLog(handler,TRUE)) {
				INFO("Rotating %s file - size above %dkB\n",
					handler->logID, handler->maxSize);
			} else {
				DBG("Could not rotate %s file\n", handler->logPath);
			}
			return TRUE;
		    /* Just truncate - maxSize given but no maxFiles */
		    } else {

			if(!ftruncate(fileno(handler->logFP),0)) {
			INFO("Truncating %s file - size above %dkB\n",
				handler->logID, handler->maxSize);
			} else {
#ifdef RUNTIME_DEBUG
/* 2.3.1: do not print to stderr or log file */
//				fprintf(stderr, "Could not truncate %s file\n", handler->logPath);
#endif
			}
			return TRUE;
		    }
		}
	}

	return FALSE;

}

void
restartLogging(RunTimeOpts* rtOpts)
{

	if(!restartLog(&rtOpts->statisticsLog, TRUE))
		NOTIFY("Failed logging to %s file\n", rtOpts->statisticsLog.logID);

	if(!restartLog(&rtOpts->recordLog, TRUE))
		NOTIFY("Failed logging to %s file\n", rtOpts->recordLog.logID);

	if(!restartLog(&rtOpts->eventLog, TRUE))
		NOTIFY("Failed logging to %s file\n", rtOpts->eventLog.logID);

	if(!restartLog(&rtOpts->statusLog, TRUE))
		NOTIFY("Failed logging to %s file\n", rtOpts->statusLog.logID);

}

void
stopLogging(RunTimeOpts* rtOpts)
{
	closeLog(&rtOpts->statisticsLog);
	closeLog(&rtOpts->recordLog);
	closeLog(&rtOpts->eventLog);
	closeLog(&rtOpts->statusLog);
}

void 
logStatistics(PtpClock * ptpClock)
{
	extern RunTimeOpts rtOpts;
	static int errorMsg = 0;
	static char sbuf[SCREEN_BUFSZ];
	int len = 0;
	TimeInternal now;
	time_t time_s;
	FILE* destination;
	static TimeInternal prev_now_sync, prev_now_delay;
	char time_str[MAXTIMESTR];

	if (!rtOpts.logStatistics) {
		return;
	}

	if(rtOpts.statisticsLog.logEnabled && rtOpts.statisticsLog.logFP != NULL)
	    destination = rtOpts.statisticsLog.logFP;
	else
	    destination = stdout;

	if (ptpClock->resetStatisticsLog) {
		ptpClock->resetStatisticsLog = FALSE;
		fprintf(destination,"# %s, State, Clock ID, One Way Delay, "
		       "Offset From Master, Slave to Master, "
		       "Master to Slave, Observed Drift, Last packet Received"
#ifdef PTPD_STATISTICS
			", One Way Delay Mean, One Way Delay Std Dev, Offset From Master Mean, Offset From Master Std Dev, Observed Drift Mean, Observed Drift Std Dev, raw delayMS, raw delaySM"
#endif
			"\n", (rtOpts.statisticsTimestamp == TIMESTAMP_BOTH) ? "Timestamp, Unix timestamp" : "Timestamp");
	}
	memset(sbuf, ' ', sizeof(sbuf));

	getTime(&now);

	/*
	 * print one log entry per X seconds for Sync and DelayResp messages, to reduce disk usage.
	 */

	if ((ptpClock->portState == PTP_SLAVE) && (rtOpts.statisticsLogInterval)) {
			
		switch(ptpClock->char_last_msg) {
			case 'S':
			if((now.seconds - prev_now_sync.seconds) < rtOpts.statisticsLogInterval){
				DBGV("Suppressed Sync statistics log entry - statisticsLogInterval configured\n");
				return;
			}
			prev_now_sync = now;
			    break;
			case 'D':
			case 'P':
			if((now.seconds - prev_now_delay.seconds) < rtOpts.statisticsLogInterval){
				DBGV("Suppressed Sync statistics log entry - statisticsLogInterval configured\n");
				return;
			}
			prev_now_delay = now;
			default:
			    break;
		}
	}


	time_s = now.seconds;

	/* output date-time timestamp if configured */
	if (rtOpts.statisticsTimestamp == TIMESTAMP_DATETIME ||
	    rtOpts.statisticsTimestamp == TIMESTAMP_BOTH) {
	    strftime(time_str, MAXTIMESTR, "%Y-%m-%d %X", localtime(&time_s));
	    len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s.%06d, %s, ",
		       time_str, (int)now.nanoseconds/1000, /* Timestamp */
		       translatePortState(ptpClock)); /* State */
	}

	/* output unix timestamp s.ns if configured */
	if (rtOpts.statisticsTimestamp == TIMESTAMP_UNIX ||
	    rtOpts.statisticsTimestamp == TIMESTAMP_BOTH) {
	    len += snprintf(sbuf + len, sizeof(sbuf) - len, "%d.%06d, %s,",
		       now.seconds, now.nanoseconds, /* Timestamp */
		       translatePortState(ptpClock)); /* State */
	}

	if (ptpClock->portState == PTP_SLAVE) {
		len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
			 &ptpClock->parentPortIdentity); /* Clock ID */

		/* 
		 * if grandmaster ID differs from parent port ID then
		 * also print GM ID 
		 */
		if (memcmp(ptpClock->grandmasterIdentity, 
			   ptpClock->parentPortIdentity.clockIdentity,
			   CLOCK_IDENTITY_LENGTH)) {
			len += snprint_ClockIdentity(sbuf + len,
						     sizeof(sbuf) - len,
						     ptpClock->grandmasterIdentity);
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		if(rtOpts.delayMechanism == E2E) {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
						    &ptpClock->meanPathDelay);
		} else {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
						    &ptpClock->peerMeanPathDelay);
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
		    &ptpClock->offsetFromMaster);

		/* print MS and SM with sign */
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");
			
		if(rtOpts.delayMechanism == E2E) {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->delaySM));
		} else {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->pdelaySM));
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
				&(ptpClock->delayMS));

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.09f, %c",
			       ptpClock->servo.observedDrift,
			       ptpClock->char_last_msg);

#ifdef PTPD_STATISTICS

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.09f, %.00f, %.09f, %.00f",
			       ptpClock->slaveStats.owdMean,
			       ptpClock->slaveStats.owdStdDev * 1E9,
			       ptpClock->slaveStats.ofmMean,
			       ptpClock->slaveStats.ofmStdDev * 1E9);

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.0f, %.0f, ",
			       ptpClock->servo.driftMean,
			       ptpClock->servo.driftStdDev);

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->rawDelayMS));

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->rawDelaySM));

#endif /* PTPD_STATISTICS */

	} else {
		if ((ptpClock->portState == PTP_MASTER) || (ptpClock->portState == PTP_PASSIVE)) {

			len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
				 &ptpClock->parentPortIdentity);
							 
			//len += snprintf(sbuf + len, sizeof(sbuf) - len, ")");
		}

		/* show the current reset number on the log */
		if (ptpClock->portState == PTP_LISTENING) {
			len += snprintf(sbuf + len,
						     sizeof(sbuf) - len,
						     " %d ", ptpClock->resetCount);
		}
	}
	
	/* add final \n in normal status lines */
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");

#if 0   /* NOTE: Do we want this? */
	if (rtOpts.nonDaemon) {
		/* in -C mode, adding an extra \n makes stats more clear intermixed with debug comments */
		len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");
	}
#endif
	/* fprintf may get interrupted by a signal - silently retry once */
	if (fprintf(destination, "%s", sbuf) < len) {
	    if (fprintf(destination, "%s", sbuf) < len) {
		if(!errorMsg) {
		    PERROR("Error while writing statistics");
		}
		errorMsg = TRUE;
	    }
	}

	if(destination == rtOpts.statisticsLog.logFP) {
		if (maintainLogSize(&rtOpts.statisticsLog))
			ptpClock->resetStatisticsLog = TRUE;
	}

}

/* periodic status update */
void
periodicUpdate(const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    char tmpBuf[200];
    char masterIdBuf[150];
    int len = 0;

    memset(tmpBuf, 0, sizeof(tmpBuf));
    memset(masterIdBuf, 0, sizeof(masterIdBuf));

    TimeInternal *mpd = &ptpClock->meanPathDelay;

    len += snprint_PortIdentity(masterIdBuf + len, sizeof(masterIdBuf) - len,
	    &ptpClock->parentPortIdentity);
    if(ptpClock->masterAddr) {
	char strAddr[MAXHOSTNAMELEN];
	struct in_addr tmpAddr;
	tmpAddr.s_addr = ptpClock->masterAddr;
	inet_ntop(AF_INET, (struct sockaddr_in *)(&tmpAddr), strAddr, MAXHOSTNAMELEN);
	len += snprintf(masterIdBuf + len, sizeof(masterIdBuf) - len, " (IPv4:%s)",strAddr);
    }

    if(ptpClock->delayMechanism == P2P) {
	mpd = &ptpClock->peerMeanPathDelay;
    }

    if(ptpClock->portState == PTP_SLAVE) {
	snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), &ptpClock->offsetFromMaster);
#ifdef PTPD_STATISTICS
	if(ptpClock->slaveStats.statsCalculated) {
	    INFO("Status update: state %s, best master %s, ofm %s s, ofm_mean % .09f s, ofm_dev % .09f s\n",
		portState_getName(ptpClock->portState),
		masterIdBuf,
		tmpBuf,
		ptpClock->slaveStats.ofmMean,
		ptpClock->slaveStats.ofmStdDev);
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), mpd);
	    if (ptpClock->delayMechanism == E2E) {
		INFO("Status update: state %s, best master %s, mpd %s s, mpd_mean % .09f s, mpd_dev % .09f s\n",
		    portState_getName(ptpClock->portState),
		    masterIdBuf,
		    tmpBuf,
		    ptpClock->slaveStats.owdMean,
		    ptpClock->slaveStats.owdStdDev);
	    } else if(ptpClock->delayMechanism == P2P) {
		INFO("Status update: state %s, best master %s, mpd %s s\n", portState_getName(ptpClock->portState), masterIdBuf, tmpBuf);
	    }
	} else {
	    INFO("Status update: state %s, best master %s, ofm %s s\n", portState_getName(ptpClock->portState), masterIdBuf, tmpBuf);
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), mpd);
	    if(ptpClock->delayMechanism != DELAY_DISABLED) {
		INFO("Status update: state %s, best master %s, mpd %s s\n", portState_getName(ptpClock->portState), masterIdBuf, tmpBuf);
	    }
	}
#else
	INFO("Status update: state %s, best master %s, ofm %s s\n", portState_getName(ptpClock->portState), masterIdBuf, tmpBuf);
	snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), mpd);
	if(ptpClock->delayMechanism != DELAY_DISABLED) {
	    INFO("Status update: state %s, best master %s, mpd %s s\n", portState_getName(ptpClock->portState), masterIdBuf, tmpBuf);
	}
#endif /* PTPD_STATISTICS */
    } else if(ptpClock->portState == PTP_MASTER) {
	if(rtOpts->unicastNegotiation || ptpClock->unicastDestinationCount) {
	    INFO("Status update: state %s, %d slaves\n", portState_getName(ptpClock->portState),
	    ptpClock->unicastDestinationCount + ptpClock->slaveCount);
	} else {
	    INFO("Status update: state %s\n", portState_getName(ptpClock->portState));
	}
    } else {
	INFO("Status update: state %s\n", portState_getName(ptpClock->portState));
    }
}


void 
displayStatus(PtpClock *ptpClock, const char *prefixMessage)
{

	static char sbuf[SCREEN_BUFSZ];
	char strAddr[MAXHOSTNAMELEN];
	int len = 0;

	memset(strAddr, 0, sizeof(strAddr));
	memset(sbuf, ' ', sizeof(sbuf));
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s", prefixMessage);
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s", 
			portState_getName(ptpClock->portState));

	if (ptpClock->portState >= PTP_MASTER) {
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", Best master: ");
		len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
			&ptpClock->parentPortIdentity);
		if(ptpClock->masterAddr) {
		    struct in_addr tmpAddr;
		    tmpAddr.s_addr = ptpClock->masterAddr;
		    inet_ntop(AF_INET, (struct sockaddr_in *)(&tmpAddr), strAddr, MAXHOSTNAMELEN);
		    len += snprintf(sbuf + len, sizeof(sbuf) - len, " (IPv4:%s)",strAddr);
		}
        }
	if(ptpClock->portState == PTP_MASTER)
    		len += snprintf(sbuf + len, sizeof(sbuf) - len, " (self)");
        len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");
        NOTICE("%s",sbuf);
}

#define STATUSPREFIX "%-19s:"
void
writeStatusFile(PtpClock *ptpClock,const RunTimeOpts *rtOpts, Boolean quiet)
{

	char outBuf[2048];
	char tmpBuf[200];

	if(rtOpts->statusLog.logFP == NULL)
	    return;
	
	char timeStr[MAXTIMESTR];
	char hostName[MAXHOSTNAMELEN];

	struct timeval now;
	memset(hostName, 0, MAXHOSTNAMELEN);

	gethostname(hostName, MAXHOSTNAMELEN);
	gettimeofday(&now, 0);
	strftime(timeStr, MAXTIMESTR, "%a %b %d %X %Z %Y", localtime((time_t*)&now.tv_sec));
	
	FILE* out = rtOpts->statusLog.logFP;
	memset(outBuf, 0, sizeof(outBuf));

	setbuf(out, outBuf);
	if(ftruncate(fileno(out), 0) < 0) {
	    DBG("writeStatusFile: ftruncate() failed\n");
	}
	rewind(out);

	fprintf(out, 		STATUSPREFIX"  %s, PID %d\n","Host info", hostName, (int)getpid());
	fprintf(out, 		STATUSPREFIX"  %s\n","Local time", timeStr);
	strftime(timeStr, MAXTIMESTR, "%a %b %d %X %Z %Y", gmtime((time_t*)&now.tv_sec));
	fprintf(out, 		STATUSPREFIX"  %s\n","Kernel time", timeStr);
	fprintf(out, 		STATUSPREFIX"  %s%s\n","Interface", rtOpts->ifaceName,
		(rtOpts->backupIfaceEnabled && ptpClock->runningBackupInterface) ? " (backup)" : (rtOpts->backupIfaceEnabled)?
		    " (primary)" : "");
	fprintf(out, 		STATUSPREFIX"  %s\n","Preset", dictionary_get(rtOpts->currentConfig, "ptpengine:preset", ""));
	fprintf(out, 		STATUSPREFIX"  %s%s","Transport", dictionary_get(rtOpts->currentConfig, "ptpengine:transport", ""),
		(rtOpts->transport==UDP_IPV4 && rtOpts->pcap == TRUE)?" + libpcap":"");

	if(rtOpts->transport != IEEE_802_3) {
	    fprintf(out,", %s", dictionary_get(rtOpts->currentConfig, "ptpengine:ip_mode", ""));
	    fprintf(out,"%s", rtOpts->unicastNegotiation ? " negotiation":"");
	}

	fprintf(out,"\n");

	fprintf(out, 		STATUSPREFIX"  %s\n","Delay mechanism", dictionary_get(rtOpts->currentConfig, "ptpengine:delay_mechanism", ""));
	if(ptpClock->portState >= PTP_MASTER) {
	fprintf(out, 		STATUSPREFIX"  %s\n","Sync mode", ptpClock->twoStepFlag ? "TWO_STEP" : "ONE_STEP");
	}
	if(ptpClock->slaveOnly && rtOpts->anyDomain) {
		fprintf(out, 		STATUSPREFIX"  %d, preferred %d\n","PTP domain",
		ptpClock->domainNumber, rtOpts->domainNumber);
	} else if(ptpClock->slaveOnly && rtOpts->unicastNegotiation) {
		fprintf(out, 		STATUSPREFIX"  %d, default %d\n","PTP domain", ptpClock->domainNumber, rtOpts->domainNumber);
	} else {
		fprintf(out, 		STATUSPREFIX"  %d\n","PTP domain", ptpClock->domainNumber);
	}
	fprintf(out, 		STATUSPREFIX"  %s\n","Port state", portState_getName(ptpClock->portState));

	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_PortIdentity(tmpBuf, sizeof(tmpBuf),
	    &ptpClock->portIdentity);
	fprintf(out, 		STATUSPREFIX"  %s\n","Local port ID", tmpBuf);


	if(ptpClock->portState >= PTP_MASTER) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_PortIdentity(tmpBuf, sizeof(tmpBuf),
	    &ptpClock->parentPortIdentity);
	fprintf(out, 		STATUSPREFIX"  %s","Best master ID", tmpBuf);
	if(ptpClock->portState == PTP_MASTER)
	    fprintf(out," (self)");
	    fprintf(out,"\n");
	}
	if(rtOpts->transport == UDP_IPV4 &&
	    ptpClock->portState > PTP_MASTER &&
	    ptpClock->masterAddr) {
	    {
	    struct in_addr tmpAddr;
	    tmpAddr.s_addr = ptpClock->masterAddr;
	    fprintf(out, 		STATUSPREFIX"  %s\n","Best master IP", inet_ntoa(tmpAddr));
	    }
	}
	if(ptpClock->portState == PTP_SLAVE) {
	fprintf(out, 		STATUSPREFIX"  Priority1 %d, Priority2 %d, clockClass %d","GM priority", 
	ptpClock->grandmasterPriority1, ptpClock->grandmasterPriority2, ptpClock->grandmasterClockQuality.clockClass);
	if(rtOpts->unicastNegotiation && ptpClock->parentGrants != NULL ) {
	    	fprintf(out, ", localPref %d", ptpClock->parentGrants->localPreference);
	}
	fprintf(out, "\n");
	}

	if(ptpClock->clockQuality.clockClass < 128 ||
		ptpClock->portState == PTP_SLAVE ||
		ptpClock->portState == PTP_PASSIVE){
	fprintf(out, 		STATUSPREFIX"  ","Time properties");
	fprintf(out, "%s timescale, ",ptpClock->timePropertiesDS.ptpTimescale ? "PTP":"ARB");
	fprintf(out, "tracbl: time %s, freq %s, src: %s(0x%02x)\n", ptpClock->timePropertiesDS.timeTraceable ? "Y" : "N",
							ptpClock->timePropertiesDS.frequencyTraceable ? "Y" : "N",
							getTimeSourceName(ptpClock->timePropertiesDS.timeSource),
							ptpClock->timePropertiesDS.timeSource);
	fprintf(out, 		STATUSPREFIX"  ","UTC properties");
	fprintf(out, "UTC valid: %s", ptpClock->timePropertiesDS.currentUtcOffsetValid ? "Y" : "N");
	fprintf(out, ", UTC offset: %d",ptpClock->timePropertiesDS.currentUtcOffset);
	fprintf(out, "%s",ptpClock->timePropertiesDS.leap61 ? 
			", LEAP61 pending" : ptpClock->timePropertiesDS.leap59 ? ", LEAP59 pending" : "");
	if (ptpClock->portState == PTP_SLAVE) {	
	    fprintf(out, "%s", rtOpts->preferUtcValid ? ", prefer UTC" : "");
	    fprintf(out, "%s", rtOpts->requireUtcValid ? ", require UTC" : "");
	}
	fprintf(out,"\n");
	}

	if(ptpClock->portState == PTP_SLAVE) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->offsetFromMaster);
	fprintf(out, 		STATUSPREFIX" %s s","Offset from Master", tmpBuf);
#ifdef PTPD_STATISTICS
	if(ptpClock->slaveStats.statsCalculated)
	fprintf(out, ", mean % .09f s, dev % .09f s",
		ptpClock->slaveStats.ofmMean,
		ptpClock->slaveStats.ofmStdDev
	);
#endif /* PTPD_STATISTICS */
	    fprintf(out,"\n");

	if(ptpClock->delayMechanism == E2E) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->meanPathDelay);
	fprintf(out, 		STATUSPREFIX" %s s","Mean Path Delay", tmpBuf);
#ifdef PTPD_STATISTICS
	if(ptpClock->slaveStats.statsCalculated)
	fprintf(out, ", mean % .09f s, dev % .09f s",
		ptpClock->slaveStats.owdMean,
		ptpClock->slaveStats.owdStdDev
	);
#endif /* PTPD_STATISTICS */
	fprintf(out,"\n");
	}
	if(ptpClock->delayMechanism == P2P) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->peerMeanPathDelay);
	fprintf(out, 		STATUSPREFIX" %s s","Mean Path (p)Delay", tmpBuf);
	fprintf(out,"\n");
	}

	fprintf(out, 		STATUSPREFIX"  ","Clock status");
		if(rtOpts->enablePanicMode) {
	    if(ptpClock->panicMode) {
		fprintf(out,"panic mode,");

	    }
	}
	if(rtOpts->calibrationDelay) {
	    fprintf(out, "%s, ",
		ptpClock->isCalibrated ? "calibrated" : "not calibrated");
	}
	fprintf(out, "%s",
		ptpClock->clockControl.granted ? "in control" : "no control");
	if(rtOpts->noAdjust) {
	    fprintf(out, ", read-only");
	} 
#ifdef PTPD_STATISTICS	
	else {
	    if (rtOpts->servoStabilityDetection) {
		fprintf(out, ", %s",
		    ptpClock->servo.isStable ? "stabilised" : "not stabilised");
	    }
	}
#endif /* PTPD_STATISTICS */
	fprintf(out,"\n");


	fprintf(out, 		STATUSPREFIX" % .03f ppm","Clock correction",
			    ptpClock->servo.observedDrift / 1000.0);
if(ptpClock->servo.runningMaxOutput)
	fprintf(out, " (slewing at maximum rate)");
else {
#ifdef PTPD_STATISTICS
	if(ptpClock->slaveStats.statsCalculated)
	    fprintf(out, ", mean % .03f ppm, dev % .03f ppm",
		ptpClock->servo.driftMean / 1000.0,
		ptpClock->servo.driftStdDev / 1000.0
	    );
	if(rtOpts->servoStabilityDetection) {
	    fprintf(out, ", dev thr % .03f ppm", 
		ptpClock->servo.stabilityThreshold / 1000.0
	    );
	}
#endif /* PTPD_STATISTICS */
}
	    fprintf(out,"\n");


	}



	if(ptpClock->portState == PTP_MASTER || ptpClock->portState == PTP_PASSIVE) {

	fprintf(out, 		STATUSPREFIX"  %d","Priority1 ", ptpClock->priority1);
	if(ptpClock->portState == PTP_PASSIVE)
		fprintf(out, " (best master: %d)", ptpClock->grandmasterPriority1);
	fprintf(out,"\n");
	fprintf(out, 		STATUSPREFIX"  %d","Priority2 ", ptpClock->priority2);
	if(ptpClock->portState == PTP_PASSIVE)
		fprintf(out, " (best master: %d)", ptpClock->grandmasterPriority2);
	fprintf(out,"\n");
	fprintf(out, 		STATUSPREFIX"  %d","ClockClass ", ptpClock->clockQuality.clockClass);
	if(ptpClock->portState == PTP_PASSIVE)
		fprintf(out, " (best master: %d)", ptpClock->grandmasterClockQuality.clockClass);
	fprintf(out,"\n");
	if(ptpClock->delayMechanism == P2P) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->peerMeanPathDelay);
	fprintf(out, 		STATUSPREFIX" %s s","Mean Path (p)Delay", tmpBuf);
	fprintf(out,"\n");
	}

	}

	if(ptpClock->portState == PTP_MASTER || ptpClock->portState == PTP_PASSIVE ||
	    ptpClock->portState == PTP_SLAVE) {

	fprintf(out,		STATUSPREFIX"  ","Message rates");

	if (ptpClock->logSyncInterval == UNICAST_MESSAGEINTERVAL)
	    fprintf(out,"[UC-unknown]");
	else if (ptpClock->logSyncInterval <= 0)
	    fprintf(out,"%.0f/s",pow(2,-ptpClock->logSyncInterval));
	else
	    fprintf(out,"1/%.0fs",pow(2,ptpClock->logSyncInterval));
	fprintf(out, " sync");


	if(ptpClock->delayMechanism == E2E) {
		if (ptpClock->logMinDelayReqInterval == UNICAST_MESSAGEINTERVAL)
		    fprintf(out,", [UC-unknown]");
		else if (ptpClock->logMinDelayReqInterval <= 0)
		    fprintf(out,", %.0f/s",pow(2,-ptpClock->logMinDelayReqInterval));
		else
		    fprintf(out,", 1/%.0fs",pow(2,ptpClock->logMinDelayReqInterval));
		fprintf(out, " delay");
	}

	if(ptpClock->delayMechanism == P2P) {
		if (ptpClock->logMinPdelayReqInterval == UNICAST_MESSAGEINTERVAL)
		    fprintf(out,", [UC-unknown]");
		else if (ptpClock->logMinPdelayReqInterval <= 0)
		    fprintf(out,", %.0f/s",pow(2,-ptpClock->logMinPdelayReqInterval));
		else
		    fprintf(out,", 1/%.0fs",pow(2,ptpClock->logMinPdelayReqInterval));
		fprintf(out, " pdelay");
	}

	if (ptpClock->logAnnounceInterval == UNICAST_MESSAGEINTERVAL)
	    fprintf(out,", [UC-unknown]");
	else if (ptpClock->logAnnounceInterval <= 0)
	    fprintf(out,", %.0f/s",pow(2,-ptpClock->logAnnounceInterval));
	else
	    fprintf(out,", 1/%.0fs",pow(2,ptpClock->logAnnounceInterval));
	fprintf(out, " announce");

	    fprintf(out,"\n");

	}

	fprintf(out, 		STATUSPREFIX"  ","TimingService");

	fprintf(out, "current %s, best %s, pref %s", (timingDomain.current != NULL) ? timingDomain.current->id : "none",
						(timingDomain.best != NULL) ? timingDomain.best->id : "none",
		    				(timingDomain.preferred != NULL) ? timingDomain.preferred->id : "none");

	if((timingDomain.current != NULL) && 
	    (timingDomain.current->holdTimeLeft > 0)) {
		fprintf(out, ", hold %d sec", timingDomain.current->holdTimeLeft);
	} else	if(timingDomain.electionLeft) {
		fprintf(out, ", elec %d sec", timingDomain.electionLeft);
	}

	fprintf(out, "\n");

	fprintf(out, 		STATUSPREFIX"  ","TimingServices");

	fprintf(out, "total %d, avail %d, oper %d, idle %d, in_ctrl %d%s\n", 
				    timingDomain.serviceCount,
				    timingDomain.availableCount,
				    timingDomain.operationalCount,
				    timingDomain.idleCount,
				    timingDomain.controlCount,
				    timingDomain.controlCount > 1 ? " (!)":"");

	fprintf(out, 		STATUSPREFIX"  ","Performance");
	fprintf(out,"Message RX %d/s, TX %d/s", ptpClock->counters.receivedMessageRate,
						  ptpClock->counters.sentMessageRate);
	if(ptpClock->portState == PTP_MASTER) {
		if(rtOpts->unicastNegotiation) {
		    fprintf(out,", slaves %d", ptpClock->slaveCount);
		} else if (rtOpts->ipMode == IPMODE_UNICAST) {
		    fprintf(out,", slaves %d", ptpClock->unicastDestinationCount);
		}
	}

	fprintf(out,"\n");

	if ( ptpClock->portState == PTP_SLAVE ||
	    ptpClock->clockQuality.clockClass == 255 ) {

	fprintf(out, 		STATUSPREFIX"  %lu\n","Announce received",
	    (unsigned long)ptpClock->counters.announceMessagesReceived);
	fprintf(out, 		STATUSPREFIX"  %lu\n","Sync received",
	    (unsigned long)ptpClock->counters.syncMessagesReceived);
	if(ptpClock->twoStepFlag)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Follow-up received",
	    (unsigned long)ptpClock->counters.followUpMessagesReceived);
	if(ptpClock->delayMechanism == E2E) {
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayReq sent",
		    (unsigned long)ptpClock->counters.delayReqMessagesSent);
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayResp received",
		    (unsigned long)ptpClock->counters.delayRespMessagesReceived);
	}
	}

	if( ptpClock->portState == PTP_MASTER ||
	    ptpClock->clockQuality.clockClass < 128 ) {
	fprintf(out, 		STATUSPREFIX"  %lu received, %lu sent \n","Announce",
	    (unsigned long)ptpClock->counters.announceMessagesReceived,
	    (unsigned long)ptpClock->counters.announceMessagesSent);
	fprintf(out, 		STATUSPREFIX"  %lu\n","Sync sent",
	    (unsigned long)ptpClock->counters.syncMessagesSent);
	if(ptpClock->twoStepFlag)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Follow-up sent",
	    (unsigned long)ptpClock->counters.followUpMessagesSent);

	if(ptpClock->delayMechanism == E2E) {
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayReq received",
		    (unsigned long)ptpClock->counters.delayReqMessagesReceived);
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayResp sent",
		    (unsigned long)ptpClock->counters.delayRespMessagesSent);
	}

	}

	if(ptpClock->delayMechanism == P2P) {

		fprintf(out, 		STATUSPREFIX"  %lu received, %lu sent\n","PdelayReq",
		    (unsigned long)ptpClock->counters.pdelayReqMessagesReceived,
		    (unsigned long)ptpClock->counters.pdelayReqMessagesSent);
		fprintf(out, 		STATUSPREFIX"  %lu received, %lu sent\n","PdelayResp",
		    (unsigned long)ptpClock->counters.pdelayRespMessagesReceived,
		    (unsigned long)ptpClock->counters.pdelayRespMessagesSent);
		fprintf(out, 		STATUSPREFIX"  %lu received, %lu sent\n","PdelayRespFollowUp",
		    (unsigned long)ptpClock->counters.pdelayRespFollowUpMessagesReceived,
		    (unsigned long)ptpClock->counters.pdelayRespFollowUpMessagesSent);

	}

	if(ptpClock->counters.domainMismatchErrors)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Domain Mismatches",
		    (unsigned long)ptpClock->counters.domainMismatchErrors);

	if(ptpClock->counters.ignoredAnnounce)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Ignored Announce",
		    (unsigned long)ptpClock->counters.ignoredAnnounce);

	if(ptpClock->counters.unicastGrantsDenied)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Denied Unicast",
		    (unsigned long)ptpClock->counters.unicastGrantsDenied);

	fprintf(out, 		STATUSPREFIX"  %lu\n","State transitions",
		    (unsigned long)ptpClock->counters.stateTransitions);
	fprintf(out, 		STATUSPREFIX"  %lu\n","PTP Engine resets",
		    (unsigned long)ptpClock->resetCount);


	fflush(out);
}

void 
displayPortIdentity(PortIdentity *port, const char *prefixMessage) 
{
	static char sbuf[SCREEN_BUFSZ];
	int len = 0;

	memset(sbuf, ' ', sizeof(sbuf));
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s ", prefixMessage);
	len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len, port);
        len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");
        INFO("%s",sbuf);
}


void
recordSync(UInteger16 sequenceId, TimeInternal * time)
{
	extern RunTimeOpts rtOpts;
	if (rtOpts.recordLog.logEnabled && rtOpts.recordLog.logFP != NULL) {
		fprintf(rtOpts.recordLog.logFP, "%d %llu\n", sequenceId, 
		  ((time->seconds * 1000000000ULL) + time->nanoseconds)
		);
		maintainLogSize(&rtOpts.recordLog);
	}
}

Boolean
nanoSleep(TimeInternal * t)
{
	struct timespec ts, tr;

	ts.tv_sec = t->seconds;
	ts.tv_nsec = t->nanoseconds;

	if (nanosleep(&ts, &tr) < 0) {
		t->seconds = tr.tv_sec;
		t->nanoseconds = tr.tv_nsec;
		return FALSE;
	}
	return TRUE;
}

void
getTime(TimeInternal * time)
{
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)

	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
		PERROR("clock_gettime() failed, exiting.");
		exit(0);
	}
	time->seconds = tp.tv_sec;
	time->nanoseconds = tp.tv_nsec;

#else

	struct timeval tv;
	gettimeofday(&tv, 0);
	time->seconds = tv.tv_sec;
	time->nanoseconds = tv.tv_usec * 1000;

#endif /* _POSIX_TIMERS */
}

void
getTimeMonotonic(TimeInternal * time)
{
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)

	struct timespec tp;
#ifndef CLOCK_MONOTINIC                                                                                                       
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
#else
	if (clock_gettime(CLOCK_MONOTONIC, &tp) < 0) {
#endif /* CLOCK_MONOTONIC */
		PERROR("clock_gettime() failed, exiting.");
		exit(0);
	}
	time->seconds = tp.tv_sec;
	time->nanoseconds = tp.tv_nsec;
#else

	struct timeval tv;
	gettimeofday(&tv, 0);
	time->seconds = tv.tv_sec;
	time->nanoseconds = tv.tv_usec * 1000;

#endif /* _POSIX_TIMERS */
}


void
setTime(TimeInternal * time)
{

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)

	struct timespec tp;
	tp.tv_sec = time->seconds;
	tp.tv_nsec = time->nanoseconds;

#else

	struct timeval tv;
	tv.tv_sec = time->seconds;
	tv.tv_usec = time->nanoseconds / 1000;

#endif /* _POSIX_TIMERS */

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)

	if (clock_settime(CLOCK_REALTIME, &tp) < 0) {
		PERROR("Could not set system time");
		return;
	}

#else

	settimeofday(&tv, 0);

#endif /* _POSIX_TIMERS */

	struct timespec tmpTs = { time->seconds,0 };

	char timeStr[MAXTIMESTR];
	strftime(timeStr, MAXTIMESTR, "%x %X", localtime(&tmpTs.tv_sec));
	WARNING("Stepped the system clock to: %s.%d\n",
	       timeStr, time->nanoseconds);

}

#ifdef HAVE_LINUX_RTC_H

/* Set the RTC to the desired time time */
void setRtc(TimeInternal *timeToSet)
{

	static Boolean deviceFound = FALSE;
	static char* rtcDev;
	struct tm* tmTime;
	time_t seconds;
	int rtcFd;
	struct stat statBuf;

	if (!deviceFound) {
	    if(stat("/dev/misc/rtc", &statBuf) == 0) {
            	rtcDev="/dev/misc/rtc\0";
		deviceFound = TRUE;
	    } else if(stat("/dev/rtc", &statBuf) == 0) {
            	rtcDev="/dev/rtc\0";
		deviceFound = TRUE;
	    }  else if(stat("/dev/rtc0", &statBuf) == 0) {
            	rtcDev="/dev/rtc0\0";
		deviceFound = TRUE;
	    } else {

			ERROR("Could not set RTC time - no suitable rtc device found\n");
			return;
	    }

	    if(!S_ISCHR(statBuf.st_mode)) {
			ERROR("Could not set RTC time - device %s is not a character device\n",
			rtcDev);
			deviceFound = FALSE;
			return;
	    }

	}

	DBGV("Usable RTC device: %s\n",rtcDev);

	if(timeToSet->seconds == 0 && timeToSet->nanoseconds==0) {
	    getTime(timeToSet);
	}



	if((rtcFd = open(rtcDev, O_RDONLY)) < 0) {
		PERROR("Could not set RTC time: error opening %s", rtcDev);
		return;
	}

	seconds = (time_t)timeToSet->seconds;
	if(timeToSet->nanoseconds >= 500000) seconds++;
	tmTime =  gmtime(&seconds);

	DBGV("Set RTC from %d seconds to y: %d m: %d d: %d \n",timeToSet->seconds,tmTime->tm_year,tmTime->tm_mon,tmTime->tm_mday);

	if(ioctl(rtcFd, RTC_SET_TIME, tmTime) < 0) {
		PERROR("Could not set RTC time on %s - ioctl failed", rtcDev);
		goto cleanup;
	}

	NOTIFY("Succesfully set RTC time using %s\n", rtcDev);

cleanup:

	close(rtcFd);

}

#endif /* HAVE_LINUX_RTC_H */

/* returns a double beween 0.0 and 1.0 */
double 
getRand(void)
{
	return((rand() * 1.0) / RAND_MAX);
}

/* Attempt setting advisory write lock on a file descriptor*/
int
lockFile(int fd)
{
        struct flock fl;

        fl.l_type = DEFAULT_LOCKMODE;
        fl.l_start = 0;
        fl.l_whence = SEEK_SET;
        fl.l_len = 0;
        return(fcntl(fd, F_SETLK, &fl));
}

/* 
 * Check if a file descriptor's lock flags match specified flags,
 * do not acquire lock - just query. If the flags match, populate
 * lockPid with the PID of the process already holding the lock(s)
 * Return values: -1 = error, 0 = locked, 1 = not locked
 */
int checkLockStatus(int fd, short lockType, int *lockPid) {

    struct flock fl;

    memset(&fl, 0, sizeof(fl));

    if(fcntl(fd, F_GETLK, &fl) == -1)
	{
	return -1;
	}
    /* Return 0 if file is already locked, but not by us */
    if((lockType & fl.l_type) && fl.l_pid != getpid()) {
	*lockPid = fl.l_pid;
	return 0;
    }

    return 1;

}

/* 
 * Check if it's possible to acquire lock on a file - if already locked,
 * populate lockPid with the PID currently holding the write lock.
 */
int
checkFileLockable(const char *fileName, int *lockPid) {

    FILE* fileHandle;
    int ret;

    if((fileHandle = fopen(fileName,"w+")) == NULL) {
	PERROR("Could not open %s for writing\n");
	return -1;
    }

    ret = checkLockStatus(fileno(fileHandle), DEFAULT_LOCKMODE, lockPid);
    if (ret == -1) {
	PERROR("Could not check lock status of %s",fileName);
    }

    fclose(fileHandle);
    return ret;
}

/*
 * If automatic lock files are used, check for potential conflicts
 * based on already existing lock files containing our interface name
 * or clock driver name
 */
Boolean
checkOtherLocks(RunTimeOpts* rtOpts)
{


char searchPattern[PATH_MAX];
char * lockPath = 0;
int lockPid = 0;
glob_t matchedFiles;
Boolean ret = TRUE;
int matches = 0, counter = 0;

	/* no need to check locks */
	if(rtOpts->ignore_daemon_lock ||
		!rtOpts->autoLockFile)
			return TRUE;

    /* 
     * Try to discover if we can run in desired mode
     * based on the presence of other lock files
     * and them being lockable
     */

	/* Check for other ptpd running on the same interface - same for all modes */
	snprintf(searchPattern, PATH_MAX,"%s/%s_*_%s.lock",
	    rtOpts->lockDirectory, PTPD_PROGNAME,rtOpts->ifaceName);

	DBGV("SearchPattern: %s\n",searchPattern);
	switch(glob(searchPattern, 0, NULL, &matchedFiles)) {

	    case GLOB_NOSPACE:
	    case GLOB_ABORTED:
		    PERROR("Could not scan %s directory\n", rtOpts->lockDirectory);;
		    ret = FALSE;
		    goto end;
	    default:
		    break;
	}

	counter = matchedFiles.gl_pathc;
	matches = counter;
	while (matches--) {
		lockPath=matchedFiles.gl_pathv[matches];
		DBG("matched: %s\n",lockPath);
	/* check if there is a lock file with our NIC in the name */
	    switch(checkFileLockable(lockPath, &lockPid)) {
		/* Could not check lock status */
		case -1:
		    ERROR("Looks like "USER_DESCRIPTION" may be already running on %s: %s found, but could not check lock\n",
		    rtOpts->ifaceName, lockPath);
		    ret = FALSE;
		    goto end;
		/* It was possible to acquire lock - file looks abandoned */
		case 1:
		    DBG("Lock file %s found, but is not locked for writing.\n", lockPath);
		    ret = TRUE;
		    break;
		/* file is locked */
		case 0:
		    ERROR("Looks like "USER_DESCRIPTION" is already running on %s: %s found and is locked by pid %d\n",
		    rtOpts->ifaceName, lockPath, lockPid);
		    ret = FALSE;
		    goto end;
	    }
	}

	if(matches > 0)
		globfree(&matchedFiles);
	/* Any mode that can control the clock - also check the clock driver */
	if(rtOpts->clockQuality.clockClass > 127 ) {
	    snprintf(searchPattern, PATH_MAX,"%s/%s_%s_*.lock",
	    rtOpts->lockDirectory,PTPD_PROGNAME,DEFAULT_CLOCKDRIVER);
	DBGV("SearchPattern: %s\n",searchPattern);

	switch(glob(searchPattern, 0, NULL, &matchedFiles)) {

	    case GLOB_NOSPACE:
	    case GLOB_ABORTED:
		    PERROR("Could not scan %s directory\n", rtOpts->lockDirectory);;
		    ret = FALSE;
		    goto end;
	    default:
		    break;
	}
	    counter = matchedFiles.gl_pathc;
	    matches = counter;
	    while (counter--) {
		lockPath=matchedFiles.gl_pathv[counter];
		DBG("matched: %s\n",lockPath);
	    /* Check if there is a lock file with our clock driver in the name */
		    switch(checkFileLockable(lockPath, &lockPid)) {
			/* could not check lock status */
			case -1:
			    ERROR("Looks like "USER_DESCRIPTION" may already be controlling the \""DEFAULT_CLOCKDRIVER
				    "\" clock: %s found, but could not check lock status.\n", lockPath);
			    ret = FALSE;
			    goto end;
			/* it was possible to acquire lock - looks abandoned */
			case 1:
			    DBG("Lock file %s found, but is not locked for writing.\n", lockPath);
			    ret = TRUE;
			    break;
			/* file is locked */
			case 0:
			    ERROR("Looks like "USER_DESCRIPTION" is already controlling the \""DEFAULT_CLOCKDRIVER
				    "\" clock: %s found and is locked by pid %d\n", lockPath, lockPid);
			default:
			    ret = FALSE;
			    goto end;
	    }
	}
	}

    ret = TRUE;

end:
    if(matches>0)
	globfree(&matchedFiles);
    return ret;

}


/* Whole block of adjtimex() functions starts here - only for systems with sys/timex.h */

#ifdef HAVE_SYS_TIMEX_H

/*
 * Apply a tick / frequency shift to the kernel clock
 */

Boolean
adjFreq(double adj)
{

	extern RunTimeOpts rtOpts;
	struct timex t;

#ifdef HAVE_STRUCT_TIMEX_TICK
	Integer32 tickAdj = 0;

#ifdef PTPD_DBG2
	double oldAdj = adj;
#endif

#endif /* HAVE_STRUCT_TIMEX_TICK */

	memset(&t, 0, sizeof(t));

	/* Clamp to max PPM */
	if (adj > rtOpts.servoMaxPpb){
		adj = rtOpts.servoMaxPpb;
	} else if (adj < -rtOpts.servoMaxPpb){
		adj = -rtOpts.servoMaxPpb;
	}

/* Y U NO HAVE TICK? */
#ifdef HAVE_STRUCT_TIMEX_TICK

	/* Get the USER_HZ value */
	Integer32 userHZ = sysconf(_SC_CLK_TCK);

	/*
	 * Get the tick resolution (ppb) - offset caused by changing the tick value by 1.
	 * The ticks value is the duration of one tick in us. So with userHz = 100  ticks per second,
	 * change of ticks by 1 (us) means a 100 us frequency shift = 100 ppm = 100000 ppb.
	 * For userHZ = 1000, change by 1 is a 1ms offset (10 times more ticks per second)
	 */
	Integer32 tickRes = userHZ * 1000;

	/*
	 * If we are outside the standard +/-512ppm, switch to a tick + freq combination:
	 * Keep moving ticks from adj to tickAdj until we get back to the normal range.
	 * The offset change will not be super smooth as we flip between tick and frequency,
	 * but this in general should only be happening under extreme conditions when dragging the
	 * offset down from very large values. When maxPPM is left at the default value, behaviour
	 * is the same as previously, clamped to 512ppm, but we keep tick at the base value,
	 * preventing long stabilisation times say when  we had a non-default tick value left over
	 * from a previous NTP run.
	 */
	if (adj > ADJ_FREQ_MAX){
		while (adj > ADJ_FREQ_MAX) {
		    tickAdj++;
		    adj -= tickRes;
		}

	} else if (adj < -ADJ_FREQ_MAX){
		while (adj < -ADJ_FREQ_MAX) {
		    tickAdj--;
		    adj += tickRes;
		}
        }
	/* Base tick duration - 10000 when userHZ = 100 */
	t.tick = 1E6 / userHZ;
	/* Tick adjustment if necessary */
        t.tick += tickAdj;


	t.modes = ADJ_TICK;

#endif /* HAVE_STRUCT_TIMEX_TICK */

	t.modes |= MOD_FREQUENCY;

	double dFreq = adj * ((1 << 16) / 1000.0);
	t.freq = (int) round(dFreq);
#ifdef HAVE_STRUCT_TIMEX_TICK
	DBG2("adjFreq: oldadj: %.09f, newadj: %.09f, tick: %d, tickadj: %d\n", oldAdj, adj,t.tick,tickAdj);
#endif /* HAVE_STRUCT_TIMEX_TICK */
	DBG2("        adj is %.09f;  t freq is %d       (float: %.09f)\n", adj, t.freq,  dFreq);
	
	return !adjtimex(&t);
}


double
getAdjFreq(void)
{
	struct timex t;
	double dFreq;

	DBGV("getAdjFreq called\n");

	memset(&t, 0, sizeof(t));
	t.modes = 0;
	adjtimex(&t);

	dFreq = (t.freq + 0.0) / ((1<<16) / 1000.0);

	DBGV("          kernel adj is: %f, kernel freq is: %d\n",
		dFreq, t.freq);

	return(dFreq);
}


/* First cut on informing the clock */
void
informClockSource(PtpClock* ptpClock)
{
	struct timex tmx;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_MAXERROR | MOD_ESTERROR;

	tmx.maxerror = (ptpClock->offsetFromMaster.seconds * 1E9 +
			ptpClock->offsetFromMaster.nanoseconds) / 1000;
	tmx.esterror = tmx.maxerror;

	if (adjtimex(&tmx) < 0)
		DBG("informClockSource: could not set adjtimex flags: %s", strerror(errno));
}


void
unsetTimexFlags(int flags, Boolean quiet) 
{
	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_STATUS;

	tmx.status = getTimexFlags();
	if(tmx.status == -1) 
		return;
	/* unset all read-only flags */
	tmx.status &= ~STA_RONLY;
	tmx.status &= ~flags;

	ret = adjtimex(&tmx);

	if (ret < 0)
		PERROR("Could not unset adjtimex flags: %s", strerror(errno));

	if(!quiet && ret > 2) {
		switch (ret) {
		case TIME_OOP:
			WARNING("Adjtimex: leap second already in progress\n");
			break;
		case TIME_WAIT:
			WARNING("Adjtimex: leap second already occurred\n");
			break;
#if !defined(TIME_BAD)
		case TIME_ERROR:
#else
		case TIME_BAD:
#endif /* TIME_BAD */
		default:
			DBGV("unsetTimexFlags: adjtimex() returned TIME_BAD\n");
			break;
		}
	}
}

int getTimexFlags(void)
{
	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = 0;
	ret = adjtimex(&tmx);
	if (ret < 0) {
		PERROR("Could not read adjtimex flags: %s", strerror(errno));
		return(-1);

	}
	return( tmx.status );
}

Boolean
checkTimexFlags(int flags) {
	int tflags = getTimexFlags();

	if (tflags == -1) 
		return FALSE;
	return ((tflags & flags) == flags);
}

/*
 * TODO: track NTP API changes - NTP API version check
 * is required - the method of setting the TAI offset
 * may change with next API versions
 */

#if defined(MOD_TAI) &&  NTP_API == 4
void
setKernelUtcOffset(int utc_offset) {

	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_TAI;
	tmx.constant = utc_offset;

	DBG2("Kernel NTP API supports TAI offset. "
	     "Setting TAI offset to %d", utc_offset);

	ret = adjtimex(&tmx);

	if (ret < 0) {
		PERROR("Could not set kernel TAI offset: %s", strerror(errno));
	}
}
Boolean
getKernelUtcOffset(int *utc_offset) {

	static Boolean warned = FALSE;
	int ret;

#if defined(HAVE_NTP_GETTIME)
	struct ntptimeval ntpv;
	memset(&ntpv, 0, sizeof(ntpv));
	ret = ntp_gettime(&ntpv);
#else
	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = 0;
	ret = adjtimex(&tmx);
#endif /* HAVE_NTP_GETTIME */

	if (ret < 0) {
		if(!warned) {
		    PERROR("Could not read adjtimex/ntp_gettime flags: %s", strerror(errno));
		}
		warned = TRUE;
		return FALSE;

	}
#if !defined(HAVE_NTP_GETTIME) && defined(HAVE_STRUCT_TIMEX_TAI)
	*utc_offset = ( tmx.tai );
	return TRUE;
#elif defined(HAVE_NTP_GETTIME) && defined(HAVE_STRUCT_NTPTIMEVAL_TAI)
	*utc_offset = (int)(ntpv.tai);
	return TRUE;
#endif /* HAVE_STRUCT_TIMEX_TAI */
	if(!warned) {
	    WARNING("No OS support for kernel TAI/UTC offset information. Cannot read UTC offset.\n");
	}
	warned = TRUE;
	return FALSE;
}
#endif /* MOD_TAI */

void
setTimexFlags(int flags, Boolean quiet)
{
	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_STATUS;

	tmx.status = getTimexFlags();
	if(tmx.status == -1) 
		return;
	/* unset all read-only flags */
	tmx.status &= ~STA_RONLY;
	tmx.status |= flags;

	ret = adjtimex(&tmx);

	if (ret < 0)
		PERROR("Could not set adjtimex flags: %s", strerror(errno));

	if(!quiet && ret > 2) {
		switch (ret) {
		case TIME_OOP:
			WARNING("Adjtimex: leap second already in progress\n");
			break;
		case TIME_WAIT:
			WARNING("Adjtimex: leap second already occurred\n");
			break;
#if !defined(TIME_BAD)
		case TIME_ERROR:
#else
		case TIME_BAD:
#endif /* TIME_BAD */
		default:
			DBGV("unsetTimexFlags: adjtimex() returned TIME_BAD\n");
			break;
		}
	}
}

#else /* SYS_TIMEX_H */

void
adjTime(Integer32 nanoseconds)
{

	struct timeval t;

	t.tv_sec = 0;
	t.tv_usec = nanoseconds / 1000;

	if (adjtime(&t, NULL) < 0)
		PERROR("failed to ajdtime");

}

#endif /* HAVE_SYS_TIMEX_H */

#define DRIFTFORMAT "%.0f"

void
restoreDrift(PtpClock * ptpClock, const RunTimeOpts * rtOpts, Boolean quiet)
{

	FILE *driftFP;
	Boolean reset_offset = FALSE;
	double recovered_drift;

	DBGV("restoreDrift called\n");

	if (ptpClock->drift_saved && rtOpts->drift_recovery_method > 0 ) {
		ptpClock->servo.observedDrift = ptpClock->last_saved_drift;
		if (!rtOpts->noAdjust && ptpClock->clockControl.granted) {
#ifndef HAVE_SYS_TIMEX_H
			adjTime(-ptpClock->last_saved_drift);
#else
			adjFreq_wrapper(rtOpts, ptpClock, -ptpClock->last_saved_drift);
#endif /* HAVE_SYS_TIMEX_H */
		}
		DBG("loaded cached drift");
		return;
	}

	switch (rtOpts->drift_recovery_method) {

		case DRIFT_FILE:

			if( (driftFP = fopen(rtOpts->driftFile,"r")) == NULL) {
			    if(errno!=ENOENT) {
				    PERROR("Could not open drift file: %s - using current kernel frequency offset. Ignore this error if ",
				    rtOpts->driftFile);
			    } else {
				    NOTICE("Drift file %s not found - will be initialised on write\n",rtOpts->driftFile);
			    }
			} else if (fscanf(driftFP, "%lf", &recovered_drift) != 1) {
				PERROR("Could not load saved offset from drift file - using current kernel frequency offset");
				fclose(driftFP);
			} else {

			if(recovered_drift == 0)
				recovered_drift = 0;

			fclose(driftFP);
			if(quiet)
				DBGV("Observed drift loaded from %s: "DRIFTFORMAT" ppb\n",
					rtOpts->driftFile,
					recovered_drift);
			else
				INFO("Observed drift loaded from %s: "DRIFTFORMAT" ppb\n",
					rtOpts->driftFile,
					recovered_drift);
				break;
			}

		case DRIFT_KERNEL:
#ifdef HAVE_SYS_TIMEX_H
			recovered_drift = -getAdjFreq();
#else
			recovered_drift = 0;
#endif /* HAVE_SYS_TIMEX_H */
			if(recovered_drift == 0)
				recovered_drift = 0;

			if (quiet)
				DBGV("Observed_drift loaded from kernel: "DRIFTFORMAT" ppb\n",
					recovered_drift);
			else
				INFO("Observed_drift loaded from kernel: "DRIFTFORMAT" ppb\n",
					recovered_drift);

		break;


		default:

			reset_offset = TRUE;

	}

	if (reset_offset) {
#ifdef HAVE_SYS_TIMEX_H
		if (!rtOpts->noAdjust && ptpClock->clockControl.granted)
		  adjFreq_wrapper(rtOpts, ptpClock, 0);
#endif /* HAVE_SYS_TIMEX_H */
		ptpClock->servo.observedDrift = 0;
		return;
	}

	ptpClock->servo.observedDrift = recovered_drift;

	ptpClock->drift_saved = TRUE;
	ptpClock->last_saved_drift = recovered_drift;

#ifdef HAVE_SYS_TIMEX_H
	if (!rtOpts->noAdjust)
		adjFreq(-recovered_drift);
#endif /* HAVE_SYS_TIMEX_H */
}



void
saveDrift(PtpClock * ptpClock, const RunTimeOpts * rtOpts, Boolean quiet)
{
	FILE *driftFP;

	DBGV("saveDrift called\n");

       if(ptpClock->portState == PTP_PASSIVE ||
              ptpClock->portState == PTP_MASTER ||
                ptpClock->clockQuality.clockClass < 128) {
                    DBGV("We're not slave - not saving drift\n");
                    return;
            }

        if(ptpClock->servo.observedDrift == 0.0 &&
            ptpClock->portState == PTP_LISTENING )
                return;

	if (rtOpts->drift_recovery_method > 0) {
		ptpClock->last_saved_drift = ptpClock->servo.observedDrift;
		ptpClock->drift_saved = TRUE;
	}

	if (rtOpts->drift_recovery_method != DRIFT_FILE)
		return;

	if(ptpClock->servo.runningMaxOutput) {
	    DBG("Servo running at maximum shift - not saving drift file");
	    return;
	}

	if( (driftFP = fopen(rtOpts->driftFile,"w")) == NULL) {
		PERROR("Could not open drift file %s for writing", rtOpts->driftFile);
		return;
	}

	/* The fractional part really won't make a difference here */
	fprintf(driftFP, "%d\n", (int)round(ptpClock->servo.observedDrift));

	if (quiet) {
		DBGV("Wrote observed drift ("DRIFTFORMAT" ppb) to %s\n", 
		    ptpClock->servo.observedDrift, rtOpts->driftFile);
	} else {
		INFO("Wrote observed drift ("DRIFTFORMAT" ppb) to %s\n", 
		    ptpClock->servo.observedDrift, rtOpts->driftFile);
	}
	fclose(driftFP);
}

#undef DRIFTFORMAT

int parseLeapFile(char *path, LeapSecondInfo *info)
{
    FILE *leapFP;
    TimeInternal now;
    char lineBuf[PATH_MAX];

    unsigned long ntpSeconds = 0;
    Integer32 utcSeconds = 0;
    Integer32 utcExpiry = 0;
    int ntpOffset = 0;
    int res;

    getTime(&now);

    info->valid = FALSE;

    if( (leapFP = fopen(path,"r")) == NULL) {
	PERROR("Could not open leap second list file %s", path);
	return 0;
    } else

    memset(info, 0, sizeof(LeapSecondInfo));

    while (fgets(lineBuf, PATH_MAX - 1, leapFP) != NULL) {  

	/* capture file expiry time */
	res = sscanf(lineBuf, "#@ %lu", &ntpSeconds);
	if(res == 1) {
	    utcExpiry = ntpSeconds - NTP_EPOCH;
	    DBG("leapfile expiry %d\n", utcExpiry);
	}
	/* capture leap seconds information */
	res = sscanf(lineBuf, "%lu %d", &ntpSeconds, &ntpOffset);
	if(res ==2) {
	    utcSeconds = ntpSeconds - NTP_EPOCH;
	    DBG("leapfile date %d offset %d\n", utcSeconds, ntpOffset);

	    /* next leap second date found */

	    if((now.seconds ) < utcSeconds) {
		info->nextOffset = ntpOffset;
		info->endTime = utcSeconds;
		info->startTime = utcSeconds - 86400;
		break;
	    } else 
	    /* current leap second value found */
		if(now.seconds >= utcSeconds) {
		info->currentOffset = ntpOffset;
	    }

	}

    }    

    fclose(leapFP);

    /* leap file past expiry date */
    if(utcExpiry && utcExpiry < now.seconds) {
	WARNING("Leap seconds file is expired. Please download the current version\n");
	return 0;
    }

    /* we have the current offset - the rest can be invalid but at least we have this */
    if(info->currentOffset != 0) {
	info->offsetValid = TRUE;
    }

    /* if anything failed, return 0 so we know we cannot use leap file information */
    if((info->startTime == 0) || (info->endTime == 0) || 
	(info->currentOffset == 0) || (info->nextOffset == 0)) {
	return 0;
	INFO("Leap seconds file %s loaded (incomplete): now %d, current %d next %d from %d to %d, type %s\n", path,
	now.seconds,
	info->currentOffset, info->nextOffset, 
	info->startTime, info->endTime, info->leapType > 0 ? "positive" : info->leapType < 0 ? "negative" : "unknown");
    }

    if(info->nextOffset > info->currentOffset) {
	info->leapType = 1;
    }

    if(info->nextOffset < info->currentOffset) {
	info->leapType = -1;
    }

    INFO("Leap seconds file %s loaded: now %d, current %d next %d from %d to %d, type %s\n", path,
	now.seconds,
	info->currentOffset, info->nextOffset, 
	info->startTime, info->endTime, info->leapType > 0 ? "positive" : info->leapType < 0 ? "negative" : "unknown");
    info->valid = TRUE;
    return 1;

}

void
updateXtmp (TimeInternal oldTime, TimeInternal newTime)
{

/* Add the old time entry to utmp/wtmp */

/* About as long as the ntpd implementation, but not any less ugly */

#ifdef HAVE_UTMPX_H
		struct utmpx utx;
	memset(&utx, 0, sizeof(utx));
		strncpy(utx.ut_user, "date", sizeof(utx.ut_user));
#ifndef OTIME_MSG
		strncpy(utx.ut_line, "|", sizeof(utx.ut_line));
#else
		strncpy(utx.ut_line, OTIME_MSG, sizeof(utx.ut_line));
#endif /* OTIME_MSG */
#ifdef OLD_TIME
		utx.ut_tv.tv_sec = oldTime.seconds;
		utx.ut_tv.tv_usec = oldTime.nanoseconds / 1000;
		utx.ut_type = OLD_TIME;
#else /* no ut_type */
		utx.ut_time = oldTime.seconds;
#endif /* OLD_TIME */

/* ======== BEGIN  OLD TIME EVENT - UTMPX / WTMPX =========== */
#ifdef HAVE_UTMPXNAME
		utmpxname("/var/log/utmp");
#endif /* HAVE_UTMPXNAME */
		setutxent();
		pututxline(&utx);
		endutxent();
#ifdef HAVE_UPDWTMPX
		updwtmpx("/var/log/wtmp", &utx);
#endif /* HAVE_IPDWTMPX */
/* ======== END    OLD TIME EVENT - UTMPX / WTMPX =========== */

#else /* NO UTMPX_H */

#ifdef HAVE_UTMP_H
		struct utmp ut;
		memset(&ut, 0, sizeof(ut));
		strncpy(ut.ut_name, "date", sizeof(ut.ut_name));
#ifndef OTIME_MSG
		strncpy(ut.ut_line, "|", sizeof(ut.ut_line));
#else
		strncpy(ut.ut_line, OTIME_MSG, sizeof(ut.ut_line));
#endif /* OTIME_MSG */

#ifdef OLD_TIME
		ut.ut_tv.tv_sec = oldTime.seconds;
		ut.ut_tv.tv_usec = oldTime.nanoseconds / 1000;
		ut.ut_type = OLD_TIME;
#else /* no ut_type */
		ut.ut_time = oldTime.seconds;
#endif /* OLD_TIME */

/* ======== BEGIN  OLD TIME EVENT - UTMP / WTMP =========== */
#ifdef HAVE_UTMPNAME
		utmpname(UTMP_FILE);
#endif /* HAVE_UTMPNAME */
#ifdef HAVE_SETUTENT
		setutent();
#endif /* HAVE_SETUTENT */
#ifdef HAVE_PUTUTLINE
		pututline(&ut);
#endif /* HAVE_PUTUTLINE */
#ifdef HAVE_ENDUTENT
		endutent();
#endif /* HAVE_ENDUTENT */
#ifdef HAVE_UTMPNAME
		utmpname(WTMP_FILE);
#endif /* HAVE_UTMPNAME */
#ifdef HAVE_SETUTENT
		setutent();
#endif /* HAVE_SETUTENT */
#ifdef HAVE_PUTUTLINE
		pututline(&ut);
#endif /* HAVE_PUTUTLINE */
#ifdef HAVE_ENDUTENT
		endutent();
#endif /* HAVE_ENDUTENT */
/* ======== END    OLD TIME EVENT - UTMP / WTMP =========== */

#endif /* HAVE_UTMP_H */
#endif /* HAVE_UTMPX_H */

/* Add the new time entry to utmp/wtmp */

#ifdef HAVE_UTMPX_H
		memset(&utx, 0, sizeof(utx));
		strncpy(utx.ut_user, "date", sizeof(utx.ut_user));
#ifndef NTIME_MSG
		strncpy(utx.ut_line, "{", sizeof(utx.ut_line));
#else
		strncpy(utx.ut_line, NTIME_MSG, sizeof(utx.ut_line));
#endif /* NTIME_MSG */
#ifdef NEW_TIME
		utx.ut_tv.tv_sec = newTime.seconds;
		utx.ut_tv.tv_usec = newTime.nanoseconds / 1000;
		utx.ut_type = NEW_TIME;
#else /* no ut_type */
		utx.ut_time = newTime.seconds;
#endif /* NEW_TIME */

/* ======== BEGIN  NEW TIME EVENT - UTMPX / WTMPX =========== */
#ifdef HAVE_UTMPXNAME
		utmpxname("/var/log/utmp");
#endif /* HAVE_UTMPXNAME */
		setutxent();
		pututxline(&utx);
		endutxent();
#ifdef HAVE_UPDWTMPX
		updwtmpx("/var/log/wtmp", &utx);
#endif /* HAVE_UPDWTMPX */
/* ======== END    NEW TIME EVENT - UTMPX / WTMPX =========== */

#else /* NO UTMPX_H */

#ifdef HAVE_UTMP_H
		memset(&ut, 0, sizeof(ut));
		strncpy(ut.ut_name, "date", sizeof(ut.ut_name));
#ifndef NTIME_MSG
		strncpy(ut.ut_line, "{", sizeof(ut.ut_line));
#else
		strncpy(ut.ut_line, NTIME_MSG, sizeof(ut.ut_line));
#endif /* NTIME_MSG */
#ifdef NEW_TIME
		ut.ut_tv.tv_sec = newTime.seconds;
		ut.ut_tv.tv_usec = newTime.nanoseconds / 1000;
		ut.ut_type = NEW_TIME;
#else /* no ut_type */
		ut.ut_time = newTime.seconds;
#endif /* NEW_TIME */

/* ======== BEGIN  NEW TIME EVENT - UTMP / WTMP =========== */
#ifdef HAVE_UTMPNAME
		utmpname(UTMP_FILE);
#endif /* HAVE_UTMPNAME */
#ifdef HAVE_SETUTENT
		setutent();
#endif /* HAVE_SETUTENT */
#ifdef HAVE_PUTUTLINE
		pututline(&ut);
#endif /* HAVE_PUTUTLINE */
#ifdef HAVE_ENDUTENT
		endutent();
#endif /* HAVE_ENDUTENT */
#ifdef HAVE_UTMPNAME
		utmpname(WTMP_FILE);
#endif /* HAVE_UTMPNAME */
#ifdef HAVE_SETUTENT
		setutent();
#endif /* HAVE_SETUTENT */
#ifdef HAVE_PUTUTLINE
		pututline(&ut);
#endif /* HAVE_PUTUTLINE */
#ifdef HAVE_ENDUTENT
		endutent();
#endif /* HAVE_ENDUTENT */
/* ======== END    NEW TIME EVENT - UTMP / WTMP =========== */

#endif /* HAVE_UTMP_H */
#endif /* HAVE_UTMPX_H */

}
