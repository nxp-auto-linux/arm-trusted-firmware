/*
 * Copyright 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32CC_LOWLEVEL_H
#define S32CC_LOWLEVEL_H

/* Secondary cores entry point */
void plat_secondary_cold_boot_setup(void);

unsigned int s32_core_pos_by_mpidr(u_register_t mpidr);

#endif /* S32CC_LOWLEVEL_H */

