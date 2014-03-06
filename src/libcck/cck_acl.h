/*-
 * libCCK - Clock Construction Kit
 *
 * Copyright (c) 2014 Wojciech Owczarek,
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
 * @file   cck_acl.h
 * 
 * @brief  libCCK ACL component interface definitions
 *
 */

#ifndef CCK_ACL_H_
#define CCK_ACL_H_

#ifndef CCK_H_INSIDE_
#error libCCK component headers should not be uncluded directly - please include cck.h only.
#endif

typedef struct CckAcl CckAcl;

#include "cck.h"
//#include "cck_component.h"
//#include "cck_transport.h"

enum {
	CCK_ACL_NULL = 0,
	CCK_ACL_IPV4,
	CCK_ACL_IPV6,
	CCK_ACL_ETHERNET
};

enum {
	CCK_ACL_PERMIT_DENY,
	CCK_ACL_DENY_PERMIT
};

struct CckAcl {
    /* mandatory - this has to be the first member */
    CckComponent header;

    /* common */
    int aclType;
    int processingOrder;

    uint32_t passedCounter;
    uint32_t droppedCounter;

    /* private / implementation specific data */
    void* internalData;

    /* data to hold permit entries and deny entries */
    void* aclPermitData;
    void* aclDenyData;

    /* the interface: function pointers to be assigned by implementations */
    int  (*testAcl)       (CckAcl*, const char*);
    int	 (*compileAcl)    (CckAcl*, const char* , const char*);
    int  (*shutdown)      (void*);
    int  (*matchAddress)  (TransportAddress*, CckAcl*);
    void (*clearCounters) (CckAcl*);
    void (*dump)          (CckAcl*);

};

/* component constructor / destructor */
CckAcl* createCckAcl(int aclType, int processingOrder, const char* instanceName);
void    setupCckAcl(CckAcl* acl, int aclType, int processingOrder, const char* instanceName);
void	freeCckAcl  (CckAcl** acl);

#include "acl/cck_acl_null.h"
#include "acl/cck_acl_ipv4.h"
#include "acl/cck_acl_ipv6.h"
#include "acl/cck_acl_ethernet.h"

#endif /* CCK_ACL_H_ */
