/*-
 * libCCK - Clock Construction Kit
 *
 * Copyright (c) 2014 Wojciech Owczarek,
 *                    Eric Satterness,
 *                    National Instruments,
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
 * @file   cck_loghandler.h
 * 
 * @brief  libCCK log handler component definition
 *
 */

#ifndef CCK_LOGHANDLER_H_
#define CCK_LOGHANDLER_H_

#include <syslog.h>
#include <stdarg.h>


void setCckLogLevel(int logLevel);
#ifdef RUNTIME_DEBUG
void setCckDebugLevel(int debugLevel);
#endif
void logCckMessage(int priority, const char * format, ...);

// Syslog ordering. We define extra debug levels above LOG_DEBUG for internal use
// extended from <sys/syslog.h>
#define LOG_CCK_DEBUG1   7
#define LOG_CCK_DEBUG2   8
#define LOG_CCK_DEBUGV   9


#define CCK_EMERGENCY(x, ...) logCckMessage(LOG_EMERG, "LibCCK EMERG   : "x, ##__VA_ARGS__)
#define CCK_ALERT(x, ...)     logCckMessage(LOG_ALERT, "LibCCK ALERT   : "x, ##__VA_ARGS__)
#define CCK_CRITICAL(x, ...)  logCckMessage(LOG_CRIT, "LibCCK CRIT    : "x, ##__VA_ARGS__)
#define CCK_ERROR(x, ...)     logCckMessage(LOG_ERR, "LibCCK ERR     : "x, ##__VA_ARGS__)
#define CCK_PERROR(x, ...)    logCckMessage(LOG_ERR, "LibCCK ERR     : "x "      (strerror: %m)\n", ##__VA_ARGS__)
#define CCK_WARNING(x, ...)   logCckMessage(LOG_WARNING, "LibCCK WARNING : "x, ##__VA_ARGS__)
#define CCK_NOTIFY(x, ...)    logCckMessage(LOG_NOTICE, "LibCCK NOTICE  : "x, ##__VA_ARGS__)
#define CCK_NOTICE(x, ...)    logCckMessage(LOG_NOTICE, "LibCCK NOTICE  : "x, ##__VA_ARGS__)
#define CCK_INFO(x, ...)      logCckMessage(LOG_INFO, "LibCCK INFO    : "x, ##__VA_ARGS__)


#ifdef RUNTIME_DEBUG

#define CCK_DBG(x, ...)       logCckMessage(LOG_CCK_DEBUG1, "LibCCK DBG     : "x, ##__VA_ARGS__)
#define CCK_DBG2(x, ...)      logCckMessage(LOG_CCK_DEBUG2, "LibCCK DBG2    : "x, ##__VA_ARGS__)
#define CCK_DBGV(x, ...)      logCckMessage(LOG_CCK_DEBUGV, "LibCCK DBGV    : "x, ##__VA_ARGS__)

#else

#define CCK_DBG(x, ...)
#define CCK_DBG2(x, ...)
#define CCK_DBGV(x, ...)

#endif

#endif /* CCK_LOGHANDLER_H_ */
