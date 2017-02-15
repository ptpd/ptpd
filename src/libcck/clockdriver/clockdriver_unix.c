/* Copyright (c) 2016-2017 Wojciech Owczarek,
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

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <sys/time.h>
#include <time.h>

#ifdef HAVE_SYS_TIMEX_H
#include <sys/timex.h>
#endif /* HAVE_SYS_TIMEX_H */

#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#else
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif /* HAVE_UTMP_H */
#endif /* HAVE_UTMPX_H */

#ifdef HAVE_LINUX_RTC_H
#include <linux/rtc.h>
#endif /* HAVE_LINUX_RTC_H */

#ifndef HAVE_ADJTIMEX
#ifdef HAVE_NTP_ADJTIME
#define adjtimex ntp_adjtime
#endif /* HAVE_NTP_ADJTIME */
#endif /* HAVE_ADJTIMEX */

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_logger.h>
#include <libcck/cck_utils.h>
#include <libcck/clockdriver.h>
#include <libcck/clockdriver_interface.h>

#if defined(linux) && !defined(ADJ_SETOFFSET)
#define ADJ_SETOFFSET 0x0100
#endif

#define THIS_COMPONENT "clock.unix: "

#define ADJ_FREQ_MAX 500000

/* accumulates all clock steps to simulate monotonic time */
static CckTimestamp _stepAccumulator = {0,0};
/* tracking the number of instances - only one allowed */
static int _instanceCount = 0;

static void setRtc(ClockDriver* self, CckTimestamp *timeToSet);
#ifdef HAVE_SYS_TIMEX_H
static bool adjFreq_unix(ClockDriver *self, double adj);
#endif

static void updateXtmp_unix (CckTimestamp oldTime, CckTimestamp newTime);

#ifdef __QNXNTO__

#include <sys/syspage.h>
#include <sys/neutrino.h>

typedef struct {
  _uint64 counter;		/* iteration counter */
  _uint64 prev_tsc;		/* previous clock cycles */
  _uint64 last_clock;		/* clock reading at last timer interrupt */
  _uint64 cps;			/* cycles per second */
  _uint64 prev_delta;		/* previous clock cycle delta */
  _uint64 cur_delta;		/* last clock cycle delta */
  _uint64 filtered_delta;	/* filtered delta */
  double ns_per_tick;		/* nanoseconds per cycle */
} TimerIntData;

/* do not access directly! tied to clock interrupt! */
static TimerIntData tData;
static bool tDataUpdated = false;
static const struct sigevent* timerIntHandler(void* data, int id);

#endif /* __QNXNTO__ */

bool
_setupClockDriver_unix(ClockDriver* self)
{

    if((self->type == SYSTEM_CLOCK_TYPE) && (_instanceCount > 0)) {
	CCK_WARNING(THIS_COMPONENT"Only one instance of the system clock driver is allowed\n");
	return false;
    }

    INIT_INTERFACE(self);
    CCK_INIT_PDATA(ClockDriver, unix, self);
    CCK_INIT_PCONFIG(ClockDriver, unix, self);

    _instanceCount++;

    self->_instanceCount = &_instanceCount;

    self->systemClock = true;

    resetIntPermanentAdev(&self->_adev);
    getTimeMonotonic(self, &self->_initTime);

    strncpy(self->name, SYSTEM_CLOCK_NAME, CCK_COMPONENT_NAME_MAX);

    CCK_INFO(THIS_COMPONENT"Unix clock driver '%s' started successfully\n", self->name);

    return true;

}

static int
clockDriver_init(ClockDriver* self, const void *config) {

    self->_init = true;
    self->inUse = true;
    memset(self->config.frequencyFile, 0, PATH_MAX);
    snprintf(self->config.frequencyFile, PATH_MAX, CLOCKDRIVER_FREQFILE_PREFIX"_systemclock.frequency");
    self->maxFrequency = ADJ_FREQ_MAX;
    self->servo.maxOutput = self->maxFrequency;
    self->_vendorInit(self);
    self->setState(self, CS_FREERUN);
    return 1;

}

static int
clockDriver_shutdown(ClockDriver *self) {
    self->_vendorShutdown(self);
    _instanceCount--;
    CCK_INFO(THIS_COMPONENT"Unix clock driver '%s' shutting down\n", self->name);
    return 1;
}

static bool
getTime (ClockDriver *self, CckTimestamp *time) {

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
      CCK_ERROR(THIS_COMPONENT"QNX: could not give process I/O privileges");
      return false;
    }

    tData.cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
    tData.ns_per_tick = 1000000000.0 / tData.cps;
    tData.prev_tsc = ClockCycles();
    clock_gettime(CLOCK_REALTIME, &tp);
    tData.last_clock = timespec2nsec(&tp);

    ret = InterruptAttach(0, timerIntHandler, &tData, sizeof(TimerIntData), _NTO_INTR_FLAGS_END | _NTO_INTR_FLAGS_TRK_MSK);

    if(ret == -1) {
      CCK_ERROR(THIS_COMPONENT"QNX: could not attach to timer interrupt");
      return false;
    }

    tDataUpdated = true;
    time->seconds = tp.tv_sec;
    time->nanoseconds = tp.tv_nsec;

    return true;

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

    CCK_DBGV("QNX getTime cps: %lld tick interval: %.09f, time since last tick: %lld\n",
    tmpData.cps, tmpData.filtered_delta * tmpData.ns_per_tick, clock_offset);

    nsec2timespec(&tp, tmpData.last_clock + clock_offset);

    time->seconds = tp.tv_sec;
    time->nanoseconds = tp.tv_nsec;
  return true;

#else

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)

	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
		CCK_PERROR(THIS_COMPONENT"clock_gettime() failed, exiting.");
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

    return true;

}

bool
getTimeMonotonic(ClockDriver *self, CckTimestamp * time)
{

	bool ret;

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) && defined(CLOCK_MONOTINIC)
	struct timespec tp;
	if (clock_gettime(CLOCK_MONOTONIC, &tp) >= 0) {
	    time->seconds = tp.tv_sec;
	    time->nanoseconds = tp.tv_nsec;
	    return true;
	}
#endif /* _POSIX_TIMERS */
	ret = self->getTime(self, time);

	/* simulate a monotonic clock, tracking all step changes */
	tsOps()->add(time, time, &_stepAccumulator);

	return ret;

}

static bool
getUtcTime (ClockDriver *self, CckTimestamp *out) {
    return true;
}

static bool
setTime (ClockDriver *self, CckTimestamp *time) {

	CCK_GET_PCONFIG(ClockDriver, unix, self, myConfig);

	CckTimestamp oldTime, delta;

	if((!self->_init) || (self->state == CS_HWFAULT)) {
	    return false;
	}

	if(self->config.readOnly || self->config.disabled) {
		return false;
	}

	getTime(self, &oldTime);
	tsOps()->sub(&delta, &oldTime, time);

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
		CCK_PERROR(THIS_COMPONENT"Could not set system time");
		return false;
	}
	tsOps()->add(&_stepAccumulator, &_stepAccumulator, &delta);
#else

	settimeofday(&tv, 0);
	tsOps()->add(&_stepAccumulator, &_stepAccumulator, &delta);
#endif /* _POSIX_TIMERS */

	if(oldTime.seconds != time->seconds) {
	    updateXtmp_unix(oldTime, *time);
	    if(myConfig->setRtc) {
		setRtc(self, time);
	    }
	}

	return true;

}

static bool
setOffset (ClockDriver *self, CckTimestamp *delta) {

	CckTimestamp newTime = {0, 0};

	if((!self->_init) || (self->state == CS_HWFAULT)) {
	    return false;
	}

	if(tsOps()->isZero(delta)) {
	    return true;
	}

	if(self->config.readOnly || self->config.disabled) {
		return false;
	}

#ifndef ADJ_SETOFFSET
	getTime(self, &newTime);
	tsOps()->add(&newTime, &newTime, delta);
	return setTime(self, &newTime);
#else
	CCK_GET_PCONFIG(ClockDriver, unix, self, myConfig);
	CckTimestamp oldTime;

	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = ADJ_SETOFFSET | ADJ_NANO;

	tmx.time.tv_sec = delta->seconds;
	tmx.time.tv_usec = delta->nanoseconds;

	if(adjtimex(&tmx) < 0) {
	    CCK_DBG("Could not set clock offset of Unix clock %s\n", self->name);
	    getTime(self, &newTime);
	    tsOps()->add(&newTime, &newTime, delta);
	    return setTime(self, &newTime);
	} else {
	    getTime(self, &oldTime);
	    tsOps()->add(&newTime, &oldTime, delta);
	}

	tsOps()->add(&_stepAccumulator, &_stepAccumulator, delta);

	if(oldTime.seconds != newTime.seconds) {
	    updateXtmp_unix(oldTime, newTime);
	    if(myConfig->setRtc) {
		setRtc(self, &newTime);
	    }
	}

	return true;
#endif

}


static bool
setFrequency (ClockDriver *self, double adj, double tau) {

    if (self->config.readOnly){
		CCK_DBGV("adjFreq2: noAdjust on, returning\n");
		return false;
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
          return false;

	adj = clamp(adj, self->maxFrequency);

	/* adjust clock for the duration of 0.9 clock update period in ticks (so we're done before the next) */
	clockadj.tick_count = 0.9 * tau * 1E9 / (period.nsec + 0.0);

	/* scale adjustment per second to adjustment per single tick */
	clockadj.tick_nsec_inc = (adj * tau / clockadj.tick_count) / 0.9;

	CCK_DBGV("QNX: adj: %.09f, dt: %.09f, ticks per dt: %d, inc per tick %d\n",
		adj, tau, clockadj.tick_count, clockadj.tick_nsec_inc);

	if (ClockAdjust(CLOCK_REALTIME, &clockadj, NULL) < 0) {
	    CCK_DBGV("QNX: failed to call ClockAdjust: %s\n", strerror(errno));
	}
/* regular adjFreq */
#elif defined(HAVE_SYS_TIMEX_H)
	CCK_DBG2("     adjfreq: call adjfreq to %.03f ppb \n", adj);
	adjFreq_unix(self, adj);
/* otherwise use adjtime */
#else
	struct timeval tv;

	adj = clamp(adj, self->maxFrequency);

	tv.tv_sec = 0;
	tv.tv_usec = (adj / 1000);
	if((tau > 0) && (tau < 1.0)) {
	    tv.tv_usec *= tau;
	}
	adjtime(&tv, NULL);
#endif

    self->lastFrequency = adj;

    return true;
}

static double
getFrequency (ClockDriver * self) {

#ifdef HAVE_SYS_TIMEX_H
	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	if(adjtimex(&tmx) < 0) {
	    CCK_PERROR(THIS_COMPONENT"Could not get frequency of clock %s ", self->name);
	    return 0;
	}

	return((tmx.freq + 0.0) / 65.536);
#else
	return self->lastFrequency;
#endif

}

static bool
getSystemClockOffset(ClockDriver *self, CckTimestamp *output)
{
    output->seconds = 0;
    output->nanoseconds = 0;
    return true;
}

static bool
    getStatus (ClockDriver *self, ClockStatus *status) {
    return true;
}

static bool
setStatus (ClockDriver *self, ClockStatus *status) {
    return true;
}

/*
 *this is the OS clock driver unaware of the other drivers,
 * so we compare the clock using the other clock's comparison.
 */
static bool
getOffsetFrom (ClockDriver *self, ClockDriver *from, CckTimestamp *output)
{

	CckTimestamp delta;

	if(from->systemClock) {
	    output->seconds = 0;
	    output->nanoseconds = 0;
	    return true;
	}

	if(!from->getOffsetFrom(from, self, &delta)) {
	    CCK_INFO(THIS_COMPONENT"%s false AGAIN?\n", self->name);
	    return false;
	}

	output->seconds = -delta.seconds;
	output->nanoseconds = -delta.nanoseconds;

	return true;
}

#ifdef HAVE_SYS_TIMEX_H
static bool
adjFreq_unix(ClockDriver *self, double adj)
{

	struct timex t;

#ifdef HAVE_STRUCT_TIMEX_TICK
	int32_t tickAdj = 0;

#ifdef CCK_DEBUG
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
	int32_t userHZ = sysconf(_SC_CLK_TCK);

	/*
	 * Get the tick resolution (ppb) - offset caused by changing the tick value by 1.
	 * The ticks value is the duration of one tick in us. So with userHz = 100  ticks per second,
	 * change of ticks by 1 (us) means a 100 us frequency shift = 100 ppm = 100000 ppb.
	 * For userHZ = 1000, change by 1 is a 1ms offset (10 times more ticks per second)
	 */
	int32_t tickRes = userHZ * 1000;

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
	CCK_DBG2("adjFreq: oldadj: %.09f, newadj: %.09f, tick: %ld, tickadj: %d\n", oldAdj, adj,t.tick,tickAdj);
#endif /* HAVE_STRUCT_TIMEX_TICK */
	CCK_DBG2("        adj is %.09f;  t freq is %ld       (float: %.09f)\n", adj, t.freq,  dFreq);
	
	return !adjtimex(&t);
}
#endif /* HAVE_SYS_TIMEX_H */

/* Set the RTC to the desired time time */
static void setRtc(ClockDriver *self, CckTimestamp *timeToSet)
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

			CCK_ERROR(THIS_COMPONENT"Could not set RTC time - no suitable rtc device found\n");
			return;
	    }

	    if(!S_ISCHR(statBuf.st_mode)) {
			CCK_ERROR(THIS_COMPONENT"Could not set RTC time - device %s is not a character device\n",
			rtcDev);
			return;
	    }

	CCK_DBGV("Usable RTC device: %s\n",rtcDev);

	if(tsOps()->isZero(timeToSet)) {
	    getTime(self, timeToSet);
	}

	if((rtcFd = open(rtcDev, O_RDONLY)) < 0) {
		CCK_PERROR(THIS_COMPONENT"Could not set RTC time: error opening %s", rtcDev);
		return;
	}

	seconds = (time_t)timeToSet->seconds;
	if(timeToSet->nanoseconds >= 500000) seconds++;
	tmTime =  gmtime(&seconds);

	CCK_DBGV("Set RTC from %d seconds to y: %d m: %d d: %d \n",timeToSet->seconds,tmTime->tm_year,tmTime->tm_mon,tmTime->tm_mday);

	if(ioctl(rtcFd, RTC_SET_TIME, tmTime) < 0) {
		CCK_PERROR(THIS_COMPONENT"Could not set RTC time on %s - ioctl failed", rtcDev);
		goto cleanup;
	}

	CCK_NOTICE(THIS_COMPONENT"Succesfully set RTC time using %s\n", rtcDev);

cleanup:

	close(rtcFd);
#endif /* HAVE_LINUX_RTC_H */

}


static void
updateXtmp_unix (CckTimestamp oldTime, CckTimestamp newTime)
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

static bool
isThisMe(ClockDriver *driver, const char* search)
{
	if(!driver->systemClock) {
	    return false;
	}
	if(!strncmp(search, SYSTEM_CLOCK_NAME, CCK_COMPONENT_NAME_MAX)) {
	    return true;
	}

	return false;

}

static void
loadVendorExt(ClockDriver *driver) {
}


static bool
privateHealthCheck(ClockDriver *driver)
{

    bool ret = true;

    if(driver == NULL) {
	return false;
    }

    CCK_DBG(THIS_COMPONENT"clock %s private health check...\n", driver->name);

    return ret;

}
