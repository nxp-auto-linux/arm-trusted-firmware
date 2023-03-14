/*
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_BL_COMMON_H
#define S32G_BL_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include "s32_bl_common.h"

#define HSE_GCR (0x40210114UL)
#define HSE_FSR (0x40210104UL)
#define HSE_STATUS_INIT_OK BIT(24)
#define HSE_PERIPH_CONFIG_DONE BIT(0)

void s32g_reinit_i2c(void);

void dt_init_pmic(void);
void dt_init_ocotp(void);
void dt_init_wkpu(void);
#endif
