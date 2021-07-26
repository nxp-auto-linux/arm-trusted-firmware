/*
 * Copyright 2020-2021 NXP
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

#include <ddr/ddr_init.h>
#include <lib/mmio.h>

static uint32_t ddrc_init_cfg(struct ddrss_config *config);
static uint32_t execute_training(struct ddrss_config *config);
static uint32_t load_phy_image(uint32_t start_addr, size_t size,
			       uint16_t image[]);

/* Main method needed to initialize ddr subsystem. */
uint32_t ddr_init(void)
{
	uint32_t ret = NO_ERR;
	size_t i;

	init_image_sizes();

	for (i = 0; i < ddrss_config_size; i++) {
		/* Init DDR controller based on selected parameter values */
		ret = ddrc_init_cfg(&configs[i]);
		if (ret != NO_ERR)
			return ret;

		/* Setup AXI ports parity */
		ret = set_axi_parity();
		if (ret != NO_ERR)
			return ret;

		/* Init PHY module */
		ret = execute_training(&configs[i]);
		if (ret != NO_ERR)
			return ret;

		/* Execute post training setup */
		ret = post_train_setup(STORE_CSR_MASK | INIT_MEM_MASK);
		if (ret != NO_ERR)
			return ret;
	}
	return ret;
}

/* Initialize ddr controller with given settings. */
static uint32_t ddrc_init_cfg(struct ddrss_config *config)
{
	uint32_t ret = NO_ERR;

	ret = load_register_cfg(config->ddrc_cfg_size, config->ddrc_cfg);
	return ret;
}

/* Execute phy training with given settings. 2D training stage is optional. */
static uint32_t execute_training(struct ddrss_config *config)
{
	uint32_t ret = NO_ERR;
	/* Apply DQ swapping settings */
	ret = load_dq_cfg(config->dq_swap_cfg_size, config->dq_swap_cfg);
	if (ret != NO_ERR)
		return ret;

	/* Initialize phy module */
	ret = load_register_cfg_16(config->phy_cfg_size, config->phy_cfg);
	if (ret != NO_ERR)
		return ret;

	/* Load 1D imem image */
	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	ret = load_phy_image(IMEM_START_ADDR, config->imem_1d_size,
			     config->imem_1d);
	if (ret != NO_ERR)
		return ret;
	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

	/* Load 1D imem image */
	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	ret = load_phy_image(DMEM_START_ADDR, config->dmem_1d_size,
			     config->dmem_1d);
	if (ret != NO_ERR)
		return ret;
	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);

	/* Configure PLL optimal settings */
	set_optimal_pll();

	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);
	mmio_write_32(APBONLY_MICRORESET, 0x00000009);
	mmio_write_32(APBONLY_MICRORESET, 0x00000001);
	mmio_write_32(APBONLY_MICRORESET, 0x00000000);

	ret = wait_firmware_execution();
	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	if (ret != NO_ERR)
		return ret;

	/*
	 * Check if 2d training images have been initialized before executing
	 * the second training stage.
	 */
	if (config->imem_2d_size > 0 && config->dmem_2d_size > 0) {
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

		/* Configure PLL optimal settings */
		set_optimal_pll();

		mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);
		mmio_write_32(APBONLY_MICRORESET, 0x00000009);
		mmio_write_32(APBONLY_MICRORESET, 0x00000001);
		mmio_write_32(APBONLY_MICRORESET, 0x00000000);

		ret = wait_firmware_execution();
		if (ret != NO_ERR)
			return ret;
	}

	mmio_write_32(MICROCONT_MUX_SEL, UNLOCK_CSR_ACCESS);
	/*  Load pie image after training has executed */
	ret = load_register_cfg_16(config->pie_cfg_size, config->pie_cfg);
	mmio_write_32(MICROCONT_MUX_SEL, LOCK_CSR_ACCESS);
	return ret;
}

/* Load register array into memory. */
uint32_t load_register_cfg_16(size_t size, struct regconf_16 cfg[])
{
	size_t i;

	for (i = 0; i < size; i++)
		mmio_write_16((uintptr_t)cfg[i].addr, cfg[i].data);

	return NO_ERR;
}

/* Load register array into memory. */
uint32_t load_register_cfg(size_t size, struct regconf cfg[])
{
	size_t i;

	for (i = 0; i < size; i++)
		mmio_write_32((uintptr_t)cfg[i].addr, cfg[i].data);

	return NO_ERR;
}

/* Load dq config array into memory. */
uint32_t load_dq_cfg(size_t size, struct dqconf cfg[])
{
	size_t i;

	for (i = 0; i < size; i++)
		mmio_write_32((uintptr_t)cfg[i].addr, cfg[i].data);

	return NO_ERR;
}

/* Load image into memory at consecutive addresses */
static uint32_t load_phy_image(uint32_t start_addr, size_t size,
			       uint16_t image[])
{
	size_t i;

	for (i = 0; i < size; i++) {
		mmio_write_32((uintptr_t)start_addr, image[i]);
		start_addr += sizeof(uint32_t);
	}
	return NO_ERR;
}

/* Ensure optimal phy pll settings. */
void set_optimal_pll(void)
{
	/* Configure phy pll for 3200MTS data rate */
	mmio_write_32(MASTER_PLLCTRL1, 0x00000021);
	mmio_write_32(MASTER_PLLTESTMODE, 0x00000024);
	mmio_write_32(MASTER_PLLCTRL4, 0x0000017f);
	mmio_write_32(MASTER_PLLCTRL2, 0x00000019);
}
