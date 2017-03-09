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
 * @file   libcck.h
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  libCCK initialisation and housekeeping declarations
 *
 */

#ifndef LIBCCK_H_
#define LIBCCK_H_

#define CCK_COMPONENT_NAME_MAX 20

#include <libcck/fd_set.h>

typedef struct {

    int clockSyncRate;
    int clockUpdateInterval;
    int clockFaultTimeout;
    int transportMonitorInterval;
    int transportFaultTimeout;
    char masterClockRefName [CCK_COMPONENT_NAME_MAX + 1];

} CckConfig;

bool		cckInit(CckFdSet *set); /* initialise libCCK and housekeeping events */
void		cckShutdown();		/* shutdown all libCCK components */
CckConfig*	getCckConfig();		/* get global libCCK configuration handle */
CckFdSet*	getCckFdSet();		/* get global FD set */
const CckConfig *cckDefaults();		/* get libcck default configuration */
void cckApplyConfig();			/* apply current common libCCK configuration */

#endif /* LIBCCK_H_ */
