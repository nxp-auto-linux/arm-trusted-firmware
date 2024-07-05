/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_BL_COMMON_H
#define S32N_BL_COMMON_H

#include <stdbool.h>
#include <stdint.h>

void s32n_el3_mmu_init(void);

/* callback to check a condition */
typedef bool (*cond_cb_t)(void *arg);

/**
 * Returns -EINVAL if the condition callback is not set, 0 if the condition is met,
 * -ETIMEDOUT if the timer expired.
 */
int wait_for_condition_timeout(cond_cb_t cond_cb, void *arg, uint32_t timeout_us);

#endif
