/**
 * @file   sys.c
 * @date   Tue Jul 20 16:19:46 2010
 * 
 * @brief  Code to call kernel time routines and also display server statistics.
 * 
 * 
 */

#include "../ptpd.h"


int 
isTimeInternalNegative(const TimeInternal * p)
{
	return (p->seconds < 0) || (p->nanoseconds < 0);
}


int 
snprint_TimeInternal(char *s, int max_len, const TimeInternal * p)
{
	int len = 0;

	if (isTimeInternalNegative(p))
		len += snprintf(&s[len], max_len - len, "-");

	len += snprintf(&s[len], max_len - len, "%d.%09d",
	    abs(p->seconds), abs(p->nanoseconds));

	return len;
}


int 
snprint_ClockIdentity(char *s, int max_len, const ClockIdentity id, const char *info)
{
	int len = 0;
	int i;

	if (info)
		len += snprintf(&s[len], max_len - len, "%s", info);

	for (i = 0; ;) {
		len += snprintf(&s[len], max_len - len, "%02x", (unsigned char) id[i]);

		if (++i >= CLOCK_IDENTITY_LENGTH)
			break;

		// uncomment the line below to print a separator after each byte except the last one
		// len += snprintf(&s[len], max_len - len, "%s", "-");
	}

	return len;
}


int 
snprint_PortIdentity(char *s, int max_len, const PortIdentity *id, const char *info)
{
	int len = 0;

	if (info)
		len += snprintf(&s[len], max_len - len, "%s", info);

	len += snprint_ClockIdentity(&s[len], max_len - len, id->clockIdentity, NULL);
	len += snprintf(&s[len], max_len - len, "/%02x", (unsigned) id->portNumber);
	return len;
}


void
message(int priority, const char * format, ...)
{
	extern RunTimeOpts rtOpts;
	va_list ap;
	va_start(ap, format);
	if(rtOpts.useSysLog) {
		static Boolean logOpened;
		if(!logOpened) {
			openlog("ptpd", 0, LOG_USER);
			logOpened = TRUE;
		}
		vsyslog(priority, format, ap);
	} else {
		fprintf(stderr, "(ptpd %s) ",
			priority == LOG_EMERG ? "emergency" :
			priority == LOG_ALERT ? "alert" :
			priority == LOG_CRIT ? "critical" :
			priority == LOG_ERR ? "error" :
			priority == LOG_WARNING ? "warning" :
			priority == LOG_NOTICE ? "notice" :
			priority == LOG_INFO ? "info" :
			priority == LOG_DEBUG ? "debug" :
			"???");
		vfprintf(stderr, format, ap);
	}
	va_end(ap);
}

char *
translatePortState(PtpClock *ptpClock)
{
	char *s;
	switch(ptpClock->portState) {
	case PTP_INITIALIZING:  s = "init";  break;
	case PTP_FAULTY:        s = "flt";   break;
	case PTP_LISTENING:     s = "lstn";  break;
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

void 
displayStats(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	static int start = 1;
	static char sbuf[SCREEN_BUFSZ];
	int len = 0;
	struct timeval now;
	char time_str[MAXTIMESTR];

	if (start && rtOpts->csvStats) {
		start = 0;
		printf("timestamp, state, one way delay, offset from master, "
		       "slave to master, master to slave, drift, variance");
		fflush(stdout);
	}
	memset(sbuf, ' ', sizeof(sbuf));

	gettimeofday(&now, 0);
	strftime(time_str, MAXTIMESTR, "%Y-%m-%d %X", localtime(&now.tv_sec));

	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s%s:%06d, %s",
		       rtOpts->csvStats ? "\n" : "\rstate: ",
		       time_str, (int)now.tv_usec,
		       translatePortState(ptpClock));

	if (ptpClock->portState == PTP_SLAVE) {
		len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
			 &ptpClock->parentPortIdentity, ", ");

		/* 
		 * if grandmaster ID differs from parent port ID then
		 * also print GM ID 
		 */
		if (memcmp(ptpClock->grandmasterIdentity, 
			   ptpClock->parentPortIdentity.clockIdentity, 
			   CLOCK_IDENTITY_LENGTH)) {
			len += snprint_ClockIdentity(sbuf + len, 
						     sizeof(sbuf) - len,
						     ptpClock->grandmasterIdentity, 
						     " GM:");
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		if (!rtOpts->csvStats)
			len += snprintf(sbuf + len, 
					sizeof(sbuf) - len, "owd: ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
		    &ptpClock->meanPathDelay);

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		if (!rtOpts->csvStats)
			len += snprintf(sbuf + len, sizeof(sbuf) - len, 
					"ofm: ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
		    &ptpClock->offsetFromMaster);

		len += sprintf(sbuf + len,
			       ", %s%d.%09d" ", %s%d.%09d",
			       rtOpts->csvStats ? "" : "stm: ",
			       ptpClock->delaySM.seconds,
			       abs(ptpClock->delaySM.nanoseconds),
			       rtOpts->csvStats ? "" : "mts: ",
			       ptpClock->delayMS.seconds,
			       abs(ptpClock->delayMS.nanoseconds));
		
		len += sprintf(sbuf + len, ", %s%d",
		    rtOpts->csvStats ? "" : "drift: ", 
			       ptpClock->observed_drift);
	}
	else {
		if (ptpClock->portState == PTP_MASTER) {
			len += snprint_ClockIdentity(sbuf + len, 
						     sizeof(sbuf) - len,
						     ptpClock->clockIdentity, 
						     " (ID:");
			len += snprintf(sbuf + len, sizeof(sbuf) - len, ")");
		}
	}
	write(1, sbuf, rtOpts->csvStats ? len : SCREEN_MAXSZ + 1);
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
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
		PERROR("clock_gettime() failed, exiting.");
		exit(0);
	}
	time->seconds = tp.tv_sec;
	time->nanoseconds = tp.tv_nsec;

}

void 
setTime(TimeInternal * time)
{
	struct timeval tv;
 
	tv.tv_sec = time->seconds;
	tv.tv_usec = time->nanoseconds / 1000;
	settimeofday(&tv, 0);
	NOTIFY("resetting system clock to %ds %dns\n",
	       time->seconds, time->nanoseconds);
}

double 
getRand(void)
{
	return ((rand() * 1.0) / RAND_MAX);
}

Boolean 
adjFreq(Integer32 adj)
{
	struct timex t;

	if (adj > ADJ_FREQ_MAX)
		adj = ADJ_FREQ_MAX;
	else if (adj < -ADJ_FREQ_MAX)
		adj = -ADJ_FREQ_MAX;

	t.modes = MOD_FREQUENCY;
	t.freq = adj * ((1 << 16) / 1000);

	return !adjtimex(&t);
}
