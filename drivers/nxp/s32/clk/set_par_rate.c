// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_clk_modules.h>
#include <common/debug.h>
#include <inttypes.h>
#include <s32_fp.h>

static unsigned long set_module_rate(struct s32gen1_clk_obj *module,
				     unsigned long rate);

static unsigned long get_fixed_pfreq(unsigned long rate,
		struct s32gen1_fixed_div *div)
{
	size_t i;

	if (!div->table)
		return 0;

	for (i = 0; i < div->n_mappings; i++) {
		if (rate == div->table[i].freq)
			return div->table[i].pfreq;
	}

	return 0;
}

static unsigned long set_pll_freq(struct s32gen1_clk_obj *module,
				  unsigned long rate)
{
	struct s32gen1_pll *pll = obj2pll(module);

	if (pll->vco_freq && pll->vco_freq != rate) {
		ERROR("PLL frequency was already set\n");
		return 0;
	}

	pll->vco_freq = rate;
	return rate;
}

static unsigned long set_pll_div_freq(struct s32gen1_clk_obj *module,
				      unsigned long rate)
{
	struct s32gen1_pll_out_div *div = obj2plldiv(module);
	unsigned long prate;
	struct s32gen1_pll *pll;
	uint64_t dc;

	if (!div->parent) {
		ERROR("Failed to identify PLL divider's parent\n");
		return 0;
	}

	pll = obj2pll(div->parent);
	if (!pll) {
		ERROR("The parent of the PLL DIV is invalid\n");
		return 0;
	}

	prate = pll->vco_freq;

	/**
	 * The PLL is not initialized yet, so let's take a risk
	 * and accept the proposed rate.
	 */
	if (!prate) {
		div->freq = rate;
		return rate;
	}

	/* Decline in case the rate cannot fit PLL’s requirements. */
	dc = fp2u(fp_div(u2fp(prate), u2fp(rate)));
	if (fp2u(fp_div(u2fp(prate), u2fp(dc))) != rate)
		return 0;

	div->freq = rate;
	return rate;
}

static unsigned long set_dfs_div_freq(struct s32gen1_clk_obj *module,
				      unsigned long rate)
{
	struct s32gen1_dfs_div *div = obj2dfsdiv(module);
	struct s32gen1_dfs *dfs;

	if (!div->parent) {
		ERROR("Failed to identify DFS divider's parent\n");
		return 0;
	}

	/* Sanity check */
	dfs = obj2dfs(div->parent);
	if (!dfs->source) {
		ERROR("Failed to identify DFS's parent\n");
		return 0;
	}

	if (div->freq && div->freq != rate) {
		ERROR("DFS DIV frequency was already set to %lu\n", div->freq);
		return 0;
	}

	div->freq = rate;
	return rate;
}

static unsigned long set_cgm_div_freq(struct s32gen1_clk_obj *module,
				      unsigned long rate)
{
	struct s32gen1_cgm_div *div = obj2cgmdiv(module);

	if (!div->parent) {
		ERROR("Failed to identify DCGM divider's parent\n");
		return 0;
	}

	div->freq = rate;
	return rate;
}

static unsigned long set_clk_freq(struct s32gen1_clk_obj *module,
				  unsigned long rate)
{
	struct s32gen1_clk *clk = obj2clk(module);

	if ((clk->min_freq && clk->max_freq) &&
	    (rate < clk->min_freq || rate > clk->max_freq)) {
		ERROR("%lu frequency is out of the allowed range: [%lu:%lu]\n",
		       rate, clk->min_freq, clk->max_freq);
		return 0;
	}

	if (clk->module)
		return set_module_rate(clk->module, rate);

	if (clk->pclock)
		return set_clk_freq(&clk->pclock->desc, rate);

	return 0;
}

static unsigned long set_fixed_div_freq(struct s32gen1_clk_obj *module,
					unsigned long rate)
{
	struct s32gen1_fixed_div *div = obj2fixeddiv(module);
	unsigned long pfreq;

	if (!div->parent) {
		ERROR("The divider doesn't have a valid parent\b");
		return 0;
	}

	pfreq = get_fixed_pfreq(rate, div);
	if (!pfreq)
		pfreq = rate * div->div;

	return set_module_rate(div->parent, pfreq) / div->div;
}

static unsigned long set_mux_freq(struct s32gen1_clk_obj *module,
				  unsigned long rate)
{
	struct s32gen1_mux *mux = obj2mux(module);
	struct s32gen1_clk *clk = get_clock(mux->source_id);

	if (!clk) {
		ERROR("%s: Mux (id:%" PRIu8 ") without a valid source (%" PRIu32 ")\n",
		      __func__, mux->index, mux->source_id);
		return 0;
	}

	return set_module_rate(&clk->desc, rate);
}

static unsigned long set_osc_freq(struct s32gen1_clk_obj *module,
				  unsigned long rate)
{
	struct s32gen1_osc *osc = obj2osc(module);

	if (osc->freq && rate != osc->freq) {
		ERROR("Already initialized oscillator. freq = %lu\n",
		       osc->freq);
		return 0;
	}

	osc->freq = rate;

	return osc->freq;
}

static unsigned long set_fixed_clk_freq(struct s32gen1_clk_obj *module,
					unsigned long rate)
{
	struct s32gen1_fixed_clock *fixed_clk = obj2fixedclk(module);

	if (fixed_clk->freq && rate != fixed_clk->freq) {
		ERROR("Already initialized clock. Current freq = %lu Req freq = %lu\n",
		       fixed_clk->freq, rate);
		return 0;
	}

	fixed_clk->freq = rate;

	return fixed_clk->freq;
}

static unsigned long set_part_link_freq(struct s32gen1_clk_obj *module,
					unsigned long rate)
{
	struct s32gen1_part_link *link = obj2partlink(module);

	if (!link->parent)
		ERROR("Partition link with no parent\n");

	return set_module_rate(link->parent, rate);
}

static unsigned long set_part_block_link_freq(struct s32gen1_clk_obj *module,
					      unsigned long rate)
{
	struct s32gen1_part_block_link *block = obj2partblocklink(module);

	if (!block->parent)
		ERROR("Partition block link with no parent\n");

	return set_module_rate(block->parent, rate);
}

static unsigned long set_module_rate(struct s32gen1_clk_obj *module,
				     unsigned long rate)
{
	switch (module->type) {
	case s32gen1_fixed_clk_t:
		return set_fixed_clk_freq(module, rate);
	case s32gen1_osc_t:
		return set_osc_freq(module, rate);
	case s32gen1_pll_t:
		return set_pll_freq(module, rate);
	case s32gen1_pll_out_div_t:
		return set_pll_div_freq(module, rate);
	case s32gen1_dfs_div_t:
		return set_dfs_div_freq(module, rate);
	case s32gen1_clk_t:
		return set_clk_freq(module, rate);
	case s32gen1_mux_t:
	case s32gen1_shared_mux_t:
	case s32gen1_cgm_sw_ctrl_mux_t:
		return set_mux_freq(module, rate);
	case s32gen1_fixed_div_t:
		return set_fixed_div_freq(module, rate);
	case s32gen1_part_link_t:
		return set_part_link_freq(module, rate);
	case s32gen1_part_block_link_t:
		return set_part_block_link_freq(module, rate);
	case s32gen1_cgm_div_t:
		return set_cgm_div_freq(module, rate);
	case s32gen1_dfs_t:
		ERROR("It's not allowed to set the frequency of a DFS !");
		return 0;
	case s32gen1_part_t:
		ERROR("It's not allowed to set the frequency of a partition !");
		return 0;
	case s32gen1_part_block_t:
		ERROR("It's not allowed to set the frequency of a partition block !");
		return 0;
	};

	return 0;
}

int add_clk_rate(struct s32gen1_clk_rates *clk_rates, unsigned long rate)
{
	size_t i = 0, pos = 0;
	size_t nrates = 0;

	if (!clk_rates)
		return -EINVAL;

	nrates = *clk_rates->nrates;
	if (nrates >= S32GEN1_MAX_NUM_FREQ)
		return -EINVAL;

	/* keep the array sorted */
	for (i = 0; i < nrates; i++) {
		if (rate < clk_rates->rates[i])
			break;
	}

	pos = i;
	if (nrates) {
		for (i = 0 ; i < nrates - pos; i++)
			clk_rates->rates[nrates - i] = clk_rates->rates[nrates - i - 1];
	}

	clk_rates->rates[pos] = rate;
	(*clk_rates->nrates)++;

	return 0;
}

unsigned long s32gen1_set_rate(struct clk *c, unsigned long rate)
{
	struct s32gen1_clk *clk;
	unsigned long orig_rate = rate;

	clk = get_clock(c->id);
	if (!clk)
		return 0;

	rate = set_module_rate(&clk->desc, rate);
	if (rate == 0) {
		ERROR("Failed to set frequency (%lu MHz) for clock %" PRIu32 "\n",
		      orig_rate, c->id);
	}

	return rate;
}

static bool check_mux_source(struct s32gen1_mux *mux, uint32_t source_id)
{
	uint8_t i;

	for (i = 0; i < mux->nclks; i++) {
		if (mux->clkids[i] == source_id)
			return true;
	}

	return false;
}

static int update_frequency(struct clk *c, struct clk *p,
			    struct s32gen1_clk *clk, struct s32gen1_clk *parent)
{
	unsigned long rate;

	if (!(is_osc(parent) || is_fixed_clk(parent))) {
		ERROR("Unknown module type: %d\n", parent->desc.type);
		return -EINVAL;
	}

	rate = clk_get_rate(p);
	if (rate == 0) {
		ERROR("Failed to get the frequency of clock %" PRIu32 "\n",
		      p->id);
		return -EINVAL;
	}

	if (set_module_rate(parent->module, rate) != rate)
		return -EINVAL;

	return 0;
}

int s32gen1_set_parent(struct clk *c, struct clk *p)
{
	struct s32gen1_clk *clk, *parent;
	struct s32gen1_mux *mux;
	int ret;

	if (!c || !p)
		return -EINVAL;

	clk = get_clock(c->id);

	if (!clk) {
		ERROR("Invalid clock\n");
		return -EINVAL;
	}

	/* The parent is a fixed /external clock */
	if (p->drv != c->drv && (is_fixed_clk(clk) || is_osc(clk))) {
		ret = update_frequency(c, p, clk, clk);
		if (ret)
			return ret;
		return 0;
	}

	parent = get_clock(p->id);
	if (!parent) {
		ERROR("Invalid parent\n");
		return -EINVAL;
	}

	if (!is_mux(clk)) {
		ERROR("Clock %" PRIu32 " is not a mux\n", c->id);
		return -EINVAL;
	}

	mux = clk2mux(clk);
	if (!mux) {
		ERROR("Failed to cast clock %" PRIu32 " to clock mux\n", c->id);
		return -EINVAL;
	}

	if (!check_mux_source(mux, p->id)) {
		ERROR("Clock %" PRIu32 " is not a valid clock for mux %" PRIu32 "\n",
		       p->id, c->id);
		return -EINVAL;
	}

	mux->source_id = p->id;
	return 0;
}
