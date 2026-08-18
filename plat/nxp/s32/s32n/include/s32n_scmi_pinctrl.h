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

#define S32N_SCMI_PROTOCOL_ID_PINCTRL	U(0x19)

bool s32n_scmi_pinctrl_handle_request(struct scmi_shared_mem *mem,
				      size_t mem_size, uint32_t msg_header);
void s32n_scmi_pinctrl_snoop_response(const struct scmi_shared_mem *mem,
				      size_t mem_size, uint32_t msg_header);

#endif /* S32N_SCMI_PINCTRL_H */
