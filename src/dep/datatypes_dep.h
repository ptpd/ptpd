#ifndef DATATYPES_DEP_H_
#define DATATYPES_DEP_H_

#include "../libcck/cck.h"

/**
*\file
* \brief Implementation specific datatype
 */
typedef enum {FALSE=0, TRUE} Boolean;
typedef char Octet;
typedef int8_t Integer8;
typedef int16_t Integer16;
typedef int32_t Integer32;
typedef uint8_t  UInteger8;
typedef uint16_t UInteger16;
typedef uint32_t UInteger32;
typedef uint16_t Enumeration16;
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
* \brief Implementation specific of UInteger48 type
 */
typedef struct {
	uint32_t lsb;
	uint16_t msb;
} UInteger48;

/**
* \brief Implementation specific of Integer64 type
 */
typedef struct {
	uint32_t lsb;
	int32_t msb;
} Integer64;

/**
* \brief Struct used to average the offset from master
*
* The FIR filtering of the offset from master input is a simple, two-sample average
 */
typedef struct {
    Integer32  nsec_prev, y;
} offset_from_master_filter;

/**
* \brief Struct used to average the one way delay
*
* It is a variable cutoff/delay low-pass, infinite impulse response (IIR) filter.
*
*  The one-way delay filter has the difference equation: s*y[n] - (s-1)*y[n-1] = x[n]/2 + x[n-1]/2, where increasing the stiffness (s) lowers the cutoff and increases the delay.
 */
typedef struct {
    Integer32  nsec_prev, y;
    Integer32  s_exp;
} one_way_delay_filter;

/**
* \brief Struct containing interface information and capabilities
 */
typedef struct {
        unsigned int flags;
        int addressFamily;
        Boolean hasHwAddress;
        Boolean hasAfAddress;
        unsigned char hwAddress[14];
        struct sockaddr afAddress;
} InterfaceInfo;

/*
typedef union {
	struct sockaddr 	inetAddr;
	struct sockaddr_in	inetAddr4;
	struct sockaddr_in6	inetAddr6;
	struct ether_addr	etherAddr;
	struct sockaddr_ll	llAdr;
	struct sockaddr_un	unAddr;
} TransportAddress;
*/

typedef struct {
	uint16_t transportType;
	TransportAddress transportAddress;
} TransportEndpoint;

/**
* \brief Struct describing network transport data
 */
typedef struct {

	TransportAddress ownAddress;
	/* Interface address and capability descriptor */
	InterfaceInfo interfaceInfo;
	/* Typically MAC address - outer 6 octers of ClockIdendity */
	Octet interfaceID[ETHER_ADDR_LEN];

} NetPath;

typedef struct {

	char* logID;
	char* openMode;
	char logPath[PATH_MAX];
	FILE* logFP;

	Boolean logEnabled;
	Boolean truncateOnReopen;
	Boolean unlinkOnClose;
	Boolean rotate;

	UInteger32 maxSize;
	UInteger32 fileSize;
	int maxFiles;

} LogFileHandler;


typedef struct{

    UInteger8 minValue;
    UInteger8 maxValue;
    UInteger8 defaultValue;

} UInteger8_option;

typedef struct{

    Integer32  minValue;
    Integer32  maxValue;
    Integer32  defaultValue;

} Integer32_option;

typedef struct{

    UInteger32  minValue;
    UInteger32  maxValue;
    UInteger32  defaultValue;

} UInteger32_option;

typedef struct{

    Integer16  minValue;
    Integer16  maxValue;
    Integer16  defaultValue;

} Integer16_option;

typedef struct{

    UInteger16  minValue;
    UInteger16  maxValue;
    UInteger16  defaultValue;

} UInteger16_option;

#endif /*DATATYPES_DEP_H_*/
