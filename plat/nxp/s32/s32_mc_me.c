/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <utils_def.h>
#include <lib/mmio.h>
#include <common/debug.h>
#include "s32_mc_rgm.h"
#include "s32_mc_me.h"

/* Apply changes to MC_ME partitions */
void mc_me_apply_hw_changes(void)
{
	mmio_write_32(S32_MC_ME_CTL_KEY, S32_MC_ME_CTL_KEY_KEY);
	mmio_write_32(S32_MC_ME_CTL_KEY, S32_MC_ME_CTL_KEY_INVERTEDKEY);
}

/*
 * PART<n>_CORE<m> register accessors
 */

static void mc_me_part_core_addr_write(uintptr_t addr, uint32_t part,
				       uint32_t core)
{
	uint32_t addr_lo;

	addr_lo = (uint32_t)(addr & 0xFFFFFFFC);
	mmio_write_32(S32_MC_ME_PRTN_N_CORE_M_ADDR(part, core), addr_lo);
}

static void mc_me_part_core_pconf_write_cce(uint32_t cce_bit, uint32_t p,
					    uint32_t c)
{
	uint32_t pconf;

	pconf = mmio_read_32(S32_MC_ME_PRTN_N_CORE_M_PCONF(p, c)) &
			~S32_MC_ME_PRTN_N_CORE_M_PCONF_CCE_MASK;
	pconf |= (cce_bit & S32_MC_ME_PRTN_N_CORE_M_PCONF_CCE_MASK);
	mmio_write_32(S32_MC_ME_PRTN_N_CORE_M_PCONF(p, c), pconf);
}

static void mc_me_part_core_pupd_write_ccupd(uint32_t ccupd_bit, uint32_t p,
					    uint32_t c)
{
	uint32_t pupd;

	pupd = mmio_read_32(S32_MC_ME_PRTN_N_CORE_M_PUPD(p, c)) &
			~S32_MC_ME_PRTN_N_CORE_M_PUPD_CCUPD_MASK;
	pupd |= (ccupd_bit & S32_MC_ME_PRTN_N_CORE_M_PUPD_CCUPD_MASK);
	mmio_write_32(S32_MC_ME_PRTN_N_CORE_M_PUPD(p, c), pupd);
}

/*
 * PART<n>_[XYZ] register accessors
 */

static void mc_me_part_pconf_write_pce(uint32_t pce_bit, uint32_t p)
{
	uint32_t pconf;

	pconf = mmio_read_32(S32_MC_ME_PRTN_N_PCONF(p)) &
			~S32_MC_ME_PRTN_N_PCONF_PCE_MASK;
	pconf |= (pce_bit & S32_MC_ME_PRTN_N_PCONF_PCE_MASK);
	mmio_write_32(S32_MC_ME_PRTN_N_PCONF(p), pconf);
}

static void mc_me_part_pconf_write_osse(uint32_t osse_bit, uint32_t p)
{
	uint32_t pconf;

	pconf = mmio_read_32(S32_MC_ME_PRTN_N_PCONF(p)) &
			~S32_MC_ME_PRTN_N_PCONF_OSSE_MASK;
	pconf |= (osse_bit & S32_MC_ME_PRTN_N_PCONF_OSSE_MASK);
	mmio_write_32(S32_MC_ME_PRTN_N_PCONF(p), pconf);
}

static void mc_me_part_pupd_write_pcud(uint32_t pcud_bit, uint32_t p)
{
	uint32_t pupd;

	pupd = mmio_read_32(S32_MC_ME_PRTN_N_PUPD(p)) &
			~S32_MC_ME_PRTN_N_PUPD_PCUD_MASK;
	pupd |= (pcud_bit & S32_MC_ME_PRTN_N_PUPD_PCUD_MASK);
	mmio_write_32(S32_MC_ME_PRTN_N_PUPD(p), pupd);
}

static void mc_me_part_pupd_update_and_wait(uint32_t mask, uint32_t p)
{
	uint32_t pupd, pconf, stat;

	pupd = mmio_read_32(S32_MC_ME_PRTN_N_PUPD(p));
	pupd |= mask;
	mmio_write_32(S32_MC_ME_PRTN_N_PUPD(p), pupd);

	mc_me_apply_hw_changes();

	/* wait for the updates to apply */
	pconf = mmio_read_32(S32_MC_ME_PRTN_N_PCONF(p));
	do {
		stat = mmio_read_32(S32_MC_ME_PRTN_N_STAT(p));
	} while ((stat & mask) != (pconf & mask));
}

/*
 * PART<n>_COFB<m> register accessors
 */

static void mc_me_part_cofb_clken_write_req(uint32_t req, uint32_t val,
					    uint32_t part)
{
	uint32_t clken;

	clken = mmio_read_32(S32_MC_ME_PRTN_N_COFB_0_CLKEN(part));
	clken |= ((val & 0x1) << req);
	mmio_write_32(S32_MC_ME_PRTN_N_COFB_0_CLKEN(part), clken);
}

/*
 * Higher-level constructs
 */

/* First part of the "Software reset partition turn-on flow chart",
 * as per S32Gen1 RefMan.
 */
void mc_me_enable_partition(uint32_t part)
{
	uint32_t reg;

	/* Partition 0 is already enabled by BootROM */
	if (part == 0)
		return;

	mc_me_part_pconf_write_pce(S32_MC_ME_PRTN_N_PCONF_PCE_MASK, part);
	mc_me_part_pupd_update_and_wait(S32_MC_ME_PRTN_N_PUPD_PCUD_MASK, part);

	/* Unlock RDC register write */
	mmio_write_32(RDC_RD_CTRL(part), RDC_CTRL_UNLOCK);
	/* Enable the XBAR interface */
	mmio_write_32(RDC_RD_CTRL(part),
		      mmio_read_32(RDC_RD_CTRL(part)) & ~RDC_CTRL_XBAR_DISABLE);
	/* Wait until XBAR interface is enabled */
	while (mmio_read_32(RDC_RD_CTRL(part)) & RDC_CTRL_XBAR_DISABLE)
		;
	/* Release partition reset */
	reg = mmio_read_32(S32_MC_RGM_PRST(part));
	reg &= ~MC_RGM_PRST_PERIPH_N_RST(0);
	mmio_write_32(S32_MC_RGM_PRST(part), reg);

	/* Clear OSSE bit */
	mc_me_part_pconf_write_osse(0, part);

	mc_me_part_pupd_update_and_wait(S32_MC_ME_PRTN_N_PUPD_OSSUD_MASK,
					part);
	while (mmio_read_32(S32_MC_RGM_PSTAT(part)) &
			    MC_RGM_STAT_PERIPH_N_STAT(0))
		;
	/* Lock RDC register write */
	reg = mmio_read_32(RDC_RD_CTRL(part));
	reg &= ~RDC_CTRL_UNLOCK;
	mmio_write_32(RDC_RD_CTRL(part), reg);
}

/* Second part of the "Software reset partition turn-on flow chart" from the
 * S32Gen1 RefMan.
 *
 * Partition blocks must only be enabled after mc_me_enable_partition()
 * has been called for their respective partition.
 */
void mc_me_enable_partition_block(uint32_t part, uint32_t block)
{
	mc_me_part_cofb_clken_write_req(block, 1, part);
	mc_me_part_pupd_update_and_wait(S32_MC_ME_PRTN_N_PUPD_PCUD_MASK, part);
}

bool s32_core_in_reset(uint32_t core)
{
	uint32_t stat, rst;

	rst = BIT(get_rgm_a53_bit(core));
	stat = mmio_read_32(S32_MC_RGM_PSTAT(S32_MC_RGM_RST_DOMAIN_CA53));
	return ((stat & rst) != 0);
}

static bool s32_core_clock_running(uint32_t part, uint32_t core)
{
	uint32_t stat;

	stat = mmio_read_32(S32_MC_ME_PRTN_N_CORE_M_STAT(part, core));
	return ((stat & S32_MC_ME_PRTN_N_CORE_M_STAT_CCS_MASK) != 0);
}

static void enable_a53_partition(void)
{
	uint32_t pconf;

	pconf = mmio_read_32(S32_MC_ME_PRTN_N_STAT(S32_MC_ME_CA53_PART));

	/* Already enabled */
	if (pconf & S32_MC_ME_PRTN_N_PCONF_PCE_MASK)
		return;

	mc_me_part_pconf_write_pce(S32_MC_ME_PRTN_N_PCONF_PCE_MASK,
				   S32_MC_ME_CA53_PART);
	mc_me_part_pupd_write_pcud(S32_MC_ME_PRTN_N_PUPD_PCUD_MASK,
				   S32_MC_ME_CA53_PART);
	mc_me_apply_hw_changes();
}

static void enable_a53_core_cluster(uint32_t core)
{
	uint32_t pconf_cluster = mc_me_get_cluster_ptrn(core);
	uint32_t stat, part = S32_MC_ME_CA53_PART;
	uint64_t addr;

	addr = MC_ME_PRTN_PART(part, pconf_cluster) +
	    S32_MC_ME_PRTN_N_STAT_OFF;
	stat = mmio_read_32(addr);

	if (stat & S32_MC_ME_PRTN_N_CORE_M_STAT_CCS_MASK)
		return;

	/* When in performance (i.e., not in lockstep) mode, the following
	 * bits from the reset sequence are only defined for the first core
	 * of each CA53 cluster. Make sure this part of the sequence only runs
	 * on even-numbered cores.
	 */
	/* Enable clock and make changes effective */
	addr = MC_ME_PRTN_PART(part, pconf_cluster) +
	    S32_MC_ME_PRTN_N_PCONF_OFF;
	mmio_write_32(addr, S32_MC_ME_PRTN_N_CORE_M_PCONF_CCE_MASK);

	addr = MC_ME_PRTN_PART(part, pconf_cluster) +
	    S32_MC_ME_PRTN_N_PUPD_OFF;
	mmio_write_32(addr, S32_MC_ME_PRTN_N_CORE_M_PUPD_CCUPD_MASK);

	mc_me_apply_hw_changes();

	/* Wait for the core clock to become active */
	addr = MC_ME_PRTN_PART(part, pconf_cluster) +
	    S32_MC_ME_PRTN_N_STAT_OFF;
	do {
		stat = mmio_read_32(addr);
		stat &= S32_MC_ME_PRTN_N_CORE_M_STAT_CCS_MASK;
	} while (stat != S32_MC_ME_PRTN_N_CORE_M_STAT_CCS_MASK);
}

static void set_core_high_addr(uintptr_t addr, uint32_t core)
{
	const struct a53_haddr_mapping *map;
	uint32_t addr_hi, reg_val, field_off, reg_off;
	size_t size;

	map = s32_get_a53_haddr_mappings(&size);

	if (core >= size)
		panic();

	reg_off = map[core].reg;
	field_off = map[core].field_off;

	addr_hi = (uint32_t)(addr >> 32);
	reg_val = mmio_read_32(GPR_BASE_ADDR + reg_off);

	reg_val |= ((addr_hi & CA53_RVBARADDR_MASK) << field_off);
	mmio_write_32(GPR_BASE_ADDR + reg_off, reg_val);
}

/** Reset and initialize secondary A53 core identified by its number
 *  in one of the MC_ME partitions
 */
void s32_kick_secondary_ca53_core(uint32_t core, uintptr_t entrypoint)
{
	uint32_t rst;
	uint32_t rst_mask = BIT(get_rgm_a53_bit(core));
	const uint32_t part = S32_MC_ME_CA53_PART;

	enable_a53_partition();

	set_core_high_addr(entrypoint, core);
	/* The MC_ME provides the 32 low-order bits for the core's
	 * start address
	 */
	mc_me_part_core_addr_write(entrypoint, part, core);

	enable_a53_core_cluster(core);

	/* Release the core reset */
	rst = mmio_read_32(S32_MC_RGM_PRST(S32_MC_RGM_RST_DOMAIN_CA53));

	/* Forced reset */
	if (!(rst & rst_mask)) {
		rst |= rst_mask;
		mmio_write_32(S32_MC_RGM_PRST(S32_MC_RGM_RST_DOMAIN_CA53),
			      rst);
		while (!s32_core_in_reset(core))
			;
	}

	rst = mmio_read_32(S32_MC_RGM_PRST(S32_MC_RGM_RST_DOMAIN_CA53));
	rst &= ~rst_mask;
	mmio_write_32(S32_MC_RGM_PRST(S32_MC_RGM_RST_DOMAIN_CA53), rst);
	/* Wait for reset bit to deassert */
	while (s32_core_in_reset(core))
		;
}

void s32_reset_core(uint8_t part, uint8_t core)
{
	uint32_t resetc;
	uint32_t statv;
	uintptr_t prst;
	uintptr_t pstat;

	if (part == S32_MC_ME_CA53_PART) {
		resetc = BIT(get_rgm_a53_bit(core));
		prst = S32_MC_RGM_PRST(S32_MC_RGM_RST_DOMAIN_CA53);
		pstat = S32_MC_RGM_PSTAT(S32_MC_RGM_RST_DOMAIN_CA53);
	} else {
		/* M7 cores */
		resetc = S32_MC_RGM_RST_CM7_BIT(core);
		prst = S32_MC_RGM_PRST(S32_MC_RGM_RST_DOMAIN_CM7);
		pstat = S32_MC_RGM_PSTAT(S32_MC_RGM_RST_DOMAIN_CM7);
	}
	statv = resetc;

	/* Assert the core reset */
	resetc |= mmio_read_32(prst);
	mmio_write_32(prst, resetc);

	/* Wait reset status */
	while (!(mmio_read_32(pstat) & statv))
		;
}

void s32_turn_off_core(uint8_t part, uint8_t core)
{
	uint32_t stat;

	/* Assumption : The core is already in WFI */
	stat = mmio_read_32(S32_MC_ME_PRTN_N_CORE_M_STAT(part, core));

	/* The clock isn't enabled */
	if (!(stat & S32_MC_ME_PRTN_N_CORE_M_STAT_CCS_MASK))
		return;

	/* Wait for WFI */
	do {
		stat = mmio_read_32(S32_MC_ME_PRTN_N_CORE_M_STAT(part, core));
	} while (!(stat & S32_MC_ME_PRTN_N_CORE_M_STAT_WFI_MASK));

	/* Disable the core clock */
	mc_me_part_core_pconf_write_cce(0, part, core);
	mc_me_part_core_pupd_write_ccupd(1, part, core);

	/* Write valid key sequence to trigger the update. */
	mc_me_apply_hw_changes();

	/* Wait for the core clock to become inactive */
	while (s32_core_clock_running(part, core))
		;

	s32_reset_core(part, core);
}

void s32_disable_cofb_clk(uint8_t part, uint32_t keep_blocks)
{
	uint32_t pconf;

	if (!mmio_read_32(S32_MC_ME_PRTN_N_COFB0_CLKEN(part)))
		return;

	/* Disable all blocks */
	mmio_write_32(S32_MC_ME_PRTN_N_COFB0_CLKEN(part), keep_blocks);

	pconf = mmio_read_32(S32_MC_ME_PRTN_N_PCONF(part));

	/* Keep the partition on if not all the blocks are disabled */
	if (keep_blocks == 0)
		pconf &= ~S32_MC_ME_PRTN_N_PCONF_PCE_MASK;

	/* Disable the clock to IPs */
	mmio_write_32(S32_MC_ME_PRTN_N_PCONF(part), pconf);

	/* Initiate the clock hardware process */
	mc_me_part_pupd_write_pcud(S32_MC_ME_PRTN_N_PUPD_PCUD_MASK, part);

	/* Write valid key sequence to trigger the update. */
	mc_me_apply_hw_changes();

	/* Make sure the COFB clock is gated */
	while (mmio_read_32(S32_MC_ME_PRTN_N_COFB0_STAT(part)) != keep_blocks)
		;
}

void s32_destructive_reset(void)
{
	/* Prevent reset escalation */
	mmio_write_32(S32_MC_RGM_DRET_ADDR, 0);

	mmio_write_32(MC_ME_MODE_CONF, MC_ME_MODE_CONF_DRST);
	mmio_write_32(MC_ME_MODE_UPD, MC_ME_MODE_UPD_UPD);

	/* Write valid key sequence to trigger the reset. */
	mc_me_apply_hw_changes();
}

