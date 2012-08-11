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
 * @file        IncomingManagementMessage.h
 * @author      Tomasz Kleinschmidt
 *
 * @brief       IncomingManagementMessage class definition.
 */

#ifndef INCOMINGMANAGEMENTMESSAGE_H
#define	INCOMINGMANAGEMENTMESSAGE_H

#include "OptBuffer.h"

#include "datatypes.h"

class IncomingManagementMessage {
public:
    IncomingManagementMessage(Octet* buf, OptBuffer* optBuf);
    virtual ~IncomingManagementMessage();
    
private:
    #define DECLARE_UNPACK( type ) void unpack##type( void*, void* );
    
    DECLARE_UNPACK( Boolean )
    DECLARE_UNPACK( Enumeration8 )
    DECLARE_UNPACK( Enumeration16 )
    DECLARE_UNPACK( Integer8 )
    DECLARE_UNPACK( UInteger8 )
    DECLARE_UNPACK( Integer16 )
    DECLARE_UNPACK( UInteger16 )
    DECLARE_UNPACK( Integer32 )
    DECLARE_UNPACK( UInteger32 )
    DECLARE_UNPACK( UInteger48 )
    DECLARE_UNPACK( Integer64 )
    DECLARE_UNPACK( Octet )
            
    #define DECLARE_UNPACK_LOWER_AND_UPPER( type ) \
        void unpack##type##Lower( void* from, void* to ); \
        void unpack##type##Upper( void* from, void* to );

    DECLARE_UNPACK_LOWER_AND_UPPER( Enumeration4 )
    DECLARE_UNPACK_LOWER_AND_UPPER( Nibble )
    DECLARE_UNPACK_LOWER_AND_UPPER( UInteger4 )
            
    void unpackClockIdentity( Octet *buf, ClockIdentity *c);
    void unpackClockQuality( Octet *buf, ClockQuality *c);
    void unpackTimeInterval( Octet *buf, TimeInterval *t);
    void unpackTimestamp( Octet *buf, Timestamp *t);
    void unpackPortIdentity( Octet *buf, PortIdentity *p);
    void unpackPortAddress( Octet *buf, PortAddress *p);
    void unpackPTPText( Octet *buf, PTPText *s);
    void unpackPhysicalAddress( Octet *buf, PhysicalAddress *p);
    
    void unpackMsgHeader(Octet *buf, MsgHeader *header);
    void unpackMsgManagement(Octet *buf, MsgManagement *m);
    void unpackManagementTLV(Octet *buf, MsgManagement *m);
    
    void msgUnpackManagement(Octet *buf, MsgManagement * manage);
    
    void unpackMMClockDescription( Octet *buf, MsgManagement* m);
    void unpackMMUserDescription(Octet *buf, MsgManagement* m);
    void unpackMMInitialize( Octet *buf, MsgManagement* m);
    void unpackMMDefaultDataSet( Octet *buf, MsgManagement* m);
    void unpackMMCurrentDataSet( Octet *buf, MsgManagement* m);
    void unpackMMParentDataSet( Octet *buf, MsgManagement* m);
    void unpackMMTimePropertiesDataSet( Octet *buf, MsgManagement* m);
    void unpackMMPortDataSet( Octet *buf, MsgManagement* m);
    void unpackMMPriority1( Octet *buf, MsgManagement* m);
    void unpackMMPriority2( Octet *buf, MsgManagement* m);
    void unpackMMDomain( Octet *buf, MsgManagement* m);
    void unpackMMSlaveOnly( Octet *buf, MsgManagement* m);
    void unpackMMLogAnnounceInterval( Octet *buf, MsgManagement* m);
    void unpackMMAnnounceReceiptTimeout( Octet *buf, MsgManagement* m);
    void unpackMMLogSyncInterval( Octet *buf, MsgManagement* m);
    void unpackMMVersionNumber( Octet *buf, MsgManagement* m);
    void unpackMMTime( Octet *buf, MsgManagement* m);
    void unpackMMClockAccuracy( Octet *buf, MsgManagement* m);
    void unpackMMUtcProperties( Octet *buf, MsgManagement* m);
    void unpackMMTraceabilityProperties( Octet *buf, MsgManagement* m);
    void unpackMMDelayMechanism( Octet *buf, MsgManagement* m);
    void unpackMMLogMinPdelayReqInterval( Octet *buf, MsgManagement* m);
    void unpackMMErrorStatus(Octet *buf, MsgManagement* m);
    
    void handleManagement(/*OptBuffer* optBuf, */Octet* buf, MsgManagement* incoming);
    void handleMMClockDescription(MsgManagement* incoming);
    void handleMMUserDescription(MsgManagement* incoming);
    void handleMMSaveInNonVolatileStorage(MsgManagement* incoming);
    void handleMMResetNonVolatileStorage(MsgManagement* incoming);
    void handleMMInitialize(MsgManagement* incoming);
    void handleMMDefaultDataSet(MsgManagement* incoming);
    void handleMMCurrentDataSet(MsgManagement* incoming);
    void handleMMParentDataSet(MsgManagement* incoming);
    void handleMMTimePropertiesDataSet(MsgManagement* incoming);
    void handleMMPortDataSet(MsgManagement* incoming);
    void handleMMPriority1(MsgManagement* incoming);
    void handleMMPriority2(MsgManagement* incoming);
    void handleMMDomain(MsgManagement* incoming);
    void handleMMSlaveOnly(MsgManagement* incoming);
    void handleMMLogAnnounceInterval(MsgManagement* incoming);
    void handleMMAnnounceReceiptTimeout(MsgManagement* incoming);
    void handleMMLogSyncInterval(MsgManagement* incoming);
    void handleMMVersionNumber(MsgManagement* incoming);
    void handleMMEnablePort(MsgManagement* incoming);
    void handleMMDisablePort(MsgManagement* incoming);
    void handleMMTime(MsgManagement* incoming);
    void handleMMClockAccuracy(MsgManagement* incoming);
    void handleMMUtcProperties(MsgManagement* incoming);
    void handleMMTraceabilityProperties(MsgManagement* incoming);
    void handleMMDelayMechanism(MsgManagement* incoming);
    void handleMMLogMinPdelayReqInterval(MsgManagement* incoming);
    void handleMMErrorStatus(MsgManagement *incoming);
    
    MsgManagement *incoming;
};

#endif	/* INCOMINGMANAGEMENTMESSAGE_H */

