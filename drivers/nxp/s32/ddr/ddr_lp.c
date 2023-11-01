/*
 * Copyright 2021-2023 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ddr_lp.h"
#include "ddr_init.h"

static void load_csr(uintptr_t load_from);
static void load_ddrc_regs(uintptr_t load_from);

#pragma weak ddrss_gpr_to_io_retention_mode

/* Store Configuration Status Registers. */
void store_csr(uintptr_t store_at)
{
	size_t i;
	uint16_t csr;
	uintptr_t current_addr = store_at;

	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	mmio_write_32(DDR_PHYA_UCCLKHCLKENABLES, HCLKEN_MASK | UCCLKEN_MASK);

	for (i = 0; i < csr_to_store_size; i++) {
		csr = mmio_read_16((uint32_t)(DDRSS_BASE_ADDR +
					      csr_to_store[i]));
		mmio_write_16(current_addr, csr);
		current_addr += sizeof(uint16_t);
	}

	mmio_write_32(DDR_PHYA_UCCLKHCLKENABLES, HCLKEN_MASK);
	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);
}

/* Load Configuration Status Registers. */
static void load_csr(uintptr_t load_from)
{
	size_t i;
	uint16_t csr;
	uintptr_t current_addr = load_from;

	for (i = 0; i < csr_to_store_size; i++) {
		csr = mmio_read_16(current_addr);
		current_addr += sizeof(uint16_t);
		mmio_write_16((uint32_t)(DDRSS_BASE_ADDR + csr_to_store[i]),
			      csr);
	}
}

/* Store DDRC registers which have been updated post-training. */
void store_ddrc_regs(uintptr_t store_at)
{
	size_t i;
	uint32_t value;
	uintptr_t current_addr = store_at;
	/* DDRC registers are stored right after the CSRs */
	current_addr += sizeof(uint16_t) * csr_to_store_size;
	current_addr += sizeof(uint32_t) - (current_addr % sizeof(uint32_t));

	for (i = 0; i < ddrc_to_store_size; i++) {
		value = mmio_read_32((uint32_t)(DDRC_BASE_ADDR +
						ddrc_to_store[i]));
		mmio_write_32(current_addr, value);
		current_addr += sizeof(uint32_t);
	}
}

/* Load DDRC registers. */
static void load_ddrc_regs(uintptr_t load_from)
{
	size_t i;
	uint32_t value;
	uintptr_t current_addr = load_from;
	/* DDRC registers are stored right after the CSRs */
	current_addr += sizeof(uint16_t) * csr_to_store_size;
	current_addr += sizeof(uint32_t) - (current_addr % sizeof(uint32_t));

	for (i = 0; i < ddrc_to_store_size; i++) {
		value = mmio_read_32(current_addr);
		mmio_write_32((uint32_t)(DDRC_BASE_ADDR + ddrc_to_store[i]),
			      value);
		current_addr += sizeof(uint32_t);
	}
}

void ddrss_gpr_to_io_retention_mode_mmio(void)
{
	uint32_t tmp32;

	/* Set PwrOkIn to 0 */
	tmp32 = mmio_read_32(DDR_RET_CONTROL_REG);
	mmio_write_32(DDR_RET_CONTROL_REG, tmp32 & (~DDR_RET_CONTROL_MASK));
	tmp32 = mmio_read_32(DDR_CONFIG_0_REG);
	mmio_write_32(DDR_CONFIG_0_REG, tmp32 | DDR_CONFIG_0_MEM_RET);
}

/* Set DDR_GPRs for DDR SubSystem transition to retention mode */
void ddrss_gpr_to_io_retention_mode(void)
{
	ddrss_gpr_to_io_retention_mode_mmio();
}

/* Transition the DDR SubSystem from normal mode to retention mode. */
void ddrss_to_io_retention_mode(void)
{
	uint32_t sbrctl, pwrctl, swctl, dfimisc, tmp32;

	/* Disable AXI Ports */
	mmio_write_32(DDRC_UMCTL2_MP_BASE_ADDR + OFFSET_DDRC_PCTRL_0,
		      DISABLE_AXI_PORT);
	mmio_write_32(DDRC_UMCTL2_MP_BASE_ADDR + OFFSET_DDRC_PCTRL_1,
		      DISABLE_AXI_PORT);
	mmio_write_32(DDRC_UMCTL2_MP_BASE_ADDR + OFFSET_DDRC_PCTRL_2,
		      DISABLE_AXI_PORT);

	do {
		tmp32 = mmio_read_32(DDRC_UMCTL2_MP_BASE_ADDR +
				     OFFSET_DDRC_STAT);
	} while (tmp32 != STAT_RESET_VALUE);

	/* Disable Scrubber */
	sbrctl = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_SBRCTL);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_SBRCTL,
		      sbrctl & (~SCRUB_EN_MASK));
	do {
		tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_SBRSTAT);
	} while ((tmp32 & SCRUB_BUSY_MASK) != SBRSTAT_SCRUBBER_NOT_BUSY);

	/* Move to Self Refresh */
	pwrctl = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_PWRCTL);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_PWRCTL,
		      pwrctl | SELFREF_SW_MASK);

	do {
		tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_STAT);
	} while ((tmp32 & OPERATING_MODE_MASK) != OPERATING_MODE_SELF_REFRESH);

	do {
		tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_STAT);
	} while ((tmp32 & SELFREF_TYPE_MASK) != SELFREF_TYPE_NOT_AUTO_SR_CTRL);

	do {
		tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_STAT);
	} while ((tmp32 & SELFREF_STATE_MASK) != SELFREF_STATE_SRPD);

	/* Transition Phy to LP3 */
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFIMISC,
		      DFIMISC_TRANSITION_PHY_TO_LP3);
	swctl = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_SWCTL);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_SWCTL,
		      swctl & (~SW_DONE_MASK));

	dfimisc = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFIMISC);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFIMISC,
		      dfimisc | DFI_FREQUENCY(DFIMISC_LP3_PHY_STATE));

#if !defined(PLAT_s32g3)
	/* Disable PHY Master. */
	tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DFIPHYMSTR);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DFIPHYMSTR,
		      tmp32 & ~DFIPHYMSTR_ENABLE);

	/* Wait for PHY Master to be disabled. */
	do {
		tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DFIPHYMSTR);
	} while ((tmp32 & DFIPHYMSTR_ENABLE) != DFIPHYMSTR_DISABLED);

	/* Wait for PHY Master request to be finished. */
	do {
		tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_STAT);
	} while (((tmp32 & SELFREF_TYPE_MASK) >>
		  SELFREF_TYPE_POS) == PHY_MASTER_REQUEST);
#endif

	/* Set DFIMISC.dfi_init_start to 1. */
	dfimisc = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFIMISC);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFIMISC,
		      dfimisc | DFI_INIT_START_MASK);

	/* Wait DFISTAT.dfi_init_complete to be 1. */
	do {
		tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFISTAT);
	} while ((tmp32 & DFI_INIT_COMPLETE_MASK) !=
		 DFISTAT_DFI_INIT_INCOMPLETE);

	dfimisc = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFIMISC);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFIMISC,
		      dfimisc | DFI_FREQUENCY(DFIMISC_LP3_PHY_STATE));

	/* Set DFIMISC.dfi_init_start to 0. */
	dfimisc = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFIMISC);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFIMISC,
		      dfimisc & (~DFI_INIT_START_MASK));

#if !defined(PLAT_s32g3)
	/* Enable PHY Master. */
	tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DFIPHYMSTR);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DFIPHYMSTR,
		      tmp32 | DFIPHYMSTR_ENABLE);

	/* Wait for PHY Master to be enabled. */
	do {
		tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DFIPHYMSTR);
	} while ((tmp32 & DFIPHYMSTR_ENABLE) != DFIPHYMSTR_ENABLE);
#endif

	/* Wait DFISTAT.dfi_init_complete to be 1. */
	do {
		tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_DFISTAT);
	} while ((tmp32 & DFI_INIT_COMPLETE_MASK) ==
		 DFISTAT_DFI_INIT_INCOMPLETE);

	swctl = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_SWCTL);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_SWCTL,
		      swctl | SW_DONE_MASK);

	do {
		tmp32 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_SWSTAT);
	} while ((tmp32 & SW_DONE_ACK_MASK) == SWSTAT_SW_NOT_DONE);

	ddrss_gpr_to_io_retention_mode();
}

/* Transition the DDR SubSystem from retention mode to normal mode. */
uint32_t ddrss_to_normal_mode(uintptr_t csr_array)
{
	uint32_t pwrctl, init0, ret;

	ret = load_register_cfg(ddrc_cfg_size, ddrc_cfg);
	load_ddrc_regs(csr_array);
	if (ret != NO_ERR)
		return ret;

	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	ret = load_dq_cfg(dq_swap_cfg_size, dq_swap_cfg);
	if (ret != NO_ERR)
		return ret;

	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

	init0 = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_INIT0);
	init0 |= SKIP_DRAM_INIT_MASK;
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_INIT0, init0);

	pwrctl = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_PWRCTL);
	mmio_write_32(DDRC_BASE_ADDR + OFFSET_DDRC_PWRCTL,
		      pwrctl | SELFREF_SW_MASK);

	/* Setup AXI ports parity */
	ret = set_axi_parity();
	if (ret != NO_ERR)
		return ret;

	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	ret = load_register_cfg_16(phy_cfg_size, phy_cfg);
	if (ret != NO_ERR)
		return ret;

	/* Optimal PLL */
	set_optimal_pll();
	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

	/* Reload saved CSRs */
	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	load_csr(csr_array);
	ret = load_register_cfg_16(pie_cfg_size, pie_cfg);
	if (ret != NO_ERR)
		return ret;

	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

	return post_train_setup(STORE_CSR_DISABLED | INIT_MEM_DISABLED |
				ADJUST_DDRC_DISABLED);
}
