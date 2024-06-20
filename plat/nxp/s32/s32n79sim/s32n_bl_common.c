/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/xlat_tables/xlat_tables_v2.h>
#include <platform_def.h>

/* To be provided by FSS */
#if (S32N_SIM == 1)
#define COUNTER_FREQUENCY       100000000 /* 100MHz */
#endif

void s32n_el3_mmu_init(void)
{
	mmap_region_t plat_regions[] = {
		MAP_REGION_FLAT(PL011_UART_BASE, PL011_UART_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),
		{0},
	};
	mmap_region_t bl_regions[] = {
		MAP_REGION_FLAT(BL_CODE_BASE,
				BL_CODE_END - BL_CODE_BASE,
				MT_CODE | MT_SECURE),
		MAP_REGION_FLAT(BL_CODE_END,
				BL_END - BL_CODE_END,
				MT_RW | MT_MEMORY | MT_SECURE),
		{0},
	};

	setup_page_tables(bl_regions, plat_regions);

	enable_mmu_el3(0);
}

unsigned int plat_get_syscnt_freq2(void)
{
#if (S32N_SIM == 1)
	return COUNTER_FREQUENCY;
#else
	return 0;
#endif
}
