/**
 * @file   datatypes_dep.h
 * @date   Tue Jul 20 16:16:48 2010
 * 
 * @brief  Platform dependent datatypes.
 * 
 * 
 */

#ifndef DATATYPES_DEP_H
#define DATATYPES_DEP_H

typedef enum {
	FALSE = 0, TRUE
}	Boolean;
typedef char Octet;
typedef signed char Integer8;
typedef signed short Integer16;
typedef signed int Integer32;
typedef unsigned char UInteger8;
typedef unsigned short UInteger16;
typedef unsigned int UInteger32;

typedef struct {
	Integer32 nsec_prev, y;
}	offset_from_master_filter;

typedef struct {
	Integer32 nsec_prev, y;
	Integer32 s_exp;
}	one_way_delay_filter;

typedef struct {
	Integer32 eventSock, generalSock, multicastAddr, unicastAddr;
}	NetPath;

#endif
