/**
 * @file   show.c
 * @date   Thurs July 5 02:12:14 2012
 * 
 * @brief  This file implements 'help' functions
 */

#include "ptpmanager.h"

void 
show_help()
{
	printf("Valid commands are:\n\
	send                     -  to send a management message\n\
	send_previous            -  to send last sent message again\n\
	show_previous_inmessage	 -  to see fields of last received message\n\
	show_commonheader        -  to see header (Table 18 of spec)\n\
	show_managementheader    -  to see management header (Table 37)\n\
	show_tlv                 -  to see management TLV fields (Table 35)\n\
	show_mgmtIds             -  to see all available management TLVs\n\
	quit                     -  to close the program\n\n");
}

void
show_commonheader()
{
	if (inmessage == NULL){
		printf("No data in last received message\n");
		return;
	} else {
		MsgHeader h; 
		unpackHeader(inmessage, &h);
		if (h.messageLength < HEADER_LENGTH){
			printf("Last received message doesn't contain valid data\n");
			return;
		}
		display_commonHeader(&h);
	}
}

void
show_managementheader()
{
	if (inmessage == NULL){
		printf("No data in last received message\n");
		return;
	} else {
		MsgHeader h; 
		MsgManagement manage;
		manage.tlv = (ManagementTLV *)malloc(sizeof(ManagementTLV));
		unpackHeader(inmessage, &h);
		if (h.messageLength < MANAGEMENT_LENGTH){
			printf("Last received message doesn't contain valid data\n\n");
			return;
		}
		unpackManagementHeader(inmessage, &manage);
		display_managementHeader(&manage);
		free(manage.tlv);
	}
}

void
show_tlv()
{
	if (inmessage == NULL){
		printf("No data in last received message\n");
		return;
	} else {
		MsgHeader h; 
		MsgManagement manage;
		manage.tlv = (ManagementTLV *)malloc(sizeof(ManagementTLV));
		unpackHeader(inmessage, &h);
		if (h.messageLength < MANAGEMENT_LENGTH + TLV_LENGTH){
			printf("Last received message doesn't contain valid data\n\n");
			return;
		}
		unpackManagementHeader(inmessage, &manage);
		display_tlvHeader(&(manage.tlv));
		free(manage.tlv);
	}

}

void
show_mgmtIds()
{
    printf("managementId name                   \tmanagementId value (hex)      \tAllowed actions               \tApplies to\n"
            "\n"
            "===Applicable to all node types===================================================================================\n"
            "NULL_MANAGEMENT                    \t0000                          \tGET, SET, COMMAND             \tport\n"
            "CLOCK_DESCRIPTION                  \t0001                          \tGET                           \tport\n"
            "USER_DESCRIPTION                   \t0002                          \tGET, SET                      \tclock\n"
            "SAVE_IN_NON_VOLATILE_STORAGE       \t0003                          \tCOMMAND                       \tclock\n"
            "RESET_NON_VOLATILE_STORAGE         \t0004                          \tCOMMAND                       \tclock\n"
            "INITIALIZE                         \t0005                          \tCOMMAND                       \tclock\n"
            "FAULT_LOG                          \t0006                          \tGET                           \tclock\n"
            "FAULT_LOG_RESET                    \t0007                          \tCOMMAND                       \tclock\n"
            "\n"
            "===Applicable to ordinary and boundary clocks=====================================================================\n"
            "DEFAULT_DATA_SET                   \t2000                          \tGET                           \tclock\n"
            "CURRENT_DATA_SET                   \t2001                          \tGET                           \tclock\n"
            "PARENT_DATA_SET                    \t2002                          \tGET                           \tclock\n"
            "TIME_PROPERTIES_DATA_SET           \t2003                          \tGET                           \tclock\n"
            "PORT_DATA_SET                      \t2004                          \tGET                           \tport\n"
            "PRIORITY1                          \t2005                          \tGET, SET                      \tclock\n"
            "PRIORITY2                          \t2006                          \tGET, SET                      \tclock\n"
            "DOMAIN                             \t2007                          \tGET, SET                      \tclock\n"
            "SLAVE_ONLY                         \t2008                          \tGET, SET                      \tclock\n"
            "LOG_ ANNOUNCE_INTERVAL             \t2009                          \tGET, SET                      \tport\n"
            "ANNOUNCE_RECEIPT_TIMEOUT           \t200A                          \tGET, SET                      \tport\n"
            "LOG_ SYNC_INTERVAL                 \t200B                          \tGET, SET                      \tport\n"
            "VERSION_NUMBER                     \t200C                          \tGET, SET                      \tport\n"
            "ENABLE_PORT                        \t200D                          \tCOMMAND                       \tport\n"
            "DISABLE_PORT                       \t200E                          \tCOMMAND                       \tport\n"
            "TIME                               \t200F                          \tGET, SET                      \tclock\n"
            "CLOCK_ACCURACY                     \t2010                          \tGET, SET                      \tclock\n"
            "UTC_PROPERTIES                     \t2011                          \tGET, SET                      \tclock\n"
            "TRACEABILITY_PROPERTIES            \t2012                          \tGET, SET                      \tclock\n"
            "TIMESCALE_PROPERTIES               \t2013                          \tGET, SET                      \tclock\n"
            "UNICAST_NEGOTIATION_ENABLE         \t2014                          \tGET, SET                      \tport\n"
            "PATH_TRACE_LIST                    \t2015                          \tGET                           \tclock\n"
            "PATH_TRACE_ENABLE                  \t2016                          \tGET, SET                      \tclock\n"
            "GRANDMASTER_CLUSTER_TABLE          \t2017                          \tGET, SET                      \tclock\n"
            "UNICAST_MASTER_TABLE               \t2018                          \tGET, SET                      \tport\n"
            "UNICAST_MASTER_MAX_TABLE_SIZE      \t2019                          \tGET                           \tport\n"
            "ACCEPTABLE_MASTER_TABLE            \t201A                          \tGET, SET                      \tclock\n"
            "ACCEPTABLE_MASTER_TABLE_ENABLED    \t201B                          \tGET, SET                      \tport\n"
            "ACCEPTABLE_MASTER_MAX_TABLE_SIZE   \t201C                          \tGET                           \tclock\n"
            "ALTERNATE_MASTER                   \t201D                          \tGET, SET                      \tport\n"
            "ALTERNATE_TIME_OFFSET_ENABLE       \t201E                          \tGET, SET                      \tclock\n"
            "ALTERNATE_TIME_OFFSET_NAME         \t201F                          \tGET, SET                      \tclock\n"
            "ALTERNATE_TIME_OFFSET_MAX_KEY      \t2020                          \tGET                           \tclock\n"
            "ALTERNATE_TIME_OFFSET_PROPERTIES   \t2021                          \tGET, SET                      \tclock\n"
            "\n"
            "===Applicable to transparent clocks==============================================================================\n"
            "TRANSPARENT_CLOCK_DEFAULT_DATA_SET \t4000                          \tGET                           \tclock\n"
            "TRANSPARENT_CLOCK_PORT_DATA_SET    \t4001                          \tGET                           \tport\n"
            "PRIMARY_DOMAIN                     \t4002                          \tGET, SET                      \tclock\n"
            "\n"
            "===Applicable to ordinary, boundary, and transparent clocks======================================================\n"
            "DELAY_MECHANISM                    \t6000                          \tGET, SET                      \tport\n"
            "LOG_MIN_ PDELAY_REQ_INTERVAL       \t6001                          \tGET, SET                      \tport\n");
}
