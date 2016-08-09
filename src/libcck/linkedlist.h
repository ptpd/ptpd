/* Copyright (c) 2015 Wojciech Owczarek,
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
 * @file   linkedlist.h
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  linked list management macros
 *
 */

#ifndef PTPD_LINKEDLIST_H_
#define PTPD_LINKEDLIST_H_

#define LINKED_LIST_HOOK(vartype) \
	static vartype *_first = NULL; \
	static vartype *_last = NULL; \
	static uint32_t _serial = 0;

#define LINKED_LIST_HOOK_LOCAL(vartype) \
	vartype *_first;\
	vartype *_last;

#define LINKED_LIST_TAG(vartype) \
	vartype *_first; \
	vartype *_next; \
	vartype *_prev;

#define LINKED_LIST_APPEND(var) \
	if(_first == NULL) { \
		_first = var; \
	} \
	if(_last != NULL) { \
	    var->_prev = _last; \
	    var->_prev->_next = var; \
	} \
	_last = var; \
	var->_first = _first; \
	var->_serial = _serial; \
	_serial++;

#define LINKED_LIST_APPEND_LOCAL(holder, var) \
	if(holder->_first == NULL) { \
		holder->_first = var; \
	} \
	if(holder->_last != NULL) { \
	    var->_prev = holder->_last; \
	    var->_prev->_next = var; \
	} \
	holder->_last = var; \
	var->_first = holder->_first;

#define LINKED_LIST_REMOVE(var) \
	if(var == _last) { \
	    _serial = var->_serial; \
	} \
	if(var->_prev != NULL) { \
		if(var == _last) { \
			_last = var->_prev; \
		} \
		if(var->_next != NULL) { \
			var->_prev->_next = var->_next; \
		} else { \
			var->_prev->_next = NULL; \
		} \
	} else if (var->_next == NULL) { \
		_first = NULL; \
		_last = NULL; \
	} \
	if(var->_next != NULL) { \
		if(var == _first) { \
			_first = var->_next; \
		} \
		if(var->_prev != NULL) { \
			var->_next->_prev = var->_prev; \
		} else { \
			var->_next->_prev = NULL; \
		} \
	}

#define LINKED_LIST_REMOVE_LOCAL(holder, var) \
	if(var->_prev != NULL) { \
		if(var == holder->_last) { \
			holder->_last = var->_prev; \
		} \
		if(var->_next != NULL) { \
			var->_prev->_next = var->_next; \
		} else { \
			var->_prev->_next = NULL; \
		} \
	} else if (var->_next == NULL) { \
		holder->_first = NULL; \
		holder->_last = NULL; \
	} \
	if(var->_next != NULL) { \
		if(var == holder->_first) { \
			holder->_first = var->_next; \
		} \
		if(var->_prev != NULL) { \
			var->_next->_prev = var->_prev; \
		} else { \
			var->_next->_prev = NULL; \
		} \
	}

#define LINKED_LIST_FOREACH(var) \
	for(var = _first; var != NULL; var = var->_next)

#define LINKED_LIST_FOREACH_LOCAL(holder, var) \
	for(var = holder->_first; var != NULL; var = var->_next)

#define LINKED_LIST_DESTROYALL(helper, fun) \
	while(_first != NULL) { \
	    cd = _last; \
	    fun(&cd); \
	}

#endif /* PTPD_LINKEDLIST_H_ */
