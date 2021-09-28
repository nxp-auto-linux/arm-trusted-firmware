/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2020-2021 NXP
 */
#ifndef S32GEN1_CLK_FUNCS_H
#define S32GEN1_CLK_FUNCS_H
#include <stdint.h>
#include <clk/clk.h>
#include <clk/s32gen1_clk_modules.h>

struct s32gen1_clk *get_clock(uint32_t id);
struct s32gen1_clk *get_plat_clock(uint32_t id);
struct s32gen1_clk *get_plat_cc_clock(uint32_t id);
unsigned long s32gen1_set_rate(struct clk *c, unsigned long rate);
int s32gen1_set_parent(struct clk *c, struct clk *p);
int s32gen1_enable(struct clk *c, int enable);
int s32gen1_disable(struct clk *c);
void s32gen1_disable_partition(struct s32gen1_clk_priv *priv,
			       uint32_t partition_n);
void s32gen1_enable_partition(struct s32gen1_clk_priv *priv,
			      uint32_t partition_n);

unsigned long s32gen1_get_rate(struct clk *clk);
unsigned long get_module_rate(struct s32gen1_clk_obj *module,
			      struct s32gen1_clk_priv *priv);
unsigned long s32gen1_get_minrate(struct clk *c);
unsigned long s32gen1_get_maxrate(struct clk *c);

void *get_base_addr(enum s32gen1_clk_source id, struct s32gen1_clk_priv *priv);

int pllclk2clk(uint32_t pll_clk_id, uint32_t *clk_id);
int get_pll_mfi_mfn(unsigned long pll_vco, unsigned long ref_freq,
		    uint32_t *mfi, uint32_t *mfn);

uint32_t s32gen1_platclk2mux(uint32_t clk_id);

static inline struct s32gen1_clk_priv *s32gen1_get_clk_priv(struct clk *c)
{
	return get_clk_drv_data(clk2clk_drv(c));
}

int cc_compound_clk_get_pid(uint32_t id, uint32_t *parent_id);

int s32gen1_get_early_clks_freqs(const struct siul2_freq_mapping **mapping);

#endif /* S32GEN1_CLK_FUNCS_H */
