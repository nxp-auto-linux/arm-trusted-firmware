/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <libc/assert.h>
#include <s32g_clocks.h>
#include <s32g_mc_me.h>

/* Number of dividers for each PLL */
static const uint32_t s32g_pll_phi_nr[S32_PLL_NR] = {2, 8, 2, 1};

/* Array of parameters for each PLL */
static const uint32_t s32g_pll_rdiv[S32_PLL_NR] = {1, 1, 1, 1};
static const uint32_t s32g_pll_mfi[S32_PLL_NR] = {50, 50, 60, 0x21};
static const uint32_t s32g_pll_mfn[S32_PLL_NR] = {0, 0, 1, 0x1800};

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

static uint64_t plldig_set_refclk(enum s32_pll_type pll,
				  enum s32g_refclk refclk)
{
	uint64_t refclk_freq = S32_ERR_CLK_FREQ;
	uint32_t pllclkmux;

	switch (refclk) {
	case S32G_REFCLK_FIRC:
		pllclkmux = PLLDIG_PLLCLKMUX_REFCLK_FIRC;
		refclk_freq = S32_FIRC_FREQ;
		break;
	case S32G_REFCLK_FXOSC:
		pllclkmux = PLLDIG_PLLCLKMUX_REFCLK_FXOSC;
		refclk_freq = S32_FXOSC_FREQ;
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
static int program_pll(enum s32_pll_type pll, enum s32g_refclk refclk,
		const uint64_t freq[], uint32_t plldv_rdiv, uint32_t plldv_mfi,
		uint32_t pllfd_mfn)
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
	if (refclk_freq == S32_ERR_CLK_FREQ)
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

/**
 * BL31SSRAM stage cannot use clock driver to set DDR clock due to SSRAM size,
 * consequently it will have a raw management over DDR clock.
 */
void s32g_plat_ddr_clock_init(void)
{
	assert(ARRAY_SIZE(s32g_ddr_pll_phi_freq) ==
	       s32g_pll_phi_nr[S32_DDR_PLL]);

	start_fxosc();
	mc_me_enable_partition(S32_MC_ME_PRTN0);
	mc_me_enable_partition_block(S32_MC_ME_DDR_0_PART,
				     S32_MC_ME_DDR_0_REQ);
	program_pll(S32_DDR_PLL, S32G_REFCLK_FXOSC, s32g_ddr_pll_phi_freq,
		    s32g_pll_rdiv[S32_DDR_PLL], s32g_pll_mfi[S32_DDR_PLL],
		    s32g_pll_mfn[S32_DDR_PLL]);
	sw_mux_clk_config(MC_CGM5, 0, MC_CGM_MUXn_CSC_SEL_DDR_PLL_PHI0);
}
