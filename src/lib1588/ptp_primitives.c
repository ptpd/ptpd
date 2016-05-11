/*-
 * Copyright (c) 2016 Wojciech Owczarek,
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


#include "ptp_primitives.h"
#include "tmp.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define tohost16 ntohs
#define tohost32 ntohl
#define tonet16 htons
#define tonet32 htonl

#define PTP_PACK_PRIMITIVE( type ) \
void pack##type( char *to, type *from, size_t len ) { \
	*(type *)to = *from; \
} \
void unpack##type( type *to, char *from, size_t len) { \
	*to = *(type *)from; \
} \
void free##type( type *var) { \
}

#define PTP_PACK_MULTIBYTE( type, size ) \
void pack##type( char *to, type *from, size_t len ) { \
	*(type *)to = tonet##size(*from); \
} \
void unpack##type( type *to, char *from, size_t len) { \
	*to = (type)tohost##size(*(type *)from); \
} \
void free##type ( type *var) { \
}

#define PTP_PACK_NIBBLETYPE( type ) \
void pack##type##Lower( char *to, type *from, size_t len ) \
{ \
        *to &= 0xF0; \
        *to |= *from; \
} \
\
void pack##type##Upper( char *to, type *from, size_t len ) \
{ \
        *to &= 0x0F; \
        *to |=  *from << 4; \
} \
\
void unpack##type##Lower( type *to, char *from, size_t len ) \
{ \
        *to = *from & 0x0F; \
} \
\
void unpack##type##Upper( type *to, char *from, size_t len ) \
{ \
        *to = (*from >> 4) & 0x0F; \
} \
void free##type##Upper( type *var) \
{ \
}\
void free##type##Lower( type *var) \
{ \
}

PTP_PACK_PRIMITIVE(PtpBoolean)
PTP_PACK_PRIMITIVE(PtpOctet)
PTP_PACK_PRIMITIVE(PtpInteger8)
PTP_PACK_PRIMITIVE(PtpUInteger8)
PTP_PACK_PRIMITIVE(PtpEnumeration8)

PTP_PACK_MULTIBYTE(PtpInteger16, 16)
PTP_PACK_MULTIBYTE(PtpInteger32, 32)
PTP_PACK_MULTIBYTE(PtpUInteger16, 16)
PTP_PACK_MULTIBYTE(PtpUInteger32, 32)
PTP_PACK_MULTIBYTE(PtpEnumeration16, 16)

PTP_PACK_NIBBLETYPE(PtpEnumeration4)
PTP_PACK_NIBBLETYPE(PtpUInteger4)
PTP_PACK_NIBBLETYPE(PtpNibble)

void
unpackPtpUInteger48( PtpUInteger48 *to, char *from, size_t len)
{
	unpackPtpUInteger16(&to->high, from, len);
	unpackPtpUInteger32(&to->low, from + 2, len);
}

void
packPtpUInteger48(char *to, PtpUInteger48 *from, size_t len)
{
	packPtpUInteger16(to, &from->high, len);
	packPtpUInteger32(to + 2, &from->low, len);
}

void
unpackPtpInteger64(PtpInteger64 *to, char *from, size_t len)
{
	unpackPtpInteger32(&to->high, from, len);
	unpackPtpUInteger32(&to->low, from + 4, len);
}

void
packPtpInteger64(char *to, PtpInteger64 *from, size_t len)
{
	packPtpInteger32(to, &from->high, len);
	packPtpUInteger32(to + 4, &from->low, len);
}


void displayPtpBoolean (PtpBoolean var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (Boolean)\t: %s\n", name, var ? "TRUE" : "FALSE");
    }
}
void displayPtpOctet (PtpOctet var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (Octet)\t: 0x%02x\n", name, (uint8_t)var);
    }
}
void displayPtpInteger8 (PtpInteger8 var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (Integer8)\t: %03d (0x%02x)\n", name, var, var);
    }
}
void displayPtpInteger16 (PtpInteger16 var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (Integer16)\t: %06d (0x%04x)\n", name, var, var);
    }
}
void displayPtpInteger32 (PtpInteger32 var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (Integer32)\t: %012ld (0x%08x)\n", name, (long int)var, var);
    }
}
void displayPtpUInteger8 (PtpUInteger8 var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (UInteger8)\t: %03u (0x%02x)\n", name, var, var);
    }
}
void displayPtpUInteger16 (PtpUInteger16 var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (UInteger16)\t: %06u (0x%04x)\n", name, var, var);
    }
}
void displayPtpUInteger32 (PtpUInteger32 var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (UInteger32)\t: %012lu (0x%08x)\n", name, (unsigned long int)var, var);
    }
}
void displayPtpEnumeration16 (PtpEnumeration16 var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (Enumeration16)\t: %06u (0x%04x)\n", name, var, var);
    }
}
void displayPtpEnumeration8 (PtpEnumeration8 var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (Enumeration8)\t: %03u (0x%02x)\n", name, var, var);
    }
}

void displayPtpEnumeration4Upper (PtpEnumeration4Upper var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (Enumeration4Upper)\t: %03u (0x%02x)\n", name, var, var);
    }
}
void displayPtpEnumeration4Lower (PtpEnumeration4Lower var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (Enumeration4Lower)\t: %03u (0x%02x)\n", name, var, var);
    }
}

void displayPtpUInteger4Upper (PtpUInteger4Upper var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (UInteger4Upper)\t: %03u (0x%02x)\n", name, var, var);
    }
}
void displayPtpUInteger4Lower (PtpUInteger4Lower var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (UInteger4Lower)\t: %03u (0x%02x)\n", name, var, var);
    }
}

void displayPtpNibbleUpper (PtpNibbleUpper var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (NibbleUpper)\t: %03u (0x%02x)\n", name, var, var);
    }
}
void displayPtpNibbleLower (PtpNibbleLower var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (NibbleLower)\t: %03u (0x%02x)\n", name, var, var);
    }
}

void displayPtpUInteger48 (PtpUInteger48 var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (UInteger48).high\t: %06u (0x%04x)\n", name, var.high, var.high);
	PTPINFO("%s (UInteger48).low\t: %012lu (0x%08x)\n", name, (unsigned long int)var.low, var.low);
    }
}

void displayPtpInteger64 (PtpInteger64 var, const char *name, size_t len) {
    if(name != NULL) {
	PTPINFO("%s (Integer64).high\t: %012ld (0x%08x)\n", name, (long int)var.high, var.high);
	PTPINFO("%s (Integer64).low\t: %012lu (0x%08x)\n", name, (unsigned long int)var.low, var.low);
    }
}

void freePtpUInteger48 (PtpUInteger48 *var) {
}

void freePtpInteger64 (PtpInteger64 *var) {
}

void packPtpOctetBuf(char *to, PtpOctetBuf *from, size_t len) {
    memcpy(to, *from, len);
}

void unpackPtpOctetBuf(PtpOctetBuf *to, char *from, size_t len) {
    memcpy(*to, from, len);
}

void displayPtpOctetBuf(PtpOctetBuf var, const char *name, size_t len) {

    char buf[2 * len + 1];
    uint8_t byte;
    int i = 0;

    memset(buf, 0, sizeof(buf));

    for(i=0; i < len; i++) {
	byte = *((uint8_t*) var);
	snprintf(&buf[2 * i], 3, "%02x", byte);
	var++;
    }

    if(name != NULL) {
	PTPINFO("%s (Octet: %d)\t: %s\n", name, (int)len, buf);
    }

}

void freePtpOctetBuf(PtpOctetBuf *var) {
}

void packPtpDynamicOctetBuf(char *to, PtpDynamicOctetBuf *from, size_t len) {
    memcpy(to, *from, len);
}

void unpackPtpDynamicOctetBuf(PtpDynamicOctetBuf *to, char *from, size_t len) {

    /* allocate an extra byte for string types to have a trailing zero */
    *to = calloc(1, len + 1);

    if(*to == NULL) {
	return;
    }

    memcpy(*to, from, len);
}

void displayPtpDynamicOctetBuf(PtpDynamicOctetBuf var, const char *name, size_t len) {
    displayPtpOctetBuf(var, name, len);
}

void freePtpDynamicOctetBuf(PtpDynamicOctetBuf *var) {

    if(*var != NULL) {
	free(*var);
    }
    *var = NULL;
}

void packPtpSharedOctetBuf(char *to, PtpSharedOctetBuf *from, size_t len) {
    /* this is already in its place */
}

void unpackPtpSharedOctetBuf(PtpSharedOctetBuf *to, char *from, size_t len) {

    /* point the destination to the source */
    *to = from;
}

void displayPtpSharedOctetBuf(PtpSharedOctetBuf var, const char *name, size_t len) {
    displayPtpOctetBuf(var, name, len);
}

void freePtpSharedOctetBuf(PtpSharedOctetBuf *var) {
}
