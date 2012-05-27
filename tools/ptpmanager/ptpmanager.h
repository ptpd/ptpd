#ifndef MANAGER_H
#define MANAGER_H

#include <sys/param.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <syslog.h>
#include <limits.h>
#include <net/if_arp.h>
#include <net/if.h>

#define MANAGEMENT_LENGTH 48
#define CLOCK_IDENTITY_LENGTH 8
#define HEADER_LENGTH 34
#define PACKET_SIZE  300
#define MANAGEMENT 13


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
typedef Octet ClockIdentity[CLOCK_IDENTITY_LENGTH];

typedef struct {
	unsigned int lsb;     /* FIXME: shouldn't uint32_t and int32_t be used here? */
	int msb;
} Integer64;

int out_length;

Boolean receivedFlag;
/**
* \brief Struct used to store network datas
 */
typedef struct {
  Integer32 eventSock, generalSock;

} NetPath;

enum {
	PTP_ETHER, PTP_DEFAULT
};

typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "portIdentity.def"
} PortIdentity;

typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "header.def"
} MsgHeader;


/**
 * \brief Management TLV message fields
 */
/* Management TLV Message */
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "managementTLV.def"
	Octet* dataField;
} ManagementTLV;

/**
* \brief Management message fields (Table 37 of the spec)
 */
/*management Message*/
typedef struct {
	#define OPERATE( name, size, type ) type name;
	#include "management.def"
	ManagementTLV* tlv;
}MsgManagement;

#define TLV_MANAGEMENT 0
#define TLV_MANAGEMENT_ERROR_STATUS 1
#define RESPONSE 0
#define ACK 1
#define MM_CLOCK_DESCRIPTION 0

#endif /*MANAGER_H_*/
