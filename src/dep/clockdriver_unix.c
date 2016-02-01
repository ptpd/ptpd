/* Copyright (c) 2015 Wojciech Owczarek,
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
 * @file   clockdriver_unix.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  Unix OS clock driver implementation
 *
 */

#include "clockdriver.h"
#include "clockdriver_interface.h"

#define THIS_COMPONENT "clock.unix: "

/* accumulates all clock steps to simulate monotonic time */
static TimeInternal _stepAccumulator = {0,0};
/* tracking the number of instances - only one allowed */
static int _instanceCount = 0;

static void setRtc(ClockDriver* self, TimeInternal *timeToSet);
static Boolean adjFreq_unix(ClockDriver *self, double adj);
static void updateXtmp_unix (TimeInternal oldTime, TimeInternal newTime);

#ifdef __QNXNTO__
static const struct sigevent* timerIntHandler(void* data, int id);
#endif

Boolean
_setupClockDriver_unix(ClockDriver* self)
{

    if((self->type == SYSTEM_CLOCK_TYPE) && (_instanceCount > 0)) {
	WARNING(THIS_COMPONENT"Only one instance of the system clock driver is allowed\n");
	return FALSE;
    }

    INIT_INTERFACE(self);
    INIT_DATA(self, linuxphc);
    INIT_CONFIG(self, linuxphc);

    _instanceCount++;

    self->_instanceCount = &_instanceCount;

    self->systemClock = TRUE;

    resetIntPermanentAdev(&self->_adev);
    getTimeMonotonic(self, &self->_initTime);

    strncpy(self->name, SYSTEM_CLOCK_NAME, CLOCKDRIVER_NAME_MAX);

    INFO(THIS_COMPONENT"Started Unix clock driver %s\n", self->name);

    return TRUE;

}

static int
clockdriver_init(ClockDriver* self, const void *config) {

    self->_init = TRUE;
    self->inUse = TRUE;
    memset(self->config.frequencyFile, 0, PATH_MAX);
    snprintf(self->config.frequencyFile, PATH_MAX, PTPD_PROGNAME"_systemclock.frequency");
    self->maxFrequency = ADJ_FREQ_MAX;
    self->servo.maxOutput = self->maxFrequency;
    self->setState(self, CS_FREERUN);
    return 1;

}

static int
clockdriver_shutdown(ClockDriver *self) {
    _instanceCount--;
    INFO(THIS_COMPONENT"Unix clock driver %s shutting down\n", self->name);
    return 1;
}



static Boolean
getTime (ClockDriver *self, TimeInternal *time) {

#ifdef __QNXNTO__
  static TimerIntData tmpData;
  int ret;
  uint64_t delta;
  double tick_delay;
  uint64_t clock_offset;
  struct timespec tp;
  if(!tDataUpdated) {
    memset(&tData, 0, sizeof(TimerIntData));
    if(ThreadCtl(_NTO_TCTL_IO, 0) == -1) {
      ERROR(THIS_COMPONENT"QNX: could not give process I/O privileges");
      return FALSE;
    }

    tData.cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
    tData.ns_per_tick = 1000000000.0 / tData.cps;
    tData.prev_tsc = ClockCycles();
    clock_gettime(CLOCK_REALTIME, &tp);
    tData.last_clock = timespec2nsec(&tp);
    ret = InterruptAttach(0, timerIntHandler, &tData, sizeof(TimerIntData), _NTO_INTR_FLAGS_END | _NTO_INTR_FLAGS_TRK_MSK);

    if(ret == -1) {
      ERROR(THIS_COMPONENT"QNX: could not attach to timer interrupt");
      return FALSE;
    }
    tDataUpdated = TRUE;
    time->seconds = tp.tv_sec;
    time->nanoseconds = tp.tv_nsec;
    return;
  }

  memcpy(&tmpData, &tData, sizeof(TimerIntData));

  delta = ClockCycles() - tmpData.prev_tsc;

  /* compute time since last clock update */
  tick_delay = (double)delta / (double)tmpData.filtered_delta;
  clock_offset = (uint64_t)(tick_delay * tmpData.ns_per_tick * (double)tmpData.filtered_delta);

  /* not filtered yet */
  if(tData.counter < 2) {
    clock_offset = 0;
  }

    DBGV("QNX getTime cps: %lld tick interval: %.09f, time since last tick: %lld\n",
    tmpData.cps, tmpData.filtered_delta * tmpData.ns_per_tick, clock_offset);

    nsec2timespec(&tp, tmpData.last_clock + clock_offset);

    time->seconds = tp.tv_sec;
    time->nanoseconds = tp.tv_nsec;
  return TRUE;
#else

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)

	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
		PERROR(THIS_COMPONENT"clock_gettime() failed, exiting.");
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
#endif /* __QNXNTO__ */

    return TRUE;

}

Boolean
getTimeMonotonic(ClockDriver *self, TimeInternal * time)
{

	Boolean ret;

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) && defined(CLOCK_MONOTINIC)
	struct timespec tp;
	if (clock_gettime(CLOCK_MONOTONIC, &tp) >= 0) {
	    time->seconds = tp.tv_sec;
	    time->nanoseconds = tp.tv_nsec;
	    return TRUE;
	}
#endif /* _POSIX_TIMERS */
	ret = self->getTime(self, time);

	/* simulate a monotonic clock, tracking all step changes */
	addTime(time, time, &_stepAccumulator);

	return ret;

}

static Boolean
getUtcTime (ClockDriver *self, TimeInternal *out) {
    return TRUE;
}

static Boolean
setTime (ClockDriver *self, TimeInternal *time, Boolean force) {

	GET_CONFIG(self, myConfig, unix);

	TimeInternal oldTime, delta;

	getTime(self, &oldTime);

	subTime(&delta, &oldTime, time);

	if(self->config.readOnly) {
		return FALSE;
	}

	if(force) {
	    self->lockedUp = FALSE;
	}

	if(!force && !self->config.negativeStep && isTimeNegative(&delta)) {
		CRITICAL(THIS_COMPONENT"Cannot step Unix clock %s backwards\n", self->name);
		CRITICAL(THIS_COMPONENT"Manual intervention required or SIGUSR1 to force %s clock step\n", self->name);
		self->lockedUp = TRUE;
		self->setState(self, CS_NEGSTEP);
		return FALSE;
	}

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)

	struct timespec tp;
	tp.tv_sec = time->seconds;
	tp.tv_nsec = time->nanoseconds;

	if(tp.tv_sec == 0) {
	    ERROR(THIS_COMPONENT"Unix clock driver %s: cannot set time to zero seconds\n", self->name);
	    return FALSE;
	}

	if(tp.tv_sec <= 0) {
	    ERROR(THIS_COMPONENT"Unic clock driver %s: cannot set time to a negative value %d\n", self->name, tp.tv_sec);
	    return FALSE;
	}

#else

	struct timeval tv;
	tv.tv_sec = time->seconds;
	tv.tv_usec = time->nanoseconds / 1000;

	if(tv.tv_sec == 0) {
	    ERROR(THIS_COMPONENT"Unix clock %s: cannot set time to zero seconds\n", self->name);
	    return FALSE;
	}

	if(tv.tv_sec < 0) {
	    ERROR(THIS_COMPONENT"Unic clock %s: cannot set time to a negative value %d\n", self->name, tv.tv_sec);
	    return FALSE;
	}


#endif /* _POSIX_TIMERS */

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
	if (clock_settime(CLOCK_REALTIME, &tp) < 0) {
		PERROR(THIS_COMPONENT"Could not set system time");
		return FALSE;
	}
	addTime(&_stepAccumulator, &_stepAccumulator, &delta);
#else

	settimeofday(&tv, 0);
	addTime(&_stepAccumulator, &_stepAccumulator, &delta);
#endif /* _POSIX_TIMERS */

	if(oldTime.seconds != time->seconds) {
	    updateXtmp_unix(oldTime, *time);
	    if(myConfig->setRtc) {
		setRtc(self, time);
	    }
	}

	self->_stepped = TRUE;

	struct timespec tmpTs = { time->seconds,0 };

	char timeStr[MAXTIMESTR];
	strftime(timeStr, MAXTIMESTR, "%x %X", localtime(&tmpTs.tv_sec));
	NOTICE(THIS_COMPONENT"Unix clock %s: stepped the system clock to: %s.%d\n", self->name,
	       timeStr, time->nanoseconds);

	self->setState(self, CS_FREERUN);

	return TRUE;

}

static Boolean
stepTime (ClockDriver *self, TimeInternal *delta, Boolean force) {

	GET_CONFIG(self, myConfig, unix);

	if(isTimeZero(delta)) {
	    return TRUE;
	}

	struct timespec nts;

	TimeInternal oldTime,newTime;
	getTime(self, &oldTime);
	addTime(&newTime, &oldTime, delta);

	if(self->config.readOnly) {
		return FALSE;
	}

	if(force) {
	    self->lockedUp = FALSE;
	}

	if(!force && !self->config.negativeStep && isTimeNegative(delta)) {
		CRITICAL(THIS_COMPONENT"Cannot step Unix clock %s backwards\n", self->name);
		CRITICAL(THIS_COMPONENT"Manual intervention required or SIGUSR1 to force %s clock step\n", self->name);
		self->lockedUp = TRUE;
		self->setState(self, CS_NEGSTEP);
		return FALSE;
	}

#ifdef ADJ_SETOFFSET
	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = ADJ_SETOFFSET | ADJ_NANO;

	tmx.time.tv_sec = delta->seconds;
	tmx.time.tv_usec = delta->nanoseconds;

	getTime(self, &newTime);
	addTime(&newTime, &newTime, delta);

	nts.tv_sec = newTime.seconds;

	if(nts.tv_sec == 0) {
	    ERROR(THIS_COMPONENT"Unix clock %s: cannot step time to zero seconds\n", self->name);
	    return FALSE;
	}

	if(nts.tv_sec < 0) {
	    ERROR(THIS_COMPONENT"Unix clock %s: cannot step time to a negative value %d\n", self->name, nts.tv_sec);
	    return FALSE;
	}

	if(adjtimex(&tmx) < 0) {
	    DBG("Could not set clock offset of Unix clock %s\n", self->name);
	    return setTime(self, &newTime, force);
	}

	NOTICE(THIS_COMPONENT"Unix clock %s: stepped clock by %s%d.%09d seconds\n", self->name,
		    (delta->seconds <0 || delta->nanoseconds <0) ? "-":"", delta->seconds, delta->nanoseconds);

	addTime(&_stepAccumulator, &_stepAccumulator, delta);

	self->_stepped = TRUE;

	self->setState(self, CS_FREERUN);

	if(oldTime.seconds != newTime.seconds) {
	    updateXtmp_unix(oldTime, newTime);
	    if(myConfig->setRtc) {
		setRtc(self, &newTime);
	    }
	}

	return TRUE;
#else
	return setTime(self, &newTime, force);
#endif


}


static Boolean
setFrequency (ClockDriver *self, double adj, double tau) {

    if (self->config.readOnly){
		DBGV("adjFreq2: noAdjust on, returning\n");
		return FALSE;
	}

	self->_tau = tau;

/*
 * adjFreq simulation for QNX: correct clock by x ns per tick over clock adjust interval,
 * to make it equal adj ns per second. Makes sense only if intervals are regular.
 */

#ifdef __QNXNTO__

      struct _clockadjust clockadj;
      struct _clockperiod period;
      if (ClockPeriod (CLOCK_REALTIME, 0, &period, 0) < 0)
          return FALSE;

	adj = clampDouble(adj, self->maxFrequency);

	/* adjust clock for the duration of 0.9 clock update period in ticks (so we're done before the next) */
	clockadj.tick_count = 0.9 * tau * 1E9 / (period.nsec + 0.0);

	/* scale adjustment per second to adjustment per single tick */
	clockadj.tick_nsec_inc = (adj * tau / clockadj.tick_count) / 0.9;

	DBGV("QNX: adj: %.09f, dt: %.09f, ticks per dt: %d, inc per tick %d\n",
		adj, tau, clockadj.tick_count, clockadj.tick_nsec_inc);

	if (ClockAdjust(CLOCK_REALTIME, &clockadj, NULL) < 0) {
	    DBGV("QNX: failed to call ClockAdjust: %s\n", strERROR(THIS_COMPONENTerrno));
	}
/* regular adjFreq */
#elif defined(HAVE_SYS_TIMEX_H)
	DBG2("     adjFreq2: call adjfreq to %.09f us \n", adj / DBG_UNIT);
	adjFreq_unix(self, adj);
/* otherwise use adjtime */
#else
	struct timeval tv;

	adj = clampDouble(adj, self->maxFrequency);

	tv.tv_sec = 0;
	tv.tv_usec = (adj / 1000);
	if((tau > 0) && (tau < 1.0)) {
	    tv.tv_usec *= tau;
	}
	adjtime(&tv, NULL);
#endif

    self->lastFrequency = adj;

    return TRUE;
}

static double
getFrequency (ClockDriver * self) {

	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	if(adjtimex(&tmx) < 0) {
	    PERROR(THIS_COMPONENT"Could not get frequency of clock %s ", self->name);
	    return 0;
	}

	return((tmx.freq + 0.0) / 65.536);

}

static Boolean
    getStatus (ClockDriver *self, ClockStatus *status) {
    return TRUE;
}

static Boolean
setStatus (ClockDriver *self, ClockStatus *status) {
    return TRUE;
}

/*
 *this is the OS clock driver unaware of the other drivers,
 * so we compare the clock using the other clock's comparison.
 */
static Boolean
getOffsetFrom (ClockDriver *self, ClockDriver *from, TimeInternal *output)
{

	TimeInternal delta;

	if(from->systemClock) {
	    output->seconds = 0;
	    output->nanoseconds = 0;
	    return TRUE;
	}

	if(!from->getOffsetFrom(from, self, &delta)) {
	    INFO(THIS_COMPONENT"%s FALSE AGAIN?\n", self->name);
	    return FALSE;
	}

	output->seconds = -delta.seconds;
	output->nanoseconds = -delta.nanoseconds;

	return TRUE;
}


static Boolean
adjFreq_unix(ClockDriver *self, double adj)
{

	struct timex t;

#ifdef HAVE_STRUCT_TIMEX_TICK
	Integer32 tickAdj = 0;

#ifdef PTPD_DBG2
	double oldAdj = adj;
#endif

#endif /* HAVE_STRUCT_TIMEX_TICK */

	memset(&t, 0, sizeof(t));

	/* Clamp to max PPM */
	if (adj > self->maxFrequency){
		adj = self->maxFrequency;
	} else if (adj < -self->maxFrequency){
		adj = -self->maxFrequency;
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





/* Set the RTC to the desired time time */
static void setRtc(ClockDriver *self, TimeInternal *timeToSet)
{

#ifdef HAVE_LINUX_RTC_H

	char* rtcDev;
	struct tm* tmTime;
	time_t seconds;
	int rtcFd;
	struct stat statBuf;


	    if(stat("/dev/misc/rtc", &statBuf) == 0) {
            	rtcDev="/dev/misc/rtc\0";
	    } else if(stat("/dev/rtc", &statBuf) == 0) {
            	rtcDev="/dev/rtc\0";
	    }  else if(stat("/dev/rtc0", &statBuf) == 0) {
            	rtcDev="/dev/rtc0\0";
	    } else {

			ERROR(THIS_COMPONENT"Could not set RTC time - no suitable rtc device found\n");
			return;
	    }

	    if(!S_ISCHR(statBuf.st_mode)) {
			ERROR(THIS_COMPONENT"Could not set RTC time - device %s is not a character device\n",
			rtcDev);
			return;
	    }

	DBGV("Usable RTC device: %s\n",rtcDev);

	if(isTimeZero(timeToSet)) {
	    getTime(self, timeToSet);
	}

	if((rtcFd = open(rtcDev, O_RDONLY)) < 0) {
		PERROR(THIS_COMPONENT"Could not set RTC time: error opening %s", rtcDev);
		return;
	}

	seconds = (time_t)timeToSet->seconds;
	if(timeToSet->nanoseconds >= 500000) seconds++;
	tmTime =  gmtime(&seconds);

	DBGV("Set RTC from %d seconds to y: %d m: %d d: %d \n",timeToSet->seconds,tmTime->tm_year,tmTime->tm_mon,tmTime->tm_mday);

	if(ioctl(rtcFd, RTC_SET_TIME, tmTime) < 0) {
		PERROR(THIS_COMPONENT"Could not set RTC time on %s - ioctl failed", rtcDev);
		goto cleanup;
	}

	NOTIFY(THIS_COMPONENT"Succesfully set RTC time using %s\n", rtcDev);

cleanup:

	close(rtcFd);
#endif /* HAVE_LINUX_RTC_H */

}


static void
updateXtmp_unix (TimeInternal oldTime, TimeInternal newTime)
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

#ifdef HAVE_STRUCT_UTMP_UT_TIME
		ut.ut_time = oldTime.seconds;
#else
		ut.ut_tv.tv_sec = oldTime.seconds;
		ut.ut_tv.tv_usec = oldTime.nanoseconds / 1000;
#endif /* HAVE_STRUCT_UTMP_UT_TIME */

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

#ifdef HAVE_STRUCT_UTMP_UT_TIME
		ut.ut_time = newTime.seconds;
#else
		ut.ut_tv.tv_sec = newTime.seconds;
		ut.ut_tv.tv_usec = newTime.nanoseconds / 1000;
#endif /* HAVE_STRUCT_UTMP_UT_TIME */
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

#ifdef __QNXNTO__

static const struct sigevent* timerIntHandler(void* data, int id) {
  struct timespec tp;
  TimerIntData* myData = (TimerIntData*)data;
  uint64_t new_tsc = ClockCycles();

  clock_gettime(CLOCK_REALTIME, &tp);

  if(new_tsc > myData->prev_tsc) {
    myData->cur_delta = new_tsc - myData->prev_tsc;
  /* when hell freezeth over, thy TSC shall roll over */
  } else {
    myData->cur_delta = myData->prev_delta;
  }
  /* 4/6 weighted average */
  myData->filtered_delta = (40 * myData->cur_delta + 60 * myData->prev_delta) / 100;
  myData->prev_delta = myData->cur_delta;
  myData->prev_tsc = new_tsc;

  if(myData->counter < 2) {
    myData->counter++;
  }

  myData->last_clock = timespec2nsec(&tp);
  return NULL;

}
#endif

static Boolean
pushPrivateConfig(ClockDriver *self, RunTimeOpts *global)
{

    GET_CONFIG(self, myConfig, unix);

    myConfig->setRtc = global->setRtc;

    return TRUE;

}

static Boolean
isThisMe(ClockDriver *driver, const char* search)
{
	if(!driver->systemClock) {
	    return FALSE;
	}
	if(!strncmp(search, SYSTEM_CLOCK_NAME, CLOCKDRIVER_NAME_MAX)) {
	    return TRUE;
	}

	return FALSE;

}
