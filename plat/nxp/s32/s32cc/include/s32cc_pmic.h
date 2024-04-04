/*
 * Copyright 2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_PMIC_H
#define S32CC_PMIC_H

#include <stdint.h>

int pmic_prepare_for_suspend(void);
void pmic_system_off(void);
int pmic_setup(void);
uint16_t pmic_stby_pwr_rails(void);

#endif /* S32CC_PMIC_H */
