/*-
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments,
 *                         Tomasz Kleinschmidt
 * Copyright (c) 2009-2010 George V. Neville-Neil, 
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen
 *
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
 *
 * All Rights Reserved
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** 
 * @file        OutgoingManagementMessage.cpp
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       OutgoingManagementMessage class implementation.
 * 
 * This class is used to handle outgoing management messages.
 */

#include "OutgoingManagementMessage.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "MgmtMsgClient.h"

#include "app_dep.h"
#include "constants_dep.h"
#include "freeing.h"

//Temporary for assigning ClockIdentity
#include <string.h>

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
PACK_SIMPLE( Enumeration8 )
PACK_SIMPLE( Integer8 )
PACK_SIMPLE( UInteger8 )
PACK_SIMPLE( Octet )

PACK_ENDIAN( Enumeration16, 16 )
PACK_ENDIAN( Integer16, 16 )
PACK_ENDIAN( UInteger16, 16 )
PACK_ENDIAN( Integer32, 32 )
PACK_ENDIAN( UInteger32, 32 )

PACK_LOWER_AND_UPPER( Enumeration4 )
PACK_LOWER_AND_UPPER( Nibble )
PACK_LOWER_AND_UPPER( UInteger4 )

/**
 * @brief OutgoingManagementMessage constructor.
 * 
 * @param buf           Buffer with a message to send.
 * @param optBuf        Buffer with application options.
 * 
 * The constructor allocates memory and trigger all necessary actions.
 */
OutgoingManagementMessage::OutgoingManagementMessage(Octet* buf, OptBuffer* optBuf) {
    XMALLOC(this->outgoing, MsgManagement*, sizeof(MsgManagement));
    
    handleManagement(optBuf, buf, this->outgoing);
}

/**
 * @brief OutgoingManagementMessage deconstructor.
 * 
 * The deconstructor frees memory.
 */
OutgoingManagementMessage::~OutgoingManagementMessage() {
    freeManagementTLV(this->outgoing);
    free(this->outgoing);
}

/**
 * @brief Pack an Integer48 type.
 * 
 * @param buf   Buffer with a message to send.
 * @param i     Integer48 object to pack.
 */
void OutgoingManagementMessage::packUInteger48( void *i, void *buf)
{
    packUInteger32(&((UInteger48*)i)->lsb, buf);
    packUInteger16(&((UInteger48*)i)->msb, static_cast<char*>(buf) + 4);
}

/**
 * @brief Pack an Integer64 type.
 * 
 * @param buf   Buffer with a message to send.
 * @param i     Integer64 object to pack.
 */
void OutgoingManagementMessage::packInteger64( void* i, void *buf )
{
    packUInteger32(&((Integer64*)i)->lsb, buf);
    packInteger32(&((Integer64*)i)->msb, static_cast<char*>(buf) + 4);
}

/**
 * @brief Pack a Clockidentity Structure.
 * 
 * @param c     Clockidentity Structure to pack.
 * @param buf   Buffer with a message to send.
 */
void OutgoingManagementMessage::packClockIdentity( ClockIdentity *c, Octet *buf)
{
    int i;
    for(i = 0; i < CLOCK_IDENTITY_LENGTH; i++) {
        packOctet(&((*c)[i]),(buf+i));
    }
}

/**
 * @brief Pack a Timestamp Structure.
 * 
 * @param t     Timestamp Structure to pack.
 * @param buf   Buffer with a message to send.
 */
void OutgoingManagementMessage::packTimestamp( Timestamp *t, Octet *buf)
{
    int offset = 0;
    Timestamp *data = t;
    #define OPERATE( name, size, type) \
        pack##type (&data->name, buf + offset); \
        offset = offset + size;
    #include "../../src/def/derivedData/timestamp.def"
}

/**
 * @brief Pack a PortIdentity Structure.
 * 
 * @param p     PortIdentity Structure to pack.
 * @param buf   Buffer with a message to send.
 */
void OutgoingManagementMessage::packPortIdentity( PortIdentity *p, Octet *buf)
{
    int offset = 0;
    PortIdentity *data = p;
    #define OPERATE( name, size, type) \
        pack##type (&data->name, buf + offset); \
        offset = offset + size;
    #include "../../src/def/derivedData/portIdentity.def"
}

/**
 * @brief Pack a PTPText Structure.
 * 
 * @param s     PTPText Structure to pack.
 * @param buf   Buffer with a message to send.
 */
void OutgoingManagementMessage::packPTPText(PTPText *s, Octet *buf)
{
    packUInteger8(&s->lengthField, buf);
    if(s->lengthField) {
        memcpy( buf+1, s->textField, s->lengthField);
    }
}

/**
 * @brief Pack a User Description ManagementTLV message.
 * 
 * @param m     Management message with a User Description ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMUserDescription( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    Octet pad = 0;
    MMUserDescription* data = (MMUserDescription*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset); \
        offset = offset + size;
    #include "../../src/def/managementTLV/userDescription.def"

    /* is the TLV length odd? TLV must be even according to Spec 5.3.8 */
    if(offset % 2) {
        /* add pad of 1 according to Table 41 to make TLV length even */
        packOctet(&pad, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset);
        offset = offset + 1;
    }

    /* return length */
    return offset;
}

/**
 * @brief Pack an Initialize ManagementTLV message.
 * 
 * @param m     Management message with an Initialize ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMInitialize( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMInitialize* data = (MMInitialize*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/initialize.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Priority1 ManagementTLV message.
 * 
 * @param m     Management message with a Priority1 ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMPriority1( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMPriority1* data = (MMPriority1*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/priority1.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Priority2 ManagementTLV message.
 * 
 * @param m     Management message with a Priority2 ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMPriority2( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMPriority2* data = (MMPriority2*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/priority2.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Domain ManagementTLV message.
 * 
 * @param m     Management message with a Domain ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMDomain( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMDomain* data = (MMDomain*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/domain.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Slave Only ManagementTLV message.
 * 
 * @param m     Management message with a Slave Only ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMSlaveOnly( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMSlaveOnly* data = (MMSlaveOnly*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/slaveOnly.def"

    /* return length */
    return offset;
}

/**
 * @brief Pack a Log Announce Interval ManagementTLV message.
 * 
 * @param m     Management message with a Log Announce Interval ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMLogAnnounceInterval( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMLogAnnounceInterval* data = (MMLogAnnounceInterval*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/logAnnounceInterval.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack an Announce Receipt Timeout ManagementTLV message.
 * 
 * @param m     Management message with an Announce Receipt Timeout ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMAnnounceReceiptTimeout( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMAnnounceReceiptTimeout* data = (MMAnnounceReceiptTimeout*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/announceReceiptTimeout.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Log Sync Interval ManagementTLV message.
 * 
 * @param m     Management message with a Log Sync Interval ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMLogSyncInterval( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMLogSyncInterval* data = (MMLogSyncInterval*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/logSyncInterval.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Version Number ManagementTLV message.
 * 
 * @param m     Management message with a Version Number ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMVersionNumber( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMVersionNumber* data = (MMVersionNumber*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/versionNumber.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Time ManagementTLV message.
 * 
 * @param m     Management message with a Time ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMTime( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMTime* data = (MMTime*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/time.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Clock Accuracy ManagementTLV message.
 * 
 * @param m     Management message with a Clock Accuracy ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMClockAccuracy( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMClockAccuracy* data = (MMClockAccuracy*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/clockAccuracy.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Utc Properties ManagementTLV message.
 * 
 * @param m     Management message with a Utc Properties ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMUtcProperties( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMUtcProperties* data = (MMUtcProperties*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/utcProperties.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Traceability Properties ManagementTLV message.
 * 
 * @param m     Management message with a Traceability Properties ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMTraceabilityProperties( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMTraceabilityProperties* data = (MMTraceabilityProperties*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/traceabilityProperties.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Delay Mechanism ManagementTLV message.
 * 
 * @param m     Management message with a Delay Mechanism ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMDelayMechanism( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMDelayMechanism* data = (MMDelayMechanism*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/delayMechanism.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack a Log Min Pdelay Req Interval ManagementTLV message.
 * 
 * @param m     Management message with a Log Min Pdelay Req Interval ManagementTLV to pack.
 * @param buf   Buffer with an outgoing message.
 */
UInteger16 OutgoingManagementMessage::packMMLogMinPdelayReqInterval( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMLogMinPdelayReqInterval* data = (MMLogMinPdelayReqInterval*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        pack##type( &data->name, buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/logMinPdelayReqInterval.def"

    /* return length*/
    return offset;
}

/**
 * @brief Pack message header.
 * 
 * @param header        Message header to pack. 
 * @param buf           Buffer with a message to send.     
 */
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

/**
 * @brief Pack management message.
 * 
 * @param m     Management message to pack.
 * @param buf   Buffer with a message to send.
 */
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

/**
 * @brief Pack management TLV.
 * 
 * @param tlv   Management TLV to pack.
 * @param buf   Buffer with a message to send.
 */
void OutgoingManagementMessage::packManagementTLV(ManagementTLV *tlv, Octet *buf)
{
    int offset = 0;
    #define OPERATE( name, size, type ) \
        pack##type( &tlv->name, buf + MANAGEMENT_LENGTH + offset ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/managementTLV.def"
}

/**
 * @brief Initialize management message packing.
 * 
 * @param buf           Buffer with a message to send.
 * @param outgoing      Management message to pack.
 */
void OutgoingManagementMessage::msgPackManagement(Octet *buf, MsgManagement *outgoing)
{
    DBG("packing management message \n");
    packMsgManagement(outgoing, buf);
}

/**
 * @brief Pack Management message into OUT buffer.
 * 
 * @param optBuf        Buffer with application options.
 * @param buf           Buffer to which the message should be packed.
 * @param outgoing      Outgoing management message to be packed.
 */
void OutgoingManagementMessage::msgPackManagementTLV(OptBuffer* optBuf, Octet *buf, MsgManagement *outgoing)
{
    DBG("packing ManagementTLV message \n");

    UInteger16 lengthField = 0;

    switch(outgoing->tlv->managementId)
    {
	case MM_NULL_MANAGEMENT:
	case MM_SAVE_IN_NON_VOLATILE_STORAGE:
	case MM_RESET_NON_VOLATILE_STORAGE:
	case MM_ENABLE_PORT:
	case MM_DISABLE_PORT:
        case MM_CLOCK_DESCRIPTION:
        case MM_DEFAULT_DATA_SET:
        case MM_CURRENT_DATA_SET:
        case MM_PARENT_DATA_SET:
        case MM_TIME_PROPERTIES_DATA_SET:
        case MM_PORT_DATA_SET:
        case MM_TIME:
            lengthField = 0;
            break;
        case MM_USER_DESCRIPTION:
            lengthField = (optBuf->action_type == SET) ? packMMUserDescription(outgoing, buf) : 0;
            break;
        case MM_INITIALIZE:
            lengthField = (optBuf->action_type == COMMAND) ? packMMInitialize(outgoing, buf) : 0;
            break;
        case MM_PRIORITY1:
            lengthField = (optBuf->action_type == SET) ? packMMPriority1(outgoing, buf) : 0;
            break;
        case MM_PRIORITY2:
            lengthField = (optBuf->action_type == SET) ? packMMPriority2(outgoing, buf) : 0;
            break;
        case MM_DOMAIN:
            lengthField = (optBuf->action_type == SET) ? packMMDomain(outgoing, buf) : 0;
            break;
	case MM_SLAVE_ONLY:
            lengthField = (optBuf->action_type == SET) ? packMMSlaveOnly(outgoing, buf) : 0;
            break;
        case MM_LOG_ANNOUNCE_INTERVAL:
            lengthField = (optBuf->action_type == SET) ? packMMLogAnnounceInterval(outgoing, buf) : 0;
            break;
        case MM_ANNOUNCE_RECEIPT_TIMEOUT:
            lengthField = (optBuf->action_type == SET) ? packMMAnnounceReceiptTimeout(outgoing, buf) : 0;
            break;
        case MM_LOG_SYNC_INTERVAL:
            lengthField = (optBuf->action_type == SET) ? packMMLogSyncInterval(outgoing, buf) : 0;
            break;
        case MM_VERSION_NUMBER:
            lengthField = (optBuf->action_type == SET) ? packMMVersionNumber(outgoing, buf) : 0;
            break;
        case MM_CLOCK_ACCURACY:
            lengthField = (optBuf->action_type == SET) ? packMMClockAccuracy(outgoing, buf) : 0;
            break;
        case MM_UTC_PROPERTIES:
            lengthField = (optBuf->action_type == SET) ? packMMUtcProperties(outgoing, buf) : 0;
            break;
        case MM_TRACEABILITY_PROPERTIES:
            lengthField = (optBuf->action_type == SET) ? packMMTraceabilityProperties(outgoing, buf) : 0;
            break;
        case MM_DELAY_MECHANISM:
            lengthField = (optBuf->action_type == SET) ? packMMDelayMechanism(outgoing, buf) : 0;
            break;
        case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
            lengthField = (optBuf->action_type == SET) ? packMMLogMinPdelayReqInterval(outgoing, buf) : 0;
            break;
	default:
            DBG("packing management msg: unsupported id \n");
    }

    /* set lengthField */
    outgoing->tlv->lengthField = lengthField;

    packManagementTLV((ManagementTLV*)outgoing->tlv, buf);
}

/**
 * @brief Init clockIdentity with MAC address and 0xFF and 0xFE.
 * 
 * @param clockIdentity Clock Identity to initialize.
 * @param hwAddr        MAC address.
 */
void OutgoingManagementMessage::initClockIdentity(ClockIdentity clockIdentity, Octet* hwAddr){
   /*
    * init clockIdentity with MAC address and 0xFF and 0xFE. see
    * spec 7.5.2.2.2
    */
    int j = 0;
    for (int i=0; i<CLOCK_IDENTITY_LENGTH; i++)
    {
        if (i==3) clockIdentity[i] = 0xFF;
        else if (i==4) clockIdentity[i] = 0xFE;
        else
        {
            clockIdentity[i] = hwAddr[j];
            j++;
        }
    }
}

/**
 * @brief Initialize outgoing management message fields.
 * 
 * @param outgoing      Outgoing management message to initialize.
 */
void OutgoingManagementMessage::initOutgoingMsgManagement(MsgManagement* outgoing)
{ 
    /* set header fields */
    outgoing->header.transportSpecific = 0x0;
    outgoing->header.messageType = MANAGEMENT;
    outgoing->header.versionPTP = VERSION_PTP;
    outgoing->header.domainNumber = DFLT_DOMAIN_NUMBER;
    /* set header flagField to zero for management messages, Spec 13.3.2.6 */
    outgoing->header.flagField0 = 0x00;
    outgoing->header.flagField1 = 0x00;
    outgoing->header.correctionField.msb = 0;
    outgoing->header.correctionField.lsb = 0;
    
    /* set default sourcePortIdentity, avoid uninitialized value */
    memset(&(outgoing->header.sourcePortIdentity), 0, sizeof(PortIdentity));
    
    outgoing->header.sequenceId = 0;
    
    outgoing->header.controlField = 0x0; /* deprecrated for ptp version 2 */
    outgoing->header.logMessageInterval = 0x7F;

    /* set management message fields */
    /* set default targetPortIdentity, avoid uninitialized value */
    memset(&(outgoing->targetPortIdentity), -1, sizeof(PortIdentity));
    
    outgoing->startingBoundaryHops = 0;
    
    outgoing->boundaryHops = outgoing->startingBoundaryHops;
    outgoing->actionField = 0; /* set default action, avoid uninitialized value */

    /* init managementTLV */
    XMALLOC(outgoing->tlv, ManagementTLV*, sizeof(ManagementTLV));
    outgoing->tlv->dataField = NULL;
}

/**
 * @brief Handle management message.
 * 
 * @param optBuf        Buffer with application options.
 * @param buf           Buffer with a message to send.
 * @param outgoing      Outgoing management message to handle.
 */
void OutgoingManagementMessage::handleManagement(OptBuffer* optBuf, Octet* buf, MsgManagement* outgoing)
{
    switch(optBuf->mgmt_id)
    {
        case MM_NULL_MANAGEMENT:
            DBG("handleManagement: Null Management\n");
            handleMMNullManagement(outgoing, optBuf->action_type);
            break;
                
        case MM_CLOCK_DESCRIPTION:
            DBG("handleManagement: Clock Description\n");
            handleMMClockDescription(outgoing, optBuf->action_type);
            break;
                
        case MM_USER_DESCRIPTION:
            DBG("handleManagement: User Description\n");
            
            if ((optBuf->action_type == SET) && !optBuf->value_set) {
                ERROR("User Description value not defined\n");
                exit(1);
            }
            
            handleMMUserDescription(outgoing, optBuf->action_type, optBuf->value);
            break;
                
        case MM_SAVE_IN_NON_VOLATILE_STORAGE:
            DBG("handleManagement: Save In Non-Volatile Storage\n");
            handleMMSaveInNonVolatileStorage(outgoing, optBuf->action_type);
            break;
            
        case MM_RESET_NON_VOLATILE_STORAGE:
            DBG("handleManagement: Reset Non-Volatile Storage\n");
            handleMMResetNonVolatileStorage(outgoing, optBuf->action_type);
            break;
                
        case MM_INITIALIZE:
            DBG("handleManagement: Initialize\n");
            
            Enumeration16 initializeKey;
            if (optBuf->action_type == COMMAND) {
                if (!optBuf->value_set) {
                    ERROR("Initialize value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%u", &initializeKey);  
            }
            else
                initializeKey = 0; /* avoid uninitialized value */
            
            handleMMInitialize(outgoing, optBuf->action_type, initializeKey);
            break;
                
        case MM_DEFAULT_DATA_SET:
            DBG("handleManagement: Default Data Set\n");
            handleMMDefaultDataSet(outgoing, optBuf->action_type);
            break;         
                    
        case MM_CURRENT_DATA_SET:
            DBG("handleManagement: Current Data Set\n");
            handleMMCurrentDataSet(outgoing, optBuf->action_type);
            break;
                
        case MM_PARENT_DATA_SET:
            DBG("handleManagement: Parent Data Set\n");
            handleMMParentDataSet(outgoing, optBuf->action_type);
            break;
                
        case MM_TIME_PROPERTIES_DATA_SET:
            DBG("handleManagement: TimeProperties Data Set\n");
            handleMMTimePropertiesDataSet(outgoing, optBuf->action_type);
            break;
                
        case MM_PORT_DATA_SET:
            DBG("handleManagement: Port Data Set\n");
            handleMMPortDataSet(outgoing, optBuf->action_type);
            break;
                
        case MM_PRIORITY1:
            DBG("handleManagement: Priority1\n");
            
            UInteger8 priority1;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Priority1 value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%u", &priority1);  
            }
            else
                priority1 = 0; /* avoid uninitialized value */
            
            handleMMPriority1(outgoing, optBuf->action_type, priority1);
            break;
            
        case MM_PRIORITY2:
            DBG("handleManagement: Priority2\n");
            
            UInteger8 priority2;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Priority2 value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%u", &priority2);  
            }
            else
                priority2 = 0; /* avoid uninitialized value */
            
            handleMMPriority2(outgoing, optBuf->action_type, priority2);
            break;
            
        case MM_DOMAIN:
            DBG("handleManagement: Domain\n");
            
            UInteger8 domainNumber;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Domain value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%u", &domainNumber);  
            }
            else
                domainNumber = 0; /* avoid uninitialized value */
            
            handleMMDomain(outgoing, optBuf->action_type, domainNumber);
            break;
                
        case MM_SLAVE_ONLY:
            DBG("handleManagement: Slave Only\n");
            
            Boolean so;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Slave Only value not defined\n");
                    exit(1);
                }
                else {
                    if ((!strcmp("TRUE", optBuf->value.textField)) ||
                            (!strcmp("True", optBuf->value.textField)) ||
                            (!strcmp("true", optBuf->value.textField)) ||
                            (!strcmp("1", optBuf->value.textField))) {
                        so = TRUE;
                    }
                    else if ((!strcmp("FALSE", optBuf->value.textField)) ||
                            (!strcmp("False", optBuf->value.textField)) ||
                            (!strcmp("false", optBuf->value.textField)) ||
                            (!strcmp("0", optBuf->value.textField))) {
                        so = FALSE;
                    }
                    else {
                        ERROR("Wrong value\n");
                        exit(1);
                    }
                }
            }
            else
                so = FALSE; /* avoid uninitialized value */
            
            handleMMSlaveOnly(outgoing, optBuf->action_type, so);
            break;
            
        case MM_LOG_ANNOUNCE_INTERVAL:
            DBG("handleManagement: Log Announce Interval\n");
            
            Integer8 logAnnounceInterval;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Log Announce Interval value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%d", &logAnnounceInterval);  
            }
            else
                logAnnounceInterval = 0; /* avoid uninitialized value */
            
            handleMMLogAnnounceInterval(outgoing, optBuf->action_type, logAnnounceInterval);
            break;
            
        case MM_ANNOUNCE_RECEIPT_TIMEOUT:
            DBG("handleManagement: Announce Receipt Timeout\n");
            
            UInteger8 announceReceiptTimeout;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Announce Receipt Timeout value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%u", &announceReceiptTimeout);  
            }
            else
                announceReceiptTimeout = 0; /* avoid uninitialized value */
            
            handleMMAnnounceReceiptTimeout(outgoing, optBuf->action_type, announceReceiptTimeout);
            break;
            
        case MM_LOG_SYNC_INTERVAL:
            DBG("handleManagement: Log Sync Interval\n");
            
            Integer8 logSyncInterval;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Log Sync Interval value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%d", &logSyncInterval);  
            }
            else
                logSyncInterval = 0; /* avoid uninitialized value */
            
            handleMMLogSyncInterval(outgoing, optBuf->action_type, logSyncInterval);
            break;
            
        case MM_VERSION_NUMBER:
            DBG("handleManagement: Version Number\n");
            
            UInteger4Lower versionNumber;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Version Number value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%x", &versionNumber);  
            }
            else
                versionNumber = 0; /* avoid uninitialized value */
            
            handleMMVersionNumber(outgoing, optBuf->action_type, versionNumber);
            break;
            
        case MM_ENABLE_PORT:
            DBG("handleManagement: Enable Port\n");
            handleMMEnablePort(outgoing, optBuf->action_type);
            break;
                
        case MM_DISABLE_PORT:
            DBG("handleManagement: Disable Port\n");
            handleMMDisablePort(outgoing, optBuf->action_type);
            break;
                
        case MM_TIME:
            DBG("handleManagement: Time\n");
            handleMMTime(outgoing, optBuf->action_type);
            break;
                
        case MM_CLOCK_ACCURACY:
            DBG("handleManagement: Clock Accuracy\n");
            
            Enumeration8 clockAccuracy;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Clock Accuracy value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%x", &clockAccuracy);  
            }
            else
                clockAccuracy = 0; /* avoid uninitialized value */
            
            handleMMClockAccuracy(outgoing, optBuf->action_type, clockAccuracy);
            break;

        case MM_UTC_PROPERTIES:
            DBG("handleManagement: Utc Properties\n");
            handleMMUtcProperties(outgoing, optBuf->action_type);
            break;
                
        case MM_TRACEABILITY_PROPERTIES:
            DBG("handleManagement: Traceability Properties\n");
            handleMMTraceabilityProperties(outgoing, optBuf->action_type);
            break;
                
        case MM_DELAY_MECHANISM:
            DBG("handleManagement: Delay Mechanism\n");
            
            Enumeration8 delayMechanism;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Delay Mechanism value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%x", &delayMechanism);  
            }
            else
                delayMechanism = 0; /* avoid uninitialized value */
            
            handleMMDelayMechanism(outgoing, optBuf->action_type, delayMechanism);
            break;
                
        case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
            DBG("handleManagement: Log Min Pdelay Req Interval\n");
            
            Integer8 logMinPdelayReqInterval;
            if (optBuf->action_type == SET) {
                if (!optBuf->value_set) {
                    ERROR("Log Min Pdelay Req Interval value not defined\n");
                    exit(1);
                }
                else
                    sscanf(optBuf->value.textField, "%d", &logMinPdelayReqInterval);  
            }
            else
                logMinPdelayReqInterval = 0; /* avoid uninitialized value */
            
            handleMMLogMinPdelayReqInterval(outgoing, optBuf->action_type, logMinPdelayReqInterval);
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
            printf("handleManagement: Currently unsupported managementTLV %d\n", optBuf->mgmt_id);
            exit(1);
        
        default:
            printf("handleManagement: Unknown managementTLV %d\n", optBuf->mgmt_id);
            exit(1);
    }
    
    /* pack ManagementTLV */
    msgPackManagementTLV(optBuf, buf, outgoing);
    
    /* set domain number */
    if (optBuf->domainNumber != DFLT_DOMAIN_NUMBER)
        outgoing->header.domainNumber = optBuf->domainNumber;
    
    /* set sequenceId */
    outgoing->header.sequenceId = optBuf->sequenceId;
    
    /* set SourcePortIdentity*/
    initClockIdentity(optBuf->sourcePortIdentity.clockIdentity, optBuf->hw_address);
    memcpy(outgoing->header.sourcePortIdentity.clockIdentity, optBuf->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH);
    outgoing->header.sourcePortIdentity.portNumber = optBuf->sourcePortIdentity.portNumber;
    
    /* set TargetPortIdentity*/
    if (optBuf->hw_address_server)
        initClockIdentity(outgoing->targetPortIdentity.clockIdentity, optBuf->hw_address_server);
    if (optBuf->portNumber != 0)
        outgoing->targetPortIdentity.portNumber = optBuf->portNumber;

    /* set header messageLength, the outgoing->tlv->lengthField is now valid */
    outgoing->header.messageLength = MANAGEMENT_LENGTH + TLV_LENGTH + outgoing->tlv->lengthField;

    msgPackManagement(buf, outgoing);
}

/**
 * @brief Handle outgoing NULL_MANAGEMENT message.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMNullManagement(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling NULL_MANAGEMENT message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_NULL_MANAGEMENT;

    switch(actionField)
    {
        case GET:
        case SET:
            DBG("GET or SET mgmt msg\n");
            break;
        case COMMAND:
            DBG("COMMAND mgmt msg\n");
            break;
        default:
            ERROR(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing CLOCK_DESCRIPTION management message.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMClockDescription(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling CLOCK_DESCRIPTION message \n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_CLOCK_DESCRIPTION;
    
    outgoing->actionField = actionField;

    switch(actionField)
    {
        case GET:
            DBG(" GET action \n");
            break;

        default:
            ERROR(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing USER_DESCRIPTION management message type.
 * 
 * @param outgoing              Outgoing management message to handle.
 * @param actionField           Management message action type.
 * @param userDescription       User Description to be set.   
 */
void OutgoingManagementMessage::handleMMUserDescription(MsgManagement* outgoing, Enumeration4 actionField, PTPText userDescription)
{
    DBG("handling USER_DESCRIPTION message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_USER_DESCRIPTION;
    
    outgoing->actionField = actionField;

    MMUserDescription* data = NULL;
    UInteger8 userDescriptionLength;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action \n");
            
            userDescriptionLength = userDescription.lengthField;
            
            if(userDescriptionLength <= USER_DESCRIPTION_MAX) {
                XMALLOC(data, MMUserDescription*, sizeof(MMUserDescription));
                XCALLOC(data->userDescription.textField, Octet*, userDescriptionLength + 1, sizeof(Octet));
                
                memcpy(data->userDescription.textField, userDescription.textField, userDescriptionLength);
                data->userDescription.textField[userDescription.lengthField] = '\0';
                
                data->userDescription.lengthField = userDescriptionLength;
            } else {
                ERROR("management user description exceeds specification length \n");
                exit(1);
            }

            outgoing->tlv->dataField = (Octet*) data;          
            break;
            
        case GET:
            DBG(" GET action \n");
            break;
            
        default:
            ERROR(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming SAVE_IN_NON_VOLATILE_STORAGE management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMSaveInNonVolatileStorage(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling SAVE_IN_NON_VOLATILE_STORAGE message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_SAVE_IN_NON_VOLATILE_STORAGE;

    switch(actionField)
    {
        case COMMAND:
            printf("management message not supported \n");
            exit(1);
            
        default:
            ERROR(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming RESET_NON_VOLATILE_STORAGE management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMResetNonVolatileStorage(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling RESET_NON_VOLATILE_STORAGE message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_RESET_NON_VOLATILE_STORAGE;

    switch(actionField)
    {
        case COMMAND:
            printf("management message not supported \n");
            exit(1);
            
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle incoming INITIALIZE management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 * @param initializeKey Initialize Key to be used.
 */
void OutgoingManagementMessage::handleMMInitialize(MsgManagement* outgoing, Enumeration4 actionField, Enumeration16 initializeKey)
{
    DBG("handling INITIALIZE message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_INITIALIZE;
    
    outgoing->actionField = actionField;

    MMInitialize* data = NULL;

    switch( actionField )
    {
        case COMMAND:
            DBG(" COMMAND action\n");
            XMALLOC(data, MMInitialize*, sizeof(MMInitialize));
            
            /* Table 45 - INITIALIZATION_KEY enumeration */
            switch( initializeKey )
            {
                case INITIALIZE_EVENT:
                    /* cause INITIALIZE event */
                    break;
                default:
                    /* do nothing, implementation specific */
                    DBG("initializeKey != 0, will do nothing\n");
            }

            data->initializeKey = initializeKey;
            
            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing DEFAULT_DATA_SET management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMDefaultDataSet(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling DEFAULT_DATA_SET message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_DEFAULT_DATA_SET;
    
    outgoing->actionField = actionField;

    switch(actionField)
    {
        case GET:
            DBG(" GET action\n");
            break;

        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing CURRENT_DATA_SET management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMCurrentDataSet(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling CURRENT_DATA_SET message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_CURRENT_DATA_SET;
    
    outgoing->actionField = actionField;

    switch(actionField)
    {
        case GET:
            DBG(" GET action\n");
            break;

        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing PARENT_DATA_SET management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMParentDataSet(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling PARENT_DATA_SET message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_PARENT_DATA_SET;
    
    outgoing->actionField = actionField;

    switch(actionField)
    {
        case GET:
            DBG(" GET action\n");
            break;

        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing PROPERTIES_DATA_SET management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMTimePropertiesDataSet(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling TIME_PROPERTIES message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_TIME_PROPERTIES_DATA_SET;

    outgoing->actionField = actionField;

    switch(actionField)
    {
        case GET:
            DBG(" GET action\n");
            break;

        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing PORT_DATA_SET management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMPortDataSet(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling PORT_DATA_SET message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_PORT_DATA_SET;

    outgoing->actionField = actionField;

    switch(actionField)
    {
        case GET:
            DBG(" GET action\n");
            break;

        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing PRIORITY1 management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 * @param priority1     Priority1 to be set.
 */
void OutgoingManagementMessage::handleMMPriority1(MsgManagement* outgoing, Enumeration4 actionField, UInteger8 priority1)
{
    DBG("handling PRIORITY1 message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_PRIORITY1;
    
    outgoing->actionField = actionField;

    MMPriority1* data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMPriority1*, sizeof(MMPriority1));

            data->priority1 = priority1;

            outgoing->tlv->dataField = (Octet*) data;
            break;
                
        case GET:
            DBG(" GET action\n");
            break;
                
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing PRIORITY2 management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 * @param priority2     Priority2 to be set.
 */
void OutgoingManagementMessage::handleMMPriority2(MsgManagement* outgoing, Enumeration4 actionField, UInteger8 priority2)
{
    DBG("handling PRIORITY2 message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_PRIORITY2;
    
    outgoing->actionField = actionField;

    MMPriority2* data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMPriority2*, sizeof(MMPriority2));

            data->priority2 = priority2;

            outgoing->tlv->dataField = (Octet*) data;
            break;
                
        case GET:
            DBG(" GET action\n");
            break;
                
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing DOMAIN management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 * @param domainNumber  Domain Number to be set.
 */
void OutgoingManagementMessage::handleMMDomain(MsgManagement* outgoing, Enumeration4 actionField, UInteger8 domainNumber)
{
    DBG("handling DOMAIN message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_DOMAIN;
    
    outgoing->actionField = actionField;

    MMDomain* data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMDomain*, sizeof(MMDomain));

            data->domainNumber = domainNumber;

            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
                
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing SLAVE_ONLY management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 * @param so            Slave Only property to be set.
 */
void OutgoingManagementMessage::handleMMSlaveOnly(MsgManagement* outgoing, Enumeration4 actionField, Boolean so)
{
    DBG("handling SLAVE_ONLY management message \n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_SLAVE_ONLY;
    
    outgoing->actionField = actionField;

    MMSlaveOnly* data = NULL;
    
    switch (actionField)
    {
        case SET:
            DBG(" SET action \n");
            XMALLOC(data, MMSlaveOnly*, sizeof(MMSlaveOnly));
            
            data->so = so;

            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
                
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing LOG_ANNOUNCE_INTERVAL management message type.
 * 
 * @param outgoing              Outgoing management message to handle.
 * @param actionField           Management message action type.
 * @param logAnnounceInterval   Log Announce Interval to be set.
 */
void OutgoingManagementMessage::handleMMLogAnnounceInterval(MsgManagement* outgoing, Enumeration4 actionField, Integer8 logAnnounceInterval)
{
    DBG("handling LOG_ANNOUNCE_INTERVAL message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_LOG_ANNOUNCE_INTERVAL;
    
    outgoing->actionField = actionField;

    MMLogAnnounceInterval* data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMLogAnnounceInterval*, sizeof(MMLogAnnounceInterval));
            
            data->logAnnounceInterval = logAnnounceInterval;

            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
                
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing ANNOUNCE_RECEIPT_TIMEOUT management message type.
 * 
 * @param outgoing                      Outgoing management message to handle.
 * @param actionField                   Management message action type.
 * @param announceReceiptTimeout        Announce Receipt Timeout to be set.
 */
void OutgoingManagementMessage::handleMMAnnounceReceiptTimeout(MsgManagement* outgoing, Enumeration4 actionField, UInteger8 announceReceiptTimeout)
{
    DBG("handling ANNOUNCE_RECEIPT_TIMEOUT message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_ANNOUNCE_RECEIPT_TIMEOUT;
    
    outgoing->actionField = actionField;

    MMAnnounceReceiptTimeout* data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMAnnounceReceiptTimeout*, sizeof(MMAnnounceReceiptTimeout));
            
            data->announceReceiptTimeout = announceReceiptTimeout;

            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
                
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing LOG_SYNC_INTERVAL management message type.
 * 
 * @param outgoing              Outgoing management message to handle.
 * @param actionField           Management message action type.
 * @param logSyncInterval       Log Sync Interval to be set.
 */
void OutgoingManagementMessage::handleMMLogSyncInterval(MsgManagement* outgoing, Enumeration4 actionField, Integer8 logSyncInterval)
{
    DBG("handling LOG_SYNC_INTERVAL message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_LOG_SYNC_INTERVAL;

    outgoing->actionField = actionField;

    MMLogSyncInterval* data = NULL;

    switch(actionField)
    {
	case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMLogSyncInterval*, sizeof(MMLogSyncInterval));
            
            data->logSyncInterval = logSyncInterval;

            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
                
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }

}

/**
 * @brief Handle outgoing VERSION_NUMBER management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 * @param versionNumber Version Number to be set.
 */
void OutgoingManagementMessage::handleMMVersionNumber(MsgManagement* outgoing, Enumeration4 actionField, UInteger4Lower versionNumber)
{
    DBG("handling VERSION_NUMBER message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_VERSION_NUMBER;
    
    outgoing->actionField = actionField;

    MMVersionNumber* data = NULL;
    
    switch(actionField)
    {
	case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMVersionNumber*, sizeof(MMVersionNumber));
            
            data->versionNumber = versionNumber;

            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
                
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }

}

/**
 * @brief Handle outgoing ENABLE_PORT management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMEnablePort(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling ENABLE_PORT message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_ENABLE_PORT;
    
    outgoing->actionField = actionField;

    switch(actionField)
    {
        case COMMAND:
            DBG(" COMMAND action\n");
            break;
                
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing DISABLE_PORT management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMDisablePort(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling DISABLE_PORT message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_DISABLE_PORT;

    outgoing->actionField = actionField;

    switch(actionField)
    {
        case COMMAND:
            DBG(" COMMAND action\n");
            break;
                
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing TIME management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMTime(MsgManagement* outgoing, Enumeration4 actionField/*, Timestamp currentTime*/)
{
    DBG("handling TIME message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_TIME;
    
    outgoing->actionField = actionField;
/* commented out to suppress unused variable compiler warning */
//    MMTime* data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
    /* commented out to suppress unused variable compiler warning */
//            XMALLOC(data, MMTime*, sizeof(MMTime));
//            
//            data->currentTime = currentTime;
//
//            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
            
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing CLOCK_ACCURACY management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 * @param clockAccuracy Clock Accuracy to be set.
 */
void OutgoingManagementMessage::handleMMClockAccuracy(MsgManagement* outgoing, Enumeration4 actionField, Enumeration8 clockAccuracy)
{
    DBG("handling CLOCK_ACCURACY message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_CLOCK_ACCURACY;
    
    outgoing->actionField = actionField;

    MMClockAccuracy* data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMClockAccuracy*, sizeof(MMClockAccuracy));
            
            data->clockAccuracy = clockAccuracy;

            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
            
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing UTC_PROPERTIES management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMUtcProperties(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling UTC_PROPERTIES message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_UTC_PROPERTIES;
    
    outgoing->actionField = actionField;

    MMUtcProperties* data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMUtcProperties*, sizeof(MMUtcProperties));
            
            Integer16 currentUtcOffset;
            bool utcv, li59, li61;
            
            printf("Values to set: \n");
            printf(" currentUtcOffset: ");
            scanf("%hd", &currentUtcOffset);
            printf(" currentUtcOffsetValid: ");
            scanf("%hhu", &utcv);
            printf(" leap59: ");
            scanf("%hhu", &li59);
            printf(" leap61: ");
            scanf("%hhu", &li61);
            printf("\n");

            data->currentUtcOffset = currentUtcOffset;
            data->utcv_li59_li61 = 0;
            
            if (utcv)
                SET_BIT(data->utcv_li59_li61, UTCV);
            if (li59)
                SET_BIT(data->utcv_li59_li61, LI59);
            if(li61)
                SET_BIT(data->utcv_li59_li61, LI61);
            
            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
            
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing TRACEABILITY_PROPERTIES management message type.
 * 
 * @param outgoing      Outgoing management message to handle.
 * @param actionField   Management message action type.
 */
void OutgoingManagementMessage::handleMMTraceabilityProperties(MsgManagement* outgoing, Enumeration4 actionField)
{
    DBG("handling TRACEABILITY_PROPERTIES message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_TRACEABILITY_PROPERTIES;
    
    outgoing->actionField = actionField;

    MMTraceabilityProperties* data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMTraceabilityProperties*, sizeof(MMTraceabilityProperties));
            
            UInteger8 ftra, ttra;
            
            printf("Values to set: \n");
            printf(" frequencyTraceable: ");
            scanf("%hhu", &ftra);
            printf(" timeTraceable: ");
            scanf("%hhu", &ttra);
            printf("\n");

            data->ftra_ttra = 0;
            
            if (ftra)
                SET_BIT(data->ftra_ttra, FTRA);
            if (ttra)
                SET_BIT(data->ftra_ttra, TTRA);

            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
            
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing DELAY_MECHANISM management message type.
 * 
 * @param outgoing              Outgoing management message to handle.
 * @param actionField           Management message action type.
 * @param delayMechanism        Delay Mechanism to be set.
 */
void OutgoingManagementMessage::handleMMDelayMechanism(MsgManagement* outgoing, Enumeration4 actionField, Enumeration8 delayMechanism)
{
    DBG("handling DELAY_MECHANISM message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_DELAY_MECHANISM;
    
    outgoing->actionField = actionField;

    MMDelayMechanism *data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMDelayMechanism*, sizeof(MMDelayMechanism));
            
            data->delayMechanism = delayMechanism;

            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
            
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing LOG_MIN_PDELAY_REQ_INTERVAL management message type.
 * 
 * @param outgoing                      Outgoing management message to handle.
 * @param actionField                   Management message action type.
 * @param logMinPdelayReqInterval       Log Min Pdelay Req Interval to be set.
 */
void OutgoingManagementMessage::handleMMLogMinPdelayReqInterval(MsgManagement* outgoing, Enumeration4 actionField, Integer8 logMinPdelayReqInterval)
{
    DBG("handling LOG_MIN_PDELAY_REQ_INTERVAL message\n");

    initOutgoingMsgManagement(outgoing);
    outgoing->tlv->tlvType = TLV_MANAGEMENT;
    outgoing->tlv->lengthField = 2;
    outgoing->tlv->managementId = MM_LOG_MIN_PDELAY_REQ_INTERVAL;
    
    outgoing->actionField = actionField;

    MMLogMinPdelayReqInterval* data = NULL;
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action\n");
            XMALLOC(data, MMLogMinPdelayReqInterval*, sizeof(MMLogMinPdelayReqInterval));
            
            data->logMinPdelayReqInterval = logMinPdelayReqInterval;

            outgoing->tlv->dataField = (Octet*) data;
            break;
            
        case GET:
            DBG(" GET action\n");
            break;
            
        default:
            DBG(" unknown actionType \n");
            exit(1);
    }
}

