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
 * @file   cck_dummy.h
 *
 * @brief  libCCK dummy component definitions
 *
 */

#ifndef CCK_DUMMY_H_
#define CCK_DUMMY_H_

#ifndef CCK_H_INSIDE_
#error libCCK component headers should not be uncluded directly - please include cck.h only.
#endif

typedef struct CckDummy CckDummy;

#include "cck.h"

#include <config.h>

/* Example data type provided by the component */
typedef struct {

    int param1;

} CckDummyStruct;

/*
  Configuration data for a dummy - some parameters are generic,
  so it's up to the dummy implementation how they are used.
  This is easier to use than per-implementation config, although
  this is still possible using configData.
*/
typedef struct {
	/* we probably don't need this, this is passed after create() */
	int dummyType;
	/* generic int type dummy specific parameter #1 - ttl for IP dummys */
	int param1;
	/* generic pointer - implementations can make use of it */
	void* configData;
} CckDummyConfig;


struct CckDummy {
	/* mandatory - this has to be the first member */
	CckComponent header;

	int dummyType;
	CckDummyConfig config;

	int param1;

	/* container for implementation-specific data */
	void* dummyData;

	/* Example callback that can be assigned by the user */

	void (*dummyCallback) (CckBool, int*);

	/* Dummy interface */

	/* === libCCK interface mandatory methods begin ==== */

	/* shut the component down */
	int (*shutdown) (void*);

	/* === libCCK interface mandatory methods end ==== */

	/* start up the dummy, perform whatever initialisation */
	int (*init) (CckDummy*, const CckDummyConfig*);
	/* example other public method */
	CckBool (*doSomething) (CckDummy*, int);
	/* example other public method demonstrating callback use*/
	CckBool (*doSomethingWithCallback) (CckDummy*, int);

};

/* libCCk core functions */
CckDummy*  createCckDummy(int dummyType, const char* instanceName);
void       setupCckDummy(CckDummy* dummy, int dummyType, const char* instanceName);
void       freeCckDummy(CckDummy** dummy);

/* any additional utility functions provided by the component */
void clearDummyStruct(CckDummyStruct* dummyStruct);

/* ============ DUMMY IMPLEMENTATIONS BEGIN ============ */

/* dummy type enum */
enum {
	CCK_DUMMY_NULL = 0,
	CCK_DUMMY_DUMMY
};

/* dummy implementation headers */

#include "dummy/cck_dummy_null.h"
#include "dummy/cck_dummy_dummy.h"

/* ============ DUMMY IMPLEMENTATIONS END ============== */

#endif /* CCK_DUMMY_H_ */
