// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021, 2023-2026 NXP
 */
#include <dt-bindings/clock/s32cc-clock-freq.h>
#include <clk/s32cc_clk_funcs.h>
#include <clk/s32cc_clk_modules.h>
#include <clk/s32g_clk_funcs.h>

#define SIUL2_MIDR2_FREQ_VAL1		(0xB)

#define S32CC_XBAR_2X_MAX_FREQ		(800 * MHZ)

static const struct s32cc_clk_group *const s32cc_soc_clk_groups[] = {
	&s32cc_core_clks,
	&s32cc_serdes0_clks,
	&s32cc_accel_pll_clks,
	&s32cc_ddr_clks,
	&s32g_clks,
	&s32g2_g3_clks,
	&s32cc_cgm0_gmac0_clks,
};

const struct s32cc_clk_group *const *s32cc_get_soc_clk_groups(size_t *n)
{
	*n = ARRAY_SIZE(s32cc_soc_clk_groups);
	return s32cc_soc_clk_groups;
}

const struct siul2_freq_mapping siul2_clk_freq_map[] = {
	SIUL2_FREQ_MAP(SIUL2_MIDR2_FREQ_VAL1, S32CC_A53_MAX_FREQ,
		       S32CC_ARM_PLL_VCO_MAX_FREQ,
		       S32CC_ARM_PLL_PHI0_MAX_FREQ,
		       S32CC_XBAR_2X_MAX_FREQ),
	{} /* empty entry */
};
