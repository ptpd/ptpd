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

#include "enc.h"
#include "../../ptpd.h"
#include "../../datatypes.h"
#include <string.h>

#ifdef TIME_CRYPTO
#include <time.h>

struct timespec tstart={0,0}, tend={0,0};

void start_timer()
{
    clock_gettime(CLOCK_MONOTONIC, &tstart);
}

void end_timer(char* cause)
{
    clock_gettime(CLOCK_MONOTONIC, &tend);
    INFO("%s took about %.9f nanoseconds\n", cause,
           ((double)(tend.tv_sec   * 1.0e-9) + tend.tv_nsec) - 
           ((double)(tstart.tv_sec * 1.0e-9) + tstart.tv_nsec));
}

#endif /* TIME_CRYPTO */

/************************/
/**       EdDSA        **/
/************************/
#ifdef USING_EdDSA

void init_key(key_type* key)
{
	wc_ed25519_init( key );
}

void signMsg(key_type *key, Octet* buf, size_t size, Signature* sig, prng_type* rng)
{
	int length = SIGNATURE_SIZE;
#ifdef TIME_CRYPTO
	start_timer();
#endif /* TIME_CRYPTO */

	/* sign the msg (hashes on it's own) */
    if( wc_ed25519_sign_msg( buf, size,
							 sig->sig, &length,
							 key ) != 0 )
    {
        WARNING("signMSG Failed :(\n");
    }

#ifdef TIME_CRYPTO
	end_timer("signMsg");
#endif /* TIME_CRYPTO */
}

Boolean verifyMsg(key_type *key, Octet* buf, size_t size, Octet* sig)
{
	int stat = 0;

#ifdef TIME_CRYPTO
	start_timer();
#endif /* TIME_CRYPTO */

	/* verify the signature (hashes the msg on it's own) */
	Integer16 pk_ret;
	if( (pk_ret = wc_ed25519_verify_msg( sig, SIGNATURE_SIZE,
										 buf, size,
										 &stat, key)) != 0 )
    {
        WARNING("verifyMSG Failed (%d) on signature\n", pk_ret);
		return FALSE;
    }

#ifdef TIME_CRYPTO
	end_timer("verifyMsg");
#endif /* TIME_CRYPTO */

	return stat == 1 ? TRUE : FALSE;
}

Boolean packPublicKey(Octet* buf, key_type *key, size_t* len)
{
	Integer16 pk_ret;
	int length = PUBLIC_KEY_SIZE;
	if( (pk_ret = wc_ed25519_export_public( key, buf, &length ) ) < 0 )
    {
		WARNING("packPublicKey Failed (%d)\n", pk_ret);
	 	return FALSE;
    }

	*len += length;
    return TRUE;
}

Boolean unpackPublicKey(Octet* buf, key_type *key)
{
	/* init the key */
	wc_ed25519_init(key);

	/* read the point */
	if (wc_ed25519_import_public( buf, PUBLIC_KEY_SIZE, key ) < 0)
	{
		return FALSE;
	}
	
	return TRUE;
}

Boolean importPublicKey(key_type *key, char* path)
{
	char pub_buffer[ED25519_PUB_KEY_SIZE];
	FILE* input = NULL;
	int ret;

	/* init the key structure */
	wc_ed25519_init( key );

	/* read the public key from the file */
	input = fopen( path, "r" );
	fread(pub_buffer, sizeof(pub_buffer), 1, input);
	/* close the input file */
	fclose(input);

	/* import the key */
	if( (ret = wc_ed25519_import_public(pub_buffer, ED25519_PUB_KEY_SIZE, key)) != 0 )
	{
		return FALSE;
	}
	
	return TRUE;
}

Boolean importPrivateKey(key_type *key, char* path, key_type *pub_key)
{
	char prv_buffer[ED25519_KEY_SIZE];
	FILE* input = NULL;
	int ret;

	/* init the key structure */
	wc_ed25519_init( key );

	/* read the public key from the file */
	input = fopen( path, "r" );
	fread(prv_buffer, sizeof(prv_buffer), 1, input);
	/* close the input file */
	fclose(input);

	/* import the key */
	if( (ret = wc_ed25519_import_private_key(prv_buffer, ED25519_KEY_SIZE, pub_key->p, ED25519_PUB_KEY_SIZE, key)) != 0 )
	{
		return FALSE;
	}
	
	return TRUE;
}

#endif /* Crypto algorithm */

#endif /* SEC_EXT_CRYPTO */
