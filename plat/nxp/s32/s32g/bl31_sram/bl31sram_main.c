/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch_helpers.h>
#include <plat/common/platform.h>

#include "bl31_sram.h"
#include "ddr/ddr_lp.h"
#include "s32g_clocks.h"
#include "s32g_mc_me.h"

static void disable_ddr_clk(void)
{
	s32_disable_cofb_clk(S32_MC_ME_USDHC_PART, 0);
	s32g_ddr2firc();
	s32g_disable_pll(S32_DDR_PLL, 1);
}

void bl31sram_main(void)
{
	disable_mmu_el3();
	ddrss_to_io_retention_mode();
	disable_ddr_clk();

	s32g_disable_fxosc();

	/* Set standby master core and request the standby transition */
	s32g_set_stby_master_core(S32G_STBY_MASTER_PART, plat_my_core_pos());

	/*
	 * A torn-apart variant of psci_power_down_wfi()
	 */
	dsb();
	wfi();

	plat_panic_handler();
}
