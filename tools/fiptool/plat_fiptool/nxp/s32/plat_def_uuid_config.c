/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>

#include <firmware_image_package.h>

#include "tbbr_config.h"

toc_entry_t plat_def_toc_entries[] = {
#if defined(HAS_DDR_FW) && !defined(CUSTOM_DDR_DRV)
	/* DDR PHY firmware */
	{
		.name = "DDR FW",
		.uuid = UUID_DDR_FW,
		.cmdline_name = "ddr-fw"
	},
#endif
	/* Content certificates */
	{
		.name = "Custom Trusted OS Firmware content certificate",
		.uuid = UUID_CUSTOM_TRUSTED_OS_FW_CONTENT_CERT,
		.cmdline_name = "ctos-fw-cert"
	},
#if defined(HAS_DDR_FW) && !defined(CUSTOM_DDR_DRV)
	{
		.name = "DDR Firmware content certificate",
		.uuid = UUID_DDR_FW_CONTENT_CERT,
		.cmdline_name = "ddr-fw-cert"
	},
	/* Key Certificates */
	{
		.name = "DDR Firmware key certificate",
		.uuid = UUID_DDR_FW_KEY_CERT,
		.cmdline_name = "ddr-fw-key-cert"
	},
#endif
	{
		.name = NULL,
		.uuid = { {0} },
		.cmdline_name = NULL,
	}
};
