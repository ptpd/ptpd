/* Copyright (c) 2016-2017 Wojciech Owczarek,
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
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  libCCK global header
 *
 */

#ifdef __sun
#	ifndef _XPG6
#		define _XPG6
#	endif /* _XPG6 */
#	ifndef _XOPEN_SOURCE
#		define _XOPEN_SOURCE 500
#	endif /* _XOPEN_SOURCE */
#	ifndef __EXTENSIONS__
#		define __EXTENSIONS__
#	endif /* __EXTENSIONS */
#endif /* __sun */

#ifndef CCK_H_
#define CCK_H_

#define CCK_API_VER 1.1
#define CCK_API_VER_STR "1.1"
#define CCK_API_INFO_STR "\tCopyright (c) 2014-2017: Wojciech Owczarek <wojciech@owczarek.co.uk>\n"\
			 "\tDistributed under BSD 2-Clause Licence"

#include <stdio.h>
#include <stdlib.h>

#include <limits.h>


#define CCKCALLOC(ptr, size) \
        if(!((ptr)=calloc(1, size))) { \
                printf("Critical: failed to allocate %lu bytes of memory.memory", (unsigned long)size); \
                exit(1); \
        }

#define CCK_INIT_PDATA(comptype, impl, self) \
    if(self->_privateData == NULL) { \
	CCKCALLOC(self->_privateData, sizeof(comptype ## Data_##impl)); \
    }
#define CCK_INIT_PCONFIG(comptype, impl, self) \
    if(self->_privateConfig == NULL) { \
	CCKCALLOC(self->_privateConfig, sizeof(comptype ##Config_##impl)); \
    }
#define CCK_INIT_EXTDATA(comptype, extimpl, self) \
    if(self->_extData == NULL) { \
	CCKCALLOC(self->_extData, sizeof(comptype ##ExtData_##extimpl)); \
    }

#define CCK_GET_PCONFIG(comptype, impl, self, var) \
    comptype ##Config_##impl *var = (comptype ##Config_##impl*)self->_privateConfig;

#define CCK_GET_PDATA(comptype, impl, self, var) \
    comptype ##Data_##impl *var = (comptype ##Data_##impl*)self->_privateData;

#define CCK_GET_EXTDATA(comptype, extimpl, self, var) \
    comptype ##ExtData_##extimpl *var = (comptype ##ExtData_##extimpl*)self->_extData;

#endif /* CCK_H_ */
