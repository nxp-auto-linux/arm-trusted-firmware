/*
 * Copyright 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <s32g_bl_common.h>
#include <s32g_vr5510.h>

static int watchdog_refresh(vr5510_t fsu)
{
	uint16_t reg;
	uint8_t *regp = (uint8_t *)&reg;
	int ret;

	ret = vr5510_read(fsu, VR5510_FS_WD_SEED, regp, sizeof(reg));
	if (ret)
		return ret;

	/* Challenger watchdog refresh */
	reg = ((~(reg * 4 + 2) & 0xFFFFFFFFU) / 4) & 0xFFFFU;

	ret = vr5510_write(fsu, VR5510_FS_WD_ANSWER, regp, sizeof(reg));
	if (ret) {
		ERROR("Failed to write VR5510 WD answer\n");
		return ret;
	}

	ret = vr5510_read(fsu, VR5510_FS_I_WD_CFG, regp, sizeof(reg));
	if (ret)
		return ret;

	if (VR5510_ERR_CNT(reg)) {
		ERROR("Failed to refresh watchdog\n");
		return -EIO;
	}

	return 0;
}

static int vr5510_goto_initfs(vr5510_t fsu)
{
	int ret;
	uint16_t reg;
	uint8_t *regp = (uint8_t *)&reg;

	ret = watchdog_refresh(fsu);
	if (ret) {
		ERROR("Failed to refresh VR5510 WDG\n");
		return ret;
	}

	ret = vr5510_read(fsu, VR5510_FS_SAFE_IOS, regp, sizeof(reg));
	if (ret)
		return ret;

	reg |= VR5510_SAFE_IOS_GOTO_INITFS;
	ret = vr5510_write(fsu, VR5510_FS_SAFE_IOS, regp, sizeof(reg));
	if (ret)
		return ret;

	ret = vr5510_read(fsu, VR5510_FS_SAFE_IOS, regp, sizeof(reg));
	if (ret)
		return ret;

	return 0;
}

int pmic_prepare_for_suspend(void)
{
	int ret;
	vr5510_t mu, fsu;

	uint16_t reg;
	uint8_t *regp = (uint8_t *)&reg;

	s32g_reinit_i2c();

	ret = vr5510_get_inst(VR5510_MU_NAME, &mu);
	if (ret)
		return ret;

	ret = vr5510_get_inst(VR5510_FSU_NAME, &fsu);
	if (ret)
		return ret;

	ret = vr5510_goto_initfs(fsu);
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

	reg = pmic_stby_pwr_rails();
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

	ret = vr5510_read(mu, VR5510_M_MODE, regp, sizeof(reg));
	if (ret) {
		ERROR("Failed to read VR5510_M_MODE register\n");
		return;
	}

	reg |= VR5510_MODE_PWRON1DIS;
	ret = vr5510_write(mu, VR5510_M_MODE, regp, sizeof(reg));
	if (ret) {
		ERROR("Failed to write VR5510_M_MODE register\n");
		return;
	}

	reg = VR5510_CTRL1_GOTO_OFF;
	ret = vr5510_write(mu, VR5510_M_SM_CTRL1, regp, sizeof(reg));
	if (ret)
		ERROR("Failed to write VR5510_M_SM_CTRL1 register\n");
}

int pmic_disable_wdg(vr5510_t fsu)
{
	uint16_t reg;
	uint8_t *regp = (uint8_t *)&reg;
	int ret;

	ret = vr5510_read(fsu, VR5510_FS_STATES, regp, sizeof(reg));
	if (ret)
		return ret;

	if (VR5510_STATE(reg) != INIT_FS) {
		WARN("VR5510 is not in INIT_FS state\n");
		return 0;
	}

	/* Disable watchdog */
	ret = vr5510_read(fsu, VR5510_FS_WD_WINDOW, regp, sizeof(reg));
	if (ret)
		return ret;

	reg &= ~VR5510_WD_WINDOW_MASK;
	ret = vr5510_write(fsu, VR5510_FS_WD_WINDOW, regp, sizeof(reg));
	if (ret) {
		ERROR("Failed write watchdog window\n");
		return ret;
	}

	reg = ~reg & 0xFFFFU;
	ret = vr5510_write(fsu, VR5510_FS_NOT_WD_WINDOW, regp, sizeof(reg));
	if (ret) {
		ERROR("Failed write watchdog window\n");
		return ret;
	}

	ret = vr5510_read(fsu, VR5510_FS_DIAG_SAFETY, regp, sizeof(reg));
	if (ret)
		return ret;

	if (!VR5510_ABIST1_OK(reg)) {
		ERROR("VR5510 is not in ABIST1 state\n");
		return -EIO;
	}

	/* Disable FCCU monitoring */
	ret = vr5510_read(fsu, VR5510_FS_I_SAFE_INPUTS, regp, sizeof(reg));
	if (ret)
		return ret;

	reg &= ~VR5510_FCCU_CFG_MASK;
	ret = vr5510_write(fsu, VR5510_FS_I_SAFE_INPUTS, regp, sizeof(reg));
	if (ret) {
		ERROR("Failed to disable FCCU\n");
		return ret;
	}

	reg = ~reg & 0xFFFFU;
	ret = vr5510_write(fsu, VR5510_FS_I_NOT_SAFE_INPUTS, regp, sizeof(reg));
	if (ret) {
		ERROR("Failed to disable FCCU\n");
		return ret;
	}

	return watchdog_refresh(fsu);
}
