#!/bin/bash

CODEFILE="../../ptp_tlv_management.c"
HEADERFILE="../../ptp_tlv_management.h"



# ************** header code


{

# **************** header prefix

read -r -d '' code <<"EOF"
/*-
 * Copyright (c) 2016-2017              Wojciech Owczarek,
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

#ifndef PTP_TLV_MANAGEMENT_H_
#define PTP_TLV_MANAGEMENT_H_

#include "ptp_tlv.h"

/* management TLV dataField offset from TLV valueField */
#define PTP_MTLV_DATAFIELD_OFFSET	2
/* length of an empty TLV (managementID only) */
#define PTP_MTLVLEN_EMPTY		2

EOF
echo "$code"
echo
echo "/* begin generated code */"
echo


# ************  mgmtId enum

echo "/* Table 40: ManagementId values */"
echo
echo "enum {"
echo

for file in `ls *.def | sed 's/\\.def//g' | grep -v "^management"`; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

line=`grep @mgmtid@ $file.def` && {

tlvtype=`echo "$line" | awk ' BEGIN{ FS="@";} { print $3; }'`
tlvid=`echo "$line" | awk ' BEGIN{ FS="@";} { print $4; }'`

echo "	$tlvtype = $tlvid,"

}

done
echo
echo "};"
echo

# *********** TLV data types

echo "/* management TLV data types */"

for file in `ls *.def | sed 's/\\.def//g' | grep -v "^management"`; do

type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

echo
read -r -d '' code <<EOF
typedef struct {
	#include "def/field_declare.h"
	#include "def/managementTlv/$file.def"
	#undef PROCESS_FIELD
} PtpTlv$type;
EOF
echo "$code"

done

# ************** TLV union container

echo
echo "/* management TLV bodybag */"
echo

echo "typedef union {"
echo
for file in `ls *.def | sed 's/\\.def//g' | grep -v "^management"`; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`
echo "		PtpTlv$type $file;"
done
echo
echo "} PtpManagementTlvBody;"
echo

# **************** header suffix

read -r -d '' code <<"EOF"

/* Management TLV container */

typedef struct {

    PtpUInteger16 lengthField; /* from parent PtpTlv */

    #include "def/field_declare.h"
    #include "def/managementTlv/management.def"
    #undef PROCESS_FIELD

    PtpManagementTlvBody body;
    PtpBoolean empty;

} PtpTlvManagement;

typedef struct {

    #include "def/field_declare.h"
    #include "def/managementTlv/managementErrorStatus.def"
    #undef PROCESS_FIELD

} PtpTlvManagementErrorStatus;

/* end generated code */

#endif /* PTP_TLV_SIGNALING_H */


EOF

echo "$code"

} > $HEADERFILE

# ************** C code
{
# **************** code prefix
read -r -d '' code <<"EOF"

/*-
 * Copyright (c) 2016      Wojciech Owczarek,
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

    #include "def/field_unpack_bufcheck.h"
    #include "def/managementTlv/$file.def"
    #undef PROCESS_FIELD

    return offset;
}

static int packPtpTlv$type(char *buf, PtpTlv$type *data, char *boundary) {

    int offset = 0;

    #include "def/field_pack_bufcheck.h"
    #include "def/managementTlv/$file.def"
    #undef PROCESS_FIELD
    return offset;
}

static void freePtpTlv$type(PtpTlv$type *data) {

    #define PROCESS_FIELD( name, size, type) \\
        free##type (&data->name);
    #include "def/managementTlv/$file.def"
    #undef PROCESS_FIELD
}

static void displayPtpTlv$type(PtpTlv$type *data) {

    PTPINFO("\tPtpTlv$type: \n");

    #define PROCESS_FIELD( name, size, type) \\
        display##type (data->name, "\t\t"#name, size);
    #include "def/managementTlv/$file.def"
    #undef PROCESS_FIELD
}

EOF

echo "$code" >> $CODEFILE


done

# ********* Unpacking wrapper

read -r -d '' code <<"EOF"

int unpackPtpManagementTlvData(PtpTlv *tlv, char* buf, char* boundary) {

    int ret = 0;

    if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS) {

	ret = unpackPtpTlvManagementErrorStatus( &tlv->body.managementErrorStatus, buf, boundary);

	if(ret != tlv->lengthField) {
	    ret = PTP_MESSAGE_CORRUPT_TLV;
	}

    } else if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT) {

	tlv->body.management.lengthField = tlv->lengthField;

	ret = unpackPtpTlvManagement( &tlv->body.management, buf, boundary);

	if(ret != tlv->lengthField) {
	    ret = PTP_MESSAGE_CORRUPT_TLV;
	} else if (ret == PTP_MTLVLEN_EMPTY) {
	    /* nothing to unpack if this is an empty TLV - and mark this */
	    tlv->body.management.empty = TRUEx;
	} else {
	     switch(tlv->body.management.managementId) {

EOF

echo "$code" >> $CODEFILE


for file in `ls *.def | sed 's/\\.def//g' | grep -v "^management"`; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

line=`grep @mgmtid@ $file.def` && {

tlvtype=`echo "$line" | awk ' BEGIN{ FS="@";} { print $3; }'`
tlvid=`echo "$line" | awk ' BEGIN{ FS="@";} { print $4; }'`

echo "		case $tlvtype:"
echo "			ret = unpackPtpTlv$type( &tlv->body.management.body.$file, tlv->body.management.dataField,"
echo "				tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );"
echo "			break;"

} >> $CODEFILE

done

read -r -d '' code <<"EOF"

		default:
			ret =  PTP_MESSAGE_UNSUPPORTED_TLV;
			PTPDEBUG("ptp_tlv_management.c:unpackPtpManagementTlvData(): Unsupported TLV type %04x\n", tlv->tlvType);
			break;

	    }
	
	    if(ret >= 0) {
		ret += PTP_MTLV_DATAFIELD_OFFSET;
	    }

	}

        if(ret < tlv->lengthField) {
	    ret =  PTP_MESSAGE_CORRUPT_TLV;
	}
	
    } else {

	ret = PTP_MESSAGE_UNSUPPORTED_TLV;

    }
    return ret;

}

EOF

echo "$code" >> $CODEFILE

# ********* packing wrapper

read -r -d '' code <<"EOF"

int packPtpManagementTlvData(char *buf, PtpTlv *tlv, char* boundary) {

    int ret = 0;

    if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS) {

	ret = packPtpTlvManagementErrorStatus( buf, &tlv->body.managementErrorStatus, boundary);

	if(!((buf == NULL) && (boundary == NULL))) {
	    if(ret != tlv->lengthField) {
		ret = PTP_MESSAGE_CORRUPT_TLV;
	    }
	}

    } else if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT) {

	if(tlv->body.management.empty) {
	    ret = 0;
	} else {
	    if(!((buf == NULL) && (boundary == NULL))) {
		tlv->body.management.dataField = calloc(1, tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET);
		if(tlv->body.management.dataField == NULL) {
		    return PTP_MESSAGE_OTHER_ERROR;
		}
	    }

	    switch(tlv->body.management.managementId) {
EOF

echo "$code" >> $CODEFILE


for file in `ls *.def | sed 's/\\.def//g' | grep -v "^management"`; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

line=`grep @mgmtid@ $file.def` && {

tlvtype=`echo "$line" | awk ' BEGIN{ FS="@";} { print $3; }'`
tlvid=`echo "$line" | awk ' BEGIN{ FS="@";} { print $4; }'`

echo "			case $tlvtype:"
echo "				ret = packPtpTlv$type( tlv->body.management.dataField, &tlv->body.management.body.$file,"
echo "					tlv->body.management.dataField + tlv->lengthField - PTP_MTLV_DATAFIELD_OFFSET );"
echo "				break;"

} >> $CODEFILE

done

read -r -d '' code <<"EOF"

		    default:
			    ret =  PTP_MESSAGE_UNSUPPORTED_TLV;
			    PTPDEBUG("ptp_tlv_management.c:packPtpManagementTlvData(): Unsupported management ID %04x\n",
				tlv->body.management.managementId);
		    	    break;

		}
	    }
	    if(ret >= 0) {
		ret += PTP_MTLV_DATAFIELD_OFFSET;
	    }

	    if(!((buf == NULL) && (boundary == NULL))) {

		if(ret != tlv->lengthField) {
		    ret =  PTP_MESSAGE_CORRUPT_TLV;
		} else if (ret >= 0) {
		    tlv->body.management.lengthField = tlv->lengthField;
		    ret = packPtpTlvManagement( tlv->valueField, &tlv->body.management, tlv->valueField + tlv->lengthField);
		    if(ret != tlv->lengthField) {
			ret = PTP_MESSAGE_CORRUPT_TLV;
		    }
		}

	    }

    } else {

	ret = PTP_MESSAGE_UNSUPPORTED_TLV;
	PTPDEBUG("ptp_tlv_management.c:packPtpManagementTlvData(): Unsupported TLV type %04x\n", tlv->tlvType);

    }
    return ret;

}

EOF

echo "$code" >> $CODEFILE


# ************* display wrapper

read -r -d '' code <<"EOF"

void displayPtpManagementTlvData(PtpTlv *tlv) {

    if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS) {

	displayPtpTlvManagementErrorStatus( &tlv->body.managementErrorStatus);

    } else if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT) {

	displayPtpTlvManagement(&tlv->body.management);
	    if(tlv->body.management.empty) {
		PTPINFO("\t(empty management TLV body)\n");
	    } else {
		switch(tlv->body.management.managementId) {

EOF

echo "$code" >> $CODEFILE


for file in `ls *.def | sed 's/\\.def//g' | grep -v "^management"`; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

line=`grep @mgmtid@ $file.def` && {

tlvtype=`echo "$line" | awk ' BEGIN{ FS="@";} { print $3; }'`
tlvid=`echo "$line" | awk ' BEGIN{ FS="@";} { print $4; }'`

echo "		case $tlvtype:"
echo "			displayPtpTlv$type( &tlv->body.management.body.$file );"
echo "			break;"

} >> $CODEFILE

done

read -r -d '' code <<"EOF"

		default:
		    PTPINFO("Unsupported management ID %04x\n", tlv->body.management.managementId);
			break;

	    }
	}
    } else {
	    PTPINFO("Unsupported TLV type %04x\n", tlv->tlvType);
    }

}

EOF

echo "$code" >> $CODEFILE


#************** free wrapper

read -r -d '' code <<"EOF"

void freePtpManagementTlvData(PtpTlv *tlv) {

    if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT_ERROR_STATUS) {

	freePtpTlvManagementErrorStatus( &tlv->body.managementErrorStatus);

    } else if(tlv->tlvType == PTP_TLVTYPE_MANAGEMENT) {

	    switch(tlv->body.management.managementId) {

EOF

echo "$code" >> $CODEFILE


for file in `ls *.def | sed 's/\\.def//g' | grep -v "^management"`; do
type=`echo $file | awk 'BEGIN {OFS="";} { print toupper(substr($0,1,1)),substr($0,2); }'`

line=`grep @mgmtid@ $file.def` && {

tlvtype=`echo "$line" | awk ' BEGIN{ FS="@";} { print $3; }'`
tlvid=`echo "$line" | awk ' BEGIN{ FS="@";} { print $4; }'`

echo "		case $tlvtype:"
echo "			freePtpTlv$type( &tlv->body.management.body.$file );"
echo "			break;"

} >> $CODEFILE

done

read -r -d '' code <<"EOF"

	    default:
			    PTPDEBUG("ptp_tlv_management.c:freePtpManagementTlvData(): Unsupported management ID %04x\n",
				tlv->body.management.managementId);
			break;

	    }

	freePtpTlvManagement(&tlv->body.management);

    } else {
	PTPDEBUG("ptp_tlv_management.c:freePtpManagementTlvData(): Unsupported TLV type %04x\n", tlv->tlvType);
    }

}

EOF

echo "$code" >> $CODEFILE





{

echo "/* end generated code */"
echo

} >> $CODEFILE

