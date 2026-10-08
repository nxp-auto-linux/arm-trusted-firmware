/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_COHORT_H
#define S32N_COHORT_H

#include <stdint.h>

/**
 * s32n_get_cohort_id - Get the cohort identifier of the running APS
 * @cohort_id: on success, receives the cohort identifier
 *
 * Return: 0 on success, negative errno otherwise.
 */
int s32n_get_cohort_id(uint32_t *cohort_id);

#endif /* S32N_COHORT_H */
