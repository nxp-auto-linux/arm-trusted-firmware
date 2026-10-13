/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2021, 2025-2026 NXP
 */
#ifndef S32G_CLK_FUNCS_H
#define S32G_CLK_FUNCS_H
#include <clk/s32cc_clk_funcs.h>

/* Clocks shared by all S32G SoCs */
extern const struct s32cc_clk_group s32g_clks;

/* PFE, SerDes and ACCEL PLL output clocks shared by S32G2 and S32G3 */
extern const struct s32cc_clk_group s32g2_g3_clks;

#endif /* S32G_CLK_FUNCS_H */

