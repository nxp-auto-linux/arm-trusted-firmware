/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32N_PLAT_FUNCS_H
#define S32N_PLAT_FUNCS_H

#include <stdbool.h>

void s32n_plat_gic_save(bool system_suspend);
void s32n_plat_gic_restore(bool system_suspend);

#endif
