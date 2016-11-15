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
 * @file   log_handler.h
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  log handler definitions
 *
 */

#ifndef CCK_LOG_HANDLER_H_
#define CCK_LOG_HANDLER_H_

#include <syslog.h>

#define CCK_DEBUG

#define CCK_EMERGENCY(x, ...) printf("LibCCK EMERG   : "x, ##__VA_ARGS__)
#define CCK_ALERT(x, ...)     printf("LibCCK ALERT   : "x, ##__VA_ARGS__)
#define CCK_CRITICAL(x, ...)  printf("LibCCK CRIT    : "x, ##__VA_ARGS__)
#define CCK_ERROR(x, ...)     printf("LibCCK ERR     : "x, ##__VA_ARGS__)
#define CCK_PERROR(x, ...)    printf("LibCCK ERR     : "x "      (strerror: %m)\n", ##__VA_ARGS__)
#define CCK_WARNING(x, ...)   printf("LibCCK WARNING : "x, ##__VA_ARGS__)
#define CCK_NOTIFY(x, ...)    printf("LibCCK NOTICE  : "x, ##__VA_ARGS__)
#define CCK_NOTICE(x, ...)    printf("LibCCK NOTICE  : "x, ##__VA_ARGS__)
#define CCK_INFO(x, ...)      printf("LibCCK INFO    : "x, ##__VA_ARGS__)

#ifdef CCK_DEBUG

#define CCK_DBG(x, ...)       printf("LibCCK DBG     : "x, ##__VA_ARGS__)
#define CCK_DBG2(x, ...)      printf("LibCCK DBG2    : "x, ##__VA_ARGS__)
#define CCK_DBGV(x, ...)      printf("LibCCK DBGV    : "x, ##__VA_ARGS__)

#else

#define CCK_DBG(x, ...)
#define CCK_DBG2(x, ...)
#define CCK_DBGV(x, ...)

#endif




#endif /* CCK_LOG_HANDLER_H_ */
