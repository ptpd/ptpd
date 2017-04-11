/* Copyright (c) 2017 Wojciech Owczarek,
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
 * @file   socket_common.c
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  common code for socket-based transports
 *
 */

#include <libcck/cck.h>
#include <libcck/cck_logger.h>

#include <libcck/ttransport.h>
#include <libcck/ttransport/socket_common.h>

#define THIS_COMPONENT "ttransport.socket_common: "

bool
initTimestamping_socket_common(TTransport *self,  TTsocketTimestampConfig *config) {

	int level = SOL_SOCKET;
	int option;
	int val = 1;

	config->arrayIndex = 0;
	config->cmsgLevel = SOL_SOCKET;

/* find which timestamp options are available */

#if defined(SO_TIMESTAMPNS) /* begin ifdef block: select available SO_XXXX option */

	CCK_DBG(THIS_COMPONENT"initTimestamping(%s): using SO_TIMESTAMPNS\n", self->name);
	option = SO_TIMESTAMPNS;
	config->cmsgType = SCM_TIMESTAMPNS;
	config->elemSize = sizeof(struct timespec);
	config->convertTs = convertTimestamp_timespec;

#elif defined(SO_BINTIME)

	CCK_DBG(THIS_COMPONENT"initTimestamping(%s): using SO_BINTIME\n", self->name);
	option = SO_BINTIME;
	config->cmsgType = SCM_BINTIME;
	config->elemSize = sizeof(struct bintime);
	config->convertTs = convertTimestamp_bintime;

#elif defined(SO_TIMESTAMP)

	CCK_DBG(THIS_COMPONENT"initTimestamping(%s): using SO_TIMESTAMP\n", self->name);
	option = SO_TIMESTAMP;
	config->cmsgType = SCM_TIMESTAMP;
	config->elemSize = sizeof(struct timeval);
	config->convertTs = convertTimestamp_timeval;

#else /* no SO_XXXX option available */

#if defined(__QNXNTO__) && defined (CCK_EXPERIMENTAL) /* interpolation on QNX is OK, no need for a warning */

	CCK_DBG(THIS_COMPONENT"initTimestamping(%s): using QNX timestamp interpolation\n", self->name);

#else /* not QNX or not CCK_EXPERIMENTAL: warn about poor precision */

	CCK_WARNING("initTimestamping(%s): no timestamp API available, timestamps will be imprecise!\n",
		    self->name);

#endif

	/* we are using naive timestamps, that is it */
	config->naive = true;
	return true;

#endif /* end ifdef block */


	/* we have an option to try */

	if (setsockopt(self->myFd.fd, level, option, &val, sizeof(int)) < 0) {
		CCK_PERROR("initTimestamping(%s): failed to enable socket timestamps", self->name);
		return false;
	}

	return true;

}
