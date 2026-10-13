// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2025-2026 NXP
 */
#include <clk/s32cc_clk_funcs.h>
#include <clk/s32cc_clk_modules.h>

/* Clocks are not handled in TF-A */
struct s32cc_clk xbar_2x_clk;

const struct siul2_freq_mapping siul2_clk_freq_map[] = {};

/* The clock tree is owned by the firmware running on M7 core */
const struct s32cc_clk_group *const *s32cc_get_soc_clk_groups(size_t *n)
{
	*n = 0U;
	return NULL;
}

int cc_compound_clk_get_pid(uint32_t id, uint32_t *parent_id)
{
	return 0;
}
