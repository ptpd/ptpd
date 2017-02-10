/* Copyright (c) 2015-2017 Wojciech Owczarek,
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
 * @file   linked_list.h
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  linked list management macros
 *
 */

#ifndef CCK_LINKEDLIST_H_
#define CCK_LINKEDLIST_H_

/* static list parent / holder in a module */
#define LL_ROOT(vartype) \
	static vartype *_first = NULL; \
	static vartype *_last = NULL; \
	static uint32_t _serial = 0;

/* list parent / holder to be included in a structure */
#define LL_HOLDER(vartype) \
	vartype *_first;\
	vartype *_last;

/* list child / member to be included in a structure */
#define LL_MEMBER(vartype) \
	vartype **_first; \
	vartype *_next; \
	vartype *_prev;

/* append variable to statically embedded linked list */
#define LL_APPEND_STATIC(var) \
	if(_first == NULL) { \
		_first = var; \
	} \
	if(_last != NULL) { \
	    var->_prev = _last; \
	    var->_prev->_next = var; \
	} \
	_last = var; \
	var->_first = &_first; \
	var->_next = NULL; \
	var->_serial = _serial; \
	_serial++;

/* append variable to linked list held in the holder variable */
#define LL_APPEND_DYNAMIC(holder, var) \
	if(holder->_first == NULL) { \
		holder->_first = var; \
	} \
	if(holder->_last != NULL) { \
	    var->_prev = holder->_last; \
	    var->_prev->_next = var; \
	} \
	holder->_last = var; \
	var->_next = NULL; \
	var->_first = &holder->_first;

/* remove variable from a statically embedded list */
#define LL_REMOVE_STATIC(var) \
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
	} \
	var->_next = NULL; \
	var->_prev = NULL; \
	var->_first = NULL;

/* remove variable from holder variable */
#define LL_REMOVE_DYNAMIC(holder, var) \
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
	} \
	var->_next = NULL; \
	var->_prev = NULL; \
	var->_first = NULL;

/* foreach loop within a module, assigning var as we walk */
#define LL_FOREACH_STATIC(var) \
	for(var = _first; var != NULL; var = var->_next)

/* reverse linked list walk within module */
#define LL_FOREACH_STATIC_REVERSE(var) \
	for(var = _last; var != NULL; var = var->_prev)

/* foreach within holder struct */
#define LL_FOREACH_DYNAMIC(holder, var) \
	for(var = (holder)->_first; var != NULL; var = var->_next)

/* reverse foreach within holder struct */
#define LL_FOREACH_DYNAMIC_REVERSE(holder, var) \
	for(var = (holder)->_last; var != NULL; var = var->_prev)

#define LL_FOREACH_INNER(holder, var) \
	for(var = (*(holder))->_first; var != NULL; var = var->_next)

/* walk list from last member downwards, calling fun to delete members */
#define LL_DESTROYALL(helper, fun) \
	while(_first != NULL) { \
	    helper = _last; \
	    fun(&helper); \
	}

/* pool holder data to be included in a structure */
#define POOL_HOLDER(vartype, count) \
    vartype _pooldata[count]; \
    vartype *_first; \
    vartype *_last; \
    struct { \
	vartype *_first; \
	vartype *_last; \
    } _pool; \

#endif /* CCK_LINKEDLIST_H_ */
