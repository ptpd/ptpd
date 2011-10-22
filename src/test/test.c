/**
 *
 * This runs unit test cases against ptpd functions.
 * Build it with 'make' in the src/test directory.
 *
 */

#include "../ptpd.h"
#include "init.h"
#include "helper.h"

#define TEST(test) \
	passCount++; \
	total++; \
	if((rc = test)) { \
		printf(#test " failed, %d\n", rc); \
		passCount--; \
	}

RunTimeOpts rtOpts;			/* statically allocated run-time
					 * configuration data */
PtpClock *G_ptpClock = NULL;

/* macro unit tests */
int test_SetField();
int test_IsSet();

/* complex data types unit tests */
int test_UInteger48();
int test_Integer64();

/* derived data types unit tests */
int test_ClockIdentity();
int test_copyClockIdentity();
int test_ClockQuality();
int test_TimeInterval();
int test_PortIdentity();
int test_PhysicalAddress();
int test_PortAddress();
int test_PTPText();
int test_Timestamp();

/* management TLV unit tests */
int test_ClockDescription();
int test_UserDescription();
int test_Initialize();
int test_DefaultDataSet();
int test_Priority1();
int test_Priority2();
int test_Domain();
int test_SlaveOnly();

/* handle functions for management TLV unit tests */
int test_handleMMNullManagement();
int test_handleMMClockDescription();
int test_handleMMUserDescription();
int test_handleMMEnablePort();
int test_handleMMSaveInNonVolatileStorage();
int test_handleMMTimePropertiesDataSet();
int test_handleMMUtcProperties();
int test_handleMMTraceabilityProperties();
int test_handleErrorManagementMessage();


int
main(int argc, char **argv)
{
	/* counters used in TEST macro */
	int passCount = 0;
	int total = 0;
	int rc = 0;

	/* test macros */
	TEST(test_SetField());
	TEST(test_IsSet());

	/* test packing/unpacking complex data types */
	TEST(test_UInteger48());
	TEST(test_Integer64());

	/* test packing/unpacking derived data types */
	TEST(test_ClockIdentity());
	TEST(test_copyClockIdentity());
	TEST(test_ClockQuality());
	TEST(test_TimeInterval());
	TEST(test_PortIdentity());
	TEST(test_PhysicalAddress());
	TEST(test_PortAddress());
	TEST(test_PTPText());
	TEST(test_Timestamp());

	/* test packing/unpacking management TLVs */
	TEST(test_ClockDescription());
	TEST(test_UserDescription());
	TEST(test_Initialize());
	TEST(test_DefaultDataSet());
	TEST(test_Priority1());
	TEST(test_Priority2());
	TEST(test_Domain());
	TEST(test_SlaveOnly());

	/* test handle management TLV functions */
	TEST(test_handleMMNullManagement());
	TEST(test_handleMMClockDescription());
	TEST(test_handleMMUserDescription());
	TEST(test_handleMMEnablePort());
	TEST(test_handleMMSaveInNonVolatileStorage());
	TEST(test_handleMMTimePropertiesDataSet());
	TEST(test_handleMMUtcProperties());
	TEST(test_handleMMTraceabilityProperties());
	TEST(test_handleErrorManagementMessage());

	if(passCount != total) {
		printf("%d tests ran, %d failed\n", total, total - passCount);
		return -1;
	}

	printf("%d tests ran, all passed\n", total);

	return 0;
}

int
test_SetField()
{
	Octet result;
	int rc = 0;
	Boolean a = TRUE;
	result = SET_FIELD(a, 0x0);
	if(result != 0x1) {
		rc += 1;
	}
	result = SET_FIELD(a, 0x2);
	if(result != 0x4) {
		rc += 1;
	}
	result = SET_FIELD(a, 0x5);
	if(result != 0x20) {
		rc += 1;
	}

	return rc;
}

int
test_IsSet()
{
	Boolean result;
	Octet a;
	int rc = 0;

	a = 0x01;
	result = IS_SET(a, 0x0);
	if(!result)
		rc += 1;
	a = 0x20;
	result = IS_SET(a, 0x5);
	if(!result)
		rc += 1;
	a = 0x4D;
	result = IS_SET(a, 0x3);
	if(!result)
		rc += 1;

	return rc;
}

int
test_ClockIdentity()
{
	ClockIdentity a = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
	ClockIdentity b = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};
	Octet msgBuf[PACKET_SIZE];

	/* init data */
	memset(msgBuf, 0, PACKET_SIZE);

	packClockIdentity(&a, msgBuf);
	unpackClockIdentity(msgBuf, &b);

	/* check dest */
	int i;
	for(i = 0; i < CLOCK_IDENTITY_LENGTH; i++) {
		if(a[i] != b[i])
			return 1;
	}

	return 0;
}

int
test_copyClockIdentity()
{
	ClockIdentity src = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
	ClockIdentity dest = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};

	copyClockIdentity(dest, src);

	/* check dest */
	int i;
	for(i = 0; i < CLOCK_IDENTITY_LENGTH; i++) {
		if(dest[i] != src[i])
			return 1;
	}

	return 0;
}

/* packing/unpacking UInteger48 */
int
test_UInteger48()
{
	UInteger48 a;
	UInteger48 b;
	Octet msgBuf[PACKET_SIZE];

	/* init data */
	memset(msgBuf, 0, PACKET_SIZE);
	a.lsb = 0x12345678;
	a.msb = 0xABCD;
	b.lsb = 0x0;
	b.msb = 0x0;

	packUInteger48(&a, msgBuf);
	unpackUInteger48(msgBuf, &b);

	/* check */
	if(a.lsb != b.lsb || a.msb != b.msb)
		return 1;

	return 0;
}

/* packing/unpacking Integer64 */
int
test_Integer64()
{
	Integer64 a;
	Integer64 b;
	Octet msgBuf[PACKET_SIZE];

	memset(msgBuf, 0, PACKET_SIZE);
	a.lsb = 0x01234567;
	a.msb = 0xFEDCBA98;
	b.lsb = 0x0;
	b.msb = 0x0;

	packInteger64(&a, msgBuf);
	unpackInteger64(msgBuf, &b);

	/* check */
	if(a.lsb != b.lsb || a.msb != b.msb)
		return 1;

	return 0;
}

/* packing/unpacking ClockQuality */
int
test_ClockQuality()
{
	ClockQuality a;
	ClockQuality b;
	Octet msgBuf[PACKET_SIZE];

	/* init */
	a.clockClass = 0x12;
	a.clockAccuracy = 0x8C;
	a.offsetScaledLogVariance = 0x5678;
	memset(&b, 0, sizeof(b));

	packClockQuality(&a, msgBuf);
	unpackClockQuality(msgBuf, &b);

	return memcmp(&a, &b, sizeof(ClockQuality));
}

/* packing/unpacking TimeInteval */
int
test_TimeInterval()
{
	TimeInterval a;
	TimeInterval b;
	Octet msgBuf[PACKET_SIZE];

	/* init */
	a.scaledNanoseconds.lsb = 0x01234567;
	a.scaledNanoseconds.msb = 0x89ABCDEF;
	memset(&b, 0, sizeof(b));

	packTimeInterval(&a, msgBuf);
	unpackTimeInterval(msgBuf, &b);

	return memcmp(&a, &b, sizeof(TimeInterval));
}

/* packing/unpacking PortIdentity */
int
test_PortIdentity()
{
	PortIdentity a;
	PortIdentity b;
	Octet msgBuf[PACKET_SIZE];

	/* init */
	a.clockIdentity[0] = 0x01;
	a.clockIdentity[1] = 0x23;
	a.clockIdentity[2] = 0x45;
	a.clockIdentity[3] = 0x67;
	a.clockIdentity[4] = 0x89;
	a.clockIdentity[5] = 0xAB;
	a.clockIdentity[6] = 0xCD;
	a.clockIdentity[7] = 0xEF;
	a.portNumber = 0xFEDC;
	memset(&b, 0, sizeof(b));

	packPortIdentity(&a, msgBuf);
	unpackPortIdentity(msgBuf, &b);

	return memcmp(&a, &b, sizeof(PortIdentity));
}

/* packing/unpacking PhysicalAddress */
int
test_PhysicalAddress()
{
	PhysicalAddress a;
	PhysicalAddress b;
	Octet msgBuf[PACKET_SIZE];
	int i;
	int rc = 0;
	/* init */
	a.addressLength = 0x10;
	MALLOC(a.addressField, a.addressLength);
	for(i = 0; i < a.addressLength; i++) {
		a.addressField[i] = i;
	}
	memset(&b, 0, sizeof(b));

	packPhysicalAddress(&a, msgBuf);
	unpackPhysicalAddress(msgBuf, &b);

	if(a.addressLength != b.addressLength)
		rc += 1;
	if (memcmp(a.addressField, b.addressField, a.addressLength))
		rc += 1;

	freePhysicalAddress(&a);
	freePhysicalAddress(&b);

	return rc;
}

/* packing/unpacking PortAddress */
int
test_PortAddress()
{
	PortAddress a;
	PortAddress b;
	Octet msgBuf[PACKET_SIZE];
	int i;
	int rc = 0;

	a.networkProtocol = 0xFEDC;
	a.addressLength = 0x20;
	MALLOC(a.addressField, a.addressLength);
	for(i = 0; i < a.addressLength; i++) {
		a.addressField[i] = i;
	}
	memset(&b, 0, sizeof(b));

	packPortAddress(&a, msgBuf);
	unpackPortAddress(msgBuf, &b);

	if(a.networkProtocol != b.networkProtocol)
		rc += 1;
	if(a.addressLength != b.addressLength)
		rc += 1;
	if(memcmp(a.addressField, b.addressField, a.addressLength))
		rc += 1;

	freePortAddress(&a);
	freePortAddress(&b);

	return rc;
}

/* packing/unpacking PTPText */
int
test_PTPText()
{
	PTPText a;
	PTPText b;
	Octet msgBuf[PACKET_SIZE];
	int i;
	int rc = 0;

	a.lengthField = 0x8;
	MALLOC(a.textField, a.lengthField);
	for(i = 0; i < a.lengthField; i++) {
		a.textField[i] = i;
	}
	memset(&b, 0, sizeof(b));

	packPTPText(&a, msgBuf);
	unpackPTPText(msgBuf, &b);

	if(a.lengthField != b.lengthField)
		rc += 1;
	if(memcmp(a.textField, b.textField, a.lengthField))
		rc += 1;

	freePTPText(&a);
	freePTPText(&b);

	return rc;
}

/* packing/unpacking Timestamp */
int
test_Timestamp()
{
	Timestamp a;
	Timestamp b;
	Octet msgBuf[PACKET_SIZE];
	int rc = 0;

	a.secondsField.lsb = 0xFDECBA98;
	a.secondsField.msb = 0x7654;
	a.nanosecondsField = 0x1234567;
	memset(&b, 0, sizeof(b));

	packTimestamp(&a, msgBuf);
	unpackTimestamp(msgBuf, &b);

	if(a.secondsField.lsb != b.secondsField.lsb ||
		a.secondsField.msb != b.secondsField.msb)
		rc += 1;
	if(a.nanosecondsField != b.nanosecondsField)
		rc += 1;

	return rc;
}

int
test_ClockDescription() {
	MsgManagement a;
	MsgManagement b;
	Octet msgBuf[PACKET_SIZE];
	int rc = 0;

	/* allocate management TLVs */
	MALLOC_TLV(a, MMClockDescription);
	if(!(b.tlv = malloc(sizeof(ManagementTLV)))) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}
	a.tlv->tlvType = TLV_MANAGEMENT;
	a.tlv->managementId = MM_CLOCK_DESCRIPTION;
	b.tlv->tlvType = TLV_MANAGEMENT;
	b.tlv->managementId = MM_CLOCK_DESCRIPTION;

	if(initMMClockDescriptionFields((MMClockDescription*)a.tlv->dataField)) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}

	int length = packMMClockDescription(&a, msgBuf);
	/* check management TLV is even */
	if(length % 2) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}
	unpackMMClockDescription(msgBuf, &b);

	/* check fields */
	MMClockDescription *aCl = (MMClockDescription*)a.tlv->dataField;
	MMClockDescription *bCl = (MMClockDescription*)b.tlv->dataField;
	if(aCl->clockType0 != bCl->clockType0 || aCl->clockType1 != bCl->clockType1)
		rc += 1;
	if(aCl->physicalAddress.addressLength != bCl->physicalAddress.addressLength)
		rc += 1;
	if(memcmp(aCl->physicalAddress.addressField, bCl->physicalAddress.addressField, aCl->physicalAddress.addressLength))
		rc += 1;
	if(aCl->protocolAddress.addressLength != bCl->protocolAddress.addressLength)
		rc += 1;
	if(memcmp(aCl->protocolAddress.addressField, bCl->protocolAddress.addressField, aCl->protocolAddress.addressLength))
		rc += 1;
	if(aCl->manufacturerIdentity0 != bCl->manufacturerIdentity0)
		rc += 1;
	if(aCl->manufacturerIdentity1 != bCl->manufacturerIdentity1)
		rc += 1;
	if(aCl->manufacturerIdentity2 != bCl->manufacturerIdentity2)
		rc += 1;
	if(aCl->productDescription.lengthField != bCl->productDescription.lengthField)
		rc += 1;
	if(memcmp(aCl->productDescription.textField, bCl->productDescription.textField, aCl->productDescription.lengthField))
		rc += 1;
	if(aCl->revisionData.lengthField != bCl->revisionData.lengthField)
		rc += 1;
	if(memcmp(aCl->revisionData.textField, bCl->revisionData.textField, aCl->revisionData.lengthField))
		rc += 1;
	if(aCl->userDescription.lengthField != bCl->userDescription.lengthField)
		rc += 1;
	if(memcmp(aCl->userDescription.textField, bCl->userDescription.textField, aCl->userDescription.lengthField))
		rc += 1;
	if(aCl->profileIdentity0 != bCl->profileIdentity0)
		rc += 1;
	if(aCl->profileIdentity1 != bCl->profileIdentity1)
		rc += 1;
	if(aCl->profileIdentity2 != bCl->profileIdentity2)
		rc += 1;
	if(aCl->profileIdentity3 != bCl->profileIdentity3)
		rc += 1;
	if(aCl->profileIdentity4 != bCl->profileIdentity4)
		rc += 1;
	if(aCl->profileIdentity5 != bCl->profileIdentity5)
		rc += 1;

	freeManagementTLV(&a);
	freeManagementTLV(&b);

	return rc;
}

int
test_UserDescription() {
	MsgManagement a;
	MsgManagement b;
	Octet msgBuf[PACKET_SIZE];
	int rc = 0;

	/* allocate management TLVs */
	MALLOC_TLV(a, MMUserDescription);
	if(!(b.tlv = malloc(sizeof(ManagementTLV)))) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}
	a.tlv->tlvType = TLV_MANAGEMENT;
	a.tlv->managementId = MM_USER_DESCRIPTION;
	b.tlv->tlvType = TLV_MANAGEMENT;
	b.tlv->managementId = MM_USER_DESCRIPTION;

	if(initMMUserDescriptionFields((MMUserDescription*)a.tlv->dataField)) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}

	int length = packMMUserDescription(&a, msgBuf);
	if(length % 2) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}

	/* unpack clock description */
	unpackMMUserDescription(msgBuf, &b);

	/* check fields */
	MMUserDescription *aU = (MMUserDescription*)a.tlv->dataField;
	MMUserDescription *bU = (MMUserDescription*)b.tlv->dataField;
	if(aU->userDescription.lengthField != bU->userDescription.lengthField)
		rc += 1;
	if(memcmp(aU->userDescription.textField, bU->userDescription.textField, aU->userDescription.lengthField))
		rc += 1;

	freeManagementTLV(&a);
	freeManagementTLV(&b);

	return rc;
}

int
test_Initialize() {
	MsgManagement a;
	MsgManagement b;
	Octet msgBuf[PACKET_SIZE];
	int rc = 0;

	/* allocate management TLVs */
	MALLOC_TLV(a, MMInitialize);
	if(!(b.tlv = malloc(sizeof(ManagementTLV)))) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}

	initMMInitializeFields((MMInitialize*)a.tlv->dataField);

	int length = packMMInitialize(&a, msgBuf);
	/* check management TLV is even */
	if(length % 2) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}
	unpackMMInitialize(msgBuf, &b);

	/* check fields */
	MMInitialize *aI = (MMInitialize*)a.tlv->dataField;
	MMInitialize *bI = (MMInitialize*)b.tlv->dataField;
	if(aI->initializeKey != bI->initializeKey)
		rc += 1;

	FREE_TLV(a);
	FREE_TLV(b);

	return rc;
}

int
test_DefaultDataSet() {
	MsgManagement a;
	MsgManagement b;
	Octet msgBuf[PACKET_SIZE];
	int rc = 0;

	/* allocate management TLVs */
	MALLOC_TLV(a, MMDefaultDataSet);
	if(!(b.tlv = malloc(sizeof(ManagementTLV)))) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}

	initMMDefaultDataSetFields((MMDefaultDataSet*)a.tlv->dataField);

	int length = packMMDefaultDataSet(&a, msgBuf);
	/* check management TLV is even */
	if(length % 2) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}
	unpackMMDefaultDataSet(msgBuf, &b);

	/* check fields */
	MMDefaultDataSet *aD = (MMDefaultDataSet*)a.tlv->dataField;
	MMDefaultDataSet *bD = (MMDefaultDataSet*)b.tlv->dataField;
	if(aD->so_tsc != bD->so_tsc)
		rc += 1;
	if(aD->reserved0 != bD->reserved0)
		rc += 1;
	if(aD->numberPorts != bD->numberPorts)
		rc += 1;
	if(aD->priority1 != bD->priority1)
		rc += 1;
	if(aD->clockQuality.clockClass != bD->clockQuality.clockClass)
		rc += 1;
	if(aD->clockQuality.clockAccuracy != bD->clockQuality.clockAccuracy)
		rc += 1;
	if(aD->clockQuality.offsetScaledLogVariance != bD->clockQuality.offsetScaledLogVariance)
		rc += 1;
	if(aD->priority2 != bD->priority2)
		rc += 1;
	if(memcmp(aD->clockIdentity, bD->clockIdentity, CLOCK_IDENTITY_LENGTH))
		rc += 1;
	if(aD->domainNumber != bD->domainNumber)
		rc += 1;
	if(aD->reserved1 != bD->reserved1)
		rc += 1;

	FREE_TLV(a);
	FREE_TLV(b);

	return rc;
}

int
test_Priority1() {
	MsgManagement a;
	MsgManagement b;
	Octet msgBuf[PACKET_SIZE];
	int rc = 0;

	/* allocate management TLVs */
	MALLOC_TLV(a, MMPriority1);
        if(!(b.tlv = malloc(sizeof(ManagementTLV)))) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
        }

	initMMPriority1Fields((MMPriority1*)a.tlv->dataField);

	int length = packMMPriority1(&a, msgBuf);
	/* check management TLV is even */
	if(length % 2) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}
	unpackMMPriority1(msgBuf, &b);

	/* check fields */
	MMPriority1 *aP = (MMPriority1*)a.tlv->dataField;
	MMPriority1 *bP = (MMPriority1*)b.tlv->dataField;
	if(aP->priority1 != bP->priority1)
		rc += 1;

	FREE_TLV(a);
	FREE_TLV(b);

	return rc;
}

int
test_Priority2() {
	MsgManagement a;
	MsgManagement b;
	Octet msgBuf[PACKET_SIZE];
	int rc = 0;

	/* allocate management TLVs */
	MALLOC_TLV(a, MMPriority2);
        if(!(b.tlv = malloc(sizeof(ManagementTLV)))) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
        }

	initMMPriority2Fields((MMPriority2*)a.tlv->dataField);

	int length = packMMPriority2(&a, msgBuf);
	/* check management TLV is even */
	if(length % 2) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}
	unpackMMPriority2(msgBuf, &b);

	/* check fields */
	MMPriority2 *aP = (MMPriority2*)a.tlv->dataField;
	MMPriority2 *bP = (MMPriority2*)b.tlv->dataField;
	if(aP->priority2 != bP->priority2)
		rc += 1;

	FREE_TLV(a);
	FREE_TLV(b);

	return rc;
}

int
test_Domain() {
	MsgManagement a;
	MsgManagement b;
	Octet msgBuf[PACKET_SIZE];
	int rc = 0;

	/* allocate management TLVs */
	MALLOC_TLV(a, MMDomain);
        if(!(b.tlv = malloc(sizeof(ManagementTLV)))) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
        }

	initMMDomainFields((MMDomain*)a.tlv->dataField);

	int length = packMMDomain(&a, msgBuf);
	/* check management TLV is even */
	if(length % 2) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}
	unpackMMDomain(msgBuf, &b);

	/* check fields */
	MMDomain *aD = (MMDomain*)a.tlv->dataField;
	MMDomain *bD = (MMDomain*)b.tlv->dataField;
	if(aD->domainNumber != bD->domainNumber)
		rc += 1;

	FREE_TLV(a);
	FREE_TLV(b);

	return rc;
}

int
test_SlaveOnly() {
	MsgManagement a;
	MsgManagement b;
	Octet msgBuf[PACKET_SIZE];
	int rc = 0;

	/* allocate management TLVs */
	MALLOC_TLV(a, MMSlaveOnly);
        if(!(b.tlv = malloc(sizeof(ManagementTLV)))) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
        }

	initMMSlaveOnlyFields((MMSlaveOnly*)a.tlv->dataField);

	int length = packMMSlaveOnly(&a, msgBuf);
	/* check management TLV is even */
	if(length % 2) {
		FREE_TLV(a);
		free(b.tlv);
		return -1;
	}
	unpackMMSlaveOnly(msgBuf, &b);

	/* check fields */
	MMSlaveOnly *aD = (MMSlaveOnly*)a.tlv->dataField;
	MMSlaveOnly *bD = (MMSlaveOnly*)b.tlv->dataField;
	if(aD->so != bD->so)
		rc += 1;

	FREE_TLV(a);
	FREE_TLV(b);

	return rc;
}

int
test_handleMMNullManagement() {
	PtpClock ptpClock;
	MsgManagement incoming;
	MsgManagement outgoing;
	int rc = 0;

	initPtpClock(&ptpClock);

	/* initialize incoming message */
        MALLOC(incoming.tlv, sizeof(ManagementTLV));
        incoming.tlv->dataField = NULL;

	/* call handle function with GET */
	incoming.actionField = GET;
	handleMMNullManagement(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.tlv->dataField != NULL)
		rc += 1;
	freeManagementTLV(&outgoing);

	/* call handle function with SET */
	incoming.actionField = SET;
	handleMMNullManagement(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.tlv->dataField != NULL)
		rc += 1;
	freeManagementTLV(&outgoing);

	/* call handle function with COMMAND */
	incoming.actionField = COMMAND;
	handleMMNullManagement(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.tlv->dataField != NULL)
		rc += 1;
	freeManagementTLV(&outgoing);

	/* call handle function with ACKNOWLEDGE */
	incoming.actionField = ACKNOWLEDGE;
	handleMMNullManagement(&incoming, &outgoing, &ptpClock);
	/* check outgoing message, this should be a MMErrorStatus due to invalid actionField */
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else if(((MMErrorStatus*)outgoing.tlv->dataField)->managementId != MM_NULL_MANAGEMENT)
		rc += 1;
	if(outgoing.tlv->managementId != NOT_SUPPORTED)
		rc += 1;
	freeManagementTLV(&outgoing);

	free(incoming.tlv);

	return rc;
}

int
test_handleMMClockDescription() {
	PtpClock ptpClock;
	MsgManagement incoming;
	MsgManagement outgoing;
	int rc = 0;

	initPtpClock(&ptpClock);

	/* initialize incoming message */
	MALLOC_TLV(incoming, MMClockDescription);
	incoming.tlv->tlvType = TLV_MANAGEMENT;
	incoming.tlv->managementId = MM_CLOCK_DESCRIPTION;

	/* init clock description fields */
	if(initMMClockDescriptionFields((MMClockDescription*)incoming.tlv->dataField)) {
		freeManagementTLV(&incoming);
		return -1;
	}

	/* call handle function with GET */
	incoming.actionField = GET;
	handleMMClockDescription(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.actionField != RESPONSE || outgoing.tlv->dataField == NULL)
		rc += 1;
	freeManagementTLV(&outgoing);

	/* call handle function with SET */
	incoming.actionField = SET;
	handleMMClockDescription(&incoming, &outgoing, &ptpClock);
	/* check outgoing message, this should be a MMErrorStatus due to invalid actionField */
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else if(((MMErrorStatus*)outgoing.tlv->dataField)->managementId != MM_CLOCK_DESCRIPTION)
		rc += 1;
	if(outgoing.tlv->managementId != NOT_SUPPORTED)
		rc += 1;
	freeManagementTLV(&outgoing);

	freeManagementTLV(&incoming);

	return rc;
}

int
test_handleMMUserDescription() {
	PtpClock ptpClock;
	MsgManagement incoming;
	MsgManagement outgoing;
	int rc = 0;

	initPtpClock(&ptpClock);

	/* initialize incoming message */
	MALLOC_TLV(incoming, MMUserDescription);
	incoming.tlv->tlvType = TLV_MANAGEMENT;
	incoming.tlv->managementId = MM_USER_DESCRIPTION;

	/* init clock description fields */
	if(initMMUserDescriptionFields((MMUserDescription*)incoming.tlv->dataField)) {
		freeManagementTLV(&incoming);
		return -1;
	}

	/* call handle function with GET */
	incoming.actionField = GET;
	handleMMUserDescription(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.actionField != RESPONSE || outgoing.tlv->dataField == NULL)
		rc += 1;
	freeManagementTLV(&outgoing);

	/* call handle function with SET */
	incoming.actionField = SET;
	handleMMUserDescription(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.actionField != RESPONSE || outgoing.tlv->dataField == NULL)
		rc += 1;
	freeManagementTLV(&outgoing);

	/* call handle function with COMMAND */
	incoming.actionField = COMMAND;
	handleMMUserDescription(&incoming, &outgoing, &ptpClock);
	/* check outgoing message, this should be a MMErrorStatus due to invalid actionField */
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else if(((MMErrorStatus*)outgoing.tlv->dataField)->managementId != MM_USER_DESCRIPTION)
		rc += 1;
	if(outgoing.tlv->managementId != NOT_SUPPORTED)
		rc += 1;
	freeManagementTLV(&outgoing);

	freeManagementTLV(&incoming);

	return rc;
}


int
test_handleMMEnablePort() {
	PtpClock ptpClock;
	MsgManagement incoming;
	MsgManagement outgoing;
	int rc = 0;

	initPtpClock(&ptpClock);

	/* initialize incoming message */
	MALLOC(incoming.tlv, sizeof(ManagementTLV));

	/* call handle function with COMMAND */
	incoming.actionField = COMMAND;
	handleMMEnablePort(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.actionField != ACKNOWLEDGE || outgoing.tlv->dataField != NULL)
		rc += 1;
	freeManagementTLV(&outgoing);

	/* call handle function with GET */
	incoming.actionField = GET;
	handleMMEnablePort(&incoming, &outgoing, &ptpClock);
	/* check outgoing message, this should be a MMErrorStatus due to invalid actionField */
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else if(((MMErrorStatus*)outgoing.tlv->dataField)->managementId != MM_ENABLE_PORT)
		rc += 1;
	if(outgoing.tlv->managementId != NOT_SUPPORTED)
		rc += 1;
	freeManagementTLV(&outgoing);

	free(incoming.tlv);

	return rc;
}

int
test_handleMMSaveInNonVolatileStorage() {
	PtpClock ptpClock;
	MsgManagement incoming;
	MsgManagement outgoing;
	int rc = 0;

	initPtpClock(&ptpClock);

	/* initialize incoming message */
	MALLOC(incoming.tlv, sizeof(ManagementTLV));

	/* call handle function with COMMAND */
	incoming.actionField = COMMAND;
	handleMMSaveInNonVolatileStorage(&incoming, &outgoing, &ptpClock);
	/* check outgoing message, this should be a MMErrorStatus due to invalid actionField */
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else if(((MMErrorStatus*)outgoing.tlv->dataField)->managementId !=
			MM_SAVE_IN_NON_VOLATILE_STORAGE)
		rc += 1;
	if(outgoing.tlv->managementId != NOT_SUPPORTED)
		rc += 1;
	freeManagementTLV(&outgoing);

	free(incoming.tlv);

	return rc;
}

int
test_handleMMTimePropertiesDataSet()
{
	PtpClock ptpClock;
	MsgManagement incoming;
	MsgManagement outgoing;
	MMTimePropertiesDataSet *data = NULL;
	int rc = 0;

	initPtpClock(&ptpClock);

	/* initialize incoming message */
	MALLOC_TLV(incoming, MMTimePropertiesDataSet);

	initMMTimePropertiesDataSetFields((MMTimePropertiesDataSet*)incoming.tlv->dataField);

	/* init flags */
        ptpClock.frequencyTraceable = 0x1;
        ptpClock.timeTraceable = 0x1;
        ptpClock.ptpTimescale = 0x1;
	ptpClock.currentUtcOffsetValid = 0x1;
        ptpClock.leap59 = 0x1;
        ptpClock.leap61 = 0x1;

	/* call handle function with GET */
	incoming.actionField = GET;
	handleMMTimePropertiesDataSet(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.actionField != RESPONSE)
		rc += 1;
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else {
		/* check fields in outgoing message */
		data = (MMTimePropertiesDataSet*)outgoing.tlv->dataField;
		if(data->ftra_ttra_ptp_utcv_li59_li61 != 0x3F) {
			rc += 1;
		}
	}
	freeManagementTLV(&outgoing);

	/* init flags */
        ptpClock.frequencyTraceable = 0x1;
        ptpClock.timeTraceable = 0x0;
        ptpClock.ptpTimescale = 0x1;
	ptpClock.currentUtcOffsetValid = 0x0;
        ptpClock.leap59 = 0x1;
        ptpClock.leap61 = 0x0;

	/* call handle function with GET */
	incoming.actionField = GET;
	handleMMTimePropertiesDataSet(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.actionField != RESPONSE)
		rc += 1;
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else {
		/* check fields in outgoing message */
		data = (MMTimePropertiesDataSet*)outgoing.tlv->dataField;
		if(data->ftra_ttra_ptp_utcv_li59_li61 != 0x2A) {
			rc += 1;
		}
	}
	freeManagementTLV(&outgoing);

	FREE_TLV(incoming);

	return rc;
}

int
test_handleMMUtcProperties() {
	PtpClock ptpClock;
	MsgManagement incoming;
	MsgManagement outgoing;
	MMUtcProperties *data = NULL;
	int rc = 0;

	initPtpClock(&ptpClock);

	/* initialize incoming message */
	MALLOC_TLV(incoming, MMUtcProperties);

	initMMUtcPropertiesFields((MMUtcProperties*)incoming.tlv->dataField);

	/* set ptpClock flags */
	ptpClock.currentUtcOffsetValid = 0x1;
        ptpClock.leap59 = 0x1;
        ptpClock.leap61 = 0x1;

	/* call handle function with GET */
	incoming.actionField = GET;
	handleMMUtcProperties(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.actionField != RESPONSE)
		rc += 1;
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else {
		/* check fields in outgoing message */
		data = (MMUtcProperties*)outgoing.tlv->dataField;
		if(data->utcv_li59_li61 != 0x7) {
			rc += 1;
		}
	}
	freeManagementTLV(&outgoing);

	/* init incoming message flag fields */
	data = (MMUtcProperties*)incoming.tlv->dataField;
	data->utcv_li59_li61 = 0x5;

	/* call handle function with SET */
	incoming.actionField = SET;
	handleMMUtcProperties(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.actionField != RESPONSE)
		rc += 1;
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else {
		/* check fields in outgoing message */
		data = (MMUtcProperties*)outgoing.tlv->dataField;
		if(data->utcv_li59_li61 != 0x5)
			rc += 1;
	}
	if(ptpClock.currentUtcOffsetValid != 0x1)
		rc += 1;
	if(ptpClock.leap59 != 0x0)
		rc += 1;
	if(ptpClock.leap61 != 0x1)
		rc += 1;
	freeManagementTLV(&outgoing);

	FREE_TLV(incoming);

	return rc;
}

int
test_handleMMTraceabilityProperties() {
	PtpClock ptpClock;
	MsgManagement incoming;
	MsgManagement outgoing;
	MMTraceabilityProperties *data = NULL;
	int rc = 0;

	initPtpClock(&ptpClock);

	/* initialize incoming message */
	MALLOC_TLV(incoming, MMTraceabilityProperties);

	initMMTraceabilityPropertiesFields((MMTraceabilityProperties*)incoming.tlv->dataField);

	/* set ptpClock flags */
        ptpClock.frequencyTraceable = 0x1;
        ptpClock.timeTraceable = 0x1;

	/* call handle function with GET */
	incoming.actionField = GET;
	handleMMTraceabilityProperties(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.actionField != RESPONSE)
		rc += 1;
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else {
		/* check fields in outgoing message */
		data = (MMTraceabilityProperties*)outgoing.tlv->dataField;
		if(data->ftra_ttra != 0x30)
			rc += 1;
	}
	freeManagementTLV(&outgoing);

	/* init incoming message flag fields */
	data = (MMTraceabilityProperties*)incoming.tlv->dataField;
	data->ftra_ttra = 0x20;

	/* call handle function with SET */
	incoming.actionField = SET;
	handleMMTraceabilityProperties(&incoming, &outgoing, &ptpClock);
	/* check outgoing message */
	if(outgoing.actionField != RESPONSE)
		rc += 1;
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else {
		/* check fields in outgoing message */
		data = (MMTraceabilityProperties*)outgoing.tlv->dataField;
		if(data->ftra_ttra != 0x20)
			rc += 1;
	}
	if(ptpClock.frequencyTraceable != 0x1)
		rc += 1;
	if(ptpClock.timeTraceable != 0x0)
		rc += 1;
	freeManagementTLV(&outgoing);

	FREE_TLV(incoming);

	return rc;
}

int
test_handleErrorManagementMessage() {
	PtpClock ptpClock;
	MsgManagement incoming;
	MsgManagement outgoing;
	int rc = 0;

	initPtpClock(&ptpClock);

	/* initialize incoming message */
	MALLOC(incoming.tlv, sizeof(ManagementTLV));

	/* unsupported managementId */
	incoming.actionField = GET;
	handleErrorManagementMessage(&incoming, &outgoing, &ptpClock, MM_FAULT_LOG, NO_SUCH_ID);
	/* check outgoing message */
	if(outgoing.tlv->dataField == NULL)
		rc += 1;
	else if(((MMErrorStatus*)outgoing.tlv->dataField)->managementId != MM_FAULT_LOG)
		rc += 1;
	if(outgoing.tlv->managementId != NO_SUCH_ID)
		rc += 1;
	if(outgoing.actionField != RESPONSE)
		rc += 1;
	freeManagementTLV(&outgoing);

	free(incoming.tlv);

	return rc;
}

