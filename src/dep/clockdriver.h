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
#define CLOCK_SYNC_RATE 4
#define SYSTEM_CLOCK_NAME "syst"

#if defined(linux) && !defined(ADJ_SETOFFSET)
#define ADJ_SETOFFSET 0x0100
#endif


enum {
    CLOCKDRIVER_UNIX = 0,
    CLOCKDRIVER_LINUXPHC = 1
};

enum {
    CD_NOTINUSE,
    CD_SHUTDOWN,
    CD_CLEANUP,
    CD_DUMP
};

enum {
    CSTEP_NEVER,
    CSTEP_ALWAYS,
    CSTEP_STARTUP,
    CSTEP_STARTUP_FORCE
};

typedef enum {
    CS_NSTEP,
    CS_SUSPENDED,
    CS_INIT,
    CS_FREERUN,
    CS_TRACKING,
    CS_HOLDOVER,
    CS_LOCKED
} ClockState;

typedef struct {
    Boolean readOnly;
    Boolean noStep;
    Boolean negativeStep;
    Boolean storeToFile;
    char frequencyFile[PATH_MAX +1];
    char frequencyDir[PATH_MAX + 1];
    int adevPeriod;
    double stableAdev;
    double unstableAdev;
    int lockedAge;
    int holdoverAge;
    int freerunAge;
    int stepType;
    int suspendTimeout;
    uint32_t suspendExitThreshold;
} ClockDriverConfig;

typedef struct {

} ClockStatus;

typedef struct ClockDriver ClockDriver;

struct ClockDriver {

    ClockStatus _status;
    Boolean	_init;

    int _serial;
    Boolean _updated;
    Boolean _stepped;
    Boolean _canResume;
    IntPermanentAdev _adev;
    IntPermanentAdev _totalAdev;
    TimeInternal _initTime;
    TimeInternal _lastSync;
    TimeInternal _lastUpdate;
    int _warningTimeout;
    double _tau;

    PIservo servo;

    Boolean	lockedUp;
    Boolean	inUse;
    TimeInternal age;

    Boolean externalReference;

    char refName[CLOCKDRIVER_NAME_MAX];
    ClockDriver *refClock;
    TimeInternal refOffset;

    ClockState lastState;
    ClockState state;

    int type;
    char name[CLOCKDRIVER_NAME_MAX];

    int timeScale;
    int utcOffset;

    double totalAdev;
    double adev;

    Boolean systemClock;
    Boolean bestClock;

    ClockDriverConfig config;

    void *privateData;
    void *privateConfig;

    double lastFrequency;
    double storedFrequency;
    double maxFrequency;

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


    Boolean (*pushPrivateConfig) (ClockDriver *, RunTimeOpts *);
    Boolean (*isThisMe) (ClockDriver *, const char* search);

    /* public interface end */

    /* inherited methods, provided by ClockDriver */
    void (*setState) (ClockDriver *, ClockState);
    void (*processUpdate) (ClockDriver *);

    Boolean (*pushConfig) (ClockDriver *, RunTimeOpts *);

    void (*setReference) (ClockDriver *, ClockDriver *);
    void (*setExternalReference) (ClockDriver *, const char *);

    Boolean (*adjustFrequency) (ClockDriver *, double, double);
    void (*restoreFrequency) (ClockDriver *);
    void (*storeFrequency) (ClockDriver *);

    Boolean (*syncClock) (ClockDriver*, double);
    Boolean (*syncClockExternal) (ClockDriver*, TimeInternal, double);
    void (*putStatsLine) (ClockDriver *, char*, int);
    void (*putInfoLine) (ClockDriver *, char*, int);

    void (*touchClock) (ClockDriver *);

    /* inherited methods end */

    LINKED_LIST_TAG(ClockDriver);

};


ClockDriver*  	createClockDriver(int driverType, const char* name);
Boolean 	setupClockDriver(ClockDriver* clockDriver, int type, const char* name);
void 		freeClockDriver(ClockDriver** clockDriver);

ClockDriver* 	getSystemClock();

void 		shutdownClockDrivers();
void		controlClockDrivers(int);

void		updateClockDrivers();

ClockDriver*	findClockDriver(char *);
ClockDriver*	getClockDriverByName(const char *);

void		syncClocks();
void		stepClocks(Boolean);
void		reconfigureClockDrivers(RunTimeOpts *);

const char*	getClockStateName(ClockState);
const char*	getClockStateShortName(ClockState);

#include "clockdriver_unix.h"
#include "clockdriver_linuxphc.h"

#endif /* PTPD_CLOCKDRIVER_H_ */
