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
	show_tlv				 -  to see management TLV fields (Table 35)\n\
	show_clock_description   -  to see dataField of clock_description TLV (Table 41)\n\
	show_default_data_set    -  to see fields of default data set (Table 50)\n\
	quit                     -  to close the program\n\
	..etc\n\n");
}

void
show_commonheader()
{
	printf("Not implemented yet\n");
}

void
show_managementheader()
{
	printf("Not implemented yet\n");
}

void
show_tlv()
{
	printf("Not implemented yet\n");
}

void
show_clock_description()
{
	printf("Not implemented yet\n");
}

void
show_default_data_set()
{
	printf("Not implemented yet\n");
}
