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
        *(type *)to = *(type *)from; \
}

#define UNPACK_ENDIAN( type, size ) \
void IncomingManagementMessage::unpack##type( void* from, void* to) \
{ \
        *(type *)to = flip##size( *(type *)from ); \
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

UNPACK_SIMPLE( Boolean )
UNPACK_SIMPLE( Enumeration8 )
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

void IncomingManagementMessage::unpackUInteger48( void *buf, void *i)
{
    unpackUInteger32(buf, &((UInteger48*)i)->lsb);
    unpackUInteger16(static_cast<char*>(buf) + 4, &((UInteger48*)i)->msb);
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

void IncomingManagementMessage::unpackClockQuality( Octet *buf, ClockQuality *c)
{
    int offset = 0;
    ClockQuality* data = c;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name); \
            offset = offset + size;
    #include "../../src/def/derivedData/clockQuality.def"
}

void IncomingManagementMessage::unpackTimeInterval( Octet *buf, TimeInterval *t)
{
    int offset = 0;
    TimeInterval* data = t;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name); \
            offset = offset + size;
    #include "../../src/def/derivedData/timeInterval.def"
}

void IncomingManagementMessage::unpackTimestamp( Octet *buf, Timestamp *t)
{
    int offset = 0;
    Timestamp* data = t;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name); \
            offset = offset + size;
    #include "../../src/def/derivedData/timestamp.def"
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

void IncomingManagementMessage::unpackPortAddress( Octet *buf, PortAddress *p)
{
    unpackEnumeration16( buf, &p->networkProtocol);
    unpackUInteger16( buf+2, &p->addressLength);
    if(p->addressLength) {
        //XMALLOC(p->addressField, p->addressLength);
        p->addressField = (Octet*) malloc (p->addressLength);
        memcpy( p->addressField, buf+4, p->addressLength);
    } else {
        p->addressField = NULL;
    }
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

void IncomingManagementMessage::unpackPhysicalAddress( Octet *buf, PhysicalAddress *p)
{
    unpackUInteger16( buf, &p->addressLength);
    if(p->addressLength) {
        //XMALLOC(p->addressField, p->addressLength);
        p->addressField = (Octet*) malloc (p->addressLength);
        memcpy( p->addressField, buf+2, p->addressLength);
    } else {
        p->addressField = NULL;
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

void IncomingManagementMessage::unpackMMClockDescription( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMClockDescription));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMClockDescription));
    MMClockDescription* data = (MMClockDescription*)m->tlv->dataField;
    memset(data, 0, sizeof(MMClockDescription));
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/clockDescription.def"

    mMClockDescription_display(data);
}

void IncomingManagementMessage::unpackMMUserDescription(Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMUserDescription));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMUserDescription));
    MMUserDescription* data = (MMUserDescription*)m->tlv->dataField;
    memset(data, 0, sizeof(MMUserDescription));
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/userDescription.def"

    mMUserDescription_display(data);
}

void IncomingManagementMessage::unpackMMInitialize( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMInitialize));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMInitialize));
    MMInitialize* data = (MMInitialize*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/initialize.def"

    mMInitialize_display(data);
}

void IncomingManagementMessage::unpackMMDefaultDataSet( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMDefaultDataSet));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMDefaultDataSet));
    MMDefaultDataSet* data = (MMDefaultDataSet*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/defaultDataSet.def"

    mMDefaultDataSet_display(data);
}

void IncomingManagementMessage::unpackMMCurrentDataSet( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMCurrentDataSet));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMCurrentDataSet));
    MMCurrentDataSet* data = (MMCurrentDataSet*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/currentDataSet.def"

    mMCurrentDataSet_display(data);
}

void IncomingManagementMessage::unpackMMParentDataSet( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMParentDataSet));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMParentDataSet));
    MMParentDataSet* data = (MMParentDataSet*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/parentDataSet.def"

    mMParentDataSet_display(data);
}

void IncomingManagementMessage::unpackMMTimePropertiesDataSet( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMTimePropertiesDataSet));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMTimePropertiesDataSet));
    MMTimePropertiesDataSet* data = (MMTimePropertiesDataSet*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/timePropertiesDataSet.def"

    mMTimePropertiesDataSet_display(data);
}

void IncomingManagementMessage::unpackMMPortDataSet( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMPortDataSet));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMPortDataSet));
    MMPortDataSet* data = (MMPortDataSet*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/portDataSet.def"

    mMPortDataSet_display(data);
}

void IncomingManagementMessage::unpackMMPriority1( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMPriority1));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMPriority1));
    MMPriority1* data = (MMPriority1*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/priority1.def"

    mMPriority1_display(data);
}

void IncomingManagementMessage::unpackMMPriority2( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMPriority2));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMPriority2));
    MMPriority2* data = (MMPriority2*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/priority2.def"

    mMPriority2_display(data);
}

void IncomingManagementMessage::unpackMMDomain( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMDomain));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMDomain));
    MMDomain* data = (MMDomain*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/domain.def"

    mMDomain_display(data);
}

void IncomingManagementMessage::unpackMMSlaveOnly( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMSlaveOnly));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMSlaveOnly));
    MMSlaveOnly* data = (MMSlaveOnly*)m->tlv->dataField;
    /* see src/def/README for a note on this X-macro */
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/slaveOnly.def"

    mMSlaveOnly_display(data);
}

void IncomingManagementMessage::unpackMMLogAnnounceInterval( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMLogAnnounceInterval));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMLogAnnounceInterval));
    MMLogAnnounceInterval* data = (MMLogAnnounceInterval*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/logAnnounceInterval.def"

    mMLogAnnounceInterval_display(data);
}

void IncomingManagementMessage::unpackMMAnnounceReceiptTimeout( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField,sizeof(MMAnnounceReceiptTimeout));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMAnnounceReceiptTimeout));
    MMAnnounceReceiptTimeout* data = (MMAnnounceReceiptTimeout*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/announceReceiptTimeout.def"

    mMAnnounceReceiptTimeout_display(data);
}

void IncomingManagementMessage::unpackMMLogSyncInterval( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMLogSyncInterval));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMLogSyncInterval));
    MMLogSyncInterval* data = (MMLogSyncInterval*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/logSyncInterval.def"

    mMLogSyncInterval_display(data);
}

void IncomingManagementMessage::unpackMMVersionNumber( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMVersionNumber));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMVersionNumber));
    MMVersionNumber* data = (MMVersionNumber*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/versionNumber.def"

    mMVersionNumber_display(data);
}

void IncomingManagementMessage::unpackMMTime( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMTime));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMTime));
    MMTime* data = (MMTime*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/time.def"

    mMTime_display(data);
}

void IncomingManagementMessage::unpackMMClockAccuracy( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMClockAccuracy));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMClockAccuracy));
    MMClockAccuracy* data = (MMClockAccuracy*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/clockAccuracy.def"

    mMClockAccuracy_display(data);
}

void IncomingManagementMessage::unpackMMUtcProperties( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMUtcProperties));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMUtcProperties));
    MMUtcProperties* data = (MMUtcProperties*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/utcProperties.def"

    mMUtcProperties_display(data);
}

void IncomingManagementMessage::unpackMMTraceabilityProperties( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMTraceabilityProperties));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMTraceabilityProperties));
    MMTraceabilityProperties* data = (MMTraceabilityProperties*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/traceabilityProperties.def"

    mMTraceabilityProperties_display(data);
}

void IncomingManagementMessage::unpackMMDelayMechanism( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMDelayMechanism));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMDelayMechanism));
    MMDelayMechanism* data = (MMDelayMechanism*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/delayMechanism.def"

    mMDelayMechanism_display(data);
}

void IncomingManagementMessage::unpackMMLogMinPdelayReqInterval( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    //XMALLOC(m->tlv->dataField, sizeof(MMLogMinPdelayReqInterval));
    m->tlv->dataField = (Octet*) malloc (sizeof(MMLogMinPdelayReqInterval));
    MMLogMinPdelayReqInterval* data = (MMLogMinPdelayReqInterval*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/logMinPdelayReqInterval.def"

    mMLogMinPdelayReqInterval_display(data);
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

    mMErrorStatus_display(data);
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
            DBG("handleManagement: Clock Description\n");
            unpackMMClockDescription(buf, incoming);
            handleMMClockDescription(incoming);
            break;
                
        case MM_USER_DESCRIPTION:
            DBG("handleManagement: User Description\n");
            unpackMMUserDescription(buf, incoming);
            handleMMUserDescription(incoming);
            break;
                
        case MM_SAVE_IN_NON_VOLATILE_STORAGE:
            DBG("handleManagement: Save In Non-Volatile Storage\n");
            handleMMSaveInNonVolatileStorage(incoming);
            break;
            
        case MM_RESET_NON_VOLATILE_STORAGE:
            DBG("handleManagement: Reset Non-Volatile Storage\n");
            handleMMResetNonVolatileStorage(incoming);
            break;
            
        case MM_INITIALIZE:
            DBG("handleManagement: Initialize\n");
            unpackMMInitialize(buf, incoming);
            handleMMInitialize(incoming);
            break;
                
        case MM_DEFAULT_DATA_SET:
            DBG("handleManagement: Default Data Set\n");
            unpackMMDefaultDataSet(buf, incoming);
            handleMMDefaultDataSet(incoming);
            break;
            
        case MM_CURRENT_DATA_SET:
            DBG("handleManagement: Current Data Set\n");
            unpackMMCurrentDataSet(buf, incoming);
            handleMMCurrentDataSet(incoming);
            break;
                
        case MM_PARENT_DATA_SET:
            DBG("handleManagement: Parent Data Set\n");
            unpackMMParentDataSet(buf, incoming);
            handleMMParentDataSet(incoming);
            break;
                
        case MM_TIME_PROPERTIES_DATA_SET:
            DBG("handleManagement: TimeProperties Data Set\n");
            unpackMMTimePropertiesDataSet(buf, incoming);
            handleMMTimePropertiesDataSet(incoming);
            break;
                
        case MM_PORT_DATA_SET:
            DBG("handleManagement: Port Data Set\n");
            unpackMMPortDataSet(buf, incoming);
            handleMMPortDataSet(incoming);
            break;
                
        case MM_PRIORITY1:
            DBG("handleManagement: Priority1\n");
            unpackMMPriority1(buf, incoming);
            handleMMPriority1(incoming);
            break;
                
        case MM_PRIORITY2:
            DBG("handleManagement: Priority2\n");
            unpackMMPriority2(buf, incoming);
            handleMMPriority2(incoming);
            break;
            
        case MM_DOMAIN:
            DBG("handleManagement: Domain\n");
            unpackMMDomain(buf, incoming);
            handleMMDomain(incoming);
            break;
                
       case MM_SLAVE_ONLY:
            DBG("handleManagement: Slave Only\n");
            unpackMMSlaveOnly(buf, incoming);
            handleMMSlaveOnly(incoming);
            break;
            
        case MM_LOG_ANNOUNCE_INTERVAL:
            DBG("handleManagement: Log Announce Interval\n");
            unpackMMLogAnnounceInterval(buf, incoming);
            handleMMLogAnnounceInterval(incoming);
            break;
            
        case MM_ANNOUNCE_RECEIPT_TIMEOUT:
            DBG("handleManagement: Announce Receipt Timeout\n");
            unpackMMAnnounceReceiptTimeout(buf, incoming);
            handleMMAnnounceReceiptTimeout(incoming);
            break;
            
        case MM_LOG_SYNC_INTERVAL:
            DBG("handleManagement: Log Sync Interval\n");
            unpackMMLogSyncInterval(buf, incoming);
            handleMMLogSyncInterval(incoming);
            break;
            
        case MM_VERSION_NUMBER:
            DBG("handleManagement: Version Number\n");
            unpackMMVersionNumber(buf, incoming);
            handleMMVersionNumber(incoming);
            break;
            
        case MM_ENABLE_PORT:
            DBG("handleManagement: Enable Port\n");
            handleMMEnablePort(incoming);
            break;
            
        case MM_DISABLE_PORT:
            DBG("handleManagement: Disable Port\n");
            handleMMDisablePort(incoming);
            break;
            
        case MM_TIME:
            DBG("handleManagement: Time\n");
            unpackMMTime(buf, incoming);
            handleMMTime(incoming);
            break;
                
        case MM_CLOCK_ACCURACY:
            DBG("handleManagement: Clock Accuracy\n");
            unpackMMClockAccuracy(buf, incoming);
            handleMMClockAccuracy(incoming);
            break;
                
        case MM_UTC_PROPERTIES:
            DBG("handleManagement: Utc Properties\n");
            unpackMMUtcProperties(buf, incoming);
            handleMMUtcProperties(incoming);
            break;
                
        case MM_TRACEABILITY_PROPERTIES:
            DBG("handleManagement: Traceability Properties\n");
            unpackMMTraceabilityProperties(buf, incoming);
            handleMMTraceabilityProperties(incoming);
            break;
                
        case MM_DELAY_MECHANISM:
            DBG("handleManagement: Delay Mechanism\n");
            unpackMMDelayMechanism(buf, incoming);
            handleMMDelayMechanism(incoming);
            break;
                
        case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
            DBG("handleManagement: Log Min Pdelay Req Interval\n");
            unpackMMLogMinPdelayReqInterval(buf, incoming);
            handleMMLogMinPdelayReqInterval(incoming);
            break;
                
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

/**
 * @brief Handle incoming CLOCK_DESCRIPTION management message
 */
void IncomingManagementMessage::handleMMClockDescription(MsgManagement* incoming)
{
    DBG("received CLOCK_DESCRIPTION management message \n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            ERROR(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming USER_DESCRIPTION management message type
 */
void IncomingManagementMessage::handleMMUserDescription(MsgManagement* incoming)
{
    DBG("received USER_DESCRIPTION message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            ERROR(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming SAVE_IN_NON_VOLATILE_STORAGE management message type
 */
void IncomingManagementMessage::handleMMSaveInNonVolatileStorage(MsgManagement* incoming)
{
    DBG("received SAVE_IN_NON_VOLATILE_STORAGE message\n");

    switch( incoming->actionField )
    {
        case ACKNOWLEDGE:
            printf("management message not supported \n");
            exit(1);
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming RESET_NON_VOLATILE_STORAGE management message type
 */
void IncomingManagementMessage::handleMMResetNonVolatileStorage(MsgManagement* incoming)
{
    DBG("received RESET_NON_VOLATILE_STORAGE message\n");

    switch( incoming->actionField )
    {
        case ACKNOWLEDGE:
            printf("management message not supported \n");
            exit(1);
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming INITIALIZE management message type
 */
void IncomingManagementMessage::handleMMInitialize(MsgManagement* incoming)
{
    DBG("received INITIALIZE message\n");

    switch( incoming->actionField )
    {
        case ACKNOWLEDGE:
            DBG(" ACKNOWLEDGE action\n");
            /* TODO: implementation specific */
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming DEFAULT_DATA_SET management message type
 */
void IncomingManagementMessage::handleMMDefaultDataSet(MsgManagement* incoming)
{
    DBG("received DEFAULT_DATA_SET message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming CURRENT_DATA_SET management message type
 */
void IncomingManagementMessage::handleMMCurrentDataSet(MsgManagement* incoming)
{
    DBG("received CURRENT_DATA_SET message\n");

    switch( incoming->actionField )
    {
        case RESPONSE:
            DBG(" RESPONSE action\n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming PARENT_DATA_SET management message type
 */
void IncomingManagementMessage::handleMMParentDataSet(MsgManagement* incoming)
{
    DBG("received PARENT_DATA_SET message\n");

    switch( incoming->actionField )
    {
        case RESPONSE:
            DBG(" RESPONSE action\n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming PROPERTIES_DATA_SET management message type
 */
void IncomingManagementMessage::handleMMTimePropertiesDataSet(MsgManagement* incoming)
{
    DBG("received TIME_PROPERTIES message\n");

    switch( incoming->actionField )
    {
        case RESPONSE:
            DBG(" RESPONSE action\n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming PORT_DATA_SET management message type
 */
void IncomingManagementMessage::handleMMPortDataSet(MsgManagement* incoming)
{
    DBG("received PORT_DATA_SET message\n");

    switch( incoming->actionField )
    {
        case RESPONSE:
            DBG(" RESPONSE action\n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming PRIORITY1 management message type
 */
void IncomingManagementMessage::handleMMPriority1(MsgManagement* incoming)
{
    DBG("received PRIORITY1 message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming PRIORITY2 management message type
 */
void IncomingManagementMessage::handleMMPriority2(MsgManagement* incoming)
{
    DBG("received PRIORITY2 message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming DOMAIN management message type
 */
void IncomingManagementMessage::handleMMDomain(MsgManagement* incoming)
{
    DBG("received DOMAIN message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming SLAVE_ONLY management message type
 */
void IncomingManagementMessage::handleMMSlaveOnly(MsgManagement* incoming)
{
    DBG("received SLAVE_ONLY management message \n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming LOG_ANNOUNCE_INTERVAL management message type
 */
void IncomingManagementMessage::handleMMLogAnnounceInterval(MsgManagement* incoming)
{
    DBG("received LOG_ANNOUNCE_INTERVAL message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming ANNOUNCE_RECEIPT_TIMEOUT management message type
 */
void IncomingManagementMessage::handleMMAnnounceReceiptTimeout(MsgManagement* incoming)
{
    DBG("received ANNOUNCE_RECEIPT_TIMEOUT message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming LOG_SYNC_INTERVAL management message type
 */
void IncomingManagementMessage::handleMMLogSyncInterval(MsgManagement* incoming)
{
    DBG("received LOG_SYNC_INTERVAL message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming VERSION_NUMBER management message type
 */
void IncomingManagementMessage::handleMMVersionNumber(MsgManagement* incoming)
{
    DBG("received VERSION_NUMBER message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming ENABLE_PORT management message type
 */
void IncomingManagementMessage::handleMMEnablePort(MsgManagement* incoming)
{
    DBG("received ENABLE_PORT message\n");

    switch( incoming->actionField )
    {
        case ACKNOWLEDGE:
            DBG(" ACKNOWLEDGE action\n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming DISABLE_PORT management message type
 */
void IncomingManagementMessage::handleMMDisablePort(MsgManagement* incoming)
{
    DBG("received DISABLE_PORT message\n");

    switch( incoming->actionField )
    {
        case ACKNOWLEDGE:
            DBG(" ACKNOWLEDGE action\n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming TIME management message type
 */
void IncomingManagementMessage::handleMMTime(MsgManagement* incoming)
{
    DBG("received TIME message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming CLOCK_ACCURACY management message type
 */
void IncomingManagementMessage::handleMMClockAccuracy(MsgManagement* incoming)
{
    DBG("received CLOCK_ACCURACY message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming UTC_PROPERTIES management message type
 */
void IncomingManagementMessage::handleMMUtcProperties(MsgManagement* incoming)
{
    DBG("received UTC_PROPERTIES message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming TRACEABILITY_PROPERTIES management message type
 */
void IncomingManagementMessage::handleMMTraceabilityProperties(MsgManagement* incoming)
{
    DBG("received TRACEABILITY_PROPERTIES message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming DELAY_MECHANISM management message type
 */
void IncomingManagementMessage::handleMMDelayMechanism(MsgManagement* incoming)
{
    DBG("received DELAY_MECHANISM message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming LOG_MIN_PDELAY_REQ_INTERVAL management message type
 */
void IncomingManagementMessage::handleMMLogMinPdelayReqInterval(MsgManagement* incoming)
{
    DBG("received LOG_MIN_PDELAY_REQ_INTERVAL message\n");

    switch(incoming->actionField)
    {
        case RESPONSE:
            DBG(" RESPONSE action \n");
            break;
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming ERROR_STATUS management message type
 */
void IncomingManagementMessage::handleMMErrorStatus(MsgManagement *incoming)
{
	DBG("received MANAGEMENT_ERROR_STATUS message \n");
	/* implementation specific */
}