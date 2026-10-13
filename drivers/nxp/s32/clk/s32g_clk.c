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

static const struct s32cc_clk_entry s32g_entries[] = {
	{ S32CC_CLK_PER, &per_clk },
	{ S32CC_CLK_CAN_PE, &can_pe_clk },
	{ S32G_CLK_LLCE_SYS, &llce_sys_clk },
};
const struct s32cc_clk_group s32g_clks =
		S32CC_CLK_GROUP(s32g_entries);
