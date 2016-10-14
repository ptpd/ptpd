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

#include <stdio.h>
#include <stdlib.h>

#define HAVE_ED25519
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/random.h>

#define PUBLIC_SUFFIX  ".pub"
#define PRIVATE_SUFFIX ".prv"

int main(int argc, char** args)
{
	ed25519_key key;
	WC_RNG		rng;
	FILE*		output = NULL;
	int			ret, length;
	char		path_buffer[1024];
	char		temp_buffer[1024];

	/* check the amount of arguments */
	if( argc != 1 + 1)
	{
		printf("Wrong amount of argumnets, expected 1\n");
		printf("Usage: %s <key file prefix>\n", args[0]);
		return -1;
	}

	/* init the RNG */
	if( (ret = wc_InitRng(&rng)) != 0)
	{
		printf("Failed to init the rng, received error code: %d\n", ret);
		return -1;
	}
	
	/* init the key structure */
	if( (ret = wc_ed25519_init( &key )) != 0)
	{
		printf("Failed to init the ed_25519, received error code: %d\n", ret);
		return -1;		
	}

	/* generate the keys */	
	if( (ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key)) != 0)
	{
		printf("Failed to generate the keys, received error code: %d\n", ret);
		return -2;
	}

	/* write the keys to two different files */

	/* 1. the public file */
	strcpy(path_buffer, args[1]);
	strcat(path_buffer, PUBLIC_SUFFIX);

	output = fopen(path_buffer, "wb");
	/* check the file open */
	if(output == NULL)
	{
		printf("Failed to open the output file (public)\n");
		return -3;
	}

	/* write the public key */
	if( (ret = wc_ed25519_export_public(&key, temp_buffer, &length)) != 0 )
	{
		printf("Failed to export the public key, received error code: %d\n", ret);
		return -4;
	}
	fwrite(temp_buffer, length, 1, output);
	
	/* close the file */
	fclose(output);
	printf("Written the public  key to the %s file\n", path_buffer);

	/* 2. the public file */
	strcpy(path_buffer, args[1]);
	strcat(path_buffer, PRIVATE_SUFFIX);

	output = fopen(path_buffer, "wb");
	/* check the file open */
	if(output == NULL)
	{
		printf("Failed to open the output file (private)\n");
		return -5;
	}

	/* write the private key */
	if( (ret = wc_ed25519_export_private_only(&key, temp_buffer, &length)) != 0 )
	{
		printf("Failed to export the private key, received error code: %d\n", ret);
		return -6;
	}
	fwrite(temp_buffer, length, 1, output);
	
	/* close the file */
	fclose(output);
	printf("Written the private key to the %s file\n", path_buffer);
}
