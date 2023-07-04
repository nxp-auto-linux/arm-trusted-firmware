/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2020-2023 NXP
 */
#ifndef S32CC_SCMI_CLK_H
#define S32CC_SCMI_CLK_H

#include <clk/clk.h>
#include <stdint.h>
#include <stdbool.h>

#define S32GEN1_CLK_DRV_NAME	"plat_clks"

#define SCMI_ARRAY_ENTRY(ID, PLAT_ID, NAME) \
	[INDEX(ID)] = { .plat_id = (PLAT_ID), .name = (NAME), }

struct s32gen1_scmi_clk {
	uint32_t plat_id;
	const char *name;
};

int cc_scmi_id2clk(uint32_t scmi_clk_id, uint32_t *clk_id);
int cc_compound_clk_get(struct clk *clk);
unsigned long cc_compound_clk_get_rate(struct clk *clk);
unsigned long cc_compound_clk_set_rate(struct clk *clk, unsigned long rate);
int cc_compound_clk_enable(struct clk *clk, int enable);
int cc_set_mux_parent(struct clk *clk, uint32_t mux_id, uint32_t mux_source);
uint32_t cc_get_nclocks(void);
const char *cc_scmi_clk_get_name(uint32_t scmi_clk_id);
int cc_scmi_clk_get_rates(struct clk *clk, unsigned long *rates,
			  size_t *nrates);
unsigned long cc_scmi_clk_set_rate(struct clk *clk, unsigned long rate);

int plat_scmi_id2clk(uint32_t scmi_clk_id, uint32_t *clk_id);
int plat_compound_clk_get(struct clk *clk);
unsigned long plat_compound_clk_get_rate(struct clk *clk);
unsigned long plat_compound_clk_set_rate(struct clk *clk, unsigned long rate);
int plat_compound_clk_enable(struct clk *clk, int enable);
int plat_compound_clk_set_parents(struct clk *clk);
uint32_t plat_get_nclocks(void);
const char *plat_scmi_clk_get_name(uint32_t scmi_clk_id);
int plat_scmi_clk_get_rates(struct clk *clk, unsigned long *rates,
			    size_t *nrates);
unsigned long plat_scmi_clk_set_rate(struct clk *clk, unsigned long rate);

int s32gen1_scmi_request(uint32_t id, struct clk *clk);
unsigned long s32gen1_scmi_get_rate(struct clk *clk);
unsigned long s32gen1_scmi_set_rate(struct clk *clk, unsigned long rate);
int s32gen1_scmi_set_parent(struct clk *clk, struct clk *parent);
int s32gen1_scmi_enable(struct clk *clk, int enable);
uint32_t s32gen1_scmi_nclocks(void);
const char *s32gen1_scmi_clk_get_name(uint32_t scmi_clk_id);
int s32gen1_scmi_clk_get_rates(struct clk *clk, unsigned long *rates,
			       size_t *nrates);
unsigned long s32gen1_scmi_clk_get_rate(struct clk *clk);
unsigned long s32gen1_scmi_clk_set_rate(struct clk *clk, unsigned long rate);
int32_t plat_scmi_clock_agent_reset(unsigned int agent_id);
int32_t plat_scmi_clocks_reset_agents(void);
void update_a53_clk_state(bool enabled);

#endif

