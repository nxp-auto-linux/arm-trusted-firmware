/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <stdint.h>
#include <common/desc_image_load.h>
#include <drivers/arm/pl011.h>
#include <drivers/generic_delay_timer.h>
#include <lib/fconf/fconf.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <lib/libfdt/libfdt.h>
#include <plat/common/platform.h>
#include <s32_bl_common.h>
#include <s32_bl2_common.h>
#include <s32n_bl_common.h>
#include <s32n_io_storage.h>
#include <s32n_fss_scmi.h>

#define S32N_FDT_UPDATES_SPACE		300U

static console_t console;

static struct image_info *s32n_get_image_info(unsigned int image_id)
{
	struct bl_mem_params_node *desc;

	desc = get_bl_mem_params_node(image_id);
	assert(desc);
	return &desc->image_info;
}

int apply_bl2_fixups(void *blob)
{
	size_t size = fdt_totalsize(blob);
	int ret;

	size += S32N_FDT_UPDATES_SPACE;
	fdt_set_totalsize(blob, size);

	ret = ft_fixup_resmem_node(blob);

	flush_dcache_range((uintptr_t)blob, size);
	return ret;
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	if (image_id == BL33_IMAGE_ID)
		return bl2_copy_bl31_dtb();

	return 0;
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

void bl2_el3_plat_arch_setup(void)
{
}

static void plat_s32n_fconf_setup(void)
{
	uintptr_t fdt, fdt_page_addr;
	size_t size;
	int result;

	fdt = get_bl2_dtb_base();
	if (!fdt)
		panic();

	/* Add MMU entry for DTB. */
	fdt_page_addr = round_down(fdt, PAGE_SIZE);
	/* Round down the size since the page where BL_CODE_BASE resides should
	 * already be mapped.
	 */
	size = round_down(BL2_BASE - fdt_page_addr, PAGE_SIZE);
	result = mmap_add_dynamic_region(fdt_page_addr, fdt_page_addr, size,
				      MT_RO_DATA | MT_SECURE);
	if (result)
		panic();

	fconf_populate("HW_CONFIG", fdt);
}

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	int ret;

	generic_delay_timer_init();

	if (is_fss_used())
		fss_scmi_init();

	console_pl011_register(PL011_UART_BASE, PL011_UART_CLOCK_HZ,
		PL011_UART_BAUDRATE, &console);

	NOTICE("BL2: welcome to S32N!\n");

	s32n_el3_mmu_init();

	ret = mmap_add_dynamic_region(BL33_DTB, BL33_DTB,
				      MMU_ROUND_UP_TO_PAGE(BL33_MAX_DTB_SIZE),
				      MT_MEMORY | MT_RW);
	if (ret < 0) {
		ERROR("DTB mapping: error %d\n", ret);
		panic();
	}

	plat_s32n_fconf_setup();

	/* Open handles to a FIP image */
	plat_s32n_io_setup();
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
