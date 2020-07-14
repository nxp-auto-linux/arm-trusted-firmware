/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32G_LOWLEVEL_H
#define S32G_LOWLEVEL_H

#include <stdbool.h>
#include <stdint.h>

int plat_core_pos_by_mpidr(u_register_t mpidr);
int plat_is_my_cpu_primary(void);
void s32g_smp_fixup(void);
void s32g_gic_setup(void);
void s32g_early_plat_init(bool skip_ddr_clk);

#endif /* S32G_LOWLEVEL_H */
