/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "pmic/vr5510.h"

int pmic_prepare_for_suspend(void)
{
	int ret;
	vr5510_t mu, fsu;

	uint16_t reg;
	uint8_t *regp = (uint8_t *)&reg;

	ret = vr5510_get_inst(VR5510_MU_NAME, &mu);
	if (ret)
		return ret;

	ret = vr5510_get_inst(VR5510_FSU_NAME, &fsu);
	if (ret)
		return ret;


	/* Clear I2C errors if any */
	reg = VR5510_FLAG3_I2C_M_REQ | VR5510_FLAG3_I2C_M_CRC;
	ret = vr5510_write(mu, VR5510_M_FLAG3, regp, sizeof(reg));
	if (ret)
		return ret;

	/* Wait forever */
	reg = 0x0;
	ret = vr5510_write(mu, VR5510_M_SM_CTRL1, regp, sizeof(reg));
	if (ret)
		return ret;

	reg = VR5510_CTRL3_VPREV_STBY | VR5510_CTRL3_HVLDO_STBY
		| VR5510_CTRL3_BUCK3_STBY |  VR5510_CTRL3_LDO2_STBY;
	ret = vr5510_write(mu, VR5510_M_REG_CTRL3, regp, sizeof(reg));
	if (ret)
		return ret;

	reg = VR5510_FLAG1_ALL_FLAGS;
	ret = vr5510_write(mu, VR5510_M_FLAG1, regp, sizeof(reg));
	if (ret)
		return ret;

	reg = VR5510_FLAG2_ALL_FLAGS;
	ret = vr5510_write(mu, VR5510_M_FLAG2, regp, sizeof(reg));
	if (ret)
		return ret;

	reg = VR5510_M_CLOCK2_600KHZ;
	ret = vr5510_write(mu, VR5510_M_CLOCK2, regp, sizeof(reg));
	if (ret)
		return ret;

	/* Check for I2C errors */
	ret = vr5510_read(mu, VR5510_M_FLAG3, regp, sizeof(reg));
	if (ret)
		return ret;

	if (reg & (VR5510_FLAG3_I2C_M_REQ | VR5510_FLAG3_I2C_M_CRC)) {
		ERROR("VR5510-MU: Detected I2C errors");
		return -EIO;
	}

	/* Clear I2C errors if any */
	reg = VR5510_GRL_FLAGS_I2C_FS_REQ | VR5510_GRL_FLAGS_I2C_FS_CRC;
	ret = vr5510_write(fsu, VR5510_FS_GRL_FLAGS, regp, sizeof(reg));
	if (ret)
		return ret;

	/* Disable I2C timeout */
	reg = 0;
	ret = vr5510_write(fsu, VR5510_FS_I_SAFE_INPUTS, regp, sizeof(reg));
	if (ret)
		return ret;

	reg = VR5510_FS_I_NOT_VALUE(reg);
	ret = vr5510_write(fsu, VR5510_FS_I_NOT_SAFE_INPUTS, regp, sizeof(reg));
	if (ret)
		return ret;

	/* Check for I2C errors */
	ret = vr5510_read(fsu, VR5510_FS_GRL_FLAGS, regp, sizeof(reg));
	if (ret)
		return ret;

	if (reg & (VR5510_GRL_FLAGS_I2C_FS_REQ | VR5510_GRL_FLAGS_I2C_FS_CRC)) {
		ERROR("VR5510-FSU: Detected I2C errors\n");
		return -EIO;
	}

	/* Standby request */
	ret = vr5510_read(fsu, VR5510_FS_SAFE_IOS, regp, sizeof(reg));
	if (ret)
		return ret;

	reg |= VR5510_SAFE_IOS_STBY_REQ;
	ret = vr5510_write(fsu, VR5510_FS_SAFE_IOS, regp, sizeof(reg));
	if (ret)
		return ret;

	return 0;
}

void pmic_system_off(void)
{
	int ret;
	vr5510_t mu;

	uint16_t reg;
	uint8_t *regp = (uint8_t *)&reg;

	ret = vr5510_get_inst(VR5510_MU_NAME, &mu);
	if (ret) {
		ERROR("Failed to get VR5510 MU\n");
		return;
	}

	reg = VR5510_CTRL1_GOTO_OFF;
	ret = vr5510_write(mu, VR5510_M_SM_CTRL1, regp, sizeof(reg));
	if (ret)
		ERROR("Failed to write VR5510_M_SM_CTRL1 register\n");
}

