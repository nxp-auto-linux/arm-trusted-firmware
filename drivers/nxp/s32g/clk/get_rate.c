// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */
#include <clk/mc_cgm_regs.h>
#include <clk/s32gen1_clk_funcs.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <s32g_fp.h>
#include <stdint.h>

static unsigned long get_osc_freq(struct s32gen1_clk_obj *module,
			  struct s32gen1_clk_priv *priv)
{
	struct s32gen1_osc *osc = obj2osc(module);

	if (!osc->freq) {
		ERROR("Uninitialized oscillator\n");
		return 0;
	}
	return osc->freq;
}

static unsigned long get_clk_freq(struct s32gen1_clk_obj *module,
			  struct s32gen1_clk_priv *priv)
{
	struct s32gen1_clk *clk = obj2clk(module);

	if (!clk) {
		ERROR("Invalid clock\n");
		return 0;
	}

	if (clk->module)
		return get_module_rate(clk->module, priv);

	if (!clk->pclock) {
		ERROR("Invalid clock parent\n");
		return 0;
	}

	return get_clk_freq(&clk->pclock->desc, priv);
}

static unsigned long get_mux_freq(struct s32gen1_clk_obj *module,
			  struct s32gen1_clk_priv *priv)
{
	struct s32gen1_mux *mux = obj2mux(module);
	struct s32gen1_clk *clk = get_clock(mux->source_id);

	if (!clk) {
		ERROR("%s: Mux (id:%d) without a valid source (%d)\n",
		      __func__, mux->index, mux->source_id);
		return 0;
	}
	return get_clk_freq(&clk->desc, priv);
}

static unsigned long get_dfs_freq(struct s32gen1_clk_obj *module,
			  struct s32gen1_clk_priv *priv)
{
	struct s32gen1_dfs *dfs = obj2dfs(module);
	void *dfs_addr;
	uint32_t ctl;

	dfs_addr = get_base_addr(dfs->instance, priv);
	if (!dfs_addr) {
		ERROR("Failed to detect DFS instance\n");
		return 0;
	}

	ctl = mmio_read_32(DFS_CTL(dfs_addr));
	/* Disabled DFS */
	if (ctl & DFS_CTL_RESET)
		return 0;

	return get_module_rate(dfs->source, priv);
}

static unsigned long get_dfs_div_freq(struct s32gen1_clk_obj *module,
			      struct s32gen1_clk_priv *priv)
{
	struct s32gen1_dfs_div *div = obj2dfsdiv(module);
	struct s32gen1_dfs *dfs;
	void *dfs_addr;
	uint32_t dvport, mfi, mfn;
	unsigned long pfreq;
	struct fp_data freq;

	dfs = get_div_dfs(div);
	if (!dfs)
		return 0;

	pfreq = get_module_rate(div->parent, priv);
	if (!pfreq)
		return 0;

	dfs_addr = get_base_addr(dfs->instance, priv);
	if (!dfs_addr) {
		ERROR("Failed to detect DFS instance\n");
		return 0;
	}

	dvport = mmio_read_32(DFS_DVPORTn(dfs_addr, div->index));

	mfi = DFS_DVPORTn_MFI(dvport);
	mfn = DFS_DVPORTn_MFN(dvport);

	/* Disabled port */
	if (!mfi && !mfn)
		return 0;

	/**
	 * Formula for input and output clocks of each port divider.
	 * See 'Digital Frequency Synthesizer' chapter from Reference Manual.
	 *
	 * freq = pfreq / (2 * (mfi + mfn / 36.0));
	 */
	freq = fp_add(u2fp(mfi), fp_div(u2fp(mfn), u2fp(36)));
	freq = fp_mul(u2fp(2), freq);
	freq = fp_div(u2fp(pfreq), freq);

	return fp2u(freq);
}

static unsigned long get_pll_freq(struct s32gen1_clk_obj *module,
			  struct s32gen1_clk_priv *priv)
{
	struct s32gen1_pll *pll = obj2pll(module);
	struct s32gen1_clk *source;
	unsigned long prate;
	void *pll_addr;
	uint32_t pllpd;
	uint32_t mfi, mfn, rdiv, plldv;
	uint32_t clk_src;
	struct fp_data t1, t2;

	pll_addr = get_base_addr(pll->instance, priv);
	if (!pll_addr) {
		ERROR("Failed to detect PLL instance\n");
		return 0;
	}

	/* Disabled PLL */
	pllpd = mmio_read_32(PLLDIG_PLLCR(pll_addr)) & PLLDIG_PLLCR_PLLPD;
	if (pllpd)
		return 0;

	clk_src = mmio_read_32(PLLDIG_PLLCLKMUX(pll_addr));
	if (pllclk2clk(clk_src, &clk_src)) {
		ERROR("Failed to get PLL clock id\n");
		return -EINVAL;
	}

	source = get_clock(clk_src);
	if (!source) {
		ERROR("Failed to get PLL source clock\n");
		return 0;
	}

	prate = get_module_rate(&source->desc, priv);
	plldv = mmio_read_32(PLLDIG_PLLDV(pll_addr));
	mfi = PLLDIG_PLLDV_MFI(plldv);
	rdiv = PLLDIG_PLLDV_RDIV(plldv);
	if (rdiv == 0)
		rdiv = 1;

	/* Frac-N mode */
	mfn = PLLDIG_PLLFD_MFN_SET(mmio_read_32(PLLDIG_PLLFD(pll_addr)));

	/* PLL VCO frequency in Fractional mode when PLLDV[RDIV] is not 0 */
	t1 = fp_div(u2fp(prate), u2fp(rdiv));
	t2 = fp_add(u2fp(mfi), fp_div(u2fp(mfn), u2fp(18432)));

	return fp2u(fp_mul(t1, t2));
}

static unsigned long get_fixed_clk_freq(struct s32gen1_clk_obj *module,
				struct s32gen1_clk_priv *priv)
{
	struct s32gen1_fixed_clock *clk = obj2fixedclk(module);

	return clk->freq;
}

static unsigned long get_fixed_div_freq(struct s32gen1_clk_obj *module,
				struct s32gen1_clk_priv *priv)
{
	struct s32gen1_fixed_div *div = obj2fixeddiv(module);
	unsigned long pfreq = get_module_rate(div->parent, priv);

	return fp2u(fp_div(u2fp(pfreq), u2fp(div->div)));
}

static unsigned long get_pll_div_freq(struct s32gen1_clk_obj *module,
			      struct s32gen1_clk_priv *priv)
{
	struct s32gen1_pll_out_div *div = obj2plldiv(module);
	struct s32gen1_pll *pll;
	void *pll_addr;
	uint32_t pllodiv;
	unsigned long pfreq;
	uint32_t dc;

	pll = get_div_pll(div);
	if (!pll) {
		ERROR("The parent of the PLL DIV is invalid\n");
		return 0;
	}

	pll_addr = get_base_addr(pll->instance, priv);
	if (!pll_addr) {
		ERROR("Failed to detect PLL instance\n");
		return 0;
	}

	pfreq = get_module_rate(div->parent, priv);
	if (!pfreq) {
		ERROR("Failed to get the frequency of PLL\n");
		return 0;
	}

	pllodiv = mmio_read_32(PLLDIG_PLLODIV(pll_addr, div->index));
	/* Disabled module */
	if (!(pllodiv & PLLDIG_PLLODIV_DE))
		return 0;

	dc = PLLDIG_PLLODIV_DIV(pllodiv);
	return fp2u(fp_div(u2fp(pfreq), u2fp(dc + 1)));
}

static unsigned long get_part_block_freq(struct s32gen1_clk_obj *module,
				 struct s32gen1_clk_priv *priv)
{
	struct s32gen1_part_block *block = obj2partblock(module);

	return get_module_rate(block->parent, priv);
}

static unsigned long calc_cgm_div_freq(unsigned long pfreq, void *cgm_addr,
			       uint32_t mux, uint32_t div_index)
{
	uint32_t dc_val = mmio_read_32(CGM_MUXn_DCm(cgm_addr, mux, div_index));
	uint32_t div;

	dc_val &= (MC_CGM_MUXn_DCm_DIV_MASK | MC_CGM_MUXn_DCm_DE);

	if (!(dc_val & MC_CGM_MUXn_DCm_DE))
		return 0;

	div = MC_CGM_MUXn_DCm_DIV_VAL(dc_val) + 1;
	return fp2u(fp_div(u2fp(pfreq), u2fp(div)));
}

static unsigned long get_cgm_div_freq(struct s32gen1_clk_obj *module,
			      struct s32gen1_clk_priv *priv)
{
	struct s32gen1_cgm_div *div = obj2cgmdiv(module);
	struct s32gen1_mux *mux;
	void *cgm_addr;
	unsigned long pfreq;

	if (!div->parent) {
		ERROR("Failed to identify CGM divider's parent\n");
		return 0;
	}

	pfreq = get_module_rate(div->parent, priv);
	if (!pfreq) {
		ERROR("Failed to get the frequency of CGM MUX\n");
		return 0;
	}

	mux = get_cgm_div_mux(div);
	if (!mux)
		return -EINVAL;

	cgm_addr = get_base_addr(mux->module, priv);
	if (!cgm_addr) {
		ERROR("Failed to get CGM base address of the module %d\n",
		       mux->module);
	}

	return calc_cgm_div_freq(pfreq, cgm_addr, mux->index, div->index);
}

unsigned long get_module_rate(struct s32gen1_clk_obj *module,
		      struct s32gen1_clk_priv *priv)
{
	if (!module) {
		ERROR("Invalid module\n");
		return 0;
	}

	switch (module->type) {
	case s32gen1_shared_mux_t:
	case s32gen1_mux_t:
		return get_mux_freq(module, priv);
	case s32gen1_clk_t:
		return get_clk_freq(module, priv);
	case s32gen1_osc_t:
		return get_osc_freq(module, priv);
	case s32gen1_pll_t:
		return get_pll_freq(module, priv);
	case s32gen1_dfs_t:
		return get_dfs_freq(module, priv);
	case s32gen1_dfs_div_t:
		return get_dfs_div_freq(module, priv);
	case s32gen1_fixed_clk_t:
		return get_fixed_clk_freq(module, priv);
	case s32gen1_fixed_div_t:
		return get_fixed_div_freq(module, priv);
	case s32gen1_pll_out_div_t:
		return get_pll_div_freq(module, priv);
	case s32gen1_cgm_div_t:
		return get_cgm_div_freq(module, priv);
	case s32gen1_part_block_t:
		return get_part_block_freq(module, priv);
	};

	return 0;
}

static struct s32gen1_clk *get_leaf_clk(struct clk *c)
{
	struct s32gen1_clk *clk;

	if (!c)
		return NULL;

	clk = get_clock(c->id);
	if (!clk) {
		ERROR("Invalid clock\n");
		return NULL;
	}

	if (clk->desc.type != s32gen1_clk_t) {
		ERROR("Invalid clock type: %d\n", clk->desc.type);
		return NULL;
	}

	return clk;
}

unsigned long s32gen1_get_rate(struct clk *c)
{
	struct s32gen1_clk *clk;
	struct s32gen1_clk_priv *priv;

	priv = s32gen1_get_clk_priv(c);

	clk = get_leaf_clk(c);
	if (!clk)
		return 0;

	return get_module_rate(&clk->desc, priv);
}

unsigned long s32gen1_get_maxrate(struct clk *c)
{
	struct s32gen1_clk *clk = get_leaf_clk(c);

	if (!clk)
		return 0;

	return clk->max_freq;
}

unsigned long s32gen1_get_minrate(struct clk *c)
{
	struct s32gen1_clk *clk = get_leaf_clk(c);

	if (!clk)
		return 0;

	return clk->min_freq;
}
