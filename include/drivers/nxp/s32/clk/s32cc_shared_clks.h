/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2020-2021, 2023, 2025 NXP
 */
#ifndef S32CC_SHARED_CLKS_H
#define S32CC_SHARED_CLKS_H

extern struct s32cc_clk gmac_ts_clk;
extern struct s32cc_part part0;
extern struct s32cc_clk cgm0_mux7_clk;
extern struct s32cc_clk xbar_clk;
extern struct s32cc_clk xbar_div3_clk;
extern struct s32cc_clk xbar_2x_clk;
extern struct s32cc_clk xbar_div2_clk;
extern struct s32cc_cgm_div per_div;
extern struct s32cc_clk cgm0_mux0_clk;
extern struct s32cc_dfs armdfs;
extern struct s32cc_clk serdes1_lane0_tx_clk;
extern struct s32cc_clk serdes1_lane0_cdr_clk;
extern struct s32cc_clk accel_pll_phi0_clk;
extern struct s32cc_clk accel_pll_phi1_clk;
extern const struct siul2_freq_mapping siul2_clk_freq_map[];

#endif

