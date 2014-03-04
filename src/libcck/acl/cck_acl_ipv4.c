/*-
 * libCCK - Clock Construction Kit
 *
 * Copyright (c) 2014 Wojciech Owczarek,
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


#include "cck_acl_ipv4.h"

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
static int ipToArray(const char* text, uint8_t dest[], int maxOctets, int isMask)
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
static int parseIpv4AclEntry(const char* line, Ipv4AclEntry* acl) {

    int result = 1;
    char* stash;
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
cmpIpv4AclEntry(const void *p1, const void *p2)
{

	const Ipv4AclEntry *left = p1;
	const Ipv4AclEntry *right = p2;

	if(left->network > right->network)
		return 1;
	if(left->network < right->network)
		return -1;
	return 0;

}

/* Parse an ACL string into an aclEntry table and return number of entries. output can be NULL */
static int
ipv4MaskParser(const char* input, Ipv4AclEntry* output)
{

    char* token;
    char* stash;
    int found = 0;
    char* text_;
    char* text__;
    Ipv4AclEntry tmp;

    tmp.hitCount=0;

    if(strlen(input)==0) return 0;

    text_=strdup(input);

    for(text__=text_;;text__=NULL) {

	token=strtok_r(text__,", ;\t",&stash);
	if(token==NULL) break;

	if(parseIpv4AclEntry(token,&tmp)<1) {

	    found = -1;
	    break;

	}

	if(output != NULL)
	    output[found]=tmp;

	found++;


    }

	if(found && (output != NULL))
		qsort(output, found, sizeof(Ipv4AclEntry), cmpIpv4AclEntry);

	/* We got input but found nothing - error */
	if (!found)
		found = -1;

	free(text_);
	return found;

}

/* Create a maskTable from a text ACL */
static Ipv4MaskTable*
createIpv4MaskTable(const char* input)
{
	Ipv4MaskTable* ret;
	int masksFound = ipv4MaskParser(input, NULL);
	if(masksFound>=0) {
		ret=(Ipv4MaskTable*)calloc(1,sizeof(Ipv4MaskTable));
		ret->entries = (Ipv4AclEntry*)calloc(masksFound, sizeof(Ipv4AclEntry));
		ret->numEntries = ipv4MaskParser(input,ret->entries);
		return ret;
	} else {
		CCK_ERROR("Error while parsing access list: \"%s\"\n", input);
		return NULL;
	}
}

/* Print the contents of a single mask table */
static void
dumpIpv4MaskTable(Ipv4MaskTable* table)
{
	int i;
	uint32_t network;
	if(table == NULL)
	    return;
	CCK_INFO("number of entries: %d\n",table->numEntries);
	if(table->entries != NULL) {
		for(i = 0; i < table->numEntries; i++) {
		    Ipv4AclEntry this = table->entries[i];
#ifdef PTPD_MSBF
		    network = this.network;
#else
		    network = htonl(this.network);
#endif
		    CCK_INFO("%d.%d.%d.%d/%d\t(0x%.8x/0x%.8x), matches: %d\n",
		    *((uint8_t*)&network), *((uint8_t*)&network+1),
		    *((uint8_t*)&network+2), *((uint8_t*)&network+3), this.netmask,
		    this.network, this.bitmask, this.hitCount);
		}
	}

}

/* Free a MaskTable structure */
static void freeIpv4MaskTable(Ipv4MaskTable** table)
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

/* Match an IP address against a MaskTable */
static int
matchIpv4Address(const uint32_t addr, Ipv4MaskTable* table)
{

	int i;
	if(table == NULL || table->entries == NULL || table->numEntries==0)
	    return -1;
	for(i = 0; i < table->numEntries; i++) {
		CCK_DBG("addr: %08x, addr & mask: %08x, network: %08x\n",addr, table->entries[i].bitmask & addr, table->entries[i].network);
		if((table->entries[i].bitmask & addr) == table->entries[i].network) {
			table->entries[i].hitCount++;
			return 1;
		}
	}

	return 0;

}



/* Clear counters in a MaskTable */
static void
clearIpv4MaskTableCounters(Ipv4MaskTable* table)
{

	int i, count;
	if(table==NULL || table->numEntries==0)
		return;
	count = table->numEntries;

	for(i=0; i<count; i++) {
		table->entries[i].hitCount = 0;
	}

}


void cckAclInit_ipv4 (CckAcl* acl)
{
/* Nothing special in this implementation */
}

int
cckAclTest_ipv4(CckAcl* acl, const char* aclText)
{
	return ipv4MaskParser(aclText, NULL);
}

int
cckAclCompile_ipv4 (CckAcl* acl, const char* permitList, const char* denyList)
{

	acl->aclPermitData = createIpv4MaskTable(permitList);
	acl->aclDenyData = createIpv4MaskTable(denyList);

	if(acl->aclPermitData == NULL || acl->aclDenyData == NULL) {
		cckAclShutdown_ipv4(acl);
		return CCK_FALSE;
	}

	return CCK_TRUE;
}

int
cckAclShutdown_ipv4 (void* component)
{

	if( component == NULL)
	    return 0;

	CckAcl* acl = (CckAcl*)component;

	freeIpv4MaskTable((Ipv4MaskTable**)&acl->aclPermitData);
	freeIpv4MaskTable((Ipv4MaskTable**)&acl->aclDenyData);

	return 1;
}

int
cckAclMatchAddress_ipv4 (TransportAddress* addr, CckAcl* acl)
{

	int ret;
	int matchPermit = 0;
	int matchDeny = 0;

	/* Non-functional ACL permits everything */
	if(acl == NULL) {
		ret = 1;
		goto end;
	}

	if(acl->aclPermitData != NULL)
		matchPermit = matchIpv4Address(ntohl(addr->inetAddr4.sin_addr.s_addr),
						(Ipv4MaskTable*)acl->aclPermitData) > 0;

	if(acl->aclDenyData != NULL)
		matchDeny = matchIpv4Address(ntohl(addr->inetAddr4.sin_addr.s_addr),
						(Ipv4MaskTable*)acl->aclDenyData) > 0;

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

void
cckAclClearCounters_ipv4 (CckAcl* acl)
{
	if(acl == NULL)
	    return;
	acl->passedCounter = 0;
	acl->droppedCounter = 0;
	clearIpv4MaskTableCounters((Ipv4MaskTable*)acl->aclPermitData);
	clearIpv4MaskTableCounters((Ipv4MaskTable*)acl->aclDenyData);
}

void cckAclDump_ipv4 (CckAcl* acl)
{

	CCK_INFO("\n\n");
	if(acl == NULL) {
		    CCK_INFO("(uninitialised ACL)\n");
		    return;
	}
	switch(acl->processingOrder) {
		case ACL_DENY_PERMIT:
		    CCK_INFO("ACL order: deny,permit\n");
		    CCK_INFO("Passed packets: %d, dropped packets: %d\n",
				acl->passedCounter, acl->droppedCounter);
		    CCK_INFO("--------\n");
		    CCK_INFO("Deny list:\n");
		    dumpIpv4MaskTable((Ipv4MaskTable*)acl->aclDenyData);
		    CCK_INFO("--------\n");
		    CCK_INFO("Permit list:\n");
		    dumpIpv4MaskTable((Ipv4MaskTable*)acl->aclPermitData);
		break;
		case ACL_PERMIT_DENY:
		default:
		    CCK_INFO("ACL order: permit,deny\n");
		    CCK_INFO("Passed packets: %d, dropped packets: %d\n",
				acl->passedCounter, acl->droppedCounter);
		    CCK_INFO("--------\n");
		    CCK_INFO("Permit list:\n");
		    dumpIpv4MaskTable((Ipv4MaskTable*)acl->aclPermitData);
		    CCK_INFO("--------\n");
		    CCK_INFO("Deny list:\n");
		    dumpIpv4MaskTable((Ipv4MaskTable*)acl->aclDenyData);
		break;
	}
		    CCK_INFO("\n\n");
}
