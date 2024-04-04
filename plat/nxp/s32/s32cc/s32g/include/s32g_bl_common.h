/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_BL_COMMON_H
#define S32G_BL_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include "s32cc_bl_common.h"

void s32g_reinit_i2c(void);

void dt_init_pmic(void);
void dt_init_ocotp(void);
void dt_init_wkpu(void);
#endif
