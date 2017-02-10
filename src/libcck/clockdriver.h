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
 * @file   clockdriver.h
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  structure definitions for the clock driver
 *
 */

#ifndef CCK_CLOCKDRIVER_H_
#define CCK_CLOCKDRIVER_H_

#include <sys/param.h>

#include "linked_list.h"
#include "piservo.h"

#include <libcck/cck.h>
#include <libcck/statistics.h>
#include <libcck/cck_types.h>

#define CLOCKDRIVER_UPDATE_INTERVAL 1
#define CLOCKDRIVER_CCK_WARNING_TIMEOUT 60
#define CLOCKDRIVER_SYNC_RATE 5
#define SYSTEM_CLOCK_NAME "syst"
#define CLOCKDRIVER_FREQFILE_PREFIX "clock"

/* clock driver types */
enum {

#define CCK_REGISTER_IMPL(typeenum, typesuffix, textname) \
    typeenum,

#include "clockdriver.def"

    CLOCKDRIVER_MAX
};


#define SYSTEM_CLOCK_TYPE CLOCKDRIVER_UNIX

/* clock group flag definitions */
#define CD_GROUP1	1 << 0
#define CD_GROUP2	1 << 1
#define CD_GROUP3	1 << 2
#define CD_GROUP4	1 << 3
#define CD_GROUP5	1 << 4
#define CD_GROUP6	1 << 5
#define CD_GROUP7	1 << 6
#define CD_GROUP8	1 << 7

#define CD_DEFAULTGROUP CD_GROUP1


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

/* reference class, in order from least to most preferred */
enum {
    RC_NONE	= 0, /* because on init it is zero */
    RC_INTERNAL = 1, /* internal clock driver */
    RC_EXTERNAL = 2, /* arbitrary external reference */
    RC_PTP	= 3
};

/* clock states */
typedef enum {
    CS_SUSPENDED,	/* clock is suspended - no updates accepted */
    CS_NEGSTEP,		/* clock is locked up because of negative offset (< -1 s) - requires manual step with SIGUSR1 */
    CS_STEP,		/* clock is suspended because of offset > 1 s, counting down to step */
    CS_HWFAULT,		/* hardware fault */
    CS_INIT,		/* initial state, proceeds to FREERUN */
    CS_FREERUN,		/* clock is not being disciplined - or has a reference but has not been updated */
    CS_FREQEST,	/* clock has (re)gained a reference and is preparing for sync */
    CS_TRACKING,	/* clock is tracking a reference but is not (yet) locked, or was locked and adev is outside threshold. */
    CS_HOLDOVER,	/* clock is in holdover: was LOCKED but has been idle (no updates) or lost its reference */
    CS_LOCKED		/* clock is locked to reference (adev below threshold) */

    /* Note: only clocks in LOCKED or HOLDOVER state are considered for best clock selection */

} ClockState;

/* state change reason codes */
enum {
    CSR_INTERNAL,	/* internal FSM work: init, shutdown, etc */
    CSR_OFFSET,		/* state change because of reference offset value */
    CSR_IDLE,		/* clock was idle */
    CSR_AGE,		/* state age: holdover->freerun, etc. */
    CSR_REFCHANGE,	/* reference */
    CSR_FAULT		/* fault */
};

/* clock driver configuration */
typedef struct {
    bool disabled;			/* clock is disabled - it's skipped from sync and all other operation.
					 * if it is being used by PTP, it is set read-only instead */

    bool required;			/* clock cannot be disabled */
    bool excluded;			/* clock is excluded from best clock selection */
    bool readOnly;			/* clock is read-only */
    bool externalOnly;			/* this clock will only accept an external reference */
    bool internalOnly;			/* this clock will only accept an internal reference */
    bool noStep;			/* clock cannot be stepped */
    bool negativeStep;			/* clock can be stepped backwards */
    bool storeToFile;			/* clock stores good frequency in a file */
    bool outlierFilter;			/* enable MAD-based outlier filter */
    bool statFilter;			/* enable statistical filter */
    bool strictSync;			/* enforce sync to LOCKED and HOLDOVER only */

    uint8_t groupFlags;			/*
					 * todo: sync groups:
					 * Clock may be in up to 8 groups,
					 * but best clock selection runs only within the same group,
					 * so we can partition clock sync: say, all externally-disciplined
					 * clocks and system clock are in group 1 (default),
					 * all other clocks are in group 2. Effect: system clock syncs
					 * with one of the externally-disciplined clocks, all other clocks
					 * sync with the system clock.
					 */

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
    int calibrationTime;		/* Frequency estimation time */
    int failureDelay;			/* Clock fault recovery countdown timer */
    int minStep;			/* Minimum delta a clock can be stepped by */
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
    bool inSync;
    bool leapInsert;
    bool leapDelete;
    int utcOffset;
} ClockStatus;

/* clock driver specification container, useful when creating clock drivers specified as string */
typedef struct {
    int type;
    char path[PATH_MAX];
    char name[CCK_COMPONENT_NAME_MAX+1];
} ClockDriverSpec;

typedef struct ClockDriver ClockDriver;

/* enTER THE C L O C K D R I V E R R R R R */
struct ClockDriver {

    int type;				/* clock driver type */
    char name[CCK_COMPONENT_NAME_MAX];	/* name of the driver's instance */
    CckTimestamp age;			/* clock's age (time spent in current state) */

    int timeScale;			/* for future use: clock's native time scale */
    int utcOffset;			/* for future use: clock timescale's offset to UTC */

    int distance;			/* number of "hops" from topmost reference */

    void *owner;			/* pointer to user structure owning or controlling this clock */

    ClockDriverConfig config;		/* config container */

    /* flags */
    bool	inUse;			/* the driver is in active use, if not, can be removed */
    bool	lockedUp;		/* clock is locked up due to failure (like negative step) */
    bool systemClock;		/* this driver is THE system clock */
    bool bestClock;			/* this driver is the current best clock */
    bool externalReference;		/* the clock is using an external reference */
    bool adevValid;			/* we have valid adev computed */

    ClockState state;			/* clock state */
    ClockState lastState;		/* previous clock state */

    char refName[CCK_COMPONENT_NAME_MAX]; /* instance name of the clock's reference */
    int refClass;			/* reference class - internal, external, PTP, etc. */
    int lastRefClass;			/* previous reference class (when we lost reference) */
    ClockDriver *refClock;		/* reference clock object */

    CckTimestamp refOffset;		/* clock's last known (filtered, accepted) offset from reference */
    CckTimestamp rawOffset;		/* clock's last known raw offset from reference */

    double totalAdev;			/* Allan deviation from the driver's init time */
    double adev;			/* running Allan deviation, periodically updated */
    double minAdev;			/* minimum Allan deviation in locked state */
    double maxAdev;			/* maximum Allan deviation in locked state */
    double minAdevTotal;		/* minimum Allan deviation from start of measurement */
    double maxAdevTotal;		/* maximum Allan deviation from start of measurement */

    PIservo servo;			/* PI servo used to sync the clock */

    double estimatedFrequency;		/* frequency estimated during calibration stage */
    double lastFrequency;		/* last known frequency offset */
    double storedFrequency;		/* stored (good) frequency offset */
    double maxFrequency;		/* maximum frequency offset that can be set */

    /* callbacks */
    struct {
	void (*onStep) (void *owner);	/* user callback to be called after clock was stepped */
	void (*onUpdate) (void *owner); /* user callback to be called on periodic update (not on clock sync) */
	void (*onSync) (void *owner);	/* user callback to be called after clock is synced */
	void (*onLock) (void *owner, bool locked);
    } callbacks;

    /* BEGIN "private" fields */

    int _serial;			/* object serial no */
    unsigned char _uid[8];		/* 64-bit uid (like MAC) */
    bool	_init;			/* the driver was successfully initialised */
    ClockStatus _status;		/* status flags - leap etc. */
    bool _updated;			/* clock has received at least one update */
    bool _stepped;			/* clock has been stepped at least once */
    bool _locked;			/* clock has locked at least once */
    bool _canResume;			/* we are OK to resume sync after step */
    bool _waitForElection;		/* do not sync to- or from- until best clock election runs next */
    IntPermanentAdev _adev;		/* running Allan deviation container */
    IntPermanentAdev _totalAdev;	/* totalAllan deviation container */
    CckTimestamp _lastOffset;		/* access to last offset for filters */
    CckTimestamp _initTime;		/* time (monotonic) when the driver was initialised */
    CckTimestamp _lastSync;		/* time (monotonic) when the driver was last adjusted */
    CckTimestamp _lastUpdate;		/* time (monotonic) when the driver received the last update */
    int _warningTimeout;		/* used to silence warnings */
    double _tau;			/* time constant (servo run interval) */
    DoubleMovingStatFilter *_filter;	/* offset filter */
    DoubleMovingStatFilter *_madFilter;	/* MAD container */
    CckTimestamp _lastDelta;			/* last offset used during frequency estimation */
    DoublePermanentMean _calMean;	/* mean contained used during frequency estimation */
    bool _skipSync;			/* skip next sync */
    int *_instanceCount;		/* instance counter for the whole component */

    /* libCCK common fields - to be included in a general object header struct */
    void *_privateData;			/* implementation-specific data */
    void *_privateConfig;		/* implementation-specific config */
    void *_extData;			/* implementation-specific external / extension / extra data */

    /* END "private" fields */

    /* inherited methods */

    void (*setState) (ClockDriver *, ClockState);
    void (*processUpdate) (ClockDriver *);

    bool (*healthCheck) (ClockDriver *);

    void (*setReference) (ClockDriver *, ClockDriver *);
    void (*setExternalReference) (ClockDriver *, const char *, int);

    bool (*stepTime) (ClockDriver*, CckTimestamp *, bool);
    bool (*adjustFrequency) (ClockDriver *, double, double);
    void (*restoreFrequency) (ClockDriver *);
    void (*storeFrequency) (ClockDriver *);

    bool (*syncClock) (ClockDriver*, double);
    bool (*syncClockExternal) (ClockDriver*, CckTimestamp, double);
    void (*putStatsLine) (ClockDriver *, char*, int);
    void (*putInfoLine) (ClockDriver *, char*, int);

    void (*touchClock) (ClockDriver *);

    /* inherited methods end */

    /* public interface - implementations must implement all of those */

    int (*shutdown) 	(ClockDriver*);
    int (*init)		(ClockDriver*, const void *);

    bool (*getTime) (ClockDriver*, CckTimestamp *);
    bool (*getTimeMonotonic) (ClockDriver*, CckTimestamp *);
    bool (*getUtcTime) (ClockDriver*, CckTimestamp *);
    bool (*setTime) (ClockDriver*, CckTimestamp *);
    bool (*setOffset) (ClockDriver*, CckTimestamp *);

    bool (*setFrequency) (ClockDriver *, double, double);

    double (*getFrequency) (ClockDriver *);
    bool (*getStatus) (ClockDriver *, ClockStatus *);
    bool (*setStatus) (ClockDriver *, ClockStatus *);
    bool (*getOffsetFrom) (ClockDriver *, ClockDriver *, CckTimestamp*);
    bool (*getSystemClockOffset) (ClockDriver *, CckTimestamp*);

    bool (*privateHealthCheck) (ClockDriver *); /* NEW! Now with private healthcare! */

//    bool (*pushPrivateConfig) (ClockDriver *, void *);
    bool (*isThisMe) (ClockDriver *, const char* search);

    void (*loadVendorExt) (ClockDriver *);

    /* public interface end */

    /* vendor-specific init and shutdown callbacks */

    int (*_vendorInit) (ClockDriver *);
    int (*_vendorShutdown) (ClockDriver *);
    int (*_vendorHealthCheck) (ClockDriver *);

    /* attach the linked list */
    LL_MEMBER(ClockDriver);

};


ClockDriver*  	createClockDriver(int driverType, const char* name);
bool 	setupClockDriver(ClockDriver* clockDriver, int type, const char* name);
void 		freeClockDriver(ClockDriver** clockDriver);
int		clockDriverDummyCallback(ClockDriver*);
void		cdDummyOwnerCallback(void *owner);

ClockDriver* 	getSystemClock();

void 		shutdownClockDrivers();
void		controlClockDrivers(int);

void		updateClockDrivers(int);

ClockDriver*	findClockDriver(const char *);
ClockDriver*	getClockDriverByName(const char *);

void		syncClocks();
void		stepClocks(bool);
void 		configureClockDriverFilters(ClockDriver *driver);
void 		reconfigureClockDrivers(bool (*pushConfig)(ClockDriver*, const void*), const void*);
bool createClockDriversFromString(const char* list, bool (*pushConfig) (ClockDriver *, const void*), const void *config, bool quiet);

const char*	getClockStateName(ClockState);
const char*	getClockStateShortName(ClockState);
const char*	getClockDriverTypeName(int);
int		getClockDriverType(const char*);
bool		parseClockDriverSpec(const char*, ClockDriverSpec *);
void		compareAllClocks();

/* invoking this without CCK_REGISTER_IMPL defined, includes the implementation headers */
#include "clockdriver.def"

#endif /* CCK_CLOCKDRIVER_H_ */
