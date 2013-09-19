#ifndef DATATYPES_DEP_H_
#define DATATYPES_DEP_H_

/**
*\file
* \brief Implementation specific datatype

 */
/* FIXME: shouldn't uint32_t and friends be used here? */
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
* \brief Implementation specific of UInteger48 type
 */
typedef struct {
	unsigned int lsb;     /* FIXME: shouldn't uint32_t and uint16_t be used here? */
	unsigned short msb;
} UInteger48;

/**
* \brief Implementation specific of Integer64 type
 */
typedef struct {
	unsigned int lsb;     /* FIXME: shouldn't uint32_t and int32_t be used here? */
	int msb;
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
* \brief Struct used to store network datas
 */
typedef struct {
	Integer32 eventSock, generalSock;
	Integer32 multicastAddr, peerMulticastAddr,unicastAddr;
	
	/* used by IGMP refresh */
	struct in_addr interfaceAddr;
	Octet port_uuid_field[PTP_UUID_LENGTH];
	
	/* used for Hybrid mode */
	Integer32 lastRecvAddr;

	uint64_t sentPackets;
	uint64_t receivedPackets;
	
	pcap_t *pcapEvent;
	Integer32 pcapEventSock;
	pcap_t *pcapGeneral;
	Integer32 pcapGeneralSock;
	Integer32 headerOffset;

	/* used for tracking the last TTL set */
	int ttlGeneral;
	int ttlEvent;
	struct ether_addr *etherDest;
} NetPath;

typedef struct {

	char* logID;
	char* openMode;
	char logPath[PATH_MAX];
	FILE* logFP;

	Boolean logEnabled;
	Boolean truncateOnReopen;
	Boolean unlinkOnClose;

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
