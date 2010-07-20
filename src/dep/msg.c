/**
 * @file   msg.c
 * @author George Neville-Neil <gnn@neville-neil.com>
 * @date   Tue Jul 20 16:17:05 2010
 * 
 * @brief  Functions to pack and unpack messages.
 * 
 * See spec annex d
 */


#include "../ptpd.h"

Boolean 
msgPeek(void *buf, ssize_t length)
{
	/* not imlpemented yet */
	return TRUE;
}

void 
msgUnpackHeader(void *buf, MsgHeader * header)
{
	header->versionPTP = flip16(*(UInteger16 *) (buf + 0));
	header->versionNetwork = flip16(*(UInteger16 *) (buf + 2));
	DBGV("msgUnpackHeader: versionPTP %d\n", header->versionPTP);
	DBGV("msgUnpackHeader: versionNetwork %d\n", header->versionNetwork);

	memcpy(header->subdomain, (buf + 4), 16);
	DBGV("msgUnpackHeader: subdomain %s\n", header->subdomain);

	header->messageType = *(UInteger8 *) (buf + 20);
	header->sourceCommunicationTechnology = *(UInteger8 *) (buf + 21);
	DBGV("msgUnpackHeader: messageType %d\n", header->messageType);
	DBGV("msgUnpackHeader: sourceCommunicationTechnology %d\n", header->sourceCommunicationTechnology);

	memcpy(header->sourceUuid, (buf + 22), 6);
	DBGV("msgUnpackHeader: sourceUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	    header->sourceUuid[0], header->sourceUuid[1], header->sourceUuid[2],
	    header->sourceUuid[3], header->sourceUuid[4], header->sourceUuid[5]);

	header->sourcePortId = flip16(*(UInteger16 *) (buf + 28));
	header->sequenceId = flip16(*(UInteger16 *) (buf + 30));
	DBGV("msgUnpackHeader: sourcePortId %d\n", header->sourcePortId);
	DBGV("msgUnpackHeader: sequenceId %d\n", header->sequenceId);

	header->control = *(UInteger8 *) (buf + 32);
	DBGV("msgUnpackHeader: control %d\n", header->control);

	memcpy(header->flags, (buf + 34), 2);
	DBGV("msgUnpackHeader: flags %02hhx %02hhx\n", header->flags[0], header->flags[1]);
}

void 
msgUnpackSync(void *buf, MsgSync * sync)
{
	sync->originTimestamp.seconds = flip32(*(UInteger32 *) (buf + 40));
	DBGV("msgUnpackSync: originTimestamp.seconds %u\n", sync->originTimestamp.seconds);
	sync->originTimestamp.nanoseconds = flip32(*(Integer32 *) (buf + 44));
	DBGV("msgUnpackSync: originTimestamp.nanoseconds %d\n", sync->originTimestamp.nanoseconds);
	sync->epochNumber = flip16(*(UInteger16 *) (buf + 48));
	DBGV("msgUnpackSync: epochNumber %d\n", sync->epochNumber);
	sync->currentUTCOffset = flip16(*(Integer16 *) (buf + 50));
	DBGV("msgUnpackSync: currentUTCOffset %d\n", sync->currentUTCOffset);
	sync->grandmasterCommunicationTechnology = *(UInteger8 *) (buf + 53);
	DBGV("msgUnpackSync: grandmasterCommunicationTechnology %d\n", sync->grandmasterCommunicationTechnology);
	memcpy(sync->grandmasterClockUuid, (buf + 54), 6);
	DBGV("msgUnpackSync: grandmasterClockUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	    sync->grandmasterClockUuid[0], sync->grandmasterClockUuid[1], sync->grandmasterClockUuid[2],
	    sync->grandmasterClockUuid[3], sync->grandmasterClockUuid[4], sync->grandmasterClockUuid[5]);
	sync->grandmasterPortId = flip16(*(UInteger16 *) (buf + 60));
	DBGV("msgUnpackSync: grandmasterPortId %d\n", sync->grandmasterPortId);
	sync->grandmasterSequenceId = flip16(*(UInteger16 *) (buf + 62));
	DBGV("msgUnpackSync: grandmasterSequenceId %d\n", sync->grandmasterSequenceId);
	sync->grandmasterClockStratum = *(UInteger8 *) (buf + 67);
	DBGV("msgUnpackSync: grandmasterClockStratum %d\n", sync->grandmasterClockStratum);
	memcpy(sync->grandmasterClockIdentifier, (buf + 68), 4);
	DBGV("msgUnpackSync: grandmasterClockIdentifier %c%c%c%c\n",
	    sync->grandmasterClockIdentifier[0], sync->grandmasterClockIdentifier[1],
	    sync->grandmasterClockIdentifier[2], sync->grandmasterClockIdentifier[3]);
	sync->grandmasterClockVariance = flip16(*(Integer16 *) (buf + 74));
	DBGV("msgUnpackSync: grandmasterClockVariance %d\n", sync->grandmasterClockVariance);
	sync->grandmasterPreferred = *(UInteger8 *) (buf + 77);
	DBGV("msgUnpackSync: grandmasterPreferred %d\n", sync->grandmasterPreferred);
	sync->grandmasterIsBoundaryClock = *(UInteger8 *) (buf + 79);
	DBGV("msgUnpackSync: grandmasterIsBoundaryClock %d\n", sync->grandmasterIsBoundaryClock);
	sync->syncInterval = *(Integer8 *) (buf + 83);
	DBGV("msgUnpackSync: syncInterval %d\n", sync->syncInterval);
	sync->localClockVariance = flip16(*(Integer16 *) (buf + 86));
	DBGV("msgUnpackSync: localClockVariance %d\n", sync->localClockVariance);
	sync->localStepsRemoved = flip16(*(UInteger16 *) (buf + 90));
	DBGV("msgUnpackSync: localStepsRemoved %d\n", sync->localStepsRemoved);
	sync->localClockStratum = *(UInteger8 *) (buf + 95);
	DBGV("msgUnpackSync: localClockStratum %d\n", sync->localClockStratum);
	memcpy(sync->localClockIdentifer, (buf + 96), PTP_CODE_STRING_LENGTH);
	DBGV("msgUnpackSync: localClockIdentifer %c%c%c%c\n",
	    sync->localClockIdentifer[0], sync->localClockIdentifer[1],
	    sync->localClockIdentifer[2], sync->localClockIdentifer[3]);
	sync->parentCommunicationTechnology = *(UInteger8 *) (buf + 101);
	DBGV("msgUnpackSync: parentCommunicationTechnology %d\n", sync->parentCommunicationTechnology);
	memcpy(sync->parentUuid, (buf + 102), PTP_UUID_LENGTH);
	DBGV("msgUnpackSync: parentUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	    sync->parentUuid[0], sync->parentUuid[1], sync->parentUuid[2],
	    sync->parentUuid[3], sync->parentUuid[4], sync->parentUuid[5]);
	sync->parentPortField = flip16(*(UInteger16 *) (buf + 110));
	DBGV("msgUnpackSync: parentPortField %d\n", sync->parentPortField);
	sync->estimatedMasterVariance = flip16(*(Integer16 *) (buf + 114));
	DBGV("msgUnpackSync: estimatedMasterVariance %d\n", sync->estimatedMasterVariance);
	sync->estimatedMasterDrift = flip32(*(Integer32 *) (buf + 116));
	DBGV("msgUnpackSync: estimatedMasterDrift %d\n", sync->estimatedMasterDrift);
	sync->utcReasonable = *(UInteger8 *) (buf + 123);
	DBGV("msgUnpackSync: utcReasonable %d\n", sync->utcReasonable);
}

void 
msgUnpackDelayReq(void *buf, MsgDelayReq * req)
{
}

void 
msgUnpackFollowUp(void *buf, MsgFollowUp * follow)
{
	follow->associatedSequenceId = flip16(*(UInteger16 *) (buf + 42));
	DBGV("msgUnpackFollowUp: associatedSequenceId %u\n", follow->associatedSequenceId);
	follow->preciseOriginTimestamp.seconds = flip32(*(UInteger32 *) (buf + 44));
	DBGV("msgUnpackFollowUp: preciseOriginTimestamp.seconds %u\n", follow->preciseOriginTimestamp.seconds);
	follow->preciseOriginTimestamp.nanoseconds = flip32(*(Integer32 *) (buf + 48));
	DBGV("msgUnpackFollowUp: preciseOriginTimestamp.nanoseconds %d\n", follow->preciseOriginTimestamp.nanoseconds);
}

void 
msgUnpackDelayResp(void *buf, MsgDelayResp * resp)
{
	resp->delayReceiptTimestamp.seconds = flip32(*(UInteger32 *) (buf + 40));
	DBGV("msgUnpackDelayResp: delayReceiptTimestamp.seconds %u\n", resp->delayReceiptTimestamp.seconds);
	resp->delayReceiptTimestamp.nanoseconds = flip32(*(Integer32 *) (buf + 44));
	DBGV("msgUnpackDelayResp: delayReceiptTimestamp.nanoseconds %d\n", resp->delayReceiptTimestamp.nanoseconds);
	resp->requestingSourceCommunicationTechnology = *(UInteger8 *) (buf + 49);
	DBGV("msgUnpackDelayResp: requestingSourceCommunicationTechnology %d\n", resp->requestingSourceCommunicationTechnology);
	memcpy(resp->requestingSourceUuid, (buf + 50), 6);
	DBGV("msgUnpackDelayResp: requestingSourceUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	    resp->requestingSourceUuid[0], resp->requestingSourceUuid[1], resp->requestingSourceUuid[2],
	    resp->requestingSourceUuid[3], resp->requestingSourceUuid[4], resp->requestingSourceUuid[5]);
	resp->requestingSourcePortId = flip16(*(UInteger16 *) (buf + 56));
	DBGV("msgUnpackDelayResp: requestingSourcePortId %d\n", resp->requestingSourcePortId);
	resp->requestingSourceSequenceId = flip16(*(UInteger16 *) (buf + 58));
	DBGV("msgUnpackDelayResp: requestingSourceSequenceId %d\n", resp->requestingSourceSequenceId);
}

void 
msgUnpackManagement(void *buf, MsgManagement * manage)
{
	manage->targetCommunicationTechnology = *(UInteger8 *) (buf + 41);
	DBGV("msgUnpackManagement: targetCommunicationTechnology %d\n", manage->targetCommunicationTechnology);
	memcpy(manage->targetUuid, (buf + 42), 6);
	DBGV("msgUnpackManagement: targetUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	    manage->targetUuid[0], manage->targetUuid[1], manage->targetUuid[2],
	    manage->targetUuid[3], manage->targetUuid[4], manage->targetUuid[5]);
	manage->targetPortId = flip16(*(UInteger16 *) (buf + 48));
	DBGV("msgUnpackManagement: targetPortId %d\n", manage->targetPortId);
	manage->startingBoundaryHops = flip16(*(Integer16 *) (buf + 50));
	DBGV("msgUnpackManagement: startingBoundaryHops %d\n", manage->startingBoundaryHops);
	manage->boundaryHops = flip16(*(Integer16 *) (buf + 52));
	DBGV("msgUnpackManagement: boundaryHops %d\n", manage->boundaryHops);
	manage->managementMessageKey = *(UInteger8 *) (buf + 55);
	DBGV("msgUnpackManagement: managementMessageKey %d\n", manage->managementMessageKey);
	manage->parameterLength = flip16(*(UInteger16 *) (buf + 58));
	DBGV("msgUnpackManagement: parameterLength %d\n", manage->parameterLength);

	if (manage->managementMessageKey == PTP_MM_GET_FOREIGN_DATA_SET)
		manage->recordKey = flip16(*(UInteger16 *) (buf + 62));
}

UInteger8 
msgUnloadManagement(void *buf, MsgManagement * manage,
    PtpClock * ptpClock, RunTimeOpts * rtOpts)
{
	TimeInternal internalTime;
	TimeRepresentation externalTime;

	switch (manage->managementMessageKey) {
	case PTP_MM_INITIALIZE_CLOCK:
		if (ptpClock->initializable)
			return PTP_INITIALIZING;
		break;

	case PTP_MM_GOTO_FAULTY_STATE:
		DBG("event FAULT_DETECTED (forced by management message)\n");
		return PTP_FAULTY;
		break;

	case PTP_MM_DISABLE_PORT:
		if (manage->targetPortId == 1) {
			DBG("event DESIGNATED_DISABLED\n");
			return PTP_DISABLED;
		}
		break;

	case PTP_MM_ENABLE_PORT:
		if (manage->targetPortId == 1) {
			DBG("event DESIGNATED_ENABLED\n");
			return PTP_INITIALIZING;
		}
		break;

	case PTP_MM_CLEAR_DESIGNATED_PREFERRED_MASTER:
		ptpClock->preferred = FALSE;
		break;

	case PTP_MM_SET_DESIGNATED_PREFERRED_MASTER:
		ptpClock->preferred = TRUE;
		break;

	case PTP_MM_DISABLE_BURST:
		break;

	case PTP_MM_ENABLE_BURST:
		break;

	case PTP_MM_SET_SYNC_INTERVAL:
		rtOpts->syncInterval = *(Integer8 *) (buf + 63);
		break;

	case PTP_MM_SET_SUBDOMAIN:
		memcpy(rtOpts->subdomainName, buf + 60, 16);
		DBG("set subdomain to %s\n", rtOpts->subdomainName);
		break;

	case PTP_MM_SET_TIME:
		externalTime.seconds = flip32(*(UInteger32 *) (buf + 60));
		externalTime.nanoseconds = flip32(*(Integer32 *) (buf + 64));
		toInternalTime(&internalTime, &externalTime, &ptpClock->halfEpoch);
		setTime(&internalTime);
		break;

	case PTP_MM_UPDATE_DEFAULT_DATA_SET:
		if (!rtOpts->slaveOnly)
			ptpClock->clock_stratum = *(UInteger8 *) (buf + 63);
		memcpy(ptpClock->clock_identifier, buf + 64, 4);
		ptpClock->clock_variance = flip16(*(Integer16 *) (buf + 70));
		ptpClock->preferred = *(UInteger8 *) (buf + 75);
		rtOpts->syncInterval = *(UInteger8 *) (buf + 79);
		memcpy(rtOpts->subdomainName, buf + 80, 16);
		break;

	case PTP_MM_UPDATE_GLOBAL_TIME_PROPERTIES:
		ptpClock->current_utc_offset = flip16(*(Integer16 *) (buf + 62));
		ptpClock->leap_59 = *(UInteger8 *) (buf + 67);
		ptpClock->leap_61 = *(UInteger8 *) (buf + 71);
		ptpClock->epoch_number = flip16(*(UInteger16 *) (buf + 74));
		break;

	default:
		break;
	}

	return ptpClock->port_state;
}

void 
msgUnpackManagementPayload(void *buf, MsgManagement * manage)
{
	switch (manage->managementMessageKey) {
		case PTP_MM_CLOCK_IDENTITY:
		DBGV("msgUnloadManagementPayload: managementMessageKey PTP_MM_CLOCK_IDENTITY\n");
		manage->payload.clockIdentity.clockCommunicationTechnology = *(UInteger8 *) (buf + 63);
		memcpy(manage->payload.clockIdentity.clockUuidField, buf + 64, PTP_UUID_LENGTH);
		manage->payload.clockIdentity.clockPortField = flip16(*(UInteger16 *) (buf + 74));
		memcpy(manage->payload.clockIdentity.manufacturerIdentity, buf + 76, MANUFACTURER_ID_LENGTH);
		break;

	case PTP_MM_DEFAULT_DATA_SET:
		DBGV("msgUnloadManagementPayload: managementMessageKey PTP_MM_DEFAULT_DATA_SET\n");
		manage->payload.defaultData.clockCommunicationTechnology = *(UInteger8 *) (buf + 63);
		memcpy(manage->payload.defaultData.clockUuidField, buf + 64, PTP_UUID_LENGTH);
		manage->payload.defaultData.clockPortField = flip16(*(UInteger16 *) (buf + 74));
		manage->payload.defaultData.clockStratum = *(UInteger16 *) (buf + 79);
		memcpy(manage->payload.defaultData.clockIdentifier, buf + 80, PTP_CODE_STRING_LENGTH);
		manage->payload.defaultData.clockVariance = flip16(*(UInteger16 *) (buf + 86));
		manage->payload.defaultData.clockFollowupCapable = *(UInteger8 *) (buf + 91);
		manage->payload.defaultData.preferred = *(UInteger8 *) (buf + 95);
		manage->payload.defaultData.initializable = *(UInteger8 *) (buf + 99);
		manage->payload.defaultData.externalTiming = *(UInteger8 *) (buf + 103);
		manage->payload.defaultData.isBoundaryClock = *(UInteger8 *) (buf + 107);
		manage->payload.defaultData.syncInterval = *(UInteger8 *) (buf + 111);
		memcpy(manage->payload.defaultData.subdomainName, buf + 112, PTP_SUBDOMAIN_NAME_LENGTH);
		manage->payload.defaultData.numberPorts = flip16(*(UInteger16 *) (buf + 130));
		manage->payload.defaultData.numberForeignRecords = flip16(*(UInteger16 *) (buf + 134));
		break;

	case PTP_MM_CURRENT_DATA_SET:
		DBGV("msgUnloadManagementPayload: managementMessageKey PTP_MM_CURRENT_DATA_SET\n");
		manage->payload.current.stepsRemoved = flip16(*(UInteger16 *) (buf + 62));
		manage->payload.current.offsetFromMaster.seconds = flip32(*(UInteger32 *) (buf + 64));
		manage->payload.current.offsetFromMaster.nanoseconds = flip32(*(UInteger32 *) (buf + 68));
		manage->payload.current.oneWayDelay.seconds = flip32(*(UInteger32 *) (buf + 72));
		manage->payload.current.oneWayDelay.nanoseconds = flip32(*(Integer32 *) (buf + 76));
		break;

	case PTP_MM_PARENT_DATA_SET:
		DBGV("msgUnloadManagementPayload: managementMessageKey PTP_MM_PORT_DATA_SET\n");
		manage->payload.parent.parentCommunicationTechnology = *(UInteger8 *) (buf + 63);
		memcpy(manage->payload.parent.parentUuid, buf + 64, PTP_UUID_LENGTH);
		manage->payload.parent.parentPortId = flip16(*(UInteger16 *) (buf + 74));
		manage->payload.parent.parentLastSyncSequenceNumber = flip16(*(UInteger16 *) (buf + 74));
		manage->payload.parent.parentFollowupCapable = *(UInteger8 *) (buf + 83);
		manage->payload.parent.parentExternalTiming = *(UInteger8 *) (buf + 87);
		manage->payload.parent.parentVariance = flip16(*(UInteger16 *) (buf + 90));
		manage->payload.parent.parentStats = *(UInteger8 *) (buf + 85);
		manage->payload.parent.observedVariance = flip16(*(Integer16 *) (buf + 98));
		manage->payload.parent.observedDrift = flip32(*(Integer32 *) (buf + 100));
		manage->payload.parent.utcReasonable = *(UInteger8 *) (buf + 107);
		manage->payload.parent.grandmasterCommunicationTechnology = *(UInteger8 *) (buf + 111);
		memcpy(manage->payload.parent.grandmasterUuidField, buf + 112, PTP_UUID_LENGTH);
		manage->payload.parent.grandmasterPortIdField = flip16(*(UInteger16 *) (buf + 122));
		manage->payload.parent.grandmasterStratum = *(UInteger8 *) (buf + 127);
		memcpy(manage->payload.parent.grandmasterIdentifier, buf + 128, PTP_CODE_STRING_LENGTH);
		manage->payload.parent.grandmasterVariance = flip16(*(Integer16 *) (buf + 134));
		manage->payload.parent.grandmasterPreferred = *(UInteger8 *) (buf + 139);
		manage->payload.parent.grandmasterIsBoundaryClock = *(UInteger8 *) (buf + 144);
		manage->payload.parent.grandmasterSequenceNumber = flip16(*(UInteger16 *) (buf + 146));
		break;

	case PTP_MM_PORT_DATA_SET:
		DBGV("msgUnloadManagementPayload: managementMessageKey PTP_MM_FOREIGN_DATA_SET\n");
		manage->payload.port.returnedPortNumber = flip16(*(UInteger16 *) (buf + 62));
		manage->payload.port.portState = *(UInteger8 *) (buf + 67);
		manage->payload.port.lastSyncEventSequenceNumber = flip16(*(UInteger16 *) (buf + 70));
		manage->payload.port.lastGeneralEventSequenceNumber = flip16(*(UInteger16 *) (buf + 74));
		manage->payload.port.portCommunicationTechnology = *(UInteger8 *) (buf + 79);
		memcpy(manage->payload.port.portUuidField, buf + 80, PTP_UUID_LENGTH);
		manage->payload.port.portIdField = flip16(*(UInteger16 *) (buf + 90));
		manage->payload.port.burstEnabled = *(UInteger8 *) (buf + 95);
		manage->payload.port.subdomainAddressOctets = *(UInteger8 *) (buf + 97);
		manage->payload.port.eventPortAddressOctets = *(UInteger8 *) (buf + 98);
		manage->payload.port.generalPortAddressOctets = *(UInteger8 *) (buf + 99);
		memcpy(manage->payload.port.subdomainAddress, buf + 100, SUBDOMAIN_ADDRESS_LENGTH);
		memcpy(manage->payload.port.eventPortAddress, buf + 106, PORT_ADDRESS_LENGTH);
		memcpy(manage->payload.port.generalPortAddress, buf + 110, PORT_ADDRESS_LENGTH);
		break;

	case PTP_MM_GLOBAL_TIME_DATA_SET:
		DBGV("msgUnloadManagementPayload: managementMessageKey PTP_MM_GLOBAL_TIME_DATA_SET\n");
		manage->payload.globalTime.localTime.seconds = flip32(*(UInteger32 *) (buf + 60));
		manage->payload.globalTime.localTime.nanoseconds = flip32(*(Integer32 *) (buf + 64));
		manage->payload.globalTime.currentUtcOffset = flip16(*(Integer16 *) (buf + 70));
		manage->payload.globalTime.leap59 = *(UInteger8 *) (buf + 75);
		manage->payload.globalTime.leap61 = *(UInteger8 *) (buf + 79);
		manage->payload.globalTime.epochNumber = flip16(*(UInteger16 *) (buf + 82));
		break;


	case PTP_MM_FOREIGN_DATA_SET:
		DBGV("msgUnloadManagementPayload: managementMessageKey PTP_MM_FOREIGN_DATA_SET\n");
		manage->payload.foreign.returnedPortNumber = flip16(*(UInteger16 *) (buf + 62));
		manage->payload.foreign.returnedRecordNumber = flip16(*(UInteger16 *) (buf + 68));
		manage->payload.foreign.foreignMasterCommunicationTechnology = *(UInteger8 *) (buf + 71);
		memcpy(manage->payload.foreign.foreignMasterUuid, buf + 72, PTP_UUID_LENGTH);
		manage->payload.foreign.foreignMasterPortId = flip16(*(UInteger16 *) (buf + 82));
		manage->payload.foreign.foreignMasterSyncs = flip16(*(UInteger16 *) (buf + 66));
		break;

	case PTP_MM_NULL:
		DBGV("msgUnloadManagementPayload: managementMessageKey NULL\n");
		break;

	default:
		DBGV("msgUnloadManagementPayload: managementMessageKey ?\n");
		break;
	}

	return;
}

void 
msgPackHeader(void *buf, PtpClock * ptpClock)
{
	*(Integer32 *) (buf + 0) = shift16(flip16(VERSION_PTP), 0) | shift16(flip16(VERSION_NETWORK), 1);
	memcpy((buf + 4), ptpClock->subdomain_name, 16);
	*(Integer32 *) (buf + 20) = shift8(ptpClock->port_communication_technology, 1);
	memcpy((buf + 22), ptpClock->port_uuid_field, 6);

	if (ptpClock->external_timing)
		setFlag((buf + 34), PTP_EXT_SYNC);
	if (ptpClock->clock_followup_capable)
		setFlag((buf + 34), PTP_ASSIST);
	if (ptpClock->is_boundary_clock)
		setFlag((buf + 34), PTP_BOUNDARY_CLOCK);
}

void 
msgPackSync(void *buf, Boolean burst,
    TimeRepresentation * originTimestamp, PtpClock * ptpClock)
{
	*(UInteger8 *) (buf + 20) = 1;	/* messageType */
	*(Integer32 *) (buf + 28) = shift16(flip16(ptpClock->port_id_field), 0) | shift16(flip16(ptpClock->last_sync_event_sequence_number), 1);
	*(UInteger8 *) (buf + 32) = PTP_SYNC_MESSAGE;	/* control */
	if (ptpClock->burst_enabled && burst)
		setFlag((buf + 34), PTP_SYNC_BURST);
	else
		clearFlag((buf + 34), PTP_SYNC_BURST);
	if (ptpClock->parent_stats)
		setFlag((buf + 34), PARENT_STATS);
	else
		clearFlag((buf + 34), PARENT_STATS);


	*(Integer32 *) (buf + 40) = flip32(originTimestamp->seconds);
	*(Integer32 *) (buf + 44) = flip32(originTimestamp->nanoseconds);
	*(Integer32 *) (buf + 48) = shift16(flip16(ptpClock->epoch_number), 0) | shift16(flip16(ptpClock->current_utc_offset), 1);
	*(Integer32 *) (buf + 52) = shift8(ptpClock->grandmaster_communication_technology, 1);
	memcpy((buf + 54), ptpClock->grandmaster_uuid_field, 6);
	*(Integer32 *) (buf + 60) = shift16(flip16(ptpClock->grandmaster_port_id_field), 0) | shift16(flip16(ptpClock->grandmaster_sequence_number), 1);
	*(Integer32 *) (buf + 64) = shift8(ptpClock->grandmaster_stratum, 3);
	memcpy((buf + 68), ptpClock->grandmaster_identifier, 4);
	*(Integer32 *) (buf + 72) = shift16(flip16(ptpClock->grandmaster_variance), 1);
	*(Integer32 *) (buf + 76) = shift16(flip16(ptpClock->grandmaster_preferred), 0) | shift16(flip16(ptpClock->grandmaster_is_boundary_clock), 1);
	*(Integer32 *) (buf + 80) = shift16(flip16(ptpClock->sync_interval), 1);
	*(Integer32 *) (buf + 84) = shift16(flip16(ptpClock->clock_variance), 1);
	*(Integer32 *) (buf + 88) = shift16(flip16(ptpClock->steps_removed), 1);
	*(Integer32 *) (buf + 92) = shift8(ptpClock->clock_stratum, 3);
	memcpy((buf + 96), ptpClock->clock_identifier, 4);
	*(Integer32 *) (buf + 100) = shift8(ptpClock->parent_communication_technology, 1);
	memcpy((buf + 102), ptpClock->parent_uuid, 6);
	*(Integer32 *) (buf + 108) = shift16(flip16(ptpClock->parent_port_id), 1);
	*(Integer32 *) (buf + 112) = shift16(flip16(ptpClock->observed_variance), 1);
	*(Integer32 *) (buf + 116) = flip32(ptpClock->observed_drift);
	*(Integer32 *) (buf + 120) = shift8(ptpClock->utc_reasonable, 3);
}

void 
msgPackDelayReq(void *buf, Boolean burst,
    TimeRepresentation * originTimestamp, PtpClock * ptpClock)
{
	*(UInteger8 *) (buf + 20) = 1;	/* messageType */
	*(Integer32 *) (buf + 28) = shift16(flip16(ptpClock->port_id_field), 0) | shift16(flip16(ptpClock->last_sync_event_sequence_number), 1);
	*(UInteger8 *) (buf + 32) = PTP_DELAY_REQ_MESSAGE;	/* control */
	if (ptpClock->burst_enabled && burst)
		setFlag((buf + 34), PTP_SYNC_BURST);
	else
		clearFlag((buf + 34), PTP_SYNC_BURST);
	if (ptpClock->parent_stats)
		setFlag((buf + 34), PARENT_STATS);
	else
		clearFlag((buf + 34), PARENT_STATS);

	*(Integer32 *) (buf + 40) = flip32(originTimestamp->seconds);
	*(Integer32 *) (buf + 44) = flip32(originTimestamp->nanoseconds);
	*(Integer32 *) (buf + 48) = shift16(flip16(ptpClock->epoch_number), 0) | shift16(flip16(ptpClock->current_utc_offset), 1);
	*(Integer32 *) (buf + 52) = shift8(ptpClock->grandmaster_communication_technology, 1);
	memcpy((buf + 54), ptpClock->grandmaster_uuid_field, 6);
	*(Integer32 *) (buf + 60) = shift16(flip16(ptpClock->grandmaster_port_id_field), 0) | shift16(flip16(ptpClock->grandmaster_sequence_number), 1);
	*(Integer32 *) (buf + 64) = shift8(ptpClock->grandmaster_stratum, 3);
	memcpy((buf + 68), ptpClock->grandmaster_identifier, 4);
	*(Integer32 *) (buf + 72) = shift16(flip16(ptpClock->grandmaster_variance), 1);
	*(Integer32 *) (buf + 76) = shift16(flip16(ptpClock->grandmaster_preferred), 0) | shift16(flip16(ptpClock->grandmaster_is_boundary_clock), 1);
	*(Integer32 *) (buf + 80) = shift16(flip16(ptpClock->sync_interval), 1);
	*(Integer32 *) (buf + 84) = shift16(flip16(ptpClock->clock_variance), 1);
	*(Integer32 *) (buf + 88) = shift16(flip16(ptpClock->steps_removed), 1);
	*(Integer32 *) (buf + 92) = shift8(ptpClock->clock_stratum, 3);
	memcpy((buf + 96), ptpClock->clock_identifier, 4);
	*(Integer32 *) (buf + 100) = shift8(ptpClock->parent_communication_technology, 1);
	memcpy((buf + 102), ptpClock->parent_uuid, 6);
	*(Integer32 *) (buf + 108) = shift16(flip16(ptpClock->parent_port_id), 1);
	*(Integer32 *) (buf + 112) = shift16(flip16(ptpClock->observed_variance), 1);
	*(Integer32 *) (buf + 116) = flip32(ptpClock->observed_drift);
	*(Integer32 *) (buf + 120) = shift8(ptpClock->utc_reasonable, 3);
}

void 
msgPackFollowUp(void *buf, UInteger16 associatedSequenceId,
    TimeRepresentation * preciseOriginTimestamp, PtpClock * ptpClock)
{
	*(UInteger8 *) (buf + 20) = 2;	/* messageType */
	*(Integer32 *) (buf + 28) = shift16(flip16(ptpClock->port_id_field), 0) | shift16(flip16(ptpClock->last_general_event_sequence_number), 1);
	*(UInteger8 *) (buf + 32) = PTP_FOLLOWUP_MESSAGE;	/* control */
	clearFlag((buf + 34), PTP_SYNC_BURST);
	clearFlag((buf + 34), PARENT_STATS);

	*(Integer32 *) (buf + 40) = shift16(flip16(associatedSequenceId), 1);
	*(Integer32 *) (buf + 44) = flip32(preciseOriginTimestamp->seconds);
	*(Integer32 *) (buf + 48) = flip32(preciseOriginTimestamp->nanoseconds);
}

void 
msgPackDelayResp(void *buf, MsgHeader * header,
    TimeRepresentation * delayReceiptTimestamp, PtpClock * ptpClock)
{
	*(UInteger8 *) (buf + 20) = 2;	/* messageType */
	*(Integer32 *) (buf + 28) = shift16(flip16(ptpClock->port_id_field), 0) | shift16(flip16(ptpClock->last_general_event_sequence_number), 1);
	*(UInteger8 *) (buf + 32) = PTP_DELAY_RESP_MESSAGE;	/* control */
	clearFlag((buf + 34), PTP_SYNC_BURST);
	clearFlag((buf + 34), PARENT_STATS);

	*(Integer32 *) (buf + 40) = flip32(delayReceiptTimestamp->seconds);
	*(Integer32 *) (buf + 44) = flip32(delayReceiptTimestamp->nanoseconds);
	*(Integer32 *) (buf + 48) = shift8(header->sourceCommunicationTechnology, 1);
	memcpy(buf + 50, header->sourceUuid, 6);
	*(Integer32 *) (buf + 56) = shift16(flip16(header->sourcePortId), 0) | shift16(flip16(header->sequenceId), 1);
}

UInteger16 
msgPackManagement(void *buf, MsgManagement * manage, PtpClock * ptpClock)
{
	*(UInteger8 *) (buf + 20) = 2;	/* messageType */
	*(Integer32 *) (buf + 28) = shift16(flip16(ptpClock->port_id_field), 0) | shift16(flip16(ptpClock->last_general_event_sequence_number), 1);
	*(UInteger8 *) (buf + 32) = PTP_MANAGEMENT_MESSAGE;	/* control */
	clearFlag((buf + 34), PTP_SYNC_BURST);
	clearFlag((buf + 34), PARENT_STATS);
	*(Integer32 *) (buf + 40) = shift8(manage->targetCommunicationTechnology, 1);
	memcpy(buf + 42, manage->targetUuid, 6);
	*(Integer32 *) (buf + 48) = shift16(flip16(manage->targetPortId), 0) | shift16(flip16(MM_STARTING_BOUNDARY_HOPS), 1);
	*(Integer32 *) (buf + 52) = shift16(flip16(MM_STARTING_BOUNDARY_HOPS), 0);

	*(UInteger8 *) (buf + 55) = manage->managementMessageKey;

	switch (manage->managementMessageKey) {
	case PTP_MM_GET_FOREIGN_DATA_SET:
		*(UInteger16 *) (buf + 62) = manage->recordKey;
		*(Integer32 *) (buf + 56) = shift16(flip16(4), 1);
		return 64;

	default:
		*(Integer32 *) (buf + 56) = shift16(flip16(0), 1);
		return 60;
	}
}

UInteger16 
msgPackManagementResponse(void *buf, MsgHeader * header, MsgManagement * manage, PtpClock * ptpClock)
{
	TimeInternal internalTime;
	TimeRepresentation externalTime;

	*(UInteger8 *) (buf + 20) = 2;	/* messageType */
	*(Integer32 *) (buf + 28) = shift16(flip16(ptpClock->port_id_field), 0) | shift16(flip16(ptpClock->last_general_event_sequence_number), 1);
	*(UInteger8 *) (buf + 32) = PTP_MANAGEMENT_MESSAGE;	/* control */
	clearFlag((buf + 34), PTP_SYNC_BURST);
	clearFlag((buf + 34), PARENT_STATS);
	*(Integer32 *) (buf + 40) = shift8(header->sourceCommunicationTechnology, 1);
	memcpy(buf + 42, header->sourceUuid, 6);
	*(Integer32 *) (buf + 48) = shift16(flip16(header->sourcePortId), 0) | shift16(flip16(MM_STARTING_BOUNDARY_HOPS), 1);
	*(Integer32 *) (buf + 52) = shift16(flip16(manage->startingBoundaryHops - manage->boundaryHops + 1), 0);

	switch (manage->managementMessageKey) {
	case PTP_MM_OBTAIN_IDENTITY:
		*(UInteger8 *) (buf + 55) = PTP_MM_CLOCK_IDENTITY;
		*(Integer32 *) (buf + 56) = shift16(flip16(64), 1);
		*(Integer32 *) (buf + 60) = shift8(ptpClock->clock_communication_technology, 3);
		memcpy(buf + 64, ptpClock->clock_uuid_field, 6);
		*(Integer32 *) (buf + 72) = shift16(flip16(ptpClock->clock_port_id_field), 1);
		memcpy((buf + 76), MANUFACTURER_ID, 48);
		return 124;

	case PTP_MM_GET_DEFAULT_DATA_SET:
		*(UInteger8 *) (buf + 55) = PTP_MM_DEFAULT_DATA_SET;
		*(Integer32 *) (buf + 56) = shift16(flip16(76), 1);
		*(Integer32 *) (buf + 60) = shift8(ptpClock->clock_communication_technology, 3);
		memcpy(buf + 64, ptpClock->clock_uuid_field, 6);
		*(Integer32 *) (buf + 72) = shift16(flip16(ptpClock->clock_port_id_field), 1);
		*(Integer32 *) (buf + 76) = shift8(ptpClock->clock_stratum, 3);
		memcpy(buf + 80, ptpClock->clock_identifier, 4);
		*(Integer32 *) (buf + 84) = shift16(flip16(ptpClock->clock_variance), 1);
		*(Integer32 *) (buf + 88) = shift8(ptpClock->clock_followup_capable, 3);
		*(Integer32 *) (buf + 92) = shift8(ptpClock->preferred, 3);
		*(Integer32 *) (buf + 96) = shift8(ptpClock->initializable, 3);
		*(Integer32 *) (buf + 100) = shift8(ptpClock->external_timing, 3);
		*(Integer32 *) (buf + 104) = shift8(ptpClock->is_boundary_clock, 3);
		*(Integer32 *) (buf + 108) = shift8(ptpClock->sync_interval, 3);
		memcpy(buf + 112, ptpClock->subdomain_name, 16);
		*(Integer32 *) (buf + 128) = shift16(flip16(ptpClock->number_ports), 1);
		*(Integer32 *) (buf + 132) = shift16(flip16(ptpClock->number_foreign_records), 1);
		return 136;

	case PTP_MM_GET_CURRENT_DATA_SET:
		*(UInteger8 *) (buf + 55) = PTP_MM_CURRENT_DATA_SET;
		*(Integer32 *) (buf + 56) = shift16(flip16(20), 1);
		*(Integer32 *) (buf + 60) = shift16(flip16(ptpClock->steps_removed), 1);

		fromInternalTime(&ptpClock->offset_from_master, &externalTime, 0);
		*(Integer32 *) (buf + 64) = flip32(externalTime.seconds);
		*(Integer32 *) (buf + 68) = flip32(externalTime.nanoseconds);

		fromInternalTime(&ptpClock->one_way_delay, &externalTime, 0);
		*(Integer32 *) (buf + 72) = flip32(externalTime.seconds);
		*(Integer32 *) (buf + 76) = flip32(externalTime.nanoseconds);
		return 80;

	case PTP_MM_GET_PARENT_DATA_SET:
		*(UInteger8 *) (buf + 55) = PTP_MM_PARENT_DATA_SET;
		*(Integer32 *) (buf + 56) = shift16(flip16(90), 1);
		*(Integer32 *) (buf + 60) = shift8(ptpClock->parent_communication_technology, 3);
		memcpy(buf + 64, ptpClock->parent_uuid, 6);
		*(Integer32 *) (buf + 72) = shift16(flip16(ptpClock->parent_port_id), 1);
		*(Integer32 *) (buf + 76) = shift16(flip16(ptpClock->parent_last_sync_sequence_number), 1);
		*(Integer32 *) (buf + 80) = shift8(ptpClock->parent_followup_capable, 1);
		*(Integer32 *) (buf + 84) = shift8(ptpClock->parent_external_timing, 3);
		*(Integer32 *) (buf + 88) = shift16(flip16(ptpClock->parent_variance), 1);
		*(Integer32 *) (buf + 92) = shift8(ptpClock->parent_stats, 3);
		*(Integer32 *) (buf + 96) = shift16(flip16(ptpClock->observed_variance), 1);
		*(Integer32 *) (buf + 100) = flip32(ptpClock->observed_drift);
		*(Integer32 *) (buf + 104) = shift8(ptpClock->utc_reasonable, 3);
		*(Integer32 *) (buf + 108) = shift8(ptpClock->grandmaster_communication_technology, 3);
		memcpy(buf + 112, ptpClock->grandmaster_uuid_field, 6);
		*(Integer32 *) (buf + 120) = shift16(flip16(ptpClock->grandmaster_port_id_field), 1);
		*(Integer32 *) (buf + 124) = shift8(ptpClock->grandmaster_stratum, 3);
		memcpy(buf + 128, ptpClock->grandmaster_identifier, 4);
		*(Integer32 *) (buf + 132) = shift16(flip16(ptpClock->grandmaster_variance), 1);
		*(Integer32 *) (buf + 136) = shift8(ptpClock->grandmaster_preferred, 3);
		*(Integer32 *) (buf + 140) = shift8(ptpClock->grandmaster_is_boundary_clock, 3);
		*(Integer32 *) (buf + 144) = shift16(flip16(ptpClock->grandmaster_sequence_number), 1);
		return 148;

	case PTP_MM_GET_PORT_DATA_SET:
		if (manage->targetPortId && manage->targetPortId != ptpClock->port_id_field) {
			*(UInteger8 *) (buf + 55) = PTP_MM_NULL;
			*(Integer32 *) (buf + 56) = shift16(flip16(0), 1);
			return 0;
		}
		*(UInteger8 *) (buf + 55) = PTP_MM_PORT_DATA_SET;
		*(Integer32 *) (buf + 56) = shift16(flip16(52), 1);
		*(Integer32 *) (buf + 60) = shift16(flip16(ptpClock->port_id_field), 1);
		*(Integer32 *) (buf + 64) = shift8(ptpClock->port_state, 3);
		*(Integer32 *) (buf + 68) = shift16(flip16(ptpClock->last_sync_event_sequence_number), 1);
		*(Integer32 *) (buf + 72) = shift16(flip16(ptpClock->last_general_event_sequence_number), 1);
		*(Integer32 *) (buf + 76) = shift8(ptpClock->port_communication_technology, 3);
		memcpy(buf + 80, ptpClock->port_uuid_field, 6);
		*(Integer32 *) (buf + 88) = shift16(flip16(ptpClock->port_id_field), 1);
		*(Integer32 *) (buf + 92) = shift8(ptpClock->burst_enabled, 3);
		*(Integer32 *) (buf + 96) = shift8(4, 1) | shift8(2, 2) | shift8(2, 3);
		memcpy(buf + 100, ptpClock->subdomain_address, 4);
		memcpy(buf + 106, ptpClock->event_port_address, 2);
		memcpy(buf + 110, ptpClock->general_port_address, 2);
		return 112;

	case PTP_MM_GET_GLOBAL_TIME_DATA_SET:
		*(UInteger8 *) (buf + 55) = PTP_MM_GLOBAL_TIME_DATA_SET;
		*(Integer32 *) (buf + 56) = shift16(flip16(24), 1);

		getTime(&internalTime);
		fromInternalTime(&internalTime, &externalTime, ptpClock->halfEpoch);
		*(Integer32 *) (buf + 60) = flip32(externalTime.seconds);
		*(Integer32 *) (buf + 64) = flip32(externalTime.nanoseconds);

		*(Integer32 *) (buf + 68) = shift16(flip16(ptpClock->current_utc_offset), 1);
		*(Integer32 *) (buf + 72) = shift8(ptpClock->leap_59, 3);
		*(Integer32 *) (buf + 76) = shift8(ptpClock->leap_61, 3);
		*(Integer32 *) (buf + 80) = shift16(flip16(ptpClock->epoch_number), 1);
		return 84;

	case PTP_MM_GET_FOREIGN_DATA_SET:
		if ((manage->targetPortId && manage->targetPortId != ptpClock->port_id_field)
		    || !manage->recordKey || manage->recordKey > ptpClock->number_foreign_records) {
			*(UInteger8 *) (buf + 55) = PTP_MM_NULL;
			*(Integer32 *) (buf + 56) = shift16(flip16(0), 1);
			return 0;
		}
		*(UInteger8 *) (buf + 55) = PTP_MM_FOREIGN_DATA_SET;
		*(Integer32 *) (buf + 56) = shift16(flip16(28), 1);
		*(Integer32 *) (buf + 60) = shift16(flip16(ptpClock->port_id_field), 1);
		*(Integer32 *) (buf + 64) = shift16(flip16(manage->recordKey - 1), 1);
		*(Integer32 *) (buf + 68) = shift8(ptpClock->foreign[manage->recordKey - 1].foreign_master_communication_technology, 3);
		memcpy(buf + 72, ptpClock->foreign[manage->recordKey - 1].foreign_master_uuid, 6);
		*(Integer32 *) (buf + 80) = shift16(flip16(ptpClock->foreign[manage->recordKey - 1].foreign_master_port_id), 1);
		*(Integer32 *) (buf + 84) = shift16(flip16(ptpClock->foreign[manage->recordKey - 1].foreign_master_syncs), 1);
		return 88;

	default:
		return 0;
	}
}
