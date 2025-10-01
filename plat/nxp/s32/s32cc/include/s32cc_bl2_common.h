/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_BL2_COMMON_H
#define S32CC_BL2_COMMON_H

#include <s32_bl2_common.h>

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

/*
 * Computes the maximum size of the BL33 DTB.
 * Adds one extra page for additional data that may
 * be added to the device tree.
 */
static inline size_t get_max_bl33_dtb_size(void)
{
	return MMU_ROUND_UP_TO_PAGE(get_bl2_dtb_size()) + PAGE_SIZE;
}

static inline uintptr_t get_bl33_dtb_base(void)
{
	return BL33_ENTRYPOINT - get_max_bl33_dtb_size();
}

#endif
