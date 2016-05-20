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
 * @file   clockdriver.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  structure definitions for the clock driver
 *
 */

#ifndef PTPD_CLOCKDRIVER_H_
#define PTPD_CLOCKDRIVER_H_

#include "../ptpd.h"
#include "../globalconfig.h"
#include "linkedlist.h"

#include "piservo.h"

#define CLOCKDRIVER_NAME_MAX 20
#define CLOCKDRIVER_UPDATE_INTERVAL 1
#define CLOCKDRIVER_WARNING_TIMEOUT 60
#define CLOCK_SYNC_RATE 5
#define SYSTEM_CLOCK_NAME "syst"

#if defined(linux) && !defined(ADJ_SETOFFSET)
#define ADJ_SETOFFSET 0x0100
#endif


/* clock driver types */
enum {
    CLOCKDRIVER_UNIX = 0,
    CLOCKDRIVER_LINUXPHC = 1,
    CLOCKDRIVER_MAX
};

#define SYSTEM_CLOCK_TYPE CLOCKDRIVER_UNIX

/* clock commands that can be sent to all clock drivers */
enum {
    CD_NOTINUSE,	/* mark all not in use */
    CD_SHUTDOWN,	/* shutdown all */
    CD_CLEANUP,		/* clean up drivers not in use */
    CD_DUMP,		/* dump clock information for all */
    CD_STEP		/* step all clocks to their last known offsets */
};

/* clock stepping behaviour */
enum {
    CSTEP_NEVER,	/* never step clock */
    CSTEP_ALWAYS,	/* always step clock */
    CSTEP_STARTUP,	/* step only on startup and only if offset > 1 s */
    CSTEP_STARTUP_FORCE /* always step on startup, regardless of offset and if it's negative */
};

/* reference class */
enum {
    RC_PTP = 0,
    RC_EXTERNAL = 1,
    RC_INTERNAL = 2
};

/* clock states */
typedef enum {
    CS_SUSPENDED,	/* clock is suspended - no updates accepted */
    CS_NEGSTEP,		/* clock is locked up after negative step attempt */
    CS_STEP,		/* clock is suspended because of offset > 1 s */
    CS_HWFAULT,		/* hardware fault */
    CS_INIT,		/* initial state */
    CS_FREERUN,		/* clock is not being disciplined */
    CS_TRACKING,	/* clock is tracking a reference but is not locked */
    CS_HOLDOVER,	/* clock is in holdover after no updates or losing reference */
    CS_LOCKED		/* clock is locked to reference (adev below threshold) */
} ClockState;

/* clock driver configuration */
typedef struct {
    Boolean disabled;			/* clock is disabled - it's skipped from sync and all other operation.
					 * if it is being used by PTP, it is set read-only instead */
    Boolean required;			/* clock cannot be disabled */
    Boolean excluded;			/* clock is excluded from best clock selection */
    Boolean readOnly;			/* clock is read-only */
    Boolean externalOnly;		/* this clock will only accept an external reference */
    Boolean internalOnly;		/* this clock will only accept an internal reference */
    Boolean noStep;			/* clock cannot be stepped */
    Boolean negativeStep;		/* clock can be stepped backwards */
    Boolean storeToFile;		/* clock stores good frequency in a file */
    Boolean outlierFilter;		/* enable MAD-based outlier filter */
    Boolean statFilter;			/* enable statistical filter */
    char frequencyFile[PATH_MAX +1];	/* frequency file - filled by the driver itself */
    char frequencyDir[PATH_MAX + 1];	/* frequency file directory */
    int adevPeriod;			/* Allan deviation measurement period */
    double stableAdev;			/* Allan deviation low watermark for LOCKED state */
    double unstableAdev;		/* Allan deviation high watermark for LOCKED state -> TRACKING */
    double madMax;			/* Mad Max! Maximum times MAD from median - outlier cutoff factor */
    int outlierFilterBlockTimeout;	/* Maximum (tau * blocked samples) before filter is forcefully unlocked */
    int madDelay;			/* Outlier filter window fill delay (samples) - run only at >= samples */
    int filterWindowSize;		/* Filter window size */
    int filterWindowType;		/* Filter window type */
    int madWindowSize;			/* MAD computation window size */
    int filterType;			/* Filter type */
    int lockedAge;			/* Maximum age in LOCKED (going into HOLDOVER) */
    int holdoverAge;			/* Maximum age in HOLDOVER (going into FREERUN) */
    int stepType;			/* Clock reaction to a 1 second+ step */
    int stepTimeout;			/* Panic mode / suspend period when step detected */
    int failureDelay;			/* Clock fault recovery countdown timer */
    int32_t offsetCorrection;		/* Offset correction / calibration value */
    uint32_t stepExitThreshold;		/* Offset from reference when we can exit panic mode early */
} ClockDriverConfig;

/* clock status commands */
enum {
    CCMD_INSYNC 	= 1 << 0,
    CCMD_LEAP_INS 	= 1 << 1,
    CCMD_LEAP_DEL 	= 1 << 2,
    CCMD_LEAP_PENDING	= 1 << 3,
    CCMD_UTC_OFFSET 	= 1 << 3
};

typedef struct {
    int cmd;
    Boolean inSync;
    Boolean leapInsert;
    Boolean leapDelete;
    int utcOffset;
} ClockStatus;

/* clock driver specification container, useful when creating clock drivers specified as string */
typedef struct {
    int type;
    char path[PATH_MAX];
    char name[CLOCKDRIVER_NAME_MAX+1];
} ClockDriverSpec;

typedef struct ClockDriver ClockDriver;

/* enTER THE C L O C K D R I V E R R R R R */
struct ClockDriver {

    int type;				/* clock driver type */
    char name[CLOCKDRIVER_NAME_MAX];	/* name of the driver's instance */
    TimeInternal age;			/* clock's age (time spent in current state) */

    int timeScale;			/* for future use: clock's native time scale */
    int utcOffset;			/* for future use: clock timescale's offset to UTC */

    int distance;			/* number of "hops" from topmost reference */

    ClockDriverConfig config;		/* config container */

    /* flags */
    Boolean	inUse;			/* the driver is in active use, if not, can be removed */
    Boolean	lockedUp;		/* clock is locked up due to failure (like negative step) */
    Boolean systemClock;		/* this driver is THE system clock */
    Boolean bestClock;			/* this driver is the current best clock */
    Boolean externalReference;		/* the clock is using an external reference */
    Boolean adevValid;			/* we have valid adev computed */

    ClockState state;			/* clock state */
    ClockState lastState;		/* previous clock state */

    char refName[CLOCKDRIVER_NAME_MAX]; /* instance name of the clock's reference */
    int refClass;			/* reference class - internal, external, PTP, etc. */
    ClockDriver *refClock;		/* reference clock object */

    TimeInternal refOffset;		/* clock's last known (filtered, accepted) offset from reference */
    TimeInternal rawOffset;		/* clock's last known raw offset from reference */

    double totalAdev;			/* Allan deviation from the driver's init time */
    double adev;			/* running Allan deviation, periodically updated */
    double minAdev;			/* minimum Allan deviation in locked state */
    double maxAdev;			/* maximum Allan deviation in locked state */
    double minAdevTotal;		/* minimum Allan deviation */
    double maxAdevTotal;		/* maximum Allan deviation */

    PIservo servo;			/* PI servo used to sync the clock */

    double lastFrequency;		/* last known frequency offset */
    double storedFrequency;		/* stored (good) frequency offset */
    double maxFrequency;		/* maximum frequency offset that can be set */


    /* BEGIN "private" fields */

    int _serial;			/* object serial no */
    Boolean	_init;			/* the driver was successfully initialised */
    ClockStatus _status;		/* status flags - leap etc. */
    Boolean _updated;			/* clock has received at least one update */
    Boolean _stepped;			/* clock has been stepped at least once */
    Boolean _locked;			/* clock has locked at least once */
    Boolean _canResume;			/* we are OK to resume sync after step */
    IntPermanentAdev _adev;		/* running Allan deviation container */
    IntPermanentAdev _totalAdev;	/* totalAllan deviation container */
    TimeInternal _initTime;		/* time (monotonic) when the driver was initialised */
    TimeInternal _lastSync;		/* time (monotonic) when the driver was last adjusted */
    TimeInternal _lastUpdate;		/* time (monotonic) when the driver received the last update */
    int _warningTimeout;		/* used to silence warnings */
    double _tau;			/* time constant (servo run interval) */
    DoubleMovingStatFilter *_filter;	/* offset filter */
    DoubleMovingStatFilter *_madFilter;	/* MAD container */
    Boolean _skipSync;			/* skip next sync */
    int *_instanceCount;		/* instance counter for the whole clock driver */
    void *_privateData;			/* implementation-specific data */
    void *_privateConfig;		/* implementation-specific config */
    void *_extData;			/* implementation-specific external / extra data */

    /* END "private" fields */

    /* inherited methods */

    void (*setState) (ClockDriver *, ClockState);
    void (*processUpdate) (ClockDriver *);

    Boolean (*pushConfig) (ClockDriver *, RunTimeOpts *);
    Boolean (*healthCheck) (ClockDriver *);

    void (*setReference) (ClockDriver *, ClockDriver *);
    void (*setExternalReference) (ClockDriver *, const char *, int);

    Boolean (*adjustFrequency) (ClockDriver *, double, double);
    void (*restoreFrequency) (ClockDriver *);
    void (*storeFrequency) (ClockDriver *);

    Boolean (*syncClock) (ClockDriver*, double);
    Boolean (*syncClockExternal) (ClockDriver*, TimeInternal, double);
    void (*putStatsLine) (ClockDriver *, char*, int);
    void (*putInfoLine) (ClockDriver *, char*, int);

    void (*touchClock) (ClockDriver *);

    /* inherited methods end */

    /* public interface - implementations must implement all of those */

    int (*shutdown) 	(ClockDriver*);
    int (*init)		(ClockDriver*, const void *);

    Boolean (*getTime) (ClockDriver*, TimeInternal *);
    Boolean (*getTimeMonotonic) (ClockDriver*, TimeInternal *);
    Boolean (*getUtcTime) (ClockDriver*, TimeInternal *);
    Boolean (*setTime) (ClockDriver*, TimeInternal *, Boolean);
    Boolean (*stepTime) (ClockDriver*, TimeInternal *, Boolean);

    Boolean (*setFrequency) (ClockDriver *, double, double);

    double (*getFrequency) (ClockDriver *);
    Boolean (*getStatus) (ClockDriver *, ClockStatus *);
    Boolean (*setStatus) (ClockDriver *, ClockStatus *);
    Boolean (*getOffsetFrom) (ClockDriver *, ClockDriver *, TimeInternal*);

    Boolean (*privateHealthCheck) (ClockDriver *); /* NEW! Now with private healthcare! */

    Boolean (*pushPrivateConfig) (ClockDriver *, RunTimeOpts *);
    Boolean (*isThisMe) (ClockDriver *, const char* search);

    /* public interface end */


    /* attach the linked list */
    LINKED_LIST_TAG(ClockDriver);

};


ClockDriver*  	createClockDriver(int driverType, const char* name);
Boolean 	setupClockDriver(ClockDriver* clockDriver, int type, const char* name);
void 		freeClockDriver(ClockDriver** clockDriver);

ClockDriver* 	getSystemClock();

void 		shutdownClockDrivers();
void		controlClockDrivers(int);

void		updateClockDrivers();

ClockDriver*	findClockDriver(const char *);
ClockDriver*	getClockDriverByName(const char *);

void		syncClocks();
void		stepClocks(Boolean);
void		reconfigureClockDrivers(RunTimeOpts *);
Boolean createClockDriversFromString(const char*, RunTimeOpts *, Boolean);

const char*	getClockStateName(ClockState);
const char*	getClockStateShortName(ClockState);
const char*	getClockDriverName(int);
int		getClockDriverType(const char*);
Boolean		parseClockDriverSpec(const char*, ClockDriverSpec *);
void		compareAllClocks();

#include "clockdriver_unix.h"
#include "clockdriver_linuxphc.h"

#endif /* PTPD_CLOCKDRIVER_H_ */
