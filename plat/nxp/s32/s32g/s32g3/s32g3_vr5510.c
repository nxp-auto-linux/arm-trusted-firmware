/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <s32g_bl_common.h>
#include <s32g_vr5510.h>

uint16_t pmic_stby_pwr_rails(void)
{
	return (VR5510_CTRL3_VPREV_STBY | VR5510_CTRL3_HVLDO_STBY
	      | VR5510_CTRL3_BUCK3_STBY | VR5510_CTRL3_BUCK1_STBY);
}

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
