/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32_SCP_UTILS_H
#define S32_SCP_UTILS_H

#include "s32_mc_rgm.h"

int s32_scp_plat_clock_init(void);
int scp_reset_ddr_periph(void);
int scp_disable_ddr_periph(void);
int scp_get_clear_reset_cause(enum reset_cause *cause);
int scp_is_lockstep_enabled(bool *lockstep_en);

#endif /* S32_SCP_UTILS_H */
