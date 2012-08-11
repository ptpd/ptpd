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
 * @file        freeing.cpp
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       Memory freeing functions.
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

/**
 * @brief Free a PortAddress Structure.
 * 
 * @param p     A PortAddress Structure to free.
 */
void freePortAddress(PortAddress *p)
{
    if(p->addressField) {
        free(p->addressField);
        p->addressField = NULL;
    }
}

/**
 * @brief Free a PTPText Structure.
 * 
 * @param s     A PTPText Structure to free.
 */
void freePTPText(PTPText *s)
{
    if(s->textField) {
        free(s->textField);
        s->textField = NULL;
    }
}

/**
 * @brief Free a PhysicalAddress Structure.
 * 
 * @param p     A PhysicalAddress Structure to free.
 */
void freePhysicalAddress(PhysicalAddress *p)
{
    if(p->addressField) {
        free(p->addressField);
        p->addressField = NULL;
    }
}

/**
 * @brief Free ManagementTLV Clock Description message.
 * 
 * @param data  A Clock Description ManagementTLV to free.
 */
void freeMMClockDescription( MMClockDescription* data)
{
    #define OPERATE( name, size, type ) \
        free##type( &data->name);
    #include "../../src/def/managementTLV/clockDescription.def"
}

/**
 * @brief Free ManagementTLV User Description message.
 * 
 * @param data  A User Description ManagementTLV to free.
 */
void freeMMUserDescription( MMUserDescription* data)
{
    #define OPERATE( name, size, type ) \
        free##type( &data->name);
    #include "../../src/def/managementTLV/userDescription.def"
}

/**
 * @brief Free ManagementTLV Error Status message.
 * 
 * @param data  An Error Status ManagementTLV to free.
 */
void freeMMErrorStatus( MMErrorStatus* data)
{
    #define OPERATE( name, size, type ) \
        free##type( &data->name);
    #include "../../src/def/managementTLV/errorStatus.def"
}

/**
 * @brief Initialize freeing ManagementTLV Error Status message.
 * 
 * @param tlv   An Error Status ManagementTLV to free.
 */
void freeMMErrorStatusTLV(ManagementTLV *tlv)
{
    DBG("cleanup managementErrorStatusTLV data \n");
    freeMMErrorStatus((MMErrorStatus*)tlv->dataField);
}

/**
 * @brief Free ManagementTLV message.
 * 
 * @param tlv   A ManagementTLV to free.
 */
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

/**
 * @brief Free ManagementTLV message.
 * 
 * @param m     A Management Message to free.
 */
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