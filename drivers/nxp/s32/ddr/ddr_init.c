/*
 * Copyright 2020-2023, 2025 NXP
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

#include "ddr_init.h"

static uint32_t ddr_init_cfg(const struct ddrss_config *config);
static uint32_t ddrc_init_cfg(const struct ddrss_config *config);
static uint32_t execute_training(const struct ddrss_config *config);
static uint32_t load_phy_image(uint32_t start_addr, size_t size,
			       const uint16_t image[]);
static struct ddrss_config load_ddr_config(uintptr_t load_from);

struct ddr_fw_layout ddr_layout;
struct ddrss_config ddr_config;

/* Main method needed to initialize ddr subsystem. */
uint32_t ddr_init(uintptr_t load_from)
{
	ddr_config = load_ddr_config(load_from + sizeof(struct ddr_fw_header));
	return ddr_init_cfg(&ddr_config);
}

/* Load ddr fw header information from given memory address. */
struct ddr_fw_header *load_ddr_fw_header(uintptr_t load_from)
{
	return (struct ddr_fw_header *)(load_from);
}

/* Load ddr fw layout information from given memory address */
struct ddr_fw_layout load_ddr_firmware_layout(uintptr_t load_from)
{
	struct ddr_fw_layout *ddr_fw = (struct ddr_fw_layout *)(load_from);
	return *ddr_fw;
}

/* Load DDR configuration from given address and populate ddrss struct */
static struct ddrss_config load_ddr_config(uintptr_t load_from)
{
	struct ddrss_config config;
	uintptr_t current_addr = load_from;

	ddr_layout = load_ddr_firmware_layout(load_from);
	current_addr += sizeof(struct ddr_fw_layout);

	config.memory_type = ddr_layout.memory_type;
	config.frequency = ddr_layout.frequency;
	config.ddrc_size = ddr_layout.ddrc_size;
	config.dq_swap_size = ddr_layout.dq_swap_size;
	config.phy_size = ddr_layout.phy_size;
	config.pie_size = ddr_layout.pie_size;
	config.imem_1d_size = ddr_layout.imem_1d_size;
	config.dmem_1d_size = ddr_layout.dmem_1d_size;
	config.imem_2d_size = ddr_layout.imem_2d_size;
	config.dmem_2d_size = ddr_layout.dmem_2d_size;
	config.phy_csr_size = ddr_layout.phy_csr_size;
	config.ddrc_csr_size = ddr_layout.ddrc_csr_size;

	config.ddrc = (struct regconf *)(current_addr + ddr_layout.ddrc_offset);
	config.dq_swap = (struct dqconf *)(current_addr + ddr_layout.dq_swap_offset);
	config.phy = (struct regconf_16 *)(current_addr + ddr_layout.phy_offset);
	config.pie = (struct regconf_16 *)(current_addr + ddr_layout.pie_offset);
	config.imem_1d = (uint16_t *)(current_addr + ddr_layout.imem_1d_offset);
	config.dmem_1d = (uint16_t *)(current_addr + ddr_layout.dmem_1d_offset);
	config.imem_2d = (uint16_t *)(current_addr + ddr_layout.imem_2d_offset);
	config.dmem_2d = (uint16_t *)(current_addr + ddr_layout.dmem_2d_offset);
	config.phy_csr = (uint32_t *)(current_addr + ddr_layout.phy_csr_offset);
	config.ddrc_csr = (uint32_t *)(current_addr + ddr_layout.ddrc_csr_offset);

	return config;
}

/* Initialize ddr based on input configuration */
static uint32_t ddr_init_cfg(const struct ddrss_config *config)
{
	uint32_t ret = NO_ERR;

	/* Init DDR controller based on selected parameter values */
	ret = ddrc_init_cfg(config);
	if (ret != NO_ERR)
		return ret;

	/* Setup AXI ports parity */
	ret = set_axi_parity();
	if (ret != NO_ERR)
		return ret;

	/* Init PHY module */
	ret = execute_training(config);
	if (ret != NO_ERR)
		return ret;

	/* Execute post training setup */
	ret = post_train_setup((uint8_t)(STORE_CSR_MASK |
					 INIT_MEM_MASK |
					 ADJUST_DDRC_MASK));

	return ret;
}

/* Initialize ddr controller with given settings. */
static uint32_t ddrc_init_cfg(const struct ddrss_config *config)
{
	uint32_t ret = NO_ERR;

	ret = load_register_cfg(config->ddrc_size, config->ddrc);
	return ret;
}

/* Execute phy training with given settings. 2D training stage is optional. */
static uint32_t execute_training(const struct ddrss_config *config)
{
	uint32_t ret = NO_ERR;

	/* Apply DQ swapping settings */
	ret = load_dq_cfg(config->dq_swap_size, config->dq_swap);
	if (ret != NO_ERR)
		return ret;

	/* Initialize phy module */
	ret = load_register_cfg_16(config->phy_size, config->phy);
	if (ret != NO_ERR)
		return ret;

	/* Configure PLL optimal settings */
	set_optimal_pll(config->frequency);

	/* Load 1D imem image */
	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	ret = load_phy_image(IMEM_START_ADDR, config->imem_1d_size,
			     config->imem_1d);
	if (ret != NO_ERR)
		return ret;
	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

	/* Load 1D dmem image */
	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	ret = load_phy_image(DMEM_START_ADDR, config->dmem_1d_size,
			     config->dmem_1d);
	if (ret != NO_ERR)
		return ret;

	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);
	mmio_write_32(APBONLY_MICRORESET, APBONLY_RESET_STALL_MASK);
	mmio_write_32(APBONLY_MICRORESET, APBONLY_STALL_TO_MICRO_MASK);
	mmio_write_32(APBONLY_MICRORESET, APBONLY_MICRORESET_CLR_MASK);

	ret = wait_firmware_execution();
	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	if (ret != NO_ERR)
		return ret;

	/* Read critical delay differences and training results */
	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	read_cdds();
	if (config->memory_type == (uint8_t)LPDDR4)
		read_vref_ca();
	if (config->memory_type == (uint8_t)DDR3L)
		compute_tphy_wrdata_delay();
	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

	/*
	 * Check if 2d training images have been initialized before executing
	 * the second training stage.
	 */
	if (config->imem_2d_size > 0U && config->dmem_2d_size > 0U) {
		/* Load 2d imem image */
		mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
		ret = load_phy_image(IMEM_START_ADDR, config->imem_2d_size,
				     config->imem_2d);
		if (ret != NO_ERR)
			return ret;
		mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

		/* Load 2d dmem image */
		mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
		ret = load_phy_image(DMEM_START_ADDR, config->dmem_2d_size,
				     config->dmem_2d);
		if (ret != NO_ERR)
			return ret;

		mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);
		mmio_write_32(APBONLY_MICRORESET, APBONLY_RESET_STALL_MASK);
		mmio_write_32(APBONLY_MICRORESET, APBONLY_STALL_TO_MICRO_MASK);
		mmio_write_32(APBONLY_MICRORESET, APBONLY_MICRORESET_CLR_MASK);

		ret = wait_firmware_execution();
		if (ret != NO_ERR)
			return ret;

		/* Read 2D training results */
		if (config->memory_type == (uint8_t)LPDDR4) {
			mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
			read_vref_dq();
			compute_tphy_wrdata_delay();
			mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);
		}
	}

	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	/*  Load pie image after training has executed */
	ret = load_register_cfg_16(config->pie_size, config->pie);
	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);
	return ret;
}

/* Load register array into memory. */
uint32_t load_register_cfg_16(size_t size, const struct regconf_16 cfg[])
{
	size_t i;

	for (i = 0; i < size; i++)
		mmio_write_16((uintptr_t)cfg[i].addr, cfg[i].data);

	return NO_ERR;
}

/* Load register array into memory. */
uint32_t load_register_cfg(size_t size, const struct regconf cfg[])
{
	size_t i;

	for (i = 0; i < size; i++)
		mmio_write_32((uintptr_t)cfg[i].addr, cfg[i].data);

	return NO_ERR;
}

/* Load dq config array into memory. */
uint32_t load_dq_cfg(size_t size, const struct dqconf cfg[])
{
	size_t i;

	for (i = 0; i < size; i++)
		mmio_write_32((uintptr_t)cfg[i].addr, cfg[i].data);

	return NO_ERR;
}

/* Load image into memory at consecutive addresses */
static uint32_t load_phy_image(uint32_t start_addr, size_t size,
			       const uint16_t image[])
{
	uint32_t current_addr = start_addr;
	size_t i;

	for (i = 0; i < size; i++) {
		mmio_write_32((uintptr_t)current_addr, image[i]);
		current_addr += (uint32_t)sizeof(uint32_t);
	}
	return NO_ERR;
}

/* Ensure optimal phy pll settings for selected frequency. */
void set_optimal_pll(uint16_t frequency)
{
	/* Configure phy pll registers */
	mmio_write_32(MASTER_PLLCTRL1, PLLCTRL1_VALUE);
	mmio_write_32(MASTER_PLLTESTMODE, PLLTESTMODE_VALUE);
	mmio_write_32(MASTER_PLLCTRL4, PLLCTRL4_VALUE);
	mmio_write_32(MASTER_PLLCTRL2, PLLCTRL2_VALUE(frequency));

	mmio_setbits_32(MASTER_CALMISC2, (CALMISC2 << CALMISC2_OFFSET));

	mmio_clrsetbits_32(MASTER_CALOFFSET,
			   CALDRV_MASK, ((CALDRV << CALDRV_OFFSET) | (CALDRV << CALDRV2_OFFSET)));
}
