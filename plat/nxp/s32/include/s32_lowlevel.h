/*
 * Copyright 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32_LOWLEVEL_H
#define S32_LOWLEVEL_H

void s32_smp_fixup(void);
/* Secondary cores entry point */
void plat_secondary_cold_boot_setup(void);

unsigned int s32_core_pos_by_mpidr(u_register_t mpidr);

#endif /* S32_LOWLEVEL_H */

