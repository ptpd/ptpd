/* Copyright (c) 2016 Wojciech Owczarek,
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

#include
#include "../ptpd.h"
#include "../globalconfig.h"
#include <libcck/linkedlist.h>
#include <libcck/cck_types.h>

#define TTRANSPORT_NAME_MAX 20

/* timestamping transport capabilities */
#define TT_CAPS_NONE			0x00	/* no capabilities... */
#define TT_CAPS_UCAST			0x01	/* unicast capable */
#define TT_CAPS_MCAST			0x02	/* multicast capable */
#define TT_CAPS_LOCAL			0x04	/* local path - unix domain socket etc. */
#define TT_CAPS_SW_TIMESTAMP		0x08	/* software timestamps */
#define TT_CAPS_HW_TIMESTAMP		0x10	/* hardware timestamps */
#define TT_CAPS_HW_TIMESTAMP_ALL	0x20	/* hardware timestamps for all packets */
#define TT_CAPS_HW_PTP_ONE_STEP		0x40	/* PTP one-step transmission support */


/* timestamping transport driver types */
enum {

    TT_TYPE_NONE = 0,

#define REGISTER_TTRANSPORT(fulltype, shorttype, textname, family, capabilities, extends) \
    fulltype,

#include "ttransport.def"

    TT_TYPE_MAX
};

/* bag to hold any possible transport config type, so we don't have to dynamically allocate in some cases */
typedef union {

#define REGISTER_TTRANSPORT(fulltype, shorttype, textname, family, capabilities, extends) \
    TTransportConfig_##fulltype shorttype;

#include "ttransport.def"

} TTransportConfigHolder;


/* commands that can be sent to all transports */
enum {
    TT_NOTINUSE,	/* mark all not in use */
    TT_SHUTDOWN,	/* shutdown all */
    TT_CLEANUP,		/* clean up transports not in use */
    TT_DUMP,		/* dump transport information  */
    TT_MONITOR,		/* monitor for changes */
    TT_REFRESH		/* any periodic actions such as multicast join */
};

/* clock driver configuration */
typedef struct {
    CckBool disabled;			/* transport is disabled */
    CckBool discarding;			/* transport is discarding data */
} TTransportConfig;

/* transport specification container, useful when creating transports specified as string */
typedef struct {
    int type;
    char path[PATH_MAX];
    char name[TTRANSPORT_NAME_MAX+1];
} TTransportSpec;

typedef struct TTransport TTransport;

struct TTransport {

    int type;				/* transport type */
    int family				/* address family type */
    char name[TTRANSPORT_NAME_MAX +1];	/* name of the driver's instance */

    TTransportConfig config;		/* config container */

    struct {
	int 		 rateInterval;		/* rate computation interval, seconds */
	CckU32 	 rxMsg;			/* total messages received */
	CckU32 	_lastRxMsg;		/* snapshot for rate computation */
	CckU32 	 txMsg;			/* total messages transmitted */
	CckU32 	_lastTxMsg;		/* snapshot for rate computation */
	CckU32 	 rxBytes;		/* total bytes received	*/
	CckU32 	_lastRxBytes;		/* snapshot for rate computation */
	CckU32 	 txBytes;		/* total bytes transmitted */
	CckU32 	_lastTxBytes;		/* total bytes transmitted */
	CckU32 	 rxRateMsg;		/* receive rate, messages/s */
	CckU32 	 txRateMsg;		/* transmit rate, messages/s */
	CckU32 	 rxRateBytes;		/* receive rate, bytes/s */
	CckU32 	 txRateBytes;		/* transmit rate, bytes/s */
	CckU32 	 rxErrors;		/* receive errors */
	CckU32 	 txErrors;		/* transmit errors */
	CckU32	 rxTimestampErrors;	/* receive timestamp errors */
	CckU32	 txTimestampErrors;	/* transmit timestamp errors */
    } counters;

    CckFd fd;				/* file descriptor wrapper */
    CckFdSet *fdSet;			/* file descriptor set watching this transport */

    CckTransportAddress ownAddress;

    CckOctet	*inputBuffer;		/* data read buffer, supplied by user */
    int		inputBufferSize;

    CckOctet	*outputBuffer;		/* data transmit buffer, supplied by user */
    int		outputBufferSize;

    /* flags */

    /* BEGIN "private" fields */

    int _serial;			/* object serial no */
    unsigned char _uid[8];		/* 64-bit uid (like MAC) */
    int	_init;			/* the driver was successfully initialised */
    void *_privateData;			/* implementation-specific data */
    void *_privateConfig;		/* implementation-specific config */
    void *_extData;			/* implementation-specific external / extension / extra data */

    /* END "private" fields */

    /* inherited methods */

    CckBool (*pushConfig) (TTransport *, RunTimeOpts *);
    CckBool (*healthCheck) (TTransport *);
    CckBool (*probe)	(TTransport*, const char *path, const int capabilities); /* check if given transport provides all required capabilities */
    CckBool (*isAddressMulticast) (const CckTransportAddress *address);
    CckBool (*isAddressEmpty) (const CckTransportAddress *address);
    CckBool (*addressEqual) (const CckTransportAddress *address);
    int (*cmpAddress) (const void *a, const void *b);
    CckU32  (*getAddressHash) (const CckTransportAddress *address); /* useful for managing hash tables / indexes based on addresses */

    /* inherited methods end */

    /* public interface - implementations must implement all of those */

    int (*shutdown) 	(TTransport*);
    int (*init)		(TTransport*, void * config);

    CckBool (*testConfig) (TTransport*, void *config); /* test configuration */

    CckBool (*privateHealthCheck) (TTransport *); /* NEW! Now with private healthcare! */
    CckBool (*pushPrivateConfig) (TTransport *, RunTimeOpts *);
    CckBool (*isThisMe) (TTransport *, const char* search);

    void (*loadVendorExt) (TTransport *, const char *);

    /* public interface end */

    /* vendor-specific init and shutdown callbacks */

    int (*_vendorInit) (TTransport *);
    int (*_vendorShutdown) (TTransport *);
    int (*_vendorHealthCheck) (TTransport *);

    /* attach the linked list */
    LINKED_LIST_TAG(TTransport);

};

int		probeTTransport(int transportType, const char *path, int capabilities);
TTransport*  	createTTransport(int driverType, const char* name);
CckBool 	setupTTransport(TTransport* clockDriver, int type, const char* name);
void 		freeTTransport(TTransport** clockDriver);
int		tTransportDummyCallback(TTransport*);
int		tTransportDummyDataCallback (TTransport*, void *data, const int len);

void 		shutdownTTransports();
void		controlTTransports(int);

//void		monitorTTransports();

TTransport*	findTTransport(const char *);
TTransport*	getTTransportByName(const char *);

void		reconfigureTTransports(RunTimeOpts *);
CckBool createTTransportsFromString(const char*, RunTimeOpts *, CckBool);

const char*	getTTransportName(int);
int		getTTransportType(const char*);
CckBool		parseTTransportSpec(const char*, TTransportSpec *);
int		parseAddressList(const char *list, CckTransportAddress *array, int arraySize);

#include "ttransport.def"

#define INIT_DATA_TTRANSPORT(var, type) \
    if(var->_privateData == NULL) { \
	XCALLOC(var->_privateData, sizeof(TTransportData_##type)); \
    }
#define INIT_CONFIG_TTRANSPORT(var, type) \
    if(var->_privateConfig == NULL) { \
	XCALLOC(var->_privateConfig, sizeof(TTransportConfig_##type)); \
    }
#define INIT_EXTDATA_TTRANSPORT(var, type) \
    if(var->_extData == NULL) { \
	XCALLOC(var->_extData, sizeof(TTransportExtData_##type)); \
    }

#define GET_CONFIG_TTRANSPORT(from, to, type) \
    TTransportConfig_##type *to = (TTransportConfig_##type*)from->_privateConfig;

#define GET_DATA_TTRANSPORT(from, to, type) \
    TTransportData_##type *to = (TTransportData_##type*)from->_privateData;

#define GET_EXTDATA_TTRANSPORT(from, to, type) \
    TTransportExtData_##type *to = (TTransportExtData_##type*)from->_extData;

#endif /* CCK_TTRANSPORT_H_ */
