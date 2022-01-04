/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <lib/mmio.h>
#include "platform_def.h"
#include "s32_clocks.h"
#include "s32_ncore.h"
#include "s32_pinctrl.h"

bool is_lockstep_enabled(void)
{
	if (mmio_read_32(GPR_BASE_ADDR + GPR06_OFF) & CA53_LOCKSTEP_EN)
		return true;

	return false;
}

void s32_early_plat_init(bool skip_ddr_clk)
{
	uint32_t caiutc;

	s32_plat_config_pinctrl();
	s32_plat_clock_init(skip_ddr_clk);

	/* Restore (clear) the CAIUTC[IsolEn] bit for the primay cluster, which
	 * we have manually set during early BL2 boot.
	 */
	caiutc = mmio_read_32(S32_NCORE_CAIU0_BASE_ADDR + NCORE_CAIUTC_OFF);
	caiutc &= ~NCORE_CAIUTC_ISOLEN_MASK;
	mmio_write_32(S32_NCORE_CAIU0_BASE_ADDR + NCORE_CAIUTC_OFF, caiutc);

	ncore_init();
	ncore_caiu_online(A53_CLUSTER0_CAIU);

	generic_delay_timer_init();
}

void plat_ea_handler(unsigned int ea_reason, uint64_t syndrome, void *cookie,
		void *handle, uint64_t flags)
{
	ERROR("Unhandled External Abort received on 0x%lx at EL3!\n",
	      read_mpidr_el1());
	ERROR(" exception reason=%u syndrome=0x%llx\n", ea_reason, syndrome);

	panic();
}

unsigned int plat_get_syscnt_freq2(void)
{
	return COUNTER_FREQUENCY;
}

