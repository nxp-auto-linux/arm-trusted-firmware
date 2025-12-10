/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32N_CLK_UTILS_H
#define S32N_CLK_UTILS_H

/**
 * enum clk_type - Clock type identifier
 * @CLK_TYPE_FIXED: Fixed clock (#clock-cells = 0)
 * @CLK_TYPE_SCMI: SCMI-managed clock (#clock-cells > 0)
 */
typedef enum {
	CLK_TYPE_FIXED,
	CLK_TYPE_SCMI
} clk_type_t;

struct clk_info {
	clk_type_t clock_type;
	uint32_t clk_scmi_id;
	uint32_t clk_freq;
};

int get_clock_info(struct clk_info *clk_info,
		   uintptr_t reg_base, const char *clk_name);

#endif /* S32N_CLK_UTILS_H */
