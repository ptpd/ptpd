#ifndef TIMINGDOMAIN_H_
#define TIMINGDOMAIN_H_

/* simple compare 2, lower wins */
#define CMP2L(a,b) \
    if (a < b ) return 1;\
    if (a > b ) return -1;

/* simple compare 2, higher wins */
#define CMP2H(a,b) \
    if (a > b ) return 1;\
    if (a < b ) return -1;

/* respect are kulture ye fenion basterb */
#define		FLAGS_ARESET(var, flegs) \
((var & (flegs)) == (flegs))

#define		FLAGS_SET(var, flegs) \
(var |= flegs)

#define		FLAGS_UNSET(var, flegs) \
(var &= ~flegs)

#define TIMINGSERVICE_OPERATIONAL 	0x01 /* functioning */
#define TIMINGSERVICE_AVAILABLE 	0x02 /* ready to control the clock */
#define TIMINGSERVICE_IN_CONTROL 	0x04 /* allowed to control the clock - has control */
#define TIMINGSERVICE_IDLE 		0x08 /* not showing clock activity */
#define TIMINGSERVICE_HOLD		0x10 /* hold timer running */
#define TIMINGSERVICE_SINK_ONLY		0x20 /* only servers time */
#define TIMINGSERVICE_NO_TOD		0x40 /* does not provide time of day */

#define MAX_TIMINGSERVICES 		16
/* #define MAX_CLOCKSOURCES 		16 */

#define TIMINGSERVICE_MAX_DESC		20

enum {
    REASON_NONE,
    REASON_IDLE,
    REASON_ELECTION,
    REASON_CTRL_NOT_BEST,
    REASON_ELIGIBLE
};

typedef enum {
    TIMINGSERVICE_NONE 	= 0x00,
    TIMINGSERVICE_PTP 	= 0x10,
    TIMINGSERVICE_PPS 	= 0x20,
    TIMINGSERVICE_GPS 	= 0x30,
    TIMINGSERVICE_NTP 	= 0x40,
    TIMINGSERVICE_OTHER 	= 0xFE,
    TIMINGSERVICE_MAX	= 0xFF
}
TimingServiceType;

typedef struct {
    uint8_t priority1;
    TimingServiceType type;
    uint8_t priority2;
}
TimingServiceDS;

typedef struct TimingService TimingService;
typedef struct TimingDomain TimingDomain;

struct TimingService {
    TimingServiceDS dataSet;
    char		 id[TIMINGSERVICE_MAX_DESC+1];	/* description like PTP-eth0 */
    uint8_t		 flags;			/* see top of file */
    int		 updateInterval;/* minimum updatwe interval: some services need to be checked less frequently */
    int		 lastUpdate;	/* seconds since last update */
    int		 minIdleTime;	/* minumum and idle time. If we sync every 5 minutes, 2 minute timeout does not make much sense */
    int		 idleTime;	/* idle time - how long have we not had updated the clock */
    int		 timeout;	/* time between going idle and handing over clock control to next best */
    int		 holdTime;	/* how long to hold clock control when stopped controlling the clock */
    int		 holdTimeLeft;	/* how long to hold clock control when stopped controlling the clock */
    Boolean	 updateDue;	/* used internally by TimingDomain: should be private... */
    Boolean 	 activity;	/* periodically updated by the time service, used to detect idle state */
    Boolean	 reloadRequested; /* WHO WANTS A RELOAD!!! */
    Boolean	 restartRequested; /* Service needs restarted */
    Boolean	 released;	/* so that we don't issue messages twice */
    void	 *controller;	/* the object behind the TimingService - such as PTPClock, NTPClock etc. */
    void	 *config;	/* configuration object used by the controller - RunTimeOpts, NTPOptions etc. */
    TimingDomain *parent;	/* pointer to the TimingDomain this service belongs to */
/*	ClockSource	 clockSource */ /* initially there is only the kernel clock... */
    int		 (*init)	(TimingService *service); /* init method that the service can assign */
    int		 (*shutdown)	(TimingService *service); /* shutdown method that the service can assign */
    int		 (*acquire)	(TimingService *service); /* method called when the given service should acquire clock control */
    int		 (*release)	(TimingService *service, int reason); /* method called when the given service should release clock control */
    int		 (*update)	(TimingService *service); /* heartbeat - called to allow the service to report if it's operational */
    int		 (*clockUpdate)	(TimingService *service); /* allows the service to inform the clock source about things like sync status, UTC offset, etc */
};

struct TimingDomain {
    TimingService	*services[MAX_TIMINGSERVICES]; /* temporary. this should be dynamic in future */
    TimingService	*current; /* pointer to the currently used service - null before election */
    TimingService	*best; /* pointer to the winning service (best out of all active) */
    TimingService	*preferred; /* naturally preferred = best looking at properties only */

    /* ClockSource	*sources[MAX_CLOCKSOURCES];*/ /* eventually */
    int		serviceCount;
    /* 	TimingService	*best[MAX_CLOCKSOURCES]; */ /* eventually, best service for given clock source */

    int		updateInterval;	/* this is so TimingDomain knows how often it's being checked */
    Boolean	noneAvailable; /* used so that the no available warning is not repeated */


    /* Counters */
    int		availableCount;
    int 	operationalCount;
    int		idleCount;
    int		controlCount;

    /* used to prevent flapping: when best is released or better exists, wait first */

    /* the delay */
    int		electionDelay;
    /* the countdown counter */
    int		electionLeft;

    int		(*init)		(TimingDomain *domain); /* init method */
    int		(*shutdown)	(TimingDomain *domain); /* shutdown method */
    int		(*update)	(TimingDomain *domain); /* main update call */
};

int	timingDomainSetup(TimingDomain *domain);
int	timingServiceSetup(TimingService *service);

extern TimingDomain timingDomain;

#endif /* TIMINGDOMAIN_H_ */
