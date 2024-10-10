// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_scmi_clk.h>
#include <dt-bindings/clock/s32gen1-clock.h>
#include <dt-bindings/reset/s32cc-scmi-reset.h>
#include <drivers/scmi-msg.h>
#include <s32_bl_common.h>
#include <s32cc_clocks.h>

#define CLK_INIT(ID)          \
{                             \
	.id = (ID),           \
	.drv = &fake_clk_dev, \
}

#define ARM_DFS_BASE_ADDR		S32_DFS_ADDR(S32_CORE_DFS)
#define PERIPH_DFS_BASE_ADDR	S32_DFS_ADDR(S32_PERIPH_DFS)

static struct s32gen1_clk_priv s32_priv = {
	.armdfs = (void *)ARM_DFS_BASE_ADDR,
	.armpll = (void *)ARM_PLL_BASE_ADDR,
	.cgm0 = (void *)MC_CGM0_BASE_ADDR,
	.cgm1 = (void *)MC_CGM1_BASE_ADDR,
	.cgm5 = (void *)MC_CGM5_BASE_ADDR,
	.ddrpll = (void *)DRAM_PLL_BASE_ADDR,
	.fxosc = (void *)S32_FXOSC_BASE_ADDR,
	.mc_me = (void *)MC_ME_BASE_ADDR,
	.periphdfs = (void *)PERIPH_DFS_BASE_ADDR,
	.periphpll = (void *)PERIPH_PLL_BASE_ADDR,
	.rdc = (void *)RDC_BASE_ADDR,
	.rgm = (void *)MC_RGM_BASE_ADDR,
};

static struct clk_driver fake_clk_dev = {
	.data = &s32_priv,
};

/* A53 clocks */
static struct clk fxosc = CLK_INIT(S32GEN1_CLK_FXOSC);
static struct clk arm_pll_mux = CLK_INIT(S32GEN1_CLK_ARM_PLL_MUX);
static struct clk arm_pll_vco = CLK_INIT(S32GEN1_CLK_ARM_PLL_VCO);
static struct clk arm_pll_phi0 = CLK_INIT(S32GEN1_CLK_ARM_PLL_PHI0);
static struct clk mc_cgm1_mux0 = CLK_INIT(S32GEN1_CLK_MC_CGM1_MUX0);
static struct clk a53_clk = CLK_INIT(S32GEN1_CLK_A53_CORE);

/* XBAR clock */
static struct clk firc = CLK_INIT(S32GEN1_CLK_FIRC);
static struct clk arm_dfs1 = CLK_INIT(S32GEN1_CLK_ARM_PLL_DFS1);
static struct clk mc_cgm0_mux0 = CLK_INIT(S32GEN1_CLK_MC_CGM0_MUX0);
static struct clk xbar_2x = CLK_INIT(S32GEN1_CLK_XBAR_2X);

/* LINFLEX clock */
static struct clk periph_pll_mux = CLK_INIT(S32GEN1_CLK_PERIPH_PLL_MUX);
static struct clk periph_pll_vco = CLK_INIT(S32GEN1_CLK_PERIPH_PLL_VCO);
static struct clk periph_pll_phi3 = CLK_INIT(S32GEN1_CLK_PERIPH_PLL_PHI3);
static struct clk mc_cgm0_mux8 = CLK_INIT(S32GEN1_CLK_MC_CGM0_MUX8);
static struct clk lin_baud = CLK_INIT(S32GEN1_CLK_LIN_BAUD);

/* SDHC */
static struct clk periph_dfs3 = CLK_INIT(S32GEN1_CLK_PERIPH_PLL_DFS3);
static struct clk mc_cgm0_mux14 = CLK_INIT(S32GEN1_CLK_MC_CGM0_MUX14);
static struct clk sdhc = CLK_INIT(S32GEN1_CLK_SDHC);

/* QSPI */
static struct clk periph_dfs1 = CLK_INIT(S32GEN1_CLK_PERIPH_PLL_DFS1);
static struct clk mc_cgm0_mux12 = CLK_INIT(S32GEN1_CLK_MC_CGM0_MUX12);
static struct clk qspi = CLK_INIT(S32GEN1_CLK_QSPI);

/* DDR clock */
static struct clk ddr_pll_mux = CLK_INIT(S32GEN1_CLK_DDR_PLL_MUX);
static struct clk ddr_pll_vco = CLK_INIT(S32GEN1_CLK_DDR_PLL_VCO);
static struct clk ddr_pll_phi0 = CLK_INIT(S32GEN1_CLK_DDR_PLL_PHI0);
static struct clk mc_cgm5_mux0 = CLK_INIT(S32GEN1_CLK_MC_CGM5_MUX0);
static struct clk ddr = CLK_INIT(S32GEN1_CLK_DDR);

static struct siul2_freq_mapping early_freqs;

static int switch_xbar_to_firc(void)
{
	int ret;

	ret = s32gen1_set_parent(&mc_cgm0_mux0, &firc);
	if (ret)
		return ret;

	ret = s32gen1_enable(&xbar_2x, 1);
	if (ret)
		return ret;

	/* Disable the clock to make it available for the next enablements */
	return s32gen1_enable(&xbar_2x, 0);
}

static int enable_xbar_clock(void)
{
	int ret;
	unsigned long rate;

	ret = s32gen1_set_parent(&mc_cgm0_mux0, &arm_dfs1);
	if (ret)
		return ret;

	rate = s32gen1_set_rate(&xbar_2x,
				early_freqs.xbar_2x_freq);
	if (rate != early_freqs.xbar_2x_freq)
		return -EINVAL;

	return s32gen1_enable(&xbar_2x, 1);
}

static int s32_enable_a53_clock(void)
{
	int ret;
	unsigned long rate;

	ret = s32gen1_get_early_clks_freqs(&early_freqs);
	if (ret)
		return ret;

	ret = s32gen1_set_parent(&arm_pll_mux, &fxosc);
	if (ret)
		return ret;

	ret = s32gen1_set_parent(&mc_cgm1_mux0, &arm_pll_phi0);
	if (ret)
		return ret;

	rate = s32gen1_set_rate(&fxosc, S32GEN1_FXOSC_FREQ);
	if (rate != S32GEN1_FXOSC_FREQ)
		return -EINVAL;

	rate = s32gen1_set_rate(&arm_pll_vco,
				early_freqs.arm_pll_vco_freq);
	if (rate != early_freqs.arm_pll_vco_freq)
		return -EINVAL;

	rate = s32gen1_set_rate(&arm_pll_phi0,
				early_freqs.arm_pll_phi0_freq);
	if (rate != early_freqs.arm_pll_phi0_freq)
		return -EINVAL;

	rate = s32gen1_set_rate(&a53_clk,
				early_freqs.a53_freq);
	if (rate != early_freqs.a53_freq)
		return -EINVAL;

	ret = s32gen1_enable(&a53_clk, 1);
	if (ret)
		return ret;

	return enable_xbar_clock();
}

int s32_a53_clock_setup(void)
{
	int ret;

	ret = mmap_clk_regions(&s32_priv);
	if (ret)
		return ret;

	return s32_enable_a53_clock();
}

static int enable_lin_clock(void)
{
	int ret;
	unsigned long rate;

	ret = s32gen1_set_parent(&periph_pll_mux, &fxosc);
	if (ret)
		return ret;

	ret = s32gen1_set_parent(&mc_cgm0_mux8, &periph_pll_phi3);
	if (ret)
		return ret;

	rate = s32gen1_set_rate(&periph_pll_vco,
				S32GEN1_PERIPH_PLL_VCO_FREQ);
	if (rate != S32GEN1_PERIPH_PLL_VCO_FREQ)
		return -EINVAL;

	rate = s32gen1_set_rate(&lin_baud, S32GEN1_LIN_BAUD_CLK_FREQ);
	if (rate != S32GEN1_LIN_BAUD_CLK_FREQ)
		return -EINVAL;

	return s32gen1_enable(&lin_baud, 1);
}

static int setup_periph_pll(void)
{
	int ret;
	unsigned long rate;

	ret = s32gen1_set_parent(&periph_pll_mux, &fxosc);
	if (ret)
		return ret;

	rate = s32gen1_set_rate(&periph_pll_vco,
				S32GEN1_PERIPH_PLL_VCO_FREQ);
	if (rate != S32GEN1_PERIPH_PLL_VCO_FREQ)
		return -EINVAL;

	return 0;
}

static int enable_sdhc_clock(void)
{
	int ret;
	unsigned long rate;
	unsigned long sdhc_clk_freq = get_sdhc_clk_freq();

	rate = s32gen1_set_rate(&periph_dfs3, S32GEN1_PERIPH_DFS3_FREQ);
	if (rate != S32GEN1_PERIPH_DFS3_FREQ)
		return -EINVAL;

	ret = s32gen1_set_parent(&mc_cgm0_mux14, &periph_dfs3);
	if (ret)
		return ret;

	rate = s32gen1_set_rate(&sdhc, sdhc_clk_freq);
	if (rate != sdhc_clk_freq)
		return -EINVAL;

	return s32gen1_enable(&sdhc, 1);
}

static int enable_qspi_clock(void)
{
	int ret;
	unsigned long rate;

	rate = s32gen1_set_rate(&periph_dfs1, S32GEN1_PERIPH_DFS1_FREQ);
	if (rate != S32GEN1_PERIPH_DFS1_FREQ)
		return -EINVAL;

	ret = s32gen1_set_parent(&mc_cgm0_mux12, &periph_dfs1);
	if (ret)
		return ret;

	rate = s32gen1_set_rate(&qspi, S32GEN1_QSPI_CLK_FREQ);
	if (rate != S32GEN1_QSPI_CLK_FREQ)
		return -EINVAL;

	return s32gen1_enable(&qspi, 1);
}

static int s32_set_ddr_clock_state(int enable)
{
	int ret;
	unsigned long rate;

	ret = s32gen1_set_parent(&ddr_pll_mux, &fxosc);
	if (ret)
		return ret;

	ret = s32gen1_set_parent(&mc_cgm5_mux0, &ddr_pll_phi0);
	if (ret)
		return ret;

	rate = s32gen1_set_rate(&ddr_pll_vco, S32GEN1_DDR_PLL_VCO_FREQ);
	if (rate != S32GEN1_DDR_PLL_VCO_FREQ)
		return -EINVAL;

	rate = s32gen1_set_rate(&ddr, S32GEN1_DDR_FREQ);
	if (rate != S32GEN1_DDR_FREQ)
		return -EINVAL;

	return s32gen1_enable(&ddr, enable);
}

int s32_enable_ddr_clock(void)
{
	return s32_set_ddr_clock_state(1);
}

int s32_reset_ddr_periph(void)
{
	int ret;

	ret = plat_scmi_rstd_set_state(0, S32CC_SCMI_RST_DDR, false);
	if (ret) {
		ERROR("Failed to deassert DDR reset (%d)\n", ret);
		return ret;
	}

	return ret;
}

int s32_a53_clock_early_setup(void)
{
	int ret;

	init_fake_plat_driver(&s32_priv, S32GEN1_CLK_DRV_NAME);

	ret = switch_xbar_to_firc();
	if (ret)
		return ret;

	return s32_a53_clock_setup();
}

int s32_periph_clock_init(void)
{
	int ret;

	if (is_scp_used())
		return 0;

	ret = mmap_clk_regions(&s32_priv);
	if (ret)
		return ret;

	ret = enable_lin_clock();
	if (ret)
		return ret;

	ret = setup_periph_pll();
	if (ret)
		return ret;

	if (fip_location_mmc) {
		ret = enable_sdhc_clock();
		if (ret)
			return ret;
	} else if (fip_location_qspi) {
		ret = enable_qspi_clock();
		if (ret)
			return ret;
	}

	return s32_enable_ddr_clock();
}
