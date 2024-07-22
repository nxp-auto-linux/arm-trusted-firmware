// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021, 2023-2024 NXP
 */
#include <dt-bindings/clock/s32gen1-clock.h>
#include <dt-bindings/clock/s32cc-scmi-clock.h>
#include <dt-bindings/clock/s32g3-clock.h>
#include <dt-bindings/clock/s32gen1-clock-freq.h>
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_clk_modules.h>

#define SIUL2_MIDR2_FREQ_VAL1		(0xB)
#define SIUL2_MIDR2_FREQ_VAL2		(0xC)
#define SIUL2_MIDR2_FREQ_VAL3		(0xE)
#define SIUL2_MIDR2_FREQ_VAL4		(0xF)

#define S32GEN1_A53_1_3_GHZ_FREQ		(1300 * MHZ)
#define S32GEN1_A53_1_1_GHZ_FREQ		(1100 * MHZ)
#define S32GEN1_A53_1_0_GHZ_FREQ		(1000 * MHZ)

#define S32GEN1_ARM_PLL_VCO_1_3_GHZ_FREQ	(2600 * MHZ)
#define S32GEN1_ARM_PLL_VCO_1_1_GHZ_FREQ	(2200 * MHZ)
#define S32GEN1_ARM_PLL_VCO_1_0_GHZ_FREQ	(2000 * MHZ)

#define S32GEN1_ARM_PLL_PHI0_1_3_GHZ_FREQ	(1300 * MHZ)
#define S32GEN1_ARM_PLL_PHI0_1_1_GHZ_FREQ	(1100 * MHZ)
#define S32GEN1_ARM_PLL_PHI0_1_0_GHZ_FREQ	(1000 * MHZ)

/* The XBAR_2X naming convention corresponds to the
 * A53/VCO/PHI0 mapped frequencies, not to the order
 * of the actual values
 */
#define S32GEN1_XBAR_2X_1_4_GHZ_FREQ	(800 * MHZ)
#define S32GEN1_XBAR_2X_1_3_GHZ_FREQ	(793236859UL)
#define S32GEN1_XBAR_2X_1_1_GHZ_FREQ	(792007785UL)
#define S32GEN1_XBAR_2X_1_0_GHZ_FREQ	(800 * MHZ)

/* GMAC_TS_CLK */
static struct s32gen1_fixed_clock gmac_ext_ts =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac_ext_ts_clk =
		S32GEN1_MODULE_CLK(gmac_ext_ts);
static struct s32gen1_mux cgm6_mux0 =
		S32GEN1_MUX_INIT(S32GEN1_CGM6, 0, 3,
				 S32GEN1_CLK_FIRC,
				 S32GEN1_CLK_PERIPH_PLL_PHI4,
				 S32GEN1_CLK_GMAC0_EXT_TS);
static struct s32gen1_clk cgm6_mux0_clk =
		S32GEN1_MODULE_CLK(cgm6_mux0);
static struct s32gen1_cgm_div gmac_ts_div =
		S32GEN1_CGM_DIV_INIT(cgm6_mux0_clk, 0);
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

static struct s32gen1_mux cgm6_mux1 =
		S32GEN1_MUX_INIT(S32GEN1_CGM6, 1, 5,
				 S32GEN1_CLK_FIRC,
				 S32GEN1_CLK_PERIPH_PLL_PHI5,
				 S32GEN1_CLK_SERDES0_LANE0_TX,
				 S32GEN1_CLK_GMAC0_EXT_TX,
				 S32GEN1_CLK_GMAC0_RMII_REF);
static struct s32gen1_clk cgm6_mux1_clk =
		S32GEN1_MODULE_CLK(cgm6_mux1);
static struct s32gen1_cgm_div gmac_tx_div =
		S32GEN1_CGM_DIV_INIT(cgm6_mux1_clk, 0);
static struct s32gen1_clk gmac_tx_clk =
		S32GEN1_FREQ_MODULE_CLK(gmac_tx_div, 2500000, 125 * MHZ);

/* GMAC0_RX_CLK */
static struct s32gen1_fixed_clock gmac0_ext_rx =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac0_ext_rx_clk =
		S32GEN1_MODULE_CLK(gmac0_ext_rx);

static struct s32gen1_mux cgm6_mux2 =
		S32GEN1_MUX_INIT(S32GEN1_CGM6, 2, 4,
				 S32GEN1_CLK_FIRC,
				 S32GEN1_CLK_GMAC0_REF_DIV,
				 S32GEN1_CLK_GMAC0_EXT_RX,
				 S32GEN1_CLK_SERDES0_LANE0_CDR);
static struct s32gen1_clk cgm6_mux2_clk =
		S32GEN1_MODULE_CLK(cgm6_mux2);
static struct s32gen1_clk gmac_rx_clk =
		S32GEN1_CHILD_CLK(cgm6_mux2_clk, 2500000, 125 * MHZ);

/* GMAC0_REF_DIV_CLK */
static struct s32gen1_mux cgm6_mux3 =
		S32GEN1_SHARED_MUX_INIT(S32GEN1_CGM6, 3, 2,
					S32GEN1_CLK_FIRC,
					S32GEN1_CLK_GMAC0_RMII_REF);
static struct s32gen1_clk cgm6_mux3_clk =
		S32GEN1_MODULE_CLK(cgm6_mux3);
static struct s32gen1_cgm_div gmac0_ref_div =
		S32GEN1_CGM_DIV_INIT(cgm6_mux3_clk, 0);
static struct s32gen1_clk gmac0_ref_clk =
		S32GEN1_FREQ_MODULE_CLK(gmac0_ref_div, 0, 50 * MHZ);

static struct s32gen1_clk *s32g3_clocks[] = {
	[ARR_CLK(S32G_CLK_MC_CGM6_MUX0)] = &cgm6_mux0_clk,
	[ARR_CLK(S32G_CLK_MC_CGM6_MUX1)] = &cgm6_mux1_clk,
	[ARR_CLK(S32G_CLK_MC_CGM6_MUX2)] = &cgm6_mux2_clk,
	[ARR_CLK(S32G_CLK_MC_CGM6_MUX3)] = &cgm6_mux3_clk,
};

static struct s32gen1_clk *s32g3_cc_clocks[] = {
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_TX)] = &gmac_tx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_EXT_TS)] = &gmac_ext_ts_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_TS)] = &gmac_ts_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_EXT_TX)] = &gmac0_ext_tx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_RMII_REF)] = &gmac0_rmii_ref_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_EXT_RX)] = &gmac0_ext_rx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_RX)] = &gmac_rx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_REF_DIV)] = &gmac0_ref_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_REF)] = &gmac0_ref_clk,
};

struct s32gen1_clk *s32g_get_plat_cc_clock(uint32_t id)
{
	id = s32gen1_platclk2mux(id);

	if (id >= ARRAY_SIZE(s32g3_cc_clocks))
		return NULL;

	return s32g3_cc_clocks[id];
}

struct s32gen1_clk *s32g_get_plat_clock(uint32_t id)
{
	if (id < S32GEN1_PLAT_CLK_ID_BASE)
		return NULL;

	id -= S32GEN1_PLAT_CLK_ID_BASE;

	if (id >= ARRAY_SIZE(s32g3_clocks))
		return NULL;

	return s32g3_clocks[id];
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
		*parent_id = S32G_CLK_MC_CGM6_MUX2;
		break;
	case S32CC_SCMI_CLK_GMAC0_TX_SGMII:
	case S32CC_SCMI_CLK_GMAC0_TX_RGMII:
	case S32CC_SCMI_CLK_GMAC0_TX_RMII:
	case S32CC_SCMI_CLK_GMAC0_TX_MII:
		*parent_id = S32G_CLK_MC_CGM6_MUX1;
		break;
	case S32CC_SCMI_CLK_GMAC0_TS:
		*parent_id = S32G_CLK_MC_CGM6_MUX0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

const struct siul2_freq_mapping siul2_clk_freq_map[] = {

	SIUL2_FREQ_MAP(SIUL2_MIDR2_FREQ_VAL1, S32GEN1_A53_1_0_GHZ_FREQ,
			S32GEN1_ARM_PLL_VCO_1_0_GHZ_FREQ,
			S32GEN1_ARM_PLL_PHI0_1_0_GHZ_FREQ,
			S32GEN1_XBAR_2X_1_0_GHZ_FREQ),

	SIUL2_FREQ_MAP(SIUL2_MIDR2_FREQ_VAL2, S32GEN1_A53_1_1_GHZ_FREQ,
			S32GEN1_ARM_PLL_VCO_1_1_GHZ_FREQ,
			S32GEN1_ARM_PLL_PHI0_1_1_GHZ_FREQ,
			S32GEN1_XBAR_2X_1_1_GHZ_FREQ),

	SIUL2_FREQ_MAP(SIUL2_MIDR2_FREQ_VAL3, S32GEN1_A53_1_3_GHZ_FREQ,
			S32GEN1_ARM_PLL_VCO_1_3_GHZ_FREQ,
			S32GEN1_ARM_PLL_PHI0_1_3_GHZ_FREQ,
			S32GEN1_XBAR_2X_1_3_GHZ_FREQ),

	SIUL2_FREQ_MAP(SIUL2_MIDR2_FREQ_VAL4, S32GEN1_A53_MAX_FREQ,
			S32GEN1_ARM_PLL_VCO_MAX_FREQ,
			S32GEN1_ARM_PLL_PHI0_MAX_FREQ,
			S32GEN1_XBAR_2X_1_4_GHZ_FREQ),

	/* Test samples, let's assume it's S32G399A */
	SIUL2_FREQ_MAP(0x0, S32GEN1_A53_1_3_GHZ_FREQ,
			S32GEN1_ARM_PLL_VCO_1_3_GHZ_FREQ,
			S32GEN1_ARM_PLL_PHI0_1_3_GHZ_FREQ,
			S32GEN1_XBAR_2X_1_3_GHZ_FREQ),

	{} /* empty entry */
};
