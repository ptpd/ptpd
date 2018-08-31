/*
 * This file contains the minimal set of declarations and constants
 * extracted from the ISC MD5 code included with NTP 4 distribution
 * version 4.2.6p5 to allow computing the MAC for NTP type 7 packets
 */

/* Original copyright notice */

/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: md5.h,v 1.16 2007/06/19 23:47:18 tbox Exp $ */

/*! \file isc/md5.h
 * \brief This is the header file for the MD5 message-digest algorithm.
 *
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 *
 * Changed so as no longer to depend on Colin Plumb's `usual.h'
 * header definitions; now uses stuff from dpkg's config.h
 *  - Ian Jackson <ijackson@nyx.cs.du.edu>.
 * Still in the public domain.
 */

#ifndef ISC_MD5_H
#define ISC_MD5_H 1

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

#define ISC_MD5_DIGESTLENGTH 16U


typedef struct {
	uint32_t buf[4];
	uint32_t bytes[2];
	uint32_t in[16];
} isc_md5_t;

void
isc_md5_init(isc_md5_t *ctx);

void
isc_md5_invalidate(isc_md5_t *ctx);

void
isc_md5_update(isc_md5_t *ctx, const unsigned char *buf, unsigned int len);

void
isc_md5_final(isc_md5_t *ctx, unsigned char *digest);

  typedef isc_md5_t             MD5_CTX;
# define MD5Init(c)             isc_md5_init(c)
# define MD5Update(c, p, s)     isc_md5_update(c, p, s)
# define MD5Final(d, c)         isc_md5_final((c), (d)) /* swapped */
  typedef MD5_CTX                       PTPD_EVP_MD_CTX;
# define EVP_DigestInit(c)              MD5Init(c)
# define EVP_DigestUpdate(c, p, s)      MD5Update(c, p, s)
# define EVP_DigestFinal(c, d, pdl)     \
        do {                            \
                MD5Final((d), (c));     \
                *(pdl) = 16;            \
        } while (0)

#define NID_md5  4
#define JAN_1970 2208988800UL
#define FRAC 4294967296LL

int MD5authencrypt( char *key, uint32_t *pkt, int length, keyid_t keyid );

#endif /* ISC_MD5_H */

