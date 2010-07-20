/**
 * @file   bmc.c
 * @date   Wed Jun 23 09:36:09 2010
 * 
 * @brief  Best master clock selection code.
 * 
 * The functions in this file are used by the daemon to select the
 * best master clock from any number of possibilities.
 */

#include "ptpd.h"

void 
initData(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	DBG("initData\n");

	if (rtOpts->slaveOnly)
		rtOpts->clockStratum = 255;

	/* Port configuration data set */
	ptpClock->last_sync_event_sequence_number = 0;
	ptpClock->last_general_event_sequence_number = 0;
	ptpClock->port_id_field = 1;
	ptpClock->burst_enabled = BURST_ENABLED;

	/* Default data set */
	ptpClock->clock_communication_technology = ptpClock->port_communication_technology;
	memcpy(ptpClock->clock_uuid_field, ptpClock->port_uuid_field, PTP_UUID_LENGTH);
	ptpClock->clock_port_id_field = 0;
	ptpClock->clock_stratum = rtOpts->clockStratum;
	memcpy(ptpClock->clock_identifier, rtOpts->clockIdentifier, PTP_CODE_STRING_LENGTH);
	ptpClock->sync_interval = rtOpts->syncInterval;

	ptpClock->clock_variance = rtOpts->clockVariance;	/* see spec 7.7 */
	ptpClock->clock_followup_capable = CLOCK_FOLLOWUP;
	ptpClock->preferred = rtOpts->clockPreferred;
	ptpClock->initializable = INITIALIZABLE;
	ptpClock->external_timing = EXTERNAL_TIMING;
	ptpClock->is_boundary_clock = BOUNDARY_CLOCK;
	memcpy(ptpClock->subdomain_name, rtOpts->subdomainName, PTP_SUBDOMAIN_NAME_LENGTH);
	ptpClock->number_ports = NUMBER_PORTS;
	ptpClock->number_foreign_records = 0;
	ptpClock->max_foreign_records = rtOpts->max_foreign_records;

	/* Global time properties data set */
	ptpClock->current_utc_offset = rtOpts->currentUtcOffset;
	ptpClock->epoch_number = rtOpts->epochNumber;

	/* other stuff */
	ptpClock->random_seed = ptpClock->port_uuid_field[PTP_UUID_LENGTH - 1];
}

/* see spec table 18 */
void 
m1(PtpClock * ptpClock)
{
	/* Default data set */
	ptpClock->steps_removed = 0;
	ptpClock->offset_from_master.seconds = 0;
	ptpClock->offset_from_master.nanoseconds = 0;
	ptpClock->one_way_delay.seconds = 0;
	ptpClock->one_way_delay.nanoseconds = 0;

	/* Parent data set */
	ptpClock->parent_communication_technology = ptpClock->clock_communication_technology;
	memcpy(ptpClock->parent_uuid, ptpClock->clock_uuid_field, PTP_UUID_LENGTH);
	ptpClock->parent_port_id = ptpClock->clock_port_id_field;
	ptpClock->parent_last_sync_sequence_number = 0;
	ptpClock->parent_followup_capable = ptpClock->clock_followup_capable;
	ptpClock->parent_external_timing = ptpClock->external_timing;
	ptpClock->parent_variance = ptpClock->clock_variance;
	ptpClock->grandmaster_communication_technology = ptpClock->clock_communication_technology;
	memcpy(ptpClock->grandmaster_uuid_field, ptpClock->clock_uuid_field, PTP_UUID_LENGTH);
	ptpClock->grandmaster_port_id_field = ptpClock->clock_port_id_field;
	ptpClock->grandmaster_stratum = ptpClock->clock_stratum;
	memcpy(ptpClock->grandmaster_identifier, ptpClock->clock_identifier, PTP_CODE_STRING_LENGTH);
	ptpClock->grandmaster_variance = ptpClock->clock_variance;
	ptpClock->grandmaster_preferred = ptpClock->preferred;
	ptpClock->grandmaster_is_boundary_clock = ptpClock->is_boundary_clock;
	ptpClock->grandmaster_sequence_number = ptpClock->last_sync_event_sequence_number;
}

/* see spec table 21 */
void 
s1(MsgHeader * header, MsgSync * sync, PtpClock * ptpClock)
{
	/* Current data set */
	ptpClock->steps_removed = sync->localStepsRemoved + 1;

	/* Parent data set */
	ptpClock->parent_communication_technology = header->sourceCommunicationTechnology;
	memcpy(ptpClock->parent_uuid, header->sourceUuid, PTP_UUID_LENGTH);
	ptpClock->parent_port_id = header->sourcePortId;
	ptpClock->parent_last_sync_sequence_number = header->sequenceId;
	ptpClock->parent_followup_capable = getFlag(header->flags, PTP_ASSIST);
	ptpClock->parent_external_timing = getFlag(header->flags, PTP_EXT_SYNC);
	ptpClock->parent_variance = sync->localClockVariance;
	ptpClock->grandmaster_communication_technology = sync->grandmasterCommunicationTechnology;
	memcpy(ptpClock->grandmaster_uuid_field, sync->grandmasterClockUuid, PTP_UUID_LENGTH);
	ptpClock->grandmaster_port_id_field = sync->grandmasterPortId;
	ptpClock->grandmaster_stratum = sync->grandmasterClockStratum;
	memcpy(ptpClock->grandmaster_identifier, sync->grandmasterClockIdentifier, PTP_CODE_STRING_LENGTH);
	ptpClock->grandmaster_variance = sync->grandmasterClockVariance;
	ptpClock->grandmaster_preferred = sync->grandmasterPreferred;
	ptpClock->grandmaster_is_boundary_clock = sync->grandmasterIsBoundaryClock;
	ptpClock->grandmaster_sequence_number = sync->grandmasterSequenceId;

	/* Global time properties data set */
	ptpClock->current_utc_offset = sync->currentUTCOffset;
	ptpClock->leap_59 = getFlag(header->flags, PTP_LI_59);
	ptpClock->leap_61 = getFlag(header->flags, PTP_LI_61);
	ptpClock->epoch_number = sync->epochNumber;
}

void 
copyD0(MsgHeader * header, MsgSync * sync, PtpClock * ptpClock)
{
	sync->grandmasterCommunicationTechnology = ptpClock->clock_communication_technology;
	memcpy(sync->grandmasterClockUuid, ptpClock->port_uuid_field, PTP_UUID_LENGTH);
	sync->grandmasterPortId = ptpClock->port_id_field;
	sync->grandmasterClockStratum = ptpClock->clock_stratum;
	memcpy(sync->grandmasterClockIdentifier, ptpClock->clock_identifier, PTP_CODE_STRING_LENGTH);
	sync->grandmasterClockVariance = ptpClock->clock_variance;
	sync->grandmasterIsBoundaryClock = ptpClock->is_boundary_clock;
	sync->grandmasterPreferred = ptpClock->preferred;
	sync->localStepsRemoved = ptpClock->steps_removed;
	header->sourceCommunicationTechnology = ptpClock->clock_communication_technology;
	memcpy(header->sourceUuid, ptpClock->port_uuid_field, PTP_UUID_LENGTH);
	header->sourcePortId = ptpClock->port_id_field;
	sync->grandmasterSequenceId = ptpClock->grandmaster_sequence_number;
	header->sequenceId = ptpClock->grandmaster_sequence_number;
}

int 
getIdentifierOrder(Octet identifier[PTP_CODE_STRING_LENGTH])
{
	if (!memcmp(identifier, IDENTIFIER_ATOM, PTP_CODE_STRING_LENGTH))
		return 1;
	else if (!memcmp(identifier, IDENTIFIER_GPS, PTP_CODE_STRING_LENGTH))
		return 1;
	else if (!memcmp(identifier, IDENTIFIER_NTP, PTP_CODE_STRING_LENGTH))
		return 2;
	else if (!memcmp(identifier, IDENTIFIER_HAND, PTP_CODE_STRING_LENGTH))
		return 3;
	else if (!memcmp(identifier, IDENTIFIER_INIT, PTP_CODE_STRING_LENGTH))
		return 4;
	else if (!memcmp(identifier, IDENTIFIER_DFLT, PTP_CODE_STRING_LENGTH))
		return 5;

	return 6;
}

/* return similar to memcmp()s
   note: communicationTechnology can be ignored because
   if they differed they would not have made it here */
Integer8 
bmcDataSetComparison(MsgHeader * headerA, MsgSync * syncA,
    MsgHeader * headerB, MsgSync * syncB, PtpClock * ptpClock)
{
	DBGV("bmcDataSetComparison: start\n");
	if (!(syncA->grandmasterPortId == syncB->grandmasterPortId
	    && !memcmp(syncA->grandmasterClockUuid, syncB->grandmasterClockUuid, PTP_UUID_LENGTH))) {
		if (syncA->grandmasterClockStratum < syncB->grandmasterClockStratum)
			goto A;
		else if (syncA->grandmasterClockStratum > syncB->grandmasterClockStratum)
			goto B;

		/* grandmasterClockStratums same */
		if (getIdentifierOrder(syncA->grandmasterClockIdentifier) < getIdentifierOrder(syncB->grandmasterClockIdentifier))
			goto A;
		if (getIdentifierOrder(syncA->grandmasterClockIdentifier) > getIdentifierOrder(syncB->grandmasterClockIdentifier))
			goto B;

		/* grandmasterClockIdentifiers same */
		if (syncA->grandmasterClockStratum > 2) {
			if (syncA->grandmasterClockVariance > syncB->grandmasterClockVariance + PTP_LOG_VARIANCE_THRESHOLD
			    || syncA->grandmasterClockVariance < syncB->grandmasterClockVariance - PTP_LOG_VARIANCE_THRESHOLD) {
				/* grandmasterClockVariances are not similar */
				if (syncA->grandmasterClockVariance < syncB->grandmasterClockVariance)
					goto A;
				else
					goto B;
			}
			/* grandmasterClockVariances are similar */
			if (!syncA->grandmasterIsBoundaryClock != !syncB->grandmasterIsBoundaryClock) {	/* XOR */
				if (syncA->grandmasterIsBoundaryClock)
					goto A;
				else
					goto B;
			}
			/* neither is grandmasterIsBoundaryClock */
			if (memcmp(syncA->grandmasterClockUuid, syncB->grandmasterClockUuid, PTP_UUID_LENGTH) < 0)
				goto A;
			else
				goto B;
		}
		/* syncA->grandmasterClockStratum <= 2 */
		if (!syncA->grandmasterPreferred != !syncB->grandmasterPreferred) {	/* XOR */
			if (syncA->grandmasterPreferred)
				return 1;	/* A1 */
			else
				return -1;	/* B1 */
		}
		/* neither or both grandmasterPreferred */
	}
	DBGV("bmcDataSetComparison: X\n");
	if (syncA->localStepsRemoved > syncB->localStepsRemoved + 1
	    || syncA->localStepsRemoved < syncB->localStepsRemoved - 1) {
		/* localStepsRemoved not within 1 */
		if (syncA->localStepsRemoved < syncB->localStepsRemoved)
			return 1;	/* A1 */
		else
			return -1;	/* B1 */
	}
	/* localStepsRemoved within 1 */
	if (syncA->localStepsRemoved < syncB->localStepsRemoved) {
		DBGV("bmcDataSetComparison: A3\n");
		if (memcmp(ptpClock->port_uuid_field, headerB->sourceUuid, PTP_UUID_LENGTH) < 0)
			return 1;	/* A1 */
		else if (memcmp(ptpClock->port_uuid_field, headerB->sourceUuid, PTP_UUID_LENGTH) > 0)
			return 2;	/* A2 */

		/* this port_uuid_field same as headerB->sourceUuid */
		if (ptpClock->port_id_field < headerB->sourcePortId)
			return 1;	/* A1 */
		else if (ptpClock->port_id_field > headerB->sourcePortId)
			return 2;	/* A2 */

		/* this port_id_field same as headerB->sourcePortId */
		return 0;		/* same */
	}
	if (syncA->localStepsRemoved > syncB->localStepsRemoved) {
		DBGV("bmcDataSetComparison: B3\n");
		if (memcmp(ptpClock->port_uuid_field, headerA->sourceUuid, PTP_UUID_LENGTH) < 0)
			return -1;	/* B1 */
		else if (memcmp(ptpClock->port_uuid_field, headerB->sourceUuid, PTP_UUID_LENGTH) > 0)
			return -2;	/* B2 */

		/* this port_uuid_field same as headerA->sourceUuid */
		if (ptpClock->port_id_field < headerA->sourcePortId)
			return -1;	/* B1 */
		else if (ptpClock->port_id_field > headerA->sourcePortId)
			return -2;	/* B2 */

		/* this port_id_field same as headerA->sourcePortId */
		return 0;		/* same */
	}
	/* localStepsRemoved same */
	if (memcmp(headerA->sourceUuid, headerB->sourceUuid, PTP_UUID_LENGTH) < 0)
		return 2;		/* A2 */
	else if (memcmp(headerA->sourceUuid, headerB->sourceUuid, PTP_UUID_LENGTH) > 0)
		return -2;		/* B2 */

	/* sourceUuid same */
	DBGV("bmcDataSetComparison: Z\n");
	if (syncA->grandmasterSequenceId > syncB->grandmasterSequenceId)
		return 3;
	else if (syncA->grandmasterSequenceId < syncB->grandmasterSequenceId)
		return -3;

	/* grandmasterSequenceId same */
	if (headerA->sequenceId > headerB->sequenceId)
		return 3;
	else if (headerA->sequenceId < headerB->sequenceId)
		return -3;

	/* sequenceId same */
	return 0;			/* same */

	/* oh no, a goto label! the horror! */
A:
	if (!syncA->grandmasterPreferred && syncB->grandmasterPreferred)
		return -1;		/* B1 */
	else
		return 1;		/* A1 */
B:
	if (syncA->grandmasterPreferred && !syncB->grandmasterPreferred)
		return 1;		/* A1 */
	else
		return -1;		/* B1 */
}

UInteger8 
bmcStateDecision(MsgHeader * header, MsgSync * sync, RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	if (rtOpts->slaveOnly) {
		s1(header, sync, ptpClock);
		return PTP_SLAVE;
	}
	copyD0(&ptpClock->msgTmpHeader, &ptpClock->msgTmp.sync, ptpClock);

	if (ptpClock->msgTmp.sync.grandmasterClockStratum < 3) {
		if (bmcDataSetComparison(&ptpClock->msgTmpHeader, &ptpClock->msgTmp.sync, header, sync, ptpClock) > 0) {
			m1(ptpClock);
			return PTP_MASTER;
		}
		s1(header, sync, ptpClock);
		return PTP_PASSIVE;
	} else if (bmcDataSetComparison(&ptpClock->msgTmpHeader, &ptpClock->msgTmp.sync, header, sync, ptpClock) > 0
	    && ptpClock->msgTmp.sync.grandmasterClockStratum != 255) {
		m1(ptpClock);
		return PTP_MASTER;
	} else {
		s1(header, sync, ptpClock);
		return PTP_SLAVE;
	}
}

UInteger8 
bmc(ForeignMasterRecord * foreign, RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	Integer16 i, best;

	if (!ptpClock->number_foreign_records) {
		if (ptpClock->port_state == PTP_MASTER)
			m1(ptpClock);
		return ptpClock->port_state;	/* no change */
	}
	for (i = 1, best = 0; i < ptpClock->number_foreign_records; ++i) {
		if (bmcDataSetComparison(&foreign[i].header, &foreign[i].sync,
		    &foreign[best].header, &foreign[best].sync, ptpClock) > 0)
			best = i;
	}

	DBGV("bmc: best record %d\n", best);
	ptpClock->foreign_record_best = best;

	return bmcStateDecision(&foreign[best].header, &foreign[best].sync, rtOpts, ptpClock);
}
