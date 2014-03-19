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
 * @file   cck_clockdriver.c
 * 
 * @brief  constructor and destructor for clockdriver component, additional helper functions
 *
 */

#include "cck.h"

/* Quick shortcut to call the setup function for different implementations */
#define CCK_REGISTER_CLOCKDRIVER(type,suffix) \
	if(clockdriverType==type) {\
	    cckClockDriverSetup_##suffix(clockdriver);\
	}


CckClockDriver*
createCckClockDriver(int clockdriverType, const char* instanceName)
{

    CckClockDriver* clockdriver;

    CCKCALLOC(clockdriver, sizeof(CckClockDriver));

    clockdriver->header._next = NULL;
    clockdriver->header._prev = NULL;
    clockdriver->header._dynamic = CCK_TRUE;

    setupCckClockDriver(clockdriver, clockdriverType, instanceName);

    return clockdriver;
}

void
setupCckClockDriver(CckClockDriver* clockdriver, int clockdriverType, const char* instanceName)
{

    clockdriver->clockDriverType = clockdriverType;

    strncpy(clockdriver->header.instanceName, instanceName, CCK_MAX_INSTANCE_NAME_LEN);

    clockdriver->header.componentType = CCK_COMPONENT_CLOCKDRIVER;

/* 
   if you write a new clockdriver,
   make sure you register it here. Format is:

   CCK_REGISTER_CLOCKDRIVER(CLOCKDRIVER_TYPE, function_name_suffix)

   where suffix is the string the setup() function name is
   suffixed with for your implementation, i.e. you
   need to define cckClockDriverSetup_bob for your "bob" clockdriver.

*/

/* ============ CLOCKDRIVER IMPLEMENTATIONS BEGIN ============ */

    CCK_REGISTER_CLOCKDRIVER(CCK_CLOCKDRIVER_NULL, null);
    CCK_REGISTER_CLOCKDRIVER(CCK_CLOCKDRIVER_UNIX, unix);

/* ============ CLOCKDRIVER IMPLEMENTATIONS END ============== */

/* we 're done with this macro */
#undef CCK_REGISTER_CLOCKDRIVER

    clockdriver->header._next = NULL;
    clockdriver->header._prev = NULL;

    cckRegister(clockdriver);

}


void
freeCckClockDriver(CckClockDriver** clockdriver)
{

    if(*clockdriver==NULL)
	    return;

    (*clockdriver)->shutdown(*clockdriver);

    cckDeregister(*clockdriver);

    if((*clockdriver)->header._dynamic) {
	free(*clockdriver);
	*clockdriver = NULL;
    }
}


/* Zero out ClockDriverStruct structure */
void
clearClockDriverStruct(CckClockDriverStruct* clockdriverStruct)
{

    memset(clockdriverStruct, 0, sizeof(CckClockDriverStruct));

}

