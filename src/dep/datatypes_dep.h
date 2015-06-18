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


/**
* \brief Struct describing network transport data
 */
typedef struct {
	Integer32 eventSock, generalSock;
	Integer32 multicastAddr, peerMulticastAddr;

	/* Interface address and capability descriptor */
	InterfaceInfo interfaceInfo;

	/* used by IGMP refresh */
	struct in_addr interfaceAddr;
	/* Typically MAC address - outer 6 octers of ClockIdendity */
	Octet interfaceID[ETHER_ADDR_LEN];
	/* source address of last received packet - used for unicast replies to Delay Requests */
	Integer32 lastSourceAddr;
	/* destination address of last received packet - used for unicast FollowUp for multiple slaves*/
	Integer32 lastDestAddr;

	uint64_t sentPackets;
	uint64_t receivedPackets;

	uint64_t sentPacketsTotal;
	uint64_t receivedPacketsTotal;

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
	Boolean joinedMulticast;
	struct ether_addr etherDest;
	struct ether_addr peerEtherDest;
	Boolean txTimestampFailure;

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

typedef union { uint32_t *uintval; int32_t *intval; double *doubleval; Boolean *boolval; char *strval; } ConfigPointer;
typedef union { uint32_t uintval; int32_t intval; double doubleval; Boolean boolval; char *strval; } ConfigSetting;

typedef struct ConfigOption ConfigOption;

struct ConfigOption {
    char *key;
    enum { CO_STRING, CO_INT, CO_UINT, CO_DOUBLE, CO_BOOL, CO_SELECT } type;
    enum { CO_MIN, CO_MAX, CO_RANGE, CO_STRLEN } restriction;
    ConfigPointer target;
    ConfigPointer defvalue;
    ConfigSetting constraint1;
    ConfigSetting constraint2;
    int restartFlags;
    ConfigOption *next;
};

typedef struct {
    int currentOffset;
    int nextOffset;
    int leapType;
    Integer32 startTime;
    Integer32 endTime;
    Boolean valid;
    Boolean offsetValid;
} LeapSecondInfo;

#endif /*DATATYPES_DEP_H_*/
