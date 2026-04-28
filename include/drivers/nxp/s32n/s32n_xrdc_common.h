/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32N_XRDC_COMMON_H
#define S32N_XRDC_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * s32n_xrdc_mmap_instance - Idempotent MMU mapping for an XRDC peripheral.
 * @base:   physical/virtual base address (identity mapping is used).
 * @size:   region size to map (rounded up to a page by the xlat lib).
 * @mapped: pointer to the per-instance bookkeeping flag; the helper reads
 *          it to short-circuit re-maps and sets it to true after success.
 * @name:   short identifier used as the "XRDC-<name>" log prefix
 *          (e.g. "AIDA", "SMMS0", "SMMS1").
 *
 * Wraps mmap_add_dynamic_region() with the MT_DEVICE | MT_RW | MT_SECURE
 * attributes used by every XRDC family peripheral on S32N.
 *
 * Return: 0 on success or if already mapped, negative errno on failure.
 */
int s32n_xrdc_mmap_instance(uintptr_t base, size_t size, bool *mapped,
			    const char *name);

#endif /* S32N_XRDC_COMMON_H */
