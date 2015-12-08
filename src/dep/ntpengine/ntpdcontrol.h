/*
 * ntpdcontrol.h - definitions for the ntpd remote query and control facility
 */

#ifndef NTPDCONTROL_H
#define NTPDCONTROL_H

#include "../../ptpd.h"

#include "../../timingdomain.h"

typedef struct {

	Boolean enableEngine;
	Boolean enableControl;
	Boolean enableFailover;
	int failoverTimeout;
	int checkInterval;
	int keyId;
	char key[20];
	Octet hostAddress[MAXHOSTNAMELEN];
} NTPoptions;

typedef struct {
	Boolean operational;
	Boolean enabled;
	Boolean isRequired;
	Boolean inControl;
	Boolean isFailOver;
	Boolean checkFailed;
	Boolean requestFailed;
	Boolean flagsCaptured;
	int originalFlags;
	Integer32 serverAddress;
	Integer32 sockFD;
	struct TimingService timingService;
} NTPcontrol;

Boolean ntpInit(NTPoptions* options, NTPcontrol* control);
Boolean ntpShutdown(NTPoptions* options, NTPcontrol* control);
int ntpdControlFlags(NTPoptions* options, NTPcontrol* control, int req, int flags);
int ntpdSetFlags(NTPoptions* options, NTPcontrol* control, int flags);
int ntpdClearFlags(NTPoptions* options, NTPcontrol* control, int flags);
int ntpdInControl(NTPoptions* options, NTPcontrol* control);
//Boolean ntpdControl(NTPoptions* options, NTPcontrol* control, Boolean quiet);


#define NTPCONTROL_YES		128
#define NTPCONTROL_NO		129
#define NTPCONTROL_AUTHERR	18
#define NTPCONTROL_TIMEOUT	19
#define NTPCONTROL_PROTOERR	20
#define NTPCONTROL_NETERR	21


typedef struct {
        union {
                uint32_t Xl_ui;
                int32_t Xl_i;
        } Ul_i;
        union {
                uint32_t Xl_uf;
                int32_t Xl_f;
        } Ul_f;
} l_fp;

#define l_ui    Ul_i.Xl_ui              /* unsigned integral part */
#define l_i     Ul_i.Xl_i               /* signed integral part */
#define l_uf    Ul_f.Xl_uf              /* unsigned fractional part */
#define l_f     Ul_f.Xl_f               /* signed fractional part */

#define M_ADD(r_i, r_f, a_i, a_f)       /* r += a */ \
        do { \
                register uint32_t lo_tmp; \
                register uint32_t hi_tmp; \
                \
                lo_tmp = ((r_f) & 0xffff) + ((a_f) & 0xffff); \
                hi_tmp = (((r_f) >> 16) & 0xffff) + (((a_f) >> 16) & 0xffff); \
                if (lo_tmp & 0x10000) \
                        hi_tmp++; \
		(r_f) = ((hi_tmp & 0xffff) << 16) | (lo_tmp & 0xffff); \
		\
		(r_i) += (a_i); \
		if (hi_tmp & 0x10000) \
		(r_i)++; \
	} while (0)    


#define L_ADD(r, a)     M_ADD((r)->l_ui, (r)->l_uf, (a)->l_ui, (a)->l_uf)

#define HTONL_FP(h, n) do { (n)->l_ui = htonl((h)->l_ui); \
		    (n)->l_uf = htonl((h)->l_uf); } while (0)

typedef unsigned short associd_t; /* association ID */
typedef int32_t keyid_t;        /* cryptographic key ID */
typedef uint32_t tstamp_t;       /* NTP seconds timestamp */


typedef int32_t s_fp;                                                         
typedef uint32_t u_fp;
typedef char s_char;
#define MAX_MAC_LEN     (6 * sizeof(uint32_t))   /* SHA */

/*
 * A request packet.  These are almost a fixed length.
 */
struct req_pkt {
	u_char rm_vn_mode;		/* response, more, version, mode */
	u_char auth_seq;		/* key, sequence number */
	u_char implementation;		/* implementation number */
	u_char request;			/* request number */
	u_short err_nitems;		/* error code/number of data items */
	u_short mbz_itemsize;		/* item size */
	char data[128 + 48];	/* data area [32 prev](176 byte max) */
					/* struct conf_peer must fit */
	l_fp tstamp;			/* time stamp, for authentication */
	keyid_t keyid;			/* (optional) encryption key */
	char mac[MAX_MAC_LEN-sizeof(keyid_t)]; /* (optional) auth code */
};

/*
 * The req_pkt_tail structure is used by ntpd to adjust for different
 * packet sizes that may arrive.
 */
struct req_pkt_tail {
	l_fp tstamp;			/* time stamp, for authentication */
	keyid_t keyid;			/* (optional) encryption key */
	char mac[MAX_MAC_LEN-sizeof(keyid_t)]; /* (optional) auth code */
};

/* MODE_PRIVATE request packet header length before optional items. */
#define	REQ_LEN_HDR	(offsetof(struct req_pkt, data))
/* MODE_PRIVATE request packet fixed length without MAC. */
#define	REQ_LEN_NOMAC	(offsetof(struct req_pkt, keyid))
/* MODE_PRIVATE req_pkt_tail minimum size (16 octet digest) */
#define REQ_TAIL_MIN	\
	(sizeof(struct req_pkt_tail) - (MAX_MAC_LEN - MAX_MD5_LEN))

/*
 * A MODE_PRIVATE response packet.  The length here is variable, this
 * is a maximally sized one.  Note that this implementation doesn't
 * authenticate responses.
 */
#define	RESP_HEADER_SIZE	(offsetof(struct resp_pkt, data))
#define	RESP_DATA_SIZE		(500)

struct resp_pkt {
	u_char rm_vn_mode;		/* response, more, version, mode */
	u_char auth_seq;		/* key, sequence number */
	u_char implementation;		/* implementation number */
	u_char request;			/* request number */
	u_short err_nitems;		/* error code/number of data items */
	u_short mbz_itemsize;		/* item size */
	char data[RESP_DATA_SIZE];	/* data area */
};


/*
 * Information error codes
 */
#define	INFO_OKAY	0
#define	INFO_ERR_IMPL	1	/* incompatable implementation */
#define	INFO_ERR_REQ	2	/* unknown request code */
#define	INFO_ERR_FMT	3	/* format error */
#define	INFO_ERR_NODATA	4	/* no data for this request */
#define	INFO_ERR_AUTH	7	/* authentication failure */
#define INFO_ERR_EMPTY	8 	/* wowczarek: problem with response items */
#define INFO_YES	126	/* wowczarek: NTP is controlling the OS clock */
#define INFO_NO		127	/* wowczarek: NTP is not controlling the OS clock */
/*
 * Maximum sequence number.
 */
#define	MAXSEQ	127

/*
 * Bit setting macros for multifield items.
 */
#define	RESP_BIT	0x80
#define	MORE_BIT	0x40

#define	ISRESPONSE(rm_vn_mode)	(((rm_vn_mode)&RESP_BIT)!=0)
#define	ISMORE(rm_vn_mode)	(((rm_vn_mode)&MORE_BIT)!=0)
#define INFO_VERSION(rm_vn_mode) ((u_char)(((rm_vn_mode)>>3)&0x7))
#define	INFO_MODE(rm_vn_mode)	((rm_vn_mode)&0x7)

#define	RM_VN_MODE(resp, more, version)		\
				((u_char)(((resp)?RESP_BIT:0)\
				|((more)?MORE_BIT:0)\
				|((version?version:(NTP_OLDVERSION+1))<<3)\
				|(MODE_PRIVATE)))

#define	INFO_IS_AUTH(auth_seq)	(((auth_seq) & 0x80) != 0)
#define	INFO_SEQ(auth_seq)	((auth_seq)&0x7f)
#define	AUTH_SEQ(auth, seq)	((u_char)((((auth)!=0)?0x80:0)|((seq)&0x7f)))

#define	INFO_ERR(err_nitems)	((u_short)((ntohs(err_nitems)>>12)&0xf))
#define	INFO_NITEMS(err_nitems)	((u_short)(ntohs(err_nitems)&0xfff))
#define	ERR_NITEMS(err, nitems)	(htons((u_short)((((u_short)(err)<<12)&0xf000)\
				|((u_short)(nitems)&0xfff))))

#define	INFO_MBZ(mbz_itemsize)	((ntohs(mbz_itemsize)>>12)&0xf)
#define	INFO_ITEMSIZE(mbz_itemsize)	((u_short)(ntohs(mbz_itemsize)&0xfff))
#define	MBZ_ITEMSIZE(itemsize)	(htons((u_short)(itemsize)))


/*
 * Implementation numbers.  One for universal use and one for ntpd.
 */
#define	IMPL_UNIV	0
#define	IMPL_XNTPD_OLD	2	/* Used by pre ipv6 ntpdc */
#define	IMPL_XNTPD	3	/* Used by post ipv6 ntpdc */

/*
 * Some limits related to authentication.  Frames which are
 * authenticated must include a time stamp which differs from
 * the receive time stamp by no more than 10 seconds.
 */
#define	INFO_TS_MAXSKEW	10.
#define NTP_OLDVERSION  ((u_char)1)
#define MODE_PRIVATE    7
/*
 * Universal request codes go here.  There aren't any.
 */

/*
 * NTPD request codes go here.
 */
#define	REQ_PEER_LIST		0	/* return list of peers */
#define	REQ_PEER_LIST_SUM	1	/* return summary info for all peers */
#define	REQ_PEER_INFO		2	/* get standard information on peer */
#define	REQ_PEER_STATS		3	/* get statistics for peer */
#define	REQ_SYS_INFO		4	/* get system information */
#define	REQ_SYS_STATS		5	/* get system stats */
#define	REQ_IO_STATS		6	/* get I/O stats */
#define REQ_MEM_STATS		7	/* stats related to peer list maint */
#define	REQ_LOOP_INFO		8	/* info from the loop filter */
#define	REQ_TIMER_STATS		9	/* get timer stats */
#define	REQ_CONFIG		10	/* configure a new peer */
#define	REQ_UNCONFIG		11	/* unconfigure an existing peer */
#define	REQ_SET_SYS_FLAG	12	/* set system flags */
#define	REQ_CLR_SYS_FLAG	13	/* clear system flags */
#define	REQ_MONITOR		14	/* (not used) */
#define	REQ_NOMONITOR		15	/* (not used) */
#define	REQ_GET_RESTRICT	16	/* return restrict list */
#define	REQ_RESADDFLAGS		17	/* add flags to restrict list */
#define	REQ_RESSUBFLAGS		18	/* remove flags from restrict list */
#define	REQ_UNRESTRICT		19	/* remove entry from restrict list */
#define	REQ_MON_GETLIST		20	/* return data collected by monitor */
#define	REQ_RESET_STATS		21	/* reset stat counters */
#define	REQ_RESET_PEER		22	/* reset peer stat counters */
#define	REQ_REREAD_KEYS		23	/* reread the encryption key file */
#define	REQ_DO_DIRTY_HACK	24	/* (not used) */
#define	REQ_DONT_DIRTY_HACK	25	/* (not used) */
#define	REQ_TRUSTKEY		26	/* add a trusted key */
#define	REQ_UNTRUSTKEY		27	/* remove a trusted key */
#define	REQ_AUTHINFO		28	/* return authentication info */
#define REQ_TRAPS		29	/* return currently set traps */
#define	REQ_ADD_TRAP		30	/* add a trap */
#define	REQ_CLR_TRAP		31	/* clear a trap */
#define	REQ_REQUEST_KEY		32	/* define a new request keyid */
#define	REQ_CONTROL_KEY		33	/* define a new control keyid */
#define	REQ_GET_CTLSTATS	34	/* get stats from the control module */
#define	REQ_GET_LEAPINFO	35	/* (not used) */
#define	REQ_GET_CLOCKINFO	36	/* get clock information */
#define	REQ_SET_CLKFUDGE	37	/* set clock fudge factors */
#define REQ_GET_KERNEL		38	/* get kernel pll/pps information */
#define	REQ_GET_CLKBUGINFO	39	/* get clock debugging info */
#define	REQ_SET_PRECISION	41	/* (not used) */
#define	REQ_MON_GETLIST_1	42	/* return collected v1 monitor data */
#define	REQ_HOSTNAME_ASSOCID	43	/* Here is a hostname + assoc_id */
#define REQ_IF_STATS		44	/* get interface statistics */
#define REQ_IF_RELOAD		45	/* reload interface list */

/* Determine size of pre-v6 version of structures */
#define v4sizeof(type)		offsetof(type, v6_flag)

/*
 * Flags in the peer information returns
 */
#define	INFO_FLAG_CONFIG	0x1
#define	INFO_FLAG_SYSPEER	0x2
#define INFO_FLAG_BURST		0x4
#define	INFO_FLAG_REFCLOCK	0x8
#define	INFO_FLAG_PREFER	0x10
#define	INFO_FLAG_AUTHENABLE	0x20
#define	INFO_FLAG_SEL_CANDIDATE	0x40
#define	INFO_FLAG_SHORTLIST	0x80
#define	INFO_FLAG_IBURST	0x100

/*
 * Flags in the system information returns
 */
#define INFO_FLAG_BCLIENT	0x1
#define INFO_FLAG_AUTHENTICATE	0x2
#define INFO_FLAG_NTP		0x4
#define INFO_FLAG_KERNEL	0x8
#define INFO_FLAG_MONITOR	0x40
#define INFO_FLAG_FILEGEN	0x80
#define INFO_FLAG_CAL		0x10
#define INFO_FLAG_PPS_SYNC	0x20



/*
 * System info.  Mostly the sys.* variables, plus a few unique to
 * the implementation.
 */
struct info_sys {
	uint32_t peer;		/* system peer address (v4) */
	u_char peer_mode;	/* mode we are syncing to peer in */
	u_char leap;		/* system leap bits */
	u_char stratum;		/* our stratum */
	s_char precision;	/* local clock precision */
	s_fp rootdelay;		/* delay from sync source */
	u_fp rootdispersion;	/* dispersion from sync source */
	uint32_t refid;		/* reference ID of sync source */
	l_fp reftime;		/* system reference time */
	uint32_t poll;		/* system poll interval */
	u_char flags;		/* system flags */
	u_char unused1;		/* unused */
	u_char unused2;		/* unused */
	u_char unused3;		/* unused */
	s_fp bdelay;		/* default broadcast offset */
	s_fp frequency;		/* frequency residual (scaled ppm)  */
	l_fp authdelay;		/* default authentication delay */
	u_fp stability;		/* clock stability (scaled ppm) */
	u_int v6_flag;		/* is this v6 or not */
	u_int unused4;		/* unused, padding for peer6 */
	struct in6_addr peer6;	/* system peer address (v6) */
};

/*
 * Structure for carrying system flags.
 */
struct conf_sys_flags {
	uint32_t flags;
};

/*
 * System flags we can set/clear
 */
#define	SYS_FLAG_BCLIENT	0x01
#define	SYS_FLAG_PPS		0x02
#define SYS_FLAG_NTP		0x04
#define SYS_FLAG_KERNEL		0x08
#define SYS_FLAG_MONITOR	0x10
#define SYS_FLAG_FILEGEN	0x20
#define SYS_FLAG_AUTH		0x40
#define SYS_FLAG_CAL		0x80

#define NTP_VERSION     ((u_char)4)

#define DEFTIMEOUT      (5)             /* 5 second time out */
#define DEFSTIMEOUT     (2)             /* 2 second time out after first */

#define INITDATASIZE    (sizeof(struct resp_pkt) * 16)
#define INCDATASIZE     (sizeof(struct resp_pkt) * 8)

/* how many time select() will retry on EINTR */
#define NTP_EINTR_RETRIES 5

#endif /* NTPDCONTROL_H */
