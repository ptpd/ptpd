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
 * @file   acl_ipv4.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  IPv4 ACL implementation
 *
 */

#include <config.h>

#include <errno.h>

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/transport_address.h>
#include <libcck/net_utils.h>
#include <libcck/acl.h>
#include <libcck/acl_interface.h>

#define THIS_COMPONENT "acl.ipv4: "

static int cckAcl_init(CckAcl* self);


/* tracking the number of instances */
static int _instanceCount = 0;

bool
_setupCckAcl_ipv4(CckAcl *self)
{

    INIT_INTERFACE(self);

    self->_instanceCount = &_instanceCount;

    _instanceCount++;

    return true;

}

static int
cckAcl_init(CckAcl* self) {

    self->_init = true;
    return 1;

}

static bool
_parseEntry (CckAcl *self, CckAclEntry* entry, const char *line, const bool quiet) {

    CckAddressToolset *tools = getAddressToolset(self->type);
    bool ret = true;
    CckAclData_ipv4 data;
    CckTransportAddress tmpAddr, maskAddr;

    if(!tools) {
	return false;
    }

    clearTransportAddress(&tmpAddr);
    clearTransportAddress(&maskAddr);

    data.network = 0;
    data.netmask = 32;
    data.bitmask = ~0;

    foreach_token_begin(tmp, line, section, "/");

	switch(counter_tmp) {
	    case 0:
		if(!tools->fromString(&tmpAddr, section)) {
		    ret = false;
		}
		break;
	    case 1:
		if(!tools->fromString(&maskAddr, section)) {
		    ret = false;
		    break;
		}

		uint32_t mask = ntohl(*(uint32_t*)tools->getData(&maskAddr));

		if(mask <= 32) {
		    data.netmask = mask;
		    if(mask < 32) {
			data.bitmask &= ~ (data.bitmask >> data.netmask);
		    }
		} else {
		    data.bitmask = ntohl(maskAddr.addr.inet4.sin_addr.s_addr);
		}

		data.network = ntohl(tmpAddr.addr.inet4.sin_addr.s_addr) & data.bitmask;
		tmpAddr.addr.inet4.sin_addr.s_addr = htonl(data.network);

		break;
	    default:
		break;

	};

    foreach_token_end(tmp);

    maskAddr.addr.inet4.sin_addr.s_addr = htonl(data.bitmask);

    if(!counter_tmp) {
	return false;
    }

    tmpstr(strAddr, tools->strLen);
    tmpstr(strMaskAddr, tools->strLen);

    if(ret) {
	CCK_DBG(THIS_COMPONENT"(%s).parseEntry(%s): resolved to %s/%s\n", self->name, line,
	    tools->toString(strAddr, strAddr_len, &tmpAddr),
	    tools->toString(strMaskAddr, strAddr_len, &maskAddr));
	/* allocate private data, copy contents */
	if(entry) {
	    CCK_INIT_PDATA(CckAcl, ipv4, entry);
	    CCK_GET_PDATA(CckAcl, ipv4, entry, myData);
	    memcpy(myData, &data, sizeof(data));
	}

    } else {
	if(quiet) {
	    CCK_ERROR(THIS_COMPONENT"(%s).parseEntry(%s): failed to parse entry\n", self->name, line);
	} else {
	    CCK_DBG(THIS_COMPONENT"(%s).parseEntry(%s): failed to parse entry\n", self->name, line);
	}

    }

    return ret;

}

static bool
_matchEntry (CckAcl *self, CckAclEntry* entry, CckTransportAddress *address) {

    uint32_t addr = ntohl(address->addr.inet4.sin_addr.s_addr);

    CCK_GET_PDATA(CckAcl, ipv4, entry, data);

    bool ret = (data->network == (addr & data->bitmask));

    entry->hitCount += ret;

    return ret;

}

static char*
_dumpEntry (CckAclEntry* entry, char *buf, const int len) {

    CckAddressToolset *tools = getAddressToolset(TT_FAMILY_IPV4);
    tmpstr(strAddr, tools->strLen);
    tmpstr(strMaskAddr, tools->strLen);
    CCK_GET_PDATA(CckAcl, ipv4, entry, data);

    CckTransportAddress netAddr, maskAddr;
    clearTransportAddress(&netAddr);
    clearTransportAddress(&maskAddr);

    netAddr.addr.inet4.sin_addr.s_addr = htonl(data->network);
    maskAddr.addr.inet4.sin_addr.s_addr = htonl(data->bitmask);

    snprintf(buf, len, "%s/%s",
	    tools->toString(strAddr, strAddr_len, &netAddr),
	    tools->toString(strMaskAddr, strAddr_len, &maskAddr));

    return buf;

}
