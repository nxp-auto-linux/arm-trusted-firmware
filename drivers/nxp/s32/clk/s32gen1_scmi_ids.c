// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_scmi_clk.h>
#include <common/debug.h>
#include <dt-bindings/clock/s32gen1-clock.h>
#include <dt-bindings/clock/s32gen1-scmi-clock.h>
#include <errno.h>
#include <stdint.h>

#define INDEX(X)	((X) - S32GEN1_SCMI_CLK_BASE_ID)

struct s32gen1_scmi_clk cc_scmi_clk[] = {
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_A53,
			 S32GEN1_CLK_A53_CORE, "a53"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SERDES_AXI,
			 S32GEN1_CLK_XBAR, "serdes_axi"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SERDES_AUX,
			 S32GEN1_CLK_FIRC, "serdes_aux"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SERDES_APB,
			 S32GEN1_CLK_XBAR_DIV3, "serdes_apb"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SERDES_REF,
			 S32GEN1_CLK_SERDES_REF, "serdes_ref"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SERDES_PAD_REF,
			 S32GEN1_SCMI_COMPLEX_CLK, "serdes_pad_ref"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_FTM0_SYS,
			 S32GEN1_CLK_PER, "ftm0_sys"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_FTM0_EXT,
			 S32GEN1_CLK_FTM0_REF, "ftm0_ext"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_FTM1_SYS,
			 S32GEN1_CLK_PER, "ftm1_sys"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_FTM1_EXT,
			 S32GEN1_CLK_FTM1_REF, "ftm1_ext"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_FLEXCAN_REG,
			 S32GEN1_CLK_XBAR_DIV3, "flexcan_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_FLEXCAN_SYS,
			 S32GEN1_CLK_XBAR_DIV3, "flexcan_sys"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_FLEXCAN_CAN,
			 S32GEN1_CLK_CAN_PE, "flexcan_can"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_FLEXCAN_TS,
			 S32GEN1_CLK_XBAR_DIV2, "flexcan_ts"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_LINFLEX_XBAR,
			 S32GEN1_CLK_LINFLEXD, "linflex_xbar"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_LINFLEX_LIN,
			 S32GEN1_CLK_LIN_BAUD, "linflex_lin"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_GMAC0_TS,
			 S32GEN1_SCMI_COMPLEX_CLK, "gmac0_ts"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_GMAC0_RX_SGMII,
			 S32GEN1_SCMI_COMPLEX_CLK, "gmac0_rx_sgmii"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_GMAC0_TX_SGMII,
			 S32GEN1_SCMI_COMPLEX_CLK, "gmac0_tx_sgmii"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_GMAC0_RX_RGMII,
			 S32GEN1_SCMI_COMPLEX_CLK, "gmac0_rx_rgmii"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_GMAC0_TX_RGMII,
			 S32GEN1_SCMI_COMPLEX_CLK, "gmac0_tx_rgmii"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_GMAC0_RX_RMII,
			 S32GEN1_SCMI_COMPLEX_CLK, "gmac0_rx_rmii"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_GMAC0_TX_RMII,
			 S32GEN1_SCMI_COMPLEX_CLK, "gmac0_tx_rmii"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_GMAC0_RX_MII,
			 S32GEN1_SCMI_COMPLEX_CLK, "gmac0_rx_mii"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_GMAC0_TX_MII,
			 S32GEN1_SCMI_COMPLEX_CLK, "gmac0_tx_mii"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_GMAC0_AXI,
			 S32GEN1_CLK_XBAR, "gmac0_axi"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SPI_REG,
			 S32GEN1_CLK_SPI, "spi_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SPI_MODULE,
			 S32GEN1_CLK_SPI, "spi_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_QSPI_REG,
			 S32GEN1_CLK_XBAR_DIV3, "qspi_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_QSPI_AHB,
			 S32GEN1_CLK_XBAR_DIV3, "qspi_ahb"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_QSPI_FLASH2X,
			 S32GEN1_CLK_QSPI_2X, "qspi_flash2x"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_QSPI_FLASH1X,
			 S32GEN1_CLK_QSPI, "qspi_flash1x"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_USDHC_AHB,
			 S32GEN1_CLK_XBAR, "usdhc_ahb"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_USDHC_MODULE,
			 S32GEN1_CLK_XBAR_DIV3, "usdhc_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_USDHC_CORE,
			 S32GEN1_CLK_SDHC, "usdhc_core"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_USDHC_MOD32K,
			 S32GEN1_CLK_SIRC, "usdhc_mod32k"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_DDR_REG,
			 S32GEN1_CLK_XBAR_DIV3, "ddr_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_DDR_PLL_REF,
			 S32GEN1_CLK_DDR, "ddr_pll_ref"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_DDR_AXI,
			 S32GEN1_CLK_DDR, "ddr_axi"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SRAM_AXI,
			 S32GEN1_CLK_XBAR, "sram_axi"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SRAM_REG,
			 S32GEN1_CLK_XBAR_DIV3, "sram_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_I2C_REG,
			 S32GEN1_CLK_XBAR_DIV3, "i2c_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_I2C_MODULE,
			 S32GEN1_CLK_XBAR_DIV3, "i2c_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_RTC_REG,
			 S32GEN1_CLK_XBAR_DIV6, "rtc_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_RTC_SIRC,
			 S32GEN1_CLK_SIRC, "rtc_sirc"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_RTC_FIRC,
			 S32GEN1_CLK_FIRC, "rtc_firc"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SIUL2_REG,
			 S32GEN1_CLK_XBAR_DIV6, "siul2_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SIUL2_FILTER,
			 S32GEN1_CLK_FIRC, "siul2_filter"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_CRC_REG,
			 S32GEN1_CLK_XBAR_DIV3, "crc_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_CRC_MODULE,
			 S32GEN1_CLK_XBAR_DIV3, "crc_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_EIM0_REG,
			 S32GEN1_CLK_A53_CORE_DIV10, "eim0_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_EIM0_MODULE,
			 S32GEN1_CLK_A53_CORE_DIV10, "eim0_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_EIM123_REG,
			 S32GEN1_CLK_XBAR_DIV6, "eim123_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_EIM123_MODULE,
			 S32GEN1_CLK_XBAR_DIV6, "eim123_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_EIM_REG,
			 S32GEN1_CLK_XBAR_DIV6, "eim_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_EIM_MODULE,
			 S32GEN1_CLK_XBAR_DIV6, "eim_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_FCCU_MODULE,
			 S32GEN1_CLK_XBAR_DIV6, "fccu_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_FCCU_SAFE,
			 S32GEN1_CLK_FIRC, "fccu_safe"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SWT_MODULE,
			 S32GEN1_CLK_XBAR_DIV3, "swt_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SWT_COUNTER,
			 S32GEN1_CLK_FIRC, "swt_counter"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_STM_MODULE,
			 S32GEN1_CLK_XBAR_DIV3, "stm_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_STM_REG,
			 S32GEN1_CLK_XBAR_DIV3, "stm_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_PIT_MODULE,
			 S32GEN1_CLK_XBAR_DIV3, "pit_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_PIT_REG,
			 S32GEN1_CLK_XBAR_DIV3, "pit_reg"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_EDMA_MODULE,
			 S32GEN1_CLK_XBAR, "edma_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_EDMA_AHB,
			 S32GEN1_CLK_XBAR, "edma_ahb"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_SAR_ADC_BUS,
			 S32GEN1_CLK_PER, "sar_adc_bus"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_CMU_MODULE,
			 S32GEN1_CLK_XBAR_DIV6, "cmu_module"),
	SCMI_ARRAY_ENTRY(S32GEN1_SCMI_CLK_CMU_REG,
			 S32GEN1_CLK_XBAR_DIV6, "cmu_reg"),
};

static int compound2clkid(uint32_t scmi_clk_id, uint32_t *clk_id)
{
	switch (scmi_clk_id) {
	case S32GEN1_SCMI_CLK_GMAC0_RX_SGMII:
	case S32GEN1_SCMI_CLK_GMAC0_RX_RGMII:
		if (clk_id)
			*clk_id = S32GEN1_CLK_GMAC0_RX;
		break;
	case S32GEN1_SCMI_CLK_GMAC0_TX_SGMII:
	case S32GEN1_SCMI_CLK_GMAC0_TX_RGMII:
		if (clk_id)
			*clk_id = S32GEN1_CLK_GMAC0_TX;
		break;
	case S32GEN1_SCMI_CLK_GMAC0_TS:
		if (clk_id)
			*clk_id = S32GEN1_CLK_GMAC0_TS;
		break;
	case S32GEN1_SCMI_CLK_SERDES_PAD_REF:
	case S32GEN1_SCMI_CLK_GMAC0_RX_RMII:
	case S32GEN1_SCMI_CLK_GMAC0_TX_RMII:
	case S32GEN1_SCMI_CLK_GMAC0_RX_MII:
	case S32GEN1_SCMI_CLK_GMAC0_TX_MII:
		if (clk_id)
			*clk_id = S32GEN1_SCMI_NOT_IMPLEMENTED_CLK;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int cc_set_mux_parent(struct clk *clk, uint32_t mux_id, uint32_t mux_source)
{
	struct clk source = *clk;
	struct clk mux = *clk;
	int ret;

	source.id = mux_source;
	mux.id = mux_id;

	ret = s32gen1_set_parent(&mux, &source);
	if (ret) {
		ERROR("Failed to set cgm0_mux11 source\n");
		return -EINVAL;
	}

	return 0;
}

static int set_gmac_rx_parent(struct clk *clk)
{
	uint32_t rx_id, parent_id;
	uint32_t clk_id = clk->id;

	if (clk_id == S32GEN1_SCMI_CLK_GMAC0_RX_SGMII) {
		rx_id = S32GEN1_CLK_SERDES0_LANE0_CDR;
	} else if (clk_id == S32GEN1_SCMI_CLK_GMAC0_RX_RGMII) {
		rx_id = S32GEN1_CLK_GMAC0_EXT_RX;
	} else {
		ERROR("Invalid GMAC RX mode\n");
		return -EINVAL;
	}

	if (cc_compound_clk_get_pid(clk_id, &parent_id))
		return -EINVAL;

	return cc_set_mux_parent(clk, parent_id, rx_id);
}

static int set_gmac_tx_parent(struct clk *clk)
{
	uint32_t tx_id, parent_id;
	uint32_t clk_id = clk->id;

	if (clk_id == S32GEN1_SCMI_CLK_GMAC0_TX_RGMII) {
		tx_id = S32GEN1_CLK_PERIPH_PLL_PHI5;
	} else if (clk_id == S32GEN1_SCMI_CLK_GMAC0_TX_SGMII) {
		tx_id = S32GEN1_CLK_SERDES0_LANE0_TX;
	} else {
		ERROR("Invalid GMAC TX mode\n");
		return -EINVAL;
	}

	if (cc_compound_clk_get_pid(clk_id, &parent_id))
		return -EINVAL;

	return cc_set_mux_parent(clk, parent_id, tx_id);
}

static int set_gmac_ts_parent(struct clk *clk)
{
	uint32_t ts_id, parent_id;
	uint32_t clk_id = clk->id;

	if (clk_id == S32GEN1_SCMI_CLK_GMAC0_TS) {
		ts_id = S32GEN1_CLK_PERIPH_PLL_PHI4;
	} else {
		ERROR("Invalid GMAC TS mode\n");
		return -EINVAL;
	}

	if (cc_compound_clk_get_pid(clk_id, &parent_id))
		return -EINVAL;

	return cc_set_mux_parent(clk, parent_id, ts_id);
}

static int cc_compound_clk_set_parents(struct clk *clk)
{
	uint32_t clk_id = clk->id;
	uint32_t id;

	if (compound2clkid(clk_id, &id)) {
		ERROR("Invalid compound clock : %u\n", clk_id);
		return -EINVAL;
	}

	switch (id) {
	case S32GEN1_CLK_GMAC0_RX:
		return set_gmac_rx_parent(clk);
	case S32GEN1_CLK_GMAC0_TX:
		return set_gmac_tx_parent(clk);
	case S32GEN1_CLK_GMAC0_TS:
		return set_gmac_ts_parent(clk);
	case S32GEN1_SCMI_NOT_IMPLEMENTED_CLK:
		return 0;
	default:
		return plat_compound_clk_set_parents(clk);
	}
}

int cc_scmi_id2clk(uint32_t scmi_clk_id, uint32_t *clk_id)
{
	if (!clk_id)
		return -EINVAL;

	if (scmi_clk_id >= S32GEN1_SCMI_PLAT_CLK_BASE_ID)
		return plat_scmi_id2clk(scmi_clk_id, clk_id);

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(cc_scmi_clk))
		return -EINVAL;

	*clk_id = cc_scmi_clk[INDEX(scmi_clk_id)].plat_id;
	if (!*clk_id) {
		ERROR("Unhandled S32GEN1 clock: %u\n", scmi_clk_id);
		return -EINVAL;
	}
	return 0;
}

int cc_compound_clk_get(struct clk *clk)
{
	uint32_t scmi_clk_id = clk->id;

	if (scmi_clk_id >= S32GEN1_SCMI_PLAT_CLK_BASE_ID)
		return plat_compound_clk_get(clk);

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(cc_scmi_clk))
		return -EINVAL;

	if (compound2clkid(scmi_clk_id, NULL)) {
		ERROR("Invalid compound clock : %u\n", scmi_clk_id);
		return -EINVAL;
	}

	return 0;
}

unsigned long cc_compound_clk_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk sclock = *clk;
	uint32_t scmi_clk_id = clk->id;
	uint32_t id;
	int ret;

	if (scmi_clk_id >= S32GEN1_SCMI_PLAT_CLK_BASE_ID)
		return plat_compound_clk_set_rate(clk, rate);

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(cc_scmi_clk))
		return -EINVAL;

	ret = cc_compound_clk_set_parents(clk);
	if (ret) {
		ERROR("Failed to set parents for %u\n", scmi_clk_id);
		return -EINVAL;
	}

	if (compound2clkid(scmi_clk_id, &id)) {
		ERROR("Invalid compound clock : %u\n", scmi_clk_id);
		return -EINVAL;
	}

	if (id == S32GEN1_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
		      cc_scmi_clk[INDEX(scmi_clk_id)].name);
		return 0;
	}

	sclock.id = id;
	return s32gen1_set_rate(&sclock, rate);
}

int cc_compound_clk_enable(struct clk *clk, int enable)
{
	struct clk sclock = *clk;
	uint32_t clk_id = clk->id;
	uint32_t id;
	int ret;

	if (clk_id >= S32GEN1_SCMI_PLAT_CLK_BASE_ID)
		return plat_compound_clk_enable(clk, enable);

	if (compound2clkid(clk_id, &id)) {
		ERROR("Invalid compound clock : %u\n", clk_id);
		return -EINVAL;
	}

	if (id == S32GEN1_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
		      cc_scmi_clk[INDEX(clk_id)].name);
		return -EINVAL;
	}

	ret = cc_compound_clk_set_parents(clk);
	if (ret) {
		ERROR("Failed to set parents for %u\n", clk_id);
		return -EINVAL;
	}

	sclock.id = id;
	ret = s32gen1_enable(&sclock, enable);
	if (ret) {
		ERROR("Failed to enable %u clock\n", clk_id);
		return ret;
	}

	return 0;
}

unsigned long cc_compound_clk_get_rate(struct clk *clk)
{
	struct clk sclock = *clk;
	uint32_t scmi_clk_id = clk->id;
	uint32_t id;

	if (scmi_clk_id >= S32GEN1_SCMI_PLAT_CLK_BASE_ID)
		return plat_compound_clk_get_rate(clk);

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(cc_scmi_clk))
		return 0;

	if (compound2clkid(scmi_clk_id, &id)) {
		ERROR("Invalid compound clock : %u\n", scmi_clk_id);
		return 0;
	}

	if (id == S32GEN1_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
		      cc_scmi_clk[INDEX(scmi_clk_id)].name);
		return 0;
	}

	sclock.id = id;
	return s32gen1_get_rate(&sclock);
}

uint32_t cc_get_nclocks(void)
{
	return plat_get_nclocks();
}

const char *cc_scmi_clk_get_name(uint32_t scmi_clk_id)
{
	if (scmi_clk_id < S32GEN1_SCMI_CLK_BASE_ID)
		return NULL;

	if (scmi_clk_id >= S32GEN1_SCMI_PLAT_CLK_BASE_ID)
		return plat_scmi_clk_get_name(scmi_clk_id);

	return cc_scmi_clk[INDEX(scmi_clk_id)].name;
}

int cc_scmi_clk_get_rates(struct clk *clk, unsigned long *rates,
			  size_t *nrates)
{
	struct clk sclock = *clk;
	uint32_t scmi_clk_id = clk->id;
	uint32_t id;
	int ret;

	if (scmi_clk_id >= S32GEN1_SCMI_PLAT_CLK_BASE_ID)
		return plat_scmi_clk_get_rates(clk, rates, nrates);

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(cc_scmi_clk))
		return -EINVAL;

	ret = cc_compound_clk_set_parents(clk);
	if (ret) {
		ERROR("Failed to set parents for %u\n", scmi_clk_id);
		return -EINVAL;
	}

	if (compound2clkid(scmi_clk_id, &id)) {
		ERROR("Invalid compound clock : %u\n", scmi_clk_id);
		return -EINVAL;
	}

	if (id == S32GEN1_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
		      cc_scmi_clk[INDEX(scmi_clk_id)].name);
		return -EINVAL;
	}

	sclock.id = id;
	rates[0] = s32gen1_get_minrate(&sclock);
	rates[1] = s32gen1_get_maxrate(&sclock);

	return 0;
}

