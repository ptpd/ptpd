/** 
 * @file OutgoingManagementMessage.cpp
 * @author Tomasz Kleinschmidt
 * 
 * @brief OutgoingManagementMessage class implementation
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

//Temporary for assigning ClockIdentity
#include <string.h>

#include "OutgoingManagementMessage.h"
#include "app_dep.h"
#include "constants_dep.h"


#define PACK_SIMPLE( type ) \
void OutgoingManagementMessage::pack##type( void* from, void* to ) \
{ \
    *(type *)to = *(type *)from; \
}

#define PACK_ENDIAN( type, size ) \
void OutgoingManagementMessage::pack##type( void* from, void* to ) \
{ \
    *(type *)to = flip##size( *(type *)from ); \
}

#define PACK_LOWER_AND_UPPER( type ) \
void OutgoingManagementMessage::pack##type##Lower( void* from, void* to ) \
{ \
    *(char *)to = *(char *)to & 0xF0; \
    *(char *)to = *(char *)to | *(type *)from; \
} \
\
void OutgoingManagementMessage::pack##type##Upper( void* from, void* to ) \
{ \
    *(char *)to = *(char *)to & 0x0F; \
    *(char *)to = *(char *)to | (*(type *)from << 4); \
}

PACK_SIMPLE( Boolean )
PACK_SIMPLE( UInteger8 )
PACK_SIMPLE( Octet )
PACK_SIMPLE( Enumeration8 )
PACK_SIMPLE( Integer8 )

PACK_ENDIAN( Enumeration16, 16 )
PACK_ENDIAN( Integer16, 16 )
PACK_ENDIAN( UInteger16, 16 )
PACK_ENDIAN( Integer32, 32 )
PACK_ENDIAN( UInteger32, 32 )

PACK_LOWER_AND_UPPER( Enumeration4 )
PACK_LOWER_AND_UPPER( UInteger4 )
PACK_LOWER_AND_UPPER( Nibble )


OutgoingManagementMessage::OutgoingManagementMessage(Octet* buf, OptBuffer* optBuf) {
    this->outgoing = (MsgManagement *)malloc(sizeof(MsgManagement));
    
    handleMMNullManagement(this->outgoing, GET);
    packMsgManagement(this->outgoing, buf);
    packManagementTLV(this->outgoing->tlv, buf);
}

OutgoingManagementMessage::~OutgoingManagementMessage() {
    free(this->outgoing->tlv);
    free(this->outgoing);
}

void OutgoingManagementMessage::packInteger64( void* i, void *buf )
{
	packUInteger32(&((Integer64*)i)->lsb, buf);
	packInteger32(&((Integer64*)i)->msb, buf + 4);
}

void OutgoingManagementMessage::packClockIdentity( ClockIdentity *c, Octet *buf)
{
	int i;
	for(i = 0; i < CLOCK_IDENTITY_LENGTH; i++) {
		packOctet(&((*c)[i]),(buf+i));
	}
}

void OutgoingManagementMessage::packPortIdentity( PortIdentity *p, Octet *buf)
{
	int offset = 0;
	PortIdentity *data = p;
	#define OPERATE( name, size, type) \
		pack##type (&data->name, buf + offset); \
		offset = offset + size;
	#include "../../src/def/derivedData/portIdentity.def"
}

void OutgoingManagementMessage::packMsgHeader(MsgHeader *h, Octet *buf)
{
    int offset = 0;

    /* set uninitalized bytes to zero */
    h->reserved0 = 0;
    h->reserved1 = 0;
    h->reserved2 = 0;

    #define OPERATE( name, size, type ) \
            pack##type( &h->name, buf + offset ); \
            offset = offset + size;
    #include "../../src/def/message/header.def"
}

void OutgoingManagementMessage::packMsgManagement(MsgManagement *m, Octet *buf)
{
    int offset = 0;
    MsgManagement *data = m;

    /* set unitialized bytes to zero */
    m->reserved0 = 0;
    m->reserved1 = 0;

    #define OPERATE( name, size, type) \
            pack##type (&data->name, buf + offset); \
            offset = offset + size;
    #include "../../src/def/message/management.def"

}

void OutgoingManagementMessage::packManagementTLV(ManagementTLV *tlv, Octet *buf)
{
    int offset = 0;
    #define OPERATE( name, size, type ) \
            pack##type( &tlv->name, buf + MANAGEMENT_LENGTH + offset ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/managementTLV.def"
}

/**
 * @brief Initialize outgoing management message fields
 */
void OutgoingManagementMessage::initOutgoingMsgManagement(/*MsgManagement* incoming, */MsgManagement* outgoing/*, PtpClock *ptpClock*/)
{ 
    /* set header fields */
    outgoing->header.transportSpecific = 0x0;
    outgoing->header.messageType = 0x0D;
    outgoing->header.versionPTP = VERSION_PTP;
    outgoing->header.domainNumber = DFLT_DOMAIN_NUMBER;
    /* set header flagField to zero for management messages, Spec 13.3.2.6 */
    outgoing->header.flagField0 = 0x00;
    outgoing->header.flagField1 = 0x00;
    outgoing->header.correctionField.msb = 0;
    outgoing->header.correctionField.lsb = 0;
    
    /* TODO: Assign value to sourcePortIdentity */
    //copyPortIdentity(&outgoing->header.sourcePortIdentity, &ptpClock->portIdentity);
    memset(&(outgoing->header.sourcePortIdentity), 0, sizeof(PortIdentity));
    
    /* TODO: Assign value to sequenceId */
    //outgoing->header.sequenceId = incoming->header.sequenceId;
    outgoing->header.sequenceId = 0;
    
    outgoing->header.controlField = 0x0; /* deprecrated for ptp version 2 */
    outgoing->header.logMessageInterval = 0x7F;

    /* set management message fields */
    /* TODO: Assign value to targetPortIdentity */
    //copyPortIdentity( &outgoing->targetPortIdentity, &incoming->header.sourcePortIdentity );
    memset(&(outgoing->targetPortIdentity), 0, sizeof(PortIdentity));
    
    /* TODO: Assign value to startingBoundaryHops */
    //outgoing->startingBoundaryHops = incoming->startingBoundaryHops - incoming->boundaryHops;
    outgoing->startingBoundaryHops = 0;
    
    outgoing->boundaryHops = outgoing->startingBoundaryHops;
    outgoing->actionField = 0; /* set default action, avoid uninitialized value */

    /* init managementTLV */
    //XMALLOC(outgoing->tlv, sizeof(ManagementTLV));
    outgoing->tlv = (ManagementTLV*) malloc (sizeof(ManagementTLV));
    outgoing->tlv->dataField = NULL;
}

/**
 * @brief Handle incoming NULL_MANAGEMENT message
 */
void OutgoingManagementMessage::handleMMNullManagement(/*MsgManagement* incoming, */MsgManagement* outgoing/*, PtpClock* ptpClock*/, Enumeration4 actionField)
{
    //DBGV("received NULL_MANAGEMENT message\n");

    initOutgoingMsgManagement(/*incoming, */outgoing/*, ptpClock*/);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_NULL_MANAGEMENT;

    switch(actionField)
    {
    case GET:
    case SET:
            //DBGV(" GET or SET mgmt msg\n");
            break;
    case COMMAND:
            //DBGV(" COMMAND mgmt msg\n");
            break;
    default:
            //DBGV(" unknown actionType \n");
            //free(outgoing->tlv);
            /*handleErrorManagementMessage(incoming, outgoing,
                    ptpClock, MM_NULL_MANAGEMENT,
                    NOT_SUPPORTED);*/
        break;
    }
}

