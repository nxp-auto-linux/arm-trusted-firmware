/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_BL_COMMON_H
#define S32G_BL_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include "s32_bl_common.h"

void s32g_reinit_i2c(void);

bool s32gen1_is_wkp_short_boot(void);

void dt_init_pmic(void);
void dt_init_ocotp(void);
#endif
