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
 * @file   cck_acl_null.h
 * 
 * @brief  libCCK null transport ACL implementation definitions
 *
 * The null ACL is the ACL for the null transport, but is also a pass-all
 * ACL, ignoring the TransportAddress it's matched against, and
 * always returning true.
 *
 */

#ifndef CCK_ACL_NULL_H_
#define CCK_ACL_NULL_H_

#ifndef CCK_H_INSIDE_
//#error libCCK component headers should not be uncluded directly - please include cck.h only.
#endif

#include "../cck.h"
#include "../cck_transport.h"
#include "../cck_acl.h"

/* NULL transport ACL init */
void cckAclInit_null (CckAcl* acl);

/* NULL transport ACL methods */
int  cckAclTest_null(CckAcl* acl, const char* aclText);
int  cckAclCompile_null (CckAcl* acl, const char* permitList, const char* denyList);
int  cckAclShutdown_null (void* component);
int  cckAclMatchAddress_null (TransportAddress* addr, CckAcl* acl);
void cckAclClearCounters_null (CckAcl* acl);
void cckAclDump_null (CckAcl* acl);

#endif /* CCK_ACL_NULL_H_ */
