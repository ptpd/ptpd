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


void 
displayStats(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	static int start = 1;
	static char sbuf[SCREEN_BUFSZ];
	char *s;
	int len = 0;

	if (start && rtOpts->csvStats) {
		start = 0;
		printf("state, one way delay, offset from master, drift");
		fflush(stdout);
	}
	memset(sbuf, ' ', sizeof(sbuf));

	switch (ptpClock->portState) {
	case PTP_INITIALIZING:
		s = "init";
		break;
	case PTP_FAULTY:
		s = "flt";
		break;
	case PTP_LISTENING:
		s = "lstn";
		break;
	case PTP_PASSIVE:
		s = "pass";
		break;
	case PTP_UNCALIBRATED:
		s = "uncl";
		break;
	case PTP_SLAVE:
		s = "slv";
		break;
	case PTP_PRE_MASTER:
		s = "pmst";
		break;
	case PTP_MASTER:
		s = "mst";
		break;
	case PTP_DISABLED:
		s = "dsbl";
		break;
	default:
		s = "?";
		break;
	}

	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s%s",
	    rtOpts->csvStats ? "\n" : "\rstate: ", s);

	if (ptpClock->portState == PTP_SLAVE) {
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		if (rtOpts->csvStats)
			len += snprintf(sbuf + len, 
					sizeof(sbuf) - len, "owd: ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
		    &ptpClock->meanPathDelay);

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		if (rtOpts->csvStats)
			len += snprintf(sbuf + len, sizeof(sbuf) - len, 
					"ofm: ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
		    &ptpClock->offsetFromMaster);

		len += sprintf(sbuf + len, ", %s%d",
		    rtOpts->csvStats ? "" : "drift: ", 
			       ptpClock->observed_drift);
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
	struct timeval tv;

	gettimeofday(&tv, 0);
	time->seconds = tv.tv_sec;
	time->nanoseconds = tv.tv_usec * 1000;
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
getRand()
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
