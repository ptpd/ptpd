/* Copyright (c) 2015 Wojciech Owczarek,
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
 * @file   net_utils.h
 * @date   Wed Jun 8 16:14:10 2016
 *
 * @brief  Definitions for network utility functions
 *
 */

#ifndef PTPD_NET_UTILS_H_
#define PTPD_NET_UTILS_H_

#include "../ptpd.h"
#include <libcck/cck_types.h>

typedef struct {

    int cmsgLevel;
    int cmsgType;
    size_t elemSize;
    int arrayIndex;
    void (*convertTimestamp) (CckTimestamp *timestamp, void *data);

} TTSocketTstampConfig;

/* struct msghdr:sys/socket.h */

/* Try getting hwAddrSize bytes of ifaceName hardware address,
   and place them in hwAddr. Return 1 on success, 0 when no suitable
   hw address available, -1 on failure.
 */
int getHwAddr (const char* ifaceName, unsigned char* hwAddr, const int hwAddrSize);

/* populate and prepare msghdr (assuming a single message) */
void prepareMsgHdr(struct msghdr *msg, struct iovec *vec, CckOctet *dataBuf, size_t dataBufSize, char *controlBuf, size_t controlLen, struct sockaddr *addr, int addrLen);
/* get control message data of minimum size len from control message header. Return 1 if found, 0 if not found, -1 if data too short */
int getCmsgData(void *data, struct cmsghdr *cmsg, const int level, const int type, const size_t len);

/* get control message data of n-th array member of size elemSize from control message header. Return 1 if found, 0 if not found, -1 if data too short */
int getCmsgItem(void *data, struct cmsghdr *cmsg, const int level, const int type, const size_t elemSize, const int arrayIndex);

/* get timestamp from control message header based on timestamp type described by tstampConfig. Return 1 if found and non-zero, 0 if not found or zero, -1 if data too short */
int getCmsgTimestamp(CckTimestamp *timestamp, struct cmsghdr *cmsg, const TTSocketTstampConfig *config);

/* get timestamp from a timestamp struct of the given type */
void convertTimestamp_timespec(CckTimestamp *timestamp, const void *data);
void convertTimestamp_timeval(CckTimestamp *timestamp, const void *data);
#ifdef SO_BINTIME
void convertTimestamp_bintime(CckTimestamp *timestamp, const void *data);
#endif /* SO_BINTIME */

#endif /* PTPD_NET_UTILS_H_ */
