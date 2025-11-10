/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_CLK_GETTER_H
#define FCONF_CLK_GETTER_H

#include <s32n_clk_utils.h>
#include <lib/fconf/fconf.h>

#define hw_config__clk_config_getter(prop) clk_config.prop

/**
 * struct fconf_clk_desc - Clock descriptor
 * @id: Clock ID (SCMI clock ID or UINT_MAX for fixed clocks)
 * @type: Clock type (CLK_TYPE_FIXED or CLK_TYPE_SCMI)
 * @freq: Clock frequency (for fixed clocks) or 0 (for SCMI clocks)
 * @name: Clock name from device tree
 */
struct fconf_clk_desc {
	uint32_t id;
	clk_type_t type;
	uint32_t freq;
	const char *name;
};

struct clk_config_entry {
	uintptr_t reg_base;
	struct fconf_clk_desc *clock_descs;
	uint32_t clock_descs_n_elem;
};

struct clk_config {
	uint32_t entries_n_elem;
	struct clk_config_entry *entries;
};

extern struct clk_config clk_config;

int fconf_populate_clk_config(uintptr_t config);

#endif /* FCONF_CLK_GETTER_H */
