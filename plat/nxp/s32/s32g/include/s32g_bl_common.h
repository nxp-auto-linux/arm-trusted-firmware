/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_BL_COMMON_H
#define S32G_BL_COMMON_H

#include <pmic/vr5510.h>
#include <stdbool.h>
#include <stdint.h>
#include "s32_bl_common.h"

int pmic_prepare_for_suspend(void);
void pmic_system_off(void);
int pmic_disable_wdg(vr5510_t fsu);
int pmic_setup(void);

void s32g_reinit_i2c(void);

bool s32gen1_is_wkp_short_boot(void);

void dt_init_pmic(void);
void dt_init_ocotp(void);
#endif
