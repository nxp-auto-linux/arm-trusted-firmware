/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_VR5510_H
#define S32G_VR5510_H

#include <pmic/vr5510.h>
#include <stdint.h>

int pmic_prepare_for_suspend(void);
void pmic_system_off(void);
int pmic_disable_wdg(vr5510_t fsu);
int pmic_setup(void);
uint16_t pmic_stby_pwr_rails(void);

#endif /* S32G_VR5510_H */
