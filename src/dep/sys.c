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

#ifdef HAVE_LINUX_IF_H
#include <linux/if.h>		/* struct ifaddr, ifreq, ifconf, ifmap, IF_NAMESIZE etc. */
#elif defined(HAVE_NET_IF_H)
#include <net/if.h>		/* struct ifaddr, ifreq, ifconf, ifmap, IF_NAMESIZE etc. */
#endif /* HAVE_LINUX_IF_H*/

#ifdef HAVE_NET_IF_ETHER_H
#  include <net/if_ether.h>
#endif

#ifdef HAVE_SYS_TIMEX_H
#include <sys/timex.h>
#endif

/* only C99 has the round function built-in */
double round (double __x);

static int closeLog(LogFileHandler* handler);

int
snprint_TimeInternal(char *s, int max_len, const TimeInternal * p)
{
	int len = 0;

	/* always print either a space, or the leading "-". This makes the stat files columns-aligned */
	len += snprintf(&s[len], max_len - len, "%c",
		isTimeNegative(p)? '-':' ');

	len += snprintf(&s[len], max_len - len, "%d.%09d",
	    abs(p->seconds), abs(p->nanoseconds));

	return len;
}

char *
translatePortState(PtpClock *ptpClock)
{
	char *s;
	switch(ptpClock->portDS.portState) {
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
	    default:                s = "strt";     break;
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

/* Write a formatted string to file pointer, with optional deduplication and filtering */
int writeMessage(FILE* destination, uint32_t *lastHash, int priority, const char * format, va_list ap) {


	extern GlobalConfig global;
	extern Boolean startupInProgress;

	char time_str[MAXTIMESTR];
	struct timeval now;
	char buf[PATH_MAX +1];
	uint32_t hash;

	bool debug = (priority >= LOG_DEBUG);

	char *filter =  debug ? global.debugFilter : global.logFilter;

	if(destination == NULL)
		return -1;

	/* If we're starting up as daemon, only print <= WARN */
	if ((destination == stderr) &&
		!global.nonDaemon && startupInProgress &&
		(priority > LOG_WARNING)){
		    return 1;
		}

	memset(buf, 0, sizeof(buf));
	vsnprintf(buf, PATH_MAX, format, ap);

	/* deduplication */
	if(!debug && global.deduplicateLog) {
	    /* check if this message produces the same hash as last */
	    hash = fnvHash(buf, sizeof(buf), 0);
	    if(lastHash != NULL) {
		if(format[0] != '\n' && lastHash != NULL) {
		    /* last message was the same - don't print the next one */
		    if( (lastHash != 0) && (hash == *lastHash)) {
			return 0;
		    }
		}
		*lastHash = hash;
	    }
	}

	/* filter the log output */
	if(strlen(filter) && !strstr(buf, filter)) {
	    return 0;
	}

	/* Print timestamps and prefixes only if we're running in foreground or logging to file*/
	if( global.nonDaemon || destination != stderr) {

		/*
		 * select debug tagged with timestamps. This will slow down PTP itself if you send a lot of messages!
		 * it also can cause problems in nested debug statements (which are solved by turning the signal
		 *  handling synchronous, and not calling this function inside asycnhronous signal processing)
		 */
		gettimeofday(&now, 0);
		strftime(time_str, MAXTIMESTR, "%F %X", localtime((time_t*)&now.tv_sec));
		fprintf(destination, "%s.%06d ", time_str, (int)now.tv_usec  );
		fprintf(destination,PTPD_PROGNAME"[%d].(%-9s ",
		(int)getpid(),
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

	}

	return fprintf(destination, "%s", buf);

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
 * Prints a message, ranging from critical to debug.
 * This either prints the message to syslog, or with timestamp+state to stderr
 */
void
logMessageWrapper(int priority, const char * format, va_list ap)
{
	extern GlobalConfig global;
	extern Boolean startupInProgress;

	va_list ap1;
	va_copy(ap1, ap);

#ifdef RUNTIME_DEBUG
	if ((priority >= LOG_DEBUG) && (priority > global.debug_level)) {
		goto end;
	}
#endif

	/* log level filter */
	if(priority > global.logLevel) {
	    goto end;
	}
	/* If we're using a log file and the message has been written OK, we're done*/
	if(global.eventLog.logEnabled && global.eventLog.logFP != NULL) {
	    if(writeMessage(global.eventLog.logFP, &global.eventLog.lastHash, priority, format, ap) > 0) {
		maintainLogSize(&global.eventLog);
		if(!startupInProgress)
		    goto end;
		else {
		    global.eventLog.lastHash = 0;
		    goto std_err;
		    }
	    }
	}

	/*
	 * Otherwise we try syslog - if we're here, we didn't write to log file.
	 * If we're running in background and we're starting up, also log first
	 * messages to syslog to at least leave a trace.
	 */
	if (global.useSysLog ||
	    (!global.nonDaemon && startupInProgress)) {
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
		if (!startupInProgress) {
			goto end;
		}
		else {
			global.eventLog.lastHash = 0;
			goto std_err;
		}
	}
std_err:

	/* Either all else failed or we're running in foreground - or we also log to stderr */
	writeMessage(stderr, &global.eventLog.lastHash, priority, format, ap1);

end:
	va_end(ap1);
	va_end(ap);
	return;
}

void logMessage(int priority, const char * format, ...) {

    va_list ap;

    va_start (ap, format);

    logMessageWrapper(priority, format, ap);

    va_end(ap);

}

/* Restart a file log target based on its settings  */
int
restartLog(LogFileHandler* handler, Boolean quiet)
{

        /* The FP is open - close it */
        if(handler->logFP != NULL) {
		handler->lastHash=0;
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
restartLogging(GlobalConfig* global)
{

	if(!restartLog(&global->statisticsLog, TRUE))
		NOTIFY("Failed logging to %s file\n", global->statisticsLog.logID);

	if(!restartLog(&global->recordLog, TRUE))
		NOTIFY("Failed logging to %s file\n", global->recordLog.logID);

	if(!restartLog(&global->eventLog, TRUE))
		NOTIFY("Failed logging to %s file\n", global->eventLog.logID);

	if(!restartLog(&global->statusLog, TRUE))
		NOTIFY("Failed logging to %s file\n", global->statusLog.logID);

}

void
stopLogging(GlobalConfig* global)
{
	closeLog(&global->statisticsLog);
	closeLog(&global->recordLog);
	closeLog(&global->eventLog);
	closeLog(&global->statusLog);
}

void
logStatistics(PtpClock * ptpClock)
{

	ClockDriver *cd = ptpClock->clockDriver;
	extern GlobalConfig global;
	static int errorMsg = 0;
	static char sbuf[SCREEN_BUFSZ * 2];
	int len = 0;
	TimeInternal now;
	time_t time_s;
	FILE* destination;
	static TimeInternal prev_now_sync, prev_now_delay;
	char time_str[MAXTIMESTR];

	if (!global.logStatistics) {
		return;
	}

	if(global.statisticsLog.logEnabled && global.statisticsLog.logFP != NULL)
	    destination = global.statisticsLog.logFP;
	else
	    destination = stdout;

	if (ptpClock->resetStatisticsLog) {
		ptpClock->resetStatisticsLog = FALSE;
		fprintf(destination,"# %s, State, Clock ID, One Way Delay, "
		       "Offset From Master, Slave to Master, "
		       "Master to Slave, Observed Drift, Last packet Received, Sequence ID"
			", One Way Delay Mean, One Way Delay Std Dev, Offset From Master Mean, Offset From Master Std Dev, Observed Drift Mean, Observed Drift Std Dev, raw delayMS, raw delaySM"
			"\n", (global.statisticsTimestamp == TIMESTAMP_BOTH) ? "Timestamp, Unix timestamp" : "Timestamp");
	}

	memset(sbuf, 0, sizeof(sbuf));

	getSystemTime(&now);

	/*
	 * print one log entry per X seconds for Sync and DelayResp messages, to reduce disk usage.
	 */

	if ((ptpClock->portDS.portState == PTP_SLAVE) && (global.statisticsLogInterval)) {
			
		switch(ptpClock->char_last_msg) {
			case 'S':
			if((now.seconds - prev_now_sync.seconds) < global.statisticsLogInterval){
				DBGV("Suppressed Sync statistics log entry - statisticsLogInterval configured\n");
				return;
			}
			prev_now_sync = now;
			    break;
			case 'D':
			case 'P':
			if((now.seconds - prev_now_delay.seconds) < global.statisticsLogInterval){
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
	if (global.statisticsTimestamp == TIMESTAMP_DATETIME ||
	    global.statisticsTimestamp == TIMESTAMP_BOTH) {
	    strftime(time_str, MAXTIMESTR, "%Y-%m-%d %X", localtime(&time_s));
	    len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s.%06d, %s, ",
		       time_str, (int)now.nanoseconds/1000, /* Timestamp */
		       translatePortState(ptpClock)); /* State */
	}

	/* output unix timestamp s.ns if configured */
	if (global.statisticsTimestamp == TIMESTAMP_UNIX ||
	    global.statisticsTimestamp == TIMESTAMP_BOTH) {
	    len += snprintf(sbuf + len, sizeof(sbuf) - len, "%d.%06d, %s,",
		       now.seconds, now.nanoseconds, /* Timestamp */
		       translatePortState(ptpClock)); /* State */
	}

	if (ptpClock->portDS.portState == PTP_SLAVE) {
		len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
			 &ptpClock->parentDS.parentPortIdentity); /* Clock ID */

		/*
		 * if grandmaster ID differs from parent port ID then
		 * also print GM ID
		 */
		if (memcmp(ptpClock->parentDS.grandmasterIdentity,
			   ptpClock->parentDS.parentPortIdentity.clockIdentity,
			   CLOCK_IDENTITY_LENGTH)) {
			len += snprint_ClockIdentity(sbuf + len,
						     sizeof(sbuf) - len,
						     ptpClock->parentDS.grandmasterIdentity);
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		if(global.delayMechanism == E2E) {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
						    &ptpClock->currentDS.meanPathDelay);
		} else {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
						    &ptpClock->portDS.peerMeanPathDelay);
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
		    &ptpClock->currentDS.offsetFromMaster);

		/* print MS and SM with sign */
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");
			
		if(global.delayMechanism == E2E) {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->delaySM));
		} else {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->pdelaySM));
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
				&(ptpClock->delayMS));

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.09f, %c, %05d",
			       cd->servo.integral,
			       ptpClock->char_last_msg,
			       ptpClock->msgTmpHeader.sequenceId);

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.09f, %.00f, %.09f, %.00f",
			       ptpClock->slaveStats.mpdMean,
			       ptpClock->slaveStats.mpdStdDev * 1E9,
			       ptpClock->slaveStats.ofmMean,
			       ptpClock->slaveStats.ofmStdDev * 1E9);
/*
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.0f, %.0f, ",
			       ptpClock->servo.driftMean,
			       ptpClock->servo.driftStdDev);
*/
		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->rawDelayMS));

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->rawDelaySM));

	} else {
		if ((ptpClock->portDS.portState == PTP_MASTER) || (ptpClock->portDS.portState == PTP_PASSIVE)) {

			len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
				 &ptpClock->parentDS.parentPortIdentity);
							
			//len += snprintf(sbuf + len, sizeof(sbuf) - len, ")");
		}

		/* show the current reset number on the log */
		if (ptpClock->portDS.portState == PTP_LISTENING) {
			len += snprintf(sbuf + len,
						     sizeof(sbuf) - len,
						     " %d ", ptpClock->resetCount);
		}
	}
	
	/* add final \n in normal status lines */
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");

	/* fprintf may get interrupted by a signal - silently retry once */
	if (fprintf(destination, "%s", sbuf) < len) {
	    if (fprintf(destination, "%s", sbuf) < len) {
		if(!errorMsg) {
		    PERROR("Error while writing statistics");
		}
		errorMsg = TRUE;
	    }
	}

	if(destination == global.statisticsLog.logFP) {
		if (maintainLogSize(&global.statisticsLog))
			ptpClock->resetStatisticsLog = TRUE;
	}

}

/* periodic status update */
void
periodicUpdate(const GlobalConfig *global, PtpClock *ptpClock)
{

    char tmpBuf[200];
    char masterIdBuf[150];
    int len = 0;

    memset(tmpBuf, 0, sizeof(tmpBuf));
    memset(masterIdBuf, 0, sizeof(masterIdBuf));

    TimeInternal *mpd = &ptpClock->currentDS.meanPathDelay;

    len += snprint_PortIdentity(masterIdBuf + len, sizeof(masterIdBuf) - len,
	    &ptpClock->parentDS.parentPortIdentity);
    if(ptpClock->bestMaster && ptpClock->bestMaster->protocolAddress) {
	tmpstr(tmpAddr, ptpAddrStrLen(ptpClock->bestMaster->protocolAddress));
	ptpAddrToString(tmpAddr, tmpAddr_len, ptpClock->bestMaster->protocolAddress);
	len += snprintf(masterIdBuf + len, sizeof(masterIdBuf) - len, " (%s)", tmpAddr);
    }

    if(ptpClock->portDS.delayMechanism == P2P) {
	mpd = &ptpClock->portDS.peerMeanPathDelay;
    }

    if(ptpClock->portDS.portState == PTP_SLAVE) {
	snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), &ptpClock->currentDS.offsetFromMaster);

	if(ptpClock->slaveStats.statsCalculated) {
	    INFO("Status update: state %s, best master %s, ofm %s s, ofm_mean % .09f s, ofm_dev % .09f s\n",
		portState_getName(ptpClock->portDS.portState),
		masterIdBuf,
		tmpBuf,
		ptpClock->slaveStats.ofmMean,
		ptpClock->slaveStats.ofmStdDev);
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), mpd);
	    if (ptpClock->portDS.delayMechanism == E2E) {
		INFO("Status update: state %s, best master %s, mpd %s s, mpd_mean % .09f s, mpd_dev % .09f s\n",
		    portState_getName(ptpClock->portDS.portState),
		    masterIdBuf,
		    tmpBuf,
		    ptpClock->slaveStats.mpdMean,
		    ptpClock->slaveStats.mpdStdDev);
	    } else if(ptpClock->portDS.delayMechanism == P2P) {
		INFO("Status update: state %s, best master %s, mpd %s s\n", portState_getName(ptpClock->portDS.portState), masterIdBuf, tmpBuf);
	    }
	} else {
	    INFO("Status update: state %s, best master %s, ofm %s s\n", portState_getName(ptpClock->portDS.portState), masterIdBuf, tmpBuf);
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), mpd);
	    if(ptpClock->portDS.delayMechanism != DELAY_DISABLED) {
		INFO("Status update: state %s, best master %s, mpd %s s\n", portState_getName(ptpClock->portDS.portState), masterIdBuf, tmpBuf);
	    }
	}
    } else if(ptpClock->portDS.portState == PTP_MASTER) {
	if(global->unicastNegotiation || ptpClock->unicastDestinationCount) {
	    INFO("Status update: state %s, %d slaves\n", portState_getName(ptpClock->portDS.portState),
	    ptpClock->unicastDestinationCount + ptpClock->slaveCount);
	} else {
	    INFO("Status update: state %s\n", portState_getName(ptpClock->portDS.portState));
	}
    } else {
	INFO("Status update: state %s\n", portState_getName(ptpClock->portDS.portState));
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
			portState_getName(ptpClock->portDS.portState));

	if (ptpClock->portDS.portState >= PTP_MASTER) {
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", Best master: ");
		len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
			&ptpClock->parentDS.parentPortIdentity);
		if(ptpClock->bestMaster && ptpClock->bestMaster->protocolAddress) {
		    tmpstr(tmpAddr, ptpAddrStrLen(ptpClock->bestMaster->protocolAddress));
		    ptpAddrToString(tmpAddr, tmpAddr_len, ptpClock->bestMaster->protocolAddress);
		    len += snprintf(sbuf + len, sizeof(sbuf) - len, " (%s)", tmpAddr);
		}
        }
	if(ptpClock->portDS.portState == PTP_MASTER)
    		len += snprintf(sbuf + len, sizeof(sbuf) - len, " (self)");
        len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");
        NOTICE("%s",sbuf);
}

#define STATUSPREFIX "%-19s:"
void
writeStatusFile(PtpClock *ptpClock,const GlobalConfig *global, Boolean quiet)
{
	ClockDriver *cd = ptpClock->clockDriver;

	char outBuf[3072];
	char tmpBuf[200];

	int n = getAlarmSummary(NULL, 0, ptpClock->alarms, ALRM_MAX);
	char alarmBuf[n];

	getAlarmSummary(alarmBuf, n, ptpClock->alarms, ALRM_MAX);

	if(global->statusLog.logFP == NULL)
	    return;
	
	char timeStr[MAXTIMESTR];
	char hostName[MAXHOSTNAMELEN];

	struct timeval now;
	memset(hostName, 0, MAXHOSTNAMELEN);

	TTransport *transport = ptpClock->eventTransport;

	gethostname(hostName, MAXHOSTNAMELEN);
	gettimeofday(&now, 0);
	strftime(timeStr, MAXTIMESTR, "%a %b %d %X %Z %Y", localtime((time_t*)&now.tv_sec));
	
	FILE* out = global->statusLog.logFP;
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

	if(transport) {
	    tmpstr(trStatus, 100);
	    fprintf(out, 		STATUSPREFIX"  %s\n","Interface", transport->getStatusLine(transport, trStatus, trStatus_len));
	    fprintf(out, 		STATUSPREFIX"  %s","Transport", transport->getInfoLine(transport, trStatus, trStatus_len));
	    fprintf(out,"%s", global->unicastNegotiation ? " negotiation":"");
	    fprintf(out,"\n");
	}

	fprintf(out, 		STATUSPREFIX"  %s\n","PTP preset", dictionary_get(global->currentConfig, "ptpengine:preset", ""));

	fprintf(out, 		STATUSPREFIX"  %s\n","Delay mechanism", dictionary_get(global->currentConfig, "ptpengine:delay_mechanism", ""));
	if(ptpClock->portDS.portState >= PTP_MASTER) {
	fprintf(out, 		STATUSPREFIX"  %s\n","Sync mode", ptpClock->defaultDS.twoStepFlag ? "TWO_STEP" : "ONE_STEP");
	}
	if(ptpClock->defaultDS.slaveOnly && global->anyDomain) {
		fprintf(out, 		STATUSPREFIX"  %d, preferred %d\n","PTP domain",
		ptpClock->defaultDS.domainNumber, global->domainNumber);
	} else if(ptpClock->defaultDS.slaveOnly && global->unicastNegotiation) {
		fprintf(out, 		STATUSPREFIX"  %d, default %d\n","PTP domain", ptpClock->defaultDS.domainNumber, global->domainNumber);
	} else {
		fprintf(out, 		STATUSPREFIX"  %d\n","PTP domain", ptpClock->defaultDS.domainNumber);
	}
	fprintf(out, 		STATUSPREFIX"  %s\n","Port state", portState_getName(ptpClock->portDS.portState));
	if(strlen(alarmBuf) > 0) {
	    fprintf(out, 		STATUSPREFIX"  %s\n","Alarms", alarmBuf);
	}
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_PortIdentity(tmpBuf, sizeof(tmpBuf),
	    &ptpClock->portDS.portIdentity);
	fprintf(out, 		STATUSPREFIX"  %s\n","Local port ID", tmpBuf);


	if(ptpClock->portDS.portState >= PTP_MASTER) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_PortIdentity(tmpBuf, sizeof(tmpBuf),
	    &ptpClock->parentDS.parentPortIdentity);
	fprintf(out, 		STATUSPREFIX"  %s","Best master ID", tmpBuf);
	if(ptpClock->portDS.portState == PTP_MASTER)
	    fprintf(out," (self)");
	    fprintf(out,"\n");
	}

	if(ptpClock->portDS.portState > PTP_MASTER &&
	ptpClock->bestMaster && !ptpAddrIsEmpty(ptpClock->bestMaster->protocolAddress)) {
		tmpstr(tmpAddr, ptpAddrStrLen(ptpClock->bestMaster->protocolAddress));
		fprintf(out, 		STATUSPREFIX"  %s\n","Best master addr",
		ptpAddrToString(tmpAddr, tmpAddr_len, ptpClock->bestMaster->protocolAddress));
	}

	if(ptpClock->portDS.portState == PTP_SLAVE) {
	fprintf(out, 		STATUSPREFIX"  Priority1 %d, Priority2 %d, clockClass %d","GM priority",
	ptpClock->parentDS.grandmasterPriority1, ptpClock->parentDS.grandmasterPriority2, ptpClock->parentDS.grandmasterClockQuality.clockClass);
	if(global->unicastNegotiation && ptpClock->parentGrants != NULL ) {
	    	fprintf(out, ", localPref %d", ptpClock->parentGrants->localPreference);
	}
	fprintf(out, "%s\n", (ptpClock->bestMaster != NULL && ptpClock->bestMaster->disqualified) ? " (timeout)" : "");
	}

	if(ptpClock->defaultDS.clockQuality.clockClass < 128 ||
		ptpClock->portDS.portState == PTP_SLAVE ||
		ptpClock->portDS.portState == PTP_PASSIVE){
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
	if (ptpClock->portDS.portState == PTP_SLAVE) {	
	    fprintf(out, "%s", global->preferUtcValid ? ", prefer UTC" : "");
	    fprintf(out, "%s", global->requireUtcValid ? ", require UTC" : "");
	}
	fprintf(out,"\n");
	}

	if(ptpClock->portDS.portState == PTP_SLAVE) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->currentDS.offsetFromMaster);
	fprintf(out, 		STATUSPREFIX" %s s","Offset from Master", tmpBuf);
	if(ptpClock->slaveStats.statsCalculated)
	fprintf(out, ", mean % .09f s, dev % .09f s",
		ptpClock->slaveStats.ofmMean,
		ptpClock->slaveStats.ofmStdDev
	);
	    fprintf(out,"\n");

	if(ptpClock->portDS.delayMechanism == E2E) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->currentDS.meanPathDelay);
	fprintf(out, 		STATUSPREFIX" %s s","Mean Path Delay", tmpBuf);

	if(ptpClock->slaveStats.statsCalculated)
	fprintf(out, ", mean % .09f s, dev % .09f s",
		ptpClock->slaveStats.mpdMean,
		ptpClock->slaveStats.mpdStdDev
	);
	fprintf(out,"\n");
	}
	if(ptpClock->portDS.delayMechanism == P2P) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->portDS.peerMeanPathDelay);
	fprintf(out, 		STATUSPREFIX" %s s","Mean Path (p)Delay", tmpBuf);
	fprintf(out,"\n");
	}

	fprintf(out, 		STATUSPREFIX"  ","PTP Clock status");
	    if(cd->state == CS_STEP) {
		fprintf(out,"panic mode,");
	    }
	    if(cd->state == CS_NEGSTEP) {
		fprintf(out,"negative step,");
	    }

	if(global->calibrationDelay) {
	    fprintf(out, "%s",
		ptpClock->isCalibrated ? "calibrated" : "not calibrated");
	}

	if(global->noAdjust) {
	    fprintf(out, ", read-only");
	} else {
		fprintf(out, ", %s",
		    (cd->state == CS_LOCKED) ? "stabilised" : "not stabilised");
	}
	fprintf(out,"\n");

	}


	if(ptpClock->portDS.portState == PTP_MASTER || ptpClock->portDS.portState == PTP_PASSIVE) {

	fprintf(out, 		STATUSPREFIX"  %d","Priority1 ", ptpClock->defaultDS.priority1);
	if(ptpClock->portDS.portState == PTP_PASSIVE)
		fprintf(out, " (best master: %d)", ptpClock->parentDS.grandmasterPriority1);
	fprintf(out,"\n");
	fprintf(out, 		STATUSPREFIX"  %d","Priority2 ", ptpClock->defaultDS.priority2);
	if(ptpClock->portDS.portState == PTP_PASSIVE)
		fprintf(out, " (best master: %d)", ptpClock->parentDS.grandmasterPriority2);
	fprintf(out,"\n");
	fprintf(out, 		STATUSPREFIX"  %d","ClockClass ", ptpClock->defaultDS.clockQuality.clockClass);
	if(ptpClock->portDS.portState == PTP_PASSIVE)
		fprintf(out, " (best master: %d)", ptpClock->parentDS.grandmasterClockQuality.clockClass);
	fprintf(out,"\n");
	if(ptpClock->portDS.delayMechanism == P2P) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->portDS.peerMeanPathDelay);
	fprintf(out, 		STATUSPREFIX" %s s","Mean Path (p)Delay", tmpBuf);
	fprintf(out,"\n");
	}

	}

	if(ptpClock->portDS.portState == PTP_MASTER || ptpClock->portDS.portState == PTP_PASSIVE ||
	    ptpClock->portDS.portState == PTP_SLAVE) {

	fprintf(out,		STATUSPREFIX"  ","Message rates");

	if (ptpClock->portDS.logSyncInterval == UNICAST_MESSAGEINTERVAL)
	    fprintf(out,"[UC-unknown]");
	else if (ptpClock->portDS.logSyncInterval <= 0)
	    fprintf(out,"%.0f/s",pow(2,-ptpClock->portDS.logSyncInterval));
	else
	    fprintf(out,"1/%.0fs",pow(2,ptpClock->portDS.logSyncInterval));
	fprintf(out, " sync");


	if(ptpClock->portDS.delayMechanism == E2E) {
		if (ptpClock->portDS.logMinDelayReqInterval == UNICAST_MESSAGEINTERVAL)
		    fprintf(out,", [UC-unknown]");
		else if (ptpClock->portDS.logMinDelayReqInterval <= 0)
		    fprintf(out,", %.0f/s",pow(2,-ptpClock->portDS.logMinDelayReqInterval));
		else
		    fprintf(out,", 1/%.0fs",pow(2,ptpClock->portDS.logMinDelayReqInterval));
		fprintf(out, " delay");
	}

	if(ptpClock->portDS.delayMechanism == P2P) {
		if (ptpClock->portDS.logMinPdelayReqInterval == UNICAST_MESSAGEINTERVAL)
		    fprintf(out,", [UC-unknown]");
		else if (ptpClock->portDS.logMinPdelayReqInterval <= 0)
		    fprintf(out,", %.0f/s",pow(2,-ptpClock->portDS.logMinPdelayReqInterval));
		else
		    fprintf(out,", 1/%.0fs",pow(2,ptpClock->portDS.logMinPdelayReqInterval));
		fprintf(out, " pdelay");
	}

	if (ptpClock->portDS.logAnnounceInterval == UNICAST_MESSAGEINTERVAL)
	    fprintf(out,", [UC-unknown]");
	else if (ptpClock->portDS.logAnnounceInterval <= 0)
	    fprintf(out,", %.0f/s",pow(2,-ptpClock->portDS.logAnnounceInterval));
	else
	    fprintf(out,", 1/%.0fs",pow(2,ptpClock->portDS.logAnnounceInterval));
	fprintf(out, " announce");

	    fprintf(out,"\n");

	}

	fprintf(out, 		STATUSPREFIX"  ","Performance");
	fprintf(out,"Message RX %d/s %d Bps, TX %d/s %d Bps",
		ptpClock->counters.messageReceiveRate,
		ptpClock->counters.bytesReceiveRate,
		ptpClock->counters.messageSendRate,
		ptpClock->counters.bytesSendRate);
	if(ptpClock->portDS.portState == PTP_MASTER) {
		if(global->unicastNegotiation) {
		    fprintf(out,", slaves %d", ptpClock->slaveCount);
		} else if (global->transportMode == TMODE_UC) {
		    fprintf(out,", slaves %d", ptpClock->unicastDestinationCount);
		}
	}

	fprintf(out,"\n");

	if ( ptpClock->portDS.portState == PTP_SLAVE ||
	    ptpClock->defaultDS.clockQuality.clockClass == 255 ) {

	fprintf(out, 		STATUSPREFIX"  %lu\n","Announce received",
	    (unsigned long)ptpClock->counters.announceMessagesReceived);
	fprintf(out, 		STATUSPREFIX"  %lu\n","Sync received",
	    (unsigned long)ptpClock->counters.syncMessagesReceived);
	if(ptpClock->defaultDS.twoStepFlag)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Follow-up received",
	    (unsigned long)ptpClock->counters.followUpMessagesReceived);
	if(ptpClock->portDS.delayMechanism == E2E) {
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayReq sent",
		    (unsigned long)ptpClock->counters.delayReqMessagesSent);
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayResp received",
		    (unsigned long)ptpClock->counters.delayRespMessagesReceived);
	}
	}

	if( ptpClock->portDS.portState == PTP_MASTER ||
	    ptpClock->defaultDS.clockQuality.clockClass < 128 ) {
	fprintf(out, 		STATUSPREFIX"  %lu received, %lu sent \n","Announce",
	    (unsigned long)ptpClock->counters.announceMessagesReceived,
	    (unsigned long)ptpClock->counters.announceMessagesSent);
	fprintf(out, 		STATUSPREFIX"  %lu\n","Sync sent",
	    (unsigned long)ptpClock->counters.syncMessagesSent);
	if(ptpClock->defaultDS.twoStepFlag)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Follow-up sent",
	    (unsigned long)ptpClock->counters.followUpMessagesSent);

	if(ptpClock->portDS.delayMechanism == E2E) {
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayReq received",
		    (unsigned long)ptpClock->counters.delayReqMessagesReceived);
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayResp sent",
		    (unsigned long)ptpClock->counters.delayRespMessagesSent);
	}

	}

	if(ptpClock->portDS.delayMechanism == P2P) {

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
	fprintf(out,"                                   \n");
	if(cd) {
	    char buf[100];
	    int i = 1;
	    for(ClockDriver *ccd = *cd->_first; ccd != NULL; ccd=ccd->_next) {
		    ccd->putInfoLine(ccd, buf, 100);
		    fprintf(out, "Clock %d: %-9s : %s\n", i++, ccd->name, buf);
	    }
	    i = 1;
	    for(ClockDriver *ccd = *cd->_first; ccd != NULL; ccd=ccd->_next) {
		    ccd->putStatsLine(ccd, buf, 100);
		    fprintf(out, "Clock %d: %-9s : %s\n",i++, ccd->name, buf);
	    }

	}

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
	extern GlobalConfig global;
	if (global.recordLog.logEnabled && global.recordLog.logFP != NULL) {
		fprintf(global.recordLog.logFP, "%d %llu\n", sequenceId,
		  ((time->seconds * 1000000000ULL) + time->nanoseconds)
		);
		maintainLogSize(&global.recordLog);
	}
}

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
checkOtherLocks(GlobalConfig* global)
{


char searchPattern[PATH_MAX];
char * lockPath = 0;
int lockPid = 0;
glob_t matchedFiles;
Boolean ret = TRUE;
int matches = 0, counter = 0;

	/* no need to check locks */
	if(global->ignoreLock ||
		!global->autoLockFile)
			return TRUE;

    /*
     * Try to discover if we can run in desired mode
     * based on the presence of other lock files
     * and them being lockable
     */

	/* Check for other ptpd running on the same interface - same for all modes */
	snprintf(searchPattern, PATH_MAX,"%s/%s_*_%s.lock",
	    global->lockDirectory, PTPD_PROGNAME,global->ifName);

	DBGV("SearchPattern: %s\n",searchPattern);
	switch(glob(searchPattern, 0, NULL, &matchedFiles)) {

	    case GLOB_NOSPACE:
	    case GLOB_ABORTED:
		    PERROR("Could not scan %s directory\n", global->lockDirectory);;
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
		    global->ifName, lockPath);
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
		    global->ifName, lockPath, lockPid);
		    ret = FALSE;
		    goto end;
	    }
	}

	if(matches > 0)
		globfree(&matchedFiles);
	/* Any mode that can control the clock - also check the clock driver */
	if(global->clockQuality.clockClass > 127 ) {
	    snprintf(searchPattern, PATH_MAX,"%s/%s_%s_*.lock",
	    global->lockDirectory,PTPD_PROGNAME,DEFAULT_CLOCKDRIVER);
	DBGV("SearchPattern: %s\n",searchPattern);

	switch(glob(searchPattern, 0, NULL, &matchedFiles)) {

	    case GLOB_NOSPACE:
	    case GLOB_ABORTED:
		    PERROR("Could not scan %s directory\n", global->lockDirectory);;
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

/* First cut on informing the clock */
void
informClockSource(PtpClock* ptpClock)
{
	struct timex tmx;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_MAXERROR | MOD_ESTERROR;

	tmx.maxerror = (ptpClock->currentDS.offsetFromMaster.seconds * 1E9 +
			ptpClock->currentDS.offsetFromMaster.nanoseconds) / 1000;
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
	     "Setting TAI offset to %d\n", utc_offset);

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

#endif /* SYS_TIMEX_H */

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

    getSystemTime(&now);

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

int setCpuAffinity(int cpu) {

#ifdef __QNXNTO__
    unsigned    num_elements = 0;
    int         *rsizep, masksize_bytes, size;
    int    *rmaskp, *imaskp;
    void        *my_data;
    uint32_t cpun;
    num_elements = RMSK_SIZE(_syspage_ptr->num_cpu);

    masksize_bytes = num_elements * sizeof(unsigned);

    size = sizeof(int) + 2 * masksize_bytes;
    if ((my_data = malloc(size)) == NULL) {
        return -1;
    } else {
        memset(my_data, 0x00, size);

        rsizep = (int *)my_data;
        rmaskp = rsizep + 1;
        imaskp = rmaskp + num_elements;

        *rsizep = num_elements;

	if(cpu > _syspage_ptr->num_cpu) {
	    return -1;
	}

	if(cpu >= 0) {
	    cpun = (uint32_t)cpu;
	    RMSK_SET(cpun, rmaskp);
    	    RMSK_SET(cpun, imaskp);
	} else {
		for(cpun = 0;  cpun < num_elements; cpun++) {
		    RMSK_SET(cpun, rmaskp);
		    RMSK_SET(cpun, imaskp);
		}
	}
	int ret = ThreadCtl( _NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT, my_data);
	free(my_data);
	return ret;
    }

#endif

#ifdef HAVE_SYS_CPUSET_H
	cpuset_t mask;
    	CPU_ZERO(&mask);
	if(cpu >= 0) {
    	    CPU_SET(cpu,&mask);
	} else {
		int i;
		for(i = 0;  i < CPU_SETSIZE; i++) {
			CPU_SET(i, &mask);
		}
	}
    	return(cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID,
			      -1, sizeof(mask), &mask));
#endif /* HAVE_SYS_CPUSET_H */

#if defined(linux) && defined(HAVE_SCHED_H)
	cpu_set_t mask;
	CPU_ZERO(&mask);
	if(cpu >= 0) {
	    CPU_SET(cpu,&mask);
	} else {
		int i;
		for(i = 0;  i < CPU_SETSIZE; i++) {
			CPU_SET(i, &mask);
		}
	}
	return sched_setaffinity(0, sizeof(mask), &mask);
#endif /* linux && HAVE_SCHED_H */

return -1;

}

