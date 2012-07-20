/** 
 * @file        freeing.h
 * @author      Tomasz Kleinschmidt
 *
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

