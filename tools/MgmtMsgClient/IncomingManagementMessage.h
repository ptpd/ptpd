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
    
    DECLARE_UNPACK( Enumeration16 )
    DECLARE_UNPACK( Integer8 )
    DECLARE_UNPACK( UInteger8 )
    DECLARE_UNPACK( Integer16 )
    DECLARE_UNPACK( UInteger16 )
    DECLARE_UNPACK( Integer32 )
    DECLARE_UNPACK( UInteger32 )
    DECLARE_UNPACK( Integer64 )
    DECLARE_UNPACK( Octet )
            
    #define DECLARE_UNPACK_LOWER_AND_UPPER( type ) \
        void unpack##type##Lower( void* from, void* to ); \
        void unpack##type##Upper( void* from, void* to );

    DECLARE_UNPACK_LOWER_AND_UPPER( Enumeration4 )
    DECLARE_UNPACK_LOWER_AND_UPPER( Nibble )
    DECLARE_UNPACK_LOWER_AND_UPPER( UInteger4 )
            
    void unpackClockIdentity( Octet *buf, ClockIdentity *c);
    void unpackPortIdentity( Octet *buf, PortIdentity *p);
    void unpackMsgHeader(Octet *buf, MsgHeader *header);
    void unpackMsgManagement(Octet *buf, MsgManagement *m);
    void unpackManagementTLV(Octet *buf, MsgManagement *m);
    
    void msgUnpackManagement(Octet *buf, MsgManagement * manage);
    
    MsgManagement *incoming;
};

#endif	/* INCOMINGMANAGEMENTMESSAGE_H */

