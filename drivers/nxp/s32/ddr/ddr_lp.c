/*
 * Copyright 2021-2023, 2025 NXP
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

/* Store PHY Configuration Status Registers. */
static void store_phy_csr(uintptr_t store_at);
/* Store DDRC registers which have been updated post-training. */
static void store_ddrc_csr(uintptr_t store_at);
/* Store ddr_config information for low power mode. */
static void store_ddrss_cfg_retention(uintptr_t store_at);
/* Restore Configuration Status Registers for phy, ddrc and ddrss information for retention mode */
static struct ddr_retention_data load_ddr_retention_data(uintptr_t load_from);

#pragma weak ddrss_gpr_to_io_retention_mode

/* Store Configuration Status Registers for phy, ddrc and ddrss information for retention mode */
void store_csr(uintptr_t store_at)
{
	store_ddrss_cfg_retention(store_at);
	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	mmio_write_32(DDR_PHYA_UCCLKHCLKENABLES, HCLKEN_MASK | UCCLKEN_MASK);
	store_phy_csr(store_at);
	mmio_write_32(DDR_PHYA_UCCLKHCLKENABLES, HCLKEN_MASK);
	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);
	store_ddrc_csr(store_at);
}

/* Store PHY Configuration Status Registers. */
static void store_phy_csr(uintptr_t store_at)
{
	struct ddr_fw_layout *ddr_fw = (struct ddr_fw_layout *)(store_at);
	uintptr_t current_addr;
	size_t i;

	current_addr = store_at + sizeof(struct ddr_fw_layout) + ddr_fw->phy_csr_offset;

	for (i = 0; i < ddr_config.phy_csr_size; i++) {
		mmio_write_32(current_addr, ddr_config.phy_csr[i]);
		current_addr += sizeof(uint32_t);
		mmio_write_16(current_addr, mmio_read_16(ddr_config.phy_csr[i]));
		current_addr += sizeof(uint32_t);
	}
}

/* Store DDRC registers which have been updated post-training. */
static void store_ddrc_csr(uintptr_t store_at)
{
	struct ddr_fw_layout *ddr_fw = (struct ddr_fw_layout *)(store_at);
	uintptr_t current_addr;
	size_t i;

	current_addr = store_at + sizeof(struct ddr_fw_layout) + ddr_fw->ddrc_csr_offset;

	for (i = 0; i < ddr_config.ddrc_csr_size; i++) {
		mmio_write_32(current_addr, ddr_config.ddrc_csr[i]);
		current_addr += sizeof(uint32_t);
		mmio_write_32(current_addr, mmio_read_32(ddr_config.ddrc_csr[i]));
		current_addr += sizeof(uint32_t);
	}
}

/* Store ddr_config information for low power mode. */
static void store_ddrss_cfg_retention(uintptr_t store_at)
{
	/* Update offsets for phy_csr & ddrc_csr */
	struct ddr_fw_layout ddr_ret_layout = ddr_layout;
	/* Set imem and dmem sizes to 0, they are not needed when exiting low-power mode */
	ddr_ret_layout.imem_1d_size = 0;
	ddr_ret_layout.dmem_1d_size = 0;
	ddr_ret_layout.imem_2d_size = 0;
	ddr_ret_layout.dmem_2d_size = 0;
	ddr_ret_layout.phy_csr_offset = ddr_ret_layout.imem_1d_offset;
	ddr_ret_layout.ddrc_csr_offset = ddr_ret_layout.phy_csr_offset +
					sizeof(struct regconf) * ddr_ret_layout.phy_csr_size;

	memcpy((void *)(store_at), &ddr_ret_layout, sizeof(struct ddr_fw_layout));
	store_at += sizeof(struct ddr_fw_layout);
	memcpy((void *)(store_at + ddr_ret_layout.ddrc_offset),
	       ddr_config.ddrc, sizeof(struct regconf) * ddr_config.ddrc_size);
	memcpy((void *)(store_at + ddr_ret_layout.dq_swap_offset),
	       ddr_config.dq_swap, sizeof(struct dqconf) * ddr_config.dq_swap_size);
	memcpy((void *)(store_at + ddr_ret_layout.phy_offset),
	       ddr_config.phy, sizeof(struct regconf_16) * ddr_config.phy_size);
	memcpy((void *)(store_at + ddr_ret_layout.pie_offset),
	       ddr_config.pie, sizeof(struct regconf_16) * ddr_config.pie_size);
}

/* Restore Configuration Status Registers for phy, ddrc and ddrss information for retention mode */
static struct ddr_retention_data load_ddr_retention_data(uintptr_t load_from)
{
	uintptr_t current_addr = load_from;
	struct ddr_retention_data ret_config;
	/* Load DDR firmware layout */
	ddr_layout = load_ddr_firmware_layout(load_from);
	current_addr += sizeof(struct ddr_fw_layout);

	/* Load content for ddrc, dq_swap, phy and pie arrays */
	ret_config.ddrc = (struct regconf *)(current_addr + ddr_layout.ddrc_offset);
	ret_config.ddrc_size = ddr_layout.ddrc_size;
	ret_config.dq_swap = (struct dqconf *)(current_addr + ddr_layout.dq_swap_offset);
	ret_config.dq_swap_size = ddr_layout.dq_swap_size;
	ret_config.phy = (struct regconf_16 *)(current_addr + ddr_layout.phy_offset);
	ret_config.phy_size = ddr_layout.phy_size;
	ret_config.pie = (struct regconf_16 *)(current_addr + ddr_layout.pie_offset);
	ret_config.pie_size = ddr_layout.pie_size;
	ret_config.phy_csr = (struct regconf *)(current_addr + ddr_layout.phy_csr_offset);
	ret_config.phy_csr_size = ddr_layout.phy_csr_size;
	ret_config.ddrc_csr = (struct regconf *)(current_addr + ddr_layout.ddrc_csr_offset);
	ret_config.ddrc_csr_size = ddr_layout.ddrc_csr_size;

	return ret_config;
}

void ddrss_gpr_to_io_retention_mode_mmio(void)
{
	/* Set PwrOkIn to 0 */
	mmio_clrbits_32(DDR_RET_CONTROL_REG, DDR_RET_CONTROL_MASK);
	mmio_setbits_32(DDR_CONFIG_0_REG, DDR_CONFIG_0_MEM_RET);
}

/* Set DDR_GPRs for DDR SubSystem transition to retention mode */
void ddrss_gpr_to_io_retention_mode(void)
{
	ddrss_gpr_to_io_retention_mode_mmio();
}

/* Transition the DDR SubSystem from normal mode to retention mode. */
void ddrss_to_io_retention_mode(void)
{
	uint32_t tmp32;

	/* Disable AXI Ports */
	mmio_write_32(DDRC_UMCTL2_MP_BASE + OFFSET_DDRC_PCTRL_0, DISABLE_AXI_PORT);
	mmio_write_32(DDRC_UMCTL2_MP_BASE + OFFSET_DDRC_PCTRL_1, DISABLE_AXI_PORT);
	mmio_write_32(DDRC_UMCTL2_MP_BASE + OFFSET_DDRC_PCTRL_2, DISABLE_AXI_PORT);

	do {
		tmp32 = mmio_read_32(DDRC_UMCTL2_MP_BASE + OFFSET_DDRC_STAT);
	} while (tmp32 != STAT_RESET_VALUE);

	/* Disable Scrubber */
	mmio_clrbits_32(DDRC_BASE + OFFSET_DDRC_SBRCTL, SCRUB_EN_MASK);

	do {
		tmp32 = mmio_read_32(DDRC_BASE + OFFSET_DDRC_SBRSTAT);
	} while ((tmp32 & SCRUB_BUSY_MASK) != SBRSTAT_SCRUBBER_NOT_BUSY);

	/* Move to Self Refresh */
	mmio_setbits_32(DDRC_BASE + OFFSET_DDRC_PWRCTL, SELFREF_SW_MASK);

	do {
		tmp32 = mmio_read_32(DDRC_BASE + OFFSET_DDRC_STAT);
	} while ((tmp32 & OPERATING_MODE_MASK) != OPERATING_MODE_SELF_REFRESH);

	do {
		tmp32 = mmio_read_32(DDRC_BASE + OFFSET_DDRC_STAT);
	} while ((tmp32 & SELFREF_TYPE_MASK) != SELFREF_TYPE_NOT_AUTO_SR_CTRL);

	do {
		tmp32 = mmio_read_32(DDRC_BASE + OFFSET_DDRC_STAT);
	} while ((tmp32 & SELFREF_STATE_MASK) != SELFREF_STATE_SRPD);

	/* Transition Phy to LP3 */
	mmio_write_32(DDRC_BASE + OFFSET_DDRC_DFIMISC, DFIMISC_TRANSITION_PHY_TO_LP3);
	mmio_clrbits_32(DDRC_BASE + OFFSET_DDRC_SWCTL, SW_DONE_MASK);
	mmio_setbits_32(DDRC_BASE + OFFSET_DDRC_DFIMISC,
			DFI_FREQUENCY(DFIMISC_LP3_PHY_STATE));

#if !defined(PLAT_s32g3)
	/* Disable PHY Master. */
	mmio_clrbits_32(DDRC_BASE + OFFSET_DFIPHYMSTR, DFIPHYMSTR_ENABLE);

	/* Wait for PHY Master to be disabled. */
	do {
		tmp32 = mmio_read_32(DDRC_BASE + OFFSET_DFIPHYMSTR);
	} while ((tmp32 & DFIPHYMSTR_ENABLE) != DFIPHYMSTR_DISABLED);

	/* Wait for PHY Master request to be finished. */
	do {
		tmp32 = mmio_read_32(DDRC_BASE + OFFSET_DDRC_STAT);
	} while (((tmp32 & SELFREF_TYPE_MASK) >>
		  SELFREF_TYPE_POS) == PHY_MASTER_REQUEST);
#endif

	/* Set DFIMISC.dfi_init_start to 1. */
	mmio_setbits_32(DDRC_BASE + OFFSET_DDRC_DFIMISC, DFI_INIT_START_MASK);

	/* Wait DFISTAT.dfi_init_complete to be 1. */
	do {
		tmp32 = mmio_read_32(DDRC_BASE + OFFSET_DDRC_DFISTAT);
	} while ((tmp32 & DFI_INIT_COMPLETE_MASK) !=
		 DFISTAT_DFI_INIT_INCOMPLETE);

	mmio_setbits_32(DDRC_BASE + OFFSET_DDRC_DFIMISC,
			DFI_FREQUENCY(DFIMISC_LP3_PHY_STATE));

	/* Set DFIMISC.dfi_init_start to 0. */
	mmio_clrbits_32(DDRC_BASE + OFFSET_DDRC_DFIMISC, DFI_INIT_START_MASK);

#if !defined(PLAT_s32g3)
	/* Enable PHY Master. */
	mmio_setbits_32(DDRC_BASE + OFFSET_DFIPHYMSTR, DFIPHYMSTR_ENABLE);

	/* Wait for PHY Master to be enabled. */
	do {
		tmp32 = mmio_read_32(DDRC_BASE + OFFSET_DFIPHYMSTR);
	} while ((tmp32 & DFIPHYMSTR_ENABLE) != DFIPHYMSTR_ENABLE);
#endif

	/* Wait DFISTAT.dfi_init_complete to be 1. */
	do {
		tmp32 = mmio_read_32(DDRC_BASE + OFFSET_DDRC_DFISTAT);
	} while ((tmp32 & DFI_INIT_COMPLETE_MASK) ==
		 DFISTAT_DFI_INIT_INCOMPLETE);

	mmio_setbits_32(DDRC_BASE + OFFSET_DDRC_SWCTL, SW_DONE_MASK);

	do {
		tmp32 = mmio_read_32(DDRC_BASE + OFFSET_DDRC_SWSTAT);
	} while ((tmp32 & SW_DONE_ACK_MASK) == SWSTAT_SW_NOT_DONE);

	ddrss_gpr_to_io_retention_mode();
}

/* Transition the DDR SubSystem from retention mode to normal mode. */
uint32_t ddrss_to_normal_mode(uintptr_t load_from)
{
	uint32_t ret;

	/* Restore ddr_config retention data */
	struct ddr_retention_data ddr_ret_config = load_ddr_retention_data(load_from);

	ret = load_register_cfg(ddr_ret_config.ddrc_size, ddr_ret_config.ddrc);
	ret = load_register_cfg(ddr_ret_config.ddrc_csr_size, ddr_ret_config.ddrc_csr);
	if (ret != NO_ERR)
		return ret;

	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	ret = load_dq_cfg(ddr_ret_config.dq_swap_size, ddr_ret_config.dq_swap);
	if (ret != NO_ERR)
		return ret;

	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

	mmio_setbits_32(DDRC_BASE + OFFSET_DDRC_INIT0, SKIP_DRAM_INIT_MASK);

	mmio_setbits_32(DDRC_BASE + OFFSET_DDRC_PWRCTL, SELFREF_SW_MASK);

	/* Setup AXI ports parity */
	ret = set_axi_parity();
	if (ret != NO_ERR)
		return ret;

	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	ret = load_register_cfg_16(ddr_ret_config.phy_size, ddr_ret_config.phy);
	if (ret != NO_ERR)
		return ret;

	/* Optimal PLL */
	set_optimal_pll(ddr_layout.frequency);
	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

	/* Reload saved CSRs */
	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	ret = load_register_cfg(ddr_ret_config.phy_csr_size, ddr_ret_config.phy_csr);
	ret = load_register_cfg_16(ddr_ret_config.pie_size, ddr_ret_config.pie);
	if (ret != NO_ERR)
		return ret;

	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

	return post_train_setup(STORE_CSR_DISABLED | INIT_MEM_DISABLED |
				ADJUST_DDRC_DISABLED);
}
