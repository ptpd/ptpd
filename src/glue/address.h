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
 * @file   address.h
 * @date   Thu Nov 24 23:19:00 2016
 *
 * @brief  Address management wrappers between PTPd / lib1588 and libCCK
 *
 *
 */

#ifndef _PTPD_CCK_GLUE_ADDRESS_H
#define _PTPD_CCK_GLUE_ADDRESS_H

#include <stdint.h>

/* address management */
int		myAddrStrLen(void *input);
int		myAddrDataSize(void *input);
char*		myAddrToString(char *buf, const int len, void *input);
uint32_t	myAddrHash(void *addr, const int modulo);
int		myAddrCompare(void *a, void *b);
bool		myAddrIsEmpty(void *input);
bool		myAddrIsMulticast(void *input);
void		myAddrClear(void *input);
void		myAddrCopy(void *dst, void *src);
void*		myAddrDup(void *src);
void		myAddrFree(void **input);
void*		myAddrGetData(void *input);
int		myAddrSize();

#endif /* _PTPD_CCK_GLUE_ADDRESS_H */

