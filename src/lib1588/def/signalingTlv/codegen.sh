#!/bin/bash

. ./codegen.conf

# variables used for code generation

CODEFILE="../../ptp_tlv_${TLVGROUPNAME}.c"
HEADERFILE="../../ptp_tlv_${TLVGROUPNAME}.h"

BASECODEFILE=`basename $CODEFILE`
BASEHEADERFILE=`basename $HEADERFILE`
TLVGROUP="${TLVGROUPNAME}Tlv"
TLVGROUPDEFNAME=`echo $TLVGROUPNAME | awk '{ print toupper($0);}'`
TLVTYPEPREFIX=`echo $TLVGROUP | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`


function gettype() {

type =`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

}

# ************** header code

{

# **************** header prefix

read -r -d '' code <<EOF
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

#ifndef PTP_TLV_${TLVGROUPDEFNAME}_H_
#define PTP_TLV_${TLVGROUPDEFNAME}_H_

#include "ptp_tlv.h"
EOF
echo "$code"
echo
echo "/* begin generated code */"
echo

# *********** TLV data types

echo "/* $TLVGROUPNAME TLV data types */"

for file in `ls *.def | sed 's/\\.def//g'`; do

type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

echo
read -r -d '' code <<EOF
typedef struct {
	#define PROCESS_FIELD( name, size, type ) type name;
	#include "def/$TLVGROUP/$file.def"
} PtpTlv$type;
EOF
echo "$code"

done

# **************** header suffix

read -r -d '' code <<EOF
/* end generated code */

#endif /* PTP_TLV_${TLVGROUPDEFNAME}_H */
EOF
echo
echo "$code"
} > $HEADERFILE

# ************** C code
{
# **************** code prefix
read -r -d '' code << "EOF"

/*-
 * Copyright (c) 2016-2017      Wojciech Owczarek,
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
#include "ptp_tlv.h"
#include "ptp_message.h"

#include "tmp.h"

#include <stdlib.h>

EOF

echo -e "$code\n" > $CODEFILE


# ************** Funcdefs for individual datatype packers / unpackers


echo "/* begin generated code */"
echo
echo "#define PTP_TYPE_FUNCDEFS( type ) \\"
echo "static int unpack##type (type *data, char *buf, char *boundary); \\"
echo "static int pack##type (char *buf, type *data, char *boundary); \\"
echo "static void display##type (type *data); \\"
echo "static void free##type (type *var);"
echo
for file in `ls *.def | sed 's/\\.def//g' `; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`
echo "PTP_TYPE_FUNCDEFS(PtpTlv$type)"

done
echo
echo "#undef PTP_TYPE_FUNCDEFS"
echo

} >> $CODEFILE


# ************** Individual datatype packers / unpackers

for file in `ls *.def | sed 's/\\.def//g' `; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`
export file
export type

read -r -d '' code <<EOF

static int unpackPtpTlv$type(PtpTlv$type *data, char *buf, char *boundary) {

    int offset = 0;
    #define PROCESS_FIELD( name, size, type) \\
	if((buf + offset + size) > boundary) { \\
	    return PTP_MESSAGE_BUFFER_TOO_SMALL; \\
	} \\
        unpack##type (&data->name, buf + offset, size); \\
	offset += size;
    #include "def/$TLVGROUP/$file.def"

    return offset;
}

static int packPtpTlv$type(char *buf, PtpTlv$type *data, char *boundary) {

    int offset = 0;


    /* if no boundary and buffer provided, we only report the length. */
    /* this is used to help pre-allocate memory to fit this */
    #define PROCESS_FIELD( name, size, type) \\
	if((buf == NULL) && (boundary == NULL)) {\\
	    offset += size; \\
	} else { \\
	     if((buf + offset + size) > boundary) { \\
		return PTP_MESSAGE_BUFFER_TOO_SMALL; \\
	    } \\
	    pack##type (buf + offset, &data->name, size); \\
	    offset += size; \\
	}
    #include "def/$TLVGROUP/$file.def"
    return offset;
}

static void freePtpTlv$type(PtpTlv$type *data) {

    #define PROCESS_FIELD( name, size, type) \\
        free##type (&data->name);
    #include "def/$TLVGROUP/$file.def"

}

static void displayPtpTlv$type(PtpTlv$type *data) {

    PTPINFO("\tPtpTlv$type: \n");

    #define PROCESS_FIELD( name, size, type) \\
        display##type (data->name, "\t\t"#name, size);
    #include "def/$TLVGROUP/$file.def"
}

EOF

echo "$code" >> $CODEFILE


done

# ********* Unpacking wrapper

# prefix
read -r -d '' code <<EOF
int unpackPtp${TLVTYPEPREFIX}Data(PtpTlv *tlv, char* buf, char* boundary) {

    int ret = 0;
    int expectedLen = -1;

    switch(tlv->tlvType) {

EOF

echo "$code" >> $CODEFILE


for file in `ls *.def | sed 's/\\.def//g'`; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

line=`grep @$DEFTAG@ $file.def` && {

tlvtype=`echo "$line" | awk ' BEGIN{ FS="@";} { print $3; }'`
tlvid=`echo "$line" | awk ' BEGIN{ FS="@";} { print $4; }'`

# case body
read -r -d '' code <<EOF
	case PTP_TLVTYPE_${tlvtype}:
	    #ifdef PTP_TLVLEN_${tlvtype}
	    expectedLen = PTP_TLVLEN_${tlvtype};
	    #endif
	    ret = unpackPtpTlv$type(&tlv->body.$file, buf, boundary);
	    break;
EOF

echo -e "\t$code"

} >> $CODEFILE

done

#suffix
read -r -d '' code <<EOF
	default:
	    PTPDEBUG("${BASECODEFILE}:unpackPtp${TLVTYPEPREFIX}Data(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    ret = PTP_MESSAGE_UNSUPPORTED_TLV;
	    break;
    }

    if (ret < 0 ) {
	return ret;
    }

    if(expectedLen >= 0) {
	if(tlv->lengthField < expectedLen) {
	    return PTP_MESSAGE_CORRUPT_TLV;
	}

	if (ret < expectedLen) {
	    return PTP_MESSAGE_BUFFER_TOO_SMALL;
	}
    }


    return ret;

}


EOF

echo -e "\t$code" >> $CODEFILE

# ********* packing wrapper

# prefix
read -r -d '' code <<EOF
int packPtp${TLVTYPEPREFIX}Data(char *buf, PtpTlv *tlv, char *boundary) {

    int ret = 0;
    int expectedLen = -1;

    switch(tlv->tlvType) {
EOF

echo "$code" >> $CODEFILE


for file in `ls *.def | sed 's/\\.def//g'`; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

line=`grep @$DEFTAG@ $file.def` && {

tlvtype=`echo "$line" | awk ' BEGIN{ FS="@";} { print $3; }'`
tlvid=`echo "$line" | awk ' BEGIN{ FS="@";} { print $4; }'`


# case body
read -r -d '' code <<EOF

	case PTP_TLVTYPE_${tlvtype}:
	    #ifdef PTP_TLVLEN_${tlvtype}
	    expectedLen = PTP_TLVLEN_${tlvtype};
	    #endif
	    ret = packPtpTlv$type(buf, &tlv->body.$file, boundary);
	    break;
EOF

echo -e "\t$code"

} >> $CODEFILE

done

# suffix
read -r -d '' code <<EOF
	default:
	    ret = PTP_MESSAGE_UNSUPPORTED_TLV;
	    PTPDEBUG("${BASECODEFILE}:packPtp${TLVTYPEPREFIX}Data(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    break;
    }

    /* no buffer and boundary given - only report length */
    if((buf == NULL) && (boundary == NULL)) {
	return ret;
    }

    if (ret < 0 ) {
	return ret;
    }

    if(expectedLen >= 0) {
	if(tlv->lengthField < expectedLen) {
	    return PTP_MESSAGE_CORRUPT_TLV;
	}

	if (ret < expectedLen) {
	    return PTP_MESSAGE_BUFFER_TOO_SMALL;
	}
    }

    if (ret < tlv->lengthField) {
	return PTP_MESSAGE_BUFFER_TOO_SMALL;
    }

    return ret;

}
EOF

echo -e "\t$code" >> $CODEFILE

# ************* display wrapper

# prefix
read -r -d '' code <<EOF
void displayPtp${TLVTYPEPREFIX}Data(PtpTlv *tlv) {

    switch(tlv->tlvType) {
EOF

echo "$code" >> $CODEFILE


for file in `ls *.def | sed 's/\\.def//g'`; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

line=`grep @$DEFTAG@ $file.def` && {

tlvtype=`echo "$line" | awk ' BEGIN{ FS="@";} { print $3; }'`
tlvid=`echo "$line" | awk ' BEGIN{ FS="@";} { print $4; }'`

# case body
read -r -d '' code <<EOF
	case PTP_TLVTYPE_${tlvtype}:
	    displayPtpTlv$type(&tlv->body.$file);
	    break;
EOF

echo -e "\t$code"

} >> $CODEFILE

done

# suffix
read -r -d '' code <<EOF
	default:
	    PTPINFO("${BASECODEFILE}:displayPtp${TLVTYPEPREFIX}Data(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    break;
    }

}

EOF

echo -e "\t$code" >> $CODEFILE


#************** free wrapper

#prefix
read -r -d '' code <<EOF
void freePtp${TLVTYPEPREFIX}Data(PtpTlv *tlv) {

    switch(tlv->tlvType) {
EOF

echo "$code" >> $CODEFILE


for file in `ls *.def | sed 's/\\.def//g'`; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

line=`grep @$DEFTAG@ $file.def` && {

tlvtype=`echo "$line" | awk ' BEGIN{ FS="@";} { print $3; }'`
tlvid=`echo "$line" | awk ' BEGIN{ FS="@";} { print $4; }'`

# case body
read -r -d '' code <<EOF
	case PTP_TLVTYPE_${tlvtype}:
	    freePtpTlv$type(&tlv->body.$file);
	    break;
EOF

echo -e "\t$code"


} >> $CODEFILE

done

# suffix
read -r -d '' code <<EOF
	default:
	    PTPDEBUG("${BASECODEFILE}:freePtp${TLVTYPEPREFIX}Data(): Unsupported TLV type %04x\n",
		tlv->tlvType);
	    break;

    }

}

EOF

echo -e "\t$code" >> $CODEFILE

{

echo "/* end generated code */"
echo

} >> $CODEFILE
