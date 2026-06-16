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

/*
 * Each SMMS XRDC governs a fixed slave-side DDR window. A DT /memory range is
 * matched against these windows so an instance only handles the bytes it
 * physically covers. The windows are disjoint; low DDR is split between the two
 * instances at 0xC0000000.
 */
#define LOW_DDR_SMMS0_START			0x80000000ULL
#define LOW_DDR_SMMS0_END			0xBFFFFFFFULL    /* 1 GB window */
#define LOW_DDR_SMMS1_START			0xC0000000ULL
#define LOW_DDR_SMMS1_END			0xFFFFFFFFULL    /* 1 GB window */

#define HIGH_DDR_SMMS0_START			0x8800000000ULL
#define HIGH_DDR_SMMS0_END			0x8BFFFFFFFFULL  /* 16 GB window */
#define HIGH_DDR_SMMS1_START			0xC000000000ULL
#define HIGH_DDR_SMMS1_END			0xC3FFFFFFFFULL  /* 16 GB window */

static const struct xrdc_midac_window smms0_windows[] = {
	{ .lo = LOW_DDR_SMMS0_START,  .hi = LOW_DDR_SMMS0_END  },
	{ .lo = HIGH_DDR_SMMS0_START, .hi = HIGH_DDR_SMMS0_END },
};

static const struct xrdc_midac_window smms1_windows[] = {
	{ .lo = LOW_DDR_SMMS1_START,  .hi = LOW_DDR_SMMS1_END  },
	{ .lo = HIGH_DDR_SMMS1_START, .hi = HIGH_DDR_SMMS1_END },
};

enum xrdc_midac_instance {
	SMMS0_MIDAC,
	SMMS1_MIDAC,
};

struct xrdc_midac s32n_xrdc_midac_instances[] = {
	[SMMS0_MIDAC] = {
		.base = SMMS0_XRDC4_MIDAC_BASE, .name = "SMMS0", .mapped = false,
		.windows = smms0_windows, .n_windows = ARRAY_SIZE(smms0_windows),
	},
	[SMMS1_MIDAC] = {
		.base = SMMS1_XRDC4_MIDAC_BASE, .name = "SMMS1", .mapped = false,
		.windows = smms1_windows, .n_windows = ARRAY_SIZE(smms1_windows),
	},
};

const uint32_t s32n_xrdc_midac_n_instances = ARRAY_SIZE(s32n_xrdc_midac_instances);
