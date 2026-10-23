/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <s32g_bl_common.h>
#include <s32g_vr5510.h>

#pragma weak s32g_vr5510_soc_setup

/*
 * SoC-specific VR5510 setup, run while the FSU is still in INIT_FS, before
 * its watchdog is disabled.
 */
int s32g_vr5510_soc_setup(vr5510_t fsu)
{
	return 0;
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

	ret = s32g_vr5510_soc_setup(fsu);
	if (ret)
		return ret;

	return pmic_disable_wdg(fsu);
}
