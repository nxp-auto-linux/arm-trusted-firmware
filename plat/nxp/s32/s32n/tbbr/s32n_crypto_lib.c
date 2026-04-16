/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <caam.h>
#include <common/debug.h>
#include <drivers/auth/crypto_mod.h>
#include <drivers/mmc.h>
#include <s32_hse_drv.h>
#include <errno.h>
#include <hash.h>
#include <hse_interface.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <mbedtls/asn1.h>
#include <mbedtls/bignum.h>
#include <mbedtls/md.h>
#include <mbedtls/oid.h>
#include <mbedtls/platform.h>
#include <mbedtls/x509.h>
#include <plat/common/platform.h>

static int hse_load_cnt_table(void)
{
	hseKeyHandle_t rpmb_key_handle = RPMB_KEY_HSE_KEYHANDLE;
	uint8_t rpmb_cnt_table[RPMB_BLOCK_SIZE];
	hseSrvDescriptor_t srv_desc = {0};
	static bool hse_cnt_table_loaded;
	void *hse_cnt_table;
	int ret;

	if (!is_secboot_active() || hse_cnt_table_loaded)
		return 0;

	ret = mmc_rpmb_read_blocks(RPMB_MONOTONIC_CNT_ADDR, (uintptr_t)rpmb_cnt_table,
				RPMB_BLOCK_SIZE, (void *)&rpmb_key_handle, sizeof(rpmb_key_handle));
	if (!ret) {
		VERBOSE("%s: rpmb_read_blocks failed\n", __func__);
		return -EINVAL;
	}

	hse_cnt_table = hse_mem_alloc(HSE_NVM_CONTAINER_CNT_TBL_SIZE);
	if (!hse_cnt_table)
		return -ENOMEM;
	hse_memcpy(hse_cnt_table, rpmb_cnt_table, HSE_NVM_CONTAINER_CNT_TBL_SIZE);

	srv_desc.srvId = HSE_SRV_ID_PUBLISH_LOAD_CNT_TBL;
	srv_desc.hseSrv.publishLoadCntTblReq.action = HSE_NVM_CNT_TBL_ACTION_LOAD;
	srv_desc.hseSrv.publishLoadCntTblReq.pNvmContainerCntTbl = hse_virt_to_phys(hse_cnt_table);

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret)
		VERBOSE("%s: hse_srv_req_sync (%d)\n", __func__, ret);

	hse_mem_free(hse_cnt_table);
	hse_cnt_table_loaded = true;

	return ret;
}

static bool has_monotonic_cnt(void)
{
	return S32_MONOTONIC_CNT_SUPPORT == 1;
}

static inline char *hse_msc_state_to_str(hseMscState_t msc_state)
{
	switch (msc_state) {
	case HSE_MSC_STATE_DEFAULT:
		return "DEFAULT";
	case HSE_MSC_STATE_MANAGED_INIT:
		return "MANAGED_INIT";
	case HSE_MSC_STATE_MANAGED:
		return "MANAGED";
	case HSE_MSC_STATE_OPEN:
		return "OPEN";
	case HSE_MSC_STATE_ERROR:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

static int hse_get_caam_msc_state(void)
{
	hseSrvDescriptor_t srv_desc = {0};
	hseMscState_t msc_state;
	void *msc_state_buff;
	int ret;

	msc_state_buff = hse_mem_alloc(sizeof(msc_state));
	if (!msc_state_buff) {
		VERBOSE("%s: Failed to allocate memory for MSC state\n", __func__);
		return -ENOMEM;
	}

	srv_desc.srvId = HSE_SRV_ID_GET_MSC_STATE;
	srv_desc.hseSrv.mscGetState.id = HSE_MSC_ID_CAAM_0;
	srv_desc.hseSrv.mscGetState.pMscState = hse_virt_to_phys(msc_state_buff);

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret) {
		ERROR("%s: Failed getting CAAM MSC state: %d\n", __func__, ret);
		goto free_msc_state_buff;
	}

	hse_memcpy(&msc_state, msc_state_buff, sizeof(msc_state));

	INFO("%s: CAAM MSC state: %s\n", __func__, hse_msc_state_to_str(msc_state));

free_msc_state_buff:
	hse_mem_free(msc_state_buff);
	return ret;
}

static int s32n_caam_init(void)
{
	int ret;

	ret = hse_get_caam_msc_state();
	if (ret)
		return ret;

	ret = mmap_add_dynamic_region(CAAM_BASE_ADDR, CAAM_BASE_ADDR, 0x200000,
				      MT_DEVICE | MT_RW | MT_SECURE);
	if (ret < 0) {
		ERROR("CAAM: mapping error %d\n", ret);
		return ret;
	}

	/* Initialize CAAM */
	sec_init(CAAM_BASE_ADDR);

	return 0;
}

static void init(void)
{
	int ret;

	ret = hse_driver_init(DEFAULT_RMEM);
	if (ret != 0 && is_secboot_active()) {
		ERROR("S32N Crypto Lib: HSE initialization failed\n");
		panic();
	}

	if (has_monotonic_cnt() && hse_load_cnt_table()) {
		ERROR("S32N Crypto Lib: HSE could not load monotonic counter table\n");
		panic();
	}

	ret = s32n_caam_init();
	if (ret != 0) {
		ERROR("S32N Crypto Lib: CAAM initialization failed\n");
		panic();
	}
}

static hseHashAlgo_t get_hse_mbedtls_hash_algo(mbedtls_md_type_t md_alg)
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

static hseHashAlgo_t get_hse_crypto_hash_algo(enum crypto_md_algo md_alg)
{
	switch (md_alg) {
	case CRYPTO_MD_SHA512:
		return HSE_HASH_ALGO_SHA2_512;
	case CRYPTO_MD_SHA384:
		return HSE_HASH_ALGO_SHA2_384;
	case CRYPTO_MD_SHA256:
		return HSE_HASH_ALGO_SHA2_256;
	default:
		return HSE_HASH_ALGO_NULL;
	}
}

static enum hash_algo get_caam_mbedtls_hash_algo(mbedtls_md_type_t md_alg)
{
	switch (md_alg) {
	case MBEDTLS_MD_SHA512:
		return SHA512;
	case MBEDTLS_MD_SHA384:
		return SHA384;
	case MBEDTLS_MD_SHA256:
		return SHA256;
	default:
		return 0;
	}
}

static hseMacAlgo_t get_hse_crypto_mac_algo(enum crypto_mac_algo mac_alg)
{
	switch (mac_alg) {
	case CRYPTO_MAC_HMAC:
		return HSE_MAC_ALGO_HMAC;
	default:
		return 0;
	}
}

static int caam_calc_hash(void *data_ptr, unsigned int data_len,
			  const mbedtls_md_info_t *md_info, void *hash)
{
	void *ctx = NULL;
	enum hash_algo hash_alg;
	uint32_t md_size;
	int i, ret;

	if (!data_ptr || !data_len || !md_info || !hash)
		return -EINVAL;

	hash_alg = get_caam_mbedtls_hash_algo(mbedtls_md_get_type(md_info));
	if (!hash_alg)
		return -EINVAL;

	md_size = mbedtls_md_get_size(md_info);
	if (!md_size)
		return -EINVAL;

	NOTICE("Verifying hash\n");
	ret = hash_init(hash_alg, &ctx);
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	/* Update hash with that of SRK table */
	ret = hash_update(hash_alg, ctx, data_ptr, data_len);
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	/* Copy hash at destination buffer */
	ret = hash_final(hash_alg, ctx, hash, md_size);
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	VERBOSE("%s Calculated hash\n", __func__);
	for (i = 0; i < md_size / 4; i++) {
		VERBOSE("%x\n", *((uint32_t *)hash + i));
	}

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

/*
 * NOTE: MBEDTLS feature gates RSA and ECDSA based on KEY_ALG make variable.
 * This makes the bootloader binary smaller, but we lose ability to dynamically
 * switch between rsa and ecdsa signing. This is okay, as signing process is
 * controlled build-time and non-trivial to change (from build perspective).
 * Here, we use same feature-gating to avoid compile error + favor smaller
 * bootloader footprint due to this addition.
 */
#if TF_MBEDTLS_USE_ECDSA
static int verify_ecdsa_signature(void *data_ptr, unsigned int data_len,
				  void *sig_ptr, unsigned int sig_len, void *pk_ptr,
				  mbedtls_md_type_t md_alg, void *sig_opts)
{
	void *sig_r_buf = NULL, *sig_r_len_buf = NULL;
	void *sig_s_buf = NULL, *sig_s_len_buf = NULL;
	void *data_buf = NULL;
	hseSrvDescriptor_t srv_desc = {0};
	hseSignSrv_t *sign_req = &srv_desc.hseSrv.signReq;
	hseSignScheme_t *sign_scheme = &sign_req->signScheme;
	unsigned char r_buf[MBEDTLS_ECP_MAX_BYTES];
	unsigned char s_buf[MBEDTLS_ECP_MAX_BYTES];
	unsigned char *p, *end;
	size_t len, r_len, s_len;
	mbedtls_mpi r, s;
	int ret;

	/* Get the signature (bitstring) */
	p = (unsigned char *)sig_ptr;
	end = (unsigned char *)(p + sig_len);
	ret = mbedtls_asn1_get_bitstring_null(&p, end, &len);
	if (ret != 0) {
		VERBOSE("%s: mbedtls_asn1_get_bitstring_null (%d)\n", __func__, ret);
		goto sig_out;
	}

	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
	if (ret != 0) {
		VERBOSE("%s: mbedtls_asn1_get_tag (%d)\n", __func__, ret);
		goto sig_out;
	}

	if (p + len != end) {
		VERBOSE("%s: mbedtls_asn1_get_tag (%d)\n", __func__, ret);
		ret = -EINVAL;
		goto sig_out;
	}

	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	ret = mbedtls_asn1_get_mpi(&p, end, &r);
	if (ret != 0) {
		VERBOSE("%s: mbedtls_asn1_get_mpi (%d)\n", __func__, ret);
		goto free_mpi;
	}

	ret = mbedtls_asn1_get_mpi(&p, end, &s);
	if (ret != 0) {
		VERBOSE("%s: mbedtls_asn1_get_mpi (%d)\n", __func__, ret);
		goto free_mpi;
	}

	r_len = mbedtls_mpi_size(&r);
	s_len = mbedtls_mpi_size(&s);

	ret = mbedtls_mpi_write_binary(&r, r_buf, sizeof(r_buf));
	if (ret != 0) {
		VERBOSE("%s: mbedtls_mpi_write_binary (%d)\n", __func__, ret);
		goto free_mpi;
	}

	ret = mbedtls_mpi_write_binary(&s, s_buf, sizeof(s_buf));
	if (ret != 0) {
		VERBOSE("%s: mbedtls_mpi_write_binary (%d)\n", __func__, ret);
		goto free_mpi;
	}

	ret = -ENOMEM;

	sig_r_buf = hse_mem_alloc(r_len);
	if (!sig_r_buf)
		goto free_mpi;
	hse_memcpy(sig_r_buf, r_buf, r_len);

	sig_r_len_buf = hse_mem_alloc(sizeof(uint32_t));
	if (!sig_r_len_buf)
		goto free_sig_r_buf;
	hse_memcpy(sig_r_len_buf, &r_len, sizeof(uint32_t));

	sig_s_buf = hse_mem_alloc(s_len);
	if (!sig_s_buf)
		goto free_sig_r_len_buf;
	hse_memcpy(sig_s_buf, s_buf, s_len);

	sig_s_len_buf = hse_mem_alloc(sizeof(uint32_t));
	if (!sig_s_len_buf)
		goto free_sig_s_buf;
	hse_memcpy(sig_s_len_buf, &s_len, sizeof(uint32_t));

	data_buf = hse_mem_alloc(data_len);
	if (!data_buf)
		goto free_sig_s_len_buf;
	hse_memcpy(data_buf, data_ptr, data_len);

	srv_desc.srvId = HSE_SRV_ID_SIGN;
	sign_req->accessMode = HSE_ACCESS_MODE_ONE_PASS;
	sign_req->authDir = HSE_AUTH_DIR_VERIFY;
	sign_req->bInputIsHashed = false;
	sign_req->keyHandle = *(hseKeyHandle_t *)pk_ptr;
	sign_req->sgtOption = HSE_SGT_OPTION_NONE;
	sign_req->inputLength = data_len;
	sign_req->pInput = hse_virt_to_phys(data_buf);
	sign_req->pSignatureLength[0] = hse_virt_to_phys(sig_r_len_buf);
	sign_req->pSignatureLength[1] = hse_virt_to_phys(sig_s_len_buf);
	sign_req->pSignature[0] = hse_virt_to_phys(sig_r_buf);
	sign_req->pSignature[1] = hse_virt_to_phys(sig_s_buf);

	sign_scheme->signSch = HSE_SIGN_ECDSA;
	sign_scheme->sch.ecdsa.hashAlgo = get_hse_mbedtls_hash_algo(md_alg);

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret != 0) {
		VERBOSE("%s: hse_srv_req_sync (%d)\n", __func__, ret);
		goto free_data_buf;
	}
	ret = CRYPTO_SUCCESS;

free_data_buf:
	hse_mem_free(data_buf);
free_sig_s_len_buf:
	hse_mem_free(sig_s_len_buf);
free_sig_s_buf:
	hse_mem_free(sig_s_buf);
free_sig_r_len_buf:
	hse_mem_free(sig_r_len_buf);
free_sig_r_buf:
	hse_mem_free(sig_r_buf);
free_mpi:
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&s);
sig_out:
	if (ret != CRYPTO_SUCCESS)
		ret = CRYPTO_ERR_SIGNATURE;
	return ret;
}

static int verify_rsa_signature(void *data_ptr, unsigned int data_len,
				void *sig_ptr, unsigned int sig_len, void *pk_ptr,
				mbedtls_md_type_t md_alg, void *sig_opts)
{
	ERROR("Unsupported key alg: rsa\n");
	return CRYPTO_ERR_SIGNATURE;
}
#else
static int verify_ecdsa_signature(void *data_ptr, unsigned int data_len,
				  void *sig_ptr, unsigned int sig_len, void *pk_ptr,
				  mbedtls_md_type_t md_alg, void *sig_opts)
{
	ERROR("Unsupported key alg: ecdsa\n");
	return CRYPTO_ERR_SIGNATURE;
}

static int verify_rsa_signature(void *data_ptr, unsigned int data_len,
				void *sig_ptr, unsigned int sig_len, void *pk_ptr,
				mbedtls_md_type_t md_alg, void *sig_opts)
{
	unsigned char hash[MBEDTLS_MD_MAX_SIZE] __aligned(CACHE_WRITEBACK_GRANULE) = {0};
	void *sig_buf = NULL, *data_buf = NULL, *sig_len_buf = NULL;
	hseSrvDescriptor_t srv_desc = {0};
	hseSignSrv_t *sign_req = &srv_desc.hseSrv.signReq;
	hseSignScheme_t *sign_scheme = &sign_req->signScheme;
	mbedtls_pk_rsassa_pss_options *pss_opts;
	const mbedtls_md_info_t *md_info;
	uint32_t md_size;
	unsigned char *p, *end;
	size_t len;
	int ret;

	/* Get the signature (bitstring) */
	p = (unsigned char *)sig_ptr;
	end = (unsigned char *)(p + sig_len);
	ret = mbedtls_asn1_get_bitstring_null(&p, end, &len);
	if (ret != 0) {
		VERBOSE("%s: mbedtls_asn1_get_bitstring_null (%d)\n", __func__, ret);
		goto sig_out;
	}
	pss_opts = (mbedtls_pk_rsassa_pss_options *)sig_opts;

	md_info = mbedtls_md_info_from_type(md_alg);
	if (md_info == NULL) {
		VERBOSE("%s: unsupported hash algorithm\n", __func__);
		return CRYPTO_ERR_SIGNATURE;
	}
	md_size = mbedtls_md_get_size(md_info);

	/* Compute hash of the data using CAAM */
	ret = caam_calc_hash(data_ptr, data_len, md_info, hash);
	if (ret != 0) {
		VERBOSE("%s: caam_calc_hash failed (%d)\n", __func__, ret);
		return CRYPTO_ERR_SIGNATURE;
	}

	ret = -ENOMEM;

	sig_buf = hse_mem_alloc(len);
	if (!sig_buf)
		goto sig_out;
	hse_memcpy(sig_buf, p, len);

	if (hse_sig_req_set_blen(sign_req, len, &sig_len_buf, sig_buf))
		goto free_sig_buf;

	data_buf = hse_mem_alloc(md_size);
	if (!data_buf)
		goto free_sig_len_buf;
	hse_memcpy(data_buf, hash, md_size);

	srv_desc.srvId = HSE_SRV_ID_SIGN;
	sign_req->accessMode = HSE_ACCESS_MODE_ONE_PASS;
	sign_req->authDir = HSE_AUTH_DIR_VERIFY;
	sign_req->bInputIsHashed = true;
	sign_req->keyHandle = *(hseKeyHandle_t *)pk_ptr;
	sign_req->sgtOption = HSE_SGT_OPTION_NONE;
	sign_req->inputLength = md_size;
	sign_req->pInput = hse_virt_to_phys(data_buf);

	sign_scheme->signSch = HSE_SIGN_RSASSA_PSS;
	sign_scheme->sch.rsaPss.hashAlgo = get_hse_mbedtls_hash_algo(md_alg);
	sign_scheme->sch.rsaPss.saltLength = pss_opts->expected_salt_len;

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret != 0) {
		VERBOSE("%s: hse_srv_req_sync (%d)\n", __func__, ret);
		goto free_data_buf;
	}

	ret = CRYPTO_SUCCESS;

free_data_buf:
	hse_mem_free(data_buf);
free_sig_len_buf:
	hse_sig_req_free(sig_len_buf);
free_sig_buf:
	hse_mem_free(sig_buf);
sig_out:
	if (ret != CRYPTO_SUCCESS)
		ret = CRYPTO_ERR_SIGNATURE;
	return ret;
}
#endif

static int verify_signature(void *data_ptr, unsigned int data_len,
			    void *sig_ptr, unsigned int sig_len,
			    void *sig_alg, unsigned int sig_alg_len,
			    void *pk_ptr, unsigned int pk_len)
{
	mbedtls_asn1_buf sig_oid, sig_params;
	mbedtls_md_type_t md_alg;
	mbedtls_pk_type_t pk_alg;
	unsigned char *p, *end;
	void *sig_opts = NULL;
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
		VERBOSE("%s: mbedtls_oid_get_sig_alg (%x)\n", __func__, ret);
		return CRYPTO_ERR_SIGNATURE;
	}

	/* We're only supporting RSA and ECDSA signatures */
	switch (pk_alg) {
	case MBEDTLS_PK_RSASSA_PSS:
		ret = verify_rsa_signature(data_ptr, data_len, sig_ptr, sig_len,
					   pk_ptr, md_alg, sig_opts);
		break;
	case MBEDTLS_PK_ECDSA:
		ret = verify_ecdsa_signature(data_ptr, data_len, sig_ptr, sig_len,
					     pk_ptr, md_alg, sig_opts);
		break;
	default:
		ret = CRYPTO_ERR_UNKNOWN;
		break;
	}

	mbedtls_free(sig_opts);
	if (ret != CRYPTO_SUCCESS)
		ret = CRYPTO_ERR_SIGNATURE;

	return ret;
}

static int verify_hash(void *data_ptr, unsigned int data_len,
		       void *digest_info_ptr, unsigned int digest_info_len)
{
	unsigned char data_hash[MBEDTLS_MD_MAX_SIZE] __aligned(CACHE_WRITEBACK_GRANULE) = {0};
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

	ret = caam_calc_hash(data_ptr, data_len, md_info, data_hash);
	if (ret != 0)
		return CRYPTO_ERR_HASH;

	/* Compare values */
	ret = memcmp(data_hash, hash, mbedtls_md_get_size(md_info));
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	return CRYPTO_SUCCESS;
}

static int calc_mac(crypto_mac_desc_t *mac_desc, void *data_ptr,
			unsigned int data_len, void *key,
			unsigned int key_len,
			unsigned char output[CRYPTO_MD_MAX_SIZE])
{
	void *data_buf = NULL, *mac_len_buf = NULL, *mac_buf = NULL;
	uint32_t mac_len = CRYPTO_MD_MAX_SIZE;
	hseSrvDescriptor_t srv_desc = {0};
	hseMacSrv_t *mac_srv = &srv_desc.hseSrv.macReq;
	int ret;

	data_buf = hse_mem_alloc(data_len);
	if (!data_buf)
		return -ENOMEM;
	hse_memcpy(data_buf, data_ptr, data_len);

	if (hse_mac_req_set_tlen(mac_srv, mac_len, &mac_len_buf)) {
		ret = -ENOMEM;
		goto free_data_buf;
	}

	mac_buf = hse_mem_alloc(mac_len);
	if (!mac_buf) {
		ret = -ENOMEM;
		goto free_mac_len_buf;
	}
	srv_desc.srvId = HSE_SRV_ID_MAC;

	mac_srv->macScheme.macAlgo = get_hse_crypto_mac_algo(mac_desc->algo);
	mac_srv->macScheme.sch.hmac.hashAlgo = get_hse_crypto_hash_algo(mac_desc->hmac.md_algo);
	mac_srv->accessMode = HSE_ACCESS_MODE_ONE_PASS;
	mac_srv->streamId = 0;
	mac_srv->authDir = HSE_AUTH_DIR_GENERATE;
	mac_srv->sgtOption = HSE_SGT_OPTION_NONE;
	mac_srv->keyHandle = *(hseKeyHandle_t *)key;
	mac_srv->inputLength = data_len;
	mac_srv->pInput = hse_virt_to_phys(data_buf);
	mac_srv->pTag = hse_virt_to_phys(mac_buf);

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret) {
		VERBOSE("%s: hse_srv_req_sync (%d)\n", __func__, ret);
		goto free_mac_buf;
	}

	hse_mac_req_update_tlen(&mac_len, mac_len_buf);
	hse_memcpy(output, mac_buf, mac_len);
	ret = CRYPTO_SUCCESS;

free_mac_buf:
	hse_mem_free(mac_buf);
free_mac_len_buf:
	hse_mac_req_free(mac_len_buf);
free_data_buf:
	hse_mem_free(data_buf);

	return ret;
}

REGISTER_CRYPTO_LIB("s32n_crypto_lib",
		    init,
		    verify_signature,
		    verify_hash,
		    NULL,
		    calc_mac,
		    NULL,
		    NULL);
