/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pdef_tbb_cert.h>
#include <pdef_tbb_ext.h>
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
	}
};

PLAT_REGISTER_COT(pdef_tbb_certs);
