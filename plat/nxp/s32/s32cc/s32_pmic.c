/*
 * Copyright 2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <s32cc_pmic.h>

#pragma weak pmic_prepare_for_suspend
#pragma weak pmic_system_off
#pragma weak pmic_setup
#pragma weak pmic_stby_pwr_rails

int pmic_prepare_for_suspend(void)
{
	return 0;
}

void pmic_system_off(void)
{
}

int pmic_setup(void)
{
	return 0;
}

uint16_t pmic_stby_pwr_rails(void)
{
	return 0;
}
