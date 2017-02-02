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
 * @file   acl_interface.h
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  ACL API function declarations and macros for inclusion by
 *         ACL imlpementations
 *
 */


/* public + private interface - implementations must implement all of those */

/* parse ACL entry, if NULL given, just test */
static bool _parseEntry (CckAcl *self, CckAclEntry* entry, const char *line, const bool quiet);
/* match a transport address against ACL entry */
static bool _matchEntry	(CckAcl *self, CckAclEntry* entry, CckTransportAddress *address);
/* dump an ACL entry */
static char *_dumpEntry	(CckAclEntry* entry, char *buf, const int len);

/* public + private interface end */

#define INIT_INTERFACE(var)\
    var->init = cckAcl_init;\
    var->_parseEntry = _parseEntry;\
    var->_matchEntry = _matchEntry;\
    var->_dumpEntry = _dumpEntry;
