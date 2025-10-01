/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_BL2_COMMON_H
#define S32_BL2_COMMON_H

#include <errno.h>
#include <platform_def.h>
#include <common/fdt_fixup.h>
#include <common/debug.h>
#include <s32_bl_common.h>

static inline int ft_fixup_resmem_node(void *blob)
{
	int ret;

	ret = fdt_add_reserved_memory(blob, "atf", BL31_BASE, BL31_SIZE);
	if (ret) {
		ERROR("Failed to add 'atf' /reserved-memory node");
		return ret;
	}

	return 0;
}

int bl2_copy_bl31_dtb(void);
int apply_bl2_fixups(void *blob);

#endif /* S32_BL2_COMMON_H */
