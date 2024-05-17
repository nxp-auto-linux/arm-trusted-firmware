/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_BL2_COMMON_H
#define S32_BL2_COMMON_H

#include <errno.h>
#include <platform_def.h>
#include <common/fdt_fixup.h>
#include <common/debug.h>

/* From generated file */
extern const unsigned int dtb_size;

static inline unsigned int get_bl2_dtb_size(void)
{
	if (!dtb_size)
		panic();

	return dtb_size;
}

static inline uintptr_t get_bl2_dtb_base(void)
{
	return BL2_BASE - get_bl2_dtb_size();
}

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
