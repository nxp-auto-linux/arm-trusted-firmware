/*
 * Copyright 2019-2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32G_LOWLEVEL_H
#define S32G_LOWLEVEL_H

#include "s32cc_lowlevel.h"

int plat_is_my_cpu_primary(void);
void reset_registers_for_lockstep(void);

#endif /* S32G_LOWLEVEL_H */
