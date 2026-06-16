/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_XRDC_MIDAC_DATA_H
#define S32N_XRDC_MIDAC_DATA_H

#include <stdbool.h>
#include <stdint.h>

struct xrdc_midac {
	uintptr_t base;
	const char *name;
	bool mapped;
};

/*
 * SMMS instance table (MMIO bases), provided by the platform and selected by
 * the build. The driver logic is agnostic to the number of instances.
 */
extern struct xrdc_midac s32n_xrdc_midac_instances[];
extern const uint32_t s32n_xrdc_midac_n_instances;

#endif /* S32N_XRDC_MIDAC_DATA_H */
