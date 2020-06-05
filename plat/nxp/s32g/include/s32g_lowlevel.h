/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32G_LOWLEVEL_H
#define S32G_LOWLEVEL_H

int plat_core_pos_by_mpidr(u_register_t mpidr);
int plat_is_my_cpu_primary(void);
void s32g_smp_fixup(void);
void s32g_gic_setup(void);

#endif /* S32G_LOWLEVEL_H */
