/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32N_XRDC_MIDAC_H
#define S32N_XRDC_MIDAC_H

#include <stdint.h>

/**
 * s32n_xrdc_midac_init - Act as Cohort Manager on SMMS0/SMMS1 MIDAC instances
 * @cohort_id: The caller's cohort identifier, already obtained from the PM
 *
 * Assumes the partitioning manager (FSS firmware) has already configured
 * cohort regions and slot-to-cohort assignments. This driver narrows each
 * PM-granted slot to the corresponding DT /memory window for @cohort_id,
 * and invalidates any remaining owned slots. Slots belonging to the
 * partitioning manager (OCID=0) are left alone.
 *
 * Return: 0 on success, negative errno on failure.
 */
int s32n_xrdc_midac_init(uint32_t cohort_id);

#endif /* S32N_XRDC_MIDAC_H */
