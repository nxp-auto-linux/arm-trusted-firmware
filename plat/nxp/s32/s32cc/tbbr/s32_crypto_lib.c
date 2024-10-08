/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/auth/crypto_mod.h>
#include <drivers/nxp/s32/hse/hse_core.h>
#include <drivers/nxp/s32/hse/hse_mem.h>
#include <errno.h>
#include <hse_interface.h>
#include <mbedtls/asn1.h>
#include <mbedtls/md.h>
#include <mbedtls/oid.h>
#include <mbedtls/platform.h>
#include <mbedtls/x509.h>
#include <plat/common/platform.h>

static void init(void)
{
	int ret;

	ret = hse_driver_init();
	if (ret != 0 && is_secboot_active()) {
		ERROR("S32 HSE Crypto Lib initialization failed\n");
		panic();
	}
}

static hseHashAlgo_t get_hse_hash_algo(mbedtls_md_type_t md_alg)
{
	switch (md_alg) {
	case MBEDTLS_MD_SHA512:
		return HSE_HASH_ALGO_SHA2_512;
	case MBEDTLS_MD_SHA384:
		return HSE_HASH_ALGO_SHA2_384;
	case MBEDTLS_MD_SHA256:
		return HSE_HASH_ALGO_SHA2_256;
	default:
		return HSE_HASH_ALGO_NULL;
	}
}

static int hse_calc_hash(void *data_ptr, unsigned int data_len,
			 const mbedtls_md_info_t *md_info, void *hash)
{
	void *data_buf = NULL, *hash_len_buf = NULL, *hash_buf = NULL;
	hseSrvDescriptor_t srv_desc = {0};
	hseHashAlgo_t hash_alg;
	uint8_t md_size;
	int ret;

	if (!data_ptr || !data_len || !md_info || !hash)
		return -EINVAL;

	hash_alg = get_hse_hash_algo(mbedtls_md_get_type(md_info));
	if (hash_alg == HSE_HASH_ALGO_NULL)
		return -EINVAL;

	md_size = mbedtls_md_get_size(md_info);
	if (!md_size)
		return -EINVAL;

	data_buf = hse_mem_alloc(data_len);
	if (!data_buf)
		return -ENOMEM;
	hse_memcpy(data_buf, data_ptr, data_len);

	hash_len_buf = hse_mem_alloc(sizeof(uint32_t));
	if (!hash_len_buf) {
		ret = -ENOMEM;
		goto free_data_buf;
	}
	hse_memcpy(hash_len_buf, &md_size, sizeof(uint32_t));

	hash_buf = hse_mem_alloc(md_size);
	if (!hash_buf) {
		ret = -ENOMEM;
		goto free_hash_len_buf;
	}
	srv_desc.srvId = HSE_SRV_ID_HASH;
	srv_desc.hseSrv.hashReq.accessMode = HSE_ACCESS_MODE_ONE_PASS;
	srv_desc.hseSrv.hashReq.sgtOption = HSE_SGT_OPTION_NONE;
	srv_desc.hseSrv.hashReq.hashAlgo = hash_alg;
	srv_desc.hseSrv.hashReq.inputLength = data_len;
	srv_desc.hseSrv.hashReq.pInput = hse_virt_to_phys(data_buf);
	srv_desc.hseSrv.hashReq.pHashLength = hse_virt_to_phys(hash_len_buf);
	srv_desc.hseSrv.hashReq.pHash = hse_virt_to_phys(hash_buf);

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret) {
		VERBOSE("%s: hse_srv_req_sync (%d)\n", __func__, ret);
		goto free_hash_buf;
	}

	hse_memcpy(hash, hash_buf, md_size);

free_hash_buf:
	hse_mem_free(hash_buf);
free_hash_len_buf:
	hse_mem_free(hash_len_buf);
free_data_buf:
	hse_mem_free(data_buf);

	return ret;
}

/*
 * NOTE: This has been made internal in mbedtls 3.6.0 and the mbedtls team has
 * advised that it's better to copy out the declaration than it would be to
 * update to 3.5.2, where this function is exposed.
 */
int mbedtls_x509_get_sig_alg(const mbedtls_x509_buf *sig_oid,
			     const mbedtls_x509_buf *sig_params,
			     mbedtls_md_type_t *md_alg,
			     mbedtls_pk_type_t *pk_alg,
			     void **sig_opts);

static int verify_signature(void *data_ptr, unsigned int data_len,
			    void *sig_ptr, unsigned int sig_len,
			    void *sig_alg, unsigned int sig_alg_len,
			    void *pk_ptr, unsigned int pk_len)
{
	void *sig_buf = NULL, *hash_buf = NULL, *sig_len_buf = NULL;
	hseSrvDescriptor_t srv_desc = {0};
	hseSignSrv_t *sign_req = &srv_desc.hseSrv.signReq;
	hseSignScheme_t *sign_scheme = &sign_req->signScheme;
	mbedtls_pk_rsassa_pss_options *pss_opts;
	unsigned char hash[MBEDTLS_MD_MAX_SIZE];
	mbedtls_asn1_buf sig_oid, sig_params;
	const mbedtls_md_info_t *md_info;
	mbedtls_md_type_t md_alg;
	mbedtls_pk_type_t pk_alg;
	unsigned char *p, *end;
	void *sig_opts = NULL;
	uint8_t md_size;
	size_t len;
	int ret;

	/* Get pointers to signature OID and parameters */
	p = (unsigned char *)sig_alg;
	end = (unsigned char *)(p + sig_alg_len);
	ret = mbedtls_asn1_get_alg(&p, end, &sig_oid, &sig_params);
	if (ret != 0) {
		VERBOSE("%s: mbedtls_asn1_get_alg (%d)\n", __func__, ret);
		return CRYPTO_ERR_SIGNATURE;
	}

	/* Get the actual signature algorithm (MD + PK) */
	ret = mbedtls_x509_get_sig_alg(&sig_oid, &sig_params, &md_alg, &pk_alg, &sig_opts);
	if (ret != 0) {
		VERBOSE("%s: mbedtls_oid_get_sig_alg (%d)\n", __func__, ret);
		return CRYPTO_ERR_SIGNATURE;
	}
	pss_opts = (mbedtls_pk_rsassa_pss_options *)sig_opts;

	/* We're only supporting RSA signatures */
	if (pk_alg != MBEDTLS_PK_RSASSA_PSS)
		goto free_sig_opts;

	/* Get the signature (bitstring) */
	p = (unsigned char *)sig_ptr;
	end = (unsigned char *)(p + sig_len);
	ret = mbedtls_asn1_get_bitstring_null(&p, end, &len);
	if (ret != 0) {
		VERBOSE("%s: mbedtls_asn1_get_bitstring_null (%d)\n", __func__, ret);
		goto free_sig_opts;
	}

	sig_buf = hse_mem_alloc(len);
	if (!sig_buf)
		goto free_sig_opts;
	hse_memcpy(sig_buf, p, len);

	sig_len_buf = hse_mem_alloc(sizeof(uint32_t));
	if (!sig_len_buf)
		goto free_sig_buf;
	hse_memcpy(sig_len_buf, &len, sizeof(uint32_t));

	/* Get info about the hash algorithm */
	md_info = mbedtls_md_info_from_type(md_alg);
	if (!md_info)
		goto free_sig_len_buf;
	md_size = mbedtls_md_get_size(md_info);

	/* Calculate the hash of the data */
	ret = hse_calc_hash(data_ptr, data_len, md_info, hash);
	if (ret != 0)
		goto free_sig_len_buf;

	hash_buf = hse_mem_alloc(md_size);
	if (!hash_buf)
		goto free_sig_len_buf;
	hse_memcpy(hash_buf, hash, md_size);

	srv_desc.srvId = HSE_SRV_ID_SIGN;
	sign_req->accessMode = HSE_ACCESS_MODE_ONE_PASS;
	sign_req->authDir = HSE_AUTH_DIR_VERIFY;
	sign_req->bInputIsHashed = true;
	sign_req->keyHandle = *(hseKeyHandle_t *)pk_ptr;
	sign_req->sgtOption = HSE_SGT_OPTION_NONE;
	sign_req->inputLength = md_size;
	sign_req->pInput = hse_virt_to_phys(hash_buf);
	sign_req->pSignatureLength[0] = hse_virt_to_phys(sig_len_buf);
	sign_req->pSignatureLength[1] = 0u;
	sign_req->pSignature[0] = hse_virt_to_phys(sig_buf);
	sign_req->pSignature[1] = 0u;

	sign_scheme->signSch = HSE_SIGN_RSASSA_PSS;
	sign_scheme->sch.rsaPss.hashAlgo = get_hse_hash_algo(md_alg);
	sign_scheme->sch.rsaPss.saltLength = pss_opts->expected_salt_len;

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret != 0) {
		VERBOSE("%s: hse_srv_req_sync (%d)\n", __func__, ret);
		goto free_hash_buf;
	}

	ret = CRYPTO_SUCCESS;

free_hash_buf:
	hse_mem_free(hash_buf);
free_sig_len_buf:
	hse_mem_free(sig_len_buf);
free_sig_buf:
	hse_mem_free(sig_buf);
free_sig_opts:
	mbedtls_free(sig_opts);

	if (ret != CRYPTO_SUCCESS)
		ret = CRYPTO_ERR_SIGNATURE;

	return ret;
}

static int verify_hash(void *data_ptr, unsigned int data_len,
		       void *digest_info_ptr, unsigned int digest_info_len)
{
	unsigned char data_hash[MBEDTLS_MD_MAX_SIZE];
	mbedtls_asn1_buf hash_oid, params;
	const mbedtls_md_info_t *md_info;
	unsigned char *p, *end, *hash;
	mbedtls_md_type_t md_alg;
	size_t len;
	int ret;

	/*
	 * Digest info should be an MBEDTLS_ASN1_SEQUENCE, but padding after
	 * it is allowed. This is necessary to support multiple hash
	 * algorithms.
	 */
	p = (unsigned char *)digest_info_ptr;
	end = p + digest_info_len;
	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED |
				  MBEDTLS_ASN1_SEQUENCE);
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	end = p + len;

	/* Get the hash algorithm */
	ret = mbedtls_asn1_get_alg(&p, end, &hash_oid, &params);
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	ret = mbedtls_oid_get_md_alg(&hash_oid, &md_alg);
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	md_info = mbedtls_md_info_from_type(md_alg);
	if (md_info == NULL) {
		return CRYPTO_ERR_HASH;
	}

	/* Hash should be octet string type and consume all bytes */
	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OCTET_STRING);
	if ((ret != 0) || ((size_t)(end - p) != len)) {
		return CRYPTO_ERR_HASH;
	}

	/* Length of hash must match the algorithm's size */
	if (len != mbedtls_md_get_size(md_info)) {
		return CRYPTO_ERR_HASH;
	}
	hash = p;

	ret = hse_calc_hash(data_ptr, data_len, md_info, data_hash);
	if (ret != 0)
		return CRYPTO_ERR_HASH;

	/* Compare values */
	ret = memcmp(data_hash, hash, mbedtls_md_get_size(md_info));
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	return CRYPTO_SUCCESS;
}

REGISTER_CRYPTO_LIB("s32_crypto_lib",
		    init,
		    verify_signature,
		    verify_hash,
		    NULL,
		    NULL,
		    NULL);
