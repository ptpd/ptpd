/**
 * @file    cckobject.c
 * @authors Jan Breuer
 * @date   Mon Feb 24 09:20:00 CET 2014
 * 
 * libcck base object
 */

#include <string.h>
#include "cckobject.h"

static void nameSet(CCKObject * obj, const char * name) {
	if (name != NULL) {
		strncpy(obj->name, name, sizeof(obj->name));
	} else {
		obj->name[0] = '\0';
	}
}

static void typeSet(CCKObject * obj, const char * type) {
	if (type != NULL) {
		strncpy(obj->type, type, sizeof(obj->type));
	} else {
		obj->type[0] = '\0';
	}
}

void cckObjectInit(CCKObject * obj, const char * type, const char * name) {
	typeSet(obj, type);
	nameSet(obj, name);
	obj->parent = NULL;
	obj->next = NULL;
}