/*
 * Copyright 2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_SCMI_H
#define S32_SCMI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <arm/css/scmi/scmi_private.h>
#include <s32_svc.h>

static inline size_t get_packet_size(uintptr_t scmi_packet)
{
	mailbox_mem_t *mbx_mem = (mailbox_mem_t *)scmi_packet;

	return offsetof(mailbox_mem_t, msg_header) + mbx_mem->len;
}

static inline bool scmi_msg_has_payload(const struct scmi_shared_mem *mem,
					size_t payload_size)
{
	return mem->length >= sizeof(mem->msg_header) + payload_size;
}

static inline bool scmi_channel_fits_payload(size_t mem_size,
					     size_t payload_size)
{
	return mem_size >= sizeof(struct scmi_shared_mem) + payload_size;
}

int copy_scmi_msg(uintptr_t to, uintptr_t from, size_t to_size);

#endif /* S32_SCMI_H */
