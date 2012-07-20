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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MgmtMsgClient.h"

#include "app_dep.h"
#include "constants_dep.h"
#include "datatypes.h"
#include "datatypes_dep.h"
#include "display.h"
#include "freeing.h"

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
    
    handleManagement(buf, this->incoming);
}

/**
 * @brief IncomingManagementMessage deconstructor.
 * 
 * The deconstructor frees memory.
 */
IncomingManagementMessage::~IncomingManagementMessage() {
//    if (this->incoming->tlv != NULL)
//        free(this->incoming->tlv);
    freeManagementTLV(this->incoming);
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

void IncomingManagementMessage::unpackPTPText( Octet *buf, PTPText *s)
{
    unpackUInteger8(buf, &s->lengthField);
    if(s->lengthField) {
        //XMALLOC(s->textField, s->lengthField);
        s->textField = (Octet*) malloc (s->lengthField);
        memcpy( s->textField, buf+1, s->lengthField);
    } else {
        s->textField = NULL;
    }
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

    /* TODO: if data->messageType != MANAGEMENT then report ERROR (or not if 
     TLV unsupported) */
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

	if (manage->header.messageLength > MANAGEMENT_LENGTH)
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

void IncomingManagementMessage::unpackMMErrorStatus(Octet *buf, MsgManagement* m)
{
        int offset = 0;
        //XMALLOC(m->tlv->dataField, sizeof(MMErrorStatus));
        m->tlv->dataField = (Octet*) malloc (sizeof(MMErrorStatus));
        MMErrorStatus* data = (MMErrorStatus*)m->tlv->dataField;
        #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset, &data->name ); \
            offset = offset + size;
        #include "../../src/def/managementTLV/errorStatus.def"

        //#ifdef PTPD_DBG
        mMErrorStatus_display(data);
        //#endif /* PTPD_DBG */
}

void IncomingManagementMessage::handleManagement(/*OptBuffer* optBuf, */Octet* buf, MsgManagement* incoming)
{
    DBG("Management message received : \n");

//    if (isFromSelf) {
//        DBG("handleManagement: Ignore message from self \n");
//        return;
//    }

    msgUnpackManagement(buf, incoming);

    if(incoming->tlv == NULL) {
        DBG("handleManagement: TLV is empty\n");
        return;
    }

    /* is this an error status management TLV? */
    if(incoming->tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS) {
        DBG("handleManagement: Error Status TLV\n");
        unpackMMErrorStatus(buf, incoming);
        handleMMErrorStatus(incoming);
//            /* cleanup msgTmp managementTLV */
//            if(ptpClock->msgTmp.manage.tlv) {
//                    DBGV("cleanup ptpClock->msgTmp.manage message \n");
//                    if(ptpClock->msgTmp.manage.tlv->dataField) {
//                            freeMMErrorStatusTLV(ptpClock->msgTmp.manage.tlv);
//                            free(ptpClock->msgTmp.manage.tlv->dataField);
//                    }
//                    free(ptpClock->msgTmp.manage.tlv);
//            }
        return;
    } else if (incoming->tlv->tlvType != TLV_MANAGEMENT) {
        /* do nothing, implemention specific handling */
        DBG("handleManagement: Currently unsupported management TLV type\n");
        return;
    }
        
    switch(incoming->tlv->managementId)
    {
        case MM_NULL_MANAGEMENT:
            DBG("handleManagement: Null Management\n");
            //handleMMNullManagement(outgoing, optBuf->action_type);
            break;
                
        case MM_CLOCK_DESCRIPTION:
        case MM_USER_DESCRIPTION:
        case MM_SAVE_IN_NON_VOLATILE_STORAGE:
        case MM_RESET_NON_VOLATILE_STORAGE:
        case MM_INITIALIZE:
        case MM_DEFAULT_DATA_SET:
        case MM_CURRENT_DATA_SET:
        case MM_PARENT_DATA_SET:
        case MM_TIME_PROPERTIES_DATA_SET:
        case MM_PORT_DATA_SET:
        case MM_PRIORITY1:
        case MM_PRIORITY2:
        case MM_DOMAIN:
        case MM_SLAVE_ONLY:
        case MM_LOG_ANNOUNCE_INTERVAL:
        case MM_ANNOUNCE_RECEIPT_TIMEOUT:
        case MM_LOG_SYNC_INTERVAL:
        case MM_VERSION_NUMBER:
        case MM_ENABLE_PORT:
        case MM_DISABLE_PORT:
        case MM_TIME:
        case MM_CLOCK_ACCURACY:
        case MM_UTC_PROPERTIES:
        case MM_TRACEABILITY_PROPERTIES:
        case MM_DELAY_MECHANISM:
        case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
        case MM_FAULT_LOG:
        case MM_FAULT_LOG_RESET:
        case MM_TIMESCALE_PROPERTIES:
        case MM_UNICAST_NEGOTIATION_ENABLE:
        case MM_PATH_TRACE_LIST:
        case MM_PATH_TRACE_ENABLE:
        case MM_GRANDMASTER_CLUSTER_TABLE:
        case MM_UNICAST_MASTER_TABLE:
        case MM_UNICAST_MASTER_MAX_TABLE_SIZE:
        case MM_ACCEPTABLE_MASTER_TABLE:
        case MM_ACCEPTABLE_MASTER_TABLE_ENABLED:
        case MM_ACCEPTABLE_MASTER_MAX_TABLE_SIZE:
        case MM_ALTERNATE_MASTER:
        case MM_ALTERNATE_TIME_OFFSET_ENABLE:
        case MM_ALTERNATE_TIME_OFFSET_NAME:
        case MM_ALTERNATE_TIME_OFFSET_MAX_KEY:
        case MM_ALTERNATE_TIME_OFFSET_PROPERTIES:
        case MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
        case MM_TRANSPARENT_CLOCK_PORT_DATA_SET:
        case MM_PRIMARY_DOMAIN:
            printf("handleManagement: Currently unsupported managementTLV %d\n", incoming->tlv->managementId);
            exit(1);
        
        default:
            printf("handleManagement: Unknown managementTLV %d\n", incoming->tlv->managementId);
            exit(1);
    }
}

/**\brief Handle incoming ERROR_STATUS management message type*/
void IncomingManagementMessage::handleMMErrorStatus(MsgManagement *incoming)
{
	DBG("received MANAGEMENT_ERROR_STATUS message \n");
	/* implementation specific */
}