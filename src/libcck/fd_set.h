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
 * @file   cck_fdset.h
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  FD set watcher definitions
 *
 */

#ifndef CCK_FD_SET_H_
#define CCK_FD_SET_H_

#include <sys/select.h>		/* fd_set */
#include <sys/time.h>		/* struct timeval */
#include <libcck/cck_types.h>
#include <libcck/linkedlist.h>

typedef struct CckFd CckFd;

/* LibCCK file descriptor wrapper */
struct CckFd {
	LINKED_LIST_TAG(CckFd);
	int 		fd;			/* the actual FD */
	void 		*owner;			/* component owning this fd */
	void (*callback) (void *, void *);	/* callback when data available */
	CckBool hasData;			/* flag marking that this fd has data available */
	CckFd *nextData;			/* linked list for fds with data available */

};

typedef struct {
	LINKED_LIST_HOOK_LOCAL(CckFd);
	CckBool hasData;
	CckFd *firstData;
	fd_set fdSet;
	fd_set _workSet;
	int maxFd;
} CckFdSet;

void	cckAddFd(CckFdSet *set, CckFd *fd);
void	cckRemoveFd(CckFdSet *set, CckFd *fd);
int	cckSelect(CckFdSet *set, struct timeval *timeout);


#endif /* CCK_FD_SET_H_ */
