/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2026 NXP
 */
#ifndef S32CC_SCMI_IDS_H
#define S32CC_SCMI_IDS_H

#include <stdint.h>

/*
 * Clock tree ID behind a common SCMI clock: the clock behind a compound clock,
 * otherwise the clock listed in the common SCMI clock table. Private to
 * drivers/nxp/s32/clk.
 */
int cc_scmi_clk_tree_id(uint32_t scmi_clk_id, uint32_t *clk_id);

#endif /* S32CC_SCMI_IDS_H */
