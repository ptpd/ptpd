/** 
 * @file        OutgoingManagementMessage.h
 * @author      Tomasz Kleinschmidt
 *
 * @brief       OutgoingManagementMessage class definition.
 */

#ifndef OUTGOINGMANAGEMENTMESSAGE_H
#define	OUTGOINGMANAGEMENTMESSAGE_H

#include "OptBuffer.h"

#include "datatypes.h"
#include "datatypes_dep.h"

class OutgoingManagementMessage {
public:
    OutgoingManagementMessage(Octet* buf, OptBuffer* optBuf);
    virtual ~OutgoingManagementMessage();
    
private:
    #define DECLARE_PACK( type ) void pack##type( void*, void* );

    DECLARE_PACK( Boolean )
    DECLARE_PACK( Enumeration8 )
    DECLARE_PACK( Enumeration16 )
    DECLARE_PACK( Integer8 )
    DECLARE_PACK( UInteger8 )
    DECLARE_PACK( Integer16)
    DECLARE_PACK( UInteger16 )
    DECLARE_PACK( Integer32 )
    DECLARE_PACK( UInteger32 )
    DECLARE_PACK( UInteger48 )
    DECLARE_PACK( Integer64 )
    DECLARE_PACK( Octet )
         
    #define DECLARE_PACK_LOWER_AND_UPPER( type ) \
        void pack##type##Lower( void* from, void* to ); \
        void pack##type##Upper( void* from, void* to );

    DECLARE_PACK_LOWER_AND_UPPER( Enumeration4 )
    DECLARE_PACK_LOWER_AND_UPPER( Nibble )
    DECLARE_PACK_LOWER_AND_UPPER( UInteger4 )        
        
    void packClockIdentity( ClockIdentity *c, Octet *buf);
    void packTimestamp( Timestamp *t, Octet *buf);
    void packPortIdentity( PortIdentity *p, Octet *buf);
    void packPTPText(PTPText *s, Octet *buf);
    
    UInteger16 packMMUserDescription( MsgManagement* m, Octet *buf);
    UInteger16 packMMInitialize( MsgManagement* m, Octet *buf);
    UInteger16 packMMPriority1( MsgManagement* m, Octet *buf);
    UInteger16 packMMPriority2( MsgManagement* m, Octet *buf);
    UInteger16 packMMDomain( MsgManagement* m, Octet *buf);
    UInteger16 packMMSlaveOnly( MsgManagement* m, Octet *buf);
    UInteger16 packMMLogAnnounceInterval( MsgManagement* m, Octet *buf);
    UInteger16 packMMAnnounceReceiptTimeout( MsgManagement* m, Octet *buf);
    UInteger16 packMMLogSyncInterval( MsgManagement* m, Octet *buf);
    UInteger16 packMMVersionNumber( MsgManagement* m, Octet *buf);
    UInteger16 packMMTime( MsgManagement* m, Octet *buf);
    UInteger16 packMMClockAccuracy( MsgManagement* m, Octet *buf);
    UInteger16 packMMUtcProperties( MsgManagement* m, Octet *buf);
    UInteger16 packMMTraceabilityProperties( MsgManagement* m, Octet *buf);
    UInteger16 packMMDelayMechanism( MsgManagement* m, Octet *buf);
    UInteger16 packMMLogMinPdelayReqInterval( MsgManagement* m, Octet *buf);
    
    void packMsgHeader(MsgHeader *h, Octet *buf);
    void packMsgManagement(MsgManagement *m, Octet *buf);
    void packManagementTLV(ManagementTLV *tlv, Octet *buf);
    
    void msgPackManagement(Octet *buf, MsgManagement *outgoing);
    void msgPackManagementTLV(OptBuffer* optBuf, Octet *buf, MsgManagement *outgoing);
    
    void initOutgoingMsgManagement(MsgManagement* outgoing);
    
    void handleManagement(OptBuffer* optBuf, Octet* buf, MsgManagement* outgoing);
    void handleMMNullManagement(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMClockDescription(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMUserDescription(MsgManagement* outgoing, Enumeration4 actionField, PTPText userDescription);
    void handleMMSaveInNonVolatileStorage(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMResetNonVolatileStorage(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMInitialize(MsgManagement* outgoing, Enumeration4 actionField, Enumeration16 initializeKey);
    void handleMMDefaultDataSet(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMCurrentDataSet(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMParentDataSet(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMTimePropertiesDataSet(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMPortDataSet(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMPriority1(MsgManagement* outgoing, Enumeration4 actionField, UInteger8 priority1);
    void handleMMPriority2(MsgManagement* outgoing, Enumeration4 actionField, UInteger8 priority2);
    void handleMMDomain(MsgManagement* outgoing, Enumeration4 actionField, UInteger8 domainNumber);
    void handleMMSlaveOnly(MsgManagement* outgoing, Enumeration4 actionField, Boolean so);
    void handleMMLogAnnounceInterval(MsgManagement* outgoing, Enumeration4 actionField, Integer8 logAnnounceInterval);
    void handleMMAnnounceReceiptTimeout(MsgManagement* outgoing, Enumeration4 actionField, UInteger8 announceReceiptTimeout);
    void handleMMLogSyncInterval(MsgManagement* outgoing, Enumeration4 actionField, Integer8 logSyncInterval);
    void handleMMVersionNumber(MsgManagement* outgoing, Enumeration4 actionField, UInteger4Lower versionNumber);
    void handleMMEnablePort(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMDisablePort(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMTime(MsgManagement* outgoing, Enumeration4 actionField/*, Timestamp currentTime*/);
    void handleMMClockAccuracy(MsgManagement* outgoing, Enumeration4 actionField, Enumeration8 clockAccuracy);
    void handleMMUtcProperties(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMTraceabilityProperties(MsgManagement* outgoing, Enumeration4 actionField);
    void handleMMDelayMechanism(MsgManagement* outgoing, Enumeration4 actionField, Enumeration8 delayMechanism);
    void handleMMLogMinPdelayReqInterval(MsgManagement* outgoing, Enumeration4 actionField, Integer8 logMinPdelayReqInterval);
    
    MsgManagement *outgoing;
};

#endif	/* OUTGOINGMANAGEMENTMESSAGE_H */

