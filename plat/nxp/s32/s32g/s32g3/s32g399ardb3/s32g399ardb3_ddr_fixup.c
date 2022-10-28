// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */
#include <stdint.h>
#include <clk/clk.h>
#include <clk/s32gen1_clk_funcs.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <drivers/nxp/s32/adc/s32_adc.h>
#include <drivers/nxp/s32/ddr/ddr_init.h>
#include <dt-bindings/clock/s32gen1-clock-freq.h>
#include <dt-bindings/clock/s32gen1-clock.h>
#include <lib/libc/errno.h>
#include <libfdt.h>
#include "s32_dt.h"

#define SARADC0_CH5		5
#define SARADC0_TOLERANCE	200

#define UMCTL2_RFSHCTL0		0x403C0050
#define UMCTL2_RFSHCTL0_RDB3F	0x80210010U

static const struct {
	const char *rev;
	const char *desc;
	uint32_t lower;
	uint32_t upper;
} rdb_revisions[] = {
	{ /* 0V */
		.rev = "",
		.lower = 0,
		.upper = 400,
	},
	{ /* 0.8v */
		.rev = "C",
		.lower = 1820 - SARADC0_TOLERANCE,
		.upper = 1820 + SARADC0_TOLERANCE,
	},
	{ /* 1.0v */
		.rev = "D",
		.lower = 2275 - SARADC0_TOLERANCE,
		.upper = 2275 + SARADC0_TOLERANCE,
	},
	{ /* 1.2v */
		.rev = "E",
		.lower = 2730 - SARADC0_TOLERANCE,
		.upper = 2730 + SARADC0_TOLERANCE,
	},
	{ /* 1.4v */
		.rev = "F",
		.lower = 3185 - SARADC0_TOLERANCE,
		.upper = 3185 + SARADC0_TOLERANCE,
	},
};

static int find_rdb_rev(uint32_t adc_value)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rdb_revisions); i++) {
		if (rdb_revisions[i].lower <= adc_value &&
		    adc_value <= rdb_revisions[i].upper)
			return i;
	}

	return -EINVAL;
}

static int ddr_fixup_rdb_revf(void)
{
	size_t i;

	for (i = 0; i < ddrc_cfg_size; i++)
		if (ddrc_cfg[i].addr == UMCTL2_RFSHCTL0) {
			ddrc_cfg[i].data = UMCTL2_RFSHCTL0_RDB3F;
			break;
		}

	return 0;
}

static int dt_init_adc(struct s32_adc *adc_driver)
{
	void *fdt = NULL;
	int adc_node;
	int ret;

	if (dt_open_and_check() < 0)
		return -EINVAL;

	if (fdt_get_address(&fdt) == 0)
		return -EINVAL;

	adc_node = fdt_node_offset_by_compatible(fdt, -1, "nxp,s32cc-adc");
	if (adc_node == -1)
		return -ENODEV;

	ret = s32_adc_get_setup_from_fdt(fdt, adc_node, adc_driver);
	if (ret)
		return ret;

	return s32_adc_init(adc_driver);
}

int ddr_config_fixup(void)
{
	struct s32_adc adc_driver;
	uint32_t adc_val;
	int rev_idx;
	int ret;

	ret = dt_init_adc(&adc_driver);
	if (ret)
		return ret;

	ret = s32_adc_read_channel(&adc_driver, SARADC0_CH5, &adc_val);
	if (ret)
		return ret;

	rev_idx =  find_rdb_rev(adc_val);
	if (rev_idx < 0) {
		ERROR("Unknown RDB board revision: ");
		return 0;
	}

	switch (rdb_revisions[rev_idx].rev[0]) {
	case 'C':
	case 'D':
	case 'E':
		break;
	case 'F':
		return ddr_fixup_rdb_revf();
	}

	return 0;
}

int enable_board_early_clocks(void)
{
	struct clk periph_pll_phi1 = (struct clk) {
		.id = (S32GEN1_CLK_PERIPH_PLL_PHI1),
		.drv = s32gen1_get_early_clk_driver()};
	struct clk mc_cgm0_mux3 = (struct clk) {
		.id = (S32GEN1_CLK_MC_CGM0_MUX3),
		.drv = s32gen1_get_early_clk_driver()};
	struct clk per = (struct clk) {
		.id = (S32GEN1_CLK_PER),
		.drv = s32gen1_get_early_clk_driver()};
	unsigned long rate;
	int ret;

	ret = s32gen1_set_parent(&mc_cgm0_mux3, &periph_pll_phi1);
	if (ret)
		return ret;

	rate = s32gen1_set_rate(&periph_pll_phi1,
				S32GEN1_PERIPH_PLL_PHI1_FREQ);
	if (rate != S32GEN1_PERIPH_PLL_PHI1_FREQ)
		return -EINVAL;

	rate = s32gen1_set_rate(&per, S32GEN1_PER_FREQ);
	if (rate != S32GEN1_PER_FREQ)
		return -EINVAL;

	return s32gen1_enable(&per, 1);
}
