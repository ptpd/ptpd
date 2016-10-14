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

#define SIG_SUFFIX  	".sig"
#define PUBLIC_SUFFIX  	".pub"
#define PRIVATE_SUFFIX 	".prv"

int main(int argc, char** args)
{
	ed25519_key key;
	WC_RNG		rng;
	FILE*		output = NULL;
	FILE*		input  = NULL;
	int			ret, length, out_length = ED25519_SIG_SIZE;
	char		path_buffer[1024];
	char		temp_buffer[1024];
	char		prv_buffer[ED25519_KEY_SIZE], pub_buffer[ED25519_PUB_KEY_SIZE];
	char		sig_buffer[ED25519_SIG_SIZE];

	/* check the amount of arguments */
	if( argc != 1 + 3)
	{
		printf("Wrong amount of argumnets, expected 1\n");
		printf("Usage: %s <key file prefix> <file to sign> <signature file name.sig>\n", args[0]);
		return -1;
	}
	
	/* init the key structure */
	wc_ed25519_init( &key );

	/* read the private key from the file */
	strcpy(path_buffer, args[1]);
	strcat(path_buffer, PRIVATE_SUFFIX);
	input = fopen( path_buffer, "r" );
	fread(prv_buffer, sizeof(prv_buffer), 1, input);
	/* close the input file */
	fclose(input);

	/* read the public key from the file */
	strcpy(path_buffer, args[1]);
	strcat(path_buffer, PUBLIC_SUFFIX);
	input = fopen( path_buffer, "r" );
	fread(pub_buffer, sizeof(pub_buffer), 1, input);
	/* close the input file */
	fclose(input);

	/* import the keys */
	if( (ret = wc_ed25519_import_private_key(prv_buffer, ED25519_KEY_SIZE, pub_buffer, ED25519_PUB_KEY_SIZE, &key)) != 0 )
	{
		printf("Failed to import the private key, received error code: %d\n", ret);
		return -2;
	}

	/* read the msg from the file */
	input = fopen( args[2], "r" );
	length = fread(temp_buffer, 1, sizeof(temp_buffer), input);
	if( (ret = wc_ed25519_sign_msg(temp_buffer, length, sig_buffer, &out_length, &key)) != 0 )
	{
		printf("Failed to sign the msg, received error code: %d, on length %d and out length %d\n", ret, length, out_length);
		return -3;
	}	
	/* close the input file */
	fclose(input);

	/* write the signature to a file */
	strcpy(path_buffer, args[3]);
	strcat(path_buffer, SIG_SUFFIX);

	output = fopen(path_buffer, "wb");
	/* check the file open */
	if(output == NULL)
	{
		printf("Failed to open the output file (signature)\n");
		return -4;
	}

	/* write the signature key */
	fwrite(sig_buffer, sizeof(sig_buffer), 1, output);
	
	/* close the file */
	fclose(output);
	printf("Written the signature to the %s file\n", path_buffer);
}
