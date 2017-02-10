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
 * @file   cck_fdset.h
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  FD set watcher definitions
 *
 */

#ifndef CCK_FD_SET_H_
#define CCK_FD_SET_H_

#include <sys/select.h>
#include <sys/time.h>
#include <libcck/cck_types.h>
#include <libcck/linked_list.h>

typedef struct CckFd CckFd;

/* LibCCK file descriptor wrapper */
struct CckFd {
	LL_MEMBER(CckFd);
	int 		fd;				/* the actual FD */
	void 		*owner;				/* component owning this fd */
	struct {
	    void (*onData) (void *me, void *owner);	/* callback when data available */
	} callbacks;
	bool hasData;			/* flag marking that this fd has data available */
	CckFd *nextData;			/* linked list for fds with data available */

};

/* LibCCK wrapper for a set of file descriptors */
typedef struct {
	LL_HOLDER(CckFd);
	bool hasData;
	CckFd *firstData;
	fd_set fdSet;
	fd_set _workSet;
	int maxFd;
	/* callbacks */
	struct {
	    void (*prePoll) (void *me, int *nfd, fd_set *fds, struct timeval *timeout, int *block);		/* before calling select() */
	    void (*postPoll) (void *me, int *nfd, fd_set *fds, struct timeval *timeout);	/* always called after select() */
	    void (*onData) (void *me, int *nfd, fd_set *fds, struct timeval *timeout);		/* when select has got something */
	    void (*onTimeout) (void *me, int *nfd, fd_set *fds, struct timeval *timeout);	/* on timeout */
	} callbacks;
} CckFdSet;

void	cckAddFd(CckFdSet *set, CckFd *fd);
void	cckRemoveFd(CckFdSet *set, CckFd *fd);
int	cckPollData(CckFdSet *set, struct timeval *timeout);
void	clearCckFdSet(CckFdSet *set);

#endif /* CCK_FD_SET_H_ */
