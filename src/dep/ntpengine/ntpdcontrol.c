/*-
 * Copyright (c) 2013-2015 Wojciech Owczarek,
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
 * @file   ntpdcontrol.c
 * @date   Tue Jul 20 22:19:20 2013
 *
 * @brief  Functions allowing remote control of the ntpd daemon
 *         using mode 7 packets
 *
 */

/*
 * This code is largely based on the source code of the ntpdc utility from the
 * NTP distribution version 4.2.6p5 and is mostly a bridge between PTPd2 APIs
 * and NTPDc code - functionality so far is limited to setting and clearing system
 * flags to allow failover to- and -from (local) NTP
 */

/* Original NTP4 copyright notice */

/***********************************************************************
 *                                                                     *
 * Copyright (c) University of Delaware 1992-2011                      *
 *                                                                     *
 * Permission to use, copy, modify, and distribute this software and   *
 * its documentation for any purpose with or without fee is hereby     *
 * granted, provided that the above copyright notice appears in all    *
 * copies and that both the copyright notice and this permission       *
 * notice appear in supporting documentation, and that the name        *
 * University of Delaware not be used in advertising or publicity      *
 * pertaining to distribution of the software without specific,        *
 * written prior permission. The University of Delaware makes no       *
 * representations about the suitability this software for any         *
 * purpose. It is provided "as is" without express or implied          *
 * warranty.                                                           *
 *                                                                     *
 ***********************************************************************/

#include "../../ptpd.h"

#define NTP_PORT 123

char *ntpdc_pktdata;

Boolean
ntpInit(NTPoptions* options, NTPcontrol* control)
{

	int res = TRUE;
	TimingService service = control->timingService;

	control->sockFD = -1;
	if(!options->enableEngine)
	    return FALSE;

	memset(control, 0, sizeof(*control));
	/* preserve TimingService... temporary */
	control->timingService = service;

	if(!hostLookup(options->hostAddress, &control->serverAddress)) {
                control->serverAddress = 0;
		return FALSE;
	}

        if ((control->sockFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
                PERROR("failed to initalize NTP control socket");
                return FALSE;
        }

	/* This will attempt to read the ntpd control flags for the first time */
	res = ntpdInControl(options, control);
	
       if (res != INFO_YES && res != INFO_NO) {
		return FALSE;
	}

	DBGV("NTPd original flags: %d\n", control->originalFlags);
	return TRUE;
}

Boolean
ntpShutdown(NTPoptions* options, NTPcontrol* control)
{

	/* Attempt reverting ntpd flags to the original value */
	if(control->flagsCaptured) {
		/* we only control the kernel and ntp flags */
	/* just to avoid -Wunused* */
#ifdef RUNTIME_DEBUG
		int resC = ntpdClearFlags(options, control, ~(control->originalFlags) & (INFO_FLAG_KERNEL | INFO_FLAG_NTP));
		int resS = ntpdSetFlags(options, control, control->originalFlags & (INFO_FLAG_KERNEL | INFO_FLAG_NTP));
		DBGV("Attempting to revert NTPd flags to %d - result: clear %d set %d\n", control->originalFlags,
			    resC, resS);
#else
		ntpdClearFlags(options, control, ~(control->originalFlags) & (INFO_FLAG_KERNEL | INFO_FLAG_NTP));
		ntpdSetFlags(options, control, control->originalFlags & (INFO_FLAG_KERNEL | INFO_FLAG_NTP));
#endif /* RUNTIME_DEBUG */
	}

        if (control->sockFD > 0)
                close(control->sockFD);
        control->sockFD = -1;

	return TRUE;
}


static ssize_t
ntpSend(NTPcontrol* control, Octet * buf, UInteger16 length)
{
        ssize_t ret = -1;
        struct sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_port = htons(NTP_PORT);

        if (control->serverAddress) {

                addr.sin_addr.s_addr = control->serverAddress;

                ret = sendto(control->sockFD, buf, length, 0,
                             (struct sockaddr *)&addr,
                             sizeof(struct sockaddr_in));
                if (ret <= 0)
                        INFO("error sending NTP control message\n");

        }
        return ret;
}


/*
 * Default values we use
 */
#define	DEFHOST		"localhost"	/* default host name */
#define	DEFTIMEOUT	(5)		/* 5 second time out */
#define	DEFSTIMEOUT	(2)		/* 2 second time out after first */
#define	DEFDELAY	0x51EB852	/* 20 milliseconds, l_fp fraction */
#define	LENHOSTNAME	256		/* host name is 256 characters long */
#define	MAXCMDS		100		/* maximum commands on cmd line */
#define	MAXHOSTS	200		/* maximum hosts on cmd line */
#define	MAXLINE		512		/* maximum line length */
#define	MAXTOKENS	(1+1+MAXARGS+MOREARGS+2)	/* maximum number of usable tokens */
#define	SCREENWIDTH  	78		/* nominal screen width in columns */

#define	INITDATASIZE	(sizeof(struct resp_pkt) * 16)
#define	INCDATASIZE	(sizeof(struct resp_pkt) * 8)

/*
 * These are used to help the magic with old and new versions of ntpd.
 */
#define IMPL_XNTPD      3
int impl_ver = IMPL_XNTPD;
#define REQ_LEN_NOMAC   (offsetof(struct req_pkt, keyid))
static int req_pkt_size = REQ_LEN_NOMAC;
#define	ERR_INCOMPLETE		16
#define	ERR_TIMEOUT		17

static void
get_systime(
        l_fp *now               /* system time */
        )
{
        double dtemp;

#if defined(HAVE_CLOCK_GETTIME) || defined(HAVE_GETCLOCK)
        struct timespec ts;     /* seconds and nanoseconds */

        /*
         * Convert Unix clock from seconds and nanoseconds to seconds.
         */
# ifdef HAVE_CLOCK_GETTIME
        clock_gettime(CLOCK_REALTIME, &ts);
# else
        getclock(TIMEOFDAY, &ts);
# endif
        now->l_i = ts.tv_sec + JAN_1970;
        dtemp = ts.tv_nsec / 1e9;

#else /* HAVE_CLOCK_GETTIME || HAVE_GETCLOCK */
        struct timeval tv;      /* seconds and microseconds */

        /*
         * Convert Unix clock from seconds and microseconds to seconds.
         */
        gettimeofday(&tv, NULL);
        now->l_i = tv.tv_sec + JAN_1970;
        dtemp = tv.tv_usec / 1e6;

#endif /* HAVE_CLOCK_GETTIME || HAVE_GETCLOCK */

        /*
         * Renormalize to seconds past 1900 and fraction.
         */
                                                    
//        dtemp += sys_residual;
        if (dtemp >= 1) {
                dtemp -= 1;
                now->l_i++;
        } else if (dtemp < -1) {
                dtemp += 1;
                now->l_i--;
        }
        dtemp *= FRAC;
        now->l_uf = (uint32_t)dtemp;
}

static int
NTPDCrequest(
	NTPoptions* options,
	NTPcontrol* control,
	int reqcode,
	int auth,
	u_int qitems,
	size_t qsize,
	char *qdata
	)
{

	l_fp delay_time = { .l_ui=0, .l_uf=0x51EB852};

	struct req_pkt qpkt;
	size_t	datasize;
	size_t	reqsize;
	l_fp	ts;
	l_fp *	ptstamp;
	int	maclen;
	static char *key;

	key=calloc(21,sizeof(char));
	strncpy(key,options->key,20);

	memset(&qpkt, 0, sizeof(qpkt));
	qpkt.rm_vn_mode = RM_VN_MODE(0, 0, 0);
	qpkt.implementation = (u_char)3;
	qpkt.request = (u_char)reqcode;
	datasize = qitems * qsize;
	if (datasize && qdata != NULL) {
		memcpy(qpkt.data, qdata, datasize);
		qpkt.err_nitems = ERR_NITEMS(0, qitems);
		qpkt.mbz_itemsize = MBZ_ITEMSIZE(qsize);
	} else {
		qpkt.err_nitems = ERR_NITEMS(0, 0);
		qpkt.mbz_itemsize = MBZ_ITEMSIZE(qsize);  /* allow for optional first item */
	}

	if (!auth) {
		qpkt.auth_seq = AUTH_SEQ(0, 0);
		return ntpSend(control, (Octet *)&qpkt, req_pkt_size);
	}

		qpkt.auth_seq = AUTH_SEQ(1, 0);

		reqsize = req_pkt_size;

	ptstamp = (void *)((u_char *)&qpkt + reqsize);
	ptstamp--;
	get_systime(&ts);
	L_ADD(&ts, &delay_time);
	HTONL_FP(&ts, ptstamp);

	maclen = MD5authencrypt(key, (void *)&qpkt, reqsize,options->keyId);
	free(key);
	if (!maclen || (maclen != (16 + sizeof(keyid_t))))
	 { 
		ERROR("Error while computing NTP MD5 hash\n");
		return 1;
	}

	return ntpSend(control, (Octet *)&qpkt, reqsize + maclen);

}


/*
 * checkitems - utility to print a message if no items were returned
 */
/*
static int
checkitems(
	int items

	)
{
	if (items == 0) {

		return 0;
	}
	return 1;
}
*/

/*
 * checkitemsize - utility to print a message if the item size is wrong
 */
static int
checkitemsize(
	int itemsize,
	int expected
	)
{
	if (itemsize != expected) {
		return 0;
	}
	return 1;
}


/*
 * check1item - check to make sure we have exactly one item
 */
static int
check1item(
	int items

	)
{
	if (items == 0) {
		return 0;
	}
	if (items > 1) {

		return 0;
	}
	return 1;
}

static int
NTPDCresponse(
	NTPoptions* options,
	NTPcontrol* control,
	int reqcode,
	int *ritems,
	int *rsize,
	char **rdata,
	int esize
	)
{
	struct resp_pkt rpkt;
	struct timeval tvo;
	int items;
	int i;
	int size;
	int datasize;
	char *datap;
	char *tmp_data;
	char haveseq[MAXSEQ+1];
	int firstpkt;
	int lastseq;
	int numrecv;
	int seq;
	fd_set fds;
	int n;
	int pad;
	int retries = NTP_EINTR_RETRIES;

        int ntpdc_pktdatasize;

	int implcode=(u_char)3;

	static  struct timeval tvout = { DEFTIMEOUT, 0 };   /* time out for reads */
	static  struct timeval tvsout = { DEFSTIMEOUT, 0 }; /* secondary time out */

//	static char *currenthost = "localhost";

        ntpdc_pktdatasize = INITDATASIZE;
        ntpdc_pktdata = realloc(ntpdc_pktdata,INITDATASIZE);

	/*
	 * This is pretty tricky.  We may get between 1 and many packets
	 * back in response to the request.  We peel the data out of
	 * each packet and collect it in one long block.  When the last
	 * packet in the sequence is received we'll know how many we
	 * should have had.  Note we use one long time out, should reconsider.
	 */
	*ritems = 0;
	*rsize = 0;
	firstpkt = 1;
	numrecv = 0;

	lastseq = 999;	/* too big to be a sequence number */
	memset(haveseq, 0, sizeof(haveseq));


    again:
	if (firstpkt)
		tvo = tvout;
	else
		tvo = tvsout;
	do {
	    FD_ZERO(&fds);
	    FD_SET(control->sockFD, &fds);
	    n = select(control->sockFD+1, &fds, (fd_set *)0, (fd_set *)0, &tvo);
	    if(n == -1) {
		if(errno == EINTR) {
		DBG("NTPDCresponse(): EINTR caught\n");
		    retries--;
		} else {
		    retries = 0;
		}
	    }
	} while ((n == -1) && retries);

	if (n == -1) {
	    DBG("NTPDCresponse(): select failed - not EINTR: %s\n", strerror(errno));
		return -1;
	}
	if (n == 0) {
		DBG("NTP response select timeout");
		if (firstpkt) {
			return ERR_TIMEOUT;
		} else {
			return ERR_INCOMPLETE;
		}
	}

	n = recv(control->sockFD, (char *)&rpkt, sizeof(rpkt), 0);

	if (n == -1) {
	    DBG("NTP response recv failed\n");
	    return -1;
	}


	/*
	 * Check for format errors.  Bug proofing.
	 */
	if (n < RESP_HEADER_SIZE) {
		goto again;
	}



	if (INFO_VERSION(rpkt.rm_vn_mode) > NTP_VERSION ||
	    INFO_VERSION(rpkt.rm_vn_mode) < NTP_OLDVERSION) {
/*		if (debug)
		    printf("Packet received with version %d\n",
			   INFO_VERSION(rpkt.rm_vn_mode));
*/

		goto again;
	}

	if (INFO_MODE(rpkt.rm_vn_mode) != MODE_PRIVATE) {
/*		if (debug)
		    printf("Packet received with mode %d\n",
			   INFO_MODE(rpkt.rm_vn_mode));
*/

		goto again;

	}
	if (INFO_IS_AUTH(rpkt.auth_seq)) {
/*
		if (debug)
		    printf("Encrypted packet received\n");
*/
		goto again;
	}
	if (!ISRESPONSE(rpkt.rm_vn_mode)) {
/*
		if (debug)
		    printf("Received request packet, wanted response\n");
*/
		goto again;
	}
	if (INFO_MBZ(rpkt.mbz_itemsize) != 0) {
/*
		if (debug)
		    printf("Received packet with nonzero MBZ field!\n");
*/
		goto again;
	}

	/*
	 * Check implementation/request.  Could be old data getting to us.
	 */

	if (rpkt.implementation != implcode || rpkt.request != reqcode) {
/*
		if (debug)
		    printf(
			    "Received implementation/request of %d/%d, wanted %d/%d",
			    rpkt.implementation, rpkt.request,
			    implcode, reqcode);
*/
		goto again;
	}

	/*
	 * Check the error code.  If non-zero, return it.
	 */
	if (INFO_ERR(rpkt.err_nitems) != INFO_OKAY) {
/*
		if (debug && ISMORE(rpkt.rm_vn_mode)) {
			printf("Error code %d received on not-final packet\n",
			       INFO_ERR(rpkt.err_nitems));
		}
*/
		return (int)INFO_ERR(rpkt.err_nitems);
	}

	/*
	 * Collect items and size.  Make sure they make sense.
	 */
	items = INFO_NITEMS(rpkt.err_nitems);

	size = INFO_ITEMSIZE(rpkt.mbz_itemsize);
	if (esize > size)
		pad = esize - size;
	else
		pad = 0;
	datasize = items * size;

	if ((size_t)datasize > (n-RESP_HEADER_SIZE)) {
/*		if (debug)
		    printf(
			    "Received items %d, size %d (total %d), data in packet is %lu\n",
			    items, size, datasize, (u_long)(n-RESP_HEADER_SIZE));
*/
		goto again;

	}

	/*
	 * If this isn't our first packet, make sure the size matches
	 * the other ones.
	 */
	if (!firstpkt && esize != *rsize) {
/*		if (debug)
		    printf("Received itemsize %d, previous %d\n",
			   size, *rsize);
*/
		goto again;
	}
	/*
	 * If we've received this before, +toss it
	 */
	seq = INFO_SEQ(rpkt.auth_seq);
	if (haveseq[seq]) {
/*		if (debug)
		    printf("Received duplicate sequence number %d\n", seq);
*/
		goto again;
	}
	haveseq[seq] = 1;

	/*
	 * If this is the last in the sequence, record that.
	 */
	if (!ISMORE(rpkt.rm_vn_mode)) {
		if (lastseq != 999) {
			DBGV("NTPDC Received second end sequence packet\n");
			goto again;
		}
		lastseq = seq;
	}

	*rdata = datap = ntpdc_pktdata;

	/*
	 * So far, so good.  Copy this data into the output array.
	 */
	if ((datap + datasize + (pad * items)) > (ntpdc_pktdata + ntpdc_pktdatasize)) {
		int offset = datap - ntpdc_pktdata;

		ntpdc_pktdatasize += INCDATASIZE;
		ntpdc_pktdata = realloc(ntpdc_pktdata, (size_t)ntpdc_pktdatasize);
		*rdata = ntpdc_pktdata; /* might have been realloced ! */
		datap = ntpdc_pktdata + offset;
	}
	/*
	 * We now move the pointer along according to size and number of
	 * items.  This is so we can play nice with older implementations
	 */

	tmp_data = rpkt.data;
	for (i = 0; i < items; i++) {
		memcpy(datap, tmp_data, (unsigned)size);
		tmp_data += size;
		memset(datap + size, 0, pad);
		datap += size + pad;
	}

	if (firstpkt) {
		firstpkt = 0;
		*rsize = size + pad;
	}
	*ritems += items;

	/*
	 * Finally, check the count of received packets.  If we've got them
	 * all, return
	 */
	++numrecv;

	if (numrecv <= lastseq)
		goto again;

	return INFO_OKAY;
}

static int
NTPDCquery(
	NTPoptions* options,
	NTPcontrol* control,
	int reqcode,
	int auth,
	int qitems,
	int qsize,
	char *qdata,
	int *ritems,
	int *rsize,
	char **rdata,
 	int quiet_mask,
	int esize
	)
{
	int res;
	int retries = NTP_EINTR_RETRIES;
	char junk[512];
	fd_set fds;
	struct timeval tvzero;
//	int implcode=(u_char)3;

	/*
	 * Poll the socket and clear out any pending data
	 */
again:
	do {
		tvzero.tv_sec = tvzero.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(control->sockFD, &fds);
		res = select(control->sockFD+1, &fds, (fd_set *)0, (fd_set *)0, &tvzero);

		if (res == -1) {
			if((errno == EINTR) && retries ) {
			    DBG("NTPDCquery(): select EINTR caught");
			    retries--;
			    res = 1;
			    continue;
			}
			DBG("NTPDCquery(): select() error - not EINTR: %s\n", strerror(errno));
			return -1;
		} else if (res > 0) {
		    (void) recv(control->sockFD, junk, sizeof junk, 0);
		    }
	} while (res > 0);

	/*
	 * send a request
	 */
	res = NTPDCrequest(options, control, reqcode, auth, qitems, qsize, qdata);
	if (res <= 0) {
		return res;
	}
	/*
	 * Get the response.  If we got a standard error, print a message
	 */
	res = NTPDCresponse(options, control, reqcode, ritems, rsize, rdata, esize);

	/*
	 * Try to be compatible with older implementations of ntpd.
	 */
	if (res == INFO_ERR_FMT && req_pkt_size != 48) {
#if defined(RUNTIME_DEBUG) || defined (PTPD_DBGV)
		int oldsize  = req_pkt_size;
#endif /* RUNTIME_DEBUG */

		switch(req_pkt_size) {
		case REQ_LEN_NOMAC:
			req_pkt_size = 160;
			break;
		case 160:
			req_pkt_size = 48;
			break;
		}
		if (impl_ver == IMPL_XNTPD) {
			DBGV(
			    "NTPDC ***Warning changing to older implementation\n");
			return INFO_ERR_IMPL;
		}

		DBGV(
		    "NTPDC ***Warning changing the request packet size from %d to %d\n",
		    oldsize, req_pkt_size);
		goto again;
	}

	return res;
}

int
ntpdControlFlags(NTPoptions* options, NTPcontrol* control, int req, int flags)
{
	struct conf_sys_flags sys;
	int items;
	int itemsize;
	char *dummy;
	int res;

	sys.flags = 0;
	res = 0;
	sys.flags = flags;

	sys.flags = htonl(sys.flags);

	res = NTPDCquery(options, control, req, 1, 1,
		      sizeof(struct conf_sys_flags), (char *)&sys, &items,
		      &itemsize, &dummy, 0, sizeof(struct conf_sys_flags));

	if ((res != INFO_OKAY) && (sys.flags == 0))
	    return 0;

	if (res != INFO_OKAY) {

	switch (res) {

	case -1:
		if(!control->requestFailed) ERROR("Cannot connect to NTP daemon\n");
		break;

	case INFO_ERR_AUTH:

		if(!control->requestFailed) ERROR("NTP permission denied: check key id, password and NTP configuration\n");
		break;		

	case ERR_TIMEOUT:

		if(!control->requestFailed) ERROR("Timeout while connecting to NTP daemon\n");
		break;

	default:
	ERROR("NTP protocol error\n");

	}
	}

	return res;

}

int
ntpdSetFlags(NTPoptions* options, NTPcontrol* control, int flags)
{

	int res;
	ntpdc_pktdata = malloc(INITDATASIZE);
	DBGV("Setting NTP flags %d\n", flags);
	res=ntpdControlFlags(options, control, REQ_SET_SYS_FLAG, flags);
	free(ntpdc_pktdata);
	return res;

}

int
ntpdClearFlags(NTPoptions* options, NTPcontrol* control, int flags)
{

	int res;
	ntpdc_pktdata = malloc(INITDATASIZE);
	DBGV("Clearing NTP flags %d\n", flags);
	res=ntpdControlFlags(options, control, REQ_CLR_SYS_FLAG, flags);
	free(ntpdc_pktdata);
	return res;
}


int
ntpdInControl(NTPoptions* options, NTPcontrol* control)
{
	struct info_sys *is;
	int items;
	int itemsize;
	int res;
        ntpdc_pktdata = malloc(INITDATASIZE);
	res = NTPDCquery(options, control, REQ_SYS_INFO, 0, 0, 0, (char *)NULL,
		      &items, &itemsize, (void *)&is, 0,
		      sizeof(struct info_sys));

	if ( res != 0 )
	    goto end;

	if (!check1item(items)) {

	    res=INFO_ERR_EMPTY;
	    goto end;
	}


	if (!checkitemsize(itemsize, sizeof(struct info_sys)) &&
	    !checkitemsize(itemsize, v4sizeof(struct info_sys))) {
	
	    res=INFO_ERR_EMPTY;
	    goto end;
	}	

	if (is->flags & INFO_FLAG_NTP) DBGV("NTP flag seen: ntp\n");
	if (is->flags & INFO_FLAG_KERNEL) DBGV("NTP flag seen: kernel\n");

	if(!control->flagsCaptured) {
		control->originalFlags = is->flags;
		 /* we only control the kernel and ntp flags */
		control->originalFlags &= (INFO_FLAG_KERNEL | INFO_FLAG_NTP);
		control->flagsCaptured = TRUE;
		res = INFO_YES;
		goto end;
	}

	if ((is->flags & INFO_FLAG_NTP) || (is->flags & INFO_FLAG_KERNEL))
	{
		res=INFO_YES;
	} else  {

		res=INFO_NO;
	}

	end:

	free(ntpdc_pktdata);


	if (res != INFO_YES && res != INFO_NO) {

	switch (res) {

	case -1:
		DBG("Could not connect to NTP daemon\n");
		break;

	case ERR_TIMEOUT:

		DBG("Timeout while connecting to NTP daemon\n");
		break;

	case INFO_ERR_AUTH:

		DBG("NTP permission denied: check NTP key id, key and if key is trusted and is a request key\n");
		break;		

	default:
	ERROR("NTP protocol error\n");

	}
	}
	return res;

}

