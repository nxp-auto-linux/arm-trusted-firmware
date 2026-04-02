/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <string.h>

#include <common/debug.h>
#include <s32_hse_drv.h>
#include <hse_interface.h>

#include <auth/s32_img_parser.h>
#include <s32n_caam.h>
#include <s32n_fconf_caam_getter.h>

int plat_img_parser_push_key(hseKeyHandle_t key_handle,
			     const hseKeyInfo_t *key_info)
{
	hseSrvDescriptor_t srv_desc;
	unsigned int caam_instance;
	uint32_t sm_offset, sm_size, key_len;

	caam_instance = FCONF_GET_PROPERTY(hw_config, caam_config, instance);
	sm_offset = FCONF_GET_PROPERTY(hw_config, caam_config, sm_offset);
	sm_size = FCONF_GET_PROPERTY(hw_config, caam_config, sm_size);

	/* Total length (in bytes) of the key material being pushed */
	if (key_info->keyType == HSE_KEY_TYPE_RSA_PUB) {
		key_len = HSE_BITS_TO_BYTES(key_info->keyBitLen) +
			  key_info->specific.pubExponentSize;
	} else {
		/* HSE_KEY_TYPE_ECC_PUB: raw X || Y coordinates */
		key_len = 2U * HSE_BITS_TO_BYTES(key_info->keyBitLen);
	}

	/* Per HSE2 MSC Keys Push service: both must be 4-byte aligned */
	if ((sm_offset % 4U) != 0U || (key_len % 4U) != 0U ||
	    key_len > sm_size) {
		ERROR("S32N79: invalid CAAM Secure Memory window (offset 0x%x, size 0x%x) for key length %u\n",
		      sm_offset, sm_size, key_len);
		return -EINVAL;
	}

	memset(&srv_desc, 0, sizeof(srv_desc));
	srv_desc.srvId = HSE_SRV_ID_MSC_KEYS_PUSH;
	srv_desc.hseSrv.mscPushKeys.mscKeyPushEntry.hseKeyHandle = key_handle;
	srv_desc.hseSrv.mscPushKeys.mscKeyPushEntry.mscId = s32n_caam_msc_id(caam_instance);
	srv_desc.hseSrv.mscPushKeys.mscKeyPushEntry.mscKeyPolices = HSE_MSC_KEY_POLICY_DENY_BULK_PUSH;
	srv_desc.hseSrv.mscPushKeys.mscKeyPushEntry.mscDestRef.mscCaamDestRef.mscCaamDestOffset = sm_offset;

	return hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
}
