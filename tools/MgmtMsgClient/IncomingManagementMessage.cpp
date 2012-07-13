/** 
 * @file        IncomingManagementMessage.cpp
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       IncomingManagementMessage class implementation.
 * 
 * This class is used to handle incoming management messages.
 */

#include "IncomingManagementMessage.h"

#include <netinet/in.h>
#include <stdlib.h>

#include "app_dep.h"
#include "constants_dep.h"
#include "datatypes.h"
#include "datatypes_dep.h"
#include "display.h"

#define UNPACK_SIMPLE( type ) \
void IncomingManagementMessage::unpack##type( void* from, void* to) \
{ \
        *(type *)from = *(type *)to; \
}

#define UNPACK_ENDIAN( type, size ) \
void IncomingManagementMessage::unpack##type( void* from, void* to) \
{ \
        *(type *)from = flip##size( *(type *)to ); \
}

#define UNPACK_LOWER_AND_UPPER( type ) \
void IncomingManagementMessage::unpack##type##Lower( void* from, void* to) \
{ \
	*(type *)to = *(char *)from & 0x0F; \
} \
\
void IncomingManagementMessage::unpack##type##Upper( void* from, void* to) \
{ \
	*(type *)to = *(char *)from & 0xF0; \
}

UNPACK_SIMPLE( Integer8 )
UNPACK_SIMPLE( UInteger8 )
UNPACK_SIMPLE( Octet )

UNPACK_ENDIAN( Enumeration16, 16 )
UNPACK_ENDIAN( Integer16, 16 )
UNPACK_ENDIAN( UInteger16, 16 )
UNPACK_ENDIAN( Integer32, 32 )
UNPACK_ENDIAN( UInteger32, 32 )

UNPACK_LOWER_AND_UPPER( Enumeration4 )
UNPACK_LOWER_AND_UPPER( Nibble )
UNPACK_LOWER_AND_UPPER( UInteger4 )

/**
 * @brief IncomingManagementMessage constructor.
 * 
 * @param buf           Buffer with a received message.
 * @param optBuf        Buffer with application options.
 * 
 * The constructor allocates memory and trigger all necessary actions.
 */
IncomingManagementMessage::IncomingManagementMessage(Octet* buf, OptBuffer* optBuf) {
    this->incoming = (MsgManagement *)malloc(sizeof(MsgManagement));
    
    msgUnpackManagement(buf, this->incoming);
}

/**
 * @brief IncomingManagementMessage deconstructor.
 * 
 * The deconstructor frees memory.
 */
IncomingManagementMessage::~IncomingManagementMessage() {
    free(this->incoming->tlv);
    free(this->incoming);
}

/**
 * @brief Unpack an Integer64 type.
 * 
 * @param buf   Buffer with a received message.
 * @param i     Integer64 object to unpack.
 */
void IncomingManagementMessage::unpackInteger64( void *buf, void *i)
{
    unpackUInteger32(buf, &((Integer64*)i)->lsb);
    unpackInteger32(static_cast<char*>(buf) + 4, &((Integer64*)i)->msb);
}

/**
 * @brief Unpack a Clockidentity Structure.
 * 
 * @param buf   Buffer with a received message.
 * @param c     Clockidentity Structure to unpack.
 */
void IncomingManagementMessage::unpackClockIdentity( Octet *buf, ClockIdentity *c)
{
    int i;
    for(i = 0; i < CLOCK_IDENTITY_LENGTH; i++) {
            unpackOctet((buf+i),&((*c)[i]));
    }
}

/**
 * @brief Unpack a PortIdentity Structure.
 * 
 * @param buf   Buffer with a received message.
 * @param p     PortIdentity Structure to unpack.
 */
void IncomingManagementMessage::unpackPortIdentity( Octet *buf, PortIdentity *p)
{
    int offset = 0;
    PortIdentity* data = p;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name); \
            offset = offset + size;
    #include "../../src/def/derivedData/portIdentity.def"
}

/**
 * @brief Unpack message header.
 * 
 * @param buf           Buffer with a received message.
 * @param header        Message header to unpack.      
 */
void IncomingManagementMessage::unpackMsgHeader(Octet *buf, MsgHeader *header)
{
    int offset = 0;
    MsgHeader* data = header;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name); \
            offset = offset + size;
    #include "../../src/def/message/header.def"

    msgHeader_display(data);
}

/**
 * @brief Unpack management message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message to unpack.
 */
void IncomingManagementMessage::unpackMsgManagement(Octet *buf, MsgManagement *m)
{
    int offset = 0;
    MsgManagement* data = m;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name); \
            offset = offset + size;
    #include "../../src/def/message/management.def"

    msgManagement_display(data);
}

/**
 * @brief Unpack management TLV.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with TLV to unpack.
 */
void IncomingManagementMessage::unpackManagementTLV(Octet *buf, MsgManagement *m)
{
    int offset = 0;
    //XMALLOC(m->tlv, sizeof(ManagementTLV));
    m->tlv = (ManagementTLV*) malloc (sizeof(ManagementTLV));
    /* read the management TLV */
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + offset, &m->tlv->name); \
            offset = offset + size;
    #include "../../src/def/managementTLV/managementTLV.def"
}

/**
 * @brief Handle management message unpacking.
 * 
 * @param buf           Buffer with a received message.
 * @param manage        Management message to be handled.        
 */
void IncomingManagementMessage::msgUnpackManagement(Octet *buf, MsgManagement * manage)
{
	unpackMsgManagement(buf, manage);

	if ( manage->header.messageLength > MANAGEMENT_LENGTH )
	{
		unpackManagementTLV(buf, manage);

		/* at this point, we know what managementTLV we have, so return and
		 * let someone else handle the data */
		manage->tlv->dataField = NULL;
	}
	else /* no TLV attached to this message */
	{
		manage->tlv = NULL;
	}
}