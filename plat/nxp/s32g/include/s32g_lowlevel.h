/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32G_LOWLEVEL_H
#define S32G_LOWLEVEL_H

#include <stdbool.h>
#include <stdint.h>

int plat_is_my_cpu_primary(void);
void s32g_smp_fixup(void);

#endif /* S32G_LOWLEVEL_H */
