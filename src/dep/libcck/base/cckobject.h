#ifndef _LIBCCK_BASE_CCKOBJECT_H_
#define _LIBCCK_BASE_CCKOBJECT_H_

/**
 * @file    cckobject.h
 * @authors Jan Breuer
 * @date   Mon Feb 24 09:20:00 CET 2014
 * 
 * libcck base object
 */

typedef struct _CCKObject CCKObject;

struct _CCKObject {
	char name[16];
	char type[16];
	CCKObject * parent;
	CCKObject * next;
};

#define CCK_OBJECT(obj)		((CCKObject *)obj)

void cckObjectInit(CCKObject * obj, const char * type, const char * name);

#endif /* _LIBCCK_BASE_CCKOBJECT_H_ */

