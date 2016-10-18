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

/**8
 * @file   fd_set.c
 * @date   Sat Jan 9 16:14:10 2016
 *
 * @brief  LibCCK FD set watcher implementation
 *
 */

#include <stdio.h>		/* NULL type */
#include <string.h>		/* memcpy() */
#include <errno.h>		/* errno / error codes */
#include <sys/select.h>		/* select() */

#include <libcck/cck_types.h>
#include <libcck/fd_set.h>

static int getMaxFd(CckFdSet *set);

void
cckAddFd(CckFdSet *set, CckFd *fd)
{

    if(fd == NULL) {
	/* DBG("Cannot add null fd to set\n"); */
	return;
    }

    if(set == NULL) {
	/* DBG("Cannot add fd to null set */
	return;
    }

    if(FD_ISSET(fd->fd, &set->fdSet)) {
	/* DBG("fd %d already in set\n", fd->fd); */
	return;
    }

    FD_SET(fd->fd, &set->fdSet);

    LINKED_LIST_APPEND_LOCAL(set, fd);

    set->maxFd = getMaxFd(set);

}

void
cckRemoveFd(CckFdSet *set, CckFd *fd)
{
    if(fd == NULL) {
	/* DBG("Cannot remove null fd from set\n"); */
	return;
    }

    if(set == NULL) {
	/* DBG("Cannot remove fd to null set */
	return;
    }

    if(!FD_ISSET(fd->fd, &set->fdSet)) {
	/* DBG("fd %d not in set, not removing\n", fd->fd); */
	return;
    }

    FD_CLR(fd->fd, &set->fdSet);

    LINKED_LIST_REMOVE_LOCAL(set, fd);

    set->maxFd = getMaxFd(set);

}

int
cckSelect(CckFdSet *set, struct timeval *timeout)
{

    int ret;
    CckFd *fd;
    CckFd *lastFd = NULL;


    if(set == NULL) {
	/* DBG("cckSelect: empty fd set given\n"); */
	return -1;
    }

    set->hasData = CCK_FALSE;
    set->firstData = NULL;

    /* copy the clean fd set to work fd set */
    memcpy(&set->_workSet, &set->fdSet, sizeof(fd_set));

    /* run select () */
    ret = select(set->maxFd + 1, &set->_workSet, NULL, NULL, timeout);

    /* these are expected - signals, etc. */
    if(ret < 0 && (errno == EAGAIN || errno == EINTR)) {
	return 0;
    }

    /* walk through all fds used, mark them if they have data to read */
    if(ret > 0) {
	LINKED_LIST_FOREACH_LOCAL(set, fd) {
	    fd->hasData = CCK_FALSE;
	    fd->nextData = NULL;
	    if(FD_ISSET(fd->fd, &set->_workSet)) {

		set->hasData = CCK_TRUE;
		fd->hasData = CCK_TRUE;

		/* build a linked list of fds which have data to read */
		if(lastFd == NULL) {
		    set->firstData = fd;
		} else {
		    lastFd->nextData = fd;
		}

		/*
		 * todo: callbacks - they will be optional. The user may want control over the order of FDs read,
		 * as is the case with PTP(d), where we always check the event transport first -
		 * naturally, if we start the transports in the right order, we achieve the same,
		 * however not explicitly so. The callback could also automatically read the data.
		 */

		/* fd->callback(fd->owner, ...); */

		lastFd = fd;

	    }

	}

    }

    return ret;


}

static int
getMaxFd(CckFdSet *set)
{
    CckFd *fd;
    int maxFd = -1;

    if(set == NULL) {
	return -1;
    }

    if(set->_first == NULL) {
	return -1;
    }

    LINKED_LIST_FOREACH_LOCAL(set, fd) {
	if(fd->fd > maxFd) {
	    maxFd = fd->fd;
	}
    }

    return maxFd;

}

