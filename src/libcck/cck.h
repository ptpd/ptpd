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
 * @file   cck.h
 * 
 * @brief  libCCK main header file. Include this to start using libCCK
 *
 */

#ifndef CCK_H_
#define CCK_H_

/* So that other headers can check if they are being included from here */
#define CCK_H_INSIDE_

/* Version strings */
#define CCK_VERSION 0.2
#define CCK_VERSION_STRING "0.2"

#define CCK_API_VERSION 0.9
#define CCK_API_VERSION_STRING "0.9"


/* libCCK macros */

#define IN_RANGE(num, min,max) \
        (num >= min && num <= max)

/* Own xmalloc/calloc - printf until we have CckLogTarget component written and can log this proper */
#define CCKCALLOC(ptr, size) \
        if(!((ptr)=malloc(size))) { \
                printf("Critical: failed to allocate %ld bytes of memory.memory", size); \
                exit(1); \
        } \
        memset(ptr, 0, size);


/* libCCK type definitions */
#include "cck_types.h"

/* base component and registry definition */
#include "cck_component.h"

/* libCCK components */
#include "cck_loghandler.h"
#include "cck_transport.h"
#include "cck_acl.h"
#include "cck_dummy.h"

#undef CCK_H_INSIDE_

#endif /* CCK_H_ */
