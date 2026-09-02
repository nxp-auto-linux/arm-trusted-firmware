/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <s32_bl_common.h>
#include <s32n_bl_common.h>
#include <s32n_clk_utils.h>
#include <fconf/s32n_fconf_ufs_getter.h>
#include <drivers/clk.h>
#include <drivers/dw_ufs.h>
#include <drivers/nxp/s32n/s32n_ufs.h>
#include <drivers/ufs.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#define SCM_ONE_US_TICK		0x2C
#define SCM_ONE_US_TICK_MASK	GENMASK(8, 0)

#define SCM_STATUS				0x30

#define SCM_STATUS_PHY_RESET_MASK		GENMASK(0, 0)
#define SCM_STATUS_PHY_RESET_OFFSET		0
#define SCM_STATUS_PHY_RESET(val)		\
	(((val) << SCM_STATUS_PHY_RESET_OFFSET) & \
	 SCM_STATUS_PHY_RESET_MASK)

#define SCM_STATUS_SRAM_BYPASS_MASK		GENMASK(1, 1)
#define SCM_STATUS_SRAM_BYPASS_OFFSET		1
#define SCM_STATUS_SRAM_BYPASS(val)		\
	(((val) << SCM_STATUS_SRAM_BYPASS_OFFSET) & \
	 SCM_STATUS_SRAM_BYPASS_MASK)

#define SCM_STATUS_SRAM_EXT_LD_DONE_MASK	GENMASK(2, 2)
#define SCM_STATUS_SRAM_INIT_DONE_MASK		GENMASK(16, 16)

#define MPHY_REG_LSB_MASK			0x00FF
#define MPHY_REG_MSB_MASK			0xFF00
#define RAWLANEN_DIG_PCS_XF_RX_OVRD_IN_0	0x3006
#define RAWLANEN_DIG_PCS_XF_RX_OVRD_IN_1	0x3106

/* M-PHY per-lane CR register addresses. */
#define RX_DAC_CTRL(n)		(0x10AF + ((n) * 0x100))
#define RX_DAC_CTRL_OVRD(n)	(0x10B0 + ((n) * 0x100))
#define RX_DAC_CTRL_SEL(n)	(0x10B1 + ((n) * 0x100))
#define RX_DAC_CTRL_EN(n)	(0x10B8 + ((n) * 0x100))
#define FAST_FLAGS(n)		(0x401C + ((n) * 0x100))
#define FW_CALIB_CCFG(n)	(0x404D + ((n) * 0x100))

#define CFG_MPHY_INIT_TIMEOUT_VALUE_US          3000000L
#define HBRN8_TIMEOUT_US			1000000L
#define USEC_PER_SEC				1000000L

#define MAX_VALID_ONE_US_TICK			SCM_ONE_US_TICK_MASK

/* One M-PHY CR (config-register) write; the per-lane offset is encoded
 * in the address.
 */
struct s32n_phy_cfg {
	uint16_t addr;
	uint16_t val;
};

/* The structure pointed to by UTRLBA is 1KB in length and shall
 * be 1KB aligned.
 */
static uint8_t ufs_desc[UFS_DESC_SIZE] __aligned(0x400);

static bool is_sram_init_done(void *arg)
{
	uintptr_t scm_base = (uintptr_t)arg;

	return mmio_read_32(scm_base + SCM_STATUS) &
		SCM_STATUS_SRAM_INIT_DONE_MASK;
}

static bool is_hibern8_reached(void *arg)
{
	unsigned int num_lanes = (unsigned int)(uintptr_t)arg;
	unsigned int fsm = 0;
	unsigned int i;
	int ret;

	for (i = 0; i < num_lanes; i++) {
		ret = ufshc_dme_get(TX_FSM_STATE_OFFSET, i, &fsm);
		if (ret != 0 || fsm != TX_FSM_STATE_HIBERN8)
			return false;
	}

	return true;
}

static int s32n_phy_read_cr(uint16_t addr, uint16_t *value)
{
	unsigned int uic_arg3 = 0;
	int ret = 0;

	/* CR LSB parallel address - 0x8116. */
	ret |= ufshc_dme_set(CBCREGADDRLSB_OFFSET, 0, addr & 0x00FF);
	/* CR MSB parallel address - 0x8117. */
	ret |= ufshc_dme_set(CBCREGADDRMSB_OFFSET, 0, addr >> 8);
	/* CR parallel control value: 1- write, 0 - read - 0x811c. */
	ret |= ufshc_dme_set(CBCREGRDWRSEL_OFFSET, 0, 0x0);

	ret |= ufshc_dme_set(VS_MPHY_CFG_UPDT_OFFSET, 0, 0x1);

	/* CR LSB parallel read value - 0x811a. */
	ret |= ufshc_dme_get(CBCREGRDLSB_OFFSET, 0, &uic_arg3);
	*value = (uint16_t)((uint16_t)uic_arg3 & MPHY_REG_LSB_MASK);
	/* CR MSB parallel read value - 0x811b. */
	ret |= ufshc_dme_get(CBCREGRDMSB_OFFSET, 0, &uic_arg3);
	*value |= (uint16_t)((uint16_t)(uic_arg3 << 8) & MPHY_REG_MSB_MASK);

	return ret;
}

static int s32n_phy_write_cr(uint16_t addr, uint16_t value)
{
	int ret = 0;

	/* CR LSB parallel address - 0x8116. */
	ret |= ufshc_dme_set(CBCREGADDRLSB_OFFSET, 0, addr & 0x00FF);
	/* CR MSB parallel address - 0x8117. */
	ret |= ufshc_dme_set(CBCREGADDRMSB_OFFSET, 0, addr >> 8);
	/* CR LSB parallel write value - 0x8118. */
	ret |= ufshc_dme_set(CBCREGWRLSB_OFFSET, 0, value & 0x00FF);
	/* CR MSB parallel write value - 0x8119. */
	ret |= ufshc_dme_set(CBCREGWRMSB_OFFSET, 0, value >> 8);
	/* CR parallel control value: 1- write, 0 - read - 0x811c. */
	ret |= ufshc_dme_set(CBCREGRDWRSEL_OFFSET, 0, 0x1);
	ret |= ufshc_dme_set(VS_MPHY_CFG_UPDT_OFFSET, 0, 0x1);

	return ret;
}

static int s32n_phy_sram_ext_loading(void)
{
	int ret = 0;
	uint16_t reg;

	ret |= s32n_phy_read_cr(0xC002, &reg);
	ret |= s32n_phy_write_cr(0xC002, reg);

	return ret;
}

static int s32n_get_core_clock_rate(unsigned long *clk_rate)
{
	uintptr_t reg_addr = FCONF_GET_PROPERTY(hw_config, ufs_config,
			reg_addr);

	*clk_rate = s32n_get_clk_freq(reg_addr, "core_clk");

	return *clk_rate == 0 ? -EINVAL : 0;
}

static unsigned long s32n_calculate_us_tick(unsigned long clk_rate)
{
	return clk_rate / USEC_PER_SEC;
}

static int s32n_phy_pre_link_calib(void)
{
	/* Calibrations that must run while the MPHY is disabled. */
	static const struct s32n_phy_cfg calib_mphy_disabled[] = {
		/* Bypass RX-AFE calibrations. */
		{ FAST_FLAGS(0), 0x4 },
		{ FAST_FLAGS(1), 0x4 },
	};
	/* Calibrations that must run while the MPHY is enabled. */
	static const struct s32n_phy_cfg calib_mphy_enabled[] = {
		/* Write ATT offset compensation code to internal DAC. */
		{ RX_DAC_CTRL_OVRD(0), 0x1 },
		{ RX_DAC_CTRL_OVRD(1), 0x1 },
		{ RX_DAC_CTRL(0), 0x8E },
		{ RX_DAC_CTRL(1), 0x91 },
		{ RX_DAC_CTRL_SEL(0), 0x1 },
		{ RX_DAC_CTRL_SEL(1), 0x1 },
		{ RX_DAC_CTRL_EN(0), 0x1 },
		{ RX_DAC_CTRL_EN(1), 0x1 },
		{ RX_DAC_CTRL_OVRD(0), 0x0 },
		{ RX_DAC_CTRL_OVRD(1), 0x0 },
		/* Write CTLE offset compensation code to internal DAC. */
		{ RX_DAC_CTRL_OVRD(0), 0x1 },
		{ RX_DAC_CTRL_OVRD(1), 0x1 },
		{ RX_DAC_CTRL(0), 0x71 },
		{ RX_DAC_CTRL(1), 0x7F },
		{ RX_DAC_CTRL_SEL(0), 0x2 },
		{ RX_DAC_CTRL_SEL(1), 0x2 },
		{ RX_DAC_CTRL_EN(0), 0x1 },
		{ RX_DAC_CTRL_EN(1), 0x1 },
		{ RX_DAC_CTRL_OVRD(0), 0x0 },
		{ RX_DAC_CTRL_OVRD(1), 0x0 },
		/* Generic FW calibration configurations. */
		{ FW_CALIB_CCFG(0), 0x100 },
		{ FW_CALIB_CCFG(1), 0x100 },
	};
	unsigned int i;
	int ret = 0;

	for (i = 0; i < ARRAY_SIZE(calib_mphy_disabled); i++)
		ret |= s32n_phy_write_cr(calib_mphy_disabled[i].addr,
					 calib_mphy_disabled[i].val);

	/* Enable MPHY before the DAC/FW calibration writes. */
	ret |= ufshc_dme_set(VS_MPHY_CFG_UPDT_OFFSET, 0, 0x1);
	ret |= ufshc_dme_set(VS_MPHY_DISABLE_OFFSET, 0, 0x0);

	for (i = 0; i < ARRAY_SIZE(calib_mphy_enabled); i++)
		ret |= s32n_phy_write_cr(calib_mphy_enabled[i].addr,
					 calib_mphy_enabled[i].val);

	ret |= ufshc_dme_set(VS_MPHY_CFG_UPDT_OFFSET, 0, 0x1);

	return ret;
}

static int s32n_phy_reset_config(void)
{
	int ret = 0;

	/* Clock control. */
	ret |= ufshc_dme_set(CBREFCLKCTRL2_OFFSET, 0, 0x80);
	/* RX squelch control. */
	ret |= ufshc_dme_set(RXSQCONTROL_OFFSET, SEL_RX_LANE0, 0x1);
	ret |= ufshc_dme_set(RXSQCONTROL_OFFSET, SEL_RX_LANE1, 0x1);
	/* RX Rhold control option. */
	ret |= ufshc_dme_set(RXRHOLDCTRLOPT_OFFSET, SEL_RX_LANE0, 0x2);
	ret |= ufshc_dme_set(RXRHOLDCTRLOPT_OFFSET, SEL_RX_LANE1, 0x2);
	/* CR parallel selection. */
	ret |= ufshc_dme_set(CBCRCTRL_OFFSET, 0, 0x1);
	/* TX Equalization Main de-emphasis 0db, large amplitude. */
	ret |= ufshc_dme_set(TX_EQ_MAIN_DE00_LA_OFFSET, SEL_TX_LANE0, 0x27);
	ret |= ufshc_dme_set(TX_EQ_MAIN_DE00_LA_OFFSET, SEL_TX_LANE1, 0x27);
	/* TX Equalization Main de-emphasis 3.5db, large amplitude. */
	ret |= ufshc_dme_set(TX_EQ_MAIN_DE03_LA_OFFSET, SEL_TX_LANE0, 0x21);
	ret |= ufshc_dme_set(TX_EQ_MAIN_DE03_LA_OFFSET, SEL_TX_LANE1, 0x21);
	/* TX Equalization Main de-emphasis 6db, small amplitude. */
	ret |= ufshc_dme_set(TX_EQ_MAIN_DE06_SA_OFFSET, SEL_TX_LANE0, 0x1D);
	ret |= ufshc_dme_set(TX_EQ_MAIN_DE06_SA_OFFSET, SEL_TX_LANE1, 0x1D);
	/* TX Equalization Main de-emphasis 6db, large amplitude. */
	ret |= ufshc_dme_set(TX_EQ_MAIN_DE06_LA_OFFSET, SEL_TX_LANE0, 0x1D);
	ret |= ufshc_dme_set(TX_EQ_MAIN_DE06_LA_OFFSET, SEL_TX_LANE1, 0x1D);
	/* TX Equalization Post de-emphasis 6db, small amplitude. */
	ret |= ufshc_dme_set(TX_EQ_POST_DE06_SA_OFFSET, SEL_TX_LANE0, 0x0A);
	ret |= ufshc_dme_set(TX_EQ_POST_DE06_SA_OFFSET, SEL_TX_LANE1, 0x0A);
	/* TX Equalization Post de-emphasis 6db, large amplitude. */
	ret |= ufshc_dme_set(TX_EQ_POST_DE06_LA_OFFSET, SEL_TX_LANE0, 0x0A);
	ret |= ufshc_dme_set(TX_EQ_POST_DE06_LA_OFFSET, SEL_TX_LANE1, 0x0A);
	/* TX PHY direct iboost/rboost configuration. */
	ret |= ufshc_dme_set(TXC10_IBOOST_RBOOST_CONF_OFFSET, SEL_TX_LANE0, 0x0);
	ret |= ufshc_dme_set(TXC10_IBOOST_RBOOST_CONF_OFFSET, SEL_TX_LANE1, 0x0);
	/* RX PHY direct configuration 7. */
	ret |= ufshc_dme_set(RXC10DIRECTCONF7_OFFSET, SEL_RX_LANE0, 0x3);
	ret |= ufshc_dme_set(RXC10DIRECTCONF7_OFFSET, SEL_RX_LANE1, 0x3);
	/* CB PHY direct MPLLB configuration. */
	ret |= ufshc_dme_set(MPLLBFRACNDENLSB_OFFSET, 0, 0x1);
	ret |= ufshc_dme_set(V2IDIV5DIV2MPLLB_OFFSET, 0, 0x1);

	ret |= ufshc_dme_set(VS_MPHY_CFG_UPDT_OFFSET, 0, 0x1);

	return ret;
}

static int s32n_phy_init(ufs_params_t *params)
{
	unsigned long core_clock_freq, one_us_tick;
	uintptr_t scm_base = 0, hci_base;
	uint8_t mphy_boot_mode;
	size_t scm_size = 0;
	int ret = 0;

	assert((params != NULL) && (params->reg_base != 0));

	hci_base = params->reg_base;

	/* There are three supported MPHY boot options implemented:
	 * - Running MPHY from internal FW ROM:
	 *  MPHY_RAM_CONFIG_STATUS.SRAM_BYPASS = 1
	 *  MPHY_RAM_CONFIG_STATUS.SRAM_EXT_LD_DONE = 0
	 * - Running MPHY from a (modified) FW ROM copy in external SRAM:
	 *  MPHY_RAM_CONFIG_STATUS.SRAM_BYPASS = 0
	 *  MPHY_RAM_CONFIG_STATUS.SRAM_EXT_LD_DONE = 0 (-> 1 after external loading is done).
	 * - MPHY initialization skipped used when running in a simulated
	 * environment or when another component is doing it.
	 */
	mphy_boot_mode = FCONF_GET_PROPERTY(hw_config, ufs_config,
					    mphy_boot_mode);

	if (mphy_boot_mode == MPHY_BOOT_NONE) {
		INFO("Skipping UFS MPHY init.\n");
		return 0;
	}

	/* SCM registers. */
	scm_base = FCONF_GET_PROPERTY(hw_config, ufs_config,
				      scm_reg_addr);
	scm_size = FCONF_GET_PROPERTY(hw_config, ufs_config,
				  scm_reg_size);

	if (!scm_base || !scm_size) {
		ERROR("No SCM regs in the enabled UFS node from the device tree\n");
		return -EINVAL;
	}

	ret = mmap_add_dynamic_region(scm_base, scm_base,
			MMU_ROUND_UP_TO_PAGE(scm_size),
			MT_DEVICE | MT_RW);

	if (ret < 0) {
		ERROR("SCM regs mapping: error %d\n", ret);
		return ret;
	}

	/* Set the micro-second tick rate generator. */
	ret = s32n_get_core_clock_rate(&core_clock_freq);
	if (ret < 0) {
		ERROR("Could not obtain ufs core rate: error %d\n", ret);
		return ret;
	}

	one_us_tick = s32n_calculate_us_tick(core_clock_freq);
	if (one_us_tick == 0 || one_us_tick > MAX_VALID_ONE_US_TICK) {
		ERROR("Invalid one_us_tick value: %lu (clk_rate: %lu Hz).\n",
		      one_us_tick, core_clock_freq);
		return -EINVAL;
	}

	/* HID is set by FSS */
	mmio_write_32(scm_base + SCM_ONE_US_TICK, one_us_tick);
	mmio_write_32(hci_base + HCLKDIV, one_us_tick);

	/* Reset MPHY_RAM_CONFIG_STATUS to default value; keep MPHY in reset */
	mmio_write_32(scm_base + SCM_STATUS, SCM_STATUS_PHY_RESET(1));
	if (mphy_boot_mode == MPHY_BOOT_ROM) {
		/* ROM Mode */
		mmio_setbits_32(scm_base + SCM_STATUS,
				SCM_STATUS_SRAM_BYPASS(1));
	}

	ret = s32n_phy_reset_config();
	assert(ret == 0);

	/* Release PHY_RESET. */
	mmio_clrbits_32(scm_base + SCM_STATUS, SCM_STATUS_PHY_RESET(1));

	/* Wait until MPHY_RAM_CONFIG_STATUS.SRAM_INIT_DONE = 1.
	 * In case of MPHY FW running from SRAM max can be as high as 15ms@100MHz
	 * reference clock (FW is copied from MPHY ROM to SRAM).
	 */
	ret = wait_for_condition_timeout(is_sram_init_done,
			(void *)scm_base, CFG_MPHY_INIT_TIMEOUT_VALUE_US);
	if (ret) {
		ERROR("UFS MPHY init not done!\n");
		return 0;
	}

	if (mphy_boot_mode == MPHY_BOOT_COPY_SRAM) {
		/* SRAM External Loading phase - MPHY control registers can be
		 * overwritten using CREG interface.
		 */
		ret |= s32n_phy_sram_ext_loading();

		/* Set MPHY_RAM_CONFIG_STATUS.SRAM_EXT_LD_DONE. */
		mmio_setbits_32(scm_base + SCM_STATUS,
				SCM_STATUS_SRAM_EXT_LD_DONE_MASK);
	}

	/* Pre-link MPHY calibration. */
	ret = s32n_phy_pre_link_calib();
	if (ret) {
		ERROR("UFS MPHY pre-link calibration failed!\n");
		return ret;
	}

	ret |= ufshc_dme_set(N_DEVICEID_OFFSET, 0, 0x0);
	if (ret)
		return ret;

	ret = wait_for_condition_timeout(is_hibern8_reached,
					 (void *)(uintptr_t)2, HBRN8_TIMEOUT_US);
	if (ret) {
		ERROR("UFS MPHY HIBERN8 wait timed out!\n");
		return ret;
	}

	return 0;
}

static int s32n_phy_set_pwr_mode(ufs_params_t *params)
{
	unsigned int data = 0;
	uint8_t mphy_boot_mode;
	int ret = 0;

	mphy_boot_mode = FCONF_GET_PROPERTY(hw_config, ufs_config,
					    mphy_boot_mode);

	/* If MPHY initialization is not handled by TF-A,
	 * invoke the generic DW power mode update procedure.
	 */
	if (mphy_boot_mode == MPHY_BOOT_NONE)
		return dwufs_phy_set_pwr_mode(params);

	/* Set Connection State to IDLE(it allows CPort Attributes to be set) */
	ret |= ufshc_dme_set(T_CONNECTIONSTATE_OFFSET, CPORT_0, CPORT_IDLE);

	ret |= ufshc_dme_set(T_CPORTFLAGS_OFFSET, CPORT_0,
			     CPORT_FLAGS(CSV_N_OFF, CSD_N_OFF, E2EFC_OFF));
	ret |= ufshc_dme_set(T_CONNECTIONSTATE_OFFSET, CPORT_0, CPORT_CONNECTED);
	assert(ret == 0);

	ret |= ufshc_dme_get(T_CONNECTIONSTATE_OFFSET, CPORT_0, &data);
	assert((ret == 0) && (data == CPORT_CONNECTED));

	/* Post startup link calibration. */
	ret |= s32n_phy_write_cr(RAWLANEN_DIG_PCS_XF_RX_OVRD_IN_0, 0xc);
	ret |= s32n_phy_write_cr(RAWLANEN_DIG_PCS_XF_RX_OVRD_IN_1, 0xc);
	ret |= s32n_phy_write_cr(RAWLANEN_DIG_PCS_XF_RX_OVRD_IN_0, 0x8);
	ret |= s32n_phy_write_cr(RAWLANEN_DIG_PCS_XF_RX_OVRD_IN_1, 0x8);
	ret |= s32n_phy_write_cr(RAWLANEN_DIG_PCS_XF_RX_OVRD_IN_0, 0x0);
	ret |= s32n_phy_write_cr(RAWLANEN_DIG_PCS_XF_RX_OVRD_IN_1, 0x0);

	/* Performing MPHY configuration for rate change:
	 * CB rate selection: 0 - rate A, 1 - rate B;
	 */
	ret |= ufshc_dme_set(CBRATESEL_OFFSET, 0, 0x1);
	ret |= ufshc_dme_set(VS_MPHY_CFG_UPDT_OFFSET, 0, 0x1);

	return dwufs_phy_set_pwr_mode(params);
}

static const ufs_ops_t s32n_ufs_ops = {
	.phy_init = s32n_phy_init,
	.phy_set_pwr_mode = s32n_phy_set_pwr_mode,
};

int s32n_ufs_init(void)
{
	ufs_params_t ufs_params = {
		.desc_base = (uintptr_t)&ufs_desc,
		.desc_size = sizeof(ufs_desc),
		.flags = 0,
	};
	size_t size;
	int ret;

	ufs_params.reg_base = FCONF_GET_PROPERTY(hw_config, ufs_config,
						 reg_addr);
	size = FCONF_GET_PROPERTY(hw_config, ufs_config,
						 reg_size);

	if (!ufs_params.reg_base || !size) {
		ERROR("No UFS node found enabled in the device tree\n");
		panic();
	}

	ret = mmap_add_dynamic_region(ufs_params.reg_base,
			ufs_params.reg_base, MMU_ROUND_UP_TO_PAGE(size),
			MT_DEVICE | MT_RW);

	if (ret < 0) {
		ERROR("UFSHC regs mapping: error %d\n", ret);
		return ret;
	}

	ret = ufs_init(&s32n_ufs_ops, &ufs_params);
	if (ret) {
		ERROR("The UFS initialization failed\n");
		return ret;
	}

	return 0;
}

