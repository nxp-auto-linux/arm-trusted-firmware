/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32N_COHORT_MANAGER_H
#define S32N_COHORT_MANAGER_H

#include <stdint.h>

/**
 * s32n_cohort_manager_init - Program the XRDC blocks owned by a cohort
 * @cohort_id: caller's cohort identifier (e.g., from XRDC4_CCID)
 *
 * Programs the XRDC AIDA (StreamID/DAUX for SMMU-capable masters) and the
 * XRDC MIDAC (per-cohort DDR access narrowed to DT /memory). Idempotent, so
 * it can be called again to re-apply the programming.
 *
 * Returns 0 immediately when S32N_COHORT_MANAGER_SUPPORT == 0.
 *
 * Return: 0 on success, negative errno on failure.
 */
int s32n_cohort_manager_init(uint32_t cohort_id);

#endif /* S32N_COHORT_MANAGER_H */
