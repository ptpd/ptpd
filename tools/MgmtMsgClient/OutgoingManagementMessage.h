/** 
 * @file        OutgoingManagementMessage.h
 * @author      Tomasz Kleinschmidt
 *
 * @brief       OutgoingManagementMessage class definition
 */

#ifndef OUTGOINGMANAGEMENTMESSAGE_H
#define	OUTGOINGMANAGEMENTMESSAGE_H

#include "datatypes.h"
#include "datatypes_dep.h"
#include "OptBuffer.h"

class OutgoingManagementMessage {
public:
    OutgoingManagementMessage(Octet* buf, OptBuffer* optBuf);
    virtual ~OutgoingManagementMessage();
    
private:
    #define DECLARE_PACK( type ) void pack##type( void*, void* );
    //PACK_SIMPLE
    //DECLARE_PACK( NibbleUpper )
    //DECLARE_PACK( Enumeration4Lower )
    //DECLARE_PACK( UInteger4Lower )
    DECLARE_PACK( UInteger16 )
    DECLARE_PACK( UInteger8 )
    DECLARE_PACK( Octet )
    DECLARE_PACK( Integer8 )
    DECLARE_PACK( UInteger48 )
    DECLARE_PACK( Integer64 )
            
    DECLARE_PACK( Boolean )
    DECLARE_PACK( Enumeration8 )
    
    //PACK_ENDIAN
    DECLARE_PACK( Enumeration16 )
    DECLARE_PACK( Integer16)
    DECLARE_PACK( Integer32 )
    DECLARE_PACK( UInteger32 )
            
    #define DECLARE_PACK_LOWER_AND_UPPER( type ) \
        void pack##type##Lower( void* from, void* to ); \
        void pack##type##Upper( void* from, void* to );

    DECLARE_PACK_LOWER_AND_UPPER( Enumeration4 )
    DECLARE_PACK_LOWER_AND_UPPER( UInteger4 )
    DECLARE_PACK_LOWER_AND_UPPER( Nibble )
            
        
    void packClockIdentity( ClockIdentity *c, Octet *buf);
    void packPortIdentity( PortIdentity *p, Octet *buf);
    void packMsgHeader(MsgHeader *h, Octet *buf);
    void packMsgManagement(MsgManagement *m, Octet *buf);
    void packManagementTLV(ManagementTLV *tlv, Octet *buf);
    
    void initOutgoingMsgManagement(/*MsgManagement* incoming, */MsgManagement* outgoing/*, PtpClock *ptpClock*/);
    
    void handleMMNullManagement(/*MsgManagement* incoming, */MsgManagement* outgoing/*, PtpClock* ptpClock*/, Enumeration4 actionField);
    
    MsgManagement *outgoing;
};

#endif	/* OUTGOINGMANAGEMENTMESSAGE_H */

