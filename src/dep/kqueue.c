/*-
 * Copyright (c) 2020      Chris Johns,
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
 * @file   kqueue.c
 * @date
 *
 * @brief  Kqueue support code
 *
 * The kqueue is shared between the networking and timer code and there
 * no shared data in their interfaces so kqueue handle is held here.
 */

#include "../ptpd.h"

static int kq = -1;

int
ptpKqueueGet(void)
{
#ifdef HAVE_KQUEUE
	if(kq < 0) {
    kq = kqueue();
    if (kq < 0)
      PERROR("kqueue failed");
  }
#endif
  return kq;
}

int
ptpKqueueWait(struct timespec* ts, struct kevent* events, int nevents)
{
  int ret;

  if (kq < 0) {
    PERROR("kqueue not initialised");
    return -1;
  }

  ret = kevent(kq, NULL, 0, events, nevents, ts);

  if (ret > 0) {
    int e;
    for (e = 0; e < ret; ++e) {
      DBG2("kevent: %d: %2d: %s\n", e, events[e].ident,
           events[e].filter == EVFILT_TIMER ? "timer" : "sock");
      if (events[e].filter == EVFILT_TIMER) {
        EventTimer* timer = events[e].udata;
        timer->expired = TRUE;
      }
    }
  }

  return ret;
}
