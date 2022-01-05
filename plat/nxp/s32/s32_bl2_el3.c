/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>

#include <common/bl_common.h>
#include <common/desc_image_load.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <lib/libc/errno.h>
#include <lib/libfdt/libfdt.h>
#include <platform.h>

#include "s32_dt.h"
#include "s32_clocks.h"
#include "s32_mc_me.h"
#include "s32_mc_rgm.h"
#include "s32_sramc.h"
#include "s32_storage.h"

void add_fip_img_to_mem_params_descs(bl_mem_params_node_t *params,
					    size_t *index)
{
	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = FIP_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      NON_SECURE | EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_PLAT_SETUP),
		.image_info.image_max_size = FIP_MAXIMUM_SIZE,
		.image_info.image_size = FIP_HEADER_SIZE,
		.image_info.image_base = FIP_BASE,
		.next_handoff_image_id = BL31_IMAGE_ID,
	};
}

void add_bl31_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{
	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = BL31_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | EXECUTABLE | EP_FIRST_EXE),
		.ep_info.spsr = SPSR_64(MODE_EL3, MODE_SP_ELX,
					DISABLE_ALL_EXCEPTIONS),
		.ep_info.pc = BL31_BASE,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_max_size = BL31_LIMIT - BL31_BASE,
		.image_info.image_base = BL31_BASE,
#ifdef SPD_opteed
		.next_handoff_image_id = BL32_IMAGE_ID,
#else
		.next_handoff_image_id = BL33_IMAGE_ID,
#endif
	};
}

#ifdef SPD_opteed
void add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{
	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = BL32_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | EXECUTABLE),
		.ep_info.pc = S32_BL32_BASE,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_max_size = S32_BL32_SIZE,
		.image_info.image_base = S32_BL32_BASE,
		.next_handoff_image_id = BL33_IMAGE_ID,
	};
}

void add_bl32_extra1_img_to_mem_params_descs(
	bl_mem_params_node_t *params,
	size_t *index)
{
	params[(*index)++] = (bl_mem_params_node_t) {

		.image_id = BL32_EXTRA1_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | NON_EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_SKIP_LOADING),
		.image_info.image_base = S32_BL32_BASE,
		.image_info.image_max_size = S32_BL32_SIZE,

		.next_handoff_image_id = INVALID_IMAGE_ID,
	};
}

#else
void add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{

}

void add_bl32_extra1_img_to_mem_params_descs(
	bl_mem_params_node_t *params,
	size_t *index)
{

}
#endif /* SPD_opteed */

void add_bl33_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{
	bl_mem_params_node_t node = {
		.image_id = BL33_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      NON_SECURE | EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_max_size = S32_BL33_IMAGE_SIZE,
		.image_info.image_base = S32_BL33_IMAGE_BASE,
		.next_handoff_image_id = INVALID_IMAGE_ID,
	};

	params[(*index)++] = node;
}

void add_invalid_img_to_mem_params_descs(bl_mem_params_node_t *params,
						size_t *index)
{
	bl_mem_params_node_t node = {
		.image_id = INVALID_IMAGE_ID,
		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_SKIP_LOADING),
	};

	params[(*index)++] = node;
}

#define MMU_ROUND_UP_TO_4K(x)	\
			(((x) & ~0xfff) == (x) ? (x) : ((x) & ~0xfff) + 0x1000)

IMPORT_SYM(uintptr_t, __RW_START__, BL2_RW_START);

static mmap_region_t s32_mmap[] = {
#if !defined(PLAT_s32r)
	MAP_REGION_FLAT(S32G_SSRAM_BASE, S32G_SSRAM_LIMIT - S32G_SSRAM_BASE,
			 MT_MEMORY | MT_RW | MT_SECURE),
#endif
	MAP_REGION_FLAT(S32_UART_BASE, S32_UART_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(S32_MC_ME_BASE_ADDR, S32_MC_ME_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(MC_CGM0_BASE_ADDR,
			MMU_ROUND_UP_TO_4K(S32_DFS_ADDR(S32_DFS_NR)),
			MT_DEVICE | MT_RW),
	/* This will cover both the MC_RGM and the GPR accesses, while reducing
	 * the number of used up MMU regions.
	 */
	MAP_REGION_FLAT(S32_MC_RGM_BASE_ADDR, S32_MC_RGM_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SRAMC0_BASE_ADDR, SRAMC_SIZE,
			MT_DEVICE | MT_RW),
#if !defined(PLAT_s32r)
	MAP_REGION_FLAT(SSRAMC_BASE_ADDR, SRAMC_SIZE,
			MT_DEVICE | MT_RW),
#endif
	MAP_REGION2(S32_BL32_BASE, S32_BL32_BASE,
			MMU_ROUND_UP_TO_4K(S32_BL32_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION2(S32_BL33_IMAGE_BASE, S32_BL33_IMAGE_BASE,
			MMU_ROUND_UP_TO_4K(S32_BL33_IMAGE_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION_FLAT(S32_PMEM_START, S32_PMEM_LEN,
			MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32_SCMI_SHARED_MEM, S32_SCMI_SHARED_MEM_SIZE,
			MT_NON_CACHEABLE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32_QSPI_BASE, S32_QSPI_SIZE, MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(FIP_BASE, FIP_MAXIMUM_SIZE, MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32_FLASH_BASE, FIP_MAXIMUM_SIZE, MT_RW | MT_SECURE),
	MAP_REGION_FLAT(DTB_BASE, BL2_BASE - DTB_BASE, MT_MEMORY | MT_RW),
	{0},
};

static int disable_qspi_mmu_entry(void)
{
	int offset;
	void *fdt = NULL;
	size_t i;

	if (dt_open_and_check() < 0) {
		ERROR("Failed to check FDT integrity\n");
		return -EFAULT;
	}

	if (fdt_get_address(&fdt) == 0) {
		ERROR("Failed to get FDT address\n");
		return -EFAULT;
	}

	offset = fdt_node_offset_by_compatible(fdt, -1, "fsl,s32gen1-qspi");
	if (offset > 0) {
		if (fdt_get_status(offset) == DT_ENABLED)
			return 0;
	}

	for (i = 0; i < ARRAY_SIZE(s32_mmap); i++) {
		if (s32_mmap[i].base_pa == S32_FLASH_BASE) {
			s32_mmap[i].size = 0;
			break;
		}
	}

	return 0;
}

int s32_el3_mmu_fixup(void)
{
	const unsigned long code_start = BL_CODE_BASE;
	const unsigned long code_size = BL_CODE_END - BL_CODE_BASE;
	const unsigned long rw_start = BL2_RW_START;
	const unsigned long rw_size = BL_END - BL2_RW_START;
	mmap_region_t regions[] = {
		{
			.base_pa = code_start,
			.base_va = code_start,
			.size = code_size,
			.attr = MT_CODE | MT_SECURE,
		},
		{
			.base_pa = rw_start,
			.base_va = rw_start,
			.size = rw_size,
			.attr = MT_RW | MT_MEMORY | MT_SECURE,
		},
	};
	int i, ret;

	ret = disable_qspi_mmu_entry();
	if (ret)
		return ret;

	/* Check the BL31/BL32/BL33 memory ranges for overlapping */
	_Static_assert(S32_BL32_BASE + S32_BL32_SIZE <= BL31_BASE,
				"BL32 and BL31 memory ranges overlap!");
	_Static_assert(BL31_BASE + BL31_SIZE <= BL33_BASE,
				"BL31 and BL33 memory ranges overlap!");

	/* The calls to mmap_add_region() consume mmap regions,
	 * so they must be counted in the static asserts
	 */
	_Static_assert(ARRAY_SIZE(s32_mmap) + ARRAY_SIZE(regions) - 1 <=
		MAX_MMAP_REGIONS,
		"Fewer MAX_MMAP_REGIONS than in s32_mmap will likely result in a MMU exception at runtime");

	_Static_assert(ARRAY_SIZE(s32_mmap) + ARRAY_SIZE(regions) - 1
#if !defined(PLAT_s32r)
		+ BL31SRAM_MAX_PAGES
#endif
		<= MAX_XLAT_TABLES,
		"Fewer MAX_XLAT_TABLES than in s32_mmap will likely result in a MMU exception at runtime");

	/* MMU initialization; while technically not necessary, improves
	 * bl2_load_images execution time.
	 */
	for (i = 0; i < ARRAY_SIZE(regions); i++)
		mmap_add_region(regions[i].base_pa, regions[i].base_va,
				regions[i].size, regions[i].attr);

	mmap_add(s32_mmap);

	init_xlat_tables();
	enable_mmu_el3(0);

	return 0;
}

struct bl_load_info *plat_get_bl_image_load_info(void)
{
	return get_bl_load_info_from_mem_params_desc();
}

void bl2_platform_setup(void)
{

}

struct bl_params *plat_get_next_bl_params(void)
{
	return get_next_bl_params_from_mem_params_desc();
}

void plat_flush_next_bl_params(void)
{
	flush_bl_params_desc();
}

