/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <utils_def.h>
#include <lib/mmio.h>
#include <common/debug.h>
#include "s32g_mc_me.h"
#include "s32g_mc_rgm.h"

void plat_secondary_cold_boot_setup(void);


/* Apply changes to MC_ME partitions */
static void mc_me_apply_hw_changes(void)
{
	mmio_write_32(S32G_MC_ME_CTL_KEY, S32G_MC_ME_CTL_KEY_KEY);
	mmio_write_32(S32G_MC_ME_CTL_KEY, S32G_MC_ME_CTL_KEY_INVERTEDKEY);
}

/*
 * PART<n>_CORE<m> register accessors
 */

static void mc_me_part_core_addr_write(uintptr_t addr, uint32_t part,
				       uint32_t core)
{
	uint32_t addr_lo;

	addr_lo = (uint32_t)(addr & 0xFFFFFFFC);
	mmio_write_32(S32G_MC_ME_PRTN_N_CORE_M_ADDR(part, core), addr_lo);
}

static void mc_me_part_core_pconf_write_cce(uint32_t cce_bit, uint32_t p,
					    uint32_t c)
{
	uint32_t pconf;

	pconf = mmio_read_32(S32G_MC_ME_PRTN_N_CORE_M_PCONF(p, c)) &
			~S32G_MC_ME_PRTN_N_CORE_M_PCONF_CCE_MASK;
	pconf |= (cce_bit & S32G_MC_ME_PRTN_N_CORE_M_PCONF_CCE_MASK);
	mmio_write_32(S32G_MC_ME_PRTN_N_CORE_M_PCONF(p, c), pconf);
}

static void mc_me_part_core_pupd_write_ccupd(uint32_t ccupd_bit, uint32_t p,
					    uint32_t c)
{
	uint32_t pupd;

	pupd = mmio_read_32(S32G_MC_ME_PRTN_N_CORE_M_PUPD(p, c)) &
			~S32G_MC_ME_PRTN_N_CORE_M_PUPD_CCUPD_MASK;
	pupd |= (ccupd_bit & S32G_MC_ME_PRTN_N_CORE_M_PUPD_CCUPD_MASK);
	mmio_write_32(S32G_MC_ME_PRTN_N_CORE_M_PUPD(p, c), pupd);
}

/*
 * PART<n>_[XYZ] register accessors
 */

static void mc_me_part_pconf_write_pce(uint32_t pce_bit, uint32_t p)
{
	uint32_t pconf;

	pconf = mmio_read_32(S32G_MC_ME_PRTN_N_PCONF(p) &
			~S32G_MC_ME_PRTN_N_PCONF_PCE_MASK);
	pconf |= (pce_bit & S32G_MC_ME_PRTN_N_PCONF_PCE_MASK);
	mmio_write_32(S32G_MC_ME_PRTN_N_PCONF(p), pconf);
}

static void mc_me_part_pupd_write_pcud(uint32_t pcud_bit, uint32_t p)
{
	uint32_t pupd;

	pupd = mmio_read_32(S32G_MC_ME_PRTN_N_PUPD(p) &
			~S32G_MC_ME_PRTN_N_PUPD_PCUD_MASK);
	pupd |= (pcud_bit & S32G_MC_ME_PRTN_N_PUPD_PCUD_MASK);
	mmio_write_32(S32G_MC_ME_PRTN_N_PUPD(p), pupd);
}

static void mc_me_part_pupd_update_and_wait(uint32_t mask, uint32_t p)
{
	uint32_t pupd, pconf, stat;

	pupd = mmio_read_32(S32G_MC_ME_PRTN_N_PUPD(p));
	pupd |= mask;
	mmio_write_32(S32G_MC_ME_PRTN_N_PUPD(p), pupd);

	mc_me_apply_hw_changes();

	/* wait for the updates to apply */
	pconf = mmio_read_32(S32G_MC_ME_PRTN_N_PCONF(p));
	do {
		stat = mmio_read_32(S32G_MC_ME_PRTN_N_STAT(p));
	} while ((stat & mask) != (pconf & mask));
}

/*
 * PART<n>_COFB<m> register accessors
 */

static void mc_me_part_cofb_clken_write_req(uint32_t req, uint32_t val,
					    uint32_t part)
{
	uint32_t clken;

	clken = mmio_read_32(S32G_MC_ME_PRTN_N_COFB_0_CLKEN(part));
	clken |= ((val & 0x1) << req);
	mmio_write_32(S32G_MC_ME_PRTN_N_COFB_0_CLKEN(part), clken);
}

/*
 * Higher-level constructs
 */

/* First part of the "Software reset partition turn-on flow chart",
 * as per S32G RefMan.
 */
void mc_me_enable_partition(uint32_t part)
{
	uint32_t reg;

	/* Partition 0 is already enabled by BootROM */
	if (part == 0)
		return;

	mc_me_part_pconf_write_pce(1, part);
	mc_me_part_pupd_update_and_wait(S32G_MC_ME_PRTN_N_PUPD_PCUD_MASK, part);

	/* Unlock RDC register write */
	mmio_write_32(RDC_RD_CTRL(part), RDC_CTRL_UNLOCK);
	/* Enable the XBAR interface */
	mmio_write_32(RDC_RD_CTRL(part),
		      mmio_read_32(RDC_RD_CTRL(part)) & ~RDC_CTRL_XBAR_DISABLE);
	/* Wait until XBAR interface is enabled */
	while (mmio_read_32(RDC_RD_CTRL(part)) & RDC_CTRL_XBAR_DISABLE)
		;
	/* Release partition reset */
	reg = mmio_read_32(S32G_MC_RGM_PRST(part));
	reg &= ~MC_RGM_PRST_PERIPH_N_RST(0);
	mmio_write_32(S32G_MC_RGM_PRST(part), reg);
	/* Clear OSSE bit */
	reg = mmio_read_32(S32G_MC_ME_PRTN_N_PCONF(part));
	reg &= ~S32G_MC_ME_PRTN_N_PCONF_OSSE_MASK;
	mmio_write_32(S32G_MC_ME_PRTN_N_PCONF(part), reg);
	mc_me_part_pupd_update_and_wait(S32G_MC_ME_PRTN_N_PUPD_OSSUD_MASK,
					part);
	while (mmio_read_32(S32G_MC_RGM_PSTAT(part)) &
			    MC_RGM_STAT_PERIPH_N_STAT(0))
		;
	/* Lock RDC register write */
	reg = mmio_read_32(RDC_RD_CTRL(part));
	reg &= ~RDC_CTRL_UNLOCK;
	mmio_write_32(RDC_RD_CTRL(part), reg);
}

/* Second part of the "Software reset partition turn-on flow chart" from the
 * S32G RefMan.
 *
 * Partition blocks must only be enabled after mc_me_enable_partition()
 * has been called for their respective partition.
 */
void mc_me_enable_partition_block(uint32_t part, uint32_t block)
{
	mc_me_part_cofb_clken_write_req(block, 1, part);
	mc_me_part_pupd_update_and_wait(S32G_MC_ME_PRTN_N_PUPD_PCUD_MASK, part);
}

static void core_high_addr_write(uintptr_t addr, uint32_t core)
{
	uint32_t addr_hi;
	uint32_t gpr09;

	addr_hi = (uint32_t)(addr >> 32);
	gpr09 = mmio_read_32(GPR_BASE_ADDR + GPR09_OFF);

	switch (core) {
	case 0:
		gpr09 |= ((addr_hi & CA53_0_0_RVBARADDR_39_32_MASK) <<
			   CA53_0_0_RVBARADDR_39_32_OFF);
		break;
	case 1:
		gpr09 |= ((addr_hi & CA53_0_1_RVBARADDR_39_32_MASK) <<
			   CA53_0_1_RVBARADDR_39_32_OFF);
		break;
	case 2:
		gpr09 |= ((addr_hi & CA53_1_0_RVBARADDR_39_32_MASK) <<
			   CA53_1_0_RVBARADDR_39_32_OFF);
		break;
	case 3:
		gpr09 |= ((addr_hi & CA53_1_1_RVBARADDR_39_32_MASK) <<
			   CA53_1_1_RVBARADDR_39_32_OFF);
		break;
	default:
		panic();
	}

	mmio_write_32(GPR_BASE_ADDR + GPR09_OFF, gpr09);
}

static bool s32g_core_in_reset(uint32_t core)
{
	uint32_t stat, rst;

	rst = S32G_MC_RGM_RST_CA53_BIT(core);
	stat = mmio_read_32(S32G_MC_RGM_PSTAT(S32G_MC_RGM_RST_DOMAIN_CA53));
	return ((stat & rst) != 0);
}

static bool s32g_core_clock_running(uint32_t core)
{
	uint32_t stat;
	const uint32_t part = S32G_MC_ME_CA53_PART;

	stat = mmio_read_32(S32G_MC_ME_PRTN_N_CORE_M_STAT(part, core & ~1));
	return ((stat & S32G_MC_ME_PRTN_N_CORE_M_STAT_CCS_MASK) != 0);
}

/** Reset and initialize secondary A53 core identified by its number
 *  in one of the MC_ME partitions
 */
static void s32g_kick_secondary_ca53_core(uint32_t core)
{
	uintptr_t core_start_addr = (uintptr_t)&plat_secondary_cold_boot_setup;
	uint32_t rst;
	const uint32_t part = S32G_MC_ME_CA53_PART;

	/* GPR09 provides the 8 high-order bits for the core's start addr */
	core_high_addr_write(core_start_addr, core);
	/* The MC_ME provides the 32 low-order bits for the core's
	 * start address
	 */
	mc_me_part_core_addr_write(core_start_addr, part, core);

	/* When in performance (i.e., not in lockstep) mode, the following
	 * bits from the reset sequence are only defined for the first core
	 * of each CA53 cluster. Make sure this part of the sequence only runs
	 * on even-numbered cores.
	 */
	/* Enable clock and make changes effective */
	mc_me_part_core_pconf_write_cce(1, part, core & ~1);
	mc_me_part_core_pupd_write_ccupd(1, part, core & ~1);
	mc_me_apply_hw_changes();
	/* Wait for the core clock to become active */
	while (!s32g_core_clock_running(core))
		;

	/* Release the core reset */
	rst = mmio_read_32(S32G_MC_RGM_PRST(S32G_MC_RGM_RST_DOMAIN_CA53));
	rst &= ~(S32G_MC_RGM_RST_CA53_BIT(core));
	mmio_write_32(S32G_MC_RGM_PRST(S32G_MC_RGM_RST_DOMAIN_CA53), rst);
	/* Wait for reset bit to deassert */
	while (s32g_core_in_reset(core))
		;
}

/** Reset and initialize all secondary A53 cores
 */
void s32g_kick_secondary_ca53_cores(void)
{
	/* Enable partition clocks */
	mc_me_part_pconf_write_pce(1, S32G_MC_ME_CA53_PART);
	mc_me_part_pupd_write_pcud(1, S32G_MC_ME_CA53_PART);
	mc_me_apply_hw_changes();

	s32g_kick_secondary_ca53_core(1);
	s32g_kick_secondary_ca53_core(2);
	s32g_kick_secondary_ca53_core(3);
}
