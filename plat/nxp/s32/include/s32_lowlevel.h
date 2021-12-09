/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32_LOWLEVEL_H
#define S32_LOWLEVEL_H

void s32_smp_fixup(void);
/* Secondary cores entry point */
void plat_secondary_cold_boot_setup(void);

#endif /* S32_LOWLEVEL_H */

