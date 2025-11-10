/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>

#include <common/tbbr/cot_def.h>
#include <drivers/auth/auth_mod.h>

#if USE_TBBR_DEFS
#include <tools_share/tbbr_oid.h>
#else
#include <platform_oid.h>
#endif

#include <platform_def.h>

#define HSE_KEY_HANDLE_SIZE	4

static unsigned char soc_fw_hash_buf[HASH_DER_LEN];
static unsigned char tos_fw_hash_buf[HASH_DER_LEN];
static unsigned char tos_fw_extra1_hash_buf[HASH_DER_LEN];
#if defined(USE_NT_FW_CONFIG)
static unsigned char nt_world_dt_hash_buf[HASH_DER_LEN];
#endif
static unsigned char nt_world_bl_hash_buf[HASH_DER_LEN];
#if defined(HAS_DDR_FW) && !defined(CUSTOM_DDR_DRV)
static unsigned char ddr_fw_hash_buf[HASH_DER_LEN];
#endif
static unsigned char content_pk_buf[PK_DER_LEN];

auth_param_type_desc_t subject_pk = AUTH_PARAM_TYPE_DESC(
	AUTH_PARAM_PUB_KEY, 0);
auth_param_type_desc_t sig = AUTH_PARAM_TYPE_DESC(
	AUTH_PARAM_SIG, 0);
auth_param_type_desc_t sig_alg = AUTH_PARAM_TYPE_DESC(
	AUTH_PARAM_SIG_ALG, 0);
auth_param_type_desc_t raw_data = AUTH_PARAM_TYPE_DESC(
	AUTH_PARAM_RAW_DATA, 0);

static auth_param_type_desc_t trusted_nv_ctr = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_NV_CTR, TRUSTED_FW_NVCOUNTER_OID);
static auth_param_type_desc_t tos_nv_ctr = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_NV_CTR, TRUSTED_OS_NVCOUNTER_OID);
static auth_param_type_desc_t non_trusted_nv_ctr = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_NV_CTR, NON_TRUSTED_FW_NVCOUNTER_OID);
#if defined(HAS_DDR_FW) && !defined(CUSTOM_DDR_DRV)
static auth_param_type_desc_t ddr_nv_ctr = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_NV_CTR, DDR_FW_NVCOUNTER_OID);
#endif
static auth_param_type_desc_t soc_fw_content_pk = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_PUB_KEY, NULL);
static auth_param_type_desc_t tos_fw_content_pk = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_PUB_KEY, NULL);
static auth_param_type_desc_t nt_fw_content_pk = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_PUB_KEY, NULL);
#if defined(HAS_DDR_FW) && !defined(CUSTOM_DDR_DRV)
static auth_param_type_desc_t ddr_fw_content_pk = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_PUB_KEY, NULL);
#endif
static auth_param_type_desc_t soc_fw_hash = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_HASH, SOC_AP_FW_HASH_OID);
static auth_param_type_desc_t tos_fw_hash = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_HASH, TRUSTED_OS_FW_HASH_OID);
static auth_param_type_desc_t tos_fw_extra1_hash = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_HASH, TRUSTED_OS_FW_EXTRA1_HASH_OID);
#if defined(USE_NT_FW_CONFIG)
static auth_param_type_desc_t nt_world_dt_hash = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_HASH, NON_TRUSTED_FW_CONFIG_HASH_OID);
#endif
static auth_param_type_desc_t nt_world_bl_hash = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_HASH, NON_TRUSTED_WORLD_BOOTLOADER_HASH_OID);
#if defined(HAS_DDR_FW) && !defined(CUSTOM_DDR_DRV)
static auth_param_type_desc_t ddr_fw_hash = AUTH_PARAM_TYPE_DESC(
		AUTH_PARAM_HASH, DDR_FW_HASH_OID);
#endif

/*
 * SoC Firmware
 */
static const auth_img_desc_t soc_fw_key_cert = {
	.img_id = SOC_FW_KEY_CERT_ID,
	.img_type = IMG_PLAT,
	.parent = NULL,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_NONE,
		}
	},
	.authenticated_data = (const auth_param_desc_t[COT_MAX_VERIFIED_PARAMS]) {
		[0] = {
			.type_desc = &soc_fw_content_pk,
			.data = {
				.ptr = (void *)content_pk_buf,
				.len = (unsigned int)HSE_KEY_HANDLE_SIZE
			}
		}
	}
};

static const auth_img_desc_t soc_fw_content_cert = {
	.img_id = SOC_FW_CONTENT_CERT_ID,
	.img_type = IMG_CERT,
	.parent = &soc_fw_key_cert,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_SIG,
			.param.sig = {
				.pk = &soc_fw_content_pk,
				.sig = &sig,
				.alg = &sig_alg,
				.data = &raw_data
			}
		},
		[1] = {
			.type = AUTH_METHOD_NV_CTR,
			.param.nv_ctr = {
				.cert_nv_ctr = &trusted_nv_ctr,
				.plat_nv_ctr = &trusted_nv_ctr
			}
		}
	},
	.authenticated_data = (const auth_param_desc_t[COT_MAX_VERIFIED_PARAMS]) {
		[0] = {
			.type_desc = &soc_fw_hash,
			.data = {
				.ptr = (void *)soc_fw_hash_buf,
				.len = (unsigned int)HASH_DER_LEN
			}
		},
	}
};

static const auth_img_desc_t bl31_image = {
	.img_id = BL31_IMAGE_ID,
	.img_type = IMG_RAW,
	.parent = &soc_fw_content_cert,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_HASH,
			.param.hash = {
				.data = &raw_data,
				.hash = &soc_fw_hash
			}
		}
	}
};

/*
 * Trusted OS Firmware
 */
static const auth_img_desc_t trusted_os_fw_key_cert = {
	.img_id = TRUSTED_OS_FW_KEY_CERT_ID,
	.img_type = IMG_PLAT,
	.parent = NULL,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_NONE,
		}
	},
	.authenticated_data = (const auth_param_desc_t[COT_MAX_VERIFIED_PARAMS]) {
		[0] = {
			.type_desc = &tos_fw_content_pk,
			.data = {
				.ptr = (void *)content_pk_buf,
				.len = (unsigned int)HSE_KEY_HANDLE_SIZE
			}
		}
	}
};
static const auth_img_desc_t trusted_os_fw_content_cert = {
	.img_id = TRUSTED_OS_FW_CONTENT_CERT_ID,
	.img_type = IMG_CERT,
	.parent = &trusted_os_fw_key_cert,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_SIG,
			.param.sig = {
				.pk = &tos_fw_content_pk,
				.sig = &sig,
				.alg = &sig_alg,
				.data = &raw_data
			}
		},
		[1] = {
			.type = AUTH_METHOD_NV_CTR,
			.param.nv_ctr = {
				.cert_nv_ctr = &tos_nv_ctr,
				.plat_nv_ctr = &tos_nv_ctr
			}
		}
	},
	.authenticated_data = (const auth_param_desc_t[COT_MAX_VERIFIED_PARAMS]) {
		[0] = {
			.type_desc = &tos_fw_hash,
			.data = {
				.ptr = (void *)tos_fw_hash_buf,
				.len = (unsigned int)HASH_DER_LEN
			}
		},
		[1] = {
			.type_desc = &tos_fw_extra1_hash,
			.data = {
				.ptr = (void *)tos_fw_extra1_hash_buf,
				.len = (unsigned int)HASH_DER_LEN
			}
		},
	}
};
static const auth_img_desc_t bl32_image = {
	.img_id = BL32_IMAGE_ID,
	.img_type = IMG_RAW,
	.parent = &trusted_os_fw_content_cert,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_HASH,
			.param.hash = {
				.data = &raw_data,
				.hash = &tos_fw_hash
			}
		}
	}
};
static const auth_img_desc_t bl32_extra1_image = {
	.img_id = BL32_EXTRA1_IMAGE_ID,
	.img_type = IMG_RAW,
	.parent = &trusted_os_fw_content_cert,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_HASH,
			.param.hash = {
				.data = &raw_data,
				.hash = &tos_fw_extra1_hash
			}
		}
	}
};

/*
 * Non-Trusted Firmware
 */
static const auth_img_desc_t non_trusted_fw_key_cert = {
	.img_id = NON_TRUSTED_FW_KEY_CERT_ID,
	.img_type = IMG_PLAT,
	.parent = NULL,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_NONE
		}
	},
	.authenticated_data = (const auth_param_desc_t[COT_MAX_VERIFIED_PARAMS]) {
		[0] = {
			.type_desc = &nt_fw_content_pk,
			.data = {
				.ptr = (void *)content_pk_buf,
				.len = (unsigned int)HSE_KEY_HANDLE_SIZE
			}
		}
	}
};
static const auth_img_desc_t non_trusted_fw_content_cert = {
	.img_id = NON_TRUSTED_FW_CONTENT_CERT_ID,
	.img_type = IMG_CERT,
	.parent = &non_trusted_fw_key_cert,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_SIG,
			.param.sig = {
				.pk = &nt_fw_content_pk,
				.sig = &sig,
				.alg = &sig_alg,
				.data = &raw_data
			}
		},
		[1] = {
			.type = AUTH_METHOD_NV_CTR,
			.param.nv_ctr = {
				.cert_nv_ctr = &non_trusted_nv_ctr,
				.plat_nv_ctr = &non_trusted_nv_ctr
			}
		}
	},
	.authenticated_data = (const auth_param_desc_t[COT_MAX_VERIFIED_PARAMS]) {
		[0] = {
			.type_desc = &nt_world_bl_hash,
			.data = {
				.ptr = (void *)nt_world_bl_hash_buf,
				.len = (unsigned int)HASH_DER_LEN
			}
		},
#if defined(USE_NT_FW_CONFIG)
		[1] = {
			.type_desc = &nt_world_dt_hash,
			.data = {
				.ptr = (void *)nt_world_dt_hash_buf,
				.len = (unsigned int)HASH_DER_LEN
			}
		},
#endif
	}
};
#if defined(USE_NT_FW_CONFIG)
static const auth_img_desc_t nt_fw_config = {
	.img_id = NT_FW_CONFIG_ID,
	.img_type = IMG_RAW,
	.parent = &non_trusted_fw_content_cert,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_HASH,
			.param.hash = {
				.data = &raw_data,
				.hash = &nt_world_dt_hash
			}
		}
	}
};
#endif
static const auth_img_desc_t bl33_image = {
	.img_id = BL33_IMAGE_ID,
	.img_type = IMG_RAW,
	.parent = &non_trusted_fw_content_cert,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_HASH,
			.param.hash = {
				.data = &raw_data,
				.hash = &nt_world_bl_hash
			}
		}
	}
};

#if defined(HAS_DDR_FW) && !defined(CUSTOM_DDR_DRV)
/*
 * DDR Firmware
 */
static const auth_img_desc_t ddr_fw_key_cert = {
	.img_id = DDR_FW_KEY_CERT_ID,
	.img_type = IMG_PLAT,
	.parent = NULL,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_NONE
		}
	},
	.authenticated_data = (const auth_param_desc_t[COT_MAX_VERIFIED_PARAMS]) {
		[0] = {
			.type_desc = &ddr_fw_content_pk,
			.data = {
				.ptr = (void *)content_pk_buf,
				.len = (unsigned int)HSE_KEY_HANDLE_SIZE
			}
		}
	}
};
static const auth_img_desc_t ddr_fw_content_cert = {
	.img_id = DDR_FW_CONTENT_CERT_ID,
	.img_type = IMG_CERT,
	.parent = &ddr_fw_key_cert,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_SIG,
			.param.sig = {
				.pk = &ddr_fw_content_pk,
				.sig = &sig,
				.alg = &sig_alg,
				.data = &raw_data
			}
		},
		[1] = {
			.type = AUTH_METHOD_NV_CTR,
			.param.nv_ctr = {
				.cert_nv_ctr = &ddr_nv_ctr,
				.plat_nv_ctr = &ddr_nv_ctr
			}
		}
	},
	.authenticated_data = (const auth_param_desc_t[COT_MAX_VERIFIED_PARAMS]) {
		[0] = {
			.type_desc = &ddr_fw_hash,
			.data = {
				.ptr = (void *)ddr_fw_hash_buf,
				.len = (unsigned int)HASH_DER_LEN
			}
		},
	}
};
static const auth_img_desc_t ddr_fw_image = {
	.img_id = DDR_FW_IMAGE_ID,
	.img_type = IMG_RAW,
	.parent = &ddr_fw_content_cert,
	.img_auth_methods = (const auth_method_desc_t[AUTH_METHOD_NUM]) {
		[0] = {
			.type = AUTH_METHOD_HASH,
			.param.hash = {
				.data = &raw_data,
				.hash = &ddr_fw_hash
			}
		}
	}
};
#endif

static const auth_img_desc_t * const cot_desc[] = {
	[SOC_FW_KEY_CERT_ID]			=	&soc_fw_key_cert,
	[SOC_FW_CONTENT_CERT_ID]		=	&soc_fw_content_cert,
	[BL31_IMAGE_ID]				=	&bl31_image,
	[TRUSTED_OS_FW_KEY_CERT_ID]		=	&trusted_os_fw_key_cert,
	[TRUSTED_OS_FW_CONTENT_CERT_ID]		=	&trusted_os_fw_content_cert,
	[BL32_IMAGE_ID]				=	&bl32_image,
	[BL32_EXTRA1_IMAGE_ID]			=	&bl32_extra1_image,
	[NON_TRUSTED_FW_KEY_CERT_ID]		=	&non_trusted_fw_key_cert,
	[NON_TRUSTED_FW_CONTENT_CERT_ID]	=	&non_trusted_fw_content_cert,
#if defined(USE_NT_FW_CONFIG)
	[NT_FW_CONFIG_ID]			=	&nt_fw_config,
#endif
	[BL33_IMAGE_ID]				=	&bl33_image,
#if defined(HAS_DDR_FW) && !defined(CUSTOM_DDR_DRV)
	[DDR_FW_KEY_CERT_ID]			=	&ddr_fw_key_cert,
	[DDR_FW_CONTENT_CERT_ID]		=	&ddr_fw_content_cert,
	[DDR_FW_IMAGE_ID]			=	&ddr_fw_image,
#endif
};

REGISTER_COT(cot_desc);
