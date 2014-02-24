/**
 * @file    cckcontainer.c
 * @authors Jan Breuer
 * @date   Mon Feb 24 09:20:00 CET 2014
 * 
 * libcck base container
 */

#include <stdlib.h>
#include <string.h>

#include "cckcontainer.h"


void cckContainerInit(CCKContainer * container, const char * type, const char * name) {
	cckObjectInit(CCK_OBJECT(container), type, name);
	container->firstChild = NULL;
}

static CCKObject ** findPrevPtr(CCKContainer * container, CCKObject * child) {
	CCKObject ** nextChild;
	
	nextChild = &container->firstChild;
	while(*nextChild && *nextChild != child) {
		nextChild = &(*nextChild)->next;
	}
	
	return nextChild;
}

BOOL cckContainerAdd(CCKContainer * container, CCKObject * child) {
	CCKObject ** nextChild;
	
	if ((child == NULL) || (child->parent != NULL) || (child->next != NULL)) {
		return FALSE;
	}

	nextChild = findPrevPtr(container, NULL);
	
	*nextChild = child;
	
	child->parent = CCK_OBJECT(container);
	child->next = NULL;
	
	return TRUE;
}

BOOL cckContainerRemove(CCKContainer * container, CCKObject * child) {
	CCKObject ** nextChild;

	if ((child == NULL) || (child->parent != CCK_OBJECT(container))) {
		return FALSE;
	}

	nextChild = findPrevPtr(container, child);

	if (*nextChild != child) {
		return FALSE;
	}
	
	*nextChild = child->next;
	child->parent = NULL;
	child->next = NULL;
	return TRUE;
}

CCKObject * cckContainerGet(CCKContainer * container, const char * name) {
	CCKObject * nextChild;
	
	nextChild = container->firstChild;
	
	while(nextChild && (strcmp(nextChild->name, name) != 0)) {
		nextChild = nextChild->next;
	}
	
	return nextChild;
}

