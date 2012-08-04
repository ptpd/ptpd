/** 
 * @file        Display.h
 * @author      Tomasz Kleinschmidt
 *
 * @brief       Display functions definitions.
 */

#ifndef DISPLAY_H
#define	DISPLAY_H

#include "datatypes.h"

#define READ_FIELD(data, bitpos) \
        (data & ( 0x1 << bitpos ))

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

