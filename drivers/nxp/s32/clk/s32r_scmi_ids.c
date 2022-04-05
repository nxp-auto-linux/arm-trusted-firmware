// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */
#include <dt-bindings/clock/s32r45-clock.h>
#include <dt-bindings/clock/s32r45-scmi-clock.h>
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_scmi_clk.h>
#include <errno.h>
#include <stdint.h>

#define INDEX(X)	((X) - S32GEN1_SCMI_PLAT_CLK_BASE_ID)

struct s32gen1_scmi_clk s32r45_scmi_clk[] = {
    /* LAX */
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_LAX_MODULE,
		S32R45_CLK_ACCEL4, "lax_module"),
    /* SPT */
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_SPT_SPT,
		S32R45_CLK_ACCEL3, "spt_spt"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_SPT_AXI,
		S32R45_CLK_ACCEL3, "spt_axi"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_SPT_MODULE,
		S32R45_CLK_ACCEL3_DIV3, "spt_module"),
	/* GMAC1 */
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_GMAC1_TS,
		S32GEN1_SCMI_COMPLEX_CLK, "gmac1_ts"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_GMAC1_RX_SGMII,
		S32GEN1_SCMI_COMPLEX_CLK, "gmac1_rx_sgmii"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_GMAC1_TX_SGMII,
		S32GEN1_SCMI_COMPLEX_CLK, "gmac1_tx_sgmii"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_GMAC1_RX_RGMII,
		S32GEN1_SCMI_COMPLEX_CLK, "gmac1_rx_rgmii"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_GMAC1_TX_RGMII,
		S32GEN1_SCMI_COMPLEX_CLK, "gmac1_tx_rgmii"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_GMAC1_RX_RMII,
		S32GEN1_SCMI_COMPLEX_CLK, "gmac1_rx_rmii"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_GMAC1_TX_RMII,
		S32GEN1_SCMI_COMPLEX_CLK, "gmac1_tx_rmii"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_GMAC1_RX_MII,
		S32GEN1_SCMI_COMPLEX_CLK, "gmac1_rx_mii"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_GMAC1_TX_MII,
		S32GEN1_SCMI_COMPLEX_CLK, "gmac1_tx_mii"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_GMAC1_AXI,
		S32GEN1_CLK_XBAR, "gmac1_axi"),
};

static int s32r_compound2clkid(uint32_t scmi_clk_id, uint32_t *clk_id)
{
	switch (scmi_clk_id) {
	case S32R45_SCMI_CLK_GMAC1_RX_SGMII:
	case S32R45_SCMI_CLK_GMAC1_RX_RGMII:
		if (clk_id)
			*clk_id = S32R45_CLK_GMAC1_RX;
		break;
	case S32R45_SCMI_CLK_GMAC1_TX_SGMII:
	case S32R45_SCMI_CLK_GMAC1_TX_RGMII:
		if (clk_id)
			*clk_id = S32R45_CLK_GMAC1_TX;
		break;
	case S32R45_SCMI_CLK_GMAC1_TS:
		if (clk_id)
			*clk_id = S32GEN1_CLK_GMAC0_TS;
		break;
	case S32R45_SCMI_CLK_GMAC1_RX_RMII:
	case S32R45_SCMI_CLK_GMAC1_TX_RMII:
	case S32R45_SCMI_CLK_GMAC1_RX_MII:
	case S32R45_SCMI_CLK_GMAC1_TX_MII:
		if (clk_id)
			*clk_id = S32GEN1_SCMI_NOT_IMPLEMENTED_CLK;
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

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32r45_scmi_clk))
		return -EINVAL;

	*clk_id = s32r45_scmi_clk[INDEX(scmi_clk_id)].plat_id;
	if (!*clk_id) {
		ERROR("Unhandled S32R clock: %u\n", scmi_clk_id);
		return -EINVAL;
	}

	return 0;
}

int plat_compound_clk_get(struct clk *clk)
{
	uint32_t scmi_clk_id = clk->id;

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32r45_scmi_clk))
		return -EINVAL;

	if (s32r_compound2clkid(scmi_clk_id, NULL)) {
		ERROR("Invalid S32R compound clock : %u\n", scmi_clk_id);
		return -EINVAL;
	}

	return 0;
}

static int set_gmac1_rx_parent(struct clk *clk)
{
	uint32_t rx_id;
	uint32_t clk_id = clk->id;

	if (clk_id == S32R45_SCMI_CLK_GMAC1_RX_SGMII) {
		rx_id = S32R45_CLK_SERDES1_LANE0_CDR;
	} else if (clk_id == S32R45_SCMI_CLK_GMAC1_RX_RGMII) {
		rx_id = S32R45_CLK_GMAC1_EXT_RX;
	} else {
		ERROR("Invalid GMAC1 RX mode\n");
		return -EINVAL;
	}

	return cc_set_mux_parent(clk, S32R45_CLK_MC_CGM2_MUX4, rx_id);
}

static int set_gmac1_tx_parent(struct clk *clk)
{
	uint32_t tx_id;
	uint32_t clk_id = clk->id;

	if (clk_id == S32R45_SCMI_CLK_GMAC1_TS) {
		tx_id = S32GEN1_CLK_PERIPH_PLL_PHI5;
	} else if (clk_id == S32R45_SCMI_CLK_GMAC1_TX_SGMII) {
		tx_id = S32R45_CLK_SERDES1_LANE0_TX;
	} else if (clk_id == S32R45_SCMI_CLK_GMAC1_TX_RGMII) {
		tx_id = S32GEN1_CLK_PERIPH_PLL_PHI5;
	} else {
		ERROR("Invalid GMAC1 TX mode\n");
		return -EINVAL;
	}

	return cc_set_mux_parent(clk, S32R45_CLK_MC_CGM2_MUX2, tx_id);
}

static int set_gmac_ts_parent(struct clk *clk)
{
	uint32_t ts_id;
	uint32_t clk_id = clk->id;

	if (clk_id == S32R45_SCMI_CLK_GMAC1_TS) {
		ts_id = S32GEN1_CLK_PERIPH_PLL_PHI4;
	} else {
		ERROR("Invalid GMAC1 TS mode\n");
		return -EINVAL;
	}

	return cc_set_mux_parent(clk, S32GEN1_CLK_MC_CGM0_MUX9, ts_id);
}

int plat_compound_clk_set_parents(struct clk *clk)
{
	uint32_t clk_id = clk->id;
	uint32_t id;

	if (s32r_compound2clkid(clk_id, &id)) {
		ERROR("Invalid compound clock : %u\n", clk_id);
		return -EINVAL;
	}

	switch (id) {
	case S32R45_CLK_GMAC1_RX:
		return set_gmac1_rx_parent(clk);
	case S32R45_CLK_GMAC1_TX:
		return set_gmac1_tx_parent(clk);
	case S32GEN1_CLK_GMAC0_TS:
		return set_gmac_ts_parent(clk);
	case S32GEN1_SCMI_NOT_IMPLEMENTED_CLK:
		return 0;
	default:
		ERROR("%s: Invalid clock %d\n", __func__, id);
		return -EINVAL;
	}
}

int plat_compound_clk_enable(struct clk *clk, int enable)
{
	struct clk sclock = *clk;
	uint32_t clk_id = clk->id;
	uint32_t id;
	int ret;

	if (s32r_compound2clkid(clk_id, &id)) {
		ERROR("Invalid S32R compound clock : %u\n", clk_id);
		return -EINVAL;
	}

	if (id == S32GEN1_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
			s32r45_scmi_clk[INDEX(clk_id)].name);
		return -EINVAL;
	}

	ret = plat_compound_clk_set_parents(clk);
	if (ret) {
		ERROR("Failed to set parents for %u\n", clk_id);
		return -EINVAL;
	}

	sclock.id = id;
	ret = s32gen1_enable(&sclock, enable);
	if (ret) {
		ERROR("%s Failed to enable %u clock\n", __func__, clk_id);
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

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32r45_scmi_clk))
		return -EINVAL;

	ret = plat_compound_clk_set_parents(clk);
	if (ret) {
		ERROR("Failed to set parents for %u\n", scmi_clk_id);
		return -EINVAL;
	}

	if (s32r_compound2clkid(scmi_clk_id, &id)) {
		ERROR("Invalid compound clock : %u\n", scmi_clk_id);
		return -EINVAL;
	}

	if (id == S32GEN1_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
			s32r45_scmi_clk[INDEX(scmi_clk_id)].name);
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

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32r45_scmi_clk))
		return 0;

	if (s32r_compound2clkid(scmi_clk_id, &id)) {
		ERROR("Invalid compound clock : %u\n", scmi_clk_id);
		return 0;
	}

	if (id == S32GEN1_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
			s32r45_scmi_clk[INDEX(scmi_clk_id)].name);
		return 0;
	}

	sclock.id = id;
	return s32gen1_get_rate(&sclock);
}

uint32_t plat_get_nclocks(void)
{
	return S32GEN1_PLAT_SCMI_CLK(ARRAY_SIZE(s32r45_scmi_clk));
}

const char *plat_scmi_clk_get_name(uint32_t scmi_clk_id)
{
	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32r45_scmi_clk))
		return NULL;

	return s32r45_scmi_clk[INDEX(scmi_clk_id)].name;
}

int plat_scmi_clk_get_rates(struct clk *clk, unsigned long *rates,
			    size_t *nrates)
{
	struct clk sclock = *clk;
	uint32_t scmi_clk_id = clk->id;
	uint32_t id;
	int ret;

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32r45_scmi_clk))
		return -EINVAL;

	ret = plat_compound_clk_set_parents(clk);
	if (ret) {
		ERROR("Failed to set parents for %u\n", scmi_clk_id);
		return -EINVAL;
	}

	if (s32r_compound2clkid(scmi_clk_id, &id)) {
		ERROR("Invalid compound clock : %u\n", scmi_clk_id);
		return -EINVAL;
	}

	if (id == S32GEN1_SCMI_NOT_IMPLEMENTED_CLK) {
		ERROR("Clock %s is not handled yet\n",
			s32r45_scmi_clk[INDEX(scmi_clk_id)].name);
		return -EINVAL;
	}

	sclock.id = id;
	rates[0] = s32gen1_get_minrate(&sclock);
	rates[1] = s32gen1_get_maxrate(&sclock);

	return 0;
}

