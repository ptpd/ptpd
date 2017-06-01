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
 * @file   cck_loger.h
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  log handler definitions
 *
 */

#ifndef CCK_LOGGER_H_
#define CCK_LOGGER_H_

#include <config.h>

#include <syslog.h>
#include <stdarg.h>

void cckLogMessage(int level, const char *format, ...);
void cckDefaultLoggerFun (int level, const char *format, va_list ap);
void cckSetLoggerFun(void (*) (int, const char*, va_list));

/* Solaris, maybe others */
#ifndef LOG_PRIMASK
#define LOG_PRIMASK 0x07
#endif
#ifndef LOG_PRI
#define LOG_PRI(prio) ((prio) & LOG_PRIMASK)
#endif

#define CCK_EMERGENCY(x, ...)		cckLogMessage(LOG_EMERG, x, ##__VA_ARGS__);
#define CCK_ALERT(x, ...)		cckLogMessage(LOG_ALERT, x, ##__VA_ARGS__);
#define CCK_CRITICAL(x, ...)		cckLogMessage(LOG_CRIT, x, ##__VA_ARGS__);
#define CCK_ERROR(x, ...)		cckLogMessage(LOG_ERR, x, ##__VA_ARGS__);
#define CCK_PERROR(x, ...)    		cckLogMessage(LOG_ERR, x "      (strerror: %m)\n", ##__VA_ARGS__);
#define CCK_WARNING(x, ...)		cckLogMessage(LOG_WARNING, x, ##__VA_ARGS__);
#define CCK_NOTICE(x, ...)		cckLogMessage(LOG_NOTICE, x, ##__VA_ARGS__);
#define CCK_INFO(x, ...) 		cckLogMessage(LOG_INFO, x, ##__VA_ARGS__);

#define CCK_QERROR(x, ...)		if(!quiet) { cckLogMessage(LOG_ERR, x, ##__VA_ARGS__); };
#define CCK_QPERROR(x, ...)    		if(!quiet) { cckLogMessage(LOG_ERR, x "      (strerror: %m)\n", ##__VA_ARGS__); };
#define CCK_QWARNING(x, ...)		if(!quiet) { cckLogMessage(LOG_WARNING, x, ##__VA_ARGS__); };
#define CCK_QNOTICE(x, ...)		if(!quiet) { cckLogMessage(LOG_NOTICE, x, ##__VA_ARGS__); };
#define CCK_QINFO(x, ...) 		if(!quiet) { cckLogMessage(LOG_INFO, x, ##__VA_ARGS__); };

#ifdef CCK_DEBUG

#define CCK_DBG(x, ...)			cckLogMessage(LOG_DEBUG, x, ##__VA_ARGS__)
#define CCK_DBG2(x, ...)		cckLogMessage(LOG_DEBUG, x, ##__VA_ARGS__)
#define CCK_DBGV(x, ...)		cckLogMessage(LOG_DEBUG, x, ##__VA_ARGS__)

#else

#define CCK_DBG(x, ...)
#define CCK_DBG2(x, ...)
#define CCK_DBGV(x, ...)

#endif

#endif /* CCK_LOGGER_H_ */
