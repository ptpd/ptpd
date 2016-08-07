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


void unpackPtpTimeInterval(PtpTimeInterval *data, char *buf, size_t len) {

    int offset = 0;
    int sign = 1;
    int64_t sns;

    #define PROCESS_FIELD( name, size, type) \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/derivedData/timeInterval.def"

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

void packPtpTimeInterval(char *buf, PtpTimeInterval *data, size_t len) {

    int offset = 0;
    int64_t sns;

    sns = data->internalTime.seconds * 1000000000;
    sns += data->internalTime.nanoseconds;
    sns <<= 16;

    data->scaledNanoseconds.high = (sns >> 32) & 0x00000000ffffffff;
    data->scaledNanoseconds.low = sns & 0x00000000ffffffff;

    #define PROCESS_FIELD( name, size, type) \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/derivedData/timeInterval.def"

}

void displayPtpTimeInterval(PtpTimeInterval data, const char *name, size_t len) {

    if(name != NULL) {
	PTPINFO("%s (TimeInterval):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data.name, "\t\t\t"#name, size);
    #include "def/derivedData/timeInterval.def"

	displayPtpTimeInternal(data.internalTime, "\t\tinternalTime");

    }

}


void freePtpTimeInterval(PtpTimeInterval *data) {
}

void unpackPtpTimestamp(PtpTimestamp *data, char *buf, size_t len) {

    int offset = 0;

    #define PROCESS_FIELD( name, size, type) \
        unpack##type (&data->name, buf + offset, size); \
	offset += size;
    #include "def/derivedData/timestamp.def"

    data->internalTime.seconds = data->secondsField.low;
    data->internalTime.nanoseconds = data->nanosecondsField;

}

void packPtpTimestamp(char *buf, PtpTimestamp *data, size_t len) {

    int offset = 0;

    data->secondsField.high = 0;
    data->secondsField.low = data->internalTime.seconds;
    data->nanosecondsField = data->internalTime.nanoseconds;

    #define PROCESS_FIELD( name, size, type) \
        pack##type (buf + offset, &data->name, size); \
	offset += size;
    #include "def/derivedData/timestamp.def"

}

void displayPtpTimestamp(PtpTimestamp data, const char *name, size_t len) {

    if(name != NULL) {
	PTPINFO("%s (Timestamp):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data.name, "\t\t\t"#name, size);
    #include "def/derivedData/timestamp.def"

	displayPtpTimeInternal(data.internalTime, "\t\tinternalTime");

    }

}

void freePtpTimestamp(PtpTimestamp *data) {
}

void unpackPtpClockIdentity(PtpClockIdentity *data, char *buf, size_t len) {
    memcpy(*data, buf, 8);
}

void packPtpClockIdentity(char *buf, PtpClockIdentity *data, size_t len) {
    memcpy(buf, *data, 8);
}

void displayPtpClockIdentity(PtpClockIdentity data, const char *name, size_t len) {

    if(name != NULL) {
	displayPtpOctetBuf((PtpOctetBuf)data, name, 8);
    }

}

void freePtpClockIdentity(PtpClockIdentity *data) {
}

void unpackPtpPortIdentity(PtpPortIdentity *data, char *buf, size_t len) {

    int offset = 0;

    #include "def/field_unpack_nocheck.h"
    #include "def/derivedData/portIdentity.def"
}

void packPtpPortIdentity(char *buf, PtpPortIdentity *data, size_t len) {

    int offset = 0;

    #include "def/field_pack_nocheck.h"
    #include "def/derivedData/portIdentity.def"

}

void displayPtpPortIdentity(PtpPortIdentity data, const char *name, size_t len) {

    if(name != NULL) {
	PTPINFO("%s (PortIdentity):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data.name, "\t\t"#name, size);
    #include "def/derivedData/portIdentity.def"

    }

}

void freePtpPortIdentity(PtpPortIdentity *data) {
}

void unpackPtpPortAddress(PtpPortAddress *data, char *buf, size_t len) {

    int offset = 0;

    #include "def/field_unpack_nocheck.h"
    #include "def/derivedData/portAddress.def"
}

void packPtpPortAddress(char *buf, PtpPortAddress *data, size_t len) {

    int offset = 0;

    #include "def/field_pack_nocheck.h"
    #include "def/derivedData/portAddress.def"

}

void displayPtpPortAddress(PtpPortAddress a, const char *name, size_t len) {

    PtpPortAddress *data = &a;

    if(name != NULL) {
	PTPINFO("%s (PortAddress):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t\t"#name, size);
    #include "def/derivedData/portAddress.def"

    }

}

void freePtpPortAddress(PtpPortAddress *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/derivedData/portAddress.def"

}

void unpackPtpClockQuality(PtpClockQuality *data, char *buf, size_t len) {

    int offset = 0;

    #include "def/field_unpack_nocheck.h"
    #include "def/derivedData/clockQuality.def"
}

void packPtpClockQuality(char *buf, PtpClockQuality *data, size_t len) {

    int offset = 0;

    #include "def/field_pack_nocheck.h"
    #include "def/derivedData/clockQuality.def"

}

void displayPtpClockQuality(PtpClockQuality data, const char *name, size_t len) {

    if(name != NULL) {
	PTPINFO("%s (ClockQuality):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data.name, "\t\t"#name, size);
    #include "def/derivedData/clockQuality.def"

    }

}

void freePtpClockQuality(PtpClockQuality *data) {
}

void unpackPtpTimePropertiesDS(PtpTimePropertiesDS *data, char *buf, size_t len) {

    int offset = 0;

    #include "def/field_unpack_nocheck.h"
    #include "def/derivedData/timePropertiesDS.def"
}

void packPtpTimePropertiesDS(char *buf, PtpTimePropertiesDS *data, size_t len) {

    int offset = 0;

    #include "def/field_pack_nocheck.h"
    #include "def/derivedData/timePropertiesDS.def"

}

void displayPtpTimePropertiesDS(PtpTimePropertiesDS data, const char *name, size_t len) {

    if(name != NULL) {
	PTPINFO("%s (TimePropertiesDS):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data.name, "\t\t"#name, size);
    #include "def/derivedData/timePropertiesDS.def"

    }

}

void freePtpTimePropertiesDS(PtpTimePropertiesDS *data) {
}

void unpackPtpText(PtpText *data, char *buf, size_t len) {

    int offset = 0;

    #include "def/field_pack_nocheck.h"
    #include "def/derivedData/ptpText.def"

}

void packPtpText(char *buf, PtpText *data, size_t len) {

    int offset = 0;

    #include "def/field_pack_nocheck.h"
    #include "def/derivedData/ptpText.def"
}

void displayPtpText(PtpText t, const char *name, size_t len) {

    PtpText *data = &t;

    if(name != NULL) {
	PTPINFO("%s (PtpText):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t\t"#name, size);
    #include "def/derivedData/ptpText.def"

	PTPINFO("\t\t\ttext: \t%s\n", data->textField);

    }

}

void freePtpText(PtpText *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/derivedData/ptpText.def"

}

/* faultRecord */

void unpackPtpPhysicalAddress(PtpPhysicalAddress *data, char *buf, size_t len) {

    int offset = 0;

    #include "def/field_unpack_nocheck.h"
    #include "def/derivedData/physicalAddress.def"
}

void packPtpPhysicalAddress(char *buf, PtpPhysicalAddress *data, size_t len) {

    int offset = 0;

    #include "def/field_pack_nocheck.h"
    #include "def/derivedData/physicalAddress.def"

}

void displayPtpPhysicalAddress(PtpPhysicalAddress a, const char *name, size_t len) {

    PtpPhysicalAddress *data = &a;

    if(name != NULL) {
	PTPINFO("%s (PtpPhysicalAddress):\n", name);
    #define PROCESS_FIELD( name, size, type) \
        display##type (data->name, "\t\t\t"#name, size);
    #include "def/derivedData/physicalAddress.def"

    }

}

void freePtpPhysicalAddress(PtpPhysicalAddress *data) {

    #define PROCESS_FIELD( name, size, type) \
        free##type (&data->name);
    #include "def/derivedData/physicalAddress.def"

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

