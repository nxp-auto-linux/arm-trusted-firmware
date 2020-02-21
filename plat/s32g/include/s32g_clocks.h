/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _S32G_CLOCKS_H_
#define _S32G_CLOCKS_H_


#define S32G_FXOSC_FREQ		(40000000ul)
#define S32G_FIRC_FREQ		(48000000ul)
#define S32G_ERR_CLK_FREQ	(0ul)

/*
 * PLL configuration
 */

#define CORE_PLL_BASE_ADDR	0x40038000ul
#define PLL_ADDR(pll)		(CORE_PLL_BASE_ADDR + (pll) * 0x4000)

#define PLL_MIN_FREQ		(1300000000ull)
#define PLL_MAX_FREQ		(5000000000ull)

#define PLLCLKMUX_OFFSET	0x20
#define PLLDIG_PLLCLKMUX(pll)	((PLL_ADDR(pll)) + PLLCLKMUX_OFFSET)
#define PLLDIG_PLLCLKMUX_REFCLK_FIRC	(0x0ul)
#define PLLDIG_PLLCLKMUX_REFCLK_FXOSC	(0x1ul)

#define PLLDIG_PLLCR_PLLPD	BIT(31)
#define PLLDIG_PLLCR(pll)	(PLL_ADDR(pll))

#define PLLDIG_PLLODIV(pll, n)		((PLL_ADDR(pll)) + 0x00000080 + n * 0x4)
#define PLLDIG_PLLODIV_DIV_MASK		(0x00FF0000)
#define PLLDIG_PLLODIV_DIV_OFFSET	(16)
#define PLLDIG_PLLODIV_DIV_SET(val) \
	(PLLDIG_PLLODIV_DIV_MASK & ((val) << PLLDIG_PLLODIV_DIV_OFFSET))
#define PLLDIG_PLLODIV_DE		BIT(31)

#define PLLDIG_PLLDV(pll)	((PLL_ADDR(pll)) + 0x00000008)
#define PLLDIG_PLLDV_MFI_MASK	(0x000000FF)
#define PLLDIG_PLLDV_MFI(div)	(PLLDIG_PLLDV_MFI_MASK & (div))
#define PLLDIG_PLLDV_RDIV_MASK		(0x00007000)
#define PLLDIG_PLLDV_RDIV_MAXVALUE	(0x7)
#define PLLDIG_PLLDV_RDIV_OFFSET	(12)
#define PLLDIG_PLLDV_RDIV_SET(val)	(PLLDIG_PLLDV_RDIV_MASK & \
	(((val) & PLLDIG_PLLDV_RDIV_MAXVALUE) << PLLDIG_PLLDV_RDIV_OFFSET))

#define PLLDIG_PLLFD(pll)		((PLL_ADDR(pll)) + 0x00000010)
#define PLLDIG_PLLFD_SMDEN		BIT(30)
#define PLLDIG_PLLFD_MFN_MASK		(0x00007FFF)
#define PLLDIG_PLLFD_MFN_SET(val)	(PLLDIG_PLLFD_MFN_MASK & (val))

#define PLLDIG_PLLSR_LOCK	BIT(2)
#define PLLDIG_PLLSR(pll)	((PLL_ADDR(pll)) + 0x00000004)

enum s32g_pll_type {
	S32G_CORE_PLL = 0,
	S32G_PERIPH_PLL,
	S32G_ACCEL_PLL,
	S32G_DDR_PLL,
	S32G_PLL_NR	/* sentinel */
};

/* Number of dividers for each PLL */
static const uint32_t s32g_pll_phi_nr[S32G_PLL_NR] = {2, 8, 2, 1};

/* This should be kept in sync with the CORE_PLL
 * configuration (MFI, MFN, RDIV). Due to not
 * having floating point support, it is impossible
 * to accurately calculate it at runtime.
 */
#define CORE_PLL_FVCO	(2000000000ul)

/* Array of parameters for each PLL */
static const uint32_t s32g_pll_rdiv[S32G_PLL_NR] = {1, 1, 1, 1};
static const uint32_t s32g_pll_mfi[S32G_PLL_NR] = {50, 50, 60, 40};
static const uint32_t s32g_pll_mfn[S32G_PLL_NR] = {0, 0, 1, 0};

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
#define DDR_PLL_PHI0_FREQ	(400000000ull)
static const uint64_t s32g_ddr_pll_phi_freq[] = {
	DDR_PLL_PHI0_FREQ,
};

/*
 * DFS configuration
 */

/* Number of ports for each DFS  */
#define S32G_DFS_PORTS_NR	6

enum s32g_dfs_type {
	S32G_CORE_DFS = 0,
	S32G_PERIPH_DFS,
	S32G_DFS_NR	/* sentinel */
};

#define S32G_DFS_BASE_ADDR	0x40054000ul
/* @dfs - One of the enum s32g_dfs_type values */
#define S32G_DFS_ADDR(dfs) (S32G_DFS_BASE_ADDR + (dfs) * 0x4000)
/* DFS register offsets */
#define DFS_PORTRESET_OFF	0X14ul
#define DFS_PORTRESET(dfs)	((S32G_DFS_ADDR(dfs)) + DFS_PORTRESET_OFF)
#define DFS_PORTSR_OFF		0xCul
#define DFS_PORTSR(dfs)		((S32G_DFS_ADDR(dfs)) + DFS_PORTSR_OFF)
#define DFS_CTL_OFF		0x18ul
#define DFS_CTL(dfs)		((S32G_DFS_ADDR(dfs)) + DFS_CTL_OFF)
#define DFS_DVPORT0_OFF		0x1Cul
#define DFS_DVPORTn(dfs, port)	((S32G_DFS_ADDR(dfs)) + DFS_DVPORT0_OFF + \
					(port) * 0x4)
/* DFS register fields */
#define DFS_PORTRESET_RESET_MASK	(0x3F)
#define DFS_PORTSR_PORTSTAT_MASK	DFS_PORTRESET_RESET_MASK
#define DFS_CTL_RESET			(1 << 1)
#define DFS_DVPORTn_MFI_MASK		(0xFF)
#define DFS_DVPORTn_MFI_SHIFT		(8)
#define DFS_DVPORTn_MFN_MASK		(0x3F)
#define DFS_DVPORTn_MFN_SHIFT		(0)
#define DFS_DVPORTn_MFI(mfi)	\
	(((mfi) & DFS_DVPORTn_MFI_MASK) << DFS_DVPORTn_MFI_SHIFT)
#define DFS_DVPORTn_MFN(mfn)	\
	(((mfn) & DFS_DVPORTn_MFN_MASK) << DFS_DVPORTn_MFN_SHIFT)

enum S32G_DFS_PARAMS {
	DFS_PORT_EN = 0,
	DFS_DVPORT_MFN,
	DFS_DVPORT_MFI,
	DFS_PARAMS_NR	/* sentinel */
};

/* Core DFS configuration params */
#define CORE_DFS1_EN	(1)
#define CORE_DFS2_EN	(1)
#define CORE_DFS3_EN	(1)
#define CORE_DFS4_EN	(1)
#define CORE_DFS5_EN	(1)
#define CORE_DFS6_EN	(1)

#define CORE_DFS1_MFN	(9)
#define CORE_DFS2_MFN	(9)
#define CORE_DFS3_MFN	(0)
#define CORE_DFS4_MFN	(12)
#define CORE_DFS5_MFN	(24)
#define CORE_DFS6_MFN	(24)

#define CORE_DFS1_MFI	(1)
#define CORE_DFS2_MFI	(1)
#define CORE_DFS3_MFI	(2)
#define CORE_DFS4_MFI	(3)
#define CORE_DFS5_MFI	(1)
#define CORE_DFS6_MFI	(1)

static const uint32_t s32g_core_dfs_params[S32G_DFS_PORTS_NR][DFS_PARAMS_NR] = {
	{CORE_DFS1_EN, CORE_DFS1_MFN, CORE_DFS1_MFI},
	{CORE_DFS2_EN, CORE_DFS2_MFN, CORE_DFS2_MFI},
	{CORE_DFS3_EN, CORE_DFS3_MFN, CORE_DFS3_MFI},
	{CORE_DFS4_EN, CORE_DFS4_MFN, CORE_DFS4_MFI},
	{CORE_DFS5_EN, CORE_DFS5_MFN, CORE_DFS5_MFI},
	{CORE_DFS6_EN, CORE_DFS6_MFN, CORE_DFS6_MFI}
};

/* Periph DFS configuration params */
#define PERIPH_DFS1_EN	(1)
#define PERIPH_DFS2_EN	(1)
#define PERIPH_DFS3_EN	(1)
#define PERIPH_DFS4_EN	(1)
#define PERIPH_DFS5_EN	(1)
#define PERIPH_DFS6_EN	(1)

#define PERIPH_DFS1_MFN (9)
#define PERIPH_DFS2_MFN (21)
#define PERIPH_DFS3_MFN (9)
#define PERIPH_DFS4_MFN (24)
#define PERIPH_DFS5_MFN (1)
#define PERIPH_DFS6_MFN (0)

#define PERIPH_DFS1_MFI (1)
#define PERIPH_DFS2_MFI (1)
#define PERIPH_DFS3_MFI (1)
#define PERIPH_DFS4_MFI (1)
#define PERIPH_DFS5_MFI (3)
#define PERIPH_DFS6_MFI (2)

static const uint32_t
s32g_periph_dfs_params[S32G_DFS_PORTS_NR][DFS_PARAMS_NR] = {
	{PERIPH_DFS1_EN, PERIPH_DFS1_MFN, PERIPH_DFS1_MFI},
	{PERIPH_DFS2_EN, PERIPH_DFS2_MFN, PERIPH_DFS2_MFI},
	{PERIPH_DFS3_EN, PERIPH_DFS3_MFN, PERIPH_DFS3_MFI},
	{PERIPH_DFS4_EN, PERIPH_DFS4_MFN, PERIPH_DFS4_MFI},
	{PERIPH_DFS5_EN, PERIPH_DFS5_MFN, PERIPH_DFS5_MFI},
	{PERIPH_DFS6_EN, PERIPH_DFS6_MFN, PERIPH_DFS6_MFI}
};

/*
 * Platform reference clocks
 */

enum s32g_refclk {
	S32G_REFCLK_FIRC,
	S32G_REFCLK_FXOSC,
};

#define S32G_FXOSC_BASE_ADDR	0x40050000ul
/* FXOSC registers */
#define FXOSC_CTRL		(S32G_FXOSC_BASE_ADDR)
#define FXOSC_STAT		(S32G_FXOSC_BASE_ADDR + 0x4ul)
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
enum s32g_mc_cgm {
	MC_CGM0,
	MC_CGM1,
	MC_CGM2,
	MC_CGM5
};

#define MC_CGM0_BASE_ADDR		(0x40030000ul)
#define MC_CGM1_BASE_ADDR		(0x40034000ul)
#define MC_CGM2_BASE_ADDR		(0x44018000ul)
#define MC_CGM5_BASE_ADDR		(0x40068000ul)

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
#define CGM_MUXn_DIV_UPD_STAT(cgm_addr, mux)	\
			(((cgm_addr) + 0x33c + (mux) * 0x40))

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
#define MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI3	21
#define MC_CGM_MUXn_CSC_SEL_DDR_PLL_PHI0	36
#define MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI0	18
#define MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI7	25
#define MC_CGM_MUXn_CSC_SEL_PERIPH_DFS_DFS3	28

void s32g_plat_clock_init(void);

#define S32G274A_A53_CORE_CLK_MIN		(48000000ul)
#define S32G274A_A53_CORE_CLK_MAX		(1000000000ul)
#define IS_A53_CORE_CLK_VALID(f)	(((f) >= S32G274A_A53_CORE_CLK_MIN) && \
					 ((f) <= S32G274A_A53_CORE_CLK_MAX))

static const uint32_t core_pll_odiv_supported[] = { 1, 2, 4, 10, 20, 40 };

int s32g_set_a53_core_clk(uint64_t freq);

#endif /* _S32G_CLOCKS_H_ */
