/* Copyright (c) 2017 Wojciech Owczarek,
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
 * @file   acl.h
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  structure definitions for the transport ACL
 *
 */

#ifndef CCK_ACL_H_
#define CCK_ACL_H_

#include <string.h>

#include <libcck/cck.h>
#include <libcck/linked_list.h>
#include <libcck/cck_types.h>
#include <libcck/transport_address.h>

enum {
    CCK_ACL_PERMIT_DENY,
    CCK_ACL_DENY_PERMIT
};

/* common ACL configuration */
typedef struct {
    uint8_t order;
    bool disabled;	/* shields up */
    bool blocking;	/* shields down */
} CckAclConfig;

/* ACL counters */
typedef struct {
    uint32_t	passed;
    uint32_t	dropped;
} CckAclCounters;

/* ACL entry */
typedef struct {
    void *_privateData;
    uint32_t hitCount;
} CckAclEntry;

typedef struct CckAcl CckAcl;

struct CckAcl {

    int type;				/* implementation type = address family */
    char name[CCK_COMPONENT_NAME_MAX +1];	/* instance name */
    void *owner;			/* pointer to user structure owning or controlling this component */

    CckAclConfig config;
    CckAclCounters counters;

    /* BEGIN "private" fields */
    int _serial;			/* object serial no */
    int	_init;				/* the object was successfully initialised */
    int *_instanceCount;		/* instance counter for the whole component */

    bool _populated;			/* no match performed if false */

    int _permitCount;			/* how many "permit" entries */
    CckAclEntry *_permitData;		/* "permit" data */

    int _denyCount;			/* how many "deny" entries */
    CckAclEntry *_denyData;		/* "deny" data */

    /* END "private" fields */

    /* inherited methods */

    /* shutdown (init is per implementation) */
    int (*shutdown) 	(CckAcl *);
    /* reset an ACL - empty its contents */
    void (*reset) 	(CckAcl *);
    /* clear counters */
    void (*clearCounters)(CckAcl *);
    /* dump ACL contents and counters */
    void (*dump)(CckAcl *);
    /* match a transport address against ACL */
    bool (*match)(CckAcl *, CckTransportAddress *);
    /* compile or re-compile */
    bool (*compile)	(CckAcl*, const char* permitList, const char* denyList, const uint8_t order);
    /* test if ACL can be compiled */
    bool (*test)	(CckAcl*, const char* permitList, const char* denyList, const uint8_t order, const bool quiet);

    /* inherited methods end */

    /* public + private interface - implementations must implement all of those */

    /* init */
    int (*init)		(CckAcl *);
    /* parse an ACL entry, only test if NULL given */
    bool (*_parseEntry) (CckAcl *, CckAclEntry*, const char *, const bool quiet);
    /* match a transport address against ACL entry */
    bool (*_matchEntry)	(CckAcl *, CckAclEntry*, CckTransportAddress *);
    /* dump an ACL entry */
    char * (*_dumpEntry)	(CckAclEntry*, char *, const int);

    /* public interface end */

    /* attach the linked list */
    LL_MEMBER(CckAcl);

};

CckAcl*	createCckAcl(int type, const char* name);
bool	setupCckAcl(CckAcl *acl, int type, const char* name);
void	freeCckAcl(CckAcl **acl);
void	shutdownCckAcls();
CckAcl*	getCckAclByName(const char *);
bool	testCckAcl(int type, const char* permitList, const char* denyList, const uint8_t order, const bool quiet);

/* invoking this without CCK_REGISTER_IMPL defined, includes the implementation headers */
#include "acl.def"

#endif /* CCK_ACL_H_ */
