/*-
 * libCCK - Clock Construction Kit
 *
 * Copyright (c) 2014 Eric Satterness,
 *               National Instruments,
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
 * @file   cck_loghandler.c
 *
 * @brief  handles logging messages from libcck
 *
 */

#include "cck.h"

static int cckLogLevel = LOG_CCK_DEBUGV;
#ifdef RUNTIME_DEBUG
static int cckDebugLevel = LOG_INFO;
#endif

/*
 * TODO: Create a cck instance structure and move the log and debug level
 * variables into it. Modify libcck functions and methods to pass around and use
 * the instance structure so that we can support multiple independent instances.
 */
void setCckLogLevel(int logLevel) {
   cckLogLevel = logLevel;
}

#ifdef RUNTIME_DEBUG
void setCckDebugLevel(int debugLevel) {
   cckDebugLevel = debugLevel;
}
#endif

/*
 * Prints a cck message if its priority is lower than the log/debug level
 * TODO: Expand libcck logging to support logging to files. Or a potentially
 * better fix would be to create a separate logging component that both ptp and
 * libcck can use.
 */
void
logCckMessage(int priority, const char * format, ...) {
   va_list ap;

#ifdef RUNTIME_DEBUG
   if ((priority >= LOG_DEBUG) && (priority > cckDebugLevel)) {
      return;
   }
#endif

   if (priority > cckLogLevel) {
      return;
   }

   va_start(ap, format);
   vprintf(format, ap);
   va_end(ap);
}
