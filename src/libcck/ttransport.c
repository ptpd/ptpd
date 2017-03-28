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
 * @file   ttransport.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  Initialisation and control of the Timestamping Transport
 *
 */

#include <config.h>

#include <sys/types.h>
#include <unistd.h>

#include <libcck/libcck.h>
#include <libcck/cck_utils.h>
#include <libcck/ttransport.h>
#include <libcck/cck_logger.h>

#define THIS_COMPONENT "transport: "


/* linked list - so that we can control all registered objects centrally */
LL_ROOT(TTransport);

static const char *ttransportNames[] = {

    [TT_TYPE_NONE] = "none",

#define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, addressfamily, capabilities, extends) \
    [typeenum] = textname,

#include "ttransport.def"

    [TT_TYPE_MAX] = "nosuchtype"

};


/*
 * fault management (incident management, problem management, change management?)
 * ITIL FAULT #0xf4kk: CMDB forgotten to implementen, reporten not senden
 */

/* mark transport as faulty */
static void markTransportFault(TTransport *);
/* clear transport fault state */
static void clearTransportFault(TTransport *transport);
/* age fault timeout */
static void ageTransportFault(TTransport *, const int interval);

/* inherited method declarations */

/* restart: shutdown + init */
static int restartTransport(TTransport *);
/* clear counters */
static void clearCounters (TTransport *);
/* dumpCounters */
static void dumpCounters (TTransport *);
/* update rates */
static void updateCounterRates (TTransport *, const int interval);
/* set up data buffers */
static void setBuffers (TTransport *, char *iBuf, const int iBufSize, char *oBuf, const int oBufSize);
/* magic cure for hypohondria: imaginary health-check */
static bool healthCheck (TTransport *);
/* send n bytes from outputBuffer / outgoingMessage to destination,  */
static ssize_t sendTo (TTransport*, const size_t len, CckTransportAddress *to);
/* send n bytes from buf to destination */
static ssize_t sendDataTo (TTransport*, char *buf, const size_t len, CckTransportAddress *to);
/* receive data into inputBuffer and incomingMessage */
static ssize_t receive (TTransport*);
/* receive data into provided buffer of size len */
static ssize_t receiveData (TTransport*, char* buf, const size_t size);
/* short status line - default implementation */
static char* getStatusLine(TTransport *self, char *buf, size_t len);
/* short info line - default implementation */
static char* getInfoLine(TTransport *self, char *buf, size_t len);
/* check address against ACLs */
static bool addressPermitted (TTransport *self, CckTransportAddress *address, bool isRegular);
/* check message against ACLs */
static bool messagePermitted (TTransport *self, TTransportMessage *message);
/* free private config data */
static void freeTTransportPrivateConfig(TTransportConfig *config);

/* inherited method declarations end */

/* transport management code */

TTransport*
createTTransport(const int type, const char* name) {

    tmpstr(aclName, CCK_COMPONENT_NAME_MAX);

    TTransport *transport = NULL;

    if(getTTransportByName(name) != NULL) {
	CCK_ERROR(THIS_COMPONENT"Cannot create transport %s: transport with this name already exists\n", name);
	return NULL;
    }

    CCKCALLOC(transport, sizeof(TTransport));

    if(!setupTTransport(transport, type, name)) {
	if(transport != NULL) {
	    free(transport);
	}
	return NULL;
    } else {
	/* maintain the linked list */
	LL_APPEND_STATIC(transport);
    }

/* init ACLs */
    snprintf(aclName, CCK_COMPONENT_NAME_MAX, "data_%s", name);
    transport->dataAcl = createCckAcl(transport->family, aclName);
    transport->dataAcl->init(transport->dataAcl);

    clearstr(aclName);
    snprintf(aclName, CCK_COMPONENT_NAME_MAX, "control_%s", name);
    transport->controlAcl = createCckAcl(transport->family, aclName);
    transport->controlAcl->init(transport->controlAcl);

    return transport;

}

bool
setupTTransport(TTransport* transport, const int type, const char* name) {

    bool found = false;
    bool setup = false;
    int privateSize = 0;

    transport->type = type;
    strncpy(transport->name, name, CCK_COMPONENT_NAME_MAX);

    /* inherited methods - implementation may wish to override them,
     * or even preserve these pointers in its private data and call both
     */

    transport->restart		= restartTransport;
    transport->clearCounters	= clearCounters;
    transport->dumpCounters	= dumpCounters;
    transport->updateCounterRates	= updateCounterRates;
    transport->setBuffers	= setBuffers;
    transport->healthCheck	= healthCheck;
    transport->sendTo		= sendTo;
    transport->sendDataTo	= sendDataTo;
    transport->receive		= receive;
    transport->receiveData	= receiveData;
    transport->getStatusLine	= getStatusLine;
    transport->getInfoLine	= getInfoLine;
    transport->addressPermitted = addressPermitted;
    transport->messagePermitted = messagePermitted;

    /* vendor extension placeholders */
    transport->_vendorInit = ttDummyCallback;
    transport->_vendorShutdown = ttDummyCallback;
    transport->_vendorHealthCheck = ttDummyCallback;

    /* inherited methods end */

    /* callbacks */
    transport->callbacks.preTx = ttDummyDataCallback;
    transport->callbacks.isRegularData = ttDummyDataCallback;
    transport->callbacks.matchData = ttDummyMatchCallback;

    /* these macros call the setup functions for existing transport implementations */

    #define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, addressfamily, capabilities, extends) \
	if(type==typeenum) {\
	    found = true;\
	    transport->family = addressfamily;\
	    setup = _setupTTransport_##typesuffix(transport);\
	    privateSize = sizeof(TTransportConfig_##typesuffix);\
	}
    #include "ttransport.def"

    if(!found) {

	CCK_ERROR(THIS_COMPONENT"Setup requested for unknown clock transport type: %d\n", type);

    } else if(!setup) {
	return false;
    } else {

	transport->config._privateSize = privateSize;
	transport->config._type = transport->type;
	transport->config._privateConfig = transport->_privateConfig;
	transport->tools = getAddressToolset(transport->family);

	CCK_DBG(THIS_COMPONENT"Created new transport type %d name %s serial %d\n", type, name, transport->_serial);

    }

    return found;

}

void
freeTTransport(TTransport** transport) {

    TTransport *ptransport = *transport;

    if(*transport == NULL) {
	return;
    }

    freeCckAcl(&ptransport->dataAcl);
    freeCckAcl(&ptransport->controlAcl);

    if(ptransport->_init) {
	ptransport->shutdown(ptransport);
    }

    /* maintain the linked list */
    LL_REMOVE_STATIC(ptransport);

    if(ptransport->_privateData != NULL) {
	free(ptransport->_privateData);
    }

    if(ptransport->_privateConfig != NULL) {
	freeTTransportPrivateConfig(&ptransport->config);
    }

    CCK_DBG(THIS_COMPONENT"Deleted transport type %d name %s serial %d\n", ptransport->type, ptransport->name, ptransport->_serial);

    free(*transport);

    *transport = NULL;

}

int
detectTTransport(const int family, const char *path, const int caps, const int specific) {

    if(specific == TT_TYPE_NONE) {
	CCK_INFO(THIS_COMPONENT"detectTTransport(%s): probing for usable %s transports\n", path,
		getAddressFamilyName(family));
    }

    #define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, addressfamily, capabilities, extends) \
	if(addressfamily == family) {\
	    if((specific == TT_TYPE_NONE) || (typeenum == specific))\
		if ((capabilities & caps)  == caps) {\
		    if(capabilities & TT_CAPS_BROKEN) { \
			CCK_WARNING(THIS_COMPONENT"detectTTransport(path: %s, af: %s): transport "textname" matches flags 0x%02x"\
						    "but is marked as broken in this LibCCK build, skipping\n",\
				path, getAddressFamilyName(family), caps);\
		    } else { \
			CCK_DBG(THIS_COMPONENT"detectTTransport(path: %s, af: %s): transport "textname" matches flags 0x%02x\n",\
				path, getAddressFamilyName(family), caps);\
			CCK_DBG(THIS_COMPONENT"detectTTransport(%s): probing "textname"...\n", path);\
			if(_probeTTransport_##typesuffix(path, caps)) {\
			    CCK_INFO(THIS_COMPONENT"detectTTransport(%s): selected '"textname"'\n", path);\
			    return typeenum; \
			}\
		    } \
		}\
	}

    #include "ttransport.def"

    if (specific != TT_TYPE_NONE) {
	CCK_ERROR("detectTTransport(%s): Selected transport implementation %s not usable!\n",
		path, getTTransportTypeName(specific));
    } else {
	CCK_ERROR("detectTTransport(%s): no usable transport implementation found!\n");
    }

    return TT_TYPE_NONE;
}

TTransportConfig*
createTTransportConfig(const int type) {

	TTransportConfig *ret = NULL;

    #define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, addressfamily, capabilities, extends) \
	    case (typeenum):\
		CCKCALLOC(ret, sizeof(TTransportConfig));\
		ret->_type = type;\
		CCK_INIT_PCONFIG(TTransport, typesuffix, ret);\
		ret->_privateSize = sizeof(TTransportConfig_##typesuffix);\
		_initTTransportConfig_##typesuffix((TTransportConfig_##typesuffix *)(ret->_privateConfig), addressfamily); \
		break;

	switch(type) {

	    #include "ttransport.def"

	    default:
		break;

	}

    return ret;

}

static void
freeTTransportPrivateConfig(TTransportConfig *config) {

	if(config->_privateConfig != NULL) {

	    #define CCK_REGISTER_IMPL(typeenum, typesuffix, textname, addressfamily, capabilities, extends) \
		case (typeenum):\
		    _freeTTransportConfig_##typesuffix((TTransportConfig_##typesuffix *)(config->_privateConfig)); \
		break;

	    switch(config->_type) {

		#include "ttransport.def"

		default:
		    break;

	    }

	    free(config->_privateConfig);
	    config->_privateConfig = NULL;

	}

}

void
freeTTransportConfig(TTransportConfig **config) {

	if(*config == NULL) {
	    return;
	}

	freeTTransportPrivateConfig(*config);

	free (*config);

	*config = NULL;

}

void
controlTTransports(const int command, const void *arg) {

    TTransport *tt;
    bool found;

    switch(command) {

	case TT_NOTINUSE:
	    LL_FOREACH_STATIC(tt) {
		tt->inUse = false;
//		tt->config.required = false;
	    }
	break;

	case TT_SHUTDOWN:
	    LL_DESTROYALL(tt, freeTTransport);
	break;

	case TT_CLEANUP:
	    do {
		found = false;
		LL_FOREACH_STATIC(tt) {
		    if(!tt->inUse) {
			freeTTransport(&tt);
			found = true;
			break;
		    }
		}
	    } while(found);
	break;

	case TT_DUMP:
	    LL_FOREACH_STATIC(tt) {
		if(!tt->config.disabled) {
		/* TODO: dump transport information */
		tt->dumpCounters(tt);
		}

	    }
	break;

	case TT_MONITOR:
	    monitorTTransports(*(const int*)arg);
	break;

	case TT_REFRESH:
	    LL_FOREACH_STATIC(tt) {
		if(!tt->config.disabled) {
		    tt->refresh(tt);
		}
	    }
	break;

	case TT_CLEARCOUNTERS:
	    LL_FOREACH_STATIC(tt) {
		    tt->clearCounters(tt);
	    }
	break;

	case TT_DUMPCOUNTERS:
	    LL_FOREACH_STATIC(tt) {
		    tt->dumpCounters(tt);
	    }
	break;

	case TT_UPDATECOUNTERS:
	    LL_FOREACH_STATIC(tt) {
		    tt->updateCounterRates(tt, *(const int *)arg);
	    }
	break;

	default:
	    CCK_ERROR(THIS_COMPONENT"controlTTransports(): Unnown transport command %02d\n", command);

    }

}

void
shutdownTTransports() {

	TTransport *tt;
	LL_DESTROYALL(tt, freeTTransport);

}

void
monitorTTransports(const int interval) {

    TTransport *tt;

    LL_FOREACH_STATIC(tt) {
	if(!tt->config.disabled && !tt->config.unmonitored) {
	    monitorTTransport(tt, interval);
	}
    }

}

void monitorTTransport(TTransport *transport, const int interval) {

    bool previousFault = transport->fault;
    bool restartNeeded = false;

    /* run the countdown timer, cancel fault state if ran out */
    ageTransportFault(transport, interval);

    if(!transport->fault) {

	/* if fault has not gone away, previousFault is true (quiet) */
	int res = transport->monitor(transport, interval, previousFault);

	if (res & CCK_INTINFO_FAULT) {
	    if(!(res & CCK_INTINFO_NOCHANGE)) {
		CCK_ERROR(THIS_COMPONENT"monitorTTransport('%s'): Transport fault detected\n", transport->name);
	    }
	    markTransportFault(transport);
	    SAFE_CALLBACK(transport->callbacks.onNetworkFault, transport, transport->owner, true);
	    return;

	}

	if (res & CCK_INTINFO_CLEAR) {
	    CCK_NOTICE(THIS_COMPONENT"monitorTTransport('%s'): Transport fault cleared\n", transport->name);
	    restartNeeded = true;
	} else if (res & CCK_INTINFO_CHANGE) {
	    CCK_WARNING(THIS_COMPONENT"monitorTTransport('%s'): Address / topology change detected\n", transport->name);
	    restartNeeded = true;
	} else if (res & CCK_INTINFO_UP) {
	    CCK_NOTICE(THIS_COMPONENT"monitorTTransport('%s'): Transport link up\n", transport->name);
	    /* strange things can happen when interfaces go up */
	    transport->_skipRxMessages = TT_CHANGE_SKIP_PACKETS;
	    transport->_skipTxMessages = TT_CHANGE_SKIP_PACKETS;
	    transport->refresh(transport);
	    if(transport->slaveTransport != NULL) {
		transport->slaveTransport->refresh(transport->slaveTransport);
	    }
	    SAFE_CALLBACK(transport->callbacks.onNetworkChange, transport, transport->owner, false);
	} else if (res & CCK_INTINFO_DOWN) {
	    CCK_NOTICE(THIS_COMPONENT"monitorTTransport('%s'): Transport link down\n", transport->name);
	}

	if(restartNeeded) {
	    int result = transport->restart(transport);
	    if(result != 1) {
		CCK_ERROR(THIS_COMPONENT"monitorTTransport('%s'): Could not restart transport\n", transport->name);
		markTransportFault(transport);
		SAFE_CALLBACK(transport->callbacks.onNetworkFault, transport, transport->owner, true);
	    } else {
		clearTransportFault(transport);
		/* strange things can happen when interfaces go up */
		transport->_skipRxMessages = TT_CHANGE_SKIP_PACKETS;
		transport->_skipTxMessages = TT_CHANGE_SKIP_PACKETS;
		SAFE_CALLBACK(transport->callbacks.onNetworkChange, transport, transport->owner, true);
	    }
	} else if (res & CCK_INTINFO_CLOCKCHANGE) {
	    CCK_NOTICE(THIS_COMPONENT"monitorTTransport('%s'): Clock driver change detected\n", transport->name);
	    SAFE_CALLBACK(transport->callbacks.onClockDriverChange, transport, transport->owner);
	}

    }

}

void
refreshTTransports() {

    TTransport *tt;

    LL_FOREACH_STATIC(tt) {
	if(!tt->config.disabled) {
	    tt->refresh(tt);
	}
    }

}

TTransport*
findTTransport(const char *search) {

	TTransport *tt;
	LL_FOREACH_STATIC(tt) {
	    if(tt->isThisMe(tt, search)) {
		return tt;
	    }
	}

	return NULL;

}

TTransport*
getTTransportByName(const char *name) {

	TTransport *tt;

	LL_FOREACH_STATIC(tt) {
	    if(!strncmp(tt->name, name, CCK_COMPONENT_NAME_MAX)) {
		return tt;
	    }
	}

	return NULL;

}

const char*
getTTransportTypeName(const int type) {

    if ((type < 0) || (type >= TT_TYPE_MAX)) {
	return NULL;
    }

    return ttransportNames[type];

}

int
getTTransportType(const char* name) {

    for(int i = 0; i < TT_TYPE_MAX; i++) {

	if(!strcmp(name, ttransportNames[i])) {
	    return i;
	}

    }

    return -1;

}

int getTTransportTypeFamily(const int type) {

    if ((type < 0) || (type >= TT_TYPE_MAX)) {
	return -1;
    }

    return transportTypeFamilies[type];

}

/* placeholder callbacks */

int
ttDummyCallback (TTransport *transport) {
    return 1;
}

int
ttDummyOwnerCallback (void *owner, void *transport) {
    return 1;
}

int
ttDummyDataCallback (void *owner, void *transport, char *data, const size_t len, bool isMulticast) {
    return 1;
}

int
ttDummyMatchCallback (void *owner, void *transport, char *a, const size_t alen, char *b, const size_t blen) {
    return 1;
}

/* restart: shutdown + init */
static int
restartTransport(TTransport *transport) {

    TTransport *child = transport->slaveTransport;
    int ret;

    CCK_NOTICE(THIS_COMPONENT"Transport '%s' restarting\n", transport->name);
    transport->shutdown(transport);
    ret = transport->init(transport, &transport->config, transport->fdSet);

    if((ret > 0) && child) {
	CCK_NOTICE(THIS_COMPONENT"Transport '%s' restarting associated transport '%s'\n", transport->name,
		    child->name);
	child->shutdown(child);
	return child->init(child, &child->config, child->fdSet);
    }

    return ret;

}

/* clear counters */
static void
clearCounters (TTransport *transport) {

	memset(&transport->counters, 0, sizeof(TTransportCounters));
	if(transport->controlAcl) {
	    transport->controlAcl->clearCounters(transport->controlAcl);
	}
	if(transport->dataAcl) {
	    transport->dataAcl->clearCounters(transport->dataAcl);
	}

}

/* dumpCounters */
static void
dumpCounters (TTransport *transport) {

	TTransportCounters *c = &transport->counters;

	CCK_INFO(THIS_COMPONENT" ======= transport '%s' counters =============\n", transport->name);
	CCK_INFO(THIS_COMPONENT"             rxMsg: %lu\n", c->rxMsg);
	CCK_INFO(THIS_COMPONENT"             txMsg: %lu\n", c->txMsg);
	CCK_INFO(THIS_COMPONENT"           rxBytes: %lu\n", c->rxBytes);
	CCK_INFO(THIS_COMPONENT"           txBytes: %lu\n", c->txBytes);
	CCK_INFO(THIS_COMPONENT" ======= transport '%s' rates ================\n", transport->name);
	CCK_INFO(THIS_COMPONENT"         rxRateMsg: %lu / s\n", c->rxRateMsg);
	CCK_INFO(THIS_COMPONENT"         txRateMsg: %lu / s\n", c->txRateMsg);
	CCK_INFO(THIS_COMPONENT"       rxRateBytes: %lu / s\n", c->rxRateBytes);
	CCK_INFO(THIS_COMPONENT"       txRateBytes: %lu / s\n", c->txRateBytes);
	CCK_INFO(THIS_COMPONENT" ======= transport '%s' error counters =======\n", transport->name);
	CCK_INFO(THIS_COMPONENT"          rxErrors: %lu\n", c->rxErrors);
	CCK_INFO(THIS_COMPONENT"          txErrors: %lu\n", c->txErrors);
	CCK_INFO(THIS_COMPONENT" rxTimestampErrors: %lu\n", c->rxTimestampErrors);
	CCK_INFO(THIS_COMPONENT" txTimestampErrors: %lu\n", c->txTimestampErrors);
	CCK_INFO(THIS_COMPONENT"        rxDiscards: %lu\n", c->rxDiscards);
	CCK_INFO(THIS_COMPONENT"        txDiscards: %lu\n", c->txDiscards);
	CCK_INFO(THIS_COMPONENT" ===========================================\n");
}

/* update rates */
static void
updateCounterRates (TTransport *transport, const int interval) {

	TTransportCounters *c = &transport->counters;

	c->rateInterval = interval;

	/* only update when ready and do not update on rollovers */
	if(c->updated) {
	    if(c->rxMsg > c->_snapRxMsg) {
		c->rxRateMsg = (c->rxMsg - c->_snapRxMsg) / interval;
	    }
	    if(c->txMsg > c->_snapTxMsg) {
		c->txRateMsg = (c->txMsg - c->_snapTxMsg) / interval;
	    }
	    if(c->rxBytes > c->_snapRxBytes) {
		c->rxRateBytes = (c->rxBytes - c->_snapRxBytes) / interval;
	     }
	    if(c->txBytes > c->_snapTxBytes) {
		c->txRateBytes = (c->txBytes - c->_snapTxBytes) / interval;
	    }
	}

	c->_snapRxMsg = c->rxMsg;
	c->_snapTxMsg = c->txMsg;
	c->_snapRxBytes = c->rxBytes;
	c->_snapTxBytes = c->txBytes;

	c->updated = true;

	if(!transport->config.unmonitored) {
	    SAFE_CALLBACK(transport->callbacks.onRateUpdate, transport, transport->owner);
	}

}

/* set up data buffers */
static void
setBuffers (TTransport *transport, char *iBuf, const int iBufSize, char *oBuf, const int oBufSize) {

	transport->inputBuffer = iBuf;
	transport->inputBufferSize = iBufSize;
	transport->incomingMessage.data = iBuf;
	transport->incomingMessage.capacity = iBufSize;

	transport->outputBuffer = oBuf;
	transport->outputBufferSize = oBufSize;
	transport->outgoingMessage.data = oBuf;
	transport->outgoingMessage.capacity = oBufSize;

}


/* magic cure for hypohondria: imaginary health check */
static bool
healthCheck (TTransport *transport) {

    bool ret = true;

    if(transport == NULL) {
	return false;
    }

    CCK_DBG(THIS_COMPONENT"transport %s health check...\n", transport->name);

    ret &= transport->privateHealthCheck(transport);
    ret &= transport->_vendorHealthCheck(transport);

    return ret;

}

/* send n bytes from outputBuffer / outgoingMessage to destination,  */
static ssize_t
sendTo (TTransport *transport, const size_t len, CckTransportAddress *to) {

	transport->outgoingMessage.destination = to;
	transport->outgoingMessage.length = max(transport->outgoingMessage.capacity, len);
	return transport->sendMessage(transport, &transport->outgoingMessage);

}

/* send n bytes from external buf to destination */
static ssize_t
sendDataTo (TTransport *transport, char *buf, const size_t len, CckTransportAddress *to) {


	TTransportMessage msg;
	clearTTransportMessage(&msg);

	msg.data = buf;
	msg.capacity = len;
	msg.length = len;
	msg.destination = to;

	return transport->sendMessage(transport, &msg);

}

/* receive data into inputBuffer and incomingMessage */
static ssize_t
receive (TTransport *transport) {

	return transport->receiveMessage(transport, &transport->incomingMessage);

}

/* receive data into provided buffer of size len */
static ssize_t
receiveData(TTransport *transport, char* buf, const size_t size) {

	TTransportMessage msg;
	clearTTransportMessage(&msg);

	msg.data = buf;
	msg.capacity = size;

	return transport->receiveMessage(transport, &msg);

}

/* short information line */
static char*
getInfoLine(TTransport *self, char *buf, size_t len) {

	snprintf(buf, len, "%s, %s", getTTransportTypeName(self->type),
		    TT_UC_MC(self->config.flags) ? "hybrid UC/MC" :
		    TT_MC(self->config.flags) ? "multicast" : "unicast");

	return buf;

}

/* short status line (default implementation only gives an empty string */
static char*
getStatusLine(TTransport *self, char *buf, size_t len) {

	return buf;

}

bool
copyTTransportConfig(TTransportConfig *to, const TTransportConfig *from) {

    if(to == NULL || from == NULL) {
	return false;
    }

    void *backup = to->_privateConfig;
    memcpy(to, from, sizeof(TTransportConfig));
    to->_privateConfig = backup;
    memcpy(to->_privateConfig, from->_privateConfig, to->_privateSize);

    CCK_DBG(THIS_COMPONENT"copyTTransportConfig(): success\n");

    return true;

}

TTransportConfig
*duplicateTTransportConfig(const TTransportConfig *from) {

    TTransportConfig *ret = NULL;

    if(from == NULL) {
	return NULL;
    }

    ret = createTTransportConfig(from->_type);

    if(ret == NULL) {
	return NULL;
    }

    copyTTransportConfig(ret, from);

    return ret;

}

/* set transport UID based on h/w and/or protocol address */
void
setTTransportUid(TTransport *transport) {

    unsigned char *hwData = getAddressData(&transport->hwAddress);
    unsigned char *addrData = getAddressData(&transport->ownAddress);

    uint16_t filler = tobe16(transport->config.uidPid ? getpid() : 0xfffe);

    uint16_t *middle = (uint16_t*)(transport->uid + 3);
    uint16_t *right = (uint16_t*)(transport->uid + 6);

    uint64_t *customUid = (uint64_t*)transport->config.customUid;

    /* custom UID, just copy it */
    if(*customUid) {

	memcpy(transport->uid, transport->config.customUid, 8);

    } else if (transport->hwAddress.populated) {

	/* copy MAC address to the outer 48 bits */
	memcpy(transport->uid, hwData, 3);
	memcpy(transport->uid + 5, hwData + 3, 3);
	*middle = filler;

    } else {

	/* copy whole IPv6 address, fill midle with pid if configured to do so */
	if(transport->family == TT_FAMILY_IPV6) {
	    memcpy(transport->uid, addrData, 8);
	    if(transport->config.uidPid) {
		*middle = filler;
	    }
	}

	if(transport->family == TT_FAMILY_IPV4) {
	    uint32_t addr = *(uint32_t*)addrData;
	    /* if uidPid, copy pid to the right and address just before it */
	    if(transport->config.uidPid) {
		*right = tobe16(getpid());
		memcpy(transport->uid + 2, &addr, 4);
	    } else {
	    /* if not, just copy address to right */
		memcpy(transport->uid + 4, &addr, 4);
	    }
	}

	/* no protocol address - fill middle with rfc2373, right with pid */
	if(!transport->ownAddress.populated) {
	    *right = tobe16(getpid());
	    *middle = tobe16(0xfffe);
	}

    }

#ifdef CCK_DEBUG
    uint8_t *uid8 = (uint8_t*)transport->uid;
#endif /* CCK_DEBUG */
    CCK_DBG(THIS_COMPONENT"setTransportUid(%s): UID set to %02x%02x%02x%02x%02x%02x%02x%02x\n",
	    transport->name, uid8[0], uid8[1], uid8[2], uid8[3], uid8[4], uid8[5], uid8[6], uid8[7]);

}

/* clear message without losing the buffer pointer */
void
clearTTransportMessage(TTransportMessage *message) {

    if (message->data) {
	memset(message->data, 0, message->capacity);
    }
    message->length = 0;
    message->fromSelf = false;
    message->toSelf = false;
    message->isMulticast = false;
    message->hasTimestamp = false;
    message->destination = NULL;

    memset(&message->from, 0, sizeof(CckTransportAddress));
    memset(&message->to, 0, sizeof(CckTransportAddress));

    tsOps.clear(&message->timestamp);

}

void	dumpTTransportMessage(const TTransportMessage *message) {

	CckAddressToolset* ts = getAddressToolset(message->_family);

	CCK_INFO("\n");
	CCK_INFO("+-------------------------------------------+\n");
	CCK_INFO("|         TTransport message dump           |\n");
	CCK_INFO("+-------------------------------------------+\n");
	if(!message) {

	    CCK_INFO(THIS_COMPONENT"  dumpTTransportMessage: null message\n");

	} else {

	    CCK_INFO("\n");
	    CCK_INFO("              length: %d\n", message->length);
	    CCK_INFO("            fromSelf: %s\n", boolstr(message->fromSelf));
	    CCK_INFO("              toSelf: %s\n", boolstr(message->toSelf));
	    CCK_INFO("         isMulticast: %s\n", boolstr(message->isMulticast));
	    CCK_INFO("        hasTimestamp: %s\n", boolstr(message->hasTimestamp));

	    if(message->hasTimestamp) {
		tmpstr(tst, CCK_TIMESTAMP_STRLEN);
		snprint_CckTimestamp(tst, tst_len, &message->timestamp);

	    CCK_INFO("           timestamp:%s\n", tst);

	    }

	    if(!ts) {

	    CCK_INFO("      address family: [unknown / unsupported]\n");

	    } else {

		    tmpstr(a1, ts->strLen);
		    tmpstr(a2, ts->strLen);
		    ts->toString(a1, a1_len, &message->from);
		    ts->toString(a2, a2_len, &message->to);

	    CCK_INFO("      address family: %s\n", getAddressFamilyName(message->_family));
	    CCK_INFO("              source: %s\n", message->from.populated ? a1 : " - ");
	    CCK_INFO("         destination: %s\n", message->to.populated ? a2 : " - ");

		    if(message->destination) {
			tmpstr(a3, ts->strLen);
			ts->toString(a3, a3_len, message->destination);

	    CCK_INFO("  for transmission to: %s\n", message->destination->populated ? a3 : " - ");

		    }

	    }

	    CCK_INFO("\n");

	    if(!message->data) {
		CCK_INFO(THIS_COMPONENT"  dumpTTransportMessage: null data buffer\n");
	    } else {
		dumpBuffer(message->data, message->length, 8, "Message contents:");
	    }
	
	}

}

/* check message source against data or control ACL */
static bool
messagePermitted (TTransport *self, TTransportMessage *message) {

    if(!message->from.populated || message->fromSelf) {
	return true;
    }

    int regular = self->callbacks.isRegularData(self, self->owner, message->data, message->length, message->isMulticast);

    return self->addressPermitted(self, &message->from, regular);

}

/* check address against ACLs */
static bool
addressPermitted (TTransport *self, CckTransportAddress *address, bool isRegular) {

    CckAcl *acl;
    bool ret = true;

    acl  = isRegular ? self->dataAcl : self->controlAcl;

    ret = acl->match(acl, address);
#ifdef CCK_DEBUG
    if(!ret) {
	tmpstr(strAddrf, self->tools->strLen);
	CCK_DBG(THIS_COMPONENT"addressPermitted(%s): ACL %s blocked message from %s\n",
	    self->name, acl->name, self->tools->toString(strAddrf, strAddrf_len, address));
    }
#endif /* CCK_DEBUG */

    return ret;

}

/* mark transport as faulty, start countdown */
static void
markTransportFault(TTransport *transport)
{

    transport->fault = true;
    transport->_faultTimeout = getCckConfig()->transportFaultTimeout;

    CCK_DBG(THIS_COMPONENT": marked transport %s as faulty, next check in %d seconds\n",
		transport->name, transport->_faultTimeout);

}

/* clear transport fault state */
static void
clearTransportFault(TTransport *transport)
{

    transport->fault = false;
    transport->_faultTimeout = 0;

    CCK_DBG(THIS_COMPONENT": cleared transport %s fault status\n",
		transport->name);

}

/* age fault timeout */
static void
ageTransportFault(TTransport * transport, const int interval)
{

    transport->_faultTimeout -= interval;

    if(transport->_faultTimeout <= 0) {
	transport->_faultTimeout = 0;
	if(transport->fault) {
	    CCK_DBG(THIS_COMPONENT"transport '%s' fault timeout, check at next monitor interval\n",
			transport->name);
	}
	transport->fault = false;
    }

}
