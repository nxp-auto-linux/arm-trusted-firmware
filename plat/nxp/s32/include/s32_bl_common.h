/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_BL_COMMON_H
#define S32_BL_COMMON_H

#include <platform_def.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <common/debug.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })

#define UPTR(PTR)			((uintptr_t)(PTR))

#define PAGE_MASK		(PAGE_SIZE_4KB - 1)
#define MMU_ROUND_UP_TO_PAGE(x)	round_up((x), PAGE_SIZE)

static inline bool is_mmu_el3_enabled(void)
{
	return (read_sctlr_el3() & SCTLR_M_BIT) != 0U;
}

static inline int s32_mmap_dynamic_region(unsigned long long base_pa, size_t size,
					  unsigned int attr, bool *is_added)
{
	int ret;

	if (is_mmu_el3_enabled() && !(*is_added)) {
		ret = mmap_add_dynamic_region(base_pa, base_pa, size, attr);
		if (ret) {
			ERROR("Failed to map region base_pa: 0x%llx error: %d\n",
			      base_pa, ret);
			return ret;
		}
		*is_added = true;
	}
	return 0;
}

unsigned long get_sdhc_clk_freq(void);

#endif /* S32_BL_COMMON_H */
