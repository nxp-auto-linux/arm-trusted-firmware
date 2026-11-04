// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2026 NXP
 */
#include <dt-bindings/clock/s32cc-clock.h>
#include <dt-bindings/clock/s32cc-scmi-clock.h>
#include <clk/s32cc_clk_funcs.h>
#include <clk/s32cc_clk_modules.h>
#include <clk/s32cc_shared_clks.h>
#include <errno.h>

/*
 * GMAC0 clocks of the SoCs that drive GMAC0 from MC_CGM0 (S32G2 and S32R45).
 * S32G3 drives GMAC0 from MC_CGM6 and defines its own GMAC0 clocks.
 */

/* GMAC_TS_CLK */
static struct s32cc_fixed_clock gmac_ext_ts =
		S32CC_FIXED_CLK_INIT();
static struct s32cc_clk gmac_ext_ts_clk =
		S32CC_MODULE_CLK(gmac_ext_ts);
static struct s32cc_mux cgm0_mux9 =
		S32CC_MUX_INIT(S32CC_CGM0, 9, 3,
			       S32CC_CLK_FIRC,
			       S32CC_CLK_PERIPH_PLL_PHI4,
			       S32CC_CLK_GMAC0_EXT_TS);
static struct s32cc_clk cgm0_mux9_clk =
		S32CC_MODULE_CLK(cgm0_mux9);
static struct s32cc_cgm_div gmac_ts_div =
		S32CC_CGM_DIV_INIT(cgm0_mux9_clk, 0);
struct s32cc_clk gmac_ts_clk =
		S32CC_FREQ_MODULE_CLK(gmac_ts_div, 5 * MHZ, 200 * MHZ);

/* GMAC0_TX_CLK */
static struct s32cc_fixed_clock gmac0_ext_tx =
		S32CC_FIXED_CLK_INIT();
static struct s32cc_clk gmac0_ext_tx_clk =
		S32CC_MODULE_CLK(gmac0_ext_tx);

static struct s32cc_fixed_clock gmac0_rmii_ref =
		S32CC_FIXED_CLK_INIT();
static struct s32cc_clk gmac0_rmii_ref_clk =
		S32CC_MODULE_CLK(gmac0_rmii_ref);

static struct s32cc_mux cgm0_mux10 =
		S32CC_MUX_INIT(S32CC_CGM0, 10, 5,
			       S32CC_CLK_FIRC,
			       S32CC_CLK_PERIPH_PLL_PHI5,
			       S32CC_CLK_SERDES0_LANE0_TX,
			       S32CC_CLK_GMAC0_EXT_TX,
			       S32CC_CLK_GMAC0_RMII_REF);
static struct s32cc_clk cgm0_mux10_clk =
		S32CC_MODULE_CLK(cgm0_mux10);
static struct s32cc_cgm_div gmac_tx_div =
		S32CC_CGM_DIV_INIT(cgm0_mux10_clk, 0);
static struct s32cc_clk gmac_tx_clk =
		S32CC_FREQ_MODULE_CLK(gmac_tx_div, 2500000, 125 * MHZ);

/* GMAC0_RX_CLK */
static struct s32cc_fixed_clock gmac0_ext_rx =
		S32CC_FIXED_CLK_INIT();
static struct s32cc_clk gmac0_ext_rx_clk =
		S32CC_MODULE_CLK(gmac0_ext_rx);

static struct s32cc_mux cgm0_mux11 =
		S32CC_MUX_INIT(S32CC_CGM0, 11, 4,
			       S32CC_CLK_FIRC,
			       S32CC_CLK_GMAC0_REF_DIV,
			       S32CC_CLK_GMAC0_EXT_RX,
			       S32CC_CLK_SERDES0_LANE0_CDR);
static struct s32cc_clk cgm0_mux11_clk =
		S32CC_MODULE_CLK(cgm0_mux11);
static struct s32cc_clk gmac_rx_clk =
		S32CC_CHILD_CLK(cgm0_mux11_clk, 2500000, 125 * MHZ);

/* GMAC0_REF_DIV_CLK */
static struct s32cc_mux cgm0_mux15 =
		S32CC_SHARED_MUX_INIT(S32CC_CGM0, 15, 2,
				      S32CC_CLK_FIRC,
				      S32CC_CLK_GMAC0_RMII_REF);
static struct s32cc_clk cgm0_mux15_clk =
		S32CC_MODULE_CLK(cgm0_mux15);
static struct s32cc_cgm_div gmac0_ref_div =
		S32CC_CGM_DIV_INIT(cgm0_mux15_clk, 0);
static struct s32cc_clk gmac0_ref_clk =
		S32CC_FREQ_MODULE_CLK(gmac0_ref_div, 0, 50 * MHZ);

static const struct s32cc_clk_entry s32cc_cgm0_gmac0_entries[] = {
	{ S32CC_CLK_GMAC0_EXT_TS, &gmac_ext_ts_clk },
	{ S32CC_CLK_MC_CGM0_MUX9, &cgm0_mux9_clk },
	{ S32CC_CLK_GMAC0_TS, &gmac_ts_clk },
	{ S32CC_CLK_GMAC0_EXT_TX, &gmac0_ext_tx_clk },
	{ S32CC_CLK_GMAC0_RMII_REF, &gmac0_rmii_ref_clk },
	{ S32CC_CLK_MC_CGM0_MUX10, &cgm0_mux10_clk },
	{ S32CC_CLK_GMAC0_TX, &gmac_tx_clk },
	{ S32CC_CLK_GMAC0_EXT_RX, &gmac0_ext_rx_clk },
	{ S32CC_CLK_MC_CGM0_MUX11, &cgm0_mux11_clk },
	{ S32CC_CLK_GMAC0_RX, &gmac_rx_clk },
	{ S32CC_CLK_MC_CGM0_MUX15, &cgm0_mux15_clk },
	{ S32CC_CLK_GMAC0_REF_DIV, &gmac0_ref_clk },
	{ S32CC_CLK_GMAC0_REF, &gmac0_ref_clk },
};
const struct s32cc_clk_group s32cc_cgm0_gmac0_clks =
		S32CC_CLK_GROUP(s32cc_cgm0_gmac0_entries);

int cc_compound_clk_get_pid(uint32_t id, uint32_t *parent_id)
{
	if (!parent_id)
		return -EINVAL;

	switch (id) {
	case S32CC_SCMI_CLK_GMAC0_RX_SGMII:
	case S32CC_SCMI_CLK_GMAC0_RX_RGMII:
	case S32CC_SCMI_CLK_GMAC0_RX_RMII:
	case S32CC_SCMI_CLK_GMAC0_RX_MII:
		*parent_id = S32CC_CLK_MC_CGM0_MUX11;
		break;
	case S32CC_SCMI_CLK_GMAC0_TX_SGMII:
	case S32CC_SCMI_CLK_GMAC0_TX_RGMII:
	case S32CC_SCMI_CLK_GMAC0_TX_RMII:
	case S32CC_SCMI_CLK_GMAC0_TX_MII:
		*parent_id = S32CC_CLK_MC_CGM0_MUX10;
		break;
	case S32CC_SCMI_CLK_GMAC0_TS:
		*parent_id = S32CC_CLK_MC_CGM0_MUX9;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
