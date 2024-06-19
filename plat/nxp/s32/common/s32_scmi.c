/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <s32_scmi.h>

int copy_scmi_msg(uintptr_t to, uintptr_t from, size_t to_size)
{
	size_t copy_len;

	copy_len = get_packet_size(from);
	if (copy_len > to_size)
		return -E2BIG;

	memcpy((void *)to, (const void *)from, copy_len);

	return 0;
}
