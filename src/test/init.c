/**
 *
 * These functions initialize data structures used in test cases against ptpd functions.
 *
 */

#include "../ptpd.h"
#include "helper.h"

/* Intialize ptpClock with some default values */
void
initPtpClock(PtpClock *ptpClock) {

	/* initialize ptpClock fields to some value */
	ptpClock->twoStepFlag = TWO_STEP_FLAG;

	ptpClock->clockIdentity[0] = 0x01;
	ptpClock->clockIdentity[1] = 0x23;
	ptpClock->clockIdentity[2] = 0x45;
	ptpClock->clockIdentity[3] = 0x67;
	ptpClock->clockIdentity[4] = 0x89;
	ptpClock->clockIdentity[5] = 0xAB;
	ptpClock->clockIdentity[6] = 0xCD;
	ptpClock->clockIdentity[7] = 0xEF;

	ptpClock->numberPorts = NUMBER_PORTS;

	ptpClock->clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
	ptpClock->clockQuality.clockAccuracy = DEFAULT_CLOCK_ACCURACY;
	ptpClock->clockQuality.offsetScaledLogVariance = DEFAULT_CLOCK_VARIANCE;

	ptpClock->priority1 = DEFAULT_PRIORITY1;
	ptpClock->priority2 = DEFAULT_PRIORITY2;
	ptpClock->domainNumber =  DFLT_DOMAIN_NUMBER;
	ptpClock->slaveOnly = SLAVE_ONLY;

	ptpClock->stepsRemoved = 0x0;
	ptpClock->offsetFromMaster.seconds = 0;
	ptpClock->offsetFromMaster.nanoseconds = 0;
	ptpClock->meanPathDelay.seconds = 0;
	ptpClock->meanPathDelay.nanoseconds = 0;

        ptpClock->parentPortIdentity.clockIdentity[0] = 0x00;
        ptpClock->parentPortIdentity.clockIdentity[1] = 0x10;
        ptpClock->parentPortIdentity.clockIdentity[2] = 0x20;
        ptpClock->parentPortIdentity.clockIdentity[3] = 0x30;
        ptpClock->parentPortIdentity.clockIdentity[4] = 0x40;
        ptpClock->parentPortIdentity.clockIdentity[5] = 0x50;
        ptpClock->parentPortIdentity.clockIdentity[6] = 0x60;
        ptpClock->parentPortIdentity.clockIdentity[7] = 0x70;
	ptpClock->parentPortIdentity.portNumber = 0x1234;

	ptpClock->parentStats = DEFAULT_PARENTS_STATS;
	ptpClock->observedParentOffsetScaledLogVariance = 0;
	ptpClock->observedParentClockPhaseChangeRate = 0;
	ptpClock->grandmasterIdentity[0] = 0xF0;
	ptpClock->grandmasterIdentity[1] = 0xE0;
	ptpClock->grandmasterIdentity[2] = 0xD0;
	ptpClock->grandmasterIdentity[3] = 0xC0;
	ptpClock->grandmasterIdentity[4] = 0xB0;
	ptpClock->grandmasterIdentity[5] = 0xA0;
	ptpClock->grandmasterIdentity[6] = 0x90;
	ptpClock->grandmasterIdentity[7] = 0x80;

	ptpClock->grandmasterClockQuality.clockClass = 0xFF;
	ptpClock->grandmasterClockQuality.clockAccuracy = 0x00;
	ptpClock->grandmasterClockQuality.offsetScaledLogVariance = 0x00;
	ptpClock->grandmasterPriority1 = DEFAULT_PRIORITY1;
        ptpClock->grandmasterPriority2 = DEFAULT_PRIORITY2;

	ptpClock->currentUtcOffset = DEFAULT_UTC_OFFSET;
	ptpClock->currentUtcOffsetValid = DEFAULT_UTC_VALID;
	ptpClock->leap59 = 0x0;
	ptpClock->leap61 = 0x0;
	ptpClock->timeTraceable = 0x0;
	ptpClock->frequencyTraceable = 0x0;
	ptpClock->ptpTimescale = 0x0;
	ptpClock->timeSource = 0x0;

	ptpClock->portIdentity.clockIdentity[0] = 0xFE;
	ptpClock->portIdentity.clockIdentity[1] = 0xDC;
	ptpClock->portIdentity.clockIdentity[2] = 0xBA;
	ptpClock->portIdentity.clockIdentity[3] = 0x98;
	ptpClock->portIdentity.clockIdentity[4] = 0x76;
	ptpClock->portIdentity.clockIdentity[5] = 0x54;
	ptpClock->portIdentity.clockIdentity[6] = 0x32;
	ptpClock->portIdentity.clockIdentity[7] = 0x10;
	ptpClock->portIdentity.portNumber = 0xFFFF;

	ptpClock->portState = PTP_SLAVE;
	ptpClock->logMinDelayReqInterval = DEFAULT_DELAYREQ_INTERVAL;
	ptpClock->peerMeanPathDelay.seconds = 0x0;
	ptpClock->peerMeanPathDelay.nanoseconds = 0x0;

	ptpClock->logAnnounceInterval = DEFAULT_ANNOUNCE_INTERVAL;
	ptpClock->announceReceiptTimeout = DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
	ptpClock->logSyncInterval = DEFAULT_SYNC_INTERVAL;
	ptpClock->delayMechanism = DEFAULT_DELAY_MECHANISM;
	ptpClock->logMinPdelayReqInterval = DEFAULT_PDELAYREQ_INTERVAL;
	ptpClock->versionNumber = VERSION_PTP;

	// ptpClock->foreign

	ptpClock->number_foreign_records = 0x0;
	ptpClock->max_foreign_records = DEFAULT_MAX_FOREIGN_RECORDS;
	ptpClock->foreign_record_i = 0x0;
	ptpClock->foreign_record_best = 0x0;
	ptpClock->random_seed = 0x0;
	ptpClock->record_update = 0x0;

	// ptpClock->msgTmpHeader
	// ptpClock->msgTmp
	// ptpClock->outgoingManageTmp

        memset(ptpClock->user_description, 0, sizeof(ptpClock->user_description));
        memcpy(ptpClock->user_description, &USER_DESCRIPTION, sizeof(USER_DESCRIPTION));

	memset(ptpClock->msgObuf, 0, PACKET_SIZE);
	memset(ptpClock->msgIbuf, 0, PACKET_SIZE);

}

/* return type is int since memory allocation can fail */
int
initMMClockDescriptionFields(MMClockDescription *data) {
	int i;

	memset(data, 0, sizeof(MMClockDescription));
	/* set data in fields */
	data->clockType0 = 0x01;
	data->clockType1 = 0x23;
	data->physicalLayerProtocol.lengthField = 0x10;
	if(!(data->physicalLayerProtocol.textField = malloc(data->physicalLayerProtocol.lengthField)))
		return -1;
	for(i = 0; i < data->physicalLayerProtocol.lengthField; i++) {
		data->physicalLayerProtocol.textField[i] = i;
	}
	data->physicalAddress.addressLength = 0x18;
	if(!(data->physicalAddress.addressField = malloc(data->physicalAddress.addressLength))) {
		freeMMClockDescription(data);
		return -1;
	}
	for(i = 0; i < data->physicalAddress.addressLength; i++) {
		data->physicalAddress.addressField[i] = i;
	}
	data->protocolAddress.networkProtocol = 0x45;
	data->protocolAddress.addressLength = 0x20;
	if(!(data->protocolAddress.addressField = malloc(data->protocolAddress.addressLength))) {
		freeMMClockDescription(data);
		return -1;
	}
	for(i = 0; i < data->protocolAddress.addressLength; i++) {
		data->protocolAddress.addressField[i] = i;
	}
	data->manufacturerIdentity0 = 0x67;
	data->manufacturerIdentity1 = 0x89;
	data->manufacturerIdentity2 = 0xAB;
	data->reserved = 0x0;
	data->productDescription.lengthField = 0x08;
	if(!(data->productDescription.textField = malloc(data->productDescription.lengthField))) {
		freeMMClockDescription(data);
		return -1;
	}
	for(i = 0; i < data->productDescription.lengthField; i++) {
		data->productDescription.textField[i] = i;
	}
	data->revisionData.lengthField = 0x08;
	if(!(data->revisionData.textField = malloc(data->revisionData.lengthField))) {
		freeMMClockDescription(data);
		return -1;
	}
	for(i = 0; i < data->revisionData.lengthField; i++) {
		data->revisionData.textField[i] = i+8;
	}
	data->userDescription.lengthField = 0x07;
	if(!(data->userDescription.textField = malloc(data->userDescription.lengthField))) {
		freeMMClockDescription(data);
		return -1;
	}
	for(i = 0; i < data->userDescription.lengthField; i++) {
		data->userDescription.textField[i] = i+16;
	}
	data->profileIdentity0 = 0x01;
	data->profileIdentity1 = 0x23;
	data->profileIdentity2 = 0x45;
	data->profileIdentity3 = 0x67;
	data->profileIdentity4 = 0x89;
	data->profileIdentity5 = 0xAB;

	return 0;
}

/* return type is int since memory allocation can fail */
int
initMMUserDescriptionFields(MMUserDescription *data) {
	int i;

	memset(data, 0, sizeof(MMUserDescription));
	/* set data in fields */
        data->userDescription.lengthField = 0x07;
        MALLOC(data->userDescription.textField, data->userDescription.lengthField);
        for(i = 0; i < data->userDescription.lengthField; i++) {
                data->userDescription.textField[i] = i+16;
        }

	return 0;
}

void
initMMInitializeFields(MMInitialize *data) {
	/* set data in fields */
        data->initializeKey = 0xFF;
}

void
initMMDefaultDataSetFields(MMDefaultDataSet *data)  {
	/* set data in fields */
	data->so_tsc = 0x03;
	data->reserved0 = 0x0;
	data->numberPorts = 0x1234;
	data->priority1 = 0x80;
	data->clockQuality.clockClass = 0xFF;
	data->clockQuality.clockAccuracy = 0x80;
	data->clockQuality.offsetScaledLogVariance = 0xFFFF;
	data->priority2 = 0x80;
	data->clockIdentity[0] = 0x00;
	data->clockIdentity[1] = 0x10;
	data->clockIdentity[2] = 0x20;
	data->clockIdentity[3] = 0x30;
	data->clockIdentity[4] = 0x40;
	data->clockIdentity[5] = 0x50;
	data->clockIdentity[6] = 0x60;
	data->clockIdentity[7] = 0x70;
	data->domainNumber = 0xFF;
	data->reserved1 = 0x0;

}

void
initMMPriority1Fields(MMPriority1 *data) {
	/* set data in fields */
	data->priority1 = 0x80;
	data->reserved - 0x0;
}

void
initMMPriority2Fields(MMPriority2 *data) {
	/* set data in fields */
	data->priority2 = 0x80;
	data->reserved = 0x0;
}

void
initMMDomainFields(MMDomain *data) {
	/* set data in fields */
	data->domainNumber = 0x80;
	data->reserved = 0x0;
}

void
initMMSlaveOnlyFields(MMSlaveOnly *data) {
	/* set data in fields */
	data->so = TRUE;
	data->reserved = 0x0;
}

void
initMMTimePropertiesDataSetFields(MMTimePropertiesDataSet *data) {
	/* set data in fields */
	data->currentUtcOffset = 0x0;
	data->ftra_ttra_ptp_utcv_li59_li61 = 0x0;
	data->timeSource = 0x0;
}

void
initMMUtcPropertiesFields(MMUtcProperties *data) {
	/* set data in fields */
	data->currentUtcOffset = 0x0;
	data->utcv_li59_li61 = 0x0;
	data->reserved = 0;
}

void
initMMTraceabilityPropertiesFields(MMTraceabilityProperties *data) {
	/* set data in fields */
	data->ftra_ttra = 0x0;
	data->reserved = 0x0;
}

