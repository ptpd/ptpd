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
 * @file   cck_acl_ipv6.h
 * 
 * @brief  libCCK IPv4 transport ACL implementation definitions
 */

#ifndef CCK_ACL_IPV6_H_
#define CCK_ACL_IPV6_H_

#ifndef CCK_H_INSIDE_
//#error libCCK component headers should not be uncluded directly - please include cck.h only.
#endif

#include "../cck.h"
#include "../cck_transport.h"
#include "../cck_acl.h"

/* IPV6 transport ACL init */
void cckAclInit_ipv6 (CckAcl* acl);

/* IPV6 transport ACL methods */
int  cckAclTest_ipv6(CckAcl* acl, const char* aclText);
int  cckAclCompile_ipv6 (CckAcl* acl, const char* permitList, const char* denyList);
int  cckAclShutdown_ipv6 (void* component);
int  cckAclMatchAddress_ipv6	(TransportAddress* addr, CckAcl* acl);
void cckAclClearCounters_ipv6 (CckAcl* acl);
void cckAclDump_ipv6 (CckAcl* acl);

#endif /* CCK_ACL_IPV6_H_ */
