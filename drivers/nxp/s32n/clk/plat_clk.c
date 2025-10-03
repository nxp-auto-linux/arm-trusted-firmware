/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>

#include <drivers/clk.h>
#include <common/debug.h>

struct s32n_clk_driver {
	struct clk_driver *driver;
};

/* Fixed frequency returned by get_rate - 100 MHz */
#define S32N_FIXED_CLOCK_RATE	100000000UL

/**
 * S32N Clock get_rate implementation
 * Always returns 100 MHz regardless of clock ID
 */
static unsigned long s32n_clk_get_rate(unsigned long clk_id)
{
	/* Suppress unused parameter warnings */
	(void)clk_id;

	VERBOSE("S32N CLK: get_rate called for clock ID %lu, returning %lu Hz\n",
		clk_id, S32N_FIXED_CLOCK_RATE);

	return S32N_FIXED_CLOCK_RATE;
}

/**
 * Unimplemented clock operations - return ENOTSUP
 */
static int s32n_clk_enable(unsigned long id)
{
	(void)id;

	VERBOSE("S32N CLK: enable/disable not supported for clock ID %lu\n", id);
	return -ENOTSUP;
}

static void s32n_clk_disable(unsigned long id)
{
	VERBOSE("S32N CLK: disable not supported for clock ID %lu\n", id);
}

static int s32n_clk_get_parent(unsigned long id)
{
	VERBOSE("S32N CLK: get_parent not supported for clock ID %lu\n", id);
	return -ENOTSUP;
}

static bool s32n_clk_is_enabled(unsigned long id)
{
	VERBOSE("S32N CLK: is_enabled not supported for clock ID %lu\n", id);
	return false;
}

static const struct clk_ops s32n_clk_ops = {
	.enable = s32n_clk_enable,
	.disable = s32n_clk_disable,
	.get_rate = s32n_clk_get_rate,
	.get_parent = s32n_clk_get_parent,
	.is_enabled = s32n_clk_is_enabled,
};

int s32n_clk_init(void)
{
	clk_register(&s32n_clk_ops);

	return 0;
}
