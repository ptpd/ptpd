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


#include "cck_acl_ethernet.h"

#define CCK_THIS_TYPE CCK_ACL_ETHERNET

/* interface (public) method definitions */
static void cckAclInit (CckAcl* acl);
static int  cckAclTest(CckAcl* acl, const char* aclText);
static int  cckAclCompile (CckAcl* acl, const char* permitList, const char* denyList);
static int  cckAclShutdown (void* component);
static int  cckAclMatchAddress (TransportAddress* addr, CckAcl* acl);
static void cckAclClearCounters (CckAcl* acl);
static void cckAclDump (CckAcl* acl);

/* private method definitions (if any) */

/* implementations follow */

void
cckAclSetup_ethernet(CckAcl* acl)
{
	if(acl->aclType==CCK_THIS_TYPE) {
	    acl->testAcl = cckAclTest;
	    acl->compileAcl = cckAclCompile;
	    acl->shutdown = cckAclShutdown;
	    acl->header.shutdown = cckAclShutdown;
	    acl->matchAddress = cckAclMatchAddress;
	    acl->clearCounters = cckAclClearCounters;
	    acl->dump = cckAclDump;

	    cckAclInit(acl);

	} else {
	    CCK_WARNING("ACL setup() called for incorrect transport: %02x, expected %02x\n",
			acl->aclType, CCK_THIS_TYPE);
	}
}

static void
cckAclInit (CckAcl* acl)
{
}

static int
cckAclTest (CckAcl* acl, const char* aclText)
{
	return 1;
}

static int
cckAclCompile (CckAcl* acl, const char* permitList, const char* denyList)
{
	return 1;
}

static int
cckAclShutdown (void* component)
{
	return 1;
}

static int
cckAclMatchAddress (TransportAddress* addr, CckAcl* acl)
{
	acl->passedCounter++;
	return CCK_TRUE;
}

static void
cckAclClearCounters (CckAcl* acl)
{
	acl->passedCounter = 0;
	acl->droppedCounter = 0;
}

static void cckAclDump (CckAcl* acl)
{
}

#undef CCK_THIS_TYPE