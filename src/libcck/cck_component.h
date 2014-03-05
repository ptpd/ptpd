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
 * @file   cck_component.h
 *
 * @brief  libCCK base component and registry definition
 *
 */

#ifndef CCK_COMPONENT_H_
#define CCK_COMPONENT_H_

#ifndef CCK_H_INSIDE_
#error libCCK component headers should not be uncluded directly - please include cck.h only.
#endif


typedef struct CckComponent CckComponent;

#include "cck.h"

#define CCK_MAX_INSTANCE_NAME_LEN 15

enum {

	CCK_COMPONENT_NULL = 0,
	CCK_COMPONENT_LOGHANDLER,
        CCK_COMPONENT_ACL,
	CCK_COMPONENT_TRANSPORT

};



struct CckComponent {

	int componentType;
	CckComponent* _prev;
	CckComponent* _next;
	CckUInt32 serial;
	char instanceName[CCK_MAX_INSTANCE_NAME_LEN + 1];
	CckBool isInitialized;
	CckBool isShutdown;
	int (*shutdown) (void*);

};

typedef struct {

	CckComponent* _first;
	CckComponent* _last;
	CckUInt32 lastSerial;
	int componentCount;

} CckRegistry;

CckBool cckInitialised(void);
CckBool cckInit(void);
CckBool cckShutdown(void);

CckBool cckRegister(void* _comp);
CckBool cckDeregister(void* _comp);


#endif /* CCK_COMPONENT_H_ */
