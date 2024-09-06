// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */
#include <clk/mc_cgm_regs.h>
#include <clk/s32gen1_clk_funcs.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <s32_fp.h>
#include <stdint.h>
#include <inttypes.h>

static inline bool is_div(struct s32gen1_clk_obj *module)
{
	if (!module)
		return NULL;

	return module->type == s32gen1_cgm_div_t ||
			module->type == s32gen1_pll_out_div_t ||
			module->type == s32gen1_dfs_div_t;
}

static inline unsigned long compute_div_freq(unsigned long pfreq, uint64_t div)
{
	return fp2u(fp_div(u2fp(pfreq), u2fp(div)));
}

static inline bool is_div_rate_valid(unsigned long pfreq, unsigned long freq, uint64_t div)
{
	return compute_div_freq(pfreq, div) == freq;
}

static int get_div_max_value(unsigned long pfreq, unsigned long freq, uint64_t *div)
{
	if (!div || freq > pfreq || !pfreq)
		return -EINVAL;

	/* If the avertised minimum frequency is 0Hz, use maximum div value. */
	*div = freq ? fp2u(fp_div(u2fp(pfreq), u2fp(freq))) : UINT8_MAX;
	if (*div > UINT8_MAX)
		*div = UINT8_MAX;

	(*div)--;
	return 0;
}

static int get_div_min_value(unsigned long pfreq, unsigned long freq, uint64_t *div)
{
	uint64_t rem;

	if (!div || freq > pfreq || !pfreq || !freq)
		return -EINVAL;

	*div = fp2u(fp_div(u2fp(pfreq), u2fp(freq)));
	if (*div > UINT8_MAX)
		*div = UINT8_MAX;

	/**
	 * Pfreq is not a multiple of the freq.
	 * Therefore we have to decrease the resulting frequency by
	 * increasing the divider's value by one unit.
	 * E.g.: pfreq = 200, freq = 150 => div = 2
	 *       pfreq = 200, freq = 60 => div = 4
	 *       pfreq = 200, freq = 66 => div = 3
	 */
	rem = pfreq % freq;
	if (rem && !is_div_rate_valid(pfreq, freq, *div)) {
		if (check_uptr_overflow(*div, 1))
			*div = UINT8_MAX;
		(*div)++;
	}

	(*div)--;
	return 0;
}

static unsigned long get_fixed_freq(unsigned long pfreq,
		struct s32gen1_fixed_div *div)
{
	size_t i;

	if (!div->table)
		return 0;

	for (i = 0; i < div->n_mappings; i++) {
		if (pfreq == div->table[i].pfreq)
			return div->table[i].freq;
	}

	return 0;
}

static uint32_t get_mfi_value(unsigned long pfreq, unsigned long freq, uint32_t mfn)
{
	struct fp_data tmp;
	uint64_t mfi;

	/* mfi = pfreq / (2 * freq) - mfn / 36.0 */
	tmp = fp_mul(u2fp(2), u2fp(freq));
	tmp = fp_div(u2fp(pfreq), tmp);
	tmp = fp_sub(tmp, fp_div(u2fp(mfn), u2fp(36)));
	mfi = fp2u(tmp);

	return mfi > UINT8_MAX ? UINT8_MAX : mfi;
}

static inline size_t get_available_slots(struct s32gen1_clk_rates *clk_rates)
{
	return S32GEN1_MAX_NUM_FREQ - 2;
}

static inline unsigned long get_clock_max_rate(struct s32gen1_clk_rates *clk_rates)
{
	size_t nrates = *clk_rates->nrates;

	if (!nrates)
		return 0;

	return clk_rates->rates[nrates - 1];
}

static inline unsigned long get_clock_min_rate(struct s32gen1_clk_rates *clk_rates)
{
	size_t nrates = *clk_rates->nrates;

	if (!nrates)
		return 0;

	return clk_rates->rates[0];
}

static inline unsigned long compute_dfs_div_freq(unsigned long pfreq, uint32_t mfi, uint32_t mfn)
{
	struct fp_data freq;

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

static inline bool is_dfs_div_rate_valid(unsigned long pfreq, unsigned long freq, uint32_t mfi, uint32_t mfn)
{
	return compute_dfs_div_freq(pfreq, mfi, mfn) == freq;
}

static int update_min_rate(struct s32gen1_clk_rates *clk_rates, unsigned long rate)
{
	size_t nrates = *clk_rates->nrates;

	if (nrates > 1 && rate > clk_rates->rates[1])
		return -EINVAL;

	clk_rates->rates[0] = rate;

	if (!nrates)
		(*clk_rates->nrates)++;

	return 0;
}

static int update_max_rate(struct s32gen1_clk_rates *clk_rates, unsigned long rate)
{
	size_t nrates = *clk_rates->nrates;

	if (nrates > 1 && rate < clk_rates->rates[nrates - 2])
		return -EINVAL;

	if (!nrates)
		nrates = ++(*clk_rates->nrates);

	clk_rates->rates[nrates - 1] = rate;

	return 0;
}

static int populate_scaler_rates(unsigned long pfreq, struct s32gen1_clk_rates *clk_rates)
{
	uint64_t div, div_min, div_max, range;
	size_t i, num_slots;
	unsigned long tmp_freq, min_freq, max_freq;
	int ret = 0;

	if (!clk_rates)
		return -EINVAL;

	min_freq = get_clock_min_rate(clk_rates);
	max_freq = get_clock_max_rate(clk_rates);

	if (get_div_min_value(pfreq, max_freq, &div_min)) {
		ERROR("Could not compute minimum divisor value\n");
		return ret;
	}

	if (get_div_max_value(pfreq, min_freq, &div_max)) {
		ERROR("Could not compute maximum divisor value\n");
		return ret;
	}

	if (!is_div_rate_valid(pfreq, min_freq, div_max + 1)) {
		ret = update_min_rate(clk_rates, compute_div_freq(pfreq, div_max + 1));
		if (ret)
			return ret;
	}

	if (!is_div_rate_valid(pfreq, max_freq, div_min + 1)) {
		ret = update_max_rate(clk_rates, compute_div_freq(pfreq, div_min + 1));
		if (ret)
			return ret;
	}

	range = div_max - div_min;
	if (!range)
		return ret;

	num_slots = get_available_slots(clk_rates);
	if (range <= num_slots)
		num_slots = range - 1;

	for (i = 1; i <= num_slots; i++) {
		div = div_min + (i * range) / (num_slots + 1);
		tmp_freq = compute_div_freq(pfreq, div + 1);

		if (add_clk_rate(clk_rates, tmp_freq))
			return -EINVAL;
	}

	return 0;
}

static int populate_dfs_scaler_rates(unsigned long pfreq, uint32_t mfn,
			struct s32gen1_clk_rates *clk_rates)
{
	uint32_t mfi, mfi_min, mfi_max, range;
	size_t i, num_slots;
	unsigned long tmp_freq, min_freq, max_freq;
	int ret = 0;

	if (!clk_rates)
		return -EINVAL;

	min_freq = get_clock_min_rate(clk_rates);
	max_freq = get_clock_max_rate(clk_rates);

	mfi_min = get_mfi_value(pfreq, max_freq, mfn);
	mfi_max = get_mfi_value(pfreq, min_freq, mfn);

	if (!is_dfs_div_rate_valid(pfreq, min_freq, mfi_max, mfn)) {
		ret = update_min_rate(clk_rates, compute_dfs_div_freq(pfreq, mfi_max, mfn));
		if (ret)
			return ret;
	}

	if (!is_dfs_div_rate_valid(pfreq, max_freq, mfi_min, mfn)) {
		ret = update_max_rate(clk_rates, compute_dfs_div_freq(pfreq, mfi_min, mfn));
		if (ret)
			return ret;
	}

	range = mfi_max - mfi_min;
	if (!range)
		return ret;

	num_slots = get_available_slots(clk_rates);
	if (range <= num_slots)
		num_slots = range - 1;

	for (i = 1; i <= num_slots; i++) {
		mfi = mfi_min + (i * range) / (num_slots + 1);
		tmp_freq = compute_dfs_div_freq(pfreq, mfi, mfn);

		if (add_clk_rate(clk_rates, tmp_freq))
			return -EINVAL;
	}

	return 0;
}

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
		ERROR("%s: Mux (id:%" PRIu8 ") without a valid source (%" PRIu32 ")\n",
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

	dfs_addr = get_base_addr(dfs->instance, priv);
	if (!dfs_addr) {
		ERROR("Failed to detect DFS instance\n");
		return 0;
	}

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
		return div->freq;

	return compute_dfs_div_freq(pfreq, mfi, mfn);
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
		return pll->vco_freq;

	clk_src = mmio_read_32(PLLDIG_PLLCLKMUX(pll_addr));
	if (pllclk2clk(clk_src, &clk_src)) {
		ERROR("Failed to get PLL clock id\n");
		return 0;
	}

	source = get_clock(clk_src);
	if (!source) {
		ERROR("Failed to get PLL source clock\n");
		return 0;
	}

	prate = get_module_rate(&source->desc, priv);
	if (!prate) {
		ERROR("Failed to get PLL's parent frequency\n");
		return 0;
	}

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
	unsigned long freq;
	unsigned long pfreq = get_module_rate(div->parent, priv);

	if (!pfreq)
		return 0;

	freq = get_fixed_freq(pfreq, div);
	if (freq)
		return freq;

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
		ERROR("Failed to get the frequency of PLL %p\n",
		      pll_addr);
		return 0;
	}

	pllodiv = mmio_read_32(PLLDIG_PLLODIV(pll_addr, div->index));
	/* Disabled module */
	if (!(pllodiv & PLLDIG_PLLODIV_DE))
		return div->freq;

	dc = PLLDIG_PLLODIV_DIV(pllodiv);
	return fp2u(fp_div(u2fp(pfreq), u2fp(dc + 1)));
}

static int get_pll_div_freqs(struct s32gen1_clk_obj *module,
			struct s32gen1_clk_priv *priv, struct s32gen1_clk_rates *clk_rates)
{
	struct s32gen1_clk_obj *parent = get_module_parent(module);
	unsigned long pfreq;

	pfreq = get_module_rate(parent, priv);
	if (!pfreq) {
		ERROR("Failed to get the frequency of PLL div parent\n");
		return -EINVAL;
	}

	return populate_scaler_rates(pfreq, clk_rates);
}

static int get_cgm_div_freqs(struct s32gen1_clk_obj *module,
			struct s32gen1_clk_priv *priv, struct s32gen1_clk_rates *clk_rates)
{
	struct s32gen1_clk_obj *parent = get_module_parent(module);
	unsigned long pfreq;

	pfreq = get_module_rate(parent, priv);
	if (!pfreq) {
		ERROR("Failed to get the frequency of CGM div parent\n");
		return -EINVAL;
	}

	return populate_scaler_rates(pfreq, clk_rates);
}

static int get_dfs_div_freqs(struct s32gen1_clk_obj *module,
		struct s32gen1_clk_priv *priv, struct s32gen1_clk_rates *clk_rates)
{
	struct s32gen1_dfs_div *div = obj2dfsdiv(module);
	struct s32gen1_dfs *dfs;
	void *dfs_addr;
	uint32_t dvport, mfi, mfn;
	unsigned long pfreq;

	dfs = get_div_dfs(div);
	if (!dfs)
		return -EINVAL;

	pfreq = get_module_rate(div->parent, priv);
	if (!pfreq)
		return -EINVAL;

	dfs_addr = get_base_addr(dfs->instance, priv);
	if (!dfs_addr) {
		ERROR("Failed to detect DFS instance\n");
		return -EINVAL;
	}

	dvport = mmio_read_32(DFS_DVPORTn(dfs_addr, div->index));
	mfi = DFS_DVPORTn_MFI(dvport);
	mfn = DFS_DVPORTn_MFN(dvport);

	if (!mfi && !mfn)
		return -EINVAL;

	return populate_dfs_scaler_rates(pfreq, mfn, clk_rates);
}

static unsigned long get_part_link_freq(struct s32gen1_clk_obj *module,
					struct s32gen1_clk_priv *priv)
{
	struct s32gen1_part_link *link = obj2partlink(module);

	return get_module_rate(link->parent, priv);
}

static unsigned long get_part_block_link_freq(struct s32gen1_clk_obj *module,
					      struct s32gen1_clk_priv *priv)
{
	struct s32gen1_part_block_link *block = obj2partblocklink(module);

	return get_module_rate(block->parent, priv);
}

static inline bool is_cgm_div_enabled(void *cgm_addr, uint32_t mux, uint32_t div_index)
{
	return (mmio_read_32(CGM_MUXn_DCm(cgm_addr, mux, div_index)) & MC_CGM_MUXn_DCm_DE);
}

static unsigned long calc_cgm_div_freq(unsigned long pfreq, void *cgm_addr,
			       uint32_t mux, uint32_t div_index)
{
	uint32_t dc_val = mmio_read_32(CGM_MUXn_DCm(cgm_addr, mux, div_index));
	uint32_t div = MC_CGM_MUXn_DCm_DIV_VAL(dc_val) + 1;

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

	mux = get_cgm_div_mux(div);
	if (!mux)
		return -EINVAL;

	cgm_addr = get_base_addr(mux->module, priv);
	if (!cgm_addr) {
		ERROR("Failed to get CGM base address of the module %d\n",
		       mux->module);
	}

	if (!is_cgm_div_enabled(cgm_addr, mux->index, div->index))
		return div->freq;

	pfreq = get_module_rate(div->parent, priv);
	if (!pfreq) {
		ERROR("Failed to get the frequency of CGM MUX %" PRIu8 "(CGM=%p)\n",
		      mux->index, cgm_addr);
		return 0;
	}

	return calc_cgm_div_freq(pfreq, cgm_addr, mux->index, div->index);
}

unsigned long get_module_rate(struct s32gen1_clk_obj *module,
		      struct s32gen1_clk_priv *priv)
{
	if (!module) {
		ERROR("Invalid module\n");
		return 0ul;
	}

	switch (module->type) {
	case s32gen1_cgm_sw_ctrl_mux_t:
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
	case s32gen1_part_link_t:
		return get_part_link_freq(module, priv);
	case s32gen1_part_block_link_t:
		return get_part_block_link_freq(module, priv);
	case s32gen1_part_t:
		ERROR("s32gen1_part_t cannot be used to get rate\n");
		return 0u;
	case s32gen1_part_block_t:
		ERROR("s32gen1_part_block_t cannot be used to get rate\n");
		return 0u;
	};

	return 0u;
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

static int get_available_frequencies(struct s32gen1_clk_obj *module, struct s32gen1_clk_priv *priv,
			struct s32gen1_clk_rates *clk_rates)
{
	if (!module)
		return -EINVAL;

	switch (module->type) {
	case s32gen1_pll_out_div_t:
		return get_pll_div_freqs(module, priv, clk_rates);
	case s32gen1_cgm_div_t:
		return get_cgm_div_freqs(module, priv, clk_rates);
	case s32gen1_dfs_div_t:
		return get_dfs_div_freqs(module, priv, clk_rates);
	default:
		return 0;
	}
}

static int get_clk_frequencies(struct s32gen1_clk_obj *module,
	struct s32gen1_clk_priv *priv, struct s32gen1_clk_rates *clk_rates)
{
	struct s32gen1_clk_obj *parent = get_module_parent(module);

	if (!module)
		return 0;

	if (is_div(module) && module->refcount == 1)
		return get_available_frequencies(module, priv, clk_rates);

	return get_clk_frequencies(parent, priv, clk_rates);
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

int s32gen1_get_rates(struct clk *c, struct s32gen1_clk_rates *clk_rates)
{
	struct s32gen1_clk *clk = get_leaf_clk(c);
	struct s32gen1_clk_priv *priv;
	unsigned long min_rate, max_rate;
	int ret;

	if (!clk || !clk_rates)
		return -EINVAL;

	priv = s32gen1_get_clk_priv(c);
	if (!priv) {
		ERROR("Could not retrieve private data for clock %" PRIu32 ".\n", c->id);
		return -EINVAL;
	}

	min_rate = s32gen1_get_minrate(c);
	max_rate = s32gen1_get_maxrate(c);

	if (add_clk_rate(clk_rates, min_rate))
		return -EINVAL;

	if (add_clk_rate(clk_rates, max_rate))
		return -EINVAL;

	if (min_rate == max_rate)
		return 0;

	if (!clk->freq_scaling)
		return 0;

	ret = get_clk_frequencies(&clk->desc, priv, clk_rates);
	if (ret)
		WARN("Could not compute available rates for clock %" PRIu32 ".\n", c->id);

	return 0;
}

