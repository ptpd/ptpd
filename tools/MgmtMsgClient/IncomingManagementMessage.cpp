/** 
 * @file        IncomingManagementMessage.cpp
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       IncomingManagementMessage class implementation
 */

#include <netinet/in.h>
#include <stdlib.h>

#include "IncomingManagementMessage.h"
#include "app_dep.h"
#include "constants_dep.h"
#include "datatypes.h"
#include "datatypes_dep.h"
#include "Display.h"

#define UNPACK_SIMPLE( type ) \
void IncomingManagementMessage::unpack##type( void* from, void* to/*, PtpClock *ptpClock */) \
{ \
        *(type *)from = *(type *)to; \
}

#define UNPACK_ENDIAN( type, size ) \
void IncomingManagementMessage::unpack##type( void* from, void* to/*, PtpClock *ptpClock */) \
{ \
        *(type *)from = flip##size( *(type *)to ); \
}

#define UNPACK_LOWER_AND_UPPER( type ) \
void IncomingManagementMessage::unpack##type##Lower( void* from, void* to/*, PtpClock *ptpClock */) \
{ \
	*(type *)to = *(char *)from & 0x0F; \
} \
\
void IncomingManagementMessage::unpack##type##Upper( void* from, void* to/*, PtpClock *ptpClock */) \
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

IncomingManagementMessage::IncomingManagementMessage(Octet* buf, OptBuffer* optBuf) {
    this->incoming = (MsgManagement *)malloc(sizeof(MsgManagement));
    
//    unpackMsgManagement(buf, this->incoming);
//    unpackManagementTLV(buf, this->incoming);
    msgUnpackManagement(buf, this->incoming);
}

IncomingManagementMessage::~IncomingManagementMessage() {
    free(this->incoming->tlv);
    free(this->incoming);
}

void IncomingManagementMessage::unpackInteger64( void *buf, void *i/*, PtpClock *ptpClock*/)
{
    unpackUInteger32(buf, &((Integer64*)i)->lsb/*, ptpClock*/);
    //unpackInteger32(buf + 4, &((Integer64*)i)->msb/*, ptpClock*/);
    unpackInteger32(static_cast<char*>(buf) + 4, &((Integer64*)i)->msb/*, ptpClock*/);
}

void IncomingManagementMessage::unpackClockIdentity( Octet *buf, ClockIdentity *c/*, PtpClock *ptpClock*/)
{
    int i;
    for(i = 0; i < CLOCK_IDENTITY_LENGTH; i++) {
            unpackOctet((buf+i),&((*c)[i])/*, ptpClock*/);
    }
}

void IncomingManagementMessage::unpackPortIdentity( Octet *buf, PortIdentity *p/*, PtpClock *ptpClock*/)
{
    int offset = 0;
    PortIdentity* data = p;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name/*, ptpClock*/); \
            offset = offset + size;
    #include "../../src/def/derivedData/portIdentity.def"
}

void IncomingManagementMessage::unpackMsgHeader(Octet *buf, MsgHeader *header/*, PtpClock *ptpClock*/)
{
    int offset = 0;
    MsgHeader* data = header;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name/*, ptpClock*/); \
            offset = offset + size;
    #include "../../src/def/message/header.def"

    msgHeader_display(data);
}

void IncomingManagementMessage::unpackMsgManagement(Octet *buf, MsgManagement *m/*, PtpClock *ptpClock*/)
{
    int offset = 0;
    MsgManagement* data = m;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name/*, ptpClock*/); \
            offset = offset + size;
    #include "../../src/def/message/management.def"

//	#ifdef PTPD_DBG
    msgManagement_display(data);
//	#endif /* PTPD_DBG */
}

void IncomingManagementMessage::unpackManagementTLV(Octet *buf, MsgManagement *m/*, PtpClock* ptpClock*/)
{
    int offset = 0;
    //XMALLOC(m->tlv, sizeof(ManagementTLV));
    m->tlv = (ManagementTLV*) malloc (sizeof(ManagementTLV));
    /* read the management TLV */
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + offset, &m->tlv->name/*, ptpClock */); \
            offset = offset + size;
    #include "../../src/def/managementTLV/managementTLV.def"
}

void IncomingManagementMessage::msgUnpackManagement(Octet *buf, MsgManagement * manage/*, MsgHeader * header*//*, PtpClock *ptpClock*/)
{
	unpackMsgManagement(buf, manage/*, ptpClock*/);

	if ( manage->header.messageLength > MANAGEMENT_LENGTH )
	{
		unpackManagementTLV(buf, manage/*, ptpClock*/);

		/* at this point, we know what managementTLV we have, so return and
		 * let someone else handle the data */
		manage->tlv->dataField = NULL;
	}
	else /* no TLV attached to this message */
	{
		manage->tlv = NULL;
	}
}