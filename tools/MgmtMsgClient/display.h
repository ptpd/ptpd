/** 
 * @file        Display.h
 * @author      Tomasz Kleinschmidt
 *
 * @brief       Display functions definitions.
 */

#ifndef DISPLAY_H
#define	DISPLAY_H

#include "datatypes.h"

void integer64_display(Integer64 * bigint);
void clockIdentity_display(ClockIdentity clockIdentity);
void clockUUID_display(Octet * sourceUuid);
void portIdentity_display(PortIdentity * portIdentity);
void PTPText_display(PTPText *p);

void msgHeader_display(MsgHeader * header);
void msgManagement_display(MsgManagement * manage);

void mMClockDescription_display(MMClockDescription *clockDescription);
void mMErrorStatus_display(MMErrorStatus* errorStatus);

#endif	/* DISPLAY_H */

