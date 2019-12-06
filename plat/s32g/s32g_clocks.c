/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <errno.h>
#include <assert.h>
#include "s32g_clocks.h"


static uint64_t plldig_set_refclk(enum s32g_pll_type pll,
				  enum s32g_refclk refclk)
{
	uint64_t refclk_freq = S32G_ERR_CLK_FREQ;
	uint32_t pllclkmux;

	switch (refclk) {
	case S32G_REFCLK_FIRC:
		pllclkmux = PLLDIG_PLLCLKMUX_REFCLK_FIRC;
		refclk_freq = S32G_FIRC_FREQ;
		break;
	case S32G_REFCLK_FXOSC:
		pllclkmux = PLLDIG_PLLCLKMUX_REFCLK_FXOSC;
		refclk_freq = S32G_FXOSC_FREQ;
		break;
	default:
		assert(0);
		break;
	}
	mmio_write_32(PLLDIG_PLLCLKMUX(pll), pllclkmux);

	return refclk_freq;
}

/*
 * Program the pll for various blocks
 *
 * @pll: Type of PLL to program
 * @refclk: Input reference clock to the PLL block
 * @freq: Array of PHI frequencies for each type of PLL
 * @plldv_rdiv: Divider of clkfreq_ref
 * @plldv_mfi: Loop multiplication factor divider
 * @pllfd_mfn: Numerator loop multiplication factor divider
 *
 * For details, please consult the "PLL Digital Interface (PLLDIG)" chapter
 * of the S32G Reference Manual.
 */
static int program_pll(enum s32g_pll_type pll, enum s32g_refclk refclk,
		       const uint64_t freq[], uint32_t plldv_rdiv,
		       uint32_t plldv_mfi, uint32_t pllfd_mfn)
{
	uint32_t i, phi_nr;
	uint64_t refclk_freq;

	phi_nr = s32g_pll_phi_nr[pll];

	/* Disable dividers */
	for (i = 0; i < phi_nr; i++)
		mmio_write_32(PLLDIG_PLLODIV(pll, i), 0x0);

	/* Disable PLL */
	mmio_write_32(PLLDIG_PLLCR(pll), PLLDIG_PLLCR_PLLPD);

	refclk_freq = plldig_set_refclk(pll, refclk);
	if (refclk_freq == S32G_ERR_CLK_FREQ)
		return -1;

	/* See chapter:
	 *   PLL Digital Interface (PLLDIG)::Modes of operation
	 * from the S32G Reference Manual.
	 *
	 * If we were allowed to build with floating-point support, we
	 * should have:
	 * fvco = refclk_freq * (plldv_mfi + (pllfd_mfn/(float)18432)) /
	 *                       (float)plldv_rdiv
	 * i.e.:
	 * fvco * rdiv * 18432 = refclk * (mfi * 18432 + mfn)
	 * odiv = (refclk * (mfi * 18432 + mfn)) / (freq[i] * rdiv * 18432) - 1
	 */

	/*
	 * VCO should have value in [PLL_MIN_FREQ, PLL_MAX_FREQ].
	 * Please consult the platform DataSheet in order to determine
	 * the allowed values.
	 */
	assert(refclk_freq * (plldv_mfi * 18432 + pllfd_mfn) >=
	       PLL_MIN_FREQ * plldv_rdiv * 18432);
	assert(refclk_freq * (plldv_mfi * 18432 + pllfd_mfn) <=
	       PLL_MAX_FREQ * plldv_rdiv * 18432);

	mmio_write_32(PLLDIG_PLLDV(pll), PLLDIG_PLLDV_RDIV_SET(plldv_rdiv) |
					 PLLDIG_PLLDV_MFI(plldv_mfi));
	mmio_write_32(PLLDIG_PLLFD(pll), PLLDIG_PLLFD_MFN_SET(pllfd_mfn) |
					 PLLDIG_PLLFD_SMDEN);

	/* Calculate divider output frequency for required PHIn outputs */
	for (i = 0; i < phi_nr; i++) {
		uint32_t tmp;
		uint32_t odiv;

		assert(freq[i]);
		/* Also, because MFN must be less than 18432 according to the
		 * refman, the following inequality must hold:
		 *    (refclk_freq * (MFI + 1) / (freq[i] * RDIV)) > 1
		 */
		assert(refclk_freq * (plldv_mfi + 1) > freq[i] * plldv_rdiv);

		odiv = (uint32_t)((refclk_freq *
				  (plldv_mfi * 18432 + pllfd_mfn)) /
				  (freq[i] * plldv_rdiv * 18432) - 1);

		tmp = mmio_read_32(PLLDIG_PLLODIV(pll, i));
		mmio_write_32(PLLDIG_PLLODIV(pll, i),
			      tmp | PLLDIG_PLLODIV_DIV_SET(odiv));
	}

	/* Enable the PLL */
	mmio_write_32(PLLDIG_PLLCR(pll), 0x0);

	/* Poll until PLL acquires lock */
	while (!(mmio_read_32(PLLDIG_PLLSR(pll)) & PLLDIG_PLLSR_LOCK))
		;

	/* Enable dividers */
	for (i = 0; i < phi_nr; i++) {
		uint32_t tmp;

		tmp = mmio_read_32(PLLDIG_PLLODIV(pll, i));
		mmio_write_32(PLLDIG_PLLODIV(pll, i), PLLDIG_PLLODIV_DE | tmp);
	}

	return 0;
}

static void start_fxosc(void)
{
	uint32_t ctrl;

	/* If FXOSC is already running, do nothing */
	if (mmio_read_32(FXOSC_CTRL) & FXOSC_CTRL_OSCON)
		return;

	ctrl = FXOSC_CTRL_COMP_EN;
	ctrl &= ~FXOSC_CTRL_OSC_BYP;
	ctrl |= FXOSC_CTRL_EOCV(0x1);
	ctrl |= FXOSC_CTRL_GM_SEL(0x7);
	mmio_write_32(FXOSC_CTRL, ctrl);

	/* Switch on the crystal oscillator */
	mmio_write_32(FXOSC_CTRL, FXOSC_CTRL_OSCON | mmio_read_32(FXOSC_CTRL));

	/* Wait until the clock is stable */
	while (!(mmio_read_32(FXOSC_STAT) & FXOSC_STAT_OSC_STAT))
		;
}

static void disable_dfs(enum s32g_dfs_type dfs)
{
	mmio_write_32(DFS_PORTRESET(dfs), DFS_PORTRESET_RESET_MASK);
	while ((mmio_read_32(DFS_PORTSR(dfs)) & DFS_PORTSR_PORTSTAT_MASK))
		;
	mmio_write_32(DFS_CTL(dfs), DFS_CTL_RESET);
}

/* DFS enablement, according to chapter
 *    "Digital Frequency Synthesizer (DFS)::Initialization information"
 * from the S32G Reference Manual.
 *
 * Any port settings (MFI, MFN) must be applied _before_ calling this function.
 */
static void enable_dfs(enum s32g_dfs_type dfs,
		       const uint32_t params[][DFS_PARAMS_NR])
{
	uint32_t port, reset = 0;

	/* deassert the master reset */
	mmio_write_32(DFS_CTL(dfs), ~DFS_CTL_RESET);
	/* deassert individual ports */
	for (port = 0; port < S32G_DFS_PORTS_NR; port++)
		reset |= (params[port][DFS_PORT_EN] << port);
	mmio_write_32(DFS_PORTRESET(dfs), ~reset);

	/* wait until all configured ports are locked */
	while ((mmio_read_32(DFS_PORTSR(dfs)) & reset) != reset)
		;
}

/* DFS init & config, according to chapter
 *    "Digital Frequency Synthesizer (DFS)::Initialization information"
 * from the S32G Reference Manual.
 *
 * DFS must be disabled before calling this function.
 */
static int program_dfs(enum s32g_dfs_type dfs,
		       const uint32_t params[][DFS_PARAMS_NR])
{
	uint32_t port = 0;

	disable_dfs(dfs);

	for (port = 0; port < S32G_DFS_PORTS_NR; port++) {
		if (!(params[port][DFS_PORT_EN]))
			continue;
		mmio_write_32(DFS_DVPORTn(dfs, port),
			      DFS_DVPORTn_MFI(params[port][DFS_DVPORT_MFI]) |
			      DFS_DVPORTn_MFN(params[port][DFS_DVPORT_MFN]));
	}

	enable_dfs(dfs, params);

	return 0;
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
int sw_mux_clk_config(uint8_t cgm, uint8_t mux, uint8_t source)
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

void s32g_plat_clock_init(void)
{
	assert(ARRAY_SIZE(s32g_core_pll_phi_freq) ==
	       s32g_pll_phi_nr[S32G_CORE_PLL]);
	assert(ARRAY_SIZE(s32g_periph_pll_phi_freq) ==
	       s32g_pll_phi_nr[S32G_PERIPH_PLL]);
	assert(ARRAY_SIZE(s32g_accel_pll_phi_freq) ==
	       s32g_pll_phi_nr[S32G_ACCEL_PLL]);
	assert(ARRAY_SIZE(s32g_ddr_pll_phi_freq) ==
	       s32g_pll_phi_nr[S32G_DDR_PLL]);

	/* Prepare FXOSC to run on */
	start_fxosc();

	/* Configure the CORE_PLL */
	program_pll(S32G_CORE_PLL, S32G_REFCLK_FXOSC, s32g_core_pll_phi_freq,
		    s32g_pll_rdiv[S32G_CORE_PLL], s32g_pll_mfi[S32G_CORE_PLL],
		    s32g_pll_mfn[S32G_CORE_PLL]);
	/* Configure the CORE_DFS) */
	program_dfs(S32G_CORE_DFS, s32g_core_dfs_params);
	/* Configure the core CGM mux */
	sw_mux_clk_config(MC_CGM1, 0,
			  MC_CGM_MUXn_CSC_SEL_ARM_PLL_PHI0);

	/* Configure the PERIPH_PLL */
	program_pll(S32G_PERIPH_PLL, S32G_REFCLK_FXOSC,
		    s32g_periph_pll_phi_freq, s32g_pll_rdiv[S32G_PERIPH_PLL],
		    s32g_pll_mfi[S32G_PERIPH_PLL],
		    s32g_pll_mfn[S32G_PERIPH_PLL]);
	/* Configure the PERIPH_DFS */
	program_dfs(S32G_PERIPH_DFS, s32g_periph_dfs_params);
	/* Configure the LinFlexD CGM mux */
	sw_mux_clk_config(MC_CGM0, 8,
			  MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI3);
}
