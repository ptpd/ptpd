/*-
 * Copyright (c) 2013-2014 Wojciech Owczarek,
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
#include "string.h"

/**
 * strdup + free are used across code using strtok_r, so as to
 * protect the original string, as strtok* modify it.
 */

/* count tokens in string delimited by delim */
static int countTokens(const char* text, const char* delim) {

    int count=0;
    char* stash = NULL;
    char* text_;
    char* text__;

    if(text==NULL || delim==NULL)
	return 0;

    text_=strdup(text);

    for(text__=text_,count=0;strtok_r(text__,delim, &stash) != NULL; text__=NULL) {
	    count++;
    }
    free(text_);
    return count;

}

/* Parse a dotted-decimal string into an uint8_t array - return -1 on error */
static int ipToArray(const char* text, uint8_t dest[], int maxOctets, Boolean isMask)
{

    char* text_;
    char* text__;
    char* subtoken;
    char* stash = NULL;
    char* endptr;
    long octet;

    int count = 0;
    int result = 0;

    memset(&dest[0], 0, maxOctets * sizeof(uint8_t));

    text_=strdup(text);

    for(text__=text_;;text__=NULL) {

	if(count > maxOctets) {
	    result = -1;
	    goto end;
	}

	subtoken = strtok_r(text__,".",&stash);

	if(subtoken == NULL)
		goto end;

	errno = 0;
	octet=strtol(subtoken,&endptr,10);
	if(errno!=0 || !IN_RANGE(octet,0,255)) {
	    result = -1;
	    goto end;

	}

	dest[count++] = (uint8_t)octet;

	/* If we're parsing a mask and an octet is less than 0xFF, whole rest is zeros */
	if(isMask && octet < 255)
		goto end;
    }

    end:

    free(text_);
    return result;

}

/* Parse a single net mask into an AclEntry */
static int parseAclEntry(const char* line, AclEntry* acl) {

    int result = 1;
    char* stash = NULL;
    char* text_;
    char* token;
    char* endptr;
    long octet = 0;

    uint8_t net_octets[4];
    uint8_t mask_octets[4];

    if(line == NULL || acl == NULL)
	return -1;

    if(countTokens(line,"/") == 0)
	return -1;

    text_=strdup(line);

    token=strtok_r(text_,"/",&stash);

    if((countTokens(token,".") > 4) ||
	(countTokens(token,".") < 1) ||
	(ipToArray(token,net_octets,4,0) < 0)) {
	    result=-1;
	    goto end;
    }

    acl->network = (net_octets[0] << 24) | (net_octets[1] << 16) | ( net_octets[2] << 8) | net_octets[3];

    token=strtok_r(NULL,"/",&stash);

    if(token == NULL) {
	acl->netmask=32;
	acl->bitmask=~0;
    } else if(countTokens(token,".") == 1) {
	errno = 0;
	octet = strtol(token,&endptr,10);
	if(errno != 0 || !IN_RANGE(octet,0,32)) {
		result = -1;
		goto end;
	}

	if(octet == 0)
	    acl->bitmask = 0;
	else
	    acl->bitmask = ~0 << (32 - octet);
	acl->netmask = (uint16_t)octet;

    } else if((countTokens(token,".") > 4) ||
	(ipToArray(token,mask_octets,4,1) < 0)) {
	    result=-1;
	    goto end;
    } else {

	acl->bitmask = (mask_octets[0] << 24) | (mask_octets[1] << 16) | ( mask_octets[2] << 8) | mask_octets[3];
	uint32_t tmp = acl->bitmask;
	int count = 0;
	for(tmp = acl->bitmask;tmp<<count;count++);
	acl->netmask=count;
    }

    end:
    free(text_);

    acl->network &= acl->bitmask;

    return result;

}


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

    char* token;
    char* stash;
    int found = 0;
    char* text_;
    char* text__;
    AclEntry tmp;

    tmp.hitCount=0;

    if(strlen(input)==0) return 0;

    text_=strdup(input);

    for(text__=text_;;text__=NULL) {

	token=strtok_r(text__,", ;\t",&stash);
	if(token==NULL) break;

	if(parseAclEntry(token,&tmp)<1) {

	    found = -1;
	    break;

	}

	if(output != NULL)
	    output[found]=tmp;

	found++;


    }

	if(found && (output != NULL))
		qsort(output, found, sizeof(AclEntry), cmpAclEntry);

	/* We got input but found nothing - error */
	if (!found)
		found = -1;

	free(text_);
	return found;

}

/* Create a maskTable from a text ACL */
static MaskTable*
createMaskTable(const char* input)
{
	MaskTable* ret;
	int masksFound = maskParser(input, NULL);
	if(masksFound>=0) {
		ret=(MaskTable*)calloc(1,sizeof(MaskTable));
		ret->entries = (AclEntry*)calloc(masksFound, sizeof(AclEntry));
		ret->numEntries = maskParser(input,ret->entries);
		return ret;
	} else {
		ERROR("Error while parsing access list: \"%s\"\n", input);
		return NULL;
	}
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
		    INFO("%d.%d.%d.%d/%d\t(0x%.8x/0x%.8x), matches: %d\n",
		    *((uint8_t*)&network), *((uint8_t*)&network+1),
		    *((uint8_t*)&network+2), *((uint8_t*)&network+3), this.netmask,
		    this.network, this.bitmask, this.hitCount);
		}
	}

}

/* Free a MaskTable structure */
static void freeMaskTable(MaskTable** table)
{
    if(*table == NULL)
	return;

    if((*table)->entries != NULL) {
	free((*table)->entries);
	(*table)->entries = NULL;
    }
    free(*table);
    *table = NULL;
}

/* Destroy an Ipv4AccessList structure */
void
freeIpv4AccessList(Ipv4AccessList** acl)
{
	if(*acl == NULL)
	    return;

	freeMaskTable(&(*acl)->permitTable);
	freeMaskTable(&(*acl)->denyTable);

	free(*acl);
	*acl = NULL;
}

/* Structure initialisation for Ipv4AccessList */
Ipv4AccessList*
createIpv4AccessList(const char* permitList, const char* denyList, int processingOrder)
{
	Ipv4AccessList* ret;
	ret = (Ipv4AccessList*)calloc(1,sizeof(Ipv4AccessList));
	ret->permitTable = createMaskTable(permitList);
	ret->denyTable = createMaskTable(denyList);
	if(ret->permitTable == NULL || ret->denyTable == NULL) {
		freeIpv4AccessList(&ret);
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

	if(acl->permitTable != NULL)
		matchPermit = matchAddress(addr,acl->permitTable) > 0;

	if(acl->denyTable != NULL)
		matchDeny = matchAddress(addr,acl->denyTable) > 0;

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
		    dumpMaskTable(acl->denyTable);
		    INFO("--------\n");
		    INFO("Permit list:\n");
		    dumpMaskTable(acl->permitTable);
		break;
		case ACL_PERMIT_DENY:
		default:
		    INFO("ACL order: permit,deny\n");
		    INFO("Passed packets: %d, dropped packets: %d\n",
				acl->passedCounter, acl->droppedCounter);
		    INFO("--------\n");
		    INFO("Permit list:\n");
		    dumpMaskTable(acl->permitTable);
		    INFO("--------\n");
		    INFO("Deny list:\n");
		    dumpMaskTable(acl->denyTable);
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
	clearMaskTableCounters(acl->permitTable);
	clearMaskTableCounters(acl->denyTable);

}
