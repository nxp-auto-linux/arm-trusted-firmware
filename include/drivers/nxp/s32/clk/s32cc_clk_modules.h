/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2020-2025 NXP
 */
#ifndef S32CC_CLK_MODULES_H
#define S32CC_CLK_MODULES_H

#include <common/debug.h>
#include <stdint.h>
#include <s32cc_bl_common.h>

#define MHZ	(1000000UL)

#define S32CC_OSC_INIT(SOURCE)       \
{                                      \
	.desc = {                      \
		.type = s32cc_osc_t, \
	},                             \
	.source = (SOURCE),            \
}

#define S32CC_FIXED_CLK_INIT()             \
{                                            \
	.desc = {                            \
		.type = s32cc_fixed_clk_t, \
	},                                   \
}

#define S32CC_MUX_TYPE_INIT(TYPE, MODULE, INDEX, NCLKS, ...) \
{                                                              \
	.desc = {                                              \
		.type = (TYPE),                                \
	},                                                     \
	.module = (MODULE),                                    \
	.index = (INDEX),                                      \
	.nclks = (NCLKS),                                      \
	.clkids = {__VA_ARGS__},                               \
}

#define S32CC_MUX_INIT(MODULE, INDEX, NCLKS, ...)  \
	S32CC_MUX_TYPE_INIT(s32cc_mux_t, MODULE, \
			      INDEX, NCLKS, __VA_ARGS__)

#define S32CC_SHARED_MUX_INIT(MODULE, INDEX, NCLKS, ...)  \
	S32CC_MUX_TYPE_INIT(s32cc_shared_mux_t, MODULE, \
			      INDEX, NCLKS, __VA_ARGS__)

#define S32CC_CGM_SW_CTRL_MUX_INIT(MODULE, INDEX, NCLKS, ...)  \
	S32CC_MUX_TYPE_INIT(s32cc_cgm_sw_ctrl_mux_t, MODULE, \
			      INDEX, NCLKS, __VA_ARGS__)

#define S32CC_FIXED_DIV_INIT_TABLE_SIZE(PARENT, DIV_RATE, \
		FIXED_PFREQ_TABLE, SIZE) \
{                                                \
	.desc = {                                \
		.type = s32cc_fixed_div_t,     \
	},                                       \
	.parent = &(PARENT).desc,                \
	.div = (DIV_RATE),                       \
	.table = (FIXED_PFREQ_TABLE),               \
	.n_mappings = (SIZE),               \
}

#define S32CC_FIXED_DIV_INIT(PARENT, DIV_RATE) \
	S32CC_FIXED_DIV_INIT_TABLE_SIZE(PARENT, DIV_RATE, NULL, 0)

#define S32CC_FIXED_DIV_INIT_TABLE(PARENT, DIV_RATE, FIXED_PFREQ_TABLE) \
				   S32CC_FIXED_DIV_INIT_TABLE_SIZE(PARENT, DIV_RATE, \
				   &(FIXED_PFREQ_TABLE)[0], ARRAY_SIZE(FIXED_PFREQ_TABLE))

#define S32CC_PLL_OUT_DIV_INIT(PARENT, INDEX)  \
{                                                \
	.desc = {                                \
		.type = s32cc_pll_out_div_t,   \
	},                                       \
	.parent = &(PARENT).desc,                \
	.index = (INDEX),						\
	.child_mux = NULL,						\
}


#define S32CC_PLL_OUT_DIV_INIT_MUX(PARENT, INDEX, MUX)  \
{                                                \
	.desc = {                                \
		.type = s32cc_pll_out_div_t,   \
	},                                       \
	.parent = &(PARENT).desc,                \
	.index = (INDEX),                       \
	.child_mux = (MUX),						\
}

#define S32CC_DFS_DIV_INIT(PARENT, INDEX)      \
{                                                \
	.desc = {                                \
		.type = s32cc_dfs_div_t,       \
	},                                       \
	.parent = &(PARENT).desc,                \
	.index = (INDEX),                        \
}

#define S32CC_CGM_DIV_INIT(PARENT, INDEX)      \
{                                                \
	.desc = {                                \
		.type = s32cc_cgm_div_t,       \
	},                                       \
	.parent = &(PARENT).desc,                \
	.index = (INDEX),                        \
}

#define S32CC_FREQ_MODULE(PARENT_MODULE, MIN, MAX, FREQ_SCALING) \
{                                                        \
	.desc = {                                        \
		.type = s32cc_clk_t,                   \
	},                                               \
	.module = &(PARENT_MODULE).desc,                 \
	.min_freq = (MIN),                               \
	.max_freq = (MAX),                               \
	.freq_scaling = (FREQ_SCALING),					 \
}

#define S32CC_FREQ_MODULE_CLK_NO_FREQ_SCALING(PARENT_MODULE, MIN, MAX) \
	S32CC_FREQ_MODULE(PARENT_MODULE, MIN, MAX, false)

#define S32CC_FREQ_MODULE_CLK(PARENT_MODULE, MIN, MAX)		\
	S32CC_FREQ_MODULE(PARENT_MODULE, MIN, MAX, true)

#define S32CC_MODULE_CLK(PARENT_MODULE) \
	S32CC_FREQ_MODULE_CLK(PARENT_MODULE, 0, 0)

#define S32CC_CHILD(PARENT, MIN, MAX, FREQ_SCALING) \
{                                           \
	.desc = {                           \
		.type = s32cc_clk_t,      \
	},                                  \
	.pclock = &(PARENT),                \
	.min_freq = (MIN),                  \
	.max_freq = (MAX),                  \
	.freq_scaling = (FREQ_SCALING),		\
}

#define S32CC_CHILD_CLK(PARENT, MIN, MAX) \
	S32CC_CHILD(PARENT, MIN, MAX, true)

#define S32CC_PART(PART_NUM)          \
{                                       \
	.desc = {                       \
		.type = s32cc_part_t, \
	},                              \
	.partition_id = (PART_NUM),     \
}

#define S32CC_PART_BLOCK_STATUS(PART_META, BLOCK_TYPE, STATUS) \
{                                                                \
	.desc = {                                                \
		.type = s32cc_part_block_t,                    \
	},                                                       \
	.part = (PART_META),                                     \
	.block = (BLOCK_TYPE),                                   \
	.status = (STATUS),                                      \
}

#define S32CC_PART_LINK(PARENT, PARTITION)  \
{                                             \
	.desc = {                             \
		.type = s32cc_part_link_t,  \
	},                                    \
	.parent = &(PARENT).desc,             \
	.part = (PARTITION),                  \
}

#define S32CC_PART_BLOCK_LINK(PARENT, BLOCK)      \
{                                                   \
	.desc = {                                   \
		.type = s32cc_part_block_link_t,  \
	},                                          \
	.parent = &(PARENT).desc,                   \
	.block = (BLOCK),                           \
}

#define S32CC_PART_BLOCK(PARENT, BLOCK_TYPE) \
	S32CC_PART_BLOCK_STATUS(PARENT, BLOCK_TYPE, true)

#define S32CC_PART_BLOCK_NO_STATUS(PARENT, BLOCK_TYPE) \
	S32CC_PART_BLOCK_STATUS(PARENT, BLOCK_TYPE, false)

#define SIUL2_FREQ_MAP(MIDR2, A53, VCO, PHI0, XBAR_2X)	\
{							\
	.siul2_midr2_freq = (MIDR2),			\
	.a53_freq = (A53),				\
	.arm_pll_vco_freq = (VCO),			\
	.arm_pll_phi0_freq = (PHI0),			\
	.xbar_2x_freq = (XBAR_2X),			\
}

struct s32cc_clk_priv {
	void *accelpll;
	void *armdfs;
	void *armpll;
	void *cgm0;
	void *cgm1;
	void *cgm2;
	void *cgm3;
	void *cgm5;
	void *cgm6;
	void *ddrpll;
	void *ddrdfs;
	void *fxosc;
	void *mc_me;
	void *periphdfs;
	void *periphpll;
	void *rdc;
	void *rgm;
};

enum s32cc_clk_source {
	S32CC_ACCEL_PLL,
	S32CC_ARM_DFS,
	S32CC_ARM_PLL,
	S32CC_CGM0,
	S32CC_CGM1,
	S32CC_CGM2,
	S32CC_CGM3,
	S32CC_CGM5,
	S32CC_CGM6,
	S32CC_DDR_DFS,
	S32CC_DDR_PLL,
	S32CC_FIRC,
	S32CC_FXOSC,
	S32CC_PERIPH_DFS,
	S32CC_PERIPH_PLL,
	S32CC_SIRC,
};

enum s32cc_clkm_type {
	s32cc_osc_t,
	s32cc_fixed_clk_t,
	s32cc_pll_t,
	s32cc_dfs_t,
	s32cc_mux_t,
	s32cc_shared_mux_t,
	s32cc_cgm_sw_ctrl_mux_t,
	s32cc_fixed_div_t,
	s32cc_pll_out_div_t,
	s32cc_dfs_div_t,
	s32cc_cgm_div_t,
	s32cc_part_t,
	s32cc_part_link_t,
	s32cc_part_block_t,
	s32cc_part_block_link_t,
	s32cc_clk_t,
};

enum s32cc_part_block_type {
	s32cc_part_block0 = 0,
	s32cc_part_block1 = 1,
	s32cc_part_block2 = 2,
	s32cc_part_block3 = 3,
	s32cc_part_block4 = 4,
	s32cc_part_block5 = 5,
	s32cc_part_block6 = 6,
	s32cc_part_block7 = 7,
	s32cc_part_block8 = 8,
	s32cc_part_block9 = 9,
	s32cc_part_block10 = 10,
	s32cc_part_block11 = 11,
	s32cc_part_block12 = 12,
	s32cc_part_block13 = 13,
	s32cc_part_block14 = 14,
	s32cc_part_block15 = 15,
	s32cc_part_block31 = 31,
};

struct s32cc_clk_obj {
	enum s32cc_clkm_type type;
	uint32_t refcount;
};

struct s32cc_clk {
	struct s32cc_clk_obj desc;
	struct s32cc_clk_obj *module;
	struct s32cc_clk *pclock;
	unsigned long min_freq;
	unsigned long max_freq;
	bool freq_scaling;
};

struct s32cc_osc {
	struct s32cc_clk_obj desc;
	enum s32cc_clk_source source;
	unsigned long freq;
	void *base;
};

struct s32cc_fixed_clock {
	struct s32cc_clk_obj desc;
	unsigned long freq;
};

struct s32cc_part {
	struct s32cc_clk_obj desc;
	uint32_t partition_id;
};

struct s32cc_part_link {
	struct s32cc_clk_obj desc;
	struct s32cc_clk_obj *parent;
	struct s32cc_part *part;
};

struct s32cc_part_block {
	struct s32cc_clk_obj desc;
	struct s32cc_part *part;
	enum s32cc_part_block_type block;
	bool status;
};

struct s32cc_part_block_link {
	struct s32cc_clk_obj desc;
	struct s32cc_clk_obj *parent;
	struct s32cc_part_block *block;
};

struct s32cc_pll {
	struct s32cc_clk_obj desc;
	struct s32cc_clk_obj *source;
	enum s32cc_clk_source instance;
	unsigned long vco_freq;
	uint32_t ndividers;
	void *base;
	bool no_pllfd;
};

struct s32cc_dfs {
	struct s32cc_clk_obj desc;
	struct s32cc_clk_obj *source;
	enum s32cc_clk_source instance;
	void *base;
};

struct s32cc_mux {
	struct s32cc_clk_obj desc;
	enum s32cc_clk_source module;
	uint8_t index;	/* Mux index in parent module */
	uint32_t source_id;	/* Selected source */
	uint8_t nclks;	/* Number of output clocks */
	uint32_t clkids[];	/* IDs of the output clocks */
};

struct s32cc_pll_out_div {
	struct s32cc_clk_obj desc;
	struct s32cc_clk_obj *parent;
	struct s32cc_mux *child_mux;
	uint32_t index;
	unsigned long freq;
};

struct s32cc_dfs_div {
	struct s32cc_clk_obj desc;
	struct s32cc_clk_obj *parent;
	uint32_t index;
	unsigned long freq;
};

struct s32cc_cgm_div {
	struct s32cc_clk_obj desc;
	struct s32cc_clk_obj *parent;
	uint32_t index;
	unsigned long freq;
};

struct s32cc_fixed_div {
	struct s32cc_clk_obj desc;
	struct s32cc_clk_obj *parent;
	uint32_t div;
	struct freq_div_mapping *table;
	size_t n_mappings;
};

/* Map values read from SIUL2_MIDR2 register to actual frequencies */
struct siul2_freq_mapping {
	uint32_t siul2_midr2_freq;
	unsigned long a53_freq;
	unsigned long arm_pll_vco_freq;
	unsigned long arm_pll_phi0_freq;
	unsigned long xbar_2x_freq;
};

struct s32cc_clk_rates {
	unsigned long *rates;
	size_t *nrates;
};

struct freq_div_mapping {
	unsigned long freq;
	unsigned long pfreq;
};

static inline struct s32cc_pll *obj2pll(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_pll, desc);
}

static inline struct s32cc_pll_out_div *obj2plldiv(struct s32cc_clk_obj
						     *mod)
{
	return container_of(mod, struct s32cc_pll_out_div, desc);
}

static inline struct s32cc_dfs_div *obj2dfsdiv(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_dfs_div, desc);
}

static inline struct s32cc_dfs *obj2dfs(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_dfs, desc);
}

static inline struct s32cc_cgm_div *obj2cgmdiv(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_cgm_div, desc);
}

static inline struct s32cc_osc *obj2osc(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_osc, desc);
}

static inline struct s32cc_fixed_clock *obj2fixedclk(struct s32cc_clk_obj
						       *mod)
{
	return container_of(mod, struct s32cc_fixed_clock, desc);
}

static inline struct s32cc_fixed_div *obj2fixeddiv(struct s32cc_clk_obj
						     *mod)
{
	return container_of(mod, struct s32cc_fixed_div, desc);
}

static inline struct s32cc_part *obj2part(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_part, desc);
}

static inline struct s32cc_part_link *
obj2partlink(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_part_link, desc);
}

static inline struct s32cc_part_block *
obj2partblock(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_part_block, desc);
}

static inline struct s32cc_part_block_link *
obj2partblocklink(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_part_block_link, desc);
}

static inline struct s32cc_mux *obj2mux(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_mux, desc);
}

static inline struct s32cc_clk *obj2clk(struct s32cc_clk_obj *mod)
{
	return container_of(mod, struct s32cc_clk, desc);
}

static inline bool is_mux(struct s32cc_clk *clk)
{
	struct s32cc_clk_obj *module;

	module = clk->module;
	if (!module)
		return NULL;

	return module->type == s32cc_mux_t ||
	       module->type == s32cc_shared_mux_t ||
	       module->type == s32cc_cgm_sw_ctrl_mux_t;
}

static inline struct s32cc_mux *clk2mux(struct s32cc_clk *clk)
{
	if (!is_mux(clk))
		return NULL;

	return container_of(clk->module, struct s32cc_mux, desc);
}

static inline bool is_osc(struct s32cc_clk *clk)
{
	struct s32cc_clk_obj *module;

	module = clk->module;
	if (!module)
		return NULL;

	return (module->type == s32cc_osc_t);
}

static inline bool is_fixed_clk(struct s32cc_clk *clk)
{
	struct s32cc_clk_obj *module;

	module = clk->module;
	if (!module)
		return NULL;

	return (module->type == s32cc_fixed_clk_t);
}

static inline struct s32cc_dfs *get_div_dfs(struct s32cc_dfs_div *div)
{
	struct s32cc_clk_obj *parent = div->parent;

	if (parent->type != s32cc_dfs_t) {
		ERROR("DFS DIV doesn't have a DFS as parent\n");
		return NULL;
	}

	return obj2dfs(parent);
}

static inline struct s32cc_pll *get_div_pll(struct s32cc_pll_out_div *div)
{
	struct s32cc_clk_obj *parent;

	parent = div->parent;
	if (!parent) {
		ERROR("Failed to identify PLL divider's parent\n");
		return NULL;
	}

	if (parent->type != s32cc_pll_t) {
		ERROR("The parent of the divider is not a PLL instance\n");
		return NULL;
	}

	return obj2pll(parent);
}

static inline struct s32cc_mux *get_cgm_div_mux(struct s32cc_cgm_div *div)
{
	struct s32cc_clk_obj *parent = div->parent;
	struct s32cc_clk_obj *mux_obj;
	struct s32cc_clk *clk;

	if (!parent) {
		ERROR("Failed to identify CGm DIV's parent\n");
		return NULL;
	}

	if (parent->type != s32cc_clk_t) {
		ERROR("The parent of the CGM DIV isn't a clock\n");
		return NULL;
	}

	clk = obj2clk(parent);

	if (!clk->module) {
		ERROR("The clock isn't connected to a module\n");
		return NULL;
	}

	mux_obj = clk->module;

	if (mux_obj->type != s32cc_mux_t &&
	    mux_obj->type != s32cc_shared_mux_t &&
	    mux_obj->type != s32cc_cgm_sw_ctrl_mux_t) {
		ERROR("The parent of the CGM DIV isn't a MUX\n");
		return NULL;
	}

	return obj2mux(mux_obj);
}

#endif /* S32CC_CLK_MODULES_H */
