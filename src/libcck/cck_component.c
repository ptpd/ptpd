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
 * @file   cck_component.c
 *
 * @brief  libCCK base component registry handling code
 *
 */

#include "cck.h"
//#include "cck_component.h"

CckRegistry* cckRegistry = NULL;

CckBool
cckInit()
{
    CCK_DBG("libCCk version "CCK_VERSION_STRING" (API "CCK_API_VERSION_STRING
	    ") (c) 2014: Wojciech Owczarek, ptpd project - starting up\n");
    CCKCALLOC(cckRegistry,sizeof(CckRegistry));
    cckRegistry->_first = NULL;
    cckRegistry->_last = NULL;
    CCK_DBG("libCCk version "CCK_VERSION_STRING" succesfully initialised\n");
    return CCK_TRUE;

}

CckBool
cckShutdown()
{

    CckComponent* marker;

    CCK_DBG("cckShutdown() called\n");

    if(cckRegistry == NULL) {
	CCK_ERROR("LibCCk not initialised - cannot shutdown\n");
	return CCK_FALSE;
    }

    if(cckRegistry->_first == NULL) {

	CCK_DBG("No components registered - instant shutdown\n");

    }
	else

    for (marker=cckRegistry->_last; marker != NULL;) {

	CCK_DBG("Shutting down component serial %08x type 0x%02x name \"%s\"\n", marker->serial, marker->componentType,
								marker->instanceName);
	marker->shutdown(marker);
	cckDeregister(marker);
	free(marker);
	marker = cckRegistry->_last;

    }

    free(cckRegistry);
    cckRegistry = NULL;

    return CCK_TRUE;

}

CckBool
cckRegister(void* _comp)
{

    CckComponent* component = _comp;

    if(cckRegistry == NULL) {

	CCK_ERROR("Registry not initialised - cannot register component\n");
	return CCK_FALSE;

    }

    if(cckRegistry->_first == NULL) {

	cckRegistry->_first = component;
	cckRegistry->_last = component;

    } else if(cckRegistry->_last == NULL) {

	CCK_ERROR("Cannot register component - last component is empty\n");
	return CCK_FALSE;

    } else {

	cckRegistry->_last->_next = component;
	component->_prev = cckRegistry->_last;
	cckRegistry->_last = component;

    }

    component->serial = ++cckRegistry->lastSerial;

    cckRegistry->componentCount++;

    CCK_DBG("Registered component serial %08x type 0x%02x name \"%s\"\n", component->serial,
	    component->componentType, component->instanceName);

    return CCK_TRUE;

}

/* Remove a component from the registy while maintaining linked list continuity */
CckBool
cckDeregister(void* _comp)
{

    CckComponent* component = _comp;

    CckComponent *tmpPrev, *tmpNext;

    if(component == NULL) {
	CCK_DBG("cckDeregister called for an empty component\n");
	return CCK_FALSE;
    }

    CCK_DBG("cckDeregister called for component serial %08x\n", component->serial);

    tmpPrev = component->_prev;
    tmpNext = component->_next;

    if(tmpPrev != NULL) {
	tmpPrev->_next = tmpNext;
    } else {
	cckRegistry->_first = tmpPrev;
    }

    if(tmpNext != NULL) {
	tmpNext->_prev = tmpPrev;
    } else {
	cckRegistry->_last = tmpPrev;
    }

    cckRegistry->componentCount--;

    CCK_DBG("Deregistered component serial %08x type 0x%02x name \"%s\"\n", 
	    component->serial, component->componentType, component->instanceName);

    return CCK_TRUE;

}

