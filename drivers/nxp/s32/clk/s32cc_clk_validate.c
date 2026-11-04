// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2026 NXP
 */
#include <clk/s32cc_clk_funcs.h>
#include <clk/s32cc_clk_modules.h>
#include <common/debug.h>
#include <dt-bindings/clock/s32cc-clock.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Consistency checks of the S32CC clock groups. The clock tables are static,
 * so the callers only run the checks when S32CC_CLOCK_DEBUG is set.
 */

static bool s32cc_clk_id_is_hw(uint32_t id)
{
	return id >= S32CC_CLK_ID_BASE &&
	       id < S32CC_CC_CLK(S32CC_CLK_NUM_PROVIDERS);
}

static int validate_mux_sources(uint32_t id, struct s32cc_mux *mux)
{
	uint32_t pll_clk_id;
	uint8_t i;

	for (i = 0; i < mux->nclks; i++) {
		switch (mux->module) {
		case S32CC_ACCEL_PLL:
		case S32CC_ARM_PLL:
		case S32CC_DDR_PLL:
		case S32CC_PERIPH_PLL:
			/* The PLLCLKMUX encoding comes from clk2pllclk() */
			if (clk2pllclk(mux->clkids[i], &pll_clk_id) == 0)
				continue;
			break;
		case S32CC_CGM0 ... S32CC_CGM6:
			/* The mux source ID is used as the SELCTL value */
			if (s32cc_clk_id_is_hw(mux->clkids[i]))
				continue;
			break;
		default:
			ERROR("Clock %" PRIu32 ": unknown mux type %d\n",
			      id, mux->module);
			return -EINVAL;
		}

		ERROR("Clock %" PRIu32 ": invalid mux source %" PRIu32 "\n",
		      id, mux->clkids[i]);
		return -EINVAL;
	}

	return 0;
}

/*
 * Check the clock groups listed by the SoC:
 *  - each entry can be indexed, has a clock and uses its own ID: building
 *    the clock index stops the boot otherwise;
 *  - each mux source can be encoded in the mux selection register.
 */
int s32cc_clk_validate(void)
{
	const struct s32cc_clk_group *const *groups;
	const struct s32cc_clk_group *group;
	size_t ngroups;
	struct s32cc_clk *clk;
	uint32_t id;
	size_t i, j;
	int ret;

	/* Build the clock index if no clock lookup has done it yet */
	s32cc_clk_index_init();

	groups = s32cc_get_soc_clk_groups(&ngroups);
	for (i = 0; i < ngroups; i++) {
		group = groups[i];

		for (j = 0; j < group->n; j++) {
			id = group->entries[j].id;
			clk = group->entries[j].clk;

			if (!is_mux(clk))
				continue;

			ret = validate_mux_sources(id, clk2mux(clk));
			if (ret)
				return ret;
		}
	}

	return 0;
}
