/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>

#include <drivers/nxp/s32n/s32n_xrdc_midac_data.h>
#include <lib/utils_def.h>

#define SMMS0_XRDC4_MIDAC_BASE			0x46370000U
#define SMMS1_XRDC4_MIDAC_BASE			0x5C370000U

enum xrdc_midac_instance {
	SMMS0_MIDAC,
	SMMS1_MIDAC,
};

struct xrdc_midac s32n_xrdc_midac_instances[] = {
	[SMMS0_MIDAC] = {
		.base = SMMS0_XRDC4_MIDAC_BASE, .name = "SMMS0", .mapped = false,
	},
	[SMMS1_MIDAC] = {
		.base = SMMS1_XRDC4_MIDAC_BASE, .name = "SMMS1", .mapped = false,
	},
};

const uint32_t s32n_xrdc_midac_n_instances = ARRAY_SIZE(s32n_xrdc_midac_instances);
