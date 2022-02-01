/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <s32g_vr5510.h>

int pmic_prepare_for_suspend(void)
{
	return 0;
}

void pmic_system_off(void)
{}

int pmic_disable_wdg(vr5510_t fsu)
{
	return 0;
}
