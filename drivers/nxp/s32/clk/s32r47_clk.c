// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2025 NXP
 */
#include <clk/s32cc_clk_modules.h>

/* Clocks are not handled in TF-A */
struct s32cc_clk xbar_2x_clk;

const struct siul2_freq_mapping siul2_clk_freq_map[] = {};

struct s32cc_clk *get_plat_clock(uint32_t id)
{
	return NULL;
}

struct s32cc_clk *get_plat_cc_clock(uint32_t id)
{
	return NULL;
}

int cc_compound_clk_get_pid(uint32_t id, uint32_t *parent_id)
{
	return 0;
}
