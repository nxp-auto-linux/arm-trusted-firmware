// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022-2024 NXP
 */
#include <dt-bindings/clock/s32r45-clock.h>
#include <dt-bindings/clock/s32cc-scmi-clock.h>
#include <dt-bindings/clock/s32gen1-clock-freq.h>
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_clk_modules.h>
#include <clk/s32gen1_scmi_clk.h>
#include <clk/s32gen1_shared_clks.h>

#define ARR_CLK(N)	S32R45_CLK_INDEX(N)

#define SIUL2_MIDR2_FREQ_VAL1		(0xA)

#define S32GEN1_XBAR_2X_MAX_FREQ	(800 * MHZ)

/* Part 0 blocks */
static struct s32gen1_part_block part0_block5 =
	S32GEN1_PART_BLOCK(&part0, s32gen1_part_block5);
static struct s32gen1_part_block part0_block6 =
	S32GEN1_PART_BLOCK(&part0, s32gen1_part_block6);
static struct s32gen1_part_block part0_block7 =
	S32GEN1_PART_BLOCK(&part0, s32gen1_part_block7);
static struct s32gen1_part_block part0_block8 =
	S32GEN1_PART_BLOCK(&part0, s32gen1_part_block8);
static struct s32gen1_part_block part0_block9 =
	S32GEN1_PART_BLOCK(&part0, s32gen1_part_block9);
static struct s32gen1_part_block part0_block11 =
	S32GEN1_PART_BLOCK(&part0, s32gen1_part_block11);
static struct s32gen1_part_block part0_block12 =
	S32GEN1_PART_BLOCK(&part0, s32gen1_part_block12);
static struct s32gen1_part_block part0_block13 =
	S32GEN1_PART_BLOCK(&part0, s32gen1_part_block13);
static struct s32gen1_part_block part0_block14 =
	S32GEN1_PART_BLOCK(&part0, s32gen1_part_block14);

/* Part 2 blocks */
struct s32gen1_part part2 = S32GEN1_PART(2);
static struct s32gen1_part_block part2_block0 =
	S32GEN1_PART_BLOCK(&part2, s32gen1_part_block0);
static struct s32gen1_part_block part2_block1 =
	S32GEN1_PART_BLOCK(&part2, s32gen1_part_block1);

/* Part 3 blocks */
struct s32gen1_part part3 = S32GEN1_PART(3);
static struct s32gen1_part_block part3_block1 =
	S32GEN1_PART_BLOCK_NO_STATUS(&part3, s32gen1_part_block1);
static struct s32gen1_part_block part3_block2 =
	S32GEN1_PART_BLOCK(&part3, s32gen1_part_block2);
static struct s32gen1_part_block part3_block3 =
	S32GEN1_PART_BLOCK(&part3, s32gen1_part_block3);
static struct s32gen1_part_block part3_block4 =
	S32GEN1_PART_BLOCK(&part3, s32gen1_part_block4);
static struct s32gen1_part_block part3_block5 =
	S32GEN1_PART_BLOCK(&part3, s32gen1_part_block5);

/* XBAR_2X */
static struct s32gen1_part_block_link xbar_div3_block_link =
		S32GEN1_PART_BLOCK_LINK(cgm0_mux0_clk, &part3_block2);
static struct s32gen1_part_block_link eim_block_link =
		S32GEN1_PART_BLOCK_LINK(xbar_div3_block_link,
					&part3_block3);
static struct s32gen1_part_block_link mipi20_block_link =
		S32GEN1_PART_BLOCK_LINK(eim_block_link, &part0_block5);
static struct s32gen1_part_block_link mipi21_block_link =
		S32GEN1_PART_BLOCK_LINK(mipi20_block_link, &part0_block6);
static struct s32gen1_part_block_link mipi22_block_link =
		S32GEN1_PART_BLOCK_LINK(mipi21_block_link, &part0_block7);
static struct s32gen1_part_block_link mipi23_block_link =
		S32GEN1_PART_BLOCK_LINK(mipi22_block_link, &part0_block8);
static struct s32gen1_part_block_link fdma_block_link =
		S32GEN1_PART_BLOCK_LINK(mipi23_block_link, &part0_block9);
struct s32gen1_clk xbar_2x_clk =
		S32GEN1_FREQ_MODULE_CLK_NO_FREQ_SCALING(fdma_block_link, 48 * MHZ, 800 * MHZ);

/* PER_CLK */
static struct s32gen1_part_block_link per_block_link =
		S32GEN1_PART_BLOCK_LINK(per_div, &part3_block1);
static struct s32gen1_clk per_clk =
		S32GEN1_FREQ_MODULE_CLK(per_block_link, 0, 80 * MHZ);
static struct s32gen1_fixed_div per_div4_div =
		S32GEN1_FIXED_DIV_INIT(per_clk, 4);
static struct s32gen1_clk per_div4_clk =
		S32GEN1_FREQ_MODULE_CLK_NO_FREQ_SCALING(per_div4_div, 0, 20 * MHZ);

/* CAN_PE_CLK */
static struct s32gen1_part_block_link can4_block_link =
		S32GEN1_PART_BLOCK_LINK(cgm0_mux7_clk, &part0_block11);
static struct s32gen1_part_block_link can5_block_link =
		S32GEN1_PART_BLOCK_LINK(can4_block_link, &part0_block12);
static struct s32gen1_part_block_link can6_block_link =
		S32GEN1_PART_BLOCK_LINK(can5_block_link, &part0_block13);
static struct s32gen1_part_block_link can7_block_link =
		S32GEN1_PART_BLOCK_LINK(can6_block_link, &part0_block14);
static struct s32gen1_clk can_pe_clk =
		S32GEN1_FREQ_MODULE_CLK(can7_block_link, 40 * MHZ, 80 * MHZ);

/* ARM DFS - PHI4 */
static struct s32gen1_dfs_div arm_dfs4_div =
		S32GEN1_DFS_DIV_INIT(armdfs, 3);
static struct s32gen1_clk arm_dfs4_clk =
		S32GEN1_FREQ_MODULE_CLK(arm_dfs4_div, 0, 400 * MHZ);
static struct s32gen1_clk arm_dfs4_2_clk =
		S32GEN1_CHILD_CLK(arm_dfs4_clk, 0, 400 * MHZ);

/* ACCEL3_CLK (SPT) */
static struct s32gen1_mux cgm2_mux0 =
		S32GEN1_MUX_INIT(S32GEN1_CGM2, 0, 2,
				 S32GEN1_CLK_FIRC,
				 S32R45_CLK_ACCEL_PLL_PHI0);
static struct s32gen1_clk cgm2_mux0_clk =
		S32GEN1_MODULE_CLK(cgm2_mux0);
static struct s32gen1_cgm_div cgm2_mux0_div =
		S32GEN1_CGM_DIV_INIT(cgm2_mux0_clk, 0);
static struct s32gen1_part_block_link bbe32ep_block_link =
		S32GEN1_PART_BLOCK_LINK(cgm2_mux0_div, &part3_block4);
static struct s32gen1_part_block_link spt_block_link =
		S32GEN1_PART_BLOCK_LINK(bbe32ep_block_link, &part3_block5);
static struct s32gen1_clk accel3_clk =
		S32GEN1_FREQ_MODULE_CLK_NO_FREQ_SCALING(spt_block_link,
							0, 600 * MHZ);
static struct s32gen1_fixed_div accle3_div3_div =
		S32GEN1_FIXED_DIV_INIT(accel3_clk, 2);
static struct s32gen1_clk accel3_div3_clk =
		S32GEN1_FREQ_MODULE_CLK_NO_FREQ_SCALING(accle3_div3_div, 0, 200 * MHZ);

/* LAX */
static struct s32gen1_mux cgm2_mux1 =
		S32GEN1_MUX_INIT(S32GEN1_CGM2, 1, 2,
				 S32GEN1_CLK_FIRC,
				 S32R45_CLK_ARM_PLL_DFS4_2);
static struct s32gen1_clk cgm2_mux1_clk =
		S32GEN1_MODULE_CLK(cgm2_mux1);
static struct s32gen1_cgm_div cgm2_mux1_div =
		S32GEN1_CGM_DIV_INIT(cgm2_mux1_clk, 0);
static struct s32gen1_clk accel4_clk =
		S32GEN1_FREQ_MODULE_CLK_NO_FREQ_SCALING(cgm2_mux1_div, 0, 400 * MHZ);
static struct s32gen1_part_block_link lax0_block_link =
		S32GEN1_PART_BLOCK_LINK(accel4_clk, &part2_block0);
static struct s32gen1_part_block_link lax1_block_link =
		S32GEN1_PART_BLOCK_LINK(accel4_clk, &part2_block1);
static struct s32gen1_clk lax_0_clk =
		S32GEN1_FREQ_MODULE_CLK_NO_FREQ_SCALING(lax0_block_link, 0, 400 * MHZ);
static struct s32gen1_clk lax_1_clk =
		S32GEN1_FREQ_MODULE_CLK_NO_FREQ_SCALING(lax1_block_link, 0, 400 * MHZ);

/* GMAC_TS_CLK */
static struct s32gen1_fixed_clock gmac_ext_ts =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac_ext_ts_clk =
		S32GEN1_MODULE_CLK(gmac_ext_ts);
static struct s32gen1_mux cgm0_mux9 =
		S32GEN1_MUX_INIT(S32GEN1_CGM0, 9, 3,
				 S32GEN1_CLK_FIRC,
				 S32GEN1_CLK_PERIPH_PLL_PHI4,
				 S32GEN1_CLK_GMAC0_EXT_TS);
static struct s32gen1_clk cgm0_mux9_clk =
		S32GEN1_MODULE_CLK(cgm0_mux9);
static struct s32gen1_cgm_div gmac_ts_div =
		S32GEN1_CGM_DIV_INIT(cgm0_mux9_clk, 0);
struct s32gen1_clk gmac_ts_clk =
		S32GEN1_FREQ_MODULE_CLK(gmac_ts_div, 5 * MHZ, 200 * MHZ);

/* GMAC0_TX_CLK */
static struct s32gen1_fixed_clock gmac0_ext_tx =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac0_ext_tx_clk =
		S32GEN1_MODULE_CLK(gmac0_ext_tx);

static struct s32gen1_fixed_clock gmac0_rmii_ref =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac0_rmii_ref_clk =
		S32GEN1_MODULE_CLK(gmac0_rmii_ref);

static struct s32gen1_mux cgm0_mux10 =
		S32GEN1_MUX_INIT(S32GEN1_CGM0, 10, 5,
				 S32GEN1_CLK_FIRC,
				 S32GEN1_CLK_PERIPH_PLL_PHI5,
				 S32GEN1_CLK_SERDES0_LANE0_TX,
				 S32GEN1_CLK_GMAC0_EXT_TX,
				 S32GEN1_CLK_GMAC0_RMII_REF);
static struct s32gen1_clk cgm0_mux10_clk =
		S32GEN1_MODULE_CLK(cgm0_mux10);
static struct s32gen1_cgm_div gmac_tx_div =
		S32GEN1_CGM_DIV_INIT(cgm0_mux10_clk, 0);
static struct s32gen1_clk gmac_tx_clk =
		S32GEN1_FREQ_MODULE_CLK(gmac_tx_div, 2500000, 125 * MHZ);

/* GMAC0_RX_CLK */
static struct s32gen1_fixed_clock gmac0_ext_rx =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac0_ext_rx_clk =
		S32GEN1_MODULE_CLK(gmac0_ext_rx);

static struct s32gen1_mux cgm0_mux11 =
		S32GEN1_MUX_INIT(S32GEN1_CGM0, 11, 4,
				 S32GEN1_CLK_FIRC,
				 S32GEN1_CLK_GMAC0_REF_DIV,
				 S32GEN1_CLK_GMAC0_EXT_RX,
				 S32GEN1_CLK_SERDES0_LANE0_CDR);
static struct s32gen1_clk cgm0_mux11_clk =
		S32GEN1_MODULE_CLK(cgm0_mux11);
static struct s32gen1_clk gmac_rx_clk =
		S32GEN1_CHILD_CLK(cgm0_mux11_clk, 2500000, 125 * MHZ);

/* GMAC0_REF_DIV_CLK */
static struct s32gen1_mux cgm0_mux15 =
		S32GEN1_SHARED_MUX_INIT(S32GEN1_CGM0, 15, 2,
					S32GEN1_CLK_FIRC,
					S32GEN1_CLK_GMAC0_RMII_REF);
static struct s32gen1_clk cgm0_mux15_clk =
		S32GEN1_MODULE_CLK(cgm0_mux15);
static struct s32gen1_cgm_div gmac0_ref_div =
		S32GEN1_CGM_DIV_INIT(cgm0_mux15_clk, 0);
static struct s32gen1_clk gmac0_ref_clk =
		S32GEN1_FREQ_MODULE_CLK(gmac0_ref_div, 0, 50 * MHZ);

/* GMAC1_EXT_TX_CLK */
static struct s32gen1_fixed_clock gmac1_ext_tx =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac1_ext_tx_clk =
		S32GEN1_MODULE_CLK(gmac1_ext_tx);

/* GMAC1_RMII_REF_CLK */
static struct s32gen1_fixed_clock gmac1_rmii_ref =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac1_rmii_ref_clk =
		S32GEN1_MODULE_CLK(gmac1_rmii_ref);

/* GMAC1_EXT_RX_CLK */
static struct s32gen1_fixed_clock gmac1_ext_rx =
		S32GEN1_FIXED_CLK_INIT();
static struct s32gen1_clk gmac1_ext_rx_clk =
		S32GEN1_MODULE_CLK(gmac1_ext_rx);

/* GMAC1_TX_CLK */
static struct s32gen1_mux cgm2_mux2 =
		S32GEN1_MUX_INIT(S32GEN1_CGM2, 2, 5,
				 S32GEN1_CLK_FIRC,
				 S32GEN1_CLK_PERIPH_PLL_PHI5,
				 S32R45_CLK_GMAC1_EXT_TX,
				 S32R45_CLK_GMAC1_RMII_REF,
				 S32R45_CLK_SERDES1_LANE0_TX);
static struct s32gen1_clk cgm2_mux2_clk =
		S32GEN1_MODULE_CLK(cgm2_mux2);
static struct s32gen1_cgm_div cgm2_mux2_div =
		S32GEN1_CGM_DIV_INIT(cgm2_mux2_clk, 0);
static struct s32gen1_clk gmac1_tx_clk =
		S32GEN1_FREQ_MODULE_CLK(cgm2_mux2_div, 2500000, 125 * MHZ);

/* GMAC1_REF_CLK */
static struct s32gen1_mux cgm2_mux3 =
		S32GEN1_MUX_INIT(S32GEN1_CGM2, 3, 2,
				 S32GEN1_CLK_FIRC,
				 S32R45_CLK_GMAC1_RMII_REF);
static struct s32gen1_clk cgm2_mux3_clk =
		S32GEN1_MODULE_CLK(cgm2_mux3);
static struct s32gen1_cgm_div gmac1_ref_div =
		S32GEN1_CGM_DIV_INIT(cgm2_mux3_clk, 0);
static struct s32gen1_clk gmac1_ref_div_clk =
		S32GEN1_FREQ_MODULE_CLK(gmac1_ref_div, 0, 50 * MHZ);

/* GMAC1_RX_CLK */
static struct s32gen1_mux cgm2_mux4 =
		S32GEN1_MUX_INIT(S32GEN1_CGM2, 4, 4,
				 S32GEN1_CLK_FIRC,
				 S32R45_CLK_GMAC1_REF_DIV,
				 S32R45_CLK_GMAC1_EXT_RX,
				 S32R45_CLK_SERDES1_LANE0_CDR);
static struct s32gen1_clk cgm2_mux4_clk =
		S32GEN1_MODULE_CLK(cgm2_mux4);
static struct s32gen1_clk gmac1_rx_clk =
		S32GEN1_FREQ_MODULE_CLK(cgm2_mux4_clk, 2500000, 125 * MHZ);

/* MIPICSI2 */
static struct s32gen1_part_block_link mipicsi2_0_block_link =
		S32GEN1_PART_BLOCK_LINK(xbar_clk, &part0_block6);
static struct s32gen1_clk mipicsi2_0_ctl_clk =
		S32GEN1_FREQ_MODULE_CLK(mipicsi2_0_block_link, 24 * MHZ, 400 * MHZ);
static struct s32gen1_part_block_link mipicsi2_1_block_link =
		S32GEN1_PART_BLOCK_LINK(xbar_clk, &part0_block5);
static struct s32gen1_clk mipicsi2_1_ctl_clk =
		S32GEN1_FREQ_MODULE_CLK(mipicsi2_1_block_link, 24 * MHZ, 400 * MHZ);
static struct s32gen1_part_block_link mipicsi2_2_block_link =
		S32GEN1_PART_BLOCK_LINK(xbar_clk, &part0_block8);
static struct s32gen1_clk mipicsi2_2_ctl_clk =
		S32GEN1_FREQ_MODULE_CLK(mipicsi2_2_block_link, 24 * MHZ, 400 * MHZ);
static struct s32gen1_part_block_link mipicsi2_3_block_link =
		S32GEN1_PART_BLOCK_LINK(xbar_clk, &part0_block7);
static struct s32gen1_clk mipicsi2_3_ctl_clk =
		S32GEN1_FREQ_MODULE_CLK(mipicsi2_3_block_link, 24 * MHZ, 400 * MHZ);

/* CTE PER */
static struct s32gen1_part_block_link cte_per_block_link =
		S32GEN1_PART_BLOCK_LINK(per_clk, &part3_block1);
static struct s32gen1_clk cte_pe_clk =
		S32GEN1_FREQ_MODULE_CLK(cte_per_block_link, 0, 80 * MHZ);

/* CTE XBAR_DIV3 */
static struct s32gen1_part_block_link cte_xbar_div3_block_link =
		S32GEN1_PART_BLOCK_LINK(xbar_div3_clk, &part3_block2);
static struct s32gen1_clk cte_xbar_div3_clk =
		S32GEN1_FREQ_MODULE_CLK(cte_xbar_div3_block_link, 8 * MHZ, 133333333);

/* BBE32EP DSP */
static struct s32gen1_part_block_link eim_dsp_xbar_div3_block_link =
		S32GEN1_PART_BLOCK_LINK(xbar_div3_clk, &part3_block3);
static struct s32gen1_clk eim_dsp_clk =
		S32GEN1_FREQ_MODULE_CLK(eim_dsp_xbar_div3_block_link, 8 * MHZ, 133333333);
static struct s32gen1_part_block_link bb332ep_dsp_block_link =
		S32GEN1_PART_BLOCK_LINK(accel3_clk, &part3_block4);
static struct s32gen1_clk bb332ep_dsp_clk =
		S32GEN1_FREQ_MODULE_CLK(bb332ep_dsp_block_link, 0, 600 * MHZ);

static struct s32gen1_clk *s32r45_cc_clocks[] = {
	[CC_ARR_CLK(S32GEN1_CLK_PER)] = &per_clk,
	[CC_ARR_CLK(S32GEN1_CLK_CAN_PE)] = &can_pe_clk,
	[CC_ARR_CLK(S32R45_CLK_ACCEL_PLL_PHI0)] = &accel_pll_phi0_clk,
	[CC_ARR_CLK(S32GEN1_CLK_ARM_PLL_DFS4)] = &arm_dfs4_clk,
	[CC_ARR_CLK(S32R45_CLK_ARM_PLL_DFS4_2)] = &arm_dfs4_2_clk,
	[CC_ARR_CLK(S32R45_CLK_GMAC1_EXT_TX)] = &gmac1_ext_tx_clk,
	[CC_ARR_CLK(S32R45_CLK_GMAC1_EXT_RX)] = &gmac1_ext_rx_clk,
	[CC_ARR_CLK(S32R45_CLK_GMAC1_RMII_REF)] = &gmac1_rmii_ref_clk,
	[CC_ARR_CLK(S32R45_CLK_SERDES1_LANE0_TX)] = &serdes1_lane0_tx_clk,
	[CC_ARR_CLK(S32R45_CLK_SERDES1_LANE0_CDR)] = &serdes1_lane0_cdr_clk,
	[CC_ARR_CLK(S32R45_CLK_GMAC1_REF_DIV)] = &gmac1_ref_div_clk,
	[CC_ARR_CLK(S32GEN1_CLK_MC_CGM0_MUX10)] = &cgm0_mux10_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_TX)] = &gmac_tx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_EXT_TS)] = &gmac_ext_ts_clk,
	[CC_ARR_CLK(S32GEN1_CLK_MC_CGM0_MUX9)] = &cgm0_mux9_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_TS)] = &gmac_ts_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_EXT_TX)] = &gmac0_ext_tx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_RMII_REF)] = &gmac0_rmii_ref_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_EXT_RX)] = &gmac0_ext_rx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_MC_CGM0_MUX11)] = &cgm0_mux11_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_RX)] = &gmac_rx_clk,
	[CC_ARR_CLK(S32GEN1_CLK_MC_CGM0_MUX15)] = &cgm0_mux15_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_REF_DIV)] = &gmac0_ref_clk,
	[CC_ARR_CLK(S32GEN1_CLK_GMAC0_REF)] = &gmac0_ref_clk,
};

static struct s32gen1_clk *s32r45_clocks[] = {
	[ARR_CLK(S32R45_CLK_MC_CGM2_MUX0)] = &cgm2_mux0_clk,
	[ARR_CLK(S32R45_CLK_ACCEL3)] = &accel3_clk,
	[ARR_CLK(S32R45_CLK_ACCEL3_DIV3)] = &accel3_div3_clk,
	[ARR_CLK(S32R45_CLK_MC_CGM2_MUX1)] = &cgm2_mux1_clk,
	[ARR_CLK(S32R45_CLK_ACCEL4)] = &accel4_clk,
	[ARR_CLK(S32R45_CLK_LAX_0_MODULE)] = &lax_0_clk,
	[ARR_CLK(S32R45_CLK_LAX_1_MODULE)] = &lax_1_clk,
	[ARR_CLK(S32R45_CLK_MC_CGM2_MUX2)] = &cgm2_mux2_clk,
	[ARR_CLK(S32R45_CLK_GMAC1_TX)] = &gmac1_tx_clk,
	[ARR_CLK(S32R45_CLK_MC_CGM2_MUX3)] = &cgm2_mux3_clk,
	[ARR_CLK(S32R45_CLK_MC_CGM2_MUX4)] = &cgm2_mux4_clk,
	[ARR_CLK(S32R45_CLK_GMAC1_RX)] = &gmac1_rx_clk,
	[ARR_CLK(S32R45_CLK_PER_DIV4)] = &per_div4_clk,
	[ARR_CLK(S32R45_CLK_MIPICSI2_0_CTL)] = &mipicsi2_0_ctl_clk,
	[ARR_CLK(S32R45_CLK_MIPICSI2_1_CTL)] = &mipicsi2_1_ctl_clk,
	[ARR_CLK(S32R45_CLK_MIPICSI2_2_CTL)] = &mipicsi2_2_ctl_clk,
	[ARR_CLK(S32R45_CLK_MIPICSI2_3_CTL)] = &mipicsi2_3_ctl_clk,
	[ARR_CLK(S32R45_CLK_CTE_PER)] = &cte_pe_clk,
	[ARR_CLK(S32R45_CLK_CTE_XBAR_DIV3)] = &cte_xbar_div3_clk,
	[ARR_CLK(S32R45_CLK_EIM_DSP)] = &eim_dsp_clk,
	[ARR_CLK(S32R45_CLK_BBE32EP_DSP)] = &bb332ep_dsp_clk,
};

struct s32gen1_clk *get_plat_cc_clock(uint32_t id)
{
	id -= S32GEN1_CLK_ID_BASE;

	if (id >= ARRAY_SIZE(s32r45_cc_clocks))
		return NULL;

	return s32r45_cc_clocks[id];
}

struct s32gen1_clk *get_plat_clock(uint32_t id)
{
	if (id < S32GEN1_PLAT_CLK_ID_BASE)
		return NULL;

	id -= S32GEN1_PLAT_CLK_ID_BASE;

	if (id >= ARRAY_SIZE(s32r45_clocks))
		return NULL;

	return s32r45_clocks[id];
}

int cc_compound_clk_get_pid(uint32_t id, uint32_t *parent_id)
{
	if (!parent_id)
		return -EINVAL;

	switch (id) {
	case S32CC_SCMI_CLK_GMAC0_RX_SGMII:
	case S32CC_SCMI_CLK_GMAC0_RX_RGMII:
	case S32CC_SCMI_CLK_GMAC0_RX_RMII:
	case S32CC_SCMI_CLK_GMAC0_RX_MII:
		*parent_id = S32GEN1_CLK_MC_CGM0_MUX11;
		break;
	case S32CC_SCMI_CLK_GMAC0_TX_SGMII:
	case S32CC_SCMI_CLK_GMAC0_TX_RGMII:
	case S32CC_SCMI_CLK_GMAC0_TX_RMII:
	case S32CC_SCMI_CLK_GMAC0_TX_MII:
		*parent_id = S32GEN1_CLK_MC_CGM0_MUX10;
		break;
	case S32CC_SCMI_CLK_GMAC0_TS:
		*parent_id = S32GEN1_CLK_MC_CGM0_MUX9;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

const struct siul2_freq_mapping siul2_clk_freq_map[] = {
	SIUL2_FREQ_MAP(SIUL2_MIDR2_FREQ_VAL1, S32GEN1_A53_MAX_FREQ,
			S32GEN1_ARM_PLL_VCO_MAX_FREQ,
			S32GEN1_ARM_PLL_PHI0_MAX_FREQ,
			S32GEN1_XBAR_2X_MAX_FREQ),
	{} /* empty entry */
};
