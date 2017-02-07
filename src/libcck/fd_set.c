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
#include <libcck/cck_logger.h>
#include <libcck/cck_utils.h>
#include <libcck/fd_set.h>

static int getMaxFd(CckFdSet *set);

void
cckAddFd(CckFdSet *set, CckFd *fd)
{

    if(fd == NULL) {
	CCK_DBG("Cannot add null fd to set\n");
	return;
    }

    if(set == NULL) {
	CCK_DBG("Cannot add fd %d to null set\n", fd->fd);
	return;
    }

    if(FD_ISSET(fd->fd, &set->fdSet)) {
	CCK_DBG("fd %d already in set, not adding\n", fd->fd);
	return;
    }

    FD_SET(fd->fd, &set->fdSet);

    LINKED_LIST_APPEND_DYNAMIC(set, fd);

    set->maxFd = getMaxFd(set);

}

void
cckRemoveFd(CckFdSet *set, CckFd *fd)
{


    if(fd == NULL) {
	CCK_DBG("Cannot remove null fd from set\n");
	return;
    }

    if(set == NULL) {
	CCK_DBG("Cannot remove fd %d from null set\n", fd->fd);
	return;
    }

    if(!FD_ISSET(fd->fd, &set->fdSet)) {
	CCK_DBG("fd %d not in set, not removing\n", fd->fd);
	return;
    }

    FD_CLR(fd->fd, &set->fdSet);

    LINKED_LIST_REMOVE_DYNAMIC(set, fd);

    set->maxFd = getMaxFd(set);

}

int
cckPollData(CckFdSet *set, struct timeval *timeout_in)
{

    int ret;
    int maxFd = set->maxFd;
    CckFd *fd;
    CckFd *lastFd = NULL;

    /* external callback may have a different notion of blocking and timeout, not just NULL */
    struct timeval timeout = {0, 0};
    int block = 1;

    if(set == NULL) {
	 CCK_DBG("cckPollData: empty fd set given\n");
	return -1;
    }

    if(timeout_in != NULL) {
	block = 0;
	timeout = *timeout_in;
    }

    set->hasData = false;
    set->firstData = NULL;

    /* copy the clean fd set to work fd set */
    memcpy(&set->_workSet, &set->fdSet, sizeof(fd_set));

    /* nfds = max + 1 */
    maxFd++;

    /* run our pre-select() callback which may add more FDs or modify block or timeout */
    SAFE_CALLBACK(set->callbacks.prePoll, set, &maxFd, &set->_workSet, &timeout, &block);

    /* run select () - block could have been modified by the callback */
    ret = select(maxFd, &set->_workSet, NULL, NULL, block ? NULL : &timeout);

    /* these are expected - signals, etc. */
    if(ret < 0 && (errno == EAGAIN || errno == EINTR)) {
	ret = 0;
	goto finalise;
    }

    /* walk through all fds used, mark them if they have data to read */
    if(ret > 0) {

	LINKED_LIST_FOREACH_DYNAMIC(set, fd) {
	    fd->hasData = false;
	    fd->nextData = NULL;
	    if(FD_ISSET(fd->fd, &set->_workSet)) {

		set->hasData = true;
		fd->hasData = true;

		/* build a linked list of fds which have data to read */
		if(lastFd == NULL) {
		    set->firstData = fd;
		} else {
		    lastFd->nextData = fd;
		}

		/* run the on-data callback for the given fd */
		SAFE_CALLBACK(fd->callbacks.onData, fd, fd->owner);

		lastFd = fd;

	    }

	}

	/* run our on-data callback (if the SET has some data) */
	SAFE_CALLBACK(set->callbacks.onData, set, &maxFd, &set->_workSet, block ? NULL : &timeout);

    }

    /* run our on-timeout callback */
    if(ret == 0){
	SAFE_CALLBACK(set->callbacks.onTimeout, set, &maxFd, &set->_workSet, block ? NULL : &timeout);
    }

finalise:

    /* run the always-run post-select callback */
    SAFE_CALLBACK(set->callbacks.postPoll, set, &maxFd, &set->_workSet, block ? NULL : &timeout);

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

    LINKED_LIST_FOREACH_DYNAMIC(set, fd) {
	if(fd->fd > maxFd) {
	    maxFd = fd->fd;
	}
    }

    return maxFd;

}

void
clearCckFdSet(CckFdSet *set)
{
    memset(set, 0, sizeof(CckFdSet));

}
