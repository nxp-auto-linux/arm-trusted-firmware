/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "pmic/vr5510.h"
#include "s32g_bl_common.h"
#include <drivers/nxp/s32g/ocotp.h>
#include <lib/libc/stdint.h>

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

static int pmic_disable_wdg(vr5510_t fsu)
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

	reg &= ~reg & 0xFFFFU;
	ret = vr5510_write(fsu, VR5510_FS_I_NOT_SAFE_INPUTS, regp, sizeof(reg));
	if (ret) {
		ERROR("Failed to disable FCCU\n");
		return ret;
	}

	return watchdog_refresh(fsu);
}

static int is_svs_needed(bool *status)
{
	uint32_t val;
	int ret;

	ret = s32gen1_ocotp_read(S32GEN1_OCOTP_DIE_PROCESS_ADDR, &val);
	if (ret)
		return ret;

	*status = !!(val & S32GEN1_OCOTP_DIE_PROCESS_MASK);

	return 0;
}

static int apply_svs(vr5510_t fsu)
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
	if (VR5510_STATE(reg) != INIT_FS) {
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

	ret = apply_svs(fsu);
	if (ret)
		return ret;

	return pmic_disable_wdg(fsu);
}
