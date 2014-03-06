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
 * @file   cck_dummy.c
 * 
 * @brief  constructor and destructor for dummy component, additional helper functions
 *
 */

#include "cck.h"

/* Quick shortcut to call the setup function for different implementations */
#define CCK_REGISTER_DUMMY(type,suffix) \
	if(dummyType==type) {\
	    cckDummySetup_##suffix(dummy);\
	}


CckDummy*
createCckDummy(int dummyType, const char* instanceName)
{

    CckDummy* dummy;

    CCKCALLOC(dummy, sizeof(CckDummy));

    dummy->header._next = NULL;
    dummy->header._prev = NULL;
    dummy->header._dynamic = CCK_TRUE;

    setupCckDummy(dummy, dummyType, instanceName);

    return dummy;
}

void
setupCckDummy(CckDummy* dummy, int dummyType, const char* instanceName)
{

    dummy->dummyType = dummyType;

    strncpy(dummy->header.instanceName, instanceName, CCK_MAX_INSTANCE_NAME_LEN);

    dummy->header.componentType = CCK_COMPONENT_DUMMY;

/* 
   if you write a new dummy,
   make sure you register it here. Format is:

   CCK_REGISTER_DUMMY(DUMMY_TYPE, function_name_suffix)

   where suffix is the string the setup() function name is
   suffixed with for your implementation, i.e. you
   need to define cckDummySetup_bob for your "bob" dummy.

*/

/* ============ DUMMY IMPLEMENTATIONS BEGIN ============ */

    CCK_REGISTER_DUMMY(CCK_DUMMY_NULL, null);
    CCK_REGISTER_DUMMY(CCK_DUMMY_DUMMY, dummy);

/* ============ DUMMY IMPLEMENTATIONS END ============== */

/* we 're done with this macro */
#undef CCK_REGISTER_DUMMY

    dummy->header._next = NULL;
    dummy->header._prev = NULL;

    cckRegister(dummy);

}


void
freeCckDummy(CckDummy** dummy)
{

    if(*dummy==NULL)
	    return;

    (*dummy)->shutdown(*dummy);

    cckDeregister(*dummy);

    if((*dummy)->header._dynamic) {
	free(*dummy);
	*dummy = NULL;
    }
}


/* Zero out DummyStruct structure */
void
clearDummyStruct(CckDummyStruct* dummyStruct)
{

    memset(dummyStruct, 0, sizeof(CckDummyStruct));

}

