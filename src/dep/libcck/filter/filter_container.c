/**
 * @file    filter_container.c
 * @authors Jan Breuer
 * @date   Mon Feb 24 10:30:00 CET 2014
 * 
 * filter container
 */

#include <stdlib.h>

#include "filter_container.h"
#include "filter.h"

#include "../../iniparser/dictionary.h"
#include "../../iniparser/iniparser.h"

#define CONTAINER_SECTION	"filter"

FilterContainer * FilterContainerCreate(void) {
	FilterContainer * fc;
	
	fc = calloc(1, sizeof(FilterContainer));
	
	FilterContainerInit(fc);
	
	return fc;
}

void FilterContainerInit(FilterContainer * container) {
    cckContainerInit(CCK_CONTAINER(container), "container_" CONTAINER_SECTION, CONTAINER_SECTION);
}

void FilterContainerDestroy(FilterContainer * container) {
    /* TODO: free all filters */
    
    free(container);
}

static int compare (const void * a, const void * b)
{
	return strcmp((char *)a, (char*)b);
}

void FilterContainerLoad(FilterContainer * container, dictionary * dict) {
	char ** seckeys;
	int nkeys, i;
	
	char name[32];
	char * key0;
	char * key1;
	char * key2;
	
	CCKObject * child;

	nkeys = iniparser_getsecnkeys(dict, CONTAINER_SECTION);
	seckeys = iniparser_getseckeys(dict, CONTAINER_SECTION);

	qsort (seckeys, nkeys, sizeof(char *), compare);
	
	name[0] = '\0';

	/* create objects */
	for(i = 0; i<nkeys; i++) {
		/* full key e.g. filter:owd:type */
		key0 = seckeys[i];
		/* object key e.g. owd.type */
		key1 = sizeof(CONTAINER_SECTION) + key0;
		/* parameter key e.g. type */
		key2 = strchr (key1, ':') + 1;
		if (strncmp(name, key1, key2 - key1 - 1) != 0) {
			strncpy(name, key1, key2 - key1 - 1);
		}
		
		if (strcmp(key2, "type") == 0) {
			/* test if object is not yet created */
			if(cckContainerGet(CCK_CONTAINER(container), name) == NULL) {
				child = CCK_OBJECT(FilterCreate(iniparser_getstring(dict, key0, NULL), name));
				if (child != NULL) {
					cckContainerAdd(CCK_CONTAINER(container), child);
				} else {
					/* TODO: exception - unable to create object */
				}
			} else {
				/* TODO: exception - object already exists */
			}
		}
	}
	
	/* configure objects */
	name[0] = '\0';
	child = NULL;

	for(i = 0; i<nkeys; i++) {
		/* full key e.g. filter:owd:type */
		key0 = seckeys[i];
		/* object key e.g. owd.type */
		key1 = sizeof(CONTAINER_SECTION) + key0;
		/* parameter key e.g. type */
		key2 = strchr (key1, ':') + 1;
		if (strncmp(name, key1, key2 - key1 - 1) != 0) {
			strncpy(name, key1, key2 - key1 - 1);
			child = cckContainerGet(CCK_CONTAINER(container), name);
			if (child == NULL) {
				/* TODO: exception - object does not exist */
			}
		}
		
		/* ignore "type" key */
		if (strcmp(key2, "type") != 0) {
			FilterConfigure(CCK_FILTER(child), key2, iniparser_getstring(dict, key0, NULL));
		}
	}

	free(seckeys);
}


