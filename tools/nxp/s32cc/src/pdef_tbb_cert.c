/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pdef_tbb_cert.h>
#include <pdef_tbb_ext.h>
#include <pdef_tbb_key.h>
#include <tbbr/tbb_key.h>

static cert_t pdef_tbb_certs[] = {
	[CUSTOM_TRUSTED_OS_FW_CONTENT_CERT - CUSTOM_TRUSTED_OS_FW_CONTENT_CERT] = {
		.id = TRUSTED_OS_FW_CONTENT_CERT,
		.opt = "ctos-fw-cert",
		.help_msg = "Custom Trusted OS Firmware Content Certificate (output file)",
		.fn = NULL,
		.cn = "Custom Trusted OS Firmware Content Certificate",
		.key = TRUSTED_OS_FW_CONTENT_CERT_KEY,
		.issuer = TRUSTED_OS_FW_CONTENT_CERT,
		.ext = {
			TRUSTED_OS_NVCOUNTER_EXT,
			TRUSTED_OS_FW_HASH_EXT,
			TRUSTED_OS_FW_EXTRA1_HASH_EXT,
		},
		.num_ext = 3
	},
#if defined(HAS_DDR_FW) && !defined(CUSTOM_DDR_DRV)
	[DDR_FW_CONTENT_CERT - CUSTOM_TRUSTED_OS_FW_CONTENT_CERT] = {
		.id = DDR_FW_CONTENT_CERT,
		.opt = "ddr-fw-cert",
		.help_msg = "DDR Firmware Content Certificate (output file)",
		.fn = NULL,
		.cn = "DDR Firmware Content Certificate",
		.key = DDR_FW_CONTENT_KEY,
		.issuer = DDR_FW_CONTENT_CERT,
		.ext = {
			DDR_FW_HASH_EXT,
			DDR_FW_NVCOUNTER_EXT,
		},
		.num_ext = 2
	},
	[DDR_FW_KEY_CERT - CUSTOM_TRUSTED_OS_FW_CONTENT_CERT] = {
		.id = DDR_FW_KEY_CERT,
		.opt = "ddr-fw-key-cert",
		.help_msg = "DDR Firmware Key Certificate (output file)",
		.fn = NULL,
		.cn = "DDR Firmware Key Certificate",
		.key = TRUSTED_WORLD_KEY,
		.issuer = DDR_FW_KEY_CERT,
		.ext = {
			DDR_FW_NVCOUNTER_EXT,
			DDR_FW_CONTENT_CERT_PK_EXT,
		},
		.num_ext = 2
	},
#endif
};

PLAT_REGISTER_COT(pdef_tbb_certs);
