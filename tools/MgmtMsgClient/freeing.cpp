/** 
 * @file        freeing.cpp
 * @author      Tomasz Kleinschmidt
 * 
 */

#include "freeing.h"

#include <stdio.h>
#include <stdlib.h>

#include "MgmtMsgClient.h"

#include "constants_dep.h"

/* The free function is intentionally empty. However, this simplifies
 * the procedure to deallocate complex data types
 */
#define FREE( type ) \
void free##type( void* x) \
{}

FREE ( Boolean )
FREE ( UInteger8 )
FREE ( Octet )
FREE ( Enumeration8 )
FREE ( Integer8 )
FREE ( Enumeration16 )
FREE ( Integer16 )
FREE ( UInteger16 )
FREE ( Integer32 )
FREE ( UInteger32 )
FREE ( Enumeration4 )
FREE ( UInteger4 )
FREE ( Nibble )

void freePortAddress(PortAddress *p)
{
    if(p->addressField) {
        free(p->addressField);
        p->addressField = NULL;
    }
}

void freePTPText(PTPText *s)
{
    if(s->textField) {
        free(s->textField);
        s->textField = NULL;
    }
}

void freePhysicalAddress(PhysicalAddress *p)
{
    if(p->addressField) {
        free(p->addressField);
        p->addressField = NULL;
    }
}

void freeMMClockDescription( MMClockDescription* data)
{
    #define OPERATE( name, size, type ) \
        free##type( &data->name);
    #include "../../src/def/managementTLV/clockDescription.def"
}

void freeMMUserDescription( MMUserDescription* data)
{
    #define OPERATE( name, size, type ) \
        free##type( &data->name);
    #include "../../src/def/managementTLV/userDescription.def"
}

void freeMMErrorStatus( MMErrorStatus* data)
{
    #define OPERATE( name, size, type ) \
        free##type( &data->name);
    #include "../../src/def/managementTLV/errorStatus.def"
}

void freeMMErrorStatusTLV(ManagementTLV *tlv)
{
    DBG("cleanup managementErrorStatusTLV data \n");
    freeMMErrorStatus((MMErrorStatus*)tlv->dataField);
}

void freeMMTLV(ManagementTLV* tlv)
{
    DBG("cleanup managementTLV data\n");
    switch(tlv->managementId)
    {
        case MM_CLOCK_DESCRIPTION:
            DBG("cleanup clock description \n");
            freeMMClockDescription((MMClockDescription*)tlv->dataField);
            break;
        case MM_USER_DESCRIPTION:
            DBG("cleanup user description \n");
            freeMMUserDescription((MMUserDescription*)tlv->dataField);
            break;
        case MM_NULL_MANAGEMENT:
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
        default:
            DBG("no managementTLV data to cleanup \n");
    }
}

void freeManagementTLV(MsgManagement *m)
{
    /* cleanup outgoing managementTLV */
    if(m->tlv) {
        if(m->tlv->dataField) {
            if(m->tlv->tlvType == TLV_MANAGEMENT) {
                freeMMTLV(m->tlv);
            } else if(m->tlv->tlvType == TLV_MANAGEMENT_ERROR_STATUS) {
                freeMMErrorStatusTLV(m->tlv);
            }
            free(m->tlv->dataField);
            m->tlv->dataField = NULL;
        }
        free(m->tlv);
        m->tlv = NULL;
    }
}