/*
 * Copyright 2019-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _S32G_CLOCKS_H_
#define _S32G_CLOCKS_H_

#include <dt-bindings/clock/s32gen1-clock-freq.h>
#include <stdint.h>
#include <stdbool.h>
#include <s32cc_clocks.h>

/*
 * PLL configuration
 */

/* This should be kept in sync with the CORE_PLL
 * configuration (MFI, MFN, RDIV). Due to not
 * having floating point support, it is impossible
 * to accurately calculate it at runtime.
 */
#define CORE_PLL_FVCO	(2000000000ul)

/*
 * Arrays of PHI frequencies
 */
#define CORE_PLL_PHI0_FREQ	(1000000000ull)
#define CORE_PLL_PHI1_FREQ	(500000000ull)
static const uint64_t s32g_core_pll_phi_freq[] = {
	CORE_PLL_PHI0_FREQ,
	CORE_PLL_PHI1_FREQ,
};
#define PERIPH_PLL_PHI0_FREQ	(100000000ull)
#define PERIPH_PLL_PHI1_FREQ	(80000000ull)
#define PERIPH_PLL_PHI2_FREQ	(80000000ull)
#define PERIPH_PLL_PHI3_FREQ	(125000000ull)
#define PERIPH_PLL_PHI4_FREQ	(200000000ull)
#define PERIPH_PLL_PHI5_FREQ	(125000000ull)
#define PERIPH_PLL_PHI6_FREQ	(100000000ull)
#define PERIPH_PLL_PHI7_FREQ	(100000000ull)
static const uint64_t s32g_periph_pll_phi_freq[] = {
	PERIPH_PLL_PHI0_FREQ,
	PERIPH_PLL_PHI1_FREQ,
	PERIPH_PLL_PHI2_FREQ,
	PERIPH_PLL_PHI3_FREQ,
	PERIPH_PLL_PHI4_FREQ,
	PERIPH_PLL_PHI5_FREQ,
	PERIPH_PLL_PHI6_FREQ,
	PERIPH_PLL_PHI7_FREQ,
};
#define ACCEL_PLL_PHI0_FREQ	(600000000ull)
#define ACCEL_PLL_PHI1_FREQ	(600000000ull)
static const uint64_t s32g_accel_pll_phi_freq[] = {
	ACCEL_PLL_PHI0_FREQ,
	ACCEL_PLL_PHI1_FREQ,
};
static const uint64_t s32g_ddr_pll_phi_freq[] = {
	S32GEN1_DDR_FREQ,
};

/*
 * DFS configuration
 */

enum S32G_DFS_PARAMS {
	DFS_PORT_EN = 0,
	DFS_DVPORT_MFN,
	DFS_DVPORT_MFI,
	DFS_PARAMS_NR	/* sentinel */
};

/* FXOSC registers */
#define FXOSC_CTRL		(S32_FXOSC_BASE_ADDR)
#define FXOSC_STAT		(S32_FXOSC_BASE_ADDR + 0x4ul)
/* FXOSC register fields */
#define FXOSC_CTRL_OSC_BYP	BIT(31)
#define FXOSC_CTRL_COMP_EN	BIT(24)
#define FXOSC_CTRL_EOCV_SHIFT	(16)
#define FXOSC_CTRL_EOCV_MASK	(0xFF)
#define FXOSC_CTRL_EOCV(eocv)	\
	(((eocv) & FXOSC_CTRL_EOCV_MASK) << FXOSC_CTRL_EOCV_SHIFT)
#define FXOSC_CTRL_GM_SEL_SHIFT	(4)
#define FXOSC_CTRL_GM_SEL_MASK	(0xF)
#define FXOSC_CTRL_GM_SEL(sel)	\
	(((sel) & FXOSC_CTRL_GM_SEL_MASK) << FXOSC_CTRL_GM_SEL_SHIFT)
#define FXOSC_CTRL_OSCON	BIT(0)
#define FXOSC_STAT_OSC_STAT	BIT(31)

/*
 * MC_CGM configuration
 */

#define MC_CGM_MUXn_CSS_SWIP		BIT(16)
#define MC_CGM_MUXn_CSC_SELCTL_MASK	(0x3F000000)
#define MC_CGM_MUXn_CSC_SELCTL_OFFSET	(24)
#define MC_CGM_MUXn_CSC_CLK_SW		BIT(2)
#define MC_CGM_MUXn_CSS_SWTRG_SUCCESS	(0x1)
#define MC_CGM_MUXn_CSS_SWTRG_MASK	(0x000E0000)
#define MC_CGM_MUXn_CSS_SWTRG_OFFSET	(17)
#define MC_CGM_MUXn_CSS_SELSTAT_MASK	(0x3F000000)
#define MC_CGM_MUXn_CSS_SELSTAT_OFFSET	(24)

#define CGM_MUXn_CSC(cgm_addr, mux)	(((cgm_addr) + 0x300 + (mux) * 0x40))
#define CGM_MUXn_CSS(cgm_addr, mux)	(((cgm_addr) + 0x304 + (mux) * 0x40))

#define CGM_MUXn_DCn(cgm_addr, mux, dc)		\
			(((cgm_addr) + 0x308 + (mux) * 0x40 + (dc) * 0x4))
#define MC_CGM_MUXn_DCn_DIV(val)	(MC_CGM_MUXn_DCn_DIV_MASK & ((val) \
			<< MC_CGM_MUXn_DCn_DIV_OFFSET))
#define MC_CGM_MUXn_DCn_DIV_MASK	(0x00070000)
#define MC_CGM_MUXn_DCn_DIV_OFFSET	(16)

#define CGM_MUXn_DIV_UPD_STAT(cgm_addr, mux)	\
			(((cgm_addr) + 0x33c + (mux) * 0x40))
#define MC_CGM_MUXn_DIV_UPD_STAT_DIVSTAT(css)	\
			((MC_CGM_MUXn_DIV_UPD_STAT_DIVSTAT_MASK & (css)) \
			>> MC_CGM_MUXn_DIV_UPD_STAT_DIVSTAT_OFFSET)
#define MC_CGM_MUXn_DIV_UPD_STAT_DIVSTAT_MASK	(0x00000001)
#define MC_CGM_MUXn_DIV_UPD_STAT_DIVSTAT_OFFSET	(0)

#define CGM0_MUXn_CSC(mux)	(CGM_MUXn_CSC(MC_CGM0_BASE_ADDR, mux))
#define CGM0_MUXn_DCn(mux, dc)	(CGM_MUXn_DCn(MC_CGM0_BASE_ADDR, mux, dc))
#define CGM0_MUXn_DIV_UPD_STAT(mux)	\
			(CGM_MUXn_DIV_UPD_STAT(MC_CGM0_BASE_ADDR, mux))

#define MC_CGM_MUXn_CSC_SELCTL(val)	\
	(MC_CGM_MUXn_CSC_SELCTL_MASK & ((val) << MC_CGM_MUXn_CSC_SELCTL_OFFSET))
#define MC_CGM_MUXn_CSS_SWTRG(css)	\
	((MC_CGM_MUXn_CSS_SWTRG_MASK & (css)) >> MC_CGM_MUXn_CSS_SWTRG_OFFSET)
#define MC_CGM_MUXn_CSS_SELSTAT(css)	((MC_CGM_MUXn_CSS_SELSTAT_MASK & (css))\
					>> MC_CGM_MUXn_CSS_SELSTAT_OFFSET)
#define MUXn_DCn_DE			BIT(31)
#define DIV_UPD_STAT_DIV_STAT		BIT(0)

#define MC_CGM_MUXn_CSC_SEL_CORE_PLL_FIRC	0
#define MC_CGM_MUXn_CSC_SEL_CORE_PLL_PHI0	4
#define MC_CGM_MUXn_CSC_SEL_CORE_PLL_DFS1	12
#define MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI3	21
#define MC_CGM_MUXn_CSC_SEL_DDR_PLL_PHI0	36
#define MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI0	18
#define MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI7	25
#define MC_CGM_MUXn_CSC_SEL_PERIPH_DFS_DFS3	28

void s32g_plat_ddr_clock_init(void);

#define S32G274A_A53_CORE_CLK_MIN		(48000000ul)
#define S32G274A_A53_CORE_CLK_MAX		(1000000000ul)
#define IS_A53_CORE_CLK_VALID(f)	(((f) >= S32G274A_A53_CORE_CLK_MIN) && \
					 ((f) <= S32G274A_A53_CORE_CLK_MAX))

static const uint32_t core_pll_odiv_supported[] = { 1, 2, 4, 10, 20, 40 };

void s32g_ddr2firc(void);
void s32g_sw_clks2firc(void);
void s32g_disable_dfs(enum s32_dfs_type dfs);
void s32g_disable_pll(enum s32_pll_type pll, uint32_t ndivs);
void s32g_disable_fxosc(void);

int sw_mux_clk_config(enum s32g_mc_cgm cgm, uint8_t mux, uint8_t source);

#endif /* _S32G_CLOCKS_H_ */
