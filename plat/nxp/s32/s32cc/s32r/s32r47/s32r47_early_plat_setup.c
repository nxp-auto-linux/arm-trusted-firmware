/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "platform_def.h"
#include "s32cc_bl_common.h"
#include "s32cc_bl2_el3.h"
#include "s32cc_clocks.h"

static void map_spin_table(void)
{
	mmap_region_t region = {
		.base_pa = S32R47_BL2_SPIN_TABLE_ADDR,
		.base_va = S32R47_BL2_SPIN_TABLE_ADDR,
		.size = PAGE_SIZE,
		.attr = MT_DEVICE | MT_RW | MT_EXECUTE_NEVER | MT_SECURE,
	};
	int ret;

	ret = mmap_add_dynamic_region(region.base_pa, region.base_va,
				      region.size, region.attr);
	if (ret) {
		ERROR("Failed to map SPIN TABLE memory region!\n");
		panic();
	}
}

void s32_early_plat_setup(void)
{
	if (s32_el3_mmu_fixup(NULL, 0))
		panic();

	map_spin_table();
}
