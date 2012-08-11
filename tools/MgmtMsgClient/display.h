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
 * @file        Display.h
 * @author      Tomasz Kleinschmidt
 *
 * @brief       Display functions definitions.
 */

#ifndef DISPLAY_H
#define	DISPLAY_H

#include "datatypes.h"

#define IS_SET(data, bitpos) \
	((data & ( 0x1 << bitpos )) == (0x1 << bitpos))

void integer64_display(Integer64 * bigint);
void uInteger48_display(UInteger48 * bigint);
void timestamp_display(Timestamp * timestamp);
void clockIdentity_display(ClockIdentity clockIdentity);
void clockUUID_display(Octet * sourceUuid);
void timeInterval_display(TimeInterval * timeInterval);
void portIdentity_display(PortIdentity * portIdentity);
void clockQuality_display(ClockQuality * clockQuality);
void PTPText_display(PTPText *p);

void msgHeader_display(MsgHeader * header);
void msgManagement_display(MsgManagement * manage);

void mMClockDescription_display(MMClockDescription *clockDescription);
void mMUserDescription_display(MMUserDescription* userDescription);
void mMInitialize_display(MMInitialize* initialize);
void mMDefaultDataSet_display(MMDefaultDataSet* defaultDataSet);
void mMCurrentDataSet_display(MMCurrentDataSet* currentDataSet);
void mMParentDataSet_display(MMParentDataSet* parentDataSet);
void mMTimePropertiesDataSet_display(MMTimePropertiesDataSet* timePropertiesDataSet);
void mMPortDataSet_display(MMPortDataSet* portDataSet);
void mMPriority1_display(MMPriority1* priority1);
void mMPriority2_display(MMPriority2* priority2);
void mMDomain_display(MMDomain* domain);
void mMSlaveOnly_display(MMSlaveOnly *slaveOnly);
void mMLogAnnounceInterval_display(MMLogAnnounceInterval* logAnnounceInterval);
void mMAnnounceReceiptTimeout_display(MMAnnounceReceiptTimeout* announceReceiptTimeout);
void mMLogSyncInterval_display(MMLogSyncInterval* logSyncInterval);
void mMVersionNumber_display(MMVersionNumber* versionNumber);
void mMTime_display(MMTime* time);
void mMClockAccuracy_display(MMClockAccuracy* clockAccuracy);
void mMUtcProperties_display(MMUtcProperties* utcProperties);
void mMTraceabilityProperties_display(MMTraceabilityProperties* traceabilityProperties);
void mMDelayMechanism_display(MMDelayMechanism* delayMechanism);
void mMLogMinPdelayReqInterval_display(MMLogMinPdelayReqInterval* logMinPdelayReqInterval);
void mMErrorStatus_display(MMErrorStatus* errorStatus);

#endif	/* DISPLAY_H */

