/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_BL_COMMON_H
#define S32N_BL_COMMON_H

#include <platform_def.h>
#include <stdbool.h>
#include <stdint.h>
#include <s32_bl2_common.h>

typedef enum {
	BOOT_SOURCE_UFS = 0,
	BOOT_SOURCE_MMC = 1,
} s32n_boot_source;

void s32n_el3_mmu_init(void);

static inline uintptr_t get_bl33_dtb_size(void)
{
	return BL33_DTB_SIZE;
}

/*
 * Computes the maximum size of the BL33 DTB.
 * Adds one extra page for additional data that may
 * be added to the device tree.
 */
static inline size_t get_max_bl33_dtb_size(void)
{
	return MMU_ROUND_UP_TO_PAGE(get_bl33_dtb_size()) + PAGE_SIZE;
}

static inline uintptr_t get_bl33_dtb_base(void)
{
	return BL33_ENTRYPOINT - get_max_bl33_dtb_size();
}

static inline int boot_source_get(void)
{
	if (S32N_UFS_BOOT_SOURCE)
		return BOOT_SOURCE_UFS;
	if (S32N_MMC_BOOT_SOURCE)
		return BOOT_SOURCE_MMC;
	return -EINVAL;
}

static inline uintptr_t get_s32n_io_buffer_base(void)
{
	return (get_bl33_dtb_base() - S32N_IO_BUFFER_SIZE - PAGE_SIZE);
}

/* callback to check a condition */
typedef bool (*cond_cb_t)(void *arg);

/**
 * Returns -EINVAL if the condition callback is not set, 0 if the condition is met,
 * -ETIMEDOUT if the timer expired.
 */
int wait_for_condition_timeout(cond_cb_t cond_cb, void *arg, uint32_t timeout_us);

unsigned long get_pl011_clk_freq(void);

uint64_t mru_interrupt_handler(uint32_t id,
			       uint32_t flags, void *handle, void *cookie);

#endif
