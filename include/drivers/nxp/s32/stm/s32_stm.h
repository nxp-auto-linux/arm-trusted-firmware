/*
 * Copyright 2023 NXP
 *
 * S32 System Timer Module
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32_STM_H
#define S32_STM_H

#include <stdbool.h>
#include <stdint.h>

struct s32_stm {
	uintptr_t base;
};

int s32_stm_init(struct s32_stm *driver);
bool s32_stm_is_enabled(struct s32_stm *stm);
void s32_stm_enable(struct s32_stm *stm, bool enable);
uint32_t s32_stm_get_count(struct s32_stm *stm);

#endif /* S32_STM_H */
