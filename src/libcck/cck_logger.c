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
 * @file   cck_logger.c
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  libCCK log writer with user override
 *
 */

#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <libcck/cck_logger.h>

static void (*cckLoggerFun) (int, const char *, va_list) = cckDefaultLoggerFun;

static const char* getLogPriorityName(int priority);

void
cckLogMessage(int priority, const char* format, ...) {

    va_list ap;
    va_start(ap, format);

    cckLoggerFun(priority, format, ap);

    va_end(ap);

}


void
cckSetLoggerFun(void (*fun) (int, const char*, va_list)) {

    if(fun) {
	cckLoggerFun = fun;
    }

}

void
cckDefaultLoggerFun(int priority, const char *format, va_list ap) {

	fprintf(stderr, "libcck.%s: ", getLogPriorityName(priority));
	vfprintf(stderr, format, ap);

}

static const char*
getLogPriorityName(int priority) {

    priority=LOG_PRI(priority);

    switch (priority) {
	case	LOG_EMERG:
	    return		"emergency";
	case	LOG_ALERT:
	    return		"alert    ";
	case	LOG_CRIT:
	    return		"critical ";
	case	LOG_ERR:
	    return		"error    ";
	case	LOG_WARNING:
	    return		"warning  ";
	case	LOG_NOTICE:
	    return		"notice   ";
	case	LOG_INFO:
	    return		"info     ";
	case	LOG_DEBUG:
	    return		"debug    ";
	default:
	    return 		"unknown  ";
    }

}
