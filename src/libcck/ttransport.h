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
 * @file   ttransport.h
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  structure definitions for the timestamping transport
 *
 */

#ifndef CCK_TTRANSPORT_H_
#define CCK_TTRANSPORT_H_

#include <libcck/cck.h>
#include <libcck/linked_list.h>
#include <libcck/cck_types.h>
#include <libcck/transport_address.h>
#include <libcck/fd_set.h>
#include <libcck/clockdriver.h>
#include <libcck/acl.h>

/*
 * timestamping transport capabilities / flags. This works both ways:
 * transport type registration presents all supported flags, and user requests
 * mandatory support of some of them, (config.flags), finally (_probeTTransport),
 * the transport reports which flags it can support for the given path,
 * path being an interface name or filename - the general transport API
 * has no notion of an interface - only implementations do.
 */

#define TT_CAPS_NONE			0x000	/* no capabilities... */
#define TT_CAPS_UCAST			0x001	/* unicast capable */
#define TT_CAPS_MCAST			0x002	/* multicast capable */
#define TT_CAPS_LOCAL			0x004	/* local path - unix domain socket etc. */
#define TT_CAPS_SW_TIMESTAMP		0x008	/* software timestamps */
#define TT_CAPS_HW_TIMESTAMP_PTP	0x010	/* hardware timestamps for PTP packets */
#define TT_CAPS_HW_TIMESTAMP_ANY	0x020	/* hardware timestamps for all packets */
#define TT_CAPS_HW_PTP_ONE_STEP		0x040	/* PTP one-step transmission support */
#define TT_CAPS_PREFER_SW		0x080	/* prefer software timestamps over hardware */
#define TT_CAPS_BROKEN			0x100	/* transport marked as broken */

/* capability helper macros */
#define TT_UC(var) (var & TT_CAPS_UCAST)
#define TT_MC(var) (var & TT_CAPS_MCAST)
#define TT_UC_MC(var) ( TT_MC(var) && TT_UC(var) )
#define TT_UC_ONLY(var) ( (var & TT_CAPS_UCAST) && !(var & TT_CAPS_MCAST) )
#define TT_MC_ONLY(var) ( (var & TT_CAPS_MCAST) && !(var & TT_CAPS_UCAST) )

/* transport header lengths: magic numbers, but it's not that they are planning to change... */
#define TT_HDRLEN_ETHERNET	14			/* 6 src + 6d st + 2 ethertype */
#define TT_HDRLEN_UDPV4		42			/* 14 eth + 20 IP + 8 UDP */
#define TT_HDRLEN_UDPV6		62			/* 14 eth + 20 IP6 + 8 UDP */

/* default monitor + rate update interval */
#define TT_MONITOR_INTERVAL 1.5

/* default network fault clear timeout */
#define TT_FAULT_TIMEOUT 10

/* how many packets to skip after a topology change or restart */
#define TT_CHANGE_SKIP_PACKETS 3

/* timestamping transport driver types - automagic */
enum {

    TT_TYPE_NONE = 0,

#define CCK_ALL_IMPL /* invoke all implementations, even those we are not building */
#define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, addressfamily, capabilities, extends) \
    typeenum,
#include "ttransport.def"

    TT_TYPE_MAX
};

/* address family types - only the ones we have implemented transports for */
static const int transportTypeFamilies[] = {
    #define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, addressfamily, capabilities, extends) \
	[typeenum] = addressfamily,
    #include "ttransport.def"
	[TT_FAMILY_MAX] = AF_UNSPEC
};

/* commands that can be sent to all transports */
enum {
    TT_NOTINUSE,	/* mark all not in use */
    TT_SHUTDOWN,	/* shutdown all */
    TT_CLEANUP,		/* clean up transports not in use */
    TT_DUMP,		/* dump transport information  */
    TT_MONITOR,		/* monitor for changes */
    TT_REFRESH,		/* any periodic actions such as multicast join */
    TT_CLEARCOUNTERS,	/* clear counters */
    TT_DUMPCOUNTERS,	/* dump counters */
    TT_UPDATECOUNTERS	/* update rate counters */
};

/* common transport configuration */
typedef struct {
    int _type;				/* transport type this config describes */
    int _privateSize;			/* size of private config structure */
    void *_privateConfig;		/* private configuration handle */
    uint32_t flags;			/* capability flags */
    bool disabled;			/* transport is disabled */
    bool discarding;			/* transport is discarding data */
    bool timestamping;			/* transport needs to deliver timestamps */
    bool uidPid;			/* use process ID in transport UID */
    bool unmonitored;			/* transport is not monitored for network changes */
    char customUid[8];			/* use custom UID */
    int rxLatency;			/* ingress latency / timestamp correction */
    int txLatency;			/* egress latency / timestamp correction */
} TTransportConfig;

/* transport specification container, useful when creating transports specified as string */
typedef struct {
    int type;
    char name[CCK_COMPONENT_NAME_MAX+1];
    char path[PATH_MAX];
} TTransportSpec;

/* Network message: data plus basic information about the data */
typedef struct {
	char *data;				/* data buffer */
	CckTransportAddress *destination;	/* destination to send the message to */
	size_t length;				/* data size */
	int capacity;				/* buffer size */
	bool fromSelf;				/* message is from own address */
	bool toSelf;				/* message is to own address */
	bool isMulticast;			/* message is multicast */
	bool hasTimestamp;			/* message timestamp is valid */
	CckTransportAddress from;		/* source of received message */
	CckTransportAddress to;			/* destination of received message */
	CckTimestamp timestamp;			/* RX timestamp on receipt, TX timestamp after transmission */
	int _family;				/* address family this message was received with */
	int _flags;				/* any internal flags a transport implementation may want to use */
} TTransportMessage;

/* transport counters */
typedef struct {
	int 		 rateInterval;		/* rate computation interval, seconds */
	bool		 updated;		/* rate snapshots ready */
	uint32_t 	 rxMsg;			/* total messages received */
	uint32_t 	_snapRxMsg;		/* snapshot for rate computation */
	uint32_t 	 txMsg;			/* total messages transmitted */
	uint32_t 	_snapTxMsg;		/* snapshot for rate computation */
	uint32_t 	 rxBytes;		/* total bytes received */
	uint32_t 	_snapRxBytes;		/* snapshot for rate computation */
	uint32_t 	 txBytes;		/* total bytes transmitted */
	uint32_t 	_snapTxBytes;		/* total bytes transmitted */
	uint32_t 	 rxRateMsg;		/* receive rate, messages/s */
	uint32_t 	 txRateMsg;		/* transmit rate, messages/s */
	uint32_t 	 rxRateBytes;		/* receive rate, bytes/s */
	uint32_t 	 txRateBytes;		/* transmit rate, bytes/s */
	uint32_t 	 rxErrors;		/* receive errors */
	uint32_t 	 txErrors;		/* transmit errors */
	uint32_t	 rxTimestampErrors;	/* receive timestamp errors */
	uint32_t	 txTimestampErrors;	/* transmit timestamp errors */
	uint32_t	 rxDiscards;		/* discarded messages: blocked by callbacks, etc. */
	uint32_t	 txDiscards;		/* discarded messages: blocked by callbacks, etc. */
} TTransportCounters;


typedef struct TTransport TTransport;

/* the transport */
struct TTransport {

    int type;				/* transport implementation type */
    int family;				/* address family type */
    int headerLen;			/* protocol header length before payload */

    char name[CCK_COMPONENT_NAME_MAX +1];	/* instance name */

    void *owner;			/* pointer to user structure owning or controlling this component */

    CckAddressToolset *tools;		/* address management function and constant holder */

    CckFdSet *fdSet;			/* file descriptor set watching this transport */

    CckFd myFd;				/* file descriptor wrapper */

    CckAcl *dataAcl;			/* data ACL */
    CckAcl *controlAcl;			/* control / management ACL */

    unsigned char uid[8];		/* 64-bit EUID (mm aa cc ff fe mm aa cc) - IP address used when no MAC */

    CckTransportAddress ownAddress;	/* my own protocol address */
    CckTransportAddress hwAddress;	/* hardware address, if available */

    TTransportConfig config;		/* config container */

    TTransportCounters counters;	/* counter container */

    char	*inputBuffer;		/* data receive buffer, supplied by user via setBuffers()*/
    int		inputBufferSize;

    char	*outputBuffer;		/* data transmit buffer, supplied by user via setBuffers() */
    int		outputBufferSize;

    TTransportMessage incomingMessage;	/* incoming message with associated info */
    TTransportMessage outgoingMessage;	/* outgoing message with associated destination etc. */

    /* user-supplied callbacks */
    struct {
	/* informs the owner if we are sending a multicast or unicast message, allows it to modify data accordingly */
	int (*preTx) (void *transport, void *owner, char *data, const size_t len, bool isMulticast);
	/* allows the owner to verify if data transmitted matches data attached to TX timestamp */
	int (*matchData) (void *transport, void *owner, char *a, const size_t alen, char *b, const size_t blen);
	/* checks if the data received is regular data or control / management data - used to select ACL being matched */
	int(*isRegularData) (void *transport, void *owner, char *data, const size_t len, bool isMulticast);
	/* informs the owner that we have had a network interface / topology change. */
	void (*onNetworkChange) (void *transport, void *owner, const bool major);
	/* informs the owner that we have had a network fault - called on fault and on recovery */
	void (*onNetworkFault) (void *transport, void *owner, const bool fault);
	/* informs the owner that the clock driver provided by this transport may have changed */
	int (*onClockDriverChange) (void *transport, void *owner);
	/* allows the owner to react to message rate update */
	void (*onRateUpdate) (void *transport, void *owner);
    } callbacks;

    /* flags */
    bool inUse;				/* transport is in active use, if not, can be removed */
    bool hasData;			/* transport has some data to read */
    bool fault;				/* transport is in faulty state */

    /* BEGIN "private" fields */

    int _serial;			/* object serial no */
    int	_init;				/* the driver was successfully initialised */
    int *_instanceCount;		/* instance counter for the whole component */
    TTransport *slaveTransport;		/* an associated transport, restarted along with this one */
    int _skipMessages;			/* dump n next messages (say, after a topology change) */

    /* libCCK common fields - to be included in a general object header struct */
    void *_privateData;			/* implementation-specific data */
    void *_privateConfig;		/* implementation-specific config */
    void *_extData;			/* implementation-specific external / extension / extra data */

    int _faultTimeout;			/* network fault timeout countdown */

    /* END "private" fields */

    /* inherited methods */

    /* restart: shutdown + init */
    int (*restart)(TTransport *);
    /* clear counters */
    void (*clearCounters)(TTransport *);
    /* dumpCounters */
    void (*dumpCounters)(TTransport *);
    /* update rates */
    void (*updateCounterRates)(TTransport *, const int interval);
    /* set up data buffers */
    void (*setBuffers) (TTransport *, char *iBuf, const int iBufSize, char *oBuf, const int oBufSize);
    /* magic cure for hypohondria: imaginary health-check */
    bool (*healthCheck) (TTransport *);
    /* send n bytes from outputBuffer / outgoingMessage to destination,  */
    ssize_t (*sendTo) (TTransport*, const size_t len, CckTransportAddress *to);
    /* send n bytes from buf to destination */
    ssize_t (*sendDataTo) (TTransport*, char *buf, const size_t len, CckTransportAddress *to);
    /* receive data into inputBuffer and incomingMessage */
    ssize_t (*receive) (TTransport*);
    /* receive data into provided buffer of size len */
    ssize_t (*receiveData) (TTransport*, char* buf, const size_t size);
    /* check address against ACLs */
    bool (*addressPermitted) (TTransport *, CckTransportAddress *address, bool isRegular);
    /* check message against ACLs */
    bool (*messagePermitted) (TTransport *, TTransportMessage *message);

    /* inherited methods end */

    /* public interface - implementations must implement all of those */

    /* init, shutdown */
    int (*init)		(TTransport*, const TTransportConfig *config, CckFdSet *fdSet);
    int (*shutdown) 	(TTransport*);

    /* check if this transport recognises itself as matching the search string (interface, port, etc.) */
    bool (*isThisMe) (TTransport *, const char* search);
    /* store and apply any configuration that can be applied in runtime, inform if restart needed */
//    bool (*applyConfig) (TTransport*, const TTransportConfig *config);
    /* test the required configuration */
    bool (*testConfig) (TTransport*, const TTransportConfig *config);
    /* NEW! Now with private healthcare! */
    bool (*privateHealthCheck) (TTransport *);
    /* send message: buffer and destination information; populate TX timestamp if we have one */
    ssize_t (*sendMessage) (TTransport * , TTransportMessage *message);
    /* receive message: buffer and source + destination information; populate RX timestamp if we have one */
    ssize_t (*receiveMessage) (TTransport * , TTransportMessage *message);
    /* get an instnce of the clock driver generating this clock's timestamps, create if not existing */
    ClockDriver* (*getClockDriver) (TTransport *);
    /* perform any periodic checks - may result in onNetworkChange callback */
    int (*monitor) (TTransport *, const int interval, const bool quiet);
    /* perform any refresh actions - mcast joins, etc. */
    int (*refresh) (TTransport *);
    /* load any vendor extensions */
    void (*loadVendorExt) (TTransport *);
    /* short transport information line */
    char* (*getInfoLine) (TTransport *, char *buf, size_t len);
    /* short transport operational status */
    char* (*getStatusLine) (TTransport *, char *buf, size_t len);

    /* public interface end */

    /* vendor-specific init and shutdown callbacks */

    int (*_vendorInit) (TTransport *);
    int (*_vendorShutdown) (TTransport *);
    int (*_vendorHealthCheck) (TTransport *);

    /* attach the linked list */
    LL_MEMBER(TTransport);

};

TTransport*	createTTransport(int type, const char* name);
bool 		setupTTransport(TTransport* transportr, int type, const char* name);
void 		freeTTransport(TTransport** transport);

/*
 * find the "best" working transport implementation for a given path and probe if it is operational.
 * if @specific is non-zero (not TT_TYPE_NONE), only probe this implementation.
 */
int		detectTTransport(const int family, const char *path, const int caps, const int specific);

TTransportConfig*	createTTransportConfig(const int type);
void			freeTTransportConfig(TTransportConfig **config);

/* dummy general-purpose callback */
int		ttDummyCallback(TTransport *transport);
/* dummy general-purpose callback for use by the owner - no libCCK-specific datatypes */
int		ttDummyOwnerCallback(void *owner, void *transport);
/* dummy callback with a data buffer */
int		ttDummyDataCallback (void *owner, void *transport, char *data, const size_t len, bool isMulticast);
/* dummy callback with two data buffers - say to match on something */
int		ttDummyMatchCallback (void *owner, void *transport, char *a, const size_t alen, char *b, const size_t blen);

void		controlTTransports(const int, const void*);
void 		shutdownTTransports();
void		monitorTTransports(const int interval);
void		monitorTTransport(TTransport *transport, const int interval);
void		refreshTTransports();

TTransport*	findTTransport(const char *);
TTransport*	getTTransportByName(const char *);

const char*	getTTransportTypeName(int);
int		getTTransportType(const char*);
int		getTTransportTypeFamily(const int);
bool		parseTTransportSpec(const char*, TTransportSpec *);

bool		copyTTransportConfig(TTransportConfig *to, const TTransportConfig *from);
TTransportConfig *duplicateTTransportConfig(const TTransportConfig *from);

void setTTransportUid(TTransport *transport);

void		clearTTransportMessage(TTransportMessage *message);
void		dumpTTransportMessage(const TTransportMessage *message);

/* invoking this without CCK_REGISTER_IMPL defined, includes the implementation headers - like here*/
#include "ttransport.def"

/* bag to hold any possible transport config type, so we don't have to dynamically allocate in some cases */
typedef union {

#define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, addressfamily, capabilities, extends) \
    TTransportConfig_##typesuffix typesuffix;

#include "ttransport.def"

} TTransportPrivateConfig;

#endif /* CCK_TTRANSPORT_H_ */
