// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_scmi_clk.h>
#include <common/debug.h>
#include <dt-bindings/clock/s32gen1-clock.h>
#include <dt-bindings/clock/s32cc-scmi-clock.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

static bool is_scmi_clk(uint32_t id)
{
	if (id < S32GEN1_CLK_ID_BASE)
		return true;

	return false;
}

static int translate_clk(struct clk *clk, bool *is_compound)
{
	uint32_t clk_id;
	int ret;

	if (!clk)
		return -EINVAL;

	*is_compound = false;

	if (!is_scmi_clk(clk->id))
		return 0;

	ret = cc_scmi_id2clk(clk->id, &clk_id);
	if (ret) {
		ERROR("Clock with ID %" PRIu32 " isn't covered by this driver\n",
		       clk->id);
		return -EINVAL;
	}

	if (clk_id == S32CC_SCMI_COMPLEX_CLK)
		*is_compound = true;
	else
		clk->id = clk_id;

	return 0;
}

int s32gen1_scmi_request(uint32_t id, struct clk *clk)
{
	int ret;
	bool is_compound;

	ret = translate_clk(clk, &is_compound);
	if (ret)
		return ret;

	if (is_compound)
		return cc_compound_clk_get(clk);

	if (!get_clock(clk->id)) {
		ERROR("Clock %" PRIu32 " is not part of the clock tree\n",
		      clk->id);
		return -EINVAL;
	}

	return 0;
}

unsigned long s32gen1_scmi_get_rate(struct clk *clk)
{
	int ret;
	bool is_compound;

	ret = translate_clk(clk, &is_compound);
	if (ret)
		return 0;

	if (is_compound)
		return cc_compound_clk_get_rate(clk);

	return s32gen1_get_rate(clk);
}

unsigned long s32gen1_scmi_set_rate(struct clk *clk, unsigned long rate)
{
	int ret;
	bool is_compound;

	ret = translate_clk(clk, &is_compound);
	if (ret)
		return 0;

	if (is_compound)
		return cc_compound_clk_set_rate(clk, rate);

	return s32gen1_set_rate(clk, rate);
}

int s32gen1_scmi_set_parent(struct clk *clk, struct clk *parent)
{
	if (is_scmi_clk(clk->id)) {
		ERROR("Is not allowed to set parents for SCMI clocks\n");
		return -EINVAL;
	}

	return s32gen1_set_parent(clk, parent);
}

int s32gen1_scmi_enable(struct clk *clk, int enable)
{
	int ret;
	bool is_compound;

	ret = translate_clk(clk, &is_compound);
	if (ret)
		return ret;

	if (is_compound)
		return cc_compound_clk_enable(clk, enable);

	return s32gen1_enable(clk, enable);
}

uint32_t s32gen1_scmi_nclocks(void)
{
	return cc_get_nclocks();
}

const char *s32gen1_scmi_clk_get_name(uint32_t scmi_clk_id)
{
	return cc_scmi_clk_get_name(scmi_clk_id);
}

int s32gen1_scmi_clk_get_rates(struct clk *clk, unsigned long *rates,
			       size_t *nrates)
{
	int ret;
	bool is_compound;
	struct s32gen1_clk_rates clk_rates;

	ret = translate_clk(clk, &is_compound);
	if (ret)
		return ret;

	if (is_compound)
		return cc_scmi_clk_get_rates(clk, rates, nrates);

	clk_rates.rates = rates;
	clk_rates.nrates = nrates;

	return s32gen1_get_rates(clk, &clk_rates);
}

unsigned long s32gen1_scmi_clk_get_rate(struct clk *clk)
{
	int ret;
	bool is_compound;

	ret = translate_clk(clk, &is_compound);
	if (ret)
		return 0;

	if (is_compound)
		return cc_compound_clk_get_rate(clk);

	return s32gen1_get_rate(clk);
}

unsigned long s32gen1_scmi_clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret;
	bool is_compound;

	ret = translate_clk(clk, &is_compound);
	if (ret)
		return 0;

	if (is_compound)
		return cc_compound_clk_set_rate(clk, rate);

	return s32gen1_set_rate(clk, rate);
}
