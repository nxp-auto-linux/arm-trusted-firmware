/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <drivers/auth/crypto_mod.h>

static void init(void)
{

}

static int verify_signature(void *data_ptr, unsigned int data_len,
			    void *sig_ptr, unsigned int sig_len,
			    void *sig_alg, unsigned int sig_alg_len,
			    void *pk_ptr, unsigned int pk_len)
{
	return CRYPTO_SUCCESS;
}

static int verify_hash(void *data_ptr, unsigned int data_len,
		       void *digest_info_ptr, unsigned int digest_info_len)
{
	return CRYPTO_SUCCESS;
}

REGISTER_CRYPTO_LIB("s32_crypto_lib",
		    init,
		    verify_signature,
		    verify_hash,
		    NULL,
		    NULL,
		    NULL);
