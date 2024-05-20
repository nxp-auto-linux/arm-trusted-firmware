/*
 * Copyright 2021-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <plat/common/platform.h>
#include <s32cc_pinctrl.h>

#include "s32cc_bl_common.h"
#include "s32cc_clocks.h"
#include "clk/clk.h"

void bl31_platform_setup(void)
{
	generic_delay_timer_init();

	update_core_state(plat_my_core_pos(), CPU_ON, CPU_ON);
	s32_gic_setup();

	s32_a53_clock_setup();
	dt_clk_init();
}

