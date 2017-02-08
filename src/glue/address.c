/*-
 * Copyright (c) 2016      Wojciech Owczarek
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
 * @file   address.c
 * @date   Thu Nov 24 23:19:00 2016
 *
 * @brief  Address management wrappers between PTPd / lib1588 and libCCK
 *
 *
 */

#include <string.h>
#include <math.h>

#include "cck_glue.h"

#include <libcck/cck_utils.h>
#include <libcck/transport_address.h>


int
myAddrStrLen(void *input)
{

    if(input == NULL) {
	return 0;
    }

    CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	    return 0;
    }

    return ts->strLen;

}

int
myAddrDataSize(void *input)
{

    if(input == NULL) {
	return 0;
    }

    CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	    return 0;
    }

    return ts->addrSize;

}

char *
myAddrToString(char *buf, const int len, void *input)
{

    if(input == NULL) {
	return NULL;
    }

    const CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	    return NULL;
    }

    return ts->toString(buf, len, addr);

}

uint32_t
myAddrHash(void *input, const int modulo)
{

    if(input == NULL) {
	return 0;
    }

    const CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	    return 0;
    }

    return ts->hash(addr, modulo);

}

int
myAddrCompare(void *a, void *b)
{

    CckTransportAddress *pa = a;

    CckAddressToolset *ts = getAddressToolset(pa->family);

    if(ts == NULL) {
	    return 0;
    }

    return ts->compare(a, b);

}

bool
myAddrIsEmpty(void *input)
{

    if(input == NULL) {
	return true;
    }

    const CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {

	    return true;
    }

    return ts->isEmpty(addr);

}

bool
myAddrIsMulticast(void *input)
{

    if(input == NULL) {
	return false;
    }

    const CckTransportAddress *addr = input;
    CckAddressToolset *ts = getAddressToolset(addr->family);

    if(ts == NULL) {
	    return false;
    }

    return ts->isMulticast(addr);

}

void
myAddrClear(void *input)
{

    if(input == NULL) {
	return;
    }

    CckTransportAddress *addr = input;

    clearTransportAddress(addr);

}

void
myAddrCopy(void *dst, void *src)
{

    CckTransportAddress *pdst = dst;
    CckTransportAddress *psrc = src;

    copyCckTransportAddress(pdst, psrc);

}

void*
myAddrDup(void *src)
{

    CckTransportAddress *addr = src;

    return (void*)duplicateCckTransportAddress(addr);

}

void
myAddrFree(void **input)
{

    CckTransportAddress **addr = (CckTransportAddress **)input;

    freeCckTransportAddress(addr);

}

int
myAddrSize()
{
    return sizeof(CckTransportAddress);
}

void*
myAddrGetData(void *input)
{
    CckTransportAddress *addr = input;
    return getAddressData(addr);

}



