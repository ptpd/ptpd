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
    
//    handleManagement(buf, this->incoming);
}

/**
 * @brief IncomingManagementMessage deconstructor.
 * 
 * The deconstructor frees memory.
 */
IncomingManagementMessage::~IncomingManagementMessage() {
    freeManagementTLV(this->incoming);
    free(this->incoming);
}

/**
 * @brief Unpack an Integer48 type.
 * 
 * @param buf   Buffer with a received message.
 * @param i     Integer48 object to unpack.
 */
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
 * @brief Unpack a ClockIdentity Structure.
 * 
 * @param buf   Buffer with a received message.
 * @param c     ClockIdentity Structure to unpack.
 */
void IncomingManagementMessage::unpackClockIdentity( Octet *buf, ClockIdentity *c)
{
    int i;
    for(i = 0; i < CLOCK_IDENTITY_LENGTH; i++) {
            unpackOctet((buf+i),&((*c)[i]));
    }
}

/**
 * @brief Unpack a ClockQuality Structure.
 * 
 * @param buf   Buffer with a received message.
 * @param c     ClockQuality Structure to unpack.
 */
void IncomingManagementMessage::unpackClockQuality( Octet *buf, ClockQuality *c)
{
    int offset = 0;
    ClockQuality* data = c;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name); \
            offset = offset + size;
    #include "../../src/def/derivedData/clockQuality.def"
}

/**
 * @brief Unpack a TimeInterval Structure.
 * 
 * @param buf   Buffer with a received message.
 * @param t     TimeInterval Structure to unpack.
 */
void IncomingManagementMessage::unpackTimeInterval( Octet *buf, TimeInterval *t)
{
    int offset = 0;
    TimeInterval* data = t;
    #define OPERATE( name, size, type) \
            unpack##type (buf + offset, &data->name); \
            offset = offset + size;
    #include "../../src/def/derivedData/timeInterval.def"
}

/**
 * @brief Unpack a Timestamp Structure.
 * 
 * @param buf   Buffer with a received message.
 * @param t     Timestamp Structure to unpack.
 */
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

/**
 * @brief Unpack a PortAddress Structure.
 * 
 * @param buf   Buffer with a received message.
 * @param p     PortAddress Structure to unpack.
 */
void IncomingManagementMessage::unpackPortAddress( Octet *buf, PortAddress *p)
{
    unpackEnumeration16( buf, &p->networkProtocol);
    unpackUInteger16( buf+2, &p->addressLength);
    if(p->addressLength) {
        XMALLOC(p->addressField, Octet*, p->addressLength);
        memcpy( p->addressField, buf+4, p->addressLength);
    } else {
        p->addressField = NULL;
    }
}

/**
 * @brief Unpack a PTPText Structure.
 * 
 * @param buf   Buffer with a received message.
 * @param p     PTPText Structure to unpack.
 */
void IncomingManagementMessage::unpackPTPText( Octet *buf, PTPText *s)
{
    unpackUInteger8(buf, &s->lengthField);
    if(s->lengthField) {
        XMALLOC(s->textField, Octet*, s->lengthField);
        memcpy( s->textField, buf+1, s->lengthField);
    } else {
        s->textField = NULL;
    }
}

/**
 * @brief Unpack a PhysicalAddress Structure.
 * 
 * @param buf   Buffer with a received message.
 * @param p     PhysicalAddress Structure to unpack.
 */
void IncomingManagementMessage::unpackPhysicalAddress( Octet *buf, PhysicalAddress *p)
{
    unpackUInteger16( buf, &p->addressLength);
    if(p->addressLength) {
        XMALLOC(p->addressField, Octet*, p->addressLength);
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

//    msgHeader_display(data);
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

//    msgManagement_display(data);
}

/**
 * @brief Unpack management TLV.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a TLV to unpack.
 */
void IncomingManagementMessage::unpackManagementTLV(Octet *buf, MsgManagement *m)
{
    int offset = 0;
    XMALLOC(m->tlv, ManagementTLV*, sizeof(ManagementTLV));
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

/**
 * @brief Unpack a Clock Description ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Clock Description ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMClockDescription( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMClockDescription));
    MMClockDescription* data = (MMClockDescription*)m->tlv->dataField;
    memset(data, 0, sizeof(MMClockDescription));
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/clockDescription.def"

    mMClockDescription_display(data);
}

/**
 * @brief Unpack a User Description ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a User Description ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMUserDescription(Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMUserDescription));
    MMUserDescription* data = (MMUserDescription*)m->tlv->dataField;
    memset(data, 0, sizeof(MMUserDescription));
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/userDescription.def"

    mMUserDescription_display(data);
}

/**
 * @brief Unpack an Initialize ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with an Initialize ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMInitialize( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMInitialize));
    MMInitialize* data = (MMInitialize*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/initialize.def"

    mMInitialize_display(data);
}

/**
 * @brief Unpack a Default Data Set ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Default Data Set ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMDefaultDataSet( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMDefaultDataSet));
    MMDefaultDataSet* data = (MMDefaultDataSet*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/defaultDataSet.def"

    mMDefaultDataSet_display(data);
}

/**
 * @brief Unpack a Current Data Set ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Current Data Set ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMCurrentDataSet( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMCurrentDataSet));
    MMCurrentDataSet* data = (MMCurrentDataSet*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/currentDataSet.def"

    mMCurrentDataSet_display(data);
}

/**
 * @brief Unpack a Parent Data Set ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Parent Data Set ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMParentDataSet( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMParentDataSet));
    MMParentDataSet* data = (MMParentDataSet*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/parentDataSet.def"

    mMParentDataSet_display(data);
}

/**
 * @brief Unpack a Time Properties Data Set ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Time Properties Data Set ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMTimePropertiesDataSet( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMTimePropertiesDataSet));
    MMTimePropertiesDataSet* data = (MMTimePropertiesDataSet*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/timePropertiesDataSet.def"

    mMTimePropertiesDataSet_display(data);
}

/**
 * @brief Unpack a Port Data Set ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Port Data Set ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMPortDataSet( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMPortDataSet));
    MMPortDataSet* data = (MMPortDataSet*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/portDataSet.def"

    mMPortDataSet_display(data);
}

/**
 * @brief Unpack a Priority1 ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Priority1 ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMPriority1( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMPriority1));
    MMPriority1* data = (MMPriority1*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/priority1.def"

    mMPriority1_display(data);
}

/**
 * @brief Unpack a Priority2 ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Priority2 ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMPriority2( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMPriority2));
    MMPriority2* data = (MMPriority2*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/priority2.def"

    mMPriority2_display(data);
}

/**
 * @brief Unpack a Domain ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Domain ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMDomain( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMDomain));
    MMDomain* data = (MMDomain*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/domain.def"

    mMDomain_display(data);
}

/**
 * @brief Unpack a Slave Only ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Slave Only ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMSlaveOnly( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMSlaveOnly));
    MMSlaveOnly* data = (MMSlaveOnly*)m->tlv->dataField;
    /* see src/def/README for a note on this X-macro */
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/slaveOnly.def"

    mMSlaveOnly_display(data);
}

/**
 * @brief Unpack a Log Announce Interval ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Log Announce Interval ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMLogAnnounceInterval( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMLogAnnounceInterval));
    MMLogAnnounceInterval* data = (MMLogAnnounceInterval*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/logAnnounceInterval.def"

    mMLogAnnounceInterval_display(data);
}

/**
 * @brief Unpack an Announce Receipt Timeout ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with an Announce Receipt Timeout ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMAnnounceReceiptTimeout( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMAnnounceReceiptTimeout));
    MMAnnounceReceiptTimeout* data = (MMAnnounceReceiptTimeout*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/announceReceiptTimeout.def"

    mMAnnounceReceiptTimeout_display(data);
}

/**
 * @brief Unpack a Log Sync Interval ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Log Sync Interval ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMLogSyncInterval( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMLogSyncInterval));
    MMLogSyncInterval* data = (MMLogSyncInterval*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/logSyncInterval.def"

    mMLogSyncInterval_display(data);
}

/**
 * @brief Unpack a Version Number ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Version Number ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMVersionNumber( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMVersionNumber));
    MMVersionNumber* data = (MMVersionNumber*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/versionNumber.def"

    mMVersionNumber_display(data);
}

/**
 * @brief Unpack a Time ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Time ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMTime( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMTime));
    MMTime* data = (MMTime*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/time.def"

    mMTime_display(data);
}

/**
 * @brief Unpack a Clock Accuracy ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Clock Accuracy ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMClockAccuracy( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMClockAccuracy));
    MMClockAccuracy* data = (MMClockAccuracy*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/clockAccuracy.def"

    mMClockAccuracy_display(data);
}

/**
 * @brief Unpack a Utc Properties ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Utc Properties ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMUtcProperties( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMUtcProperties));
    MMUtcProperties* data = (MMUtcProperties*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/utcProperties.def"

    mMUtcProperties_display(data);
}

/**
 * @brief Unpack a Traceability Properties ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Traceability Properties ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMTraceabilityProperties( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMTraceabilityProperties));
    MMTraceabilityProperties* data = (MMTraceabilityProperties*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/traceabilityProperties.def"

    mMTraceabilityProperties_display(data);
}

/**
 * @brief Unpack a Delay Mechanism ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Delay Mechanism ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMDelayMechanism( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMDelayMechanism));
    MMDelayMechanism* data = (MMDelayMechanism*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/delayMechanism.def"

    mMDelayMechanism_display(data);
}

/**
 * @brief Unpack a Log Min Pdelay Req Interval ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with a Log Min Pdelay Req Interval ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMLogMinPdelayReqInterval( Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMLogMinPdelayReqInterval));
    MMLogMinPdelayReqInterval* data = (MMLogMinPdelayReqInterval*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset,\
                            &data->name ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/logMinPdelayReqInterval.def"

    mMLogMinPdelayReqInterval_display(data);
}

/**
 * @brief Unpack an Error Status ManagementTLV message.
 * 
 * @param buf   Buffer with a received message.
 * @param m     Management message with an Error Status ManagementTLV to unpack.
 */
void IncomingManagementMessage::unpackMMErrorStatus(Octet *buf, MsgManagement* m)
{
    int offset = 0;
    XMALLOC(m->tlv->dataField, Octet*, sizeof(MMErrorStatus));
    MMErrorStatus* data = (MMErrorStatus*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
        unpack##type( buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset, &data->name ); \
        offset = offset + size;
    #include "../../src/def/managementTLV/errorStatus.def"

    mMErrorStatus_display(data);
}

/**
 * @brief Handle incoming management message.
 * 
 * @param buf           Buffer with a received message.
 * @param incoming      Incoming management message.
 */
bool IncomingManagementMessage::handleManagement(OptBuffer* optBuf, Octet* buf, MsgManagement* incoming)
{
    msgUnpackManagement(buf, incoming);
    
    if (incoming->header.messageType != MANAGEMENT) {
        DBG("handleManagement: Ignore message other than management \n");
        return FALSE;
    }
    
    /*Spec 9.5.2.2*/	
    bool isFromSelf = ((incoming->header.sourcePortIdentity.portNumber == optBuf->sourcePortIdentity.portNumber) 
    && !memcmp(incoming->header.sourcePortIdentity.clockIdentity, optBuf->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH));
    
    if (isFromSelf) {
        DBG("handleManagement: Ignore message from self \n");
        return FALSE;
    }
    
    /* At this point we have a valid management message */
    DBG("Management message received : \n");
    msgHeader_display(&incoming->header);
    msgManagement_display(incoming);

    if(incoming->tlv == NULL) {
        DBG("handleManagement: TLV is empty\n");
        return TRUE;
    }

    /* is this an error status management TLV? */
    if(incoming->tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS) {
        DBG("handleManagement: Error Status TLV\n");
        unpackMMErrorStatus(buf, incoming);
        handleMMErrorStatus(incoming);
        return TRUE;
    } else if (incoming->tlv->tlvType != TLV_MANAGEMENT) {
        /* do nothing, implemention specific handling */
        DBG("handleManagement: Currently unsupported management TLV type\n");
        return TRUE;
    }
        
    switch(incoming->tlv->managementId)
    {
        case MM_NULL_MANAGEMENT:
            DBG("handleManagement: Null Management\n");
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
            return TRUE;
        
        default:
            printf("handleManagement: Unknown managementTLV %d\n", incoming->tlv->managementId);
            return TRUE;
    }
    
    return TRUE;
}

/**
 * @brief Handle incoming CLOCK_DESCRIPTION management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming USER_DESCRIPTION management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming SAVE_IN_NON_VOLATILE_STORAGE management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming RESET_NON_VOLATILE_STORAGE management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming INITIALIZE management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming DEFAULT_DATA_SET management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming CURRENT_DATA_SET management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming PARENT_DATA_SET management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming PROPERTIES_DATA_SET management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming PORT_DATA_SET management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming PRIORITY1 management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming PRIORITY2 management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming DOMAIN management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming SLAVE_ONLY management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming LOG_ANNOUNCE_INTERVAL management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming ANNOUNCE_RECEIPT_TIMEOUT management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming LOG_SYNC_INTERVAL management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming VERSION_NUMBER management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming ENABLE_PORT management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming DISABLE_PORT management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming TIME management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming CLOCK_ACCURACY management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming UTC_PROPERTIES management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming TRACEABILITY_PROPERTIES management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming DELAY_MECHANISM management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming LOG_MIN_PDELAY_REQ_INTERVAL management message type.
 * 
 * @param incoming      Incoming management message.
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
 * @brief Handle incoming ERROR_STATUS management message type.
 * 
 * @param incoming      Incoming management message.
 */
void IncomingManagementMessage::handleMMErrorStatus(MsgManagement *incoming)
{
	DBG("received MANAGEMENT_ERROR_STATUS message \n");
	/* implementation specific */
}