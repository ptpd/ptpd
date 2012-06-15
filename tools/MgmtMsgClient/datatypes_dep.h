/** 
 * @file datatypes_dep.h
 *
 * @brief Implementation specific datatypes
 */

#ifndef DATATYPES_DEP_H
#define	DATATYPES_DEP_H

typedef enum {FALSE=0, TRUE} Boolean;
typedef char Octet;
typedef signed char Integer8;
typedef signed short Integer16;
typedef signed int Integer32;
typedef unsigned char UInteger8;
typedef unsigned short UInteger16;
typedef unsigned int UInteger32;
typedef unsigned short Enumeration16;
typedef unsigned char Enumeration8;
typedef unsigned char Enumeration4;
typedef unsigned char Enumeration4Upper;
typedef unsigned char Enumeration4Lower;
typedef unsigned char UInteger4;
typedef unsigned char UInteger4Upper;
typedef unsigned char UInteger4Lower;
typedef unsigned char Nibble;
typedef unsigned char NibbleUpper;
typedef unsigned char NibbleLower;

/**
 * @brief Implementation specific of UInteger48 type
 */
typedef struct {
	unsigned int lsb;     /* FIXME: shouldn't uint32_t and uint16_t be used here? */
	unsigned short msb;
} UInteger48;

/**
 * @brief Implementation specific of Integer64 type
 */
typedef struct {
	unsigned int lsb;     /* FIXME: shouldn't uint32_t and int32_t be used here? */
	int msb;
} Integer64;

#endif	/* DATATYPES_DEP_H */

