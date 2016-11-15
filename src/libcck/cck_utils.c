/* Copyright (c) 2016 Wojciech Owczarek,
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
 * @file   cck_utils.c
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  General helper functions used by libCCK
 *
 */

#include <ctype.h>
#include <libcck/cck_utils.h>

CckU32
getFnvHash(const void *input, const size_t len, const int modulo)
{

    static const uint32_t prime = 16777619;
    static const uint32_t basis = 2166136261;

    int i = 0;

    uint32_t hash = basis;
    uint8_t *buf = (uint8_t*)input;

    for(i = 0; i < len; i++)  {
        hash *= prime;
        hash ^= *(buf + i);
    }

    return (modulo > 0 ? hash % modulo : hash);

}

int
hexDigitToInt(const unsigned char digit)
{

    int c = tolower(digit);

    if((c >= 'a') && (c <= 'f')) {
	return (c - 'a' + 10);
    }

    if((c >= '0') && (c <= '9')) {
	return c - '0';
    }

    return -1;

}