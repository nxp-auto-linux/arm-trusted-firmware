// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021, 2023-2024 NXP
 */
#include <dt-bindings/clock/s32g-clock.h>
#include <dt-bindings/clock/s32cc-scmi-clock.h>
#include <dt-bindings/clock/s32gen1-clock-freq.h>
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_clk_modules.h>

#define SIUL2_MIDR2_FREQ_VAL1		(0xB)

#define S32GEN1_XBAR_2X_MAX_FREQ	(800 * MHZ)

/* GMAC_TS_CLK */
static struct s32gen1_fixed_clock gmac_ext_ts =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac_ext_ts_clk =
		S32GEN1_MODULE_CLK(gmac_ext_ts);
static struct s32gen1_mux cgm0_mux9 =
		S32GEN1_MUX_INIT(S32GEN1_CGM0, 9, 3,
				 S32GEN1_CLK_FIRC,
				 S32GEN1_CLK_PERIPH_PLL_PHI4,
				 S32GEN1_CLK_GMAC0_EXT_TS);
static struct s32gen1_clk cgm0_mux9_clk =
		S32GEN1_MODULE_CLK(cgm0_mux9);
static struct s32gen1_cgm_div gmac_ts_div =
		S32GEN1_CGM_DIV_INIT(cgm0_mux9_clk, 0);
struct s32gen1_clk gmac_ts_clk =
		S32GEN1_FREQ_MODULE_CLK(gmac_ts_div, 5 * MHZ, 200 * MHZ);

/* GMAC0_TX_CLK */
static struct s32gen1_fixed_clock gmac0_ext_tx =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac0_ext_tx_clk =
		S32GEN1_MODULE_CLK(gmac0_ext_tx);

static struct s32gen1_fixed_clock gmac0_rmii_ref =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac0_rmii_ref_clk =
		S32GEN1_MODULE_CLK(gmac0_rmii_ref);

static struct s32gen1_mux cgm0_mux10 =
		S32GEN1_MUX_INIT(S32GEN1_CGM0, 10, 5,
				 S32GEN1_CLK_FIRC,
				 S32GEN1_CLK_PERIPH_PLL_PHI5,
				 S32GEN1_CLK_SERDES0_LANE0_TX,
				 S32GEN1_CLK_GMAC0_EXT_TX,
				 S32GEN1_CLK_GMAC0_RMII_REF);
static struct s32gen1_clk cgm0_mux10_clk =
		S32GEN1_MODULE_CLK(cgm0_mux10);
static struct s32gen1_cgm_div gmac_tx_div =
		S32GEN1_CGM_DIV_INIT(cgm0_mux10_clk, 0);
static struct s32gen1_clk gmac_tx_clk =
		S32GEN1_FREQ_MODULE_CLK(gmac_tx_div, 2500000, 125 * MHZ);

/* GMAC0_RX_CLK */
static struct s32gen1_fixed_clock gmac0_ext_rx =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac0_ext_rx_clk =
		S32GEN1_MODULE_CLK(gmac0_ext_rx);

static struct s32gen1_mux cgm0_mux11 =
		S32GEN1_MUX_INIT(S32GEN1_CGM0, 11, 4,
				 S32GEN1_CLK_FIRC,
				 S32GEN1_CLK_GMAC0_REF_DIV,
				 S32GEN1_CLK_GMAC0_EXT_RX,
				 S32GEN1_CLK_SERDES0_LANE0_CDR);
static struct s32gen1_clk cgm0_mux11_clk =
		S32GEN1_MODULE_CLK(cgm0_mux11);
static struct s32gen1_clk gmac_rx_clk =
		S32GEN1_CHILD_CLK(cgm0_mux11_clk, 2500000, 125 * MHZ);

/* GMAC0_REF_DIV_CLK */
static struct s32gen1_mux cgm0_mux15 =
		S32GEN1_SHARED_MUX_INIT(S32GEN1_CGM0, 15, 2,
					S32GEN1_CLK_FIRC,
					S32GEN1_CLK_GMAC0_RMII_REF);
static struct s32gen1_clk cgm0_mux15_clk =
		S32GEN1_MODULE_CLK(cgm0_mux15);
static struct s32gen1_cgm_div gmac0_ref_div =
		S32GEN1_CGM_DIV_INIT(cgm0_mux15_clk, 0);
static struct s32gen1_clk gmac0_ref_clk =
		S32GEN1_FREQ_MODULE_CLK(gmac0_ref_div, 0, 50 * MHZ);

static struct s32gen1_clk *s32g274a_cc_clocks[] = {
	[CC_ARR_CLK(S32GEN1_CLK_MC_CGM0_MUX10)] = &cgm0_mux10_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_TX)] = &gmac_tx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_EXT_TS)] = &gmac_ext_ts_clk,
	[CC_ARR_CLK(S32GEN1_CLK_MC_CGM0_MUX9)] = &cgm0_mux9_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_TS)] = &gmac_ts_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_EXT_TX)] = &gmac0_ext_tx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_RMII_REF)] = &gmac0_rmii_ref_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_EXT_RX)] = &gmac0_ext_rx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_MC_CGM0_MUX11)] = &cgm0_mux11_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_RX)] = &gmac_rx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_MC_CGM0_MUX15)] = &cgm0_mux15_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_REF_DIV)] = &gmac0_ref_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_REF)] = &gmac0_ref_clk,
};

struct s32gen1_clk *s32g_get_plat_cc_clock(uint32_t id)
{
	id = s32gen1_platclk2mux(id);

	if (id >= ARRAY_SIZE(s32g274a_cc_clocks))
		return NULL;

	return s32g274a_cc_clocks[id];
}

struct s32gen1_clk *s32g_get_plat_clock(uint32_t id)
{
	return NULL;
}

int cc_compound_clk_get_pid(uint32_t id, uint32_t *parent_id)
{
	if (!parent_id)
		return -EINVAL;

	switch (id) {
	case S32CC_SCMI_CLK_GMAC0_RX_SGMII:
	case S32CC_SCMI_CLK_GMAC0_RX_RGMII:
	case S32CC_SCMI_CLK_GMAC0_RX_RMII:
	case S32CC_SCMI_CLK_GMAC0_RX_MII:
		*parent_id = S32GEN1_CLK_MC_CGM0_MUX11;
		break;
	case S32CC_SCMI_CLK_GMAC0_TX_SGMII:
	case S32CC_SCMI_CLK_GMAC0_TX_RGMII:
	case S32CC_SCMI_CLK_GMAC0_TX_RMII:
	case S32CC_SCMI_CLK_GMAC0_TX_MII:
		*parent_id = S32GEN1_CLK_MC_CGM0_MUX10;
		break;
	case S32CC_SCMI_CLK_GMAC0_TS:
		*parent_id = S32GEN1_CLK_MC_CGM0_MUX9;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

const struct siul2_freq_mapping siul2_clk_freq_map[] = {
	SIUL2_FREQ_MAP(SIUL2_MIDR2_FREQ_VAL1, S32GEN1_A53_MAX_FREQ,
			S32GEN1_ARM_PLL_VCO_MAX_FREQ,
			S32GEN1_ARM_PLL_PHI0_MAX_FREQ,
			S32GEN1_XBAR_2X_MAX_FREQ),
	{} /* empty entry */
};
