// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_scmi_clk.h>
#include <common/debug.h>
#include <dt-bindings/clock/s32g-clock.h>
#include <dt-bindings/clock/s32g-scmi-clock.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

#define INDEX(X)	((X) - S32CC_SCMI_PLAT_CLK_BASE_ID)

struct s32gen1_scmi_clk s32g_scmi_clk[] = {
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_USB_MEM,
			 S32GEN1_CLK_XBAR_DIV4, "usb_mem"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_USB_LOW,
			 S32GEN1_CLK_SIRC, "usb_low"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE_AXI,
			 S32G_CLK_PFE_SYS, "pfe_axi"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE_APB,
			 S32G_CLK_PFE_SYS, "pfe_apb"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE_TS,
			 S32G_CLK_PFE_TS, "pfe_ts"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE_PE,
			 S32G_CLK_PFE_PE, "pfe_pe"),
	/* PFE0 */
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE0_RX_SGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe0_rx_sgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE0_TX_SGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe0_tx_sgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE0_RX_RGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe0_rx_rgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE0_TX_RGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe0_tx_rgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE0_RX_RMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe0_rx_rmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE0_TX_RMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe0_tx_rmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE0_RX_MII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe0_rx_mii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE0_TX_MII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe0_tx_mii"),
	/* PFE1 */
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE1_RX_SGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe1_rx_sgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE1_TX_SGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe1_tx_sgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE1_RX_RGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe1_rx_rgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE1_TX_RGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe1_tx_rgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE1_RX_RMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe1_rx_rmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE1_TX_RMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe1_tx_rmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE1_RX_MII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe1_rx_mii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE1_TX_MII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe1_tx_mii"),
	/* PFE2 */
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE2_RX_SGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe2_rx_sgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE2_TX_SGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe2_tx_sgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE2_RX_RGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe2_rx_rgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE2_TX_RGMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe2_tx_rgmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE2_RX_RMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe2_rx_rmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE2_TX_RMII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe2_tx_rmii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE2_RX_MII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe2_rx_mii"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_PFE2_TX_MII,
			 S32CC_SCMI_COMPLEX_CLK, "pfe2_tx_mii"),
	/* LLCE */
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_LLCE_SYS,
			 S32G_CLK_LLCE_SYS, "llce_sys"),
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_LLCE_CAN_PE,
			 S32GEN1_CLK_CAN_PE, "llce_can_pe"),

	/* LPSPI */
	SCMI_ARRAY_ENTRY(S32G_SCMI_CLK_LLCE_PER,
			 S32GEN1_CLK_PER, "llce_per"),
};

static int s32g_compound2clkid(uint32_t scmi_clk_id, uint32_t *clk_id)
{
	switch (scmi_clk_id) {
	case S32G_SCMI_CLK_PFE0_RX_SGMII:
	case S32G_SCMI_CLK_PFE0_RX_RGMII:
	case S32G_SCMI_CLK_PFE0_RX_RMII:
	case S32G_SCMI_CLK_PFE0_RX_MII:
		if (clk_id)
			*clk_id = S32G_CLK_PFE_MAC0_RX;
		break;
	case S32G_SCMI_CLK_PFE0_TX_SGMII:
	case S32G_SCMI_CLK_PFE0_TX_RGMII:
	case S32G_SCMI_CLK_PFE0_TX_RMII:
	case S32G_SCMI_CLK_PFE0_TX_MII:
		if (clk_id)
			*clk_id = S32G_CLK_PFE_MAC0_TX_DIV;
		break;
	case S32G_SCMI_CLK_PFE1_RX_SGMII:
	case S32G_SCMI_CLK_PFE1_RX_RGMII:
	case S32G_SCMI_CLK_PFE1_RX_RMII:
	case S32G_SCMI_CLK_PFE1_RX_MII:
		if (clk_id)
			*clk_id = S32G_CLK_PFE_MAC1_RX;
		break;
	case S32G_SCMI_CLK_PFE1_TX_SGMII:
	case S32G_SCMI_CLK_PFE1_TX_RGMII:
	case S32G_SCMI_CLK_PFE1_TX_RMII:
	case S32G_SCMI_CLK_PFE1_TX_MII:
		if (clk_id)
			*clk_id = S32G_CLK_PFE_MAC1_TX;
		break;
	case S32G_SCMI_CLK_PFE2_RX_SGMII:
	case S32G_SCMI_CLK_PFE2_RX_RGMII:
	case S32G_SCMI_CLK_PFE2_RX_RMII:
	case S32G_SCMI_CLK_PFE2_RX_MII:
		if (clk_id)
			*clk_id = S32G_CLK_PFE_MAC2_RX;
		break;
	case S32G_SCMI_CLK_PFE2_TX_SGMII:
	case S32G_SCMI_CLK_PFE2_TX_RGMII:
	case S32G_SCMI_CLK_PFE2_TX_RMII:
	case S32G_SCMI_CLK_PFE2_TX_MII:
		if (clk_id)
			*clk_id = S32G_CLK_PFE_MAC2_TX;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int plat_scmi_id2clk(uint32_t scmi_clk_id, uint32_t *clk_id)
{
	if (!clk_id)
		return -EINVAL;

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32g_scmi_clk))
		return -EINVAL;

	*clk_id = s32g_scmi_clk[INDEX(scmi_clk_id)].plat_id;
	if (!*clk_id) {
		ERROR("Unhandled S32G clock: %" PRIu32 "\n", scmi_clk_id);
		return -EINVAL;
	}

	return 0;
}

int plat_compound_clk_get(struct clk *clk)
{
	uint32_t scmi_clk_id = clk->id;

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32g_scmi_clk))
		return -EINVAL;

	if (s32g_compound2clkid(scmi_clk_id, NULL)) {
		ERROR("Invalid S32G compound clock : %" PRIu32 "\n", scmi_clk_id);
		return -EINVAL;
	}

	return 0;
}

static int set_mac0_rx_parent(struct clk *clk)
{
	uint32_t rx_id;
	uint32_t clk_id = clk->id;
	int ret;

	if (clk_id == S32G_SCMI_CLK_PFE0_RX_SGMII) {
		rx_id = S32G_CLK_SERDES1_LANE0_CDR;
	} else if (clk_id == S32G_SCMI_CLK_PFE0_RX_RGMII ||
		   clk_id == S32G_SCMI_CLK_PFE0_RX_MII) {
		rx_id = S32G_CLK_PFE_MAC0_EXT_RX;
	} else if (clk_id == S32G_SCMI_CLK_PFE0_RX_RMII) {
		rx_id = S32G_CLK_PFE_MAC0_REF_DIV;
		ret = cc_set_mux_parent(clk, S32G_CLK_MC_CGM2_MUX7,
					S32G_CLK_PFE_MAC0_RMII_REF);
		if (ret)
			return ret;
	} else {
		ERROR("Invalid PFE0 RX mode\n");
		return -EINVAL;
	}

	return cc_set_mux_parent(clk, S32G_CLK_MC_CGM2_MUX4, rx_id);
}

static int set_mac0_tx_parent(struct clk *clk)
{
	uint32_t tx_id;
	uint32_t clk_id = clk->id;

	if (clk_id == S32G_SCMI_CLK_PFE0_TX_SGMII) {
		tx_id = S32G_CLK_SERDES1_LANE0_TX;
	} else if (clk_id == S32G_SCMI_CLK_PFE0_TX_RGMII) {
		tx_id = S32GEN1_CLK_PERIPH_PLL_PHI5;
	} else if (clk_id == S32G_SCMI_CLK_PFE0_TX_RMII) {
		tx_id = S32G_CLK_PFE_MAC0_RMII_REF;
	} else if (clk_id == S32G_SCMI_CLK_PFE0_TX_MII) {
		tx_id = S32G_CLK_PFE_MAC0_EXT_TX;
	} else {
		ERROR("Invalid PFE0 TX mode\n");
		return -EINVAL;
	}

	return cc_set_mux_parent(clk, S32G_CLK_MC_CGM2_MUX1, tx_id);
}

static int set_mac1_rx_parent(struct clk *clk)
{
	uint32_t rx_id;
	uint32_t clk_id = clk->id;
	int ret;

	if (clk_id == S32G_SCMI_CLK_PFE1_RX_SGMII) {
		rx_id = S32G_CLK_SERDES1_LANE1_CDR;
	} else if (clk_id == S32G_SCMI_CLK_PFE1_RX_RGMII ||
		   clk_id == S32G_SCMI_CLK_PFE1_RX_MII) {
		rx_id = S32G_CLK_PFE_MAC1_EXT_RX;
	} else if (clk_id == S32G_SCMI_CLK_PFE1_RX_RMII) {
		rx_id = S32G_CLK_PFE_MAC1_REF_DIV;
		ret = cc_set_mux_parent(clk, S32G_CLK_MC_CGM2_MUX8,
					S32G_CLK_PFE_MAC1_RMII_REF);
		if (ret)
			return ret;
	} else {
		ERROR("Invalid PFE1 RX mode\n");
		return -EINVAL;
	}

	return cc_set_mux_parent(clk, S32G_CLK_MC_CGM2_MUX5, rx_id);
}

static int set_mac1_tx_parent(struct clk *clk)
{
	uint32_t tx_id;
	uint32_t clk_id = clk->id;

	if (clk_id == S32G_SCMI_CLK_PFE1_TX_SGMII) {
		tx_id = S32G_CLK_SERDES1_LANE1_TX;
	} else if (clk_id == S32G_SCMI_CLK_PFE1_TX_RGMII) {
		tx_id = S32GEN1_CLK_PERIPH_PLL_PHI5;
	} else if (clk_id == S32G_SCMI_CLK_PFE1_TX_RMII) {
		tx_id = S32G_CLK_PFE_MAC1_RMII_REF;
	} else if (clk_id == S32G_SCMI_CLK_PFE1_TX_MII) {
		tx_id = S32G_CLK_PFE_MAC1_EXT_TX;
	} else {
		ERROR("Invalid PFE1 TX mode\n");
		return -EINVAL;
	}

	return cc_set_mux_parent(clk, S32G_CLK_MC_CGM2_MUX2, tx_id);
}

static int set_mac2_rx_parent(struct clk *clk)
{
	uint32_t rx_id;
	uint32_t clk_id = clk->id;
	int ret;

	if (clk_id == S32G_SCMI_CLK_PFE2_RX_SGMII) {
		rx_id = S32G_CLK_SERDES0_LANE1_CDR;
	} else if (clk_id == S32G_SCMI_CLK_PFE2_RX_RGMII ||
		   clk_id == S32G_SCMI_CLK_PFE2_RX_MII) {
		rx_id = S32G_CLK_PFE_MAC2_EXT_RX;
	} else if (clk_id == S32G_SCMI_CLK_PFE2_RX_RMII) {
		rx_id = S32G_CLK_PFE_MAC2_REF_DIV;
		ret = cc_set_mux_parent(clk, S32G_CLK_MC_CGM2_MUX9,
					S32G_CLK_PFE_MAC2_RMII_REF);
		if (ret)
			return ret;

	} else {
		ERROR("Invalid PFE2 RX mode\n");
		return -EINVAL;
	}

	return cc_set_mux_parent(clk, S32G_CLK_MC_CGM2_MUX6, rx_id);
}

static int set_mac2_tx_parent(struct clk *clk)
{
	uint32_t tx_id;
	uint32_t clk_id = clk->id;

	if (clk_id == S32G_SCMI_CLK_PFE2_TX_SGMII) {
		tx_id = S32G_CLK_SERDES0_LANE1_TX;
	} else if (clk_id == S32G_SCMI_CLK_PFE2_TX_RGMII) {
		tx_id = S32GEN1_CLK_PERIPH_PLL_PHI5;
	} else if (clk_id == S32G_SCMI_CLK_PFE2_TX_RMII) {
		tx_id = S32G_CLK_PFE_MAC2_RMII_REF;
	} else if (clk_id == S32G_SCMI_CLK_PFE2_TX_MII) {
		tx_id = S32G_CLK_PFE_MAC2_EXT_TX;
	} else {
		ERROR("Invalid PFE2 TX mode\n");
		return -EINVAL;
	}

	return cc_set_mux_parent(clk, S32G_CLK_MC_CGM2_MUX3, tx_id);
}

int plat_compound_clk_set_parents(struct clk *clk)
{
	uint32_t clk_id = clk->id;
	uint32_t id;

	if (s32g_compound2clkid(clk_id, &id)) {
		ERROR("Invalid compound clock : %" PRIu32 "\n", clk_id);
		return -EINVAL;
	}

	switch (id) {
	case S32G_CLK_PFE_MAC0_TX_DIV:
		return set_mac0_tx_parent(clk);
	case S32G_CLK_PFE_MAC0_RX:
		return set_mac0_rx_parent(clk);
	case S32G_CLK_PFE_MAC1_TX:
		return set_mac1_tx_parent(clk);
	case S32G_CLK_PFE_MAC1_RX:
		return set_mac1_rx_parent(clk);
	case S32G_CLK_PFE_MAC2_TX:
		return set_mac2_tx_parent(clk);
	case S32G_CLK_PFE_MAC2_RX:
		return set_mac2_rx_parent(clk);
	case S32CC_SCMI_NOT_IMPLEMENTED_CLK:
		return 0;
	default:
		ERROR("%s: Invalid clock %" PRIu32 "\n", __func__, id);
		return -EINVAL;
	}
}

int plat_compound_clk_enable(struct clk *clk, int enable)
{
	struct clk sclock = *clk;
	uint32_t clk_id = clk->id;
	uint32_t id;
	int ret;

	if (s32g_compound2clkid(clk_id, &id)) {
		ERROR("Invalid s32g compound clock : %" PRIu32 "\n", clk_id);
		return -EINVAL;
	}

	if (id == S32CC_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
		      s32g_scmi_clk[INDEX(clk_id)].name);
		return -EINVAL;
	}

	ret = plat_compound_clk_set_parents(clk);
	if (ret) {
		ERROR("Failed to set parents for %" PRIu32 " (%s)\n", clk_id,
		      s32g_scmi_clk[INDEX(clk_id)].name);
		return -EINVAL;
	}

	sclock.id = id;
	ret = s32gen1_enable(&sclock, enable);
	if (ret) {
		ERROR("Failed to %s %" PRIu32 " clock (%s)\n",
		      enable ? "enable" : "disable", clk_id,
		      s32g_scmi_clk[INDEX(clk_id)].name);
		return ret;
	}

	return 0;
}

unsigned long plat_compound_clk_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk sclock = *clk;
	uint32_t scmi_clk_id = clk->id;
	uint32_t id;
	int ret;

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32g_scmi_clk))
		return -EINVAL;

	ret = plat_compound_clk_set_parents(clk);
	if (ret) {
		ERROR("Failed to set parents for %" PRIu32 " (%s)\n",
		      scmi_clk_id,
		      s32g_scmi_clk[INDEX(scmi_clk_id)].name);
		return -EINVAL;
	}

	if (s32g_compound2clkid(scmi_clk_id, &id)) {
		ERROR("Invalid compound clock : %" PRIu32 " (%s)\n",
		      scmi_clk_id,
		      s32g_scmi_clk[INDEX(scmi_clk_id)].name);
		return -EINVAL;
	}

	if (id == S32CC_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
		      s32g_scmi_clk[INDEX(scmi_clk_id)].name);
		return 0;
	}

	sclock.id = id;
	return s32gen1_set_rate(&sclock, rate);
}

unsigned long plat_compound_clk_get_rate(struct clk *clk)
{
	struct clk sclock = *clk;
	uint32_t scmi_clk_id = clk->id;
	uint32_t id;

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32g_scmi_clk))
		return 0;

	if (s32g_compound2clkid(scmi_clk_id, &id)) {
		ERROR("Invalid compound clock : %" PRIu32 "\n", scmi_clk_id);
		return 0;
	}

	if (id == S32CC_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
		      s32g_scmi_clk[INDEX(scmi_clk_id)].name);
		return 0;
	}

	sclock.id = id;
	return s32gen1_get_rate(&sclock);
}

uint32_t plat_get_nclocks(void)
{
	return S32CC_PLAT_SCMI_CLK(ARRAY_SIZE(s32g_scmi_clk));
}

const char *plat_scmi_clk_get_name(uint32_t scmi_clk_id)
{
	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32g_scmi_clk))
		return NULL;

	return s32g_scmi_clk[INDEX(scmi_clk_id)].name;
}

int plat_scmi_clk_get_rates(struct clk *clk, unsigned long *rates,
			    size_t *nrates)
{
	struct clk sclock = *clk;
	struct s32gen1_clk_rates clk_rates;
	uint32_t scmi_clk_id = clk->id;
	uint32_t id;
	int ret;

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32g_scmi_clk))
		return -EINVAL;

	ret = plat_compound_clk_set_parents(clk);
	if (ret) {
		ERROR("Failed to set parents for %" PRIu32 "\n", scmi_clk_id);
		return -EINVAL;
	}

	if (s32g_compound2clkid(scmi_clk_id, &id)) {
		ERROR("Invalid compound clock : %" PRIu32 "\n", scmi_clk_id);
		return -EINVAL;
	}

	if (id == S32CC_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
		      s32g_scmi_clk[INDEX(scmi_clk_id)].name);
		return -EINVAL;
	}

	sclock.id = id;
	clk_rates.rates = rates;
	clk_rates.nrates = nrates;

	return s32gen1_get_rates(&sclock, &clk_rates);
}
