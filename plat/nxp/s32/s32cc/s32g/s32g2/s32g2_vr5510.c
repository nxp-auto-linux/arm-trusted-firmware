/*
 * Copyright 2021-2022, 2025-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <s32g_bl_common.h>
#include <s32g_vr5510.h>
#include <drivers/nxp/s32/ocotp.h>

uint16_t pmic_stby_pwr_rails(void)
{
	return (VR5510_CTRL3_VPREV_STBY | VR5510_CTRL3_HVLDO_STBY
	      | VR5510_CTRL3_BUCK3_STBY | VR5510_CTRL3_LDO2_STBY);
}

static int is_svs_needed(bool *status)
{
	uint32_t val;
	int ret;

	ret = s32cc_ocotp_read(S32CC_OCOTP_DIE_PROCESS_ADDR, &val);
	if (ret)
		return ret;

	*status = !!(val & S32CC_OCOTP_DIE_PROCESS_MASK);

	return 0;
}

/* S32G2: apply the SVS trimming before the watchdog is disabled */
int s32g_vr5510_soc_setup(vr5510_t fsu)
{
	int ret;
	uint16_t reg;
	bool enable_svs;
	uint8_t *regp = (uint8_t *)&reg;

	ret = is_svs_needed(&enable_svs);
	if (ret)
		return ret;

	if (!enable_svs)
		return 0;

	ret = vr5510_read(fsu, VR5510_FS_STATES, regp, sizeof(reg));
	if ((VR5510_STATE(reg) != INIT_FS) || ret) {
		ERROR("Cannot apply SVS in state %u\n",
			VR5510_STATE(reg));
		return -1;
	}

	/**
	 * Targeted voltage according to S32G & S32R DS : 0.76875V ->
	 * 5 SVS steps
	 */
	reg = 5 << VR5510_FS_I_SVS_SVS_OFFSET;
	ret = vr5510_write(fsu, VR5510_FS_I_SVS, regp, sizeof(reg));
	if (ret) {
		ERROR("Failed to write SVS\n");
		return ret;
	}

	reg = ~reg & 0xFFFFU;
	ret = vr5510_write(fsu, VR5510_FS_I_NOT_SVS, regp, sizeof(reg));
	if (ret) {
		ERROR("Failed to write NOT_SVS\n");
		return ret;
	}

	return 0;
}
