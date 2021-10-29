/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <s32g_bl_common.h>

int pmic_setup(void)
{
	vr5510_t fsu;
	int ret;

	s32g_reinit_i2c();

	ret = vr5510_get_inst(VR5510_FSU_NAME, &fsu);
	if (ret) {
		ERROR("Failed to get VR5510 FSU\n");
		return ret;
	}

	return pmic_disable_wdg(fsu);
}
