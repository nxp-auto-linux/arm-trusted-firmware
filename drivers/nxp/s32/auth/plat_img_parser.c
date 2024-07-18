/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/auth/img_parser_mod.h>
#include <drivers/nxp/s32/hse/hse_core.h>
#include <drivers/nxp/s32/hse/hse_mem.h>
#include <hse_interface.h>

#define LIB_NAME	"HSE Key Handles"

static void init(void)
{
	int ret;

	ret = hse_driver_init();
	if (ret && is_secboot_active()) {
		ERROR("S32 Image Parser Library initialization failed\n");
		panic();
	}
}

static int check_integrity(void *img, unsigned int img_len)
{
	hseKeyHandle_t key_handle = *(hseKeyHandle_t *)img;
	hseSrvDescriptor_t srv_desc;
	void *key_info_buf = NULL;
	hseKeyInfo_t key_info;
	int ret;

	key_info_buf = hse_mem_alloc(sizeof(hseKeyInfo_t));
	if (!key_info_buf)
		return IMG_PARSER_ERR;

	srv_desc.srvId = HSE_SRV_ID_GET_KEY_INFO;
	srv_desc.hseSrv.getKeyInfoReq.keyHandle = key_handle;
	srv_desc.hseSrv.getKeyInfoReq.pKeyInfo = hse_virt_to_phys(key_info_buf);

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret) {
		ret = IMG_PARSER_ERR;
		goto out;
	}

	hse_memcpy(&key_info, key_info_buf, sizeof(key_info));

	if (key_info.keyType != HSE_KEY_TYPE_RSA_PUB) {
		ret = IMG_PARSER_ERR_FORMAT;
		goto out;
	}

	if (!(key_info.keyFlags & HSE_KF_USAGE_VERIFY)) {
		ret = IMG_PARSER_ERR_FORMAT;
		goto out;
	}

	ret = IMG_PARSER_OK;

out:
	hse_mem_free(key_info_buf);

	return ret;
}

static int get_auth_param(const auth_param_type_desc_t *type_desc, void *img,
		   unsigned int img_len, void **param, unsigned int *param_len)
{
	switch (type_desc->type) {

	case AUTH_PARAM_PUB_KEY:
		*param = img;
		*param_len = sizeof(hseKeyHandle_t);
		break;

	default:
		return IMG_PARSER_ERR_NOT_FOUND;
	}

	return IMG_PARSER_OK;
}

REGISTER_IMG_PARSER_LIB(IMG_PLAT, LIB_NAME, init,
			check_integrity, get_auth_param);
