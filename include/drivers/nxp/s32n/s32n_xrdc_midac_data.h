/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_XRDC_MIDAC_DATA_H
#define S32N_XRDC_MIDAC_DATA_H

#include <stdbool.h>
#include <stdint.h>

/* Inclusive [lo..hi] slave-side DDR window an SMMS instance physically gates. */
struct xrdc_midac_window {
	uint64_t lo;
	uint64_t hi;
};

struct xrdc_midac {
	uintptr_t base;
	const char *name;
	bool mapped;
	const struct xrdc_midac_window *windows;
	uint32_t n_windows;
};

/*
 * SMMS instance table (MMIO bases and the DDR windows each governs), provided
 * by the platform and selected by the build. The driver logic is agnostic to
 * the number of instances.
 */
extern struct xrdc_midac s32n_xrdc_midac_instances[];
extern const uint32_t s32n_xrdc_midac_n_instances;

#endif /* S32N_XRDC_MIDAC_DATA_H */
