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

#include "ptp_derived_types.h"
#include "tmp.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

void displayPtpTimeInternal(PtpTimeInternal data, const char *name) {

    int negative = 0;

    if((data.seconds < 0) || (data.nanoseconds < 0)) {
	negative = 1;
    }

    PTPINFO("\t %s (TimeInternal)\t: %s%.09ld.%09ld\n",
	name,
	negative ? "-" : " ",
	labs(data.seconds),
	labs(data.nanoseconds)
    );
}


void unpackPtpTimeInterval(PtpTimeInterval *to, char *from, size_t len) {

    PtpTimeInterval *data = (PtpTimeInterval*) to;
    int offset = 0;
    int sign = 1;
    int64_t sns;

    #define PROCESS_FIELD( name, size, type) \
        unpack##type (&data->name, from + offset, size); \
	offset += size;
    #include "definitions/derivedData/timeInterval.def"

    sns = data->scaledNanoseconds.high;
    sns <<= 32;
    sns += data->scaledNanoseconds.low;

    if(sns < 0) {
	sign = -1;
    }

    sns >>= 16;

    data->internalTime.seconds = sign * (sns / 1000000000);
    data->internalTime.nanoseconds = sign * (sns % 1000000000);

}

void packPtpTimeInterval(char *to, PtpTimeInterval *from, size_t len) {

    PtpTimeInterval *data = (PtpTimeInterval*) to;
    int offset = 0;
    int64_t sns;

    sns = data->internalTime.seconds * 1000000000;
    sns += data->internalTime.nanoseconds;
    sns <<= 16;

    data->scaledNanoseconds.high = (sns >> 32) & 0x00000000ffffffff;
    data->scaledNanoseconds.low = sns & 0x00000000ffffffff;

    #define PROCESS_FIELD( name, size, type) \
        pack##type (to + offset, &data->name, size); \
	offset += size;
    #include "definitions/derivedData/timeInterval.def"

}

void displayPtpTimeInterval(PtpTimeInterval data, const char *name, size_t len) {

    if(name != NULL) {
	PTPINFO("%s (TimeInterval):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data.name, "\t\t"#name, size);
    #include "definitions/derivedData/timeInterval.def"

	displayPtpTimeInternal(data.internalTime, "\tinternalTime");

    }

}


void freePtpTimeInterval(PtpTimeInterval *var) {
}

void unpackPtpTimestamp(PtpTimestamp *to, char *from, size_t len) {

    PtpTimestamp *data = (PtpTimestamp*) to;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        unpack##type (&data->name, from + offset, size); \
	offset += size;
    #include "definitions/derivedData/timestamp.def"

    data->internalTime.seconds = data->secondsField.low;
    data->internalTime.nanoseconds = data->nanosecondsField;

}

void packPtpTimestamp(char *to, PtpTimestamp *from, size_t len) {

    PtpTimestamp *data = (PtpTimestamp*) from;
    int offset = 0;

    data->secondsField.high = 0;
    data->secondsField.low = data->internalTime.seconds;
    data->nanosecondsField = data->internalTime.nanoseconds;

    #define PROCESS_FIELD( name, size, type) \
        pack##type (to + offset, &data->name, size); \
	offset += size;
    #include "definitions/derivedData/timestamp.def"

}

void displayPtpTimestamp(PtpTimestamp data, const char *name, size_t len) {

    if(name != NULL) {
	PTPINFO("%s (Timestamp):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data.name, "\t\t"#name, size);
    #include "definitions/derivedData/timestamp.def"

	displayPtpTimeInternal(data.internalTime, "\tinternalTime");

    }

}

void freePtpTimestamp(PtpTimestamp *var) {
}

void unpackPtpClockIdentity(PtpClockIdentity *to, char *from, size_t len) {
    memcpy(*to, from, 8);
}

void packPtpClockIdentity(char *to, PtpClockIdentity *from, size_t len) {
    memcpy(to, *from, 8);
}

void displayPtpClockIdentity(PtpClockIdentity data, const char *name, size_t len) {

    if(name != NULL) {
	displayPtpOctetBuf((PtpOctetBuf)data, name, 8);
    }

}

void freePtpClockIdentity(PtpClockIdentity *var) {
}

void unpackPtpPortIdentity(PtpPortIdentity *to, char *from, size_t len) {

    PtpPortIdentity *data = (PtpPortIdentity*) to;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        unpack##type (&data->name, from + offset, size); \
	offset += size;
    #include "definitions/derivedData/portIdentity.def"
}

void packPtpPortIdentity(char *to, PtpPortIdentity *from, size_t len) {

    PtpPortIdentity *data = (PtpPortIdentity*) from;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        pack##type (to + offset, &data->name, size); \
	offset += size;
    #include "definitions/derivedData/portIdentity.def"

}

void displayPtpPortIdentity(PtpPortIdentity data, const char *name, size_t len) {

    if(name != NULL) {
	PTPINFO("%s (PortIdentity):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data.name, "\t\t"#name, size);
    #include "definitions/derivedData/portIdentity.def"

    }

}

void freePtpPortIdentity(PtpPortIdentity *var) {
}

void unpackPtpPortAddress(PtpPortAddress *to, char *from, size_t len) {

    PtpPortAddress *data = (PtpPortAddress*) to;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        unpack##type (&data->name, from + offset, size); \
	offset += size;
    #include "definitions/derivedData/portAddress.def"
}

void packPtpPortAddress(char *to, PtpPortAddress *from, size_t len) {

    PtpPortAddress *data = (PtpPortAddress*) from;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        pack##type (to + offset, &data->name, size); \
	offset += size;
    #include "definitions/derivedData/portAddress.def"

}

void displayPtpPortAddress(PtpPortAddress a, const char *name, size_t len) {

    PtpPortAddress *data = &a;

    if(name != NULL) {
	PTPINFO("%s (PortAddress):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t\t"#name, size);
    #include "definitions/derivedData/portAddress.def"

    }

}

void freePtpPortAddress(PtpPortAddress *var) {

    PtpPortAddress *data = var;

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/derivedData/portAddress.def"

}

void unpackPtpClockQuality(PtpClockQuality *to, char *from, size_t len) {

    PtpClockQuality *data = (PtpClockQuality*) to;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        unpack##type (&data->name, from + offset, size); \
	offset += size;
    #include "definitions/derivedData/clockQuality.def"
}

void packPtpClockQuality(char *to, PtpClockQuality *from, size_t len) {

    PtpClockQuality *data = (PtpClockQuality*) from;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        pack##type (to + offset, &data->name, size); \
	offset += size;
    #include "definitions/derivedData/clockQuality.def"

}

void displayPtpClockQuality(PtpClockQuality data, const char *name, size_t len) {

    if(name != NULL) {
	PTPINFO("%s (ClockQuality):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data.name, "\t\t"#name, size);
    #include "definitions/derivedData/clockQuality.def"

    }

}

void freePtpClockQuality(PtpClockQuality *var) {
}

void unpackPtpTimePropertiesDS(PtpTimePropertiesDS *to, char *from, size_t len) {

    PtpTimePropertiesDS *data = (PtpTimePropertiesDS*) to;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        unpack##type (&data->name, from + offset, size); \
	offset += size;
    #include "definitions/derivedData/timePropertiesDS.def"
}

void packPtpTimePropertiesDS(char *to, PtpTimePropertiesDS *from, size_t len) {

    PtpTimePropertiesDS *data = (PtpTimePropertiesDS*) from;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        pack##type (to + offset, &data->name, size); \
	offset += size;
    #include "definitions/derivedData/timePropertiesDS.def"

}

void displayPtpTimePropertiesDS(PtpTimePropertiesDS data, const char *name, size_t len) {

    if(name != NULL) {
	PTPINFO("%s (TimePropertiesDS):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data.name, "\t\t"#name, size);
    #include "definitions/derivedData/timePropertiesDS.def"

    }

}

void freePtpTimePropertiesDS(PtpTimePropertiesDS *var) {
}

void unpackPtpText(PtpText *to, char *from, size_t len) {

    PtpText *data = (PtpText*) to;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        unpack##type (&data->name, from + offset, size); \
	offset += size;
    #include "definitions/derivedData/ptpText.def"
}

void packPtpText(char *to, PtpText *from, size_t len) {

    PtpText *data = (PtpText*) from;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        pack##type (to + offset, &data->name, size); \
	offset += size;
    #include "definitions/derivedData/ptpText.def"
}

void displayPtpText(PtpText t, const char *name, size_t len) {

    PtpText *data = &t;

    if(name != NULL) {
	PTPINFO("%s (PtpText):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t\t"#name, size);
    #include "definitions/derivedData/ptpText.def"

	PTPINFO("\t\t\ttext: \t%s\n", data->textField);

    }

}

void freePtpText(PtpText *var) {

    PtpText *data = var;

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/derivedData/ptpText.def"

}

/* faultRecord */

void unpackPtpPhysicalAddress(PtpPhysicalAddress *to, char *from, size_t len) {

    PtpPhysicalAddress *data = (PtpPhysicalAddress*) to;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        unpack##type (&data->name, from + offset, size); \
	offset += size;
    #include "definitions/derivedData/physicalAddress.def"
}

void packPtpPhysicalAddress(char *to, PtpPhysicalAddress *from, size_t len) {

    PtpPhysicalAddress *data = (PtpPhysicalAddress*) from;
    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        pack##type (to + offset, &data->name, size); \
	offset += size;
    #include "definitions/derivedData/physicalAddress.def"

}

void displayPtpPhysicalAddress(PtpPhysicalAddress a, const char *name, size_t len) {

    PtpPhysicalAddress *data = &a;

    if(name != NULL) {
	PTPINFO("%s (PtpPhysicalAddress):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t\t"#name, size);
    #include "definitions/derivedData/physicalAddress.def"

    }

}

void freePtpPhysicalAddress(PtpPhysicalAddress *var) {

    PtpPhysicalAddress *data = var;

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "definitions/derivedData/physicalAddress.def"

}

PtpText createPtpTextLen(const char *text, size_t len) {

    PtpText t;

    t.lengthField = len;
    t.textField = calloc(1, len +1);

    memcpy(t.textField, text, len);

    return t;

}

PtpText createPtpText(const char *text) {

    PtpText t;

    t.lengthField = strlen(text);
    t.textField = strdup(text);

    return t;

}

