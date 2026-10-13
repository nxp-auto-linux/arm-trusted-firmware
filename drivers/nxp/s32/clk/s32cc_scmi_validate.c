// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2026 NXP
 */
#include <clk/s32cc_clk_funcs.h>
#include <clk/s32cc_scmi_clk.h>
#include <common/debug.h>
#include <dt-bindings/clock/s32cc-scmi-clock.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>

#include "s32cc_scmi_ids.h"

/*
 * Consistency check of the common S32CC SCMI clock table. The table is
 * static, so the caller only runs the check when S32CC_CLOCK_DEBUG is set.
 */

/*
 * Check that every named common SCMI clock is backed by a clock of the SoC's
 * clock tree.
 */
int cc_scmi_clk_validate(void)
{
	uint32_t scmi_clk_id;
	const char *name;
	uint32_t id;

	for (scmi_clk_id = S32CC_SCMI_CLK(0U);
	     scmi_clk_id < S32CC_SCMI_PLAT_CLK_BASE_ID; scmi_clk_id++) {
		/* Unused IDs have no name */
		name = cc_scmi_clk_get_name(scmi_clk_id);
		if (!name)
			continue;

		if (cc_scmi_clk_tree_id(scmi_clk_id, &id))
			return -EINVAL;

		if (id == S32CC_SCMI_NOT_IMPLEMENTED_CLK)
			continue;

		if (!get_clock(id)) {
			ERROR("SCMI clock %s: clock %" PRIu32 " is not part of the clock tree\n",
			      name, id);
			return -EINVAL;
		}
	}

	return 0;
}
