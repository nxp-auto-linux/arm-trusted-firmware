/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32N_COHORT_MANAGER_H
#define S32N_COHORT_MANAGER_H

/**
 * s32n_cohort_manager_init - BL2-side Cohort Manager bring-up.
 *
 * Bundles the BL2-as-Cohort-Manager responsibilities into a single entry
 * point. Currently programs XRDC AIDA (StreamID/DAUX for SMMU-capable
 * masters); future cohort-scoped initialization (e.g., XRDC MIDAC) is
 * added here without touching the call site.
 *
 * AIDA is not strictly a Cohort Manager construct per the MIDAC RM, but
 * on S32N it is owned by the same software entity (BL2 on APP cores) and
 * is therefore included in this bundle for convenience and locality.
 *
 * When S32N_BL2_AS_COHORT_MANAGER == 0, BL2 does not act as the Cohort
 * Manager and this function returns 0 immediately so the call site does
 * not need build-time guards.
 *
 * Return: 0 on success, negative errno on failure.
 */
int s32n_cohort_manager_init(void);

#endif /* S32N_COHORT_MANAGER_H */
