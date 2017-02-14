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
 * @file   acl.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  Initialisation and control of the ACL
 *
 */

#include <config.h>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <libcck/cck_utils.h>
#include <libcck/transport_address.h>
#include <libcck/acl.h>
#include <libcck/cck_logger.h>

#define THIS_COMPONENT "acl: "


/* linked list - so that we can control all registered objects centrally */
LL_ROOT(CckAcl);

/* inherited method declarations */

/* shutdown */
static int aclShutdown 	(CckAcl * self);
/* reset an ACL - empty its contents */
static void aclReset 	(CckAcl *self);
/* clear counters */
static void aclClearCounters (CckAcl *self);
/* dump ACL contents and counters */
static void aclDump (CckAcl *self);
/* match a transport address against ACL */
static bool aclMatch (CckAcl *self, CckTransportAddress *address);
/* compile or re-compile */
static bool aclCompile	(CckAcl *self, const char* permitList, const char* denyList, const uint8_t order);
/* test if ACL can be compiled */
static bool aclTest	(CckAcl *self, const char* permitList, const char* denyList, const uint8_t order, const bool quiet);

/* inherited method declarations end */

/* parse a line of ACL entries into CckAclEntry* table, return count of valid elements */
static int aclParse(CckAcl *self, CckAclEntry **table, const char *line, const bool quiet);

/* ACL management code */

CckAcl*
createCckAcl(const int type, const char* name) {

    CckAcl *acl = NULL;

    if(getCckAclByName(name) != NULL) {
	CCK_ERROR(THIS_COMPONENT"Cannot create ACL %s: AC: with this name already exists\n", name);
	return NULL;
    }

    CCKCALLOC(acl, sizeof(CckAcl));

    if(!setupCckAcl(acl, type, name)) {
	if(acl != NULL) {
	    free(acl);
	}
	return NULL;
    } else {
	/* maintain the linked list */
	LL_APPEND_STATIC(acl);
    }

    return acl;

}

bool
setupCckAcl(CckAcl* acl, const int type, const char* name) {

    bool found = false;
    bool setup = false;

    acl->type = type;
    strncpy(acl->name, name, CCK_COMPONENT_NAME_MAX);

    /* inherited methods - implementation may wish to override them,
     * or even preserve these pointers in its private data and call both
     */

    acl->shutdown = aclShutdown;
    acl->reset = aclReset;
    acl->clearCounters = aclClearCounters;
    acl->dump = aclDump;
    acl->match = aclMatch;
    acl->compile = aclCompile;
    acl->test = aclTest;

    /* these macros call the setup functions for existing acl implementations */

    #define CCK_REGISTER_IMPL(typeenum, typesuffix) \
	if(type==typeenum) {\
	    found = true;\
	    setup = _setupCckAcl_##typesuffix(acl);\
	}
    #include "acl.def"

    if(!found) {

	CCK_ERROR(THIS_COMPONENT"Setup requested for unknown / unsupported ACL type: %d\n", type);

    } else if(!setup) {
	return false;
    } else {

	CCK_DBG(THIS_COMPONENT"Created new ACL type %d name %s serial %d\n", type, name, acl->_serial);

    }

    return found;

}

void
freeCckAcl(CckAcl** acl) {

    CckAcl *pacl = *acl;

    if(*acl == NULL) {
	return;
    }

    if(pacl->_init) {
	pacl->shutdown(pacl);
    }

    /* maintain the linked list */
    LL_REMOVE_STATIC(pacl);

    CCK_DBG(THIS_COMPONENT"Deleted ACL type %d name %s serial %d\n", pacl->type, pacl->name, pacl->_serial);

    free(*acl);

    *acl = NULL;

}


void
shutdownCckAcls() {

	CckAcl *tt;
	LL_DESTROYALL(tt, freeCckAcl);

}

CckAcl*
getCckAclByName(const char *name) {

	CckAcl *acl;

	LL_FOREACH_STATIC(acl) {
	    if(!strncmp(acl->name, name, CCK_COMPONENT_NAME_MAX)) {
		return acl;
	    }
	}

	return NULL;

}

static int
aclShutdown (CckAcl * self) {


    self->reset(self);

    CCK_DBG(THIS_COMPONENT"ACL %s shutdown\n", self->name);

    self->_init = false;

    return 1;

}

static void
aclReset (CckAcl *self) {

    if(!self->_init || !self->_populated) {
	return;
    }

    self->clearCounters(self);

    if(self->_permitData) {

	for(int i = 0; i < self->_permitCount; i++) {
	    if(self->_permitData[i]._privateData != NULL) {
		free(self->_permitData[i]._privateData);
		self->_permitData[i]._privateData = NULL;
	    }
	}

	free(self->_permitData);
	self->_permitData = NULL;
	self->_permitCount = 0;

    }

    if(self->_denyData) {

	for(int i = 0; i < self->_denyCount; i++) {
	    if(self->_denyData[i]._privateData != NULL) {
		free(self->_denyData[i]._privateData);
		self->_denyData[i]._privateData = NULL;
	    }
	}

	free(self->_denyData);
	self->_denyData = NULL;
	self->_denyCount = 0;

    }

    self->_populated = false;


    CCK_DBG(THIS_COMPONENT"ACL %s reset\n", self->name);

}

static void
aclClearCounters (CckAcl *self) {

    if(!self->_init || !self->_populated) {
	return;
    }

    for(int i = 0; i < self->_permitCount; i++) {
	self->_permitData[i].hitCount = 0;
    }

    for(int i = 0; i < self->_denyCount; i++) {
	self->_denyData[i].hitCount = 0;
    }

    self->counters.passed = 0;
    self->counters.dropped = 0;

    CCK_DBG(THIS_COMPONENT"ACL %s counters cleared\n", self->name);

}

static void
aclDump (CckAcl *self) {

    tmpstr(entrydump, 200);

    CCK_INFO("=== %s Access Control List '%s':\n", getAddressFamilyName(self->type), self->name);
    CCK_INFO("\n");
    CCK_INFO("  Order    : %s\n", self->config.order == CCK_ACL_PERMIT_DENY ? "permit, deny" : "deny, permit");
    CCK_INFO("  Disabled : %s\n", boolyes(self->config.disabled));
    CCK_INFO("  Blocking : %s\n", boolyes(self->config.blocking));
    CCK_INFO("  Passed   : %d\n", self->counters.passed);
    CCK_INFO("  Dropped  : %d\n", self->counters.dropped);

    CCK_INFO("\n");
    CCK_INFO("  Permit list:\n");
    if(self->_permitCount) {
	for(int i = 0; i < self->_permitCount; i++) {
	    self->_dumpEntry(&self->_permitData[i], entrydump, entrydump_len);
	    CCK_INFO("   %02d: %s     hits: %d\n", i+1, entrydump, self->_permitData[i].hitCount);
	    clearstr(entrydump);
	}
    } else {
	CCK_INFO("  (empty list)\n");
    }

    CCK_INFO("\n");
    CCK_INFO("  Deny list:\n");

    if(self->_denyCount) {
	for(int i = 0; i < self->_denyCount; i++) {
	    self->_dumpEntry(&self->_denyData[i], entrydump, entrydump_len);
	    CCK_INFO("   %02d: %s     hits: %d\n", i+1, entrydump, self->_denyData[i].hitCount);
	    clearstr(entrydump);
	}
    } else {
	CCK_INFO("  (empty list)\n");
    }

    CCK_INFO("\n");

}

static bool
aclMatch (CckAcl *self, CckTransportAddress *address) {


    bool ret;

    bool matchPermit = false;
    bool matchDeny = false;

    if(!self->_init || self->config.blocking) {
	CCK_DBG(THIS_COMPONENT"ACL %s is set to blocking - dropping message\n", self->name);
	ret = false;
	goto gameover;
    }

    if(!self->_populated || self->config.disabled) {
	CCK_DBG(THIS_COMPONENT"ACL %s is open - permitting message\n", self->name);
	ret = true;
	goto gameover;
    }

    if(self->_permitCount && self->_permitData) {

	for(int i = 0; i < self->_permitCount; i++) {
	    if(self->_matchEntry(self, &self->_permitData[i], address)) {
		matchPermit = true;
		break;
	    }
	}

    }

    if(self->_denyCount && self->_denyData) {

	for(int i = 0; i < self->_denyCount; i++) {
	    if(self->_matchEntry(self, &self->_denyData[i], address)) {
		matchDeny = true;
		break;
	    }
	}

    }

    switch(self->config.order) {
	case CCK_ACL_PERMIT_DENY:
	    if(!matchPermit) {
		ret = false;
		break;
	    }
	    if(matchDeny) {
		ret = false;
		break;
	    }
	    ret = true;
	    break;
		default:
	case CCK_ACL_DENY_PERMIT:
	    if (matchDeny && !matchPermit) {
		ret = false;
		break;
	    } else {
		ret = true;
		break;
	    }
    }

gameover:

    if(ret) {
	self->counters.passed++;
    } else {
	self->counters.dropped++;
    }

#ifdef CCK_DEBUG
{
	CckAddressToolset *tools = getAddressToolset(self->type);
	if(tools) {
	    tmpstr(strAddrf, tools->strLen);
		CCK_DBG(THIS_COMPONENT"aclMatch(%s): ACL %s message from %s: permit %smatch, deny %smatch\n",
		    self->name, ret ? "permitted" : "dropped",
		    tools->toString(strAddrf, strAddrf_len, address),
		    matchPermit ?  "" : "no ", matchDeny ? "" : "no ");
	}
}
#endif

    return ret;
}

static int aclParse(CckAcl *self, CckAclEntry **table, const char *line, const bool quiet) {


    int count = 0;

    if(!strlen(line)) {
	goto gameover;
    }

    foreach_token_begin(acl, line, item, DEFAULT_TOKEN_DELIM) 
	if(self->_parseEntry(self, NULL, item, quiet)) {
	    count++;
	}
    foreach_token_end(acl)

    if(!count) {
	goto gameover;
    }

    if(table) {
	int found = 0;
	*table = calloc(count, sizeof(CckAclEntry));
	
	foreach_token_begin(acl, line, item, DEFAULT_TOKEN_DELIM)

	    if(self->_parseEntry(self, &(*table)[found], item, quiet)) {
		found++;
	    }

	foreach_token_end(acl)

    }


gameover:

    CCK_DBG(THIS_COMPONENT"aclParse(%s): count: %d\n", self->name, count);

    return count;

}

static bool
aclCompile (CckAcl *self, const char* permitList, const char* denyList, const uint8_t order) {


    self->reset(self);

    self->_permitData = NULL;
    self->_denyData = NULL;

    CCK_DBG(THIS_COMPONENT"aclCompile(%s): parsing permit list\n", self->name);
    self->_permitCount = aclParse(self, &self->_permitData, permitList, true);
    CCK_DBG(THIS_COMPONENT"aclCompile(%s): parsing deny list\n", self->name);
    self->_denyCount = aclParse(self, &self->_denyData, denyList, true);

    self->_populated = (self->_permitCount || self->_denyCount);

    self->config.order = order;
    return true;

}

static bool
aclTest (CckAcl *self, const char* permitList, const char* denyList, const uint8_t order, const bool quiet) {

    bool ret = true;

    int permitTokens = countTokens(permitList, DEFAULT_TOKEN_DELIM, true);
    int denyTokens = countTokens(denyList, DEFAULT_TOKEN_DELIM, true);

    int permitParsed, denyParsed;

    permitParsed = aclParse(self, NULL, permitList, false);
    if(permitTokens != permitParsed ) {
	ret = false;
	if(!quiet) {
	    CCK_ERROR(THIS_COMPONENT"aclTest(%s): Error(s) while parsing permit list\n", self->name);
	}

    }

    denyParsed = aclParse(self, NULL, denyList, false);
    if(denyTokens != denyParsed) {
	ret = false;
	if(!quiet) {
	    CCK_ERROR(THIS_COMPONENT"aclTest(%s): Error(s) while parsing deny list\n", self->name);
	}
    }

    if(order != CCK_ACL_PERMIT_DENY && order != CCK_ACL_DENY_PERMIT) {
	ret = false;
	if(!quiet) {
	    CCK_ERROR(THIS_COMPONENT"aclTest(%s): Unknown ACL order %d, cannot parse\n", self->name, order);
	}
    }

    return ret;

}

bool
testCckAcl(int type, const char* permitList, const char* denyList, const uint8_t order, const bool quiet) {

    bool ret = false;

    CckAcl *tacl = createCckAcl(type, "__test");

    if(!tacl) {
	if(!quiet) {
	    CCK_ERROR(THIS_COMPONENT"testAcl(): Could not create ACL component!\n");
	}
	return false;
    }

    ret = tacl->test(tacl, permitList, denyList, order, quiet);

    freeCckAcl(&tacl);

    return ret;
}
