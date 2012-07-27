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
    this->outgoing = (MsgManagement *)malloc(sizeof(MsgManagement));
    
    handleManagement(optBuf, buf, this->outgoing);
}

/**
 * @brief OutgoingManagementMessage deconstructor.
 * 
 * The deconstructor frees memory.
 */
OutgoingManagementMessage::~OutgoingManagementMessage() {
    //free(this->outgoing->tlv);
    freeManagementTLV(this->outgoing);
    free(this->outgoing);
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

void OutgoingManagementMessage::packPTPText(PTPText *s, Octet *buf)
{
	packUInteger8(&s->lengthField, buf);
	if(s->lengthField) {
		memcpy( buf+1, s->textField, s->lengthField);
	}
}

UInteger16 OutgoingManagementMessage::packMMUserDescription( MsgManagement* m, Octet *buf)
{
	int offset = 0;
	Octet pad = 0;
	MMUserDescription* data = (MMUserDescription*)m->tlv->dataField;
	#define OPERATE( name, size, type ) \
		pack##type( &data->name,\
			    buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset); \
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

UInteger16 OutgoingManagementMessage::packMMInitialize( MsgManagement* m, Octet *buf)
{
    int offset = 0;
    MMInitialize* data = (MMInitialize*)m->tlv->dataField;
    #define OPERATE( name, size, type ) \
            pack##type( &data->name,\
                        buf + MANAGEMENT_LENGTH + TLV_LENGTH + offset ); \
            offset = offset + size;
    #include "../../src/def/managementTLV/initialize.def"

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

void OutgoingManagementMessage::msgPackManagement(Octet *buf, MsgManagement *outgoing)
{
    DBG("packing management message \n");
    packMsgManagement(outgoing, buf);
}

/* Pack Management message into OUT buffer */
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
            lengthField = 0;
            break;
	case MM_CLOCK_DESCRIPTION:
            lengthField = 0;
            break;
//		lengthField = packMMClockDescription(outgoing, buf);
//		#ifdef PTPD_DBG
//		mMClockDescription_display(
//				(MMClockDescription*)outgoing->tlv->dataField, ptpClock);
//		#endif /* PTPD_DBG */
//		break;
        case MM_USER_DESCRIPTION:
            if (optBuf->action_type == SET)
                lengthField = packMMUserDescription(outgoing, buf);
            else
                lengthField = 0;
//                lengthField = packMMUserDescription(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMUserDescription_display(
//                                (MMUserDescription*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
            break;
        case MM_INITIALIZE:
            if (optBuf->action_type == COMMAND)
                lengthField = packMMInitialize(outgoing, buf);
            else
                lengthField = 0;
//                lengthField = packMMInitialize(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMInitialize_display(
//                                (MMInitialize*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
            break;
        case MM_DEFAULT_DATA_SET:
//                lengthField = packMMDefaultDataSet(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMDefaultDataSet_display(
//                                (MMDefaultDataSet*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_CURRENT_DATA_SET:
//                lengthField = packMMCurrentDataSet(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMCurrentDataSet_display(
//                                (MMCurrentDataSet*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_PARENT_DATA_SET:
//                lengthField = packMMParentDataSet(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMParentDataSet_display(
//                                (MMParentDataSet*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_TIME_PROPERTIES_DATA_SET:
//                lengthField = packMMTimePropertiesDataSet(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMTimePropertiesDataSet_display(
//                                (MMTimePropertiesDataSet*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_PORT_DATA_SET:
//                lengthField = packMMPortDataSet(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMPortDataSet_display(
//                                (MMPortDataSet*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_PRIORITY1:
//                lengthField = packMMPriority1(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMPriority1_display(
//                                (MMPriority1*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_PRIORITY2:
//                lengthField = packMMPriority2(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMPriority2_display(
//                                (MMPriority2*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_DOMAIN:
//                lengthField = packMMDomain(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMDomain_display(
//                                (MMDomain*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
	case MM_SLAVE_ONLY:
//		lengthField = packMMSlaveOnly(outgoing, buf);
//		#ifdef PTPD_DBG
//		mMSlaveOnly_display(
//				(MMSlaveOnly*)outgoing->tlv->dataField, ptpClock);
//		#endif /* PTPD_DBG */
//		break;
        case MM_LOG_ANNOUNCE_INTERVAL:
//                lengthField = packMMLogAnnounceInterval(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMLogAnnounceInterval_display(
//                                (MMLogAnnounceInterval*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_ANNOUNCE_RECEIPT_TIMEOUT:
//                lengthField = packMMAnnounceReceiptTimeout(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMAnnounceReceiptTimeout_display(
//                                (MMAnnounceReceiptTimeout*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_LOG_SYNC_INTERVAL:
//                lengthField = packMMLogSyncInterval(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMLogSyncInterval_display(
//                                (MMLogSyncInterval*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_VERSION_NUMBER:
//                lengthField = packMMVersionNumber(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMVersionNumber_display(
//                                (MMVersionNumber*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_TIME:
//                lengthField = packMMTime(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMTime_display(
//                                (MMTime*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_CLOCK_ACCURACY:
//                lengthField = packMMClockAccuracy(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMClockAccuracy_display(
//                                (MMClockAccuracy*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_UTC_PROPERTIES:
//                lengthField = packMMUtcProperties(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMUtcProperties_display(
//                                (MMUtcProperties*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_TRACEABILITY_PROPERTIES:
//                lengthField = packMMTraceabilityProperties(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMTraceabilityProperties_display(
//                                (MMTraceabilityProperties*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_DELAY_MECHANISM:
//                lengthField = packMMDelayMechanism(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMDelayMechanism_display(
//                                (MMDelayMechanism*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
        case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
//                lengthField = packMMLogMinPdelayReqInterval(outgoing, buf);
//                #ifdef PTPD_DBG
//                mMLogMinPdelayReqInterval_display(
//                                (MMLogMinPdelayReqInterval*)outgoing->tlv->dataField, ptpClock);
//                #endif /* PTPD_DBG */
//                break;
	default:
		DBG("packing management msg: unsupported id \n");
    }

    /* set lengthField */
    outgoing->tlv->lengthField = lengthField;

    packManagementTLV((ManagementTLV*)outgoing->tlv, buf);
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
    
    /* TODO: Assign value to sourcePortIdentity */
    memset(&(outgoing->header.sourcePortIdentity), 0, sizeof(PortIdentity));
    
    /* TODO: Assign value to sequenceId */
    outgoing->header.sequenceId = 0;
    
    outgoing->header.controlField = 0x0; /* deprecrated for ptp version 2 */
    outgoing->header.logMessageInterval = 0x7F;

    /* set management message fields */
    /* TODO: Assign value to targetPortIdentity */
    memset(&(outgoing->targetPortIdentity), 1, sizeof(PortIdentity));
    
    /* TODO: Assign value to startingBoundaryHops */
    outgoing->startingBoundaryHops = 0;
    
    outgoing->boundaryHops = outgoing->startingBoundaryHops;
    outgoing->actionField = 0; /* set default action, avoid uninitialized value */

    /* init managementTLV */
    //XMALLOC(outgoing->tlv, sizeof(ManagementTLV));
    outgoing->tlv = (ManagementTLV*) malloc (sizeof(ManagementTLV));
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
            //unpackMMClockDescription(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
            break;
                
        case MM_USER_DESCRIPTION:
            DBG("handleManagement: User Description\n");
            //unpackMMUserDescription(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
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
//            unpackMMInitialize(ptpClock->msgIbuf, &ptpClock->msgTmp.manage, ptpClock);
            Enumeration16 initializeKey;
            if (optBuf->action_type == COMMAND)
                sscanf(optBuf->value.textField, "%uh", &initializeKey);
            else
                initializeKey = 0; /* avoid uninitialized value */
            
            handleMMInitialize(outgoing, optBuf->action_type, initializeKey);
            break;
                
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
            printf("handleManagement: Currently unsupported managementTLV %d\n", optBuf->mgmt_id);
            exit(1);
        
        default:
            printf("handleManagement: Unknown managementTLV %d\n", optBuf->mgmt_id);
            exit(1);
    }
    
    /* pack ManagementTLV */
    msgPackManagementTLV(optBuf, buf, outgoing);

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
 * @brief Handle outgoing CLOCK_DESCRIPTION management message
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
//        case RESPONSE:
//            DBG(" RESPONSE action \n");
//            break;
        default:
            ERROR(" unknown actionType \n");
            exit(1);
    }
}

/**
 * @brief Handle outgoing USER_DESCRIPTION management message type
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
//    outgoing->tlv->dataField = (MMUserDescription*) malloc (sizeof(MMUserDescription));
    
    switch(actionField)
    {
        case SET:
            DBG(" SET action \n");
            data = (MMUserDescription*) malloc (sizeof(MMUserDescription));
            userDescriptionLength = userDescription.lengthField;
            if(userDescriptionLength <= USER_DESCRIPTION_MAX) {
                memset(data, 0, sizeof(MMUserDescription));
                memcpy(data->userDescription.textField, userDescription.textField, userDescriptionLength);
            } else {
                ERROR("management user description exceeds specification length \n");
                exit(1);
            }
            //outgoing->tlv->dataField = data;
//            data = (MMUserDescription*)incoming->tlv->dataField;
//            UInteger8 userDescriptionLength = data->userDescription.lengthField;
//            if(userDescriptionLength <= USER_DESCRIPTION_MAX) {
//                memset(ptpClock->user_description, 0, sizeof(ptpClock->user_description));
//                memcpy(ptpClock->user_description, data->userDescription.textField,
//                                userDescriptionLength);
//                /* add null-terminator to make use of C string function strlen later */
//                ptpClock->user_description[userDescriptionLength] = '\0';
//            } else {
//                WARNING("management user description exceeds specification length \n");
//            }
            outgoing->tlv->dataField = (Octet*) data;          
            break;
            
        case GET:
            DBG(" GET action \n");
//            outgoing->actionField = RESPONSE;
//            XMALLOC(outgoing->tlv->dataField, sizeof( MMUserDescription));
//            data = (MMUserDescription*)outgoing->tlv->dataField;
//            memset(data, 0, sizeof(MMUserDescription));
//            /* GET actions */
//            data->userDescription.lengthField = strlen(ptpClock->user_description);
//            XMALLOC(data->userDescription.textField,
//                                    data->userDescription.lengthField);
//            memcpy(data->userDescription.textField,
//                    ptpClock->user_description,
//                    data->userDescription.lengthField);
            break;
            
        default:
            ERROR(" unknown actionType \n");
//            free(outgoing->tlv);
//            handleErrorManagementMessage(incoming, outgoing,
//                    ptpClock, MM_USER_DESCRIPTION,
//                    NOT_SUPPORTED);
            exit(1);
    }
}

/**
 * @brief Handle incoming SAVE_IN_NON_VOLATILE_STORAGE management message type
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
                /* issue a NOT_SUPPORTED error management message, intentionally fall through */
        case ACKNOWLEDGE:
                /* issue a NOT_SUPPORTED error management message, intentionally fall through */
        default:
            ERROR(" unknown actionType \n");
//                free(outgoing->tlv);
//                handleErrorManagementMessage(incoming, outgoing,
//                        ptpClock, MM_SAVE_IN_NON_VOLATILE_STORAGE,
//                        NOT_SUPPORTED);
            exit(1);
    }
}

/**
 * @brief Handle incoming RESET_NON_VOLATILE_STORAGE management message type
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
                /* issue a NOT_SUPPORTED error management message, intentionally fall through */
        case ACKNOWLEDGE:
                /* issue a NOT_SUPPORTED error management message, intentionally fall through */
        default:
            DBG(" unknown actionType \n");
//            free(outgoing->tlv);
//            handleErrorManagementMessage(incoming, outgoing,
//                    ptpClock, MM_RESET_NON_VOLATILE_STORAGE,
//                    NOT_SUPPORTED);
            exit(1);
    }
}

/**
 * @brief Handle incoming INITIALIZE management message type
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
            data = (MMInitialize*) malloc (sizeof(MMInitialize));
//            XMALLOC(outgoing->tlv->dataField, sizeof(MMInitialize));
//            incomingData = (MMInitialize*)incoming->tlv->dataField;
//            outgoingData = (MMInitialize*)outgoing->tlv->dataField;
            
            /* Table 45 - INITIALIZATION_KEY enumeration */
            switch( initializeKey )
            {
                case INITIALIZE_EVENT:
                    /* cause INITIALIZE event */
//                    ptpClock->portState = PTP_INITIALIZING;
                    break;
                default:
                    /* do nothing, implementation specific */
                    DBG("initializeKey != 0, do nothing\n");
            }
//            outgoingData->initializeKey = incomingData->initializeKey;
            data->initializeKey = initializeKey;
            outgoing->tlv->dataField = (Octet*) data;
            break;
            
//        case ACKNOWLEDGE:
//            DBG(" ACKNOWLEDGE action\n");
//            /* TODO: implementation specific */
//            break;
            
        default:
            DBG(" unknown actionType \n");
//            free(outgoing->tlv);
//            handleErrorManagementMessage(incoming, outgoing,
//                    ptpClock, MM_INITIALIZE,
//                    NOT_SUPPORTED);
            exit(1);
    }
}