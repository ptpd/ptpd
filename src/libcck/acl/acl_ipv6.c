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
 * @file   acl_ipv6.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  IPv6 ACL implementation
 *
 */

#include <config.h>

#include <errno.h>
#include <stdlib.h>

#include <libcck/cck.h>
#include <libcck/cck_types.h>
#include <libcck/cck_utils.h>
#include <libcck/cck_logger.h>
#include <libcck/transport_address.h>
#include <libcck/net_utils.h>
#include <libcck/acl.h>
#include <libcck/acl_interface.h>

#define THIS_COMPONENT "acl.ipv6: "

static int cckAcl_init(CckAcl* self);


/* tracking the number of instances */
static int _instanceCount = 0;

bool
_setupCckAcl_ipv6(CckAcl *self)
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
    int maskwidth = 8 * tools->addrSize;
    char *endptr = NULL;
    bool ret = true;
    CckAclData_ipv6 data;
    CckTransportAddress tmpAddr, maskAddr;

    if(!tools) {
	return false;
    }

    clearTransportAddress(&tmpAddr);
    clearTransportAddress(&maskAddr);

    memset(&data, 0, sizeof(data));

    /* default mask width = host mask */
    data.netmask = maskwidth;

    memset((void*)data.bitmask, 0xff, sizeof(data.bitmask));

    foreach_token_begin(tmp, line, section, "/");

	switch(counter_tmp) {
	    case 0:
		if(!tools->fromString(&tmpAddr, section)) {
		    ret = false;
		}
		break;
	    case 1:

		data.netmask = strtoul(section, &endptr, 10);

		if(data.netmask == 0) {
		    if(errno || (endptr == section)) {
			ret = false;
		    }
		}

		if(data.netmask > maskwidth) {
		    ret = false;
		}

		break;
	    default:
		break;

	};

    foreach_token_end(tmp);


    if(!counter_tmp) {
	return false;
    }

    tmpstr(strAddr, tools->strLen);

    if(ret) {

	setBufferBits(data.bitmask, tools->addrSize, maskwidth - data.netmask, 0);
	andBuffer(data.network, tools->getData(&tmpAddr), data.bitmask, tools->addrSize);
	andBuffer(tools->getData(&tmpAddr), tools->getData(&tmpAddr), data.bitmask, tools->addrSize);

	CCK_DBG(THIS_COMPONENT"(%s).parseEntry(%s): resolved to %s/%d\n", self->name, line,
	    tools->toString(strAddr, strAddr_len, &tmpAddr),
	    data.netmask);
	/* allocate private data, copy contents */
	if(entry) {
	    CCK_INIT_PDATA(CckAcl, ipv6, entry);
	    CCK_GET_PDATA(CckAcl, ipv6, entry, myData);
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

    CckAddressToolset *tools = getAddressToolset(self->type);
    unsigned char buf[tools->addrSize];

    CCK_GET_PDATA(CckAcl, ipv6, entry, data);

    andBuffer(buf, tools->getData(address), data->bitmask, tools->addrSize);

    bool ret = !memcmp(buf, data->network, tools->addrSize);

    entry->hitCount += ret;

    return ret;

}

static char*
_dumpEntry (CckAclEntry* entry, char *buf, const int len) {

    CckAddressToolset *tools = getAddressToolset(TT_FAMILY_IPV6);
    tmpstr(strAddr, tools->strLen);
    tmpstr(strMaskAddr, tools->strLen);
    CCK_GET_PDATA(CckAcl, ipv6, entry, data);

    CckTransportAddress netAddr, maskAddr;
    clearTransportAddress(&netAddr);
    clearTransportAddress(&maskAddr);

    netAddr.family = TT_FAMILY_IPV6;
    maskAddr.family = TT_FAMILY_IPV6;

    netAddr.addr.inet.sa_family = AF_INET6;
    maskAddr.addr.inet.sa_family = AF_INET6;

    memcpy(tools->getData(&netAddr), data->network, tools->addrSize);
    memcpy(tools->getData(&maskAddr), data->bitmask, tools->addrSize);

    snprintf(buf, len, "%s/%d",
	    tools->toString(strAddr, strAddr_len, &netAddr),
	    data->netmask);

    return buf;

}
