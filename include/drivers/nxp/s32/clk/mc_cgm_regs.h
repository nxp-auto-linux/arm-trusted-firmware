// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021, 2023-2024 NXP
 */

#ifndef MC_CGM_REGS_H
#define MC_CGM_REGS_H

#include <s32cc_bl_common.h>

/* FXOSC registers. */
#define FXOSC_CTRL(FXOSC)		(UPTR(FXOSC) + 0x0)
#define FXOSC_CTRL_OSC_BYP		BIT(31)
#define FXOSC_CTRL_COMP_EN		BIT(24)

#define FXOSC_CTRL_EOCV(val)		(FXOSC_CTRL_EOCV_MASK & ((val) << \
					 FXOSC_CTRL_EOCV_OFFSET))
#define FXOSC_CTRL_EOCV_MASK		(0x00FF0000)
#define FXOSC_CTRL_EOCV_OFFSET		(16)

#define FXOSC_CTRL_GM_SEL(val)		(FXOSC_CTRL_GM_SEL_MASK & ((val) << \
					 FXOSC_CTRL_GM_SEL_OFFSET))
#define FXOSC_CTRL_GM_SEL_MASK		(0x000000F0)
#define FXOSC_CTRL_GM_SEL_OFFSET	(4)

#define FXOSC_CTRL_OSCON		BIT(0)

#define FXOSC_STAT(FXOSC)		(UPTR(FXOSC) + 0x4)
#define FXOSC_STAT_OSC_STAT		BIT(31)

/* MC_CGM registers definitions */
/* MC_CGM_MUX_n_CSC */
#define CGM_MUXn_CSC(cgm_addr, mux)	((UPTR(cgm_addr) + 0x300 + \
					 (mux) * 0x40))
#define MC_CGM_MUXn_CSC_SELCTL(val)	(MC_CGM_MUXn_CSC_SELCTL_MASK & ((val) \
					 << MC_CGM_MUXn_CSC_SELCTL_OFFSET))
#define MC_CGM_MUXn_CSC_SELCTL_MASK	(0x3F000000)
#define MC_CGM_MUXn_CSC_SELCTL_OFFSET	(24)

#define MC_CGM_MUXn_CSC_CLK_SW		BIT(2)
#define MC_CGM_MUXn_CSC_SAFE_SW		BIT(3)

/* MC_CGM_MUX_n_CSS */
#define CGM_MUXn_CSS(cgm_addr, mux)	((UPTR(cgm_addr) + 0x304 + \
					 (mux) * 0x40))
#define MC_CGM_MUXn_CSS_SELSTAT(css)	((MC_CGM_MUXn_CSS_SELSTAT_MASK & (css))\
					 >> MC_CGM_MUXn_CSS_SELSTAT_OFFSET)
#define MC_CGM_MUXn_CSS_SELSTAT_MASK	(0x3F000000)
#define MC_CGM_MUXn_CSS_SELSTAT_OFFSET	(24)

#define MC_CGM_MUXn_CSS_SWIP		BIT(16)
#define MC_CGM_MUXn_CSS_SAFE_SW		BIT(3)
#define MC_CGM_MUXn_CSS_SWTRG(css)	((MC_CGM_MUXn_CSS_SWTRG_MASK & (css)) \
					 >> MC_CGM_MUXn_CSS_SWTRG_OFFSET)
#define MC_CGM_MUXn_CSS_SWTRG_MASK	(0x000E0000)
#define MC_CGM_MUXn_CSS_SWTRG_OFFSET	(17)
#define MC_CGM_MUXn_CSS_SWTRG_SUCCESS	(0x1)
#define MC_CGM_MUXn_CSS_SWTRG_SAFE_CLK	(0x4)
#define MC_CGM_MUXn_CSS_SWTRG_SAFE_CLK_INACTIVE	(0x5)

/* MC_CGM Software controlled mux CSC */
#define MC_CGM_SW_MUXn_CSC_SELCTL(val)	MC_CGM_MUXn_CSC_SELCTL(val)
#define MC_CGM_SW_MUXn_CSC_FCG		BIT_32(3)
#define MC_CGM_SW_MUXn_CSC_CG		BIT_32(2)

/* MC_CGM Software controlled mux CSS */
#define MC_CGM_SW_MUXn_CSS_SELSTAT(css)	MC_CGM_MUXn_CSS_SELSTAT(css)
#define MC_CGM_SW_MUXn_CSS_CS		BIT_32(17)
#define MC_CGM_SW_MUXn_CSS_GRIP		BIT_32(16)

/* MC_CGM_SC_DCn */
#define CGM_MUXn_DCm(cgm_addr, mux, dc)	((UPTR(cgm_addr) + 0x308) + \
					 ((mux) * 0x40))
#define MC_CGM_MUXn_DCm_DIV(val)	(MC_CGM_MUXn_DCm_DIV_MASK & ((val) \
					 << MC_CGM_MUXn_DCm_DIV_OFFSET))
#define MC_CGM_MUXn_DCm_DIV_VAL(val)	((MC_CGM_MUXn_DCm_DIV_MASK & val) \
					 >> MC_CGM_MUXn_DCm_DIV_OFFSET)
#define MC_CGM_MUXn_DCm_DIV_MASK	(0x00FF0000)
#define MC_CGM_MUXn_DCm_DIV_OFFSET	(16)
#define MC_CGM_MUXn_DCm_DE		BIT(31)

/* DIV_UPD_STAT */
#define CGM_MUXn_DIV_UPD_STAT(cgm_addr, mux)	((UPTR(cgm_addr) + 0x33C + \
						 (mux) * 0x40))
#define MC_CGM_MUXn_DIV_UPD_STAT_DIVSTAT(css)	((MC_CGM_MUXn_DIV_UPD_STAT_DIVSTAT_MASK \
						  & (css)) \
						  >> MC_CGM_MUXn_DIV_UPD_STAT_DIVSTAT_OFFSET)
#define MC_CGM_MUXn_DIV_UPD_STAT_DIVSTAT_MASK	(0x00000001)
#define MC_CGM_MUXn_DIV_UPD_STAT_DIVSTAT_OFFSET	(0)

#define pll_addr(pll)			UPTR(pll)
#define dfs_addr(pll)			UPTR(pll)

/* PLLDIG PLL Control Register (PLLDIG_PLLCR) */
#define PLLDIG_PLLCR(pll)		(pll_addr(pll))
#define PLLDIG_PLLCR_PLLPD		BIT(31)

/* PLLDIG PLL Status Register (PLLDIG_PLLSR) */
#define PLLDIG_PLLSR(pll)		((pll_addr(pll)) + 0x00000004)
#define PLLDIG_PLLSR_LOCK		BIT(2)

/* PLLDIG PLL Divider Register (PLLDIG_PLLDV) */
#define PLLDIG_PLLDV(pll)		((pll_addr(pll)) + 0x00000008)
#define PLLDIG_PLLDV_MFI(div)		(PLLDIG_PLLDV_MFI_MASK & (div))
#define PLLDIG_PLLDV_MFI_MASK		(0x000000FF)

#define PLLDIG_PLLDV_RDIV_SET(val)	(PLLDIG_PLLDV_RDIV_MASK & \
					 (((val) & PLLDIG_PLLDV_RDIV_MAXVALUE) \
					  << PLLDIG_PLLDV_RDIV_OFFSET))
#define PLLDIG_PLLDV_RDIV_MAXVALUE	(0x7)
#define PLLDIG_PLLDV_RDIV_MASK		(0x00007000)
#define PLLDIG_PLLDV_RDIV_OFFSET	(12)
#define PLLDIG_PLLDV_RDIV(val)		(((val) & PLLDIG_PLLDV_RDIV_MASK) >> \
					 PLLDIG_PLLDV_RDIV_OFFSET)

/* PLLDIG PLL Fractional  Divide Register (PLLDIG_PLLFD) */
#define PLLDIG_PLLFD(pll)		((pll_addr(pll)) + 0x00000010)
#define PLLDIG_PLLFD_MFN_SET(val)	(PLLDIG_PLLFD_MFN_MASK & (val))
#define PLLDIG_PLLFD_MFN_MASK		(0x00007FFF)
#define PLLDIG_PLLFD_SMDEN		BIT(30)

/* PLL Clock Mux (PLLCLKMUX) */
#define PLLDIG_PLLCLKMUX(pll)			(UPTR(pll) + 0x00000020)

/* PLL Output Divider (PLLODIV0 - PLLODIV7) */
#define PLLDIG_PLLODIV(pll, n)		((pll_addr(pll)) + 0x00000080 + n * 0x4)
#define PLLDIG_PLLODIV_DIV_SET(val)	(PLLDIG_PLLODIV_DIV_MASK & \
					 ((val) << PLLDIG_PLLODIV_DIV_OFFSET))
#define PLLDIG_PLLODIV_DIV_MASK		(0x00FF0000)
#define PLLDIG_PLLODIV_DIV_OFFSET	(16)
#define PLLDIG_PLLODIV_DIV(val)		(((val) & PLLDIG_PLLODIV_DIV_MASK) >> \
					 PLLDIG_PLLODIV_DIV_OFFSET)

#define PLLDIG_PLLODIV_DE		BIT(31)

/* Digital Frequency Synthesizer (DFS) */
/* According to the manual there are DFS modules for ARM_PLL, PERIPH_PLL */

/* DFS Control Register (DFS_CTL) */
#define DFS_CTL(dfs)			((dfs_addr(dfs)) + 0x00000018)
#define DFS_CTL_RESET			BIT(1)

/* DFS Port Status Register (DFS_PORTSR) */
#define DFS_PORTSR(dfs)			((dfs_addr(dfs)) + 0x0000000C)
/* DFS Port Reset Register (DFS_PORTRESET) */
#define DFS_PORTRESET(dfs)			((dfs_addr(dfs)) + 0x00000014)
#define DFS_PORTRESET_PORTRESET_SET(val)	\
			(((val) & DFS_PORTRESET_PORTRESET_MASK) \
			<< DFS_PORTRESET_PORTRESET_OFFSET)
#define DFS_PORTRESET_PORTRESET_MAXVAL		(0x0000003F)
#define DFS_PORTRESET_PORTRESET_MASK		(0x0000003F)
#define DFS_PORTRESET_PORTRESET_OFFSET		(0)

/* DFS Divide Register Portn (DFS_DVPORTn) */
#define DFS_DVPORTn(dfs, n)			((dfs_addr(dfs)) + \
						 (0x1C + ((n) * 0x4)))

/* Port Loss of Lock Status (PORTLOLSR) */
#define DFS_PORTOLSR(dfs)			((dfs_addr(dfs)) + 0x00000010)
#define DFS_PORTOLSR_LOL(n)			(BIT(n) & 0x3FU)

/*
 * The mathematical formula for fdfs_clockout is the following:
 * fdfs_clckout = fdfs_clkin / (2 * (DFS_DVPORTn[MFI] + (DFS_DVPORTn[MFN]/36)))
 */
#define DFS_DVPORTn_MFI_SET(val)	(DFS_DVPORTn_MFI_MASK & \
		(((val) & DFS_DVPORTn_MFI_MAXVAL) << DFS_DVPORTn_MFI_OFFSET))
#define DFS_DVPORTn_MFN_SET(val)	(DFS_DVPORTn_MFN_MASK & \
		(((val) & DFS_DVPORTn_MFN_MAXVAL) << DFS_DVPORTn_MFN_OFFSET))
#define DFS_DVPORTn_MFI(val)		(((val) & DFS_DVPORTn_MFI_MASK) >> \
					 DFS_DVPORTn_MFI_OFFSET)
#define DFS_DVPORTn_MFN(val)		(((val) & DFS_DVPORTn_MFN_MASK) >> \
					 DFS_DVPORTn_MFN_OFFSET)
#define DFS_DVPORTn_MFI_MASK		(0x0000FF00)
#define DFS_DVPORTn_MFN_MASK		(0x000000FF)
#define DFS_DVPORTn_MFI_MAXVAL		(0xFF)
#define DFS_DVPORTn_MFN_MAXVAL		(0xFF)
#define DFS_DVPORTn_MFI_OFFSET		(8)
#define DFS_DVPORTn_MFN_OFFSET		(0)

#endif /*MC_CGM_REGS_H */
