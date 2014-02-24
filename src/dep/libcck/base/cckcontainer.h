#ifndef _LIBCCK_BASE_CCKCONTAINER_H_
#define _LIBCCK_BASE_CCKCONTAINER_H_

/**
 * @file    cckcontainer.h
 * @authors Jan Breuer
 * @date   Mon Feb 24 09:20:00 CET 2014
 * 
 * libcck base container
 */

#include "ccktypes.h"
#include "cckobject.h"


typedef struct {
	CCKObject ccko;
	
	CCKObject * firstChild;
} CCKContainer;

#define CCK_CONTAINER(obj)		((CCKContainer *)obj)

void cckContainerInit(CCKContainer * container, const char * type, const char * name);
BOOL cckContainerAdd(CCKContainer * container, CCKObject * child);
BOOL cckContainerRemove(CCKContainer * container, CCKObject * child);
CCKObject * cckContainerGet(CCKContainer * container, const char * name);

#endif /* _LIBCCK_BASE_CCKCONTAINER_H_ */

