/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC__S32_CLOCKS_H_
#define S32CC__S32_CLOCKS_H_

#define S32_FXOSC_FREQ		(40000000ul)
#define S32_FIRC_FREQ		(48000000ul)
#define S32_ERR_CLK_FREQ	(0ul)

/*
 * PLL configuration
 */

#define CORE_PLL_BASE_ADDR	0x40038000ul
#define PLL_ADDR(pll)		(CORE_PLL_BASE_ADDR + (pll) * 0x4000)

#define ARM_PLL_BASE_ADDR	PLL_ADDR(S32_CORE_PLL)
#define ARM_PLL_SIZE		(0x88)
#define PERIPH_PLL_BASE_ADDR	PLL_ADDR(S32_PERIPH_PLL)
#define PERIPH_PLL_SIZE		(0x100)
#define ACCEL_PLL_SIZE		(0x88)
#define DRAM_PLL_BASE_ADDR	PLL_ADDR(S32_DDR_PLL)
#define DRAM_PLL_SIZE		(0x84)

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

enum s32_pll_type {
	S32_CORE_PLL = 0,
	S32_PERIPH_PLL,
	S32_ACCEL_PLL,
	S32_DDR_PLL,
	S32_PLL_NR	/* sentinel */
};

/*
 * DFS configuration
 */

/* Number of ports for each DFS  */
#define S32_DFS_PORTS_NR	6

enum s32_dfs_type {
	S32_CORE_DFS = 0,
	S32_PERIPH_DFS,
	S32_DFS_NR	/* sentinel */
};

#define S32_DFS_BASE_ADDR		(0x40054000ul)

/* @dfs - One of the enum s32_dfs_type values */
#define S32_DFS_ADDR(dfs)	(S32_DFS_BASE_ADDR + (dfs) * 0x4000)

#define ARM_DFS_BASE_ADDR	S32_DFS_ADDR(S32_CORE_DFS)
#define ARM_DFS_SIZE		(0x34)
#define PERIPH_DFS_BASE_ADDR	S32_DFS_ADDR(S32_PERIPH_DFS)
#define PERIPH_DFS_SIZE		(0x34)

/* DFS register offsets */
#define DFS_PORTRESET_OFF	0X14ul
#define DFS_PORTRESET(dfs)	((S32_DFS_ADDR(dfs)) + DFS_PORTRESET_OFF)
#define DFS_PORTSR_OFF		0xCul
#define DFS_PORTSR(dfs)		((S32_DFS_ADDR(dfs)) + DFS_PORTSR_OFF)
#define DFS_CTL_OFF			0x18ul
#define DFS_CTL(dfs)		((S32_DFS_ADDR(dfs)) + DFS_CTL_OFF)
#define DFS_DVPORT0_OFF		0x1Cul
#define DFS_DVPORTn(dfs, port)	((S32_DFS_ADDR(dfs)) + DFS_DVPORT0_OFF + \
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

/*
 * Platform reference clocks
 */

enum s32g_refclk {
	S32G_REFCLK_FIRC,
	S32G_REFCLK_FXOSC,
};


#define MC_RGM_BASE_ADDR	0x40078000
#define MC_ME_BASE_ADDR		(0x40088000ul)
#define MC_ME_SIZE		(0xf34)
#define RDC_BASE_ADDR		(0x40080000ul)
#define RDC_SIZE		(0x90)

#define S32_FXOSC_BASE_ADDR	(0x40050000ul)
#define S32_FXOSC_SIZE		(0x8)

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
#define MC_CGM0_SIZE			(0x708)
#define MC_CGM1_BASE_ADDR		(0x40034000ul)
#define MC_CGM1_SIZE			(0x308)
#ifndef MC_CGM5_BASE_ADDR
#define MC_CGM5_BASE_ADDR		(0x40068000ul)
#endif
#define MC_CGM5_SIZE			(0x308)

/* This should be kept in sync with other defines in this file,
 * as it cannot be determined at run-time.
 */
#define I2C_CLK_FREQ		(133 * 1000 * 1000)

int s32_a53_clock_early_setup(void);
int s32_a53_clock_setup(void);
int s32_periph_clock_init(void);
int s32_enable_ddr_clock(void);
int s32_reset_ddr_periph(void);

#endif /* S32CC__S32_CLOCKS_H_ */

