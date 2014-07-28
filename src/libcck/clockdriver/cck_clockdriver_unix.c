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
 * @file   cck_clockdriver_unix.c
 *
 * @brief  libCCK clockDriver component clockDriver implementation - full implementatione example
 *
 */

#include "cck_clockdriver_unix.h"

#define CCK_THIS_TYPE CCK_CLOCKDRIVER_UNIX

/* interface (public) method definitions */
static int     cckClockDriverInit (CckClockDriver* self, const CckClockDriverConfig* config);
static int     cckClockDriverShutdown (void* component);

/* private method definitions (if any) */


/* implementations follow */

void
cckClockDriverSetup_unix(CckClockDriver* self)
{
	if(self->clockDriverType == CCK_THIS_TYPE) {
            self->clockDriverCallback = NULL;
	    self->init = cckClockDriverInit;
	    self->shutdown = cckClockDriverShutdown;
	    self->header.shutdown = cckClockDriverShutdown;
	} else {
	    CCK_WARNING("setup() called for incorrect component implementation: %02x, expected %02x\n",
			self->clockDriverType, CCK_THIS_TYPE);
	}
}

static int
cckClockDriverInit (CckClockDriver* self, const CckClockDriverConfig* config)
{

    /*
     * implementation-specific data - commented out to prevent compiler warning
     * with -Wunused-but-set-variable
     *
    CckClockDriverClockDriverData* data = NULL;
     */

    if(self == NULL) {
	CCK_ERROR("ClockDriver ClockDriver init called for an empty clockDriver\n");
	return -1;
    }

    if(config == NULL) {
	CCK_ERROR("ClockDriver ClockDriver init called with empty config\n");
	return -1;
    }

    /* Shutdown will be called on self object anyway, OK to exit without explicitly freeing self */
    CCKCALLOC(self->clockDriverData, sizeof(CckClockDriverClockDriverData));

    /*
    data = (CckClockDriverClockDriverData*)self->clockDriverData;
    */

    return 1;
}

static int
cckClockDriverShutdown (void* component)
{

    if(component == NULL)
	return -1;

    CckClockDriver* self = (CckClockDriver*)component;

    if(self->clockDriverData != NULL) {
	free(self->clockDriverData);
	self->clockDriverData = NULL;
    }

    return 1;

}

/*

static CckBool
cckClockDriverDoSomething (CckClockDriver* self, int param1)
{

    if(private1(self, &param1)) {

	return CCK_TRUE;

    }

    return CCK_FALSE;

}


static CckBool
cckClockDriverDoSomethingWithCallback (CckClockDriver* self, int param1)
{


    if(self->clockDriverCallback != NULL) {

	self->clockDriverCallback(CCK_FALSE, &param1);
	return CCK_TRUE;

    }

    return CCK_FALSE;

}

static int
private1(CckClockDriver* self, int* param1)
{

    self->param1 = 42;
    *param1 = 42;
    return -1;

}
*/

#undef CCK_THIS_TYPE
