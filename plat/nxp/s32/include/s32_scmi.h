/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_SCMI_H
#define S32_SCMI_H

#include <stdint.h>
#include <arm/css/scmi/scmi_private.h>

static inline size_t get_packet_size(uintptr_t scmi_packet)
{
	mailbox_mem_t *mbx_mem = (mailbox_mem_t *)scmi_packet;

	return offsetof(mailbox_mem_t, msg_header) + mbx_mem->len;
}

int copy_scmi_msg(uintptr_t to, uintptr_t from, size_t to_size);

#endif /* S32_SCMI_H */
