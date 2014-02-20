#ifndef DATATYPES_DEP_H_
#define DATATYPES_DEP_H_

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
* \brief Struct containing interface information and capabilities
 */
typedef struct {
        unsigned int flags;
        int addressFamily;
        Boolean hasHwAddress;
        Boolean hasAfAddress;
        unsigned char hwAddress[14];
        struct in_addr afAddress;
} InterfaceInfo;


/**
* \brief Struct describing network transport data
 */
typedef struct {
	Integer32 eventSock, generalSock;
	Integer32 multicastAddr, peerMulticastAddr,unicastAddr;

	/* Interface address and capability descriptor */
	InterfaceInfo interfaceInfo;

	/* used by IGMP refresh */
	struct in_addr interfaceAddr;
	/* Typically MAC address - outer 6 octers of ClockIdendity */
	Octet interfaceID[ETHER_ADDR_LEN];
	/* used for Hybrid mode */
	Integer32 lastRecvAddr;

	uint64_t sentPackets;
	uint64_t receivedPackets;

#ifdef PTPD_PCAP
	pcap_t *pcapEvent;
	pcap_t *pcapGeneral;
	Integer32 pcapEventSock;
	Integer32 pcapGeneralSock;
#endif
	Integer32 headerOffset;

	/* used for tracking the last TTL set */
	int ttlGeneral;
	int ttlEvent;
	struct ether_addr etherDest;
	struct ether_addr peerEtherDest;
#ifdef SO_TIMESTAMPING
	Boolean txTimestampFailure;
#endif /* SO_TIMESTAMPING */

	Ipv4AccessList* timingAcl;
	Ipv4AccessList* managementAcl;

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
