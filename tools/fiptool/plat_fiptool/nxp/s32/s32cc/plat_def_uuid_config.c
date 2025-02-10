/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>

#include <firmware_image_package.h>

#include "tbbr_config.h"

toc_entry_t plat_def_toc_entries[] = {
	/* DDR PHY firmware */
	{
		.name = "DDR FW",
		.uuid = UUID_DDR_FW,
		.cmdline_name = "ddr-fw"
	},
	/* Content certificates */
	{
		.name = "Custom Trusted OS Firmware content certificate",
		.uuid = UUID_CUSTOM_TRUSTED_OS_FW_CONTENT_CERT,
		.cmdline_name = "ctos-fw-cert"
	},
	{
		.name = NULL,
		.uuid = { {0} },
		.cmdline_name = NULL,
	}
};
