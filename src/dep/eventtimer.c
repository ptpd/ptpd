/*-
 * Copyright (c) 2015      Wojciech Owczarek,
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
 * @file   timer.c
 * @date   Wed Oct 1 00:41:26 2014
 *
 * @brief  Common code for the EventTimer object
 *
 * Creation and deletion plus maintaining a linked list of
 * all created instances.
 */

#include "../ptpd.h"

/* linked list - so that we can control all registered objects centrally */
static EventTimer *_first = NULL;
static EventTimer *_last = NULL;

EventTimer
*createEventTimer(const char* id)
{

	EventTimer *timer;

        if ( !(timer = calloc (1, sizeof(EventTimer))) ) {
            return NULL;
        }


	setupEventTimer(timer);

        strncpy(timer->id, id, EVENTTIMER_MAX_DESC);

	/* maintain the linked list */

	if(_first == NULL) {
		_first = timer;
	}

	if(_last != NULL) {
	    timer->_prev = _last;
	    timer->_prev->_next = timer;
	}

	_last = timer;

	timer->_first = _first;

	DBGV("created itimer eventtimer %s\n", timer->id);

        return timer;
}

void
freeEventTimer
(EventTimer **timer)
{
	if(timer == NULL) {
	    return;
	}

	EventTimer *ptimer = *timer;

	if(ptimer == NULL) {
	    return;
	}

	ptimer->shutdown(ptimer);

	/* maintain the linked list */

	if(ptimer->_prev != NULL) {

		if(ptimer == _last) {
			_last = ptimer->_prev;
		}

		if(ptimer->_next != NULL) {
			ptimer->_prev->_next = ptimer->_next;
		} else {
			ptimer->_prev->_next = NULL;
		}
	/* last one */
	} else if (ptimer->_next == NULL) {
		_first = NULL;
	}

	if(ptimer->_next != NULL) {

		if(ptimer == _first) {
			_first = ptimer->_next;
		}

		if(ptimer->_prev != NULL) {
			ptimer->_next->_prev = ptimer->_prev;
		} else {
			ptimer->_next->_prev = NULL;
		}

	}

	if(*timer != NULL) {
	    free(*timer);
	}

	*timer = NULL;

}
