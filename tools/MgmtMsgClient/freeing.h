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
 * @file        freeing.h
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       Memory freeing functions definitions.
 */

#ifndef FREEING_H
#define	FREEING_H

#include "datatypes.h"
#include "datatypes_dep.h"

/* The free function is intentionally empty. However, this simplifies
 * the procedure to deallocate complex data types
 */
#define DECLARE_FREE( type ) \
void free##type( void* x);

DECLARE_FREE ( Boolean )
DECLARE_FREE ( UInteger8 )
DECLARE_FREE ( Octet )
DECLARE_FREE ( Enumeration8 )
DECLARE_FREE ( Integer8 )
DECLARE_FREE ( Enumeration16 )
DECLARE_FREE ( Integer16 )
DECLARE_FREE ( UInteger16 )
DECLARE_FREE ( Integer32 )
DECLARE_FREE ( UInteger32 )
DECLARE_FREE ( Enumeration4 )
DECLARE_FREE ( UInteger4 )
DECLARE_FREE ( Nibble )
        
void freePortAddress(PortAddress *p);
void freePTPText(PTPText *s);
void freePhysicalAddress(PhysicalAddress *p);

void freeMMClockDescription( MMClockDescription* data);
void freeMMUserDescription( MMUserDescription* data);
void freeMMErrorStatus( MMErrorStatus* data);
void freeMMErrorStatusTLV(ManagementTLV *tlv);

void freeMMTLV(ManagementTLV* tlv);
void freeManagementTLV(MsgManagement *m);

#endif	/* FREEING_H */

