/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2021, 2025-2026 NXP
 */
#ifndef S32G_CLK_FUNCS_H
#define S32G_CLK_FUNCS_H
#include <clk/s32cc_clk_funcs.h>

struct s32cc_clk *s32g_get_plat_cc_clock(uint32_t id);
struct s32cc_clk *s32g_get_plat_clock(uint32_t id);

/* S32G2/G3 SoC-specific clocks (implemented per family: G2, G3) */
struct s32cc_clk *s32g2g3_get_soc_cc_clk(uint32_t id);
struct s32cc_clk *s32g2g3_get_soc_clk(uint32_t id);

#endif /* S32G_CLK_FUNCS_H */

