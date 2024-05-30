/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32CC_SCP_UTILS_H
#define S32CC_SCP_UTILS_H

#include "s32cc_mc_rgm.h"

int scp_a53_clock_setup(void);
int scp_periph_clock_init(void);
int scp_reset_ddr_periph(void);
int scp_disable_ddr_periph(void);
int scp_get_clear_reset_cause(enum reset_cause *cause);
int scp_is_lockstep_enabled(bool *lockstep_en);
int scp_ddrss_gpr_to_io_retention_mode(void);

#endif /* S32CC_SCP_UTILS_H */
