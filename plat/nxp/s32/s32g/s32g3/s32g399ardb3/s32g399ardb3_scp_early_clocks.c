// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#include <s32_bl_common.h>
#include <s32_clocks.h>

#include <dt-bindings/clock/s32gen1-scmi-clock.h>

int scp_enable_board_early_clocks(void)
{
	return scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_SAR_ADC_BUS);
}
