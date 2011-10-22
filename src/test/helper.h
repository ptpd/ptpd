/**
 * test helper functions
 */

/* NOTE: these macros can be refactored into functions */

/* if memory allocation fails return -1 */
#define MALLOC(ptr,size) \
        if(!((ptr)=malloc(size))) { \
                printf("failed to allocate memory\n"); \
                return -1; \
        }

#define MALLOC_TLV(msg, type) \
        if(!(msg.tlv =  malloc(sizeof(ManagementTLV)))) { \
		printf("failed to allocate memory\n"); \
                return -1; \
	} \
        if(!(msg.tlv->dataField = malloc( sizeof(type)))) { \
		printf("failed to allocate memory\n"); \
                free(msg.tlv); \
                return -1; \
        }

#define FREE_TLV(msg) \
	free(msg.tlv->dataField); \
	free(msg.tlv);
