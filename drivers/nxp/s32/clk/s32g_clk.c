// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2026 NXP
 */
#include <dt-bindings/clock/s32g-clock.h>
#include <clk/s32g_clk_funcs.h>
#include <clk/s32cc_clk_modules.h>
#include <clk/s32cc_shared_clks.h>

/* XBAR_2X */
struct s32cc_clk xbar_2x_clk =
		S32CC_CHILD_CLK(cgm0_mux0_clk, 48 * MHZ, 800 * MHZ);

/* LLCE */
static struct s32cc_part part3 = S32CC_PART(3);
static struct s32cc_part_link llce_part_link =
		S32CC_PART_LINK(xbar_div2_clk, &part3);
static struct s32cc_clk llce_sys_clk =
		S32CC_FREQ_MODULE_CLK(llce_part_link, 24 * MHZ, 200 * MHZ);

/* PER_CLK */
static struct s32cc_clk per_clk =
		S32CC_FREQ_MODULE_CLK(per_div, 0, 80 * MHZ);

/* CAN_PE_CLK */
static struct s32cc_clk can_pe_clk =
		S32CC_CHILD_CLK(cgm0_mux7_clk, 40 * MHZ, 80 * MHZ);

static struct s32cc_clk *s32g_clocks[] = {
	[ARR_CLK(S32G_CLK_LLCE_SYS)] = &llce_sys_clk,
};

struct s32g_clk {
	uint32_t id;
	struct s32cc_clk *clk;
};

/* Keep this array sorted in ascending order of the clock IDs. */
static struct s32g_clk s32g_cc_clocks[] = {
	{ .id = S32CC_CLK_PER, .clk = &per_clk, },
	{ .id = S32CC_CLK_CAN_PE, .clk = &can_pe_clk, },
};

struct s32cc_clk *get_plat_cc_clock(uint32_t id)
{
	uint32_t mux_id = s32cc_platclk2mux(id);
	uint32_t lo = 0U;
	uint32_t hi = (uint32_t)ARRAY_SIZE(s32g_cc_clocks);

	while (lo < hi) {
		uint32_t mid = lo + ((hi - lo) / 2U);
		uint32_t entry_mux_id = s32cc_platclk2mux(s32g_cc_clocks[mid].id);

		if (entry_mux_id == mux_id)
			return s32g_cc_clocks[mid].clk;

		if (entry_mux_id < mux_id)
			lo = mid + 1U;
		else
			hi = mid;
	}

	return s32g_get_plat_cc_clock(id);
}

struct s32cc_clk *get_plat_clock(uint32_t id)
{
	uint32_t index;

	if (id < S32CC_PLAT_CLK_ID_BASE)
		return NULL;

	index = id - S32CC_PLAT_CLK_ID_BASE;

	if (index < ARRAY_SIZE(s32g_clocks) && s32g_clocks[index])
		return s32g_clocks[index];

	return s32g_get_plat_clock(id);
}
