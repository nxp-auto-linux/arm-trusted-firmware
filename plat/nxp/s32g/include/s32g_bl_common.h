/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_BL_COMMON_H
#define S32G_BL_COMMON_H

#include <stdbool.h>
#include <stdint.h>

void s32g_gic_setup(void);
void plat_gic_save(void);
void plat_gic_restore(void);
void s32g_early_plat_init(bool skip_ddr_clk);
int pmic_prepare_for_suspend(void);
void pmic_system_off(void);

void update_core_state(uint32_t core, uint32_t state);
bool is_last_core(void);
bool is_cluster0_off(void);
bool is_cluster1_off(void);

#endif
