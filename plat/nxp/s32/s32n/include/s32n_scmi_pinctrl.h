/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_SCMI_PINCTRL_H
#define S32N_SCMI_PINCTRL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/utils_def.h>
#include <s32_svc.h>
#include <s32n_scmi_local.h>

#define S32N_SCMI_PROTOCOL_ID_PINCTRL	U(0x19)

/* Per-message ops table for the pinctrl protocol, consumed by the dispatcher */
extern const struct s32n_scmi_msg_table s32n_scmi_pinctrl_msg_table;

#endif /* S32N_SCMI_PINCTRL_H */
