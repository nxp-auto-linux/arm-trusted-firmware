/*
 * Copyright 2024-2026 NXP
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
 * When Linux Kernel Image is used as BL33, use
 * Linux DTB instead of BL2 DTB.
 * Add one extra page for additional data that
 * may be added to the device tree.
 */
static inline size_t get_max_bl33_dtb_size(void)
{
#if (S32_LINUX_KERNEL_AS_BL33 == 1)
	return MMU_ROUND_UP_TO_PAGE(BL33_DTB_SIZE) + PAGE_SIZE;
#else
	return MMU_ROUND_UP_TO_PAGE(get_bl2_dtb_size()) + PAGE_SIZE;
#endif
}

static inline uintptr_t get_bl33_dtb_base(void)
{
	return BL33_ENTRYPOINT - get_max_bl33_dtb_size();
}

#endif
