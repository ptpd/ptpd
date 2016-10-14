/*-
 * Copyright (c) 2015-2016 Eyal Itkin (TAU),
 *                         Avishai Wool (TAU),
 *                         In accordance to http://arxiv.org/abs/1603.00707 .
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

#ifdef SEC_EXT_CRYPTO 

#ifndef CRYPTO_ENC_H_
#define CRYPTO_ENC_H_

#include <netinet/in.h>
#include <netinet/ether.h>
#include "../ipv4_acl.h"
#include <stdio.h>
#include <stdlib.h>
#define PATH_MAX 4096
#include <dep/datatypes_dep.h>
#include <constants.h>

#define USING_EdDSA

/* defines */
#define HAVE_ED25519
/* includes */
#include <wolfssl/wolfcrypt/ed25519.h>
/* constants */
#define SIGNATURE_SIZE				ED25519_SIG_SIZE /* 64 */
#define PUBLIC_KEY_SIZE				ED25519_KEY_SIZE /* 32 */
/* types */
#undef USE_RANDOM
/* structs */
typedef ed25519_key		 			key_type;
typedef WC_RNG			 			prng_type;

#define SECURITY_FLAG 				( 1 << 7 )

#define TIME_CRYPTO
#undef  TIME_CRYPTO /* timming disabled */

/**
* \brief Signature structure to handle pk signatures
 */
typedef struct {
	Octet sig[SIGNATURE_SIZE];
} Signature;

/**
* \brief Public Key structure to handle pk
 */
typedef struct {
	Octet key[PUBLIC_KEY_SIZE];
} PubKey;

#define FOLLOW_UP_EXT_LENGTH	(FOLLOW_UP_LENGTH + SIGNATURE_SIZE)
#define ANNOUNCE_EXT_LENGTH		(ANNOUNCE_LENGTH  + PUBLIC_KEY_SIZE + SIGNATURE_SIZE)
#define ANNOUNCE_VERIFY_LENGTH	(ANNOUNCE_PAYLOAD + PUBLIC_KEY_SIZE)

/* inits the key type */
void init_key(key_type* key);

/* sign the PTP msg (header + timestamp) */
void signMsg(key_type *key, Octet* buf, size_t size, Signature* sig, prng_type* ctr_drbg);

/* verify the PTP msg's signature */
Boolean verifyMsg(key_type *key, Octet* buf, size_t size, Octet* sig);

/* ready the public key for sending */
Boolean packPublicKey(Octet* buf, key_type *key, size_t* len);

/* extract the public key from the given msg */
Boolean unpackPublicKey(Octet* buf, key_type *key);

/* read the public key from the given file */
Boolean importPublicKey(key_type *key, char* path);

/* read the private key from the given file */
Boolean importPrivateKey(key_type *key, char* path, key_type *pub_key);

#endif /* CRYPTO_ENC */
#endif /* SEC_EXT_CRYPTO */
