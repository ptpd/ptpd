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
 * @file   acl_dummy.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  dummy ACL implementation
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

#define THIS_COMPONENT "acl.dummy: "

static int cckAcl_init(CckAcl* self);

/* tracking the number of instances */
static int _instanceCount = 0;

bool
_setupCckAcl_dummy(CckAcl *self)
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

    return true;

}

static bool
_matchEntry (CckAcl *self, CckAclEntry* entry, CckTransportAddress *address) {

    return true;

}

static char*
_dumpEntry (CckAclEntry* entry, char *buf, const int len) {

    return NULL;

}
