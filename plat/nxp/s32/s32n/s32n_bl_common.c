/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <drivers/clk.h>
#include <drivers/delay_timer.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <platform_def.h>
#include <s32_bl_common.h>
#include <s32n_bl_common.h>

/* To be provided by FSS */
#define COUNTER_FREQUENCY       50000000 /* 50MHz */
#define USDHC_CLK_FREQUENCY     400000000U /* 400 MHz */

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
	return COUNTER_FREQUENCY;
}

int wait_for_condition_timeout(cond_cb_t cond_cb, void *arg, uint32_t timeout_us)
{
	uint64_t timeout = timeout_init_us(timeout_us);

	if (!cond_cb)
		return -EINVAL;

	do {
		if (cond_cb(arg))
			return 0;
	} while (!timeout_elapsed(timeout));

	return -ETIMEDOUT;
}

unsigned long get_sdhc_clk_freq(void)
{
	return USDHC_CLK_FREQUENCY;
}
