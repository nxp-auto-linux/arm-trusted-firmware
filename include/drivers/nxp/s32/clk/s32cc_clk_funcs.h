/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2020-2025 NXP
 */
#ifndef S32CC_CLK_FUNCS_H
#define S32CC_CLK_FUNCS_H
#include <stdint.h>
#include <clk/clk.h>
#include <clk/s32cc_clk_modules.h>

#define S32CC_MAX_NUM_FREQ		10U

struct s32cc_clk *get_clock(uint32_t id);
struct s32cc_clk *get_plat_clock(uint32_t id);
struct s32cc_clk *get_plat_cc_clock(uint32_t id);
struct s32cc_clk_obj *get_module_parent(struct s32cc_clk_obj *module);
unsigned long s32cc_set_rate(struct clk *c, unsigned long rate);
int s32cc_set_parent(struct clk *c, struct clk *p);
int s32cc_enable(struct clk *c, int enable);
int s32cc_disable(struct clk *c);
int s32cc_enable_cgm_mux(struct s32cc_mux *mux,
			 struct s32cc_clk_priv *priv, int enable);
int s32cc_cgm_mux_to_safe(struct s32cc_mux *mux,
			  struct s32cc_clk_priv *priv);
int add_clk_rate(struct s32cc_clk_rates *clk_rates, unsigned long rate);

unsigned long s32cc_get_rate(struct clk *clk);
int s32cc_get_rates(struct clk *c, struct s32cc_clk_rates *clk_rates);
unsigned long get_module_rate(struct s32cc_clk_obj *module,
			      struct s32cc_clk_priv *priv);
unsigned long s32cc_get_minrate(struct clk *c);
unsigned long s32cc_get_maxrate(struct clk *c);

void *get_base_addr(enum s32cc_clk_source id, struct s32cc_clk_priv *priv);

int pllclk2clk(uint32_t pll_clk_id, uint32_t *clk_id);
int get_pll_mfi_mfn(unsigned long pll_vco, unsigned long ref_freq,
		    uint32_t *mfi, uint32_t *mfn);

uint32_t s32cc_platclk2mux(uint32_t clk_id);

static inline struct s32cc_clk_priv *s32cc_get_clk_priv(struct clk *c)
{
	return get_clk_drv_data(clk2clk_drv(c));
}

int cc_compound_clk_get_pid(uint32_t id, uint32_t *parent_id);

int s32cc_get_early_clks_freqs(struct siul2_freq_mapping *mapping);

#endif /* S32CC_CLK_FUNCS_H */
