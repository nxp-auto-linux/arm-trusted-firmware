/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2020-2021 NXP
 */
#ifndef S32GEN1_CLK_MODULES_H
#define S32GEN1_CLK_MODULES_H

#include <common/debug.h>
#include <stdint.h>
#include <s32_bl_common.h>

#define MHZ	(1000000UL)

#define S32GEN1_OSC_INIT(SOURCE)       \
{                                      \
	.desc = {                      \
		.type = s32gen1_osc_t, \
	},                             \
	.source = (SOURCE),            \
}

#define S32GEN1_FIXED_CLK_INIT()             \
{                                            \
	.desc = {                            \
		.type = s32gen1_fixed_clk_t, \
	},                                   \
}

#define S32GEN1_MUX_TYPE_INIT(TYPE, MODULE, INDEX, NCLKS, ...) \
{                                                              \
	.desc = {                                              \
		.type = (TYPE),                                \
	},                                                     \
	.module = (MODULE),                                    \
	.index = (INDEX),                                      \
	.nclks = (NCLKS),                                      \
	.clkids = {__VA_ARGS__},                               \
}

#define S32GEN1_MUX_INIT(MODULE, INDEX, NCLKS, ...)  \
	S32GEN1_MUX_TYPE_INIT(s32gen1_mux_t, MODULE, \
			      INDEX, NCLKS, __VA_ARGS__)

#define S32GEN1_SHARED_MUX_INIT(MODULE, INDEX, NCLKS, ...)  \
	S32GEN1_MUX_TYPE_INIT(s32gen1_shared_mux_t, MODULE, \
			      INDEX, NCLKS, __VA_ARGS__)

#define S32GEN1_FIXED_DIV_INIT(PARENT, DIV_RATE) \
{                                                \
	.desc = {                                \
		.type = s32gen1_fixed_div_t,     \
	},                                       \
	.parent = &(PARENT).desc,                \
	.div = (DIV_RATE),                       \
}

#define S32GEN1_PLL_OUT_DIV_INIT(PARENT, INDEX)  \
{                                                \
	.desc = {                                \
		.type = s32gen1_pll_out_div_t,   \
	},                                       \
	.parent = &(PARENT).desc,                \
	.index = (INDEX),                        \
}

#define S32GEN1_DFS_DIV_INIT(PARENT, INDEX)      \
{                                                \
	.desc = {                                \
		.type = s32gen1_dfs_div_t,       \
	},                                       \
	.parent = &(PARENT).desc,                \
	.index = (INDEX),                        \
}

#define S32GEN1_CGM_DIV_INIT(PARENT, INDEX)      \
{                                                \
	.desc = {                                \
		.type = s32gen1_cgm_div_t,       \
	},                                       \
	.parent = &(PARENT).desc,                \
	.index = (INDEX),                        \
}

#define S32GEN1_FREQ_MODULE_CLK(PARENT_MODULE, MIN, MAX) \
{                                                        \
	.desc = {                                        \
		.type = s32gen1_clk_t,                   \
	},                                               \
	.module = &(PARENT_MODULE).desc,                 \
	.min_freq = (MIN),                               \
	.max_freq = (MAX),                               \
}

#define S32GEN1_MODULE_CLK(PARENT_MODULE) \
	S32GEN1_FREQ_MODULE_CLK(PARENT_MODULE, 0, 0)

#define S32GEN1_CHILD_CLK(PARENT, MIN, MAX) \
{                                           \
	.desc = {                           \
		.type = s32gen1_clk_t,      \
	},                                  \
	.pclock = &(PARENT),                \
	.min_freq = (MIN),                  \
	.max_freq = (MAX),                  \
}

#define S32GEN1_PART_BLOCK_STATUS(PARENT, PART, BLOCK, STATUS) \
{                                                              \
	.desc = {                                              \
		.type = s32gen1_part_block_t,                  \
	},                                                     \
	.parent = &(PARENT).desc,                              \
	.partition = (PART),                                   \
	.block = (BLOCK),                                      \
	.status = (STATUS),                                    \
}

#define S32GEN1_PART_BLOCK(PARENT, PART, BLOCK) \
	S32GEN1_PART_BLOCK_STATUS(PARENT, PART, BLOCK, true)

#define S32GEN1_PART_BLOCK_NO_STATUS(PARENT, PART, BLOCK) \
	S32GEN1_PART_BLOCK_STATUS(PARENT, PART, BLOCK, false)

#define SIUL2_FREQ_MAP(MIDR2, A53, VCO, PHI0, XBAR_2X)	\
{							\
	.siul2_midr2_freq = (MIDR2),			\
	.a53_freq = (A53),				\
	.arm_pll_vco_freq = (VCO),			\
	.arm_pll_phi0_freq = (PHI0),			\
	.xbar_2x_freq = (XBAR_2X),			\
}

struct s32gen1_clk_priv {
	void *accelpll;
	void *armdfs;
	void *armpll;
	void *cgm0;
	void *cgm1;
	void *cgm2;
	void *cgm5;
	void *cgm6;
	void *ddrpll;
	void *fxosc;
	void *mc_me;
	void *periphdfs;
	void *periphpll;
	void *rdc;
	void *rgm;
};

enum s32gen1_clk_source {
	S32GEN1_ACCEL_PLL,
	S32GEN1_ARM_DFS,
	S32GEN1_ARM_PLL,
	S32GEN1_CGM0,
	S32GEN1_CGM1,
	S32GEN1_CGM2,
	S32GEN1_CGM5,
	S32GEN1_CGM6,
	S32GEN1_DDR_PLL,
	S32GEN1_FIRC,
	S32GEN1_FXOSC,
	S32GEN1_PERIPH_DFS,
	S32GEN1_PERIPH_PLL,
	S32GEN1_SIRC,
};

enum s32gen1_clkm_type {
	s32gen1_osc_t,
	s32gen1_fixed_clk_t,
	s32gen1_pll_t,
	s32gen1_dfs_t,
	s32gen1_mux_t,
	s32gen1_shared_mux_t,
	s32gen1_fixed_div_t,
	s32gen1_pll_out_div_t,
	s32gen1_dfs_div_t,
	s32gen1_cgm_div_t,
	s32gen1_part_block_t,
	s32gen1_clk_t,
};

enum s32gen1_part_block_type {
	s32gen1_part_block0,
	s32gen1_part_block1,
	s32gen1_part_block2,
	s32gen1_part_block3,
	s32gen1_part_block4,
	s32gen1_part_block5,
	s32gen1_part_block6,
	s32gen1_part_block7,
	s32gen1_part_block8,
	s32gen1_part_block9,
	s32gen1_part_block10,
	s32gen1_part_block11,
	s32gen1_part_block12,
	s32gen1_part_block13,
	s32gen1_part_block14,
	s32gen1_part_block15,
};

struct s32gen1_clk_obj {
	enum s32gen1_clkm_type type;
	uint32_t refcount;
};

struct s32gen1_clk {
	struct s32gen1_clk_obj desc;
	struct s32gen1_clk_obj *module;
	struct s32gen1_clk *pclock;
	unsigned long min_freq;
	unsigned long max_freq;
};

struct s32gen1_osc {
	struct s32gen1_clk_obj desc;
	enum s32gen1_clk_source source;
	unsigned long freq;
	void *base;
};

struct s32gen1_fixed_clock {
	struct s32gen1_clk_obj desc;
	unsigned long freq;
};

struct s32gen1_part_block {
	struct s32gen1_clk_obj desc;
	struct s32gen1_clk_obj *parent;
	uint32_t partition;
	enum s32gen1_part_block_type block;
	bool status;
};

struct s32gen1_pll {
	struct s32gen1_clk_obj desc;
	struct s32gen1_clk_obj *source;
	enum s32gen1_clk_source instance;
	unsigned long vco_freq;
	uint32_t ndividers;
	void *base;
};

struct s32gen1_dfs {
	struct s32gen1_clk_obj desc;
	struct s32gen1_clk_obj *source;
	enum s32gen1_clk_source instance;
	void *base;
};

struct s32gen1_mux {
	struct s32gen1_clk_obj desc;
	enum s32gen1_clk_source module;
	uint8_t index;	/* Mux index in parent module */
	uint32_t source_id;	/* Selected source */
	uint8_t nclks;	/* Number of output clocks */
	uint32_t clkids[];	/* IDs of the output clocks */
};

struct s32gen1_pll_out_div {
	struct s32gen1_clk_obj desc;
	struct s32gen1_clk_obj *parent;
	uint32_t index;
	unsigned long freq;
};

struct s32gen1_dfs_div {
	struct s32gen1_clk_obj desc;
	struct s32gen1_clk_obj *parent;
	uint32_t index;
	unsigned long freq;
};

struct s32gen1_cgm_div {
	struct s32gen1_clk_obj desc;
	struct s32gen1_clk_obj *parent;
	uint32_t index;
	unsigned long freq;
};

struct s32gen1_fixed_div {
	struct s32gen1_clk_obj desc;
	struct s32gen1_clk_obj *parent;
	uint32_t div;
};

/* Map values read from SIUL2_MIDR2 register to actual frequencies */
struct siul2_freq_mapping {
	uint32_t siul2_midr2_freq;
	unsigned long a53_freq;
	unsigned long arm_pll_vco_freq;
	unsigned long arm_pll_phi0_freq;
	unsigned long xbar_2x_freq;
};

static inline struct s32gen1_pll *obj2pll(struct s32gen1_clk_obj *mod)
{
	return container_of(mod, struct s32gen1_pll, desc);
}

static inline struct s32gen1_pll_out_div *obj2plldiv(struct s32gen1_clk_obj
						     *mod)
{
	return container_of(mod, struct s32gen1_pll_out_div, desc);
}

static inline struct s32gen1_dfs_div *obj2dfsdiv(struct s32gen1_clk_obj *mod)
{
	return container_of(mod, struct s32gen1_dfs_div, desc);
}

static inline struct s32gen1_dfs *obj2dfs(struct s32gen1_clk_obj *mod)
{
	return container_of(mod, struct s32gen1_dfs, desc);
}

static inline struct s32gen1_cgm_div *obj2cgmdiv(struct s32gen1_clk_obj *mod)
{
	return container_of(mod, struct s32gen1_cgm_div, desc);
}

static inline struct s32gen1_osc *obj2osc(struct s32gen1_clk_obj *mod)
{
	return container_of(mod, struct s32gen1_osc, desc);
}

static inline struct s32gen1_fixed_clock *obj2fixedclk(struct s32gen1_clk_obj
						       *mod)
{
	return container_of(mod, struct s32gen1_fixed_clock, desc);
}

static inline struct s32gen1_fixed_div *obj2fixeddiv(struct s32gen1_clk_obj
						     *mod)
{
	return container_of(mod, struct s32gen1_fixed_div, desc);
}

static inline struct s32gen1_part_block *obj2partblock(struct s32gen1_clk_obj
						       *mod)
{
	return container_of(mod, struct s32gen1_part_block, desc);
}

static inline struct s32gen1_mux *obj2mux(struct s32gen1_clk_obj *mod)
{
	return container_of(mod, struct s32gen1_mux, desc);
}

static inline struct s32gen1_clk *obj2clk(struct s32gen1_clk_obj *mod)
{
	return container_of(mod, struct s32gen1_clk, desc);
}

static inline bool is_mux(struct s32gen1_clk *clk)
{
	struct s32gen1_clk_obj *module;

	module = clk->module;
	if (!module)
		return NULL;

	return module->type == s32gen1_mux_t ||
	       module->type == s32gen1_shared_mux_t;
}

static inline struct s32gen1_mux *clk2mux(struct s32gen1_clk *clk)
{
	if (!is_mux(clk))
		return NULL;

	return container_of(clk->module, struct s32gen1_mux, desc);
}

static inline bool is_osc(struct s32gen1_clk *clk)
{
	struct s32gen1_clk_obj *module;

	module = clk->module;
	if (!module)
		return NULL;

	return (module->type == s32gen1_osc_t);
}

static inline bool is_fixed_clk(struct s32gen1_clk *clk)
{
	struct s32gen1_clk_obj *module;

	module = clk->module;
	if (!module)
		return NULL;

	return (module->type == s32gen1_fixed_clk_t);
}

static inline struct s32gen1_dfs *get_div_dfs(struct s32gen1_dfs_div *div)
{
	struct s32gen1_clk_obj *parent = div->parent;

	if (parent->type != s32gen1_dfs_t) {
		ERROR("DFS DIV doesn't have a DFS as parent\n");
		return NULL;
	}

	return obj2dfs(parent);
}

static inline struct s32gen1_pll *get_div_pll(struct s32gen1_pll_out_div *div)
{
	struct s32gen1_clk_obj *parent;

	parent = div->parent;
	if (!parent) {
		ERROR("Failed to identify PLL divider's parent\n");
		return NULL;
	}

	if (parent->type != s32gen1_pll_t) {
		ERROR("The parent of the divider is not a PLL instance\n");
		return NULL;
	}

	return obj2pll(parent);
}

static inline struct s32gen1_mux *get_cgm_div_mux(struct s32gen1_cgm_div *div)
{
	struct s32gen1_clk_obj *parent = div->parent;
	struct s32gen1_clk_obj *mux_obj;
	struct s32gen1_clk *clk;

	if (!parent) {
		ERROR("Failed to identify CGm DIV's parent\n");
		return NULL;
	}

	if (parent->type != s32gen1_clk_t) {
		ERROR("The parent of the CGM DIV isn't a clock\n");
		return NULL;
	}

	clk = obj2clk(parent);

	if (!clk->module) {
		ERROR("The clock isn't connected to a module\n");
		return NULL;
	}

	mux_obj = clk->module;

	if (mux_obj->type != s32gen1_mux_t &&
	    mux_obj->type != s32gen1_shared_mux_t) {
		ERROR("The parent of the CGM DIV isn't a MUX\n");
		return NULL;
	}

	return obj2mux(mux_obj);
}

#endif /* S32GEN1_CLK_MODULES_H */
