// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2026 NXP
 */
#include <clk/s32cc_clk_funcs.h>
#include <clk/s32cc_scmi_clk.h>
#include <common/debug.h>
#include <dt-bindings/reset/s32cc-scmi-reset.h>
#include <drivers/scmi-msg.h>
#include <stdbool.h>
#include <s32cc_clocks.h>

/*
 * Generic S32CC implementation to deassert only the common DDR reset line.
 */
int s32_reset_ddr_periph(void)
{
	int ret;

	ret = plat_scmi_rstd_set_state(0, S32CC_SCMI_RST_DDR, false);
	if (ret) {
		ERROR("Failed to deassert DDR reset (%d)\n", ret);
		return ret;
	}

	return ret;
}
