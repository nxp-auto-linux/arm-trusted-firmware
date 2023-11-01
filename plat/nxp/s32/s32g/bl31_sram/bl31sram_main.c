/*
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch_helpers.h>
#include <plat/common/platform.h>

#include "bl31_sram.h"
#include "ddr_lp.h"
#include "s32g_clocks.h"
#include "s32g_mc_me.h"
#include "s32_bl_common.h"
#include <lib/bakery_lock.h>
#include <s32_scp_scmi.h>
#include <s32_scp_utils.h>

/**
 * Dummy implementation for lock get and release operations.
 *
 * No concurrent access is expected during platform suspend operation.
 */
void bakery_lock_get(bakery_lock_t *bakery)
{
}

void bakery_lock_release(bakery_lock_t *bakery)
{
}

/* No irq during bl31sram */
void plat_ic_set_interrupt_pending(unsigned int id)
{
}

static void disable_ddr_clk(void)
{
	s32_disable_cofb_clk(S32_MC_ME_USDHC_PART, 0);
	s32g_ddr2firc();
	s32g_disable_pll(S32_DDR_PLL, 1);
}

void bl31sram_main(void)
{
	disable_mmu_el3();

	if (!is_scp_used()) {
		ddrss_to_io_retention_mode();
		disable_ddr_clk();
		s32g_disable_fxosc();

		/* Set standby master core and request the standby transition */
		s32g_set_stby_master_core(S32G_STBY_MASTER_PART,
					  plat_my_core_pos());
	} else {
		scp_scmi_init(false);
		ddrss_to_io_retention_mode();
		scp_disable_ddr_periph();
		scp_suspend_platform();
	}

	core_turn_off();
	plat_panic_handler();
}
