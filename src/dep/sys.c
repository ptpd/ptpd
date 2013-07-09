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
		    if(ptpClock->reset_count == 1){
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
	if(rtOpts.useLogFile && rtOpts.logFP != NULL) {
	    if(writeMessage(rtOpts.logFP, priority, format, ap) > 0) {
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

/* Reopen file pointer if condition is true, close if open and condition is false. */
int
restartLog(FILE** logFP, const Boolean condition, const char * logPath, const char * description)
{

        /* The FP is open - close it */
        if(*logFP != NULL) {
                fclose(*logFP);
		/*
		 * fclose doesn't do this at least on Linux - changes the underlying FD to -1,
		 * but not the FP to NULL - with this we can tell if the FP is closed
		 */
		*logFP=NULL;
                /* If we're not logging to file (any more), call it quits */
                if (!condition) {
                    INFO("Logging to %s file disabled. Closing file.\n", description);
                    return 1;
                }
        }
        /* FP is not open and we're not logging */
        if (!condition)
                return 1;

	/* Open the file */
        if ((*logFP = fopen(logPath, "a+")) == NULL)
                PERROR("Could not open %s file", description);
        else
		/* \n flushes output for us, no need for fflush() */
                setlinebuf(*logFP);

        return (*logFP != NULL);
}

void 
logStatistics(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	static char sbuf[SCREEN_BUFSZ];
	int len = 0;
	TimeInternal now;
	time_t time_s;
	FILE* destination;
	static TimeInternal prev_now;
	char time_str[MAXTIMESTR];

	if (!rtOpts->logStatistics) {
		return;
	}

	if(rtOpts->useStatisticsFile && rtOpts->statisticsFP != NULL)
	    destination = rtOpts->statisticsFP;
	else
	    destination = stdout;

	if (ptpClock->resetStatisticsLog) {
		ptpClock->resetStatisticsLog = FALSE;
		fprintf(destination,"# Timestamp, State, Clock ID, One Way Delay, "
		       "Offset From Master, Slave to Master, "
		       "Master to Slave, Observed Drift, Last packet Received\n");
	}
	memset(sbuf, ' ', sizeof(sbuf));

	getTime(&now);

	/*
	 * print one log entry per X seconds, to reduce disk usage.
	 * This only happens to SLAVE SYNC statistics lines, which are the bulk of the log.
	 * All other lines are printed, including delayreqs.
	 */

	if ((ptpClock->portState == PTP_SLAVE) && (rtOpts->statisticsInterval)) {
		if(ptpClock->last_packet_was_sync){
			ptpClock->last_packet_was_sync = FALSE;
			if((now.seconds - prev_now.seconds) < rtOpts->statisticsInterval){
				//leave early and do not print the log message to save disk space
				DBGV("Skipped printing of Sync message because of option -V\n");
				return;
			}
			prev_now = now;
		}
	}
	ptpClock->last_packet_was_sync = FALSE;

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

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %d, %c",
			       ptpClock->observed_drift,
			       ptpClock->char_last_msg);

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
						     " %d ", ptpClock->reset_count);
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

	if (fprintf(destination,sbuf) < len) {
		PERROR("Error while writing statistics");
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

	if (ptpClock->portState == PTP_SLAVE ||
	    ptpClock->portState == PTP_MASTER ||
	    ptpClock->portState == PTP_PASSIVE) {
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", Best master: ");
		len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
			&ptpClock->foreign[ptpClock->foreign_record_best].header.sourcePortIdentity);
        }

        len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");
        NOTICE("%s",sbuf);
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
	if (rtOpts->recordFP)
		fprintf(rtOpts->recordFP, "%d %llu\n", sequenceId, 
		  ((time->seconds * 1000000000ULL) + time->nanoseconds)
		);
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


/*
 * Apply a tick / frequency shift to the kernel clock
 */
#if !defined(__APPLE__)
Boolean
adjFreq(Integer32 adj)
{

	extern RunTimeOpts rtOpts;
	struct timex t;
	Integer32 tickAdj = 0;

#ifdef RUNTIME_DEBUG
	Integer32 oldAdj = adj;
#endif

	memset(&t, 0, sizeof(t));

	/* Get the USER_HZ value */
	Integer32 userHZ = sysconf(_SC_CLK_TCK);

	/*
	 * Get the tick resolution (ppb) - offset caused by changing the tick value by 1.
	 * The ticks value is the duration of one tick in us. So with userHz = 100  ticks per second,
	 * change of ticks by 1 (us) means a 100 us frequency shift = 100 ppm = 100000 ppb.
	 * For userHZ = 1000, change by 1 is a 1ms offset (10 times more ticks per second)
	 */
	Integer32 tickRes = userHZ * 1000;

	/* Clamp to max PPM */
	if (adj > rtOpts.servoMaxPpb){
		adj = rtOpts.servoMaxPpb;
	} else if (adj < -rtOpts.servoMaxPpb){
		adj = -rtOpts.servoMaxPpb;
	}

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
#ifdef HAVE_STRUCT_TIMEX_TICK
	/* Base tick duration - 10000 when userHZ = 100 */
	t.tick = 1E6 / userHZ;
	/* Tick adjustment if necessary */
        t.tick += tickAdj;

	t.modes = MOD_FREQUENCY | ADJ_TICK;
#else
	t.modes = MOD_FREQUENCY;
#endif /* HAVE_STRUCT_TIMEX_TICK */
	t.freq = adj * ((1 << 16) / 1000);

	/* do calculation in double precision, instead of Integer32 */
	int t1 = t.freq;
	int t2;
	
	float f = (adj + 0.0) * (((1 << 16) + 0.0) / 1000.0);  /* could be float f = adj * 65.536 */
	t2 = t1;  // just to avoid compiler warning
	t2 = (int)round(f);
	t.freq = t2;
	DBG2("adjFreq: oldadj: %d, newadj: %d, tick: %d, tickadj: %d\n", oldAdj, adj,t.tick,tickAdj);
	DBG2("        adj is %d;  t freq is %d       (float: %f Integer32: %d)\n", adj, t.freq,  f, t1);
	
	return !adjtimex(&t);
}

Integer32
getAdjFreq(void)
{
	struct timex t;
	Integer32 freq;
	float float_freq;

	DBGV("getAdjFreq called\n");

	memset(&t, 0, sizeof(t));
	t.modes = 0;
	adjtimex(&t);

	float_freq = (t.freq + 0.0) / ((1<<16) / 1000.0);
	freq = (Integer32)round(float_freq);
	DBGV("          kernel adj is: float %f, Integer32 %d, kernel freq is: %d",
		float_freq, freq, t.freq);

	return(freq);
}

void
restoreDrift(PtpClock * ptpClock, RunTimeOpts * rtOpts, Boolean quiet)
{

	FILE *driftFP;
	Boolean reset_offset = FALSE;
	Integer32 recovered_drift;

	DBGV("restoreDrift called\n");

	if (ptpClock->drift_saved && rtOpts->drift_recovery_method > 0 ) {
		ptpClock->observed_drift = ptpClock->last_saved_drift;
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

			if (fscanf(driftFP, "%d", &recovered_drift) != 1) {
				PERROR("Could not load saved offset from drift file - using current kernel frequency offset");
			} else {

			fclose(driftFP);
			if(quiet)
				DBGV("Observed drift loaded from %s: %d\n",
					rtOpts->driftFile,
					recovered_drift);
			else
				INFO("Observed drift loaded from %s: %d\n",
					rtOpts->driftFile,
					recovered_drift);
				break;
			}

		case DRIFT_KERNEL:

			recovered_drift = -getAdjFreq();
			if (quiet)
				DBGV("Observed_drift loaded from kernel: %d\n",
					recovered_drift);
			else
				INFO("Observed_drift loaded from kernel: %d\n",
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
		ptpClock->observed_drift = 0;
		return;
	}

	ptpClock->observed_drift = recovered_drift;

/*
 * temporary: will be used when clock stabilisation
 * estimation is implemented
 */
//	ptpClock->drift_saved = TRUE;
	ptpClock->last_saved_drift = recovered_drift;
	if (!rtOpts->noAdjust)
#if !defined(__APPLE__)
		adjFreq(-recovered_drift);
#endif /* __APPLE__ */
}

void
saveDrift(PtpClock * ptpClock, RunTimeOpts * rtOpts, Boolean quiet)
{
	FILE *driftFP;

	DBGV("saveDrift called\n");

	if (rtOpts->drift_recovery_method > 0) {
		ptpClock->last_saved_drift = ptpClock->observed_drift;
		ptpClock->drift_saved = TRUE;
	}

	if (rtOpts->drift_recovery_method != DRIFT_FILE)
		return;

	if( (driftFP = fopen(rtOpts->driftFile,"w")) == NULL) {
		PERROR("Could not open drift file %s for writing", rtOpts->driftFile);
		return;
	}

	fprintf(driftFP, "%d\n", ptpClock->observed_drift);

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


