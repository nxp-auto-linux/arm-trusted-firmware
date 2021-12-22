/*
 * Copyright 2019-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "s32g_clocks.h"
#include "s32g_mc_me.h"

#include <assert.h>
#include <errno.h>
#include <lib/mmio.h>
#include <stdbool.h>

void s32g_disable_pll(enum s32_pll_type pll, uint32_t ndivs)
{
	uint32_t div;

	/* Disable all dividers */
	for (div = 0; div < ndivs; div++)
		mmio_write_32(PLLDIG_PLLODIV(pll, div), 0);

	/* Disable pll */
	mmio_write_32(PLLDIG_PLLCR(pll), PLLDIG_PLLCR_PLLPD);
}

void s32g_disable_fxosc(void)
{
	uint32_t ctrl = mmio_read_32(FXOSC_CTRL);

	/* Switch OFF the crystal oscillator. */
	ctrl &= ~FXOSC_CTRL_OSCON;

	mmio_write_32(FXOSC_CTRL, ctrl);

	while (mmio_read_32(FXOSC_STAT) & FXOSC_STAT_OSC_STAT)
		;
}

void s32g_disable_dfs(enum s32_dfs_type dfs)
{
	mmio_write_32(DFS_PORTRESET(dfs), DFS_PORTRESET_RESET_MASK);
	while ((mmio_read_32(DFS_PORTSR(dfs)) & DFS_PORTSR_PORTSTAT_MASK))
		;
	mmio_write_32(DFS_CTL(dfs), DFS_CTL_RESET);
}

static uintptr_t mc_cgm_addr(enum s32g_mc_cgm mc_cgm)
{
	switch (mc_cgm)	{
	case MC_CGM0:
		return (uintptr_t)MC_CGM0_BASE_ADDR;
	case MC_CGM1:
		return (uintptr_t)MC_CGM1_BASE_ADDR;
	case MC_CGM2:
		return (uintptr_t)MC_CGM2_BASE_ADDR;
	case MC_CGM5:
		return (uintptr_t)MC_CGM5_BASE_ADDR;
	default:
		return (uintptr_t)S32G_ERR_PTR;

	}
}

/* Program a software-controlled clock multiplexer as per chapter
 * "Clock Generation Module (MC_CGM)::
 *    Functional description::
 *      Clock selection multiplexer::
 *        Software-controlled clock multiplexer"
 */
int sw_mux_clk_config(enum s32g_mc_cgm cgm, uint8_t mux, uint8_t source)
{
	uint32_t css, csc;
	uintptr_t cgm_addr;

	cgm_addr = mc_cgm_addr(cgm);
	if (cgm_addr == S32G_ERR_PTR)
		return -EINVAL;

	/* Ongoing clock switch? */
	while (mmio_read_32(CGM_MUXn_CSS(cgm_addr, mux)) & MC_CGM_MUXn_CSS_SWIP)
		;

	csc = mmio_read_32(CGM_MUXn_CSC(cgm_addr, mux));
	/* Clear previous source. */
	csc &= ~(MC_CGM_MUXn_CSC_SELCTL_MASK);
	/* select the clock source and trigger the clock switch. */
	mmio_write_32(CGM_MUXn_CSC(cgm_addr, mux),
		csc | MC_CGM_MUXn_CSC_SELCTL(source) | MC_CGM_MUXn_CSC_CLK_SW);
	/* Wait for configuration bit to auto-clear. */
	while (mmio_read_32(CGM_MUXn_CSC(cgm_addr, mux))
		& MC_CGM_MUXn_CSC_CLK_SW)
		;

	/* Is the clock switch completed? */
	while (mmio_read_32(CGM_MUXn_CSS(cgm_addr, mux)) & MC_CGM_MUXn_CSS_SWIP)
		;

	/* Chech if the switch succeeded.
	 * Check switch trigger cause and the source.
	 */
	css = mmio_read_32(CGM_MUXn_CSS(cgm_addr, mux));
	if ((MC_CGM_MUXn_CSS_SWTRG(css) == MC_CGM_MUXn_CSS_SWTRG_SUCCESS) &&
		(MC_CGM_MUXn_CSS_SELSTAT(css) == source))
		return 0;

	return -EIO;
}

static void switch_muxes_to_firc(enum s32g_mc_cgm cgm, uint8_t *muxes,
				 size_t nmuxes)
{
	size_t i;

	for (i = 0; i < nmuxes; i++)
		sw_mux_clk_config(cgm, muxes[i],
				  MC_CGM_MUXn_CSC_SEL_CORE_PLL_FIRC);
}

/**
 * Switch all platform's clocks to FIRC
 */
void s32g_sw_clks2firc(void)
{
	/* MC_CGM_MUXn_CSC_SEL_FIRC */
	uint8_t cgm0_muxes[] = {
		16, /* SPI */
		15, /* GMAC_0_REF */
		14, /* SDHC */
		12, /* QSPI */
		11, /* GMAC_0_RX */
		10, /* GMAC_0_TX */
		9, /* GMAC_TS */
		8, /* LINFLEX */
		7, /* CAN_PE */
		6, /* FLEXRAY */
		5, /* FTM_1 */
		4, /* FTM_0 */
		3, /* PER */
		0, /* XBAR_2X */
	};

	uint8_t cgm1_muxes[] = {
		0, /* A53_CORE */
	};

	uint8_t cgm2_muxes[] = {
		9, /* PFE_MAC_2_REF */
		8, /* PFE_MAC_1_REF */
		7, /* PFE_MAC_0_REF */
		6, /* PEF_MAC_2_RX */
		5, /* PEF_MAC_1_RX */
		4, /* PEF_MAC_0_RX */
		3, /* PFE_MAC_2_TX */
		2, /* PFE_MAC_1_TX */
		1, /* PFE_MAC_0_TX_DIV */
		0, /* PFE_PE */
	};

	switch_muxes_to_firc(MC_CGM2, &cgm2_muxes[0],
			     ARRAY_SIZE(cgm2_muxes));
	switch_muxes_to_firc(MC_CGM1, &cgm1_muxes[0],
			     ARRAY_SIZE(cgm1_muxes));
	switch_muxes_to_firc(MC_CGM0, &cgm0_muxes[0],
			     ARRAY_SIZE(cgm0_muxes));
}

void s32g_ddr2firc(void)
{
	uint8_t cgm5_muxes[] = {
		0, /* DDR */
	};

	switch_muxes_to_firc(MC_CGM5, &cgm5_muxes[0],
			     ARRAY_SIZE(cgm5_muxes));
}
