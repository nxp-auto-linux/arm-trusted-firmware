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
#include <endian.h>
#include <errno.h>
#include <hash.h>
#include <hse_interface.h>
#include <jobdesc.h>
#include <limits.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <mbedtls/asn1.h>
#include <mbedtls/bignum.h>
#include <mbedtls/md.h>
#include <mbedtls/oid.h>
#include <mbedtls/platform.h>
#include <mbedtls/rsa.h>
#include <mbedtls/x509.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <s32_bl_common.h>
#include <s32n_caam.h>
#include <s32n_fconf_caam_getter.h>

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

static int hse_get_caam_msc_state(unsigned int caam_instance)
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
	srv_desc.hseSrv.mscGetState.id = s32n_caam_msc_id(caam_instance);
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
	uintptr_t jr_base;
	size_t jr_size;
	unsigned int instance;
	int ret;

	jr_base = FCONF_GET_PROPERTY(hw_config, caam_config, jr_base);
	jr_size = FCONF_GET_PROPERTY(hw_config, caam_config, jr_size);
	instance = FCONF_GET_PROPERTY(hw_config, caam_config, instance);
	if (jr_base == 0U || jr_size == 0U) {
		ERROR("CAAM: no valid secure Job Ring found enabled in device tree (base 0x%lx, size 0x%zx)\n",
		      jr_base, jr_size);
		return -ENODEV;
	}

	ret = hse_get_caam_msc_state(instance);
	if (ret)
		return ret;

	/*
	 * Only map the Job Ring's own register page; the CAAM Manager Page
	 * is not mapped, since it is owned by HSE2 and is not accessible to
	 * TF-A.
	 */
	ret = mmap_add_dynamic_region(jr_base, jr_base,
				      MMU_ROUND_UP_TO_PAGE(jr_size),
				      MT_DEVICE | MT_RW | MT_SECURE);
	if (ret < 0) {
		ERROR("CAAM: mapping error %d\n", ret);
		return ret;
	}

	/* Initialize CAAM, using the Job Ring found in the device tree */
	sec_init_jr(jr_base);

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

#if TF_MBEDTLS_USE_ECDSA
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
#endif

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
	uint32_t i;
	int ret;

	if (!data_ptr || !data_len || !md_info || !hash)
		return -EINVAL;

	hash_alg = get_caam_mbedtls_hash_algo(mbedtls_md_get_type(md_info));
	if (!hash_alg)
		return -EINVAL;

	md_size = mbedtls_md_get_size(md_info);
	if (!md_size)
		return -EINVAL;

	VERBOSE("%s: computing hash via CAAM\n", __func__);
	ret = hash_init(hash_alg, &ctx);
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	ret = hash_update(hash_alg, ctx, data_ptr, data_len);
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	/* Copy hash at destination buffer */
	ret = hash_final(hash_alg, ctx, hash, md_size);
	if (ret != 0) {
		return CRYPTO_ERR_HASH;
	}

	VERBOSE("%s: calculated hash\n", __func__);
	for (i = 0; i < md_size / sizeof(uint32_t); i++) {
		VERBOSE("%x\n", *((uint32_t *)hash + i));
	}

	return CRYPTO_SUCCESS;
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

/*
 * Get RSA key information from HSE2 firmware.
 * Returns modulus bit length and public exponent size.
 */
static int hse_get_rsa_key_info(hseKeyHandle_t key_handle,
				unsigned int *mod_bit_len,
				uint8_t *pub_exp_size)
{
	hseSrvDescriptor_t srv_desc = {0};
	hseGetKeyInfoSrv_t *key_info_req = &srv_desc.hseSrv.getKeyInfoReq;
	void *key_info_buf = NULL;
	hseKeyInfo_t key_info;
	int ret;

	*mod_bit_len = 0U;
	*pub_exp_size = 0U;

	key_info_buf = hse_mem_alloc(sizeof(hseKeyInfo_t));
	if (!key_info_buf)
		return -ENOMEM;

	srv_desc.srvId = HSE_SRV_ID_GET_KEY_INFO;
	key_info_req->keyHandle = key_handle;
	key_info_req->pKeyInfo = hse_virt_to_phys(key_info_buf);

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret != 0) {
		VERBOSE("%s: hse_srv_req_sync failed (%d)\n", __func__, ret);
		goto free_key_info;
	}

	hse_memcpy(&key_info, key_info_buf, sizeof(key_info));

	*mod_bit_len = key_info.keyBitLen;
	*pub_exp_size = key_info.specific.pubExponentSize;

free_key_info:
	hse_mem_free(key_info_buf);
	return ret;
}

/* Length of the MGF1 counter C, an octet string of 4 bytes (RFC 8017 B.2.1) */
#define MGF1_COUNTER_LEN		4U

/* RSASSA-PSS encoding (EMSA-PSS, RFC 8017 Section 9.1): */
/* EM = maskedDB || H || trailerField, with trailerField = 0xbc */
#define PSS_TRAILER_FIELD		0xBCU
#define PSS_TRAILER_LEN			1U
/* DB = PS || 0x01 || salt, with PS being a (possibly empty) run of 0x00 */
#define PSS_DB_SEPARATOR		0x01U
#define PSS_DB_SEPARATOR_LEN		1U
/* Minimum EM overhead: trailerField and separator (zero-length PS and salt) */
#define PSS_MIN_OVERHEAD		(PSS_TRAILER_LEN + PSS_DB_SEPARATOR_LEN)
/* M' = (0x)00 00 00 00 00 00 00 00 || mHash || salt */
#define PSS_M_PRIME_PADDING_LEN		8U
/*
 * Leftmost bit(s) of maskedDB that must be zero, i.e. 8 * emLen - emBits
 * of them. The key's modulus bit length is required to be a multiple of 8
 * (see verify_rsa_signature()), so emBits = modBits - 1, emLen = modBits / 8
 * and thus exactly one bit has to be cleared.
 */
#define PSS_DB_LEFTMOST_BIT_MASK	0x80U

/*
 * MGF1 mask generation function as per RFC 8017 Section B.2.1
 */
static int mgf1_xor_mask(const unsigned char *seed, size_t seed_len,
			 unsigned char *mask, size_t mask_len,
			 const mbedtls_md_info_t *md_info)
{
	mbedtls_md_context_t ctx;
	unsigned char counter[MGF1_COUNTER_LEN];
	unsigned char hash_buf[MBEDTLS_MD_MAX_SIZE];
	size_t hash_len = mbedtls_md_get_size(md_info);
	size_t offset = 0;
	uint32_t i = 0;
	int ret;

	mbedtls_md_init(&ctx);
	ret = mbedtls_md_setup(&ctx, md_info, 0);
	if (ret != 0) {
		mbedtls_md_free(&ctx);
		return ret;
	}

	while (offset < mask_len) {
		be32enc(counter, i);

		ret = mbedtls_md_starts(&ctx);
		if (ret != 0)
			break;

		ret = mbedtls_md_update(&ctx, seed, seed_len);
		if (ret != 0)
			break;

		ret = mbedtls_md_update(&ctx, counter, sizeof(counter));
		if (ret != 0)
			break;

		ret = mbedtls_md_finish(&ctx, hash_buf);
		if (ret != 0)
			break;

		/* XOR with mask */
		for (size_t j = 0; j < hash_len && offset < mask_len; j++, offset++) {
			mask[offset] ^= hash_buf[j];
		}

		i++;
	}

	mbedtls_md_free(&ctx);
	return ret;
}

/*
 * Verify RSASSA-PSS encoding as per RFC 8017 Section 9.1.2
 * This runs in software after CAAM performs RSA exponentiation.
 *
 * 'salt_len' is the expected salt length, or MBEDTLS_RSA_SALT_LEN_ANY (-1)
 * if any salt length is acceptable. In the latter case, the salt length is
 * not known upfront: it must be derived from the position of the 0x01
 * marker that separates PS (zero padding) from the salt within DB, found
 * by scanning DB from the left for the first non-zero byte (per RFC 8017
 * Section 9.1.2, step 10), rather than computed from a fixed formula that
 * assumes a known salt length.
 */
static int verify_pss_padding(const unsigned char *em, unsigned int em_len,
			      const unsigned char *msg_hash, size_t hash_len,
			      mbedtls_md_type_t md_alg, int salt_len)
{
	const mbedtls_md_info_t *md_info;
	mbedtls_md_context_t md_ctx;
	unsigned char *db = NULL;
	unsigned char h_prime[MBEDTLS_MD_MAX_SIZE];
	unsigned char m_prime_prefix[PSS_M_PRIME_PADDING_LEN] = {0};
	size_t db_len;
	size_t ps_len;
	size_t observed_salt_len;
	size_t i;
	int ret;

	md_info = mbedtls_md_info_from_type(md_alg);
	if (md_info == NULL)
		return CRYPTO_ERR_SIGNATURE;

	/*
	 * Step 3: Check EM length. When salt_len is not known upfront
	 * (MBEDTLS_RSA_SALT_LEN_ANY), the minimum valid EM length assumes a
	 * zero-length salt.
	 */
	if (salt_len == MBEDTLS_RSA_SALT_LEN_ANY) {
		if (em_len < hash_len + PSS_MIN_OVERHEAD)
			return CRYPTO_ERR_SIGNATURE;
	} else {
		if (salt_len < 0 ||
		    em_len < hash_len + (size_t)salt_len + PSS_MIN_OVERHEAD)
			return CRYPTO_ERR_SIGNATURE;
	}

	/* Step 4: Check rightmost byte is the trailer field (0xBC) */
	if (em[em_len - PSS_TRAILER_LEN] != PSS_TRAILER_FIELD)
		return CRYPTO_ERR_SIGNATURE;

	db_len = em_len - hash_len - PSS_TRAILER_LEN;

	/* Step 5: Extract maskedDB and H */
	db = mbedtls_calloc(1, db_len);
	if (!db)
		return CRYPTO_ERR_SIGNATURE;

	memcpy(db, em, db_len);
	/* H is at em + db_len, length hash_len */

	/* Step 6: Check leftmost bits are zero */
	if ((db[0] & PSS_DB_LEFTMOST_BIT_MASK) != 0U) {
		ret = CRYPTO_ERR_SIGNATURE;
		goto cleanup;
	}

	/* Step 7: Apply MGF1 to recover DB */
	ret = mgf1_xor_mask(em + db_len, hash_len, db, db_len, md_info);
	if (ret != 0) {
		ret = CRYPTO_ERR_SIGNATURE;
		goto cleanup;
	}

	/* Step 8: Clear leftmost bits */
	db[0] &= (unsigned char)~PSS_DB_LEFTMOST_BIT_MASK;

	/*
	 * Step 9/10: DB = PS || 0x01 || salt, where PS is a run of zero
	 * bytes. Find the 0x01 marker by scanning from the left, instead of
	 * assuming a fixed PS length, so that MBEDTLS_RSA_SALT_LEN_ANY is
	 * handled correctly.
	 */
	for (i = 0U; i < db_len && db[i] == 0U; i++)
		;
	if (i >= db_len || db[i] != PSS_DB_SEPARATOR) {
		ret = CRYPTO_ERR_SIGNATURE;
		goto cleanup;
	}
	ps_len = i;
	observed_salt_len = db_len - ps_len - PSS_DB_SEPARATOR_LEN;

	if (salt_len != MBEDTLS_RSA_SALT_LEN_ANY &&
	    observed_salt_len != (size_t)salt_len) {
		ret = CRYPTO_ERR_SIGNATURE;
		goto cleanup;
	}

	/* Step 10-12: Compute H' = Hash(M') where M' = (0x)00 00 00 00 00 00 00 00 || mHash || salt */
	mbedtls_md_init(&md_ctx);
	ret = mbedtls_md_setup(&md_ctx, md_info, 0);
	if (ret != 0) {
		mbedtls_md_free(&md_ctx);
		ret = CRYPTO_ERR_SIGNATURE;
		goto cleanup;
	}

	ret = mbedtls_md_starts(&md_ctx);
	if (ret != 0)
		goto free_md_ctx;

	/* 8 bytes of zeros */
	ret = mbedtls_md_update(&md_ctx, m_prime_prefix,
				sizeof(m_prime_prefix));
	if (ret != 0)
		goto free_md_ctx;

	/* mHash */
	ret = mbedtls_md_update(&md_ctx, msg_hash, hash_len);
	if (ret != 0)
		goto free_md_ctx;

	/* salt (located right after the separator) */
	ret = mbedtls_md_update(&md_ctx, db + ps_len + PSS_DB_SEPARATOR_LEN,
				observed_salt_len);
	if (ret != 0)
		goto free_md_ctx;

	ret = mbedtls_md_finish(&md_ctx, h_prime);

free_md_ctx:
	mbedtls_md_free(&md_ctx);
	if (ret != 0) {
		ret = CRYPTO_ERR_SIGNATURE;
		goto cleanup;
	}

	/* Step 13: Compare H and H' */
	ret = (memcmp(em + db_len, h_prime, hash_len) == 0) ?
	      CRYPTO_SUCCESS : CRYPTO_ERR_SIGNATURE;

cleanup:
	mbedtls_free(db);
	return ret;
}

/*
 * Perform RSA public key operation using CAAM hardware accelerator.
 * Key material (n || e) is located in the CAAM Secure Memory window
 * belonging to the Job Ring in use (derived from FCONF, see
 * caam_sm_rsa_key_addr()).
 *
 * Computes: output = input^e mod n
 */
static int caam_rsa_public_op(uint8_t *sign, uint8_t *to, uint8_t *rsa_pub_key,
			      unsigned int klen, uint8_t pub_exp_size)
{
	struct rsa_context ctx;
	struct job_descriptor jobdesc __aligned(CACHE_WRITEBACK_GRANULE);
	int ret = 0;

	jobdesc.callback = NULL;

	memset(&ctx, 0, sizeof(struct rsa_context));

	ctx.pkin.a = sign;
	ctx.pkin.a_siz = klen;
	ctx.pkin.n = rsa_pub_key;
	ctx.pkin.n_siz = klen;
	ctx.pkin.e = rsa_pub_key + klen;
	ctx.pkin.e_siz = pub_exp_size;

	cnstr_jobdesc_pkha_rsaexp(jobdesc.desc, &ctx.pkin, to, klen);

#if defined(SEC_MEM_NON_COHERENT) && defined(IMAGE_BL2)
	clean_dcache_range((uintptr_t)sign, klen);
	clean_dcache_range((uintptr_t)to, klen);
#endif

	ret = run_descriptor_jr(&jobdesc);
	if (ret != 0) {
		ERROR("Error in running descriptor\n");
		ret = -1;
	}
#if defined(SEC_MEM_NON_COHERENT) && defined(IMAGE_BL2)
	inv_dcache_range((uintptr_t)to, klen);
#endif
	return ret;
}

/*
 * Get the address (as seen by CAAM) of the RSA public key (n || e) pushed by
 * HSE2 into the CAAM Secure Memory window owned by the Job Ring in use (see
 * plat_img_parser.c's check_integrity()). Both the destination offset used
 * there and the one used here are derived from the same FCONF 'caam_config'
 * getter, so they are always in sync regardless of which CAAM instance/Job
 * Ring is enabled.
 */
static int caam_sm_rsa_key_addr(unsigned int mod_byte_len,
				uint8_t pub_exp_size, uint8_t **key_addr)
{
	size_t sm_offset = FCONF_GET_PROPERTY(hw_config, caam_config, sm_offset);
	size_t sm_size = FCONF_GET_PROPERTY(hw_config, caam_config, sm_size);

	if ((size_t)mod_byte_len + pub_exp_size > sm_size) {
		VERBOSE("%s: key (n || e) does not fit in the Secure Memory window (size %zu)\n",
			__func__, sm_size);
		return -EINVAL;
	}

	*key_addr = (uint8_t *)(CAAM_SM_BASE_ADDR + sm_offset);

	return 0;
}

static int verify_rsa_signature(void *data_ptr, unsigned int data_len,
				void *sig_ptr, unsigned int sig_len, void *pk_ptr,
				mbedtls_md_type_t md_alg, void *sig_opts)
{
	unsigned char hash[MBEDTLS_MD_MAX_SIZE] __aligned(CACHE_WRITEBACK_GRANULE) = {0};
	unsigned char sig_buf[MBEDTLS_MPI_MAX_SIZE] __aligned(CACHE_WRITEBACK_GRANULE) = {0};
	mbedtls_pk_rsassa_pss_options *pss_opts;
	const mbedtls_md_info_t *md_info;
	uint32_t md_size;
	uint8_t pub_exp_size;
	uint8_t *key_addr;
	unsigned char *p, *end;
	unsigned int mod_bit_len, mod_byte_len;
	size_t len;
	int ret;

	/* Get the signature (bitstring) */
	p = (unsigned char *)sig_ptr;
	end = (unsigned char *)(p + sig_len);
	ret = mbedtls_asn1_get_bitstring_null(&p, end, &len);
	if (ret != 0) {
		VERBOSE("%s: mbedtls_asn1_get_bitstring_null (%d)\n", __func__, ret);
		return CRYPTO_ERR_SIGNATURE;
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

	/* Query HSE2 for RSA key information */
	ret = hse_get_rsa_key_info(*(hseKeyHandle_t *)pk_ptr, &mod_bit_len, &pub_exp_size);
	if (ret != 0) {
		VERBOSE("%s: hse_get_rsa_key_info failed (%d)\n", __func__, ret);
		return CRYPTO_ERR_SIGNATURE;
	}

	/*
	 * The PSS encoding check below (see PSS_DB_LEFTMOST_BIT_MASK) assumes
	 * that the modulus size is a whole number of bytes, i.e. that
	 * emBits = modBits - 1 is 8 * emLen - 1.
	 */
	if ((mod_bit_len % CHAR_BIT) != 0U) {
		VERBOSE("%s: unsupported modulus size (%u bits)\n", __func__, mod_bit_len);
		return CRYPTO_ERR_SIGNATURE;
	}
	mod_byte_len = mod_bit_len / CHAR_BIT;

	/* Validate signature length matches modulus length */
	if (len != mod_byte_len) {
		VERBOSE("%s: signature length mismatch (sig=%zu, mod=%u)\n",
			__func__, len, mod_byte_len);
		return CRYPTO_ERR_SIGNATURE;
	}

	/*
	 * sig_buf is a fixed-size stack buffer; make sure the CAAM RSA
	 * public key operation (which writes mod_byte_len bytes to it)
	 * cannot overflow it.
	 */
	if (mod_byte_len > sizeof(sig_buf)) {
		VERBOSE("%s: modulus too large (%u bytes)\n", __func__, mod_byte_len);
		return CRYPTO_ERR_SIGNATURE;
	}

	ret = caam_sm_rsa_key_addr(mod_byte_len, pub_exp_size, &key_addr);
	if (ret != 0)
		return CRYPTO_ERR_SIGNATURE;

	ret = caam_rsa_public_op(p, sig_buf, key_addr, mod_byte_len, pub_exp_size);
	if (ret != 0) {
		VERBOSE("%s: caam_rsa_public_op failed (%d)\n", __func__, ret);
		return CRYPTO_ERR_SIGNATURE;
	}

	/* Verify PSS encoding in software */
	ret = verify_pss_padding(sig_buf, mod_byte_len, hash, md_size, md_alg,
				 pss_opts->expected_salt_len);
	if (ret != 0) {
		VERBOSE("%s: verify_pss_padding failed (%d)\n", __func__, ret);
		return CRYPTO_ERR_SIGNATURE;
	}

	return CRYPTO_SUCCESS;
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
