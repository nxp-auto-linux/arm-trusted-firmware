/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <plat/common/platform.h>

#include "s32_bl_common.h"
#include "s32_clocks.h"
#include "clk/clk.h"

void bl31_platform_setup(void)
{
	generic_delay_timer_init();

	update_core_state(plat_my_core_pos(), 1);
	s32_gic_setup();

	s32_enable_a53_clock();
	dt_clk_init();
}

