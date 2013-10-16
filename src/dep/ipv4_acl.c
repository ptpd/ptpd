/*-
 * Copyright (c) 2013 Wojciech Owczarek,
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
 * @file 	   ipv4_acl.c
 * @date   Sun Oct 13 21:02:23 2013
 * 
 * @brief  Code to handle IPv4 access control lists
 * 
 * Functions in this file parse, create and match IPv4 ACLs.
 * 
 */

#include "../ptpd.h"

/* qsort() compare function for sorting ACL entries */
static int
cmpAclEntry(const void *p1, const void *p2)
{

	const AclEntry *left = p1;
	const AclEntry *right = p2;

	if(left->network > right->network)
		return 1;
	if(left->network < right->network)
		return -1;
	return 0;

}

/* Parse an ACL string into an aclEntry table and return number of entries. output can be NULL */
int
maskParser(const char* input, AclEntry* output)
{
	int i;
	int offset = 0;
	int masks_found = 0;
	int matches;
	int oct1, oct2, oct3, oct4, mask;
	uint32_t network, bitmask;

	static char* expr1="%d.%d.%d.%d/%d%[, ]";
	static char* expr2="%d.%d.%d.%d/%d";

	char** expr = &expr1;
	char junk[10];
	char buf[PATH_MAX+1];

	int len = strlen(input) > PATH_MAX ? PATH_MAX : strlen(input);

	for(i=0; i <= len; i++) {

		memset(buf,0,PATH_MAX);
		strncpy(buf,input+offset,i-offset);
		if(i==len) expr = &expr2;
		matches=sscanf(buf,*expr,&oct1,&oct2,&oct3,&oct4,&mask,&junk);
		/* Only useful for "deep" debugging */
//		DBGV("%d + %d of %d, buf: \"%s\" , matches: %d\n",offset,i,len,buf,matches);
		if(matches==6 || (i==len && matches==5)) {

			offset = i;
			if(!IN_RANGE(oct1,0,255))
			    return -1;
			if(!IN_RANGE(oct2,0,255))
			    return -1;
			if(!IN_RANGE(oct3,0,255))
			    return -1;
			if(!IN_RANGE(oct4,0,255))
			    return -1;
			if(!IN_RANGE(mask,0,32))
			    return -1;
			network = (oct1 << 24) | (oct2 << 16) | (oct3 << 8) | oct4;
			/* shifting a 32-bit field by 32 bits is undefined in C */
			if(mask==0)
			    bitmask = 0;
			else
			    bitmask = ~0 << (32 - mask);
			network &= bitmask;
			if(output != NULL) {
				output[masks_found].bitmask = bitmask;
				output[masks_found].network = network;
				output[masks_found].netmask = (uint16_t)mask;
			}
			DBGV("Got mask: %d.%d.%d.%d/%d, network: %08x, netmask %08x\n", 
				oct1, oct2, oct3, oct4, mask, network, bitmask);
			masks_found++;
		}

	}

	if(output != NULL)
		qsort(output, masks_found, sizeof(AclEntry), cmpAclEntry);

	/* We got input but found nothing - error */
	if (!masks_found && len > 0)
		masks_found = -1;

	/* We did not complete parsing the ACL - error */
	if(len != offset)
		masks_found = -1;

	return masks_found;

}


/* Create a maskTable from a text ACL */
static MaskTable*
createMaskTable(const char* input)
{
	MaskTable* ret;
	int masksFound = maskParser(input, NULL);
	if(masksFound>0) {
		ret=(MaskTable*)calloc(1,sizeof(MaskTable));
		ret->entries = (AclEntry*)calloc(masksFound, sizeof(AclEntry));
		ret->numEntries = maskParser(input,ret->entries);
	} else {
		if(masksFound < 0)
		    ERROR("Error while parsing access list: \"%s\"\n", input);
		return NULL;
	}
	return ret;
}

/* Print the contents of a single mask table */
static void
dumpMaskTable(MaskTable* table)
{
	int i;
	uint32_t network;
	if(table == NULL)
	    return;
	INFO("number of entries: %d\n",table->numEntries);
	if(table->entries != NULL) {
		for(i = 0; i < table->numEntries; i++) {
		    AclEntry this = table->entries[i];
#ifdef PTPD_MSBF
		    network = this.network;
#else
		    network = htonl(this.network);
#endif
		    INFO("%d.%d.%d.%d/%d (0x%.8x / 0x%.8x), matches: %d\n",
		    *((uint8_t*)&network), *((uint8_t*)&network+1),
		    *((uint8_t*)&network+2), *((uint8_t*)&network+3), this.netmask,
		    this.network, this.bitmask, this.hitCount);
		}
	}

}

/* Destroy an Ipv4AccessList structure */
void 
freeIpv4AccessList(Ipv4AccessList* acl)
{
	if(acl == NULL)
	    return;
	if(acl->permitEntries != NULL && acl->permitEntries->entries != NULL)
	    free(acl->permitEntries->entries);
	if(acl->permitEntries != NULL)
	    free(acl->permitEntries);
	if(acl != NULL)
	    free(acl);
}

/* Structure initialisation for Ipv4AccessList */
Ipv4AccessList*
createIpv4AccessList(const char* permitList, const char* denyList, int processingOrder)
{
	Ipv4AccessList* ret;
	ret = (Ipv4AccessList*)calloc(1,sizeof(Ipv4AccessList));
	ret->permitEntries = createMaskTable(permitList);
	ret->denyEntries = createMaskTable(denyList);
	if(ret->permitEntries == NULL || ret->denyEntries == NULL) {
		freeIpv4AccessList(ret);
		return NULL;
	}
	ret->processingOrder = processingOrder;
	return ret;
}


/* Match an IP address against a MaskTable */
static int
matchAddress(const uint32_t addr, MaskTable* table)
{

	int i;
	if(table == NULL || table->entries == NULL || table->numEntries==0)
	    return -1;
	for(i = 0; i < table->numEntries; i++) {
		DBGV("addr: %08x, addr & mask: %08x, network: %08x\n",addr, table->entries[i].bitmask & addr, table->entries[i].network);
		if((table->entries[i].bitmask & addr) == table->entries[i].network) {
			table->entries[i].hitCount++;
			return 1;
		}
	}

	return 0;

}

/* Test an IP address against an ACL */
int
matchIpv4AccessList(Ipv4AccessList* acl, const uint32_t addr)
{

	int ret;
	int matchPermit = 0;
	int matchDeny = 0;

	/* Non-functional ACL permits everything */
	if(acl == NULL) {
		ret = 1;
		goto end;
	}

	if(acl->permitEntries != NULL)
		matchPermit = matchAddress(addr,acl->permitEntries) > 0;

	if(acl->denyEntries != NULL)
		matchDeny = matchAddress(addr,acl->denyEntries) > 0;

	switch(acl->processingOrder) {
		case ACL_PERMIT_DENY:
			if(!matchPermit) {
				ret = 0;
				break;
			}
			if(matchDeny) {
				ret = 0;
				break;
			}
			ret = 1;
			break;
		default:
		case ACL_DENY_PERMIT:
			if (matchDeny && !matchPermit) {
				ret = 0;
				break;
			}
			else {
				ret = 1;
				break;
			}
	}

	end:

	if(ret)
	    acl->passedCounter++;
	else
	    acl->droppedCounter++;

	return ret;

}

/* Dump the contents and hit counters of an ACL */
void dumpIpv4AccessList(Ipv4AccessList* acl)
{

		    INFO("\n\n");
	if(acl == NULL) {
		    INFO("(uninitialised ACL)\n");
		    return;
	}
	switch(acl->processingOrder) {
		case ACL_DENY_PERMIT:
		    INFO("ACL order: deny,permit\n");
		    INFO("Passed packets: %d, dropped packets: %d\n",
				acl->passedCounter, acl->droppedCounter);
		    INFO("--------\n");
		    INFO("Deny list:\n");
		    dumpMaskTable(acl->denyEntries);
		    INFO("--------\n");
		    INFO("Permit list:\n");
		    dumpMaskTable(acl->permitEntries);
		break;
		case ACL_PERMIT_DENY:
		default:
		    INFO("ACL order: permit,deny\n");
		    INFO("Passed packets: %d, dropped packets: %d\n",
				acl->passedCounter, acl->droppedCounter);
		    INFO("--------\n");
		    INFO("Permit list:\n");
		    dumpMaskTable(acl->permitEntries);
		    INFO("--------\n");
		    INFO("Deny list:\n");
		    dumpMaskTable(acl->denyEntries);
		break;
	}
		    INFO("\n\n");
}

/* Clear counters in a MaskTable */
static void
clearMaskTableCounters(MaskTable* table)
{

	int i, count;
	if(table==NULL || table->numEntries==0)
		return;
	count = table->numEntries;

	for(i=0; i<count; i++) {
		table->entries[i].hitCount = 0;
	}

}

/* Clear ACL counter */
void clearIpv4AccessListCounters(Ipv4AccessList* acl) {

	if(acl == NULL)
		return;
	acl->passedCounter=0;
	acl->droppedCounter=0;
	clearMaskTableCounters(acl->permitEntries);
	clearMaskTableCounters(acl->denyEntries);

}