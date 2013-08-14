/*-
 * Copyright (c) 2012-2013 Wojciech Owczarek,
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

#if defined(linux)
#  include <netinet/ether.h>
#elif defined( __FreeBSD__ )
#  include <net/ethernet.h>
#elif defined( __NetBSD__ )
#  include <net/if_ether.h>
#elif defined( __OpenBSD__ )
#  include <some ether header>  // force build error
#endif


/* only C99 has the round function built-in */
double round (double __x);


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

#if defined(linux) || defined(__NetBSD__)
	if (memcmp(addr->ether_addr_octet, &prev_addr, 
		  sizeof(struct ether_addr )) != 0) {
		valid = 0;
	}
#else // e.g. defined(__FreeBSD__)
	if (memcmp(addr->octet, &prev_addr, 
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
	strcpy(hostname, buf);
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
#if defined(linux) || defined(__NetBSD__)
			e.ether_addr_octet[j] = (uint8_t) id[i];
#else // e.g. defined(__FreeBSD__)
			e.octet[j] = (uint8_t) id[i];
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

	len += snprintf(&s[len], max_len - len, "/%02x", (unsigned) id->portNumber);
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
		strftime(time_str, MAXTIMESTR, "%F %X", localtime(&now.tv_sec));
		fprintf(destination, "%s.%06d ", time_str, (int)now.tv_usec  );
		fprintf(destination,PTPD_PROGNAME"[%d].%s (%-9s ",
		getpid(), startupInProgress ? "startup" : rtOpts.ifaceName,
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
	if (fstat(fileno(handler->logFP), &st)!=-1) {
		handler->fileSize = st.st_size;
		DBG("%s file size is %d\n", handler->logID, handler->fileSize);
	} else {
		DBG("fstat on %s file failed!\n", handler->logID);
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
		    goto stderr;
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
			goto stderr;
	}
stderr:
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


/* Return TRUE only if the log file had to be rotated / truncated - FALSE does not mean error */
/* Mini-logrotate: truncate file if exceeds preset size, also rotate up to n number of files if configured */
Boolean
maintainLogSize(LogFileHandler* handler)
{

	if(handler->maxSize) {
		if(handler->logFP == NULL)
		    return FALSE;
		updateLogSize(handler);
		DBG("%s logsize: %d\n", handler->logID, handler->fileSize);
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
				DBG("Could not truncate %s file\n", handler->logPath);
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
logStatistics(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	static char sbuf[SCREEN_BUFSZ];
	int len = 0;
	TimeInternal now;
	time_t time_s;
	FILE* destination;
	static TimeInternal prev_now_sync, prev_now_delay;
	char time_str[MAXTIMESTR];

	if (!rtOpts->logStatistics) {
		return;
	}

	if(rtOpts->statisticsLog.logEnabled && rtOpts->statisticsLog.logFP != NULL)
	    destination = rtOpts->statisticsLog.logFP;
	else
	    destination = stdout;

	if (ptpClock->resetStatisticsLog) {
		ptpClock->resetStatisticsLog = FALSE;
		fprintf(destination,"# Timestamp, State, Clock ID, One Way Delay, "
		       "Offset From Master, Slave to Master, "
		       "Master to Slave, Observed Drift, Last packet Received"
#ifdef PTPD_STATISTICS
			", One Way Delay Mean, One Way Delay Std Dev, Offset From Master Mean, Offset From Master Std Dev, Observed Drift Mean, Observed Drift Std Dev"
#endif
			"\n");
	}
	memset(sbuf, ' ', sizeof(sbuf));

	getTime(&now);

	/*
	 * print one log entry per X seconds for Sync and DelayResp messages, to reduce disk usage.
	 */

	if ((ptpClock->portState == PTP_SLAVE) && (rtOpts->statisticsLogInterval)) {
			
		switch(ptpClock->char_last_msg) {
			case 'S':
			if((now.seconds - prev_now_sync.seconds) < rtOpts->statisticsLogInterval){
				DBGV("Suppressed Sync statistics log entry - statisticsLogInterval configured\n");
				return;
			}
			prev_now_sync = now;
			    break;
			case 'D':
			if((now.seconds - prev_now_delay.seconds) < rtOpts->statisticsLogInterval){
				DBGV("Suppressed Sync statistics log entry - statisticsLogInterval configured\n");
				return;
			}
			prev_now_delay = now;
			default:
			    break;
		}
	}


	time_s = now.seconds;
	strftime(time_str, MAXTIMESTR, "%Y-%m-%d %X", localtime(&time_s));
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s.%06d, %s, ",
		       time_str, (int)now.nanoseconds/1000, /* Timestamp */
		       translatePortState(ptpClock)); /* State */

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

		if(rtOpts->delayMechanism == E2E) {
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
			
		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
				&(ptpClock->delaySM));

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
				&(ptpClock->delayMS));

#ifndef PTPD_DOUBLE_SERVO
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %d, %c",
			       ptpClock->servo.observedDrift,
			       ptpClock->char_last_msg);
#else
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.09f, %c",
			       ptpClock->servo.observedDrift,
			       ptpClock->char_last_msg);

#endif /* PTPD_DOUBLE_SERVO */

#ifdef PTPD_STATISTICS

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.09f, %.00f, %.09f, %.00f",
			       ptpClock->slaveStats.owdMean,
			       ptpClock->slaveStats.owdStdDev * 1E9,
			       ptpClock->slaveStats.ofmMean,
			       ptpClock->slaveStats.ofmStdDev * 1E9);

#ifdef PTPD_DOUBLE_SERVO
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.0f, %.0f",
#else
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %d, %d",
#endif /* PTPD_DOUBLE_SERVO */
			       ptpClock->servo.driftMean,
			       ptpClock->servo.driftStdDev
);



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
	if (rtOpts->nonDaemon) {
		/* in -C mode, adding an extra \n makes stats more clear intermixed with debug comments */
		len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");
	}
#endif

	if (fprintf(destination, "%s", sbuf) < len) {
		PERROR("Error while writing statistics");
	}

	if(destination == rtOpts->statisticsLog.logFP) {
		if (maintainLogSize(&rtOpts->statisticsLog))
			ptpClock->resetStatisticsLog = TRUE;
	}

}

void 
displayStatus(PtpClock *ptpClock, const char *prefixMessage)
{

	static char sbuf[SCREEN_BUFSZ];
	int len = 0;

	memset(sbuf, ' ', sizeof(sbuf));
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s", prefixMessage);
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s", 
			portState_getName(ptpClock->portState));

	if (ptpClock->portState >= PTP_MASTER) {
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", Best master: ");
		len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
			&ptpClock->parentPortIdentity);
        }
	if(ptpClock->portState == PTP_MASTER)
    		len += snprintf(sbuf + len, sizeof(sbuf) - len, " (self)");
        len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");
        NOTICE("%s",sbuf);
}

#define STATUSPREFIX "%-19s:"
void
writeStatusFile(PtpClock *ptpClock,RunTimeOpts *rtOpts, Boolean quiet)
{

	if(rtOpts->statusLog.logFP == NULL)
	    return;

	char outBuf[4096];
	char tmpBuf[200];
	FILE* out = rtOpts->statusLog.logFP;
	memset(outBuf, 0, sizeof(outBuf));

	setbuf(out, outBuf);
	if(ftruncate(fileno(out), 0) < 0) {
	    DBG("writeStatusFile: ftruncate() failed\n");
	}
	rewind(out);

	fprintf(out, 		STATUSPREFIX"  %s\n","Interface", dictionary_get(rtOpts->currentConfig, "ptpengine:interface", ""));
	fprintf(out, 		STATUSPREFIX"  %s\n","Preset", dictionary_get(rtOpts->currentConfig, "ptpengine:preset", ""));
	fprintf(out, 		STATUSPREFIX"  %s\n","Transport", dictionary_get(rtOpts->currentConfig, "ptpengine:transport", ""));
	if(rtOpts->transport != IEEE_802_3)
	fprintf(out, 		STATUSPREFIX"  %s\n","IP mode", dictionary_get(rtOpts->currentConfig, "ptpengine:ip_mode", ""));
	fprintf(out, 		STATUSPREFIX"  %s\n","Delay mechanism", dictionary_get(rtOpts->currentConfig, "ptpengine:delay_mechanism", ""));
	if(ptpClock->portState >= PTP_MASTER) {
	fprintf(out, 		STATUSPREFIX"  %s\n","Sync mode", ptpClock->twoStepFlag ? "TWO_STEP" : "ONE_STEP");
	}
	fprintf(out, 		STATUSPREFIX"  %d\n","PTP domain", rtOpts->domainNumber);
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

	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->meanPathDelay);
	fprintf(out, 		STATUSPREFIX" %s s","One-way delay", tmpBuf);
#ifdef PTPD_STATISTICS
	if(ptpClock->slaveStats.statsCalculated)
	fprintf(out, ", mean % .09f s, dev % .09f s",
		ptpClock->slaveStats.owdMean,
		ptpClock->slaveStats.owdStdDev
	);
#endif /* PTPD_STATISTICS */
	fprintf(out,"\n");
#ifndef PTPD_STATISTICS
if(rtOpts->enablePanicMode)
	fprintf(out, 		STATUSPREFIX"  ","Clock status");
		if(rtOpts->enablePanicMode) {
	    if(ptpClock->panicMode) {
		fprintf(out,"panic mode");
	    }
	}
#else
if(rtOpts->enablePanicMode || rtOpts->calibrationDelay || rtOpts->servoStabilityDetection) {
	fprintf(out, 		STATUSPREFIX"  ","Clock status");
		if(rtOpts->enablePanicMode) {
	    if(ptpClock->panicMode) {
		fprintf(out,"panic mode");
		if (rtOpts->servoStabilityDetection || rtOpts->calibrationDelay)
		fprintf(out, ", ");
	    }
	}
	if(rtOpts->calibrationDelay) {
	    fprintf(out, "%s",
		ptpClock->isCalibrated ? "calibrated" : "not calibrated");
	    if (rtOpts->servoStabilityDetection)
		fprintf(out, ", ");
	}
	if (rtOpts->servoStabilityDetection)
	    fprintf(out, "%s",
		ptpClock->servo.isStable ? "stabilised" : "not stabilised");
	fprintf(out,"\n");
}
#endif /* PTPD_STATISTICS */

#ifdef PTPD_NTPDC
	
	if(rtOpts->ntpOptions.enableEngine) {
		fprintf(out, 		STATUSPREFIX"  ","NTP control status ");

		    fprintf(out, "%s", (ptpClock->ntpControl.operational) ?
			    "operational" : "not operational");

		    fprintf(out, "%s", (rtOpts->ntpOptions.ntpInControl) ?
			    ", preferring NTP" : "");

		    fprintf(out, "%s", (ptpClock->ntpControl.inControl) ?
			    ", NTP in control" : ptpClock->portState == PTP_SLAVE ? ", PTP in control" : "");

		    fprintf(out, "%s", (ptpClock->ntpControl.isFailOver) ?
			    ", Failover requested" : "");

		    fprintf(out, "%s", (ptpClock->ntpControl.checkFailed) ?
			    ", Check failed" : "");

		    fprintf(out, "%s", (ptpClock->ntpControl.requestFailed) ?
			    ", Request failed" : "");

		    fprintf(out, "\n");

	}
#endif /* PTPD_NTPDC */

	fprintf(out, 		STATUSPREFIX" % .03f ppm","Drift correction",
			    ptpClock->servo.observedDrift / 1000);
if(ptpClock->servo.runningMaxOutput)
	fprintf(out, " (slewing at maximum rate)");
else {
#ifdef PTPD_STATISTICS
	if(ptpClock->slaveStats.statsCalculated)
	fprintf(out, ", mean % .03f ppm, dev % .03f ppm",
		ptpClock->servo.driftMean / 1000,
		ptpClock->servo.driftStdDev / 1000
	);
#endif /* PTPD_STATISTICS */
}
	    fprintf(out,"\n");

	fprintf(out,		STATUSPREFIX"  ","Message rates");

	if (ptpClock->logSyncInterval <= 0)
	    fprintf(out,"%.0f/s",pow(2,-ptpClock->logSyncInterval));
	else
	    fprintf(out,"1/%.0fs",pow(2,ptpClock->logSyncInterval));
	fprintf(out, " sync");

	if(ptpClock->delayMechanism == E2E) {
		if (ptpClock->logMinDelayReqInterval <= 0)
		    fprintf(out,", %.0f/s",pow(2,-ptpClock->logMinDelayReqInterval));
		else
		    fprintf(out,", 1/%.0fs",pow(2,ptpClock->logMinDelayReqInterval));
		fprintf(out, " delay");
	}

	if(ptpClock->delayMechanism == P2P) {
		if (ptpClock->logMinPdelayReqInterval <= 0)
		    fprintf(out,", %.0f/s",pow(2,-ptpClock->logMinPdelayReqInterval));
		else
		    fprintf(out,", 1/%.0fs",pow(2,ptpClock->logMinPdelayReqInterval));
		fprintf(out, " pdelay");
	}

	    fprintf(out,"\n");

	}
	if(ptpClock->portState == PTP_MASTER || ptpClock->portState == PTP_PASSIVE) {

	fprintf(out, 		STATUSPREFIX"  %d","Priority1 ", ptpClock->priority1);
	if(ptpClock->portState > PTP_MASTER)
		fprintf(out, " (master: %d)\n", ptpClock->grandmasterPriority1);
	fprintf(out, 		STATUSPREFIX"  %d","Priority2 ", ptpClock->priority2);
	if(ptpClock->portState > PTP_MASTER)
		fprintf(out, " (master: %d)\n", ptpClock->grandmasterPriority2);
	fprintf(out, 		STATUSPREFIX"  %d","ClockClass ", ptpClock->clockQuality.clockClass);
	if(ptpClock->portState > PTP_MASTER)
		fprintf(out, " (master: %d)\n", ptpClock->grandmasterClockQuality.clockClass);
	}


	fprintf(out, "\n");

	if (ptpClock->clockQuality.clockClass > 127) {
	fprintf(out, 		STATUSPREFIX"  %d\n","Announce received",
	    ptpClock->counters.announceMessagesReceived);
	fprintf(out, 		STATUSPREFIX"  %d\n","Sync received",
	    ptpClock->counters.syncMessagesReceived);
	if(ptpClock->twoStepFlag)
	fprintf(out, 		STATUSPREFIX"  %d\n","Follow-up received",
	    ptpClock->counters.followUpMessagesReceived);
	fprintf(out, 		STATUSPREFIX"  %d\n","DelayReq sent",
	    ptpClock->counters.delayReqMessagesSent);
	fprintf(out, 		STATUSPREFIX"  %d\n","DelayResp received",
	    ptpClock->counters.delayRespMessagesReceived);
	if(ptpClock->counters.domainMismatchErrors)
	fprintf(out, 		STATUSPREFIX"  %d\n","Domain Mismatches",
		    ptpClock->counters.domainMismatchErrors);
	}

	fprintf(out, 		STATUSPREFIX"  %d\n","State transitions",
		    ptpClock->counters.stateTransitions);
	fprintf(out, 		STATUSPREFIX"  %d\n","PTP Engine resets",
		    ptpClock->resetCount);


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
recordSync(RunTimeOpts * rtOpts, UInteger16 sequenceId, TimeInternal * time)
{
	if (rtOpts->recordLog.logEnabled && rtOpts->recordLog.logFP != NULL) {
		fprintf(rtOpts->recordLog.logFP, "%d %llu\n", sequenceId, 
		  ((time->seconds * 1000000000ULL) + time->nanoseconds)
		);
		maintainLogSize(&rtOpts->recordLog);
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


/* returns a double beween 0.0 and 1.0 */
double 
getRand(void)
{
	return ((rand() * 1.0) / RAND_MAX);
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


/* Whole block of adjtimex() functions starts here - not for Apple */
#if !defined(__APPLE__)

/*
 * Apply a tick / frequency shift to the kernel clock
 */

/* Integer32 version */
#ifndef PTPD_DOUBLE_SERVO

Boolean
adjFreq(Integer32 adj)
{

	extern RunTimeOpts rtOpts;
	struct timex t;
#ifdef HAVE_STRUCT_TIMEX_TICK
	Integer32 tickAdj = 0;
#ifdef RUNTIME_DEBUG
	Integer32 oldAdj = adj;
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

	t.freq = adj * ((1 << 16) / 1000);

	/* do calculation in double precision, instead of Integer32 */
	int t1 = t.freq;
	int t2;
	
	float f = (adj + 0.0) * (((1 << 16) + 0.0) / 1000.0);  /* could be float f = adj * 65.536 */
	t2 = t1;  // just to avoid compiler warning
	t2 = (int)round(f);
	t.freq = t2;

#ifdef HAVE_STRUCT_TIMEX_TICK
	DBG2("adjFreq: oldadj: %d, newadj: %d, tick: %d, tickadj: %d\n", oldAdj, adj,t.tick,tickAdj);
#endif /* HAVE_STRUCT_TIMEX_TICK */

	DBG2("        adj is %d;  t freq is %d       (float: %f Integer32: %d)\n", adj, t.freq,  f, t1);

	return !adjtimex(&t);

}

/* Version for double precision servo */
#else

Boolean
adjFreq(double adj)
{

	extern RunTimeOpts rtOpts;
	struct timex t;

#ifdef HAVE_STRUCT_TIMEX_TICK
	Integer32 tickAdj = 0;

#ifdef RUNTIME_DEBUG
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

#endif /* PTPD_DOUBLE_SERVO */

#ifndef PTPD_DOUBLE_SERVO
Integer32
getAdjFreq(void)
#else
double
getAdjFreq(void)
#endif /* PTPD_DOUBLE_SERVO */
{
	struct timex t;
	Integer32 freq;
	double dFreq;

	DBGV("getAdjFreq called\n");

	memset(&t, 0, sizeof(t));
	t.modes = 0;
	adjtimex(&t);

	dFreq = (t.freq + 0.0) / ((1<<16) / 1000.0);

	freq = (Integer32)round(dFreq);

	DBGV("          kernel adj is: float %f, Integer32 %d, kernel freq is: %d",
		dFreq, freq, t.freq);

#ifdef PTPD_DOUBLE_SERVO
	return(dFreq);
#else
	return(freq);
#endif /* PTPD_DOUBLE_SERVO */
}

#ifdef PTPD_DOUBLE_SERVO
#define DRIFTFORMAT "%.0f"
#else
#define DRIFTFORMAT "%d"
#endif /* PTPD_DOUBLE_SERVO */

void
restoreDrift(PtpClock * ptpClock, RunTimeOpts * rtOpts, Boolean quiet)
{

	FILE *driftFP;
	Boolean reset_offset = FALSE;
#ifndef PTPD_DOUBLE_SERVO
	Integer32 recovered_drift;
#else
	double recovered_drift;
#endif /* PTPD_DOUBLE_SERVO */

	DBGV("restoreDrift called\n");

	if (ptpClock->drift_saved && rtOpts->drift_recovery_method > 0 ) {
		ptpClock->servo.observedDrift = ptpClock->last_saved_drift;
		if (!rtOpts->noAdjust) {
#if defined(__APPLE__)
			adjTime(-ptpClock->last_saved_drift);
#else
			adjFreq_wrapper(rtOpts, ptpClock, -ptpClock->last_saved_drift);
#endif /* __APPLE__ */
		}
		DBG("loaded cached drift");
		return;
	}

	switch (rtOpts->drift_recovery_method) {

		case DRIFT_FILE:

			if( (driftFP = fopen(rtOpts->driftFile,"r")) == NULL) {
				PERROR("Could not open drift file: %s - using current kernel frequency offset",
					rtOpts->driftFile);
			} else

#ifndef PTPD_DOUBLE_SERVO
			if (fscanf(driftFP, "%d", &recovered_drift) != 1) {
#else
			if (fscanf(driftFP, "%lf", &recovered_drift) != 1) {
#endif /* PTPD_DOUBLE_SERVO */
				PERROR("Could not load saved offset from drift file - using current kernel frequency offset");
			} else {


			fclose(driftFP);
			if(quiet)
				DBGV("Observed drift loaded from %s: "DRIFTFORMAT"\n",
					rtOpts->driftFile,
					recovered_drift);
			else
				INFO("Observed drift loaded from %s: "DRIFTFORMAT"\n",
					rtOpts->driftFile,
					recovered_drift);
				break;
			}

		case DRIFT_KERNEL:

			recovered_drift = -getAdjFreq();
			if (quiet)
				DBGV("Observed_drift loaded from kernel: "DRIFTFORMAT"\n",
					recovered_drift);
			else
				INFO("Observed_drift loaded from kernel: "DRIFTFORMAT"\n",
					recovered_drift);

		break;


		default:

			reset_offset = TRUE;

	}

	if (reset_offset) {
		if (!rtOpts->noAdjust)
#if !defined(__APPLE__)
		  adjFreq_wrapper(rtOpts, ptpClock, 0);
#endif /* __APPLE__ */
		ptpClock->servo.observedDrift = 0;
		return;
	}

	ptpClock->servo.observedDrift = recovered_drift;

	ptpClock->drift_saved = TRUE;
	ptpClock->last_saved_drift = recovered_drift;
	if (!rtOpts->noAdjust)
#if !defined(__APPLE__)
		adjFreq(-recovered_drift);
#endif /* __APPLE__ */
}

#undef DRIFTFORMAT

void
saveDrift(PtpClock * ptpClock, RunTimeOpts * rtOpts, Boolean quiet)
{
	FILE *driftFP;

	DBGV("saveDrift called\n");

	if (rtOpts->drift_recovery_method > 0) {
		ptpClock->last_saved_drift = ptpClock->servo.observedDrift;
		ptpClock->drift_saved = TRUE;
	}

	if (rtOpts->drift_recovery_method != DRIFT_FILE)
		return;

	if( (driftFP = fopen(rtOpts->driftFile,"w")) == NULL) {
		PERROR("Could not open drift file %s for writing", rtOpts->driftFile);
		return;
	}

#ifndef PTPD_DOUBLE_SERVO
	fprintf(driftFP, "%d\n", ptpClock->servo.observedDrift);
#else
	/* The fractional part really won't make a difference here */
	fprintf(driftFP, "%d\n", (int)round(ptpClock->servo.observedDrift));
#endif /* PTPD_DOUBLE_SERVO */

	if (quiet) {
		DBGV("Wrote observed drift to %s\n", rtOpts->driftFile);
	} else {
		INFO("Wrote observed drift to %s\n", rtOpts->driftFile);
	}
	fclose(driftFP);
}

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
#endif /* MOD_TAI */


#else

void
adjTime(Integer32 nanoseconds)
{

	struct timeval t;

	t.tv_sec = 0;
	t.tv_usec = nanoseconds / 1000;

	if (adjtime(&t, NULL) < 0)
		PERROR("failed to ajdtime");

}

#endif /* __APPLE__ */


