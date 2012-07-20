/** 
 * @file        constants.h
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       Constants used throughout the code.
 */

#ifndef CONSTANTS_H
#define	CONSTANTS_H

#define MAX_ADDR_STR_LEN 50
#define MAX_PORT_STR_LEN 50

#define MAX_MGMTID_STR_LEN 50

#define PACKET_SIZE  300 //ptpdv1 value kept because of use of TLV...

#define RECV_TIMEOUT 3 //Timeout for a receive function (in seconds)

#define U_ADDRESS "127.0.0.1"

#endif	/* CONSTANTS_H */

