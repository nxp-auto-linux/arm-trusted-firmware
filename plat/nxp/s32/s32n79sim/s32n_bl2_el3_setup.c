/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <stdint.h>
#include <common/desc_image_load.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat/common/platform.h>
#include <drivers/arm/pl011.h>
#include <s32_bl_common.h>
#include <s32n_bl_common.h>
#include <s32n_io_storage.h>

static console_t console;

static struct image_info *s32n_get_image_info(unsigned int image_id)
{
	struct bl_mem_params_node *desc;

	desc = get_bl_mem_params_node(image_id);
	assert(desc);
	return &desc->image_info;
}

int bl2_plat_handle_pre_image_load(unsigned int image_id)
{
	struct image_info *image_info;

	image_info = s32n_get_image_info(image_id);

	return mmap_add_dynamic_region(image_info->image_base,
			image_info->image_base,
			MMU_ROUND_UP_TO_PAGE(image_info->image_max_size),
			MT_MEMORY | MT_RW | MT_SECURE);
}

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	console_pl011_register(PL011_UART_BASE, PL011_UART_CLOCK_HZ,
		PL011_UART_BAUDRATE, &console);

	NOTICE("BL2: welcome to S32N!\n");

	/* Open handles to a FIP image */
	plat_s32n_io_setup();
}

void bl2_el3_plat_arch_setup(void)
{
	int ret;

	s32n_el3_mmu_init();

	ret = mmap_add_dynamic_region(BL33_DTB, BL33_DTB,
				      MMU_ROUND_UP_TO_PAGE(BL33_MAX_DTB_SIZE),
				      MT_MEMORY | MT_RW);
	if (ret < 0) {
		ERROR("DTB mapping: error %d\n", ret);
		panic();
	}
}

void bl2_platform_setup(void)
{
}

struct bl_load_info *plat_get_bl_image_load_info(void)
{
	return get_bl_load_info_from_mem_params_desc();
}

struct bl_params *plat_get_next_bl_params(void)
{
	return get_next_bl_params_from_mem_params_desc();
}

void plat_flush_next_bl_params(void)
{
	flush_bl_params_desc();
}
