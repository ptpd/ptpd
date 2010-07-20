/**
 * @file   probe.c
 * @date   Tue Jul 20 16:13:30 2010
 * 
 * @brief  Management packet handling
 * 
 * 
 */

#include "ptpd.h"

#define KEY_ARRAY_LEN 6
UInteger8 management_key_array[KEY_ARRAY_LEN] =
{PTP_MM_OBTAIN_IDENTITY, PTP_MM_GET_DEFAULT_DATA_SET, PTP_MM_GET_CURRENT_DATA_SET,
PTP_MM_GET_PARENT_DATA_SET, PTP_MM_GET_PORT_DATA_SET, PTP_MM_GET_GLOBAL_TIME_DATA_SET};

void	displayHeader(MsgHeader *);
void	displayManagement(MsgHeader *, MsgManagement *);

void 
probe(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	UInteger16 i;
	UInteger16 length;
	TimeInternal interval, now, finish, timestamp;

	/* check */
	if (rtOpts->probe_management_key == PTP_MM_UPDATE_DEFAULT_DATA_SET
	    || rtOpts->probe_management_key == PTP_MM_UPDATE_GLOBAL_TIME_PROPERTIES
	    || rtOpts->probe_management_key == PTP_MM_SET_SYNC_INTERVAL) {
		ERROR("send not supported for that management message\n");
		return;
	}
	/* init */
	if (!netInit(&ptpClock->netPath, rtOpts, ptpClock)) {
		ERROR("failed to initialize network\n");
		return;
	}
	initData(rtOpts, ptpClock);
	msgPackHeader(ptpClock->msgObuf, ptpClock);

	memset(&ptpClock->msgTmp.manage, 0, sizeof(MsgManagement));
	ptpClock->msgTmp.manage.targetCommunicationTechnology = PTP_DEFAULT;

	/* send */
	for (i = 0; i < KEY_ARRAY_LEN; ++i) {
		if (rtOpts->probe_management_key > 0) {
			ptpClock->msgTmp.manage.managementMessageKey = rtOpts->probe_management_key;
			ptpClock->msgTmp.manage.recordKey = rtOpts->probe_record_key;
		} else
			ptpClock->msgTmp.manage.managementMessageKey = management_key_array[i];

		if (!(length = msgPackManagement(ptpClock->msgObuf, &ptpClock->msgTmp.manage, ptpClock))) {
			ERROR("failed to pack management message\n");
			return;
		}
		printf("\n(sending managementMessageKey %hhu)\n", ptpClock->msgTmp.manage.managementMessageKey);

		if (!netSendGeneral(ptpClock->msgObuf, length, &ptpClock->netPath)) {
			ERROR("failed to send message\n");
			return;
		}
		if (rtOpts->probe_management_key > 0)
			break;
	}

	getTime(&finish);
	finish.seconds += PTP_SYNC_INTERVAL_TIMEOUT(ptpClock->sync_interval);
	for (;;) {
		interval.seconds = PTP_SYNC_INTERVAL_TIMEOUT(ptpClock->sync_interval);
		interval.nanoseconds = 0;
		netSelect(&interval, &ptpClock->netPath);

		netRecvEvent(ptpClock->msgIbuf, &timestamp, &ptpClock->netPath);

		if (netRecvGeneral(ptpClock->msgIbuf, &ptpClock->netPath)) {
			msgUnpackHeader(ptpClock->msgIbuf, &ptpClock->msgTmpHeader);

			if (ptpClock->msgTmpHeader.control == PTP_MANAGEMENT_MESSAGE) {
				msgUnpackManagement(ptpClock->msgIbuf, &ptpClock->msgTmp.manage);
				msgUnpackManagementPayload(ptpClock->msgIbuf, &ptpClock->msgTmp.manage);

				displayManagement(&ptpClock->msgTmpHeader, &ptpClock->msgTmp.manage);
			}
			fflush(stdout);
		}
		getTime(&now);
		if (now.seconds > finish.seconds || (now.seconds == finish.seconds
		    && now.nanoseconds > finish.nanoseconds))
			break;
	}

	/* done */
	printf("\n");
	ptpdShutdown();

	exit(0);
}

void 
displayHeader(MsgHeader * header)
{
	printf(
	    "  sourceCommunicationTechnology %hhu\n"
	    "  sourceUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n"
	    "  sourcePortId %hu\n",
	    header->sourceCommunicationTechnology,
	    header->sourceUuid[0], header->sourceUuid[1], header->sourceUuid[2],
	    header->sourceUuid[3], header->sourceUuid[4], header->sourceUuid[5],
	    header->sourcePortId);
}

void 
displayManagement(MsgHeader * header, MsgManagement * manage)
{
	Integer16 i;

	switch (manage->managementMessageKey) {
	case PTP_MM_CLOCK_IDENTITY:
		printf("\n");
		displayHeader(header);
		printf("  managementMessageKey %hhu (PTP_MM_CLOCK_IDENTITY)\n", manage->managementMessageKey);
		printf("  clockCommunicationTechnology %hhu\n", manage->payload.clockIdentity.clockCommunicationTechnology);
		printf("  clockUuidField %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
		    manage->payload.clockIdentity.clockUuidField[0], manage->payload.clockIdentity.clockUuidField[1],
		    manage->payload.clockIdentity.clockUuidField[2], manage->payload.clockIdentity.clockUuidField[3],
		    manage->payload.clockIdentity.clockUuidField[4], manage->payload.clockIdentity.clockUuidField[5]);
		printf("  clockPortField %hu\n", manage->payload.clockIdentity.clockPortField);
		printf("  manufacturerIdentity ");
		for (i = 0; i < MANUFACTURER_ID_LENGTH && manage->payload.clockIdentity.manufacturerIdentity[i]; ++i)
			putchar(manage->payload.clockIdentity.manufacturerIdentity[i]);
		putchar('\n');
		break;

	case PTP_MM_DEFAULT_DATA_SET:
		printf("\n");
		displayHeader(header);
		printf("  managementMessageKey %hhu (PTP_MM_DEFAULT_DATA_SET)\n", manage->managementMessageKey);
		printf("  clockCommunicationTechnology %hhu\n", manage->payload.defaultData.clockCommunicationTechnology);
		printf("  clockUuidField %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
		    manage->payload.defaultData.clockUuidField[0], manage->payload.defaultData.clockUuidField[1],
		    manage->payload.defaultData.clockUuidField[2], manage->payload.defaultData.clockUuidField[3],
		    manage->payload.defaultData.clockUuidField[4], manage->payload.defaultData.clockUuidField[5]);
		printf("  clockPortField %hu\n", manage->payload.defaultData.clockPortField);
		printf("  clockStratum %hhu\n", manage->payload.defaultData.clockStratum);
		printf("  clockIdentifier ");
		for (i = 0; i < PTP_CODE_STRING_LENGTH && manage->payload.defaultData.clockIdentifier[i]; ++i)
			putchar(manage->payload.defaultData.clockIdentifier[i]);
		putchar('\n');
		printf("  clockVariance %hd\n", manage->payload.defaultData.clockVariance);
		printf("  clockFollowupCapable %hhu\n", manage->payload.defaultData.clockFollowupCapable);
		printf("  preferred %hhu\n", manage->payload.defaultData.preferred);
		printf("  initializable %hhu\n", manage->payload.defaultData.initializable);
		printf("  externalTiming %hhu\n", manage->payload.defaultData.externalTiming);
		printf("  isBoundaryClock %hhu\n", manage->payload.defaultData.isBoundaryClock);
		printf("  syncInterval %hhd\n", manage->payload.defaultData.syncInterval);
		printf("  subdomainName ");
		for (i = 0; i < PTP_SUBDOMAIN_NAME_LENGTH && manage->payload.defaultData.subdomainName[i]; ++i)
			putchar(manage->payload.defaultData.subdomainName[i]);
		putchar('\n');
		printf("  numberPorts %hu\n", manage->payload.defaultData.numberPorts);
		printf("  numberForeignRecords %hu\n", manage->payload.defaultData.numberForeignRecords);
		break;

	case PTP_MM_CURRENT_DATA_SET:
		printf("\n");
		displayHeader(header);
		printf("  managementMessageKey %hhu (PTP_MM_CURRENT_DATA_SET)\n", manage->managementMessageKey);
		printf("  stepsRemoved %hu\n", manage->payload.current.stepsRemoved);
		printf("  offsetFromMaster %s%u.%09d\n",
		    manage->payload.current.offsetFromMaster.nanoseconds & 0x80000000 ? "-" : "",
		    manage->payload.current.offsetFromMaster.seconds,
		    manage->payload.current.offsetFromMaster.nanoseconds & ~0x80000000);
		printf("  oneWayDelay %s%u.%09d\n",
		    manage->payload.current.oneWayDelay.nanoseconds & 0x80000000 ? "-" : "",
		    manage->payload.current.oneWayDelay.seconds,
		    manage->payload.current.oneWayDelay.nanoseconds & ~0x80000000);
		break;

	case PTP_MM_PARENT_DATA_SET:
		printf("\n");
		displayHeader(header);
		printf("  managementMessageKey %hhu (PTP_MM_PARENT_DATA_SET)\n", manage->managementMessageKey);
		printf("  parentCommunicationTechnology %hhu\n", manage->payload.parent.parentCommunicationTechnology);
		printf("  parentUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
		    manage->payload.parent.parentUuid[0], manage->payload.parent.parentUuid[1],
		    manage->payload.parent.parentUuid[2], manage->payload.parent.parentUuid[3],
		    manage->payload.parent.parentUuid[4], manage->payload.parent.parentUuid[5]);
		printf("  parentPortId %hu\n", manage->payload.parent.parentPortId);
		printf("  parentLastSyncSequenceNumber %hu\n", manage->payload.parent.parentLastSyncSequenceNumber);
		printf("  parentFollowupCapable %hhu\n", manage->payload.parent.parentFollowupCapable);
		printf("  parentExternalTiming %hhu\n", manage->payload.parent.parentExternalTiming);
		printf("  parentVariance %hd\n", manage->payload.parent.parentVariance);
		printf("  parentStats %hhu\n", manage->payload.parent.parentStats);
		printf("  observedVariance %hd\n", manage->payload.parent.observedVariance);
		printf("  observedDrift %d\n", manage->payload.parent.observedDrift);
		printf("  utcReasonable %hhu\n", manage->payload.parent.utcReasonable);
		printf("  grandmasterCommunicationTechnology %hhu\n", manage->payload.parent.grandmasterCommunicationTechnology);
		printf("  grandmasterUuidField %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
		    manage->payload.parent.grandmasterUuidField[0], manage->payload.parent.grandmasterUuidField[1],
		    manage->payload.parent.grandmasterUuidField[2], manage->payload.parent.grandmasterUuidField[3],
		    manage->payload.parent.grandmasterUuidField[4], manage->payload.parent.grandmasterUuidField[5]);
		printf("  grandmasterPortIdField %hu\n", manage->payload.parent.grandmasterPortIdField);
		printf("  grandmasterStratum %hhu\n", manage->payload.parent.grandmasterStratum);
		printf("  grandmasterIdentifier ");
		for (i = 0; i < PTP_CODE_STRING_LENGTH && manage->payload.parent.grandmasterIdentifier[i]; ++i)
			putchar(manage->payload.parent.grandmasterIdentifier[i]);
		putchar('\n');
		printf("  grandmasterVariance %hd\n", manage->payload.parent.grandmasterVariance);
		printf("  grandmasterPreferred %hhu\n", manage->payload.parent.grandmasterPreferred);
		printf("  grandmasterIsBoundaryClock %hhu\n", manage->payload.parent.grandmasterIsBoundaryClock);
		printf("  grandmasterSequenceNumber %hu\n", manage->payload.parent.grandmasterSequenceNumber);
		break;

	case PTP_MM_PORT_DATA_SET:
		printf("\n");
		displayHeader(header);
		printf("  managementMessageKey %hhu (PTP_MM_PORT_DATA_SET)\n", manage->managementMessageKey);
		printf("  returnedPortNumber %hu\n", manage->payload.port.returnedPortNumber);
		printf("  portState %hhu\n", manage->payload.port.portState);
		printf("  lastSyncEventSequenceNumber %hu\n", manage->payload.port.lastSyncEventSequenceNumber);
		printf("  lastGeneralEventSequenceNumber %hu\n", manage->payload.port.lastGeneralEventSequenceNumber);
		printf("  portCommunicationTechnology %hhu\n", manage->payload.port.portCommunicationTechnology);
		printf("  portUuidField %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
		    manage->payload.port.portUuidField[0], manage->payload.port.portUuidField[1],
		    manage->payload.port.portUuidField[2], manage->payload.port.portUuidField[3],
		    manage->payload.port.portUuidField[4], manage->payload.port.portUuidField[5]);
		printf("  portIdField %hu\n", manage->payload.port.portIdField);
		printf("  burstEnabled %hhu\n", manage->payload.port.burstEnabled);
		printf("  subdomainAddressOctets %hhu\n", manage->payload.port.subdomainAddressOctets);
		printf("  eventPortAddressOctets %hhu\n", manage->payload.port.eventPortAddressOctets);
		printf("  generalPortAddressOctets %hhu\n", manage->payload.port.generalPortAddressOctets);
		printf("  subdomainAddress ");
		printf("%hhu", manage->payload.port.subdomainAddress[0]);
		for (i = 1; i < SUBDOMAIN_ADDRESS_LENGTH; ++i)
			printf(".%hhu", manage->payload.port.subdomainAddress[i]);
		putchar('\n');
		printf("  eventPortAddress %hu\n", *(UInteger16 *) manage->payload.port.eventPortAddress);
		printf("  generalPortAddress %hu\n", *(UInteger16 *) manage->payload.port.generalPortAddress);
		break;

	case PTP_MM_GLOBAL_TIME_DATA_SET:
		printf("\n");
		displayHeader(header);
		printf("  managementMessageKey %hhu (PTP_MM_GLOBAL_TIME_DATA_SET)\n", manage->managementMessageKey);
		printf("  localTime %s%u.%09d\n",
		    manage->payload.globalTime.localTime.nanoseconds & 0x80000000 ? "-" : "",
		    manage->payload.globalTime.localTime.seconds,
		    manage->payload.globalTime.localTime.nanoseconds & ~0x80000000);
		printf("  currentUtcOffset %hd\n", manage->payload.globalTime.currentUtcOffset);
		printf("  leap59 %hhu\n", manage->payload.globalTime.leap59);
		printf("  leap61 %hhu\n", manage->payload.globalTime.leap61);
		printf("  epochNumber %hu\n", manage->payload.globalTime.epochNumber);
		break;


	case PTP_MM_FOREIGN_DATA_SET:
		printf("\n");
		displayHeader(header);
		printf("  managementMessageKey %hhu (PTP_MM_FOREIGN_DATA_SET)\n", manage->managementMessageKey);
		printf("  returnedPortNumber %hu\n", manage->payload.foreign.returnedPortNumber);
		printf("  returnedRecordNumber %hu\n", manage->payload.foreign.returnedRecordNumber);
		printf("  foreignMasterCommunicationTechnology %hu\n", manage->payload.foreign.foreignMasterCommunicationTechnology);
		printf("  foreignMasterUuid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
		    manage->payload.foreign.foreignMasterUuid[0], manage->payload.foreign.foreignMasterUuid[1],
		    manage->payload.foreign.foreignMasterUuid[2], manage->payload.foreign.foreignMasterUuid[3],
		    manage->payload.foreign.foreignMasterUuid[4], manage->payload.foreign.foreignMasterUuid[5]);

		printf("  foreignMasterPortId %hu\n", manage->payload.foreign.foreignMasterPortId);
		printf("  foreignMasterSyncs %hu\n", manage->payload.foreign.foreignMasterSyncs);
		break;

	case PTP_MM_NULL:
		printf("\n");
		displayHeader(header);
		printf("  managementMessageKey %hhu (NULL)\n", manage->managementMessageKey);
		break;

	default:
		break;
	}

	return;
}
