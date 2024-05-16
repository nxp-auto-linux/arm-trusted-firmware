/*
 * Copyright 2020-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_SRAMC_H
#define S32CC_SRAMC_H

#include <lib/utils_def.h>

#define SRAMC0_BASE_ADDR        0x4019C000
#define SRAMC1_BASE_ADDR        0x401A0000

#define SRAMC_SIZE              0x14

#ifndef __ASSEMBLER__
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct sram_ctrl {
	uintptr_t base_addr;
	uint32_t min_sram_addr;
	uint32_t max_sram_addr;
	bool mmap_added;
	/**
	 * Translate an A53 SRAM address to SRAM controller offset
	 * associated to that memory region.
	 * This algorithm is platform specific.
	 */
	uintptr_t (*a53_to_sramc_offset)(uintptr_t addr);
};

int s32_sram_clear(uintptr_t start, uintptr_t end);
void s32_ssram_clear(void);
void s32_get_sramc(struct sram_ctrl **ctrls, size_t *size);

#endif
#endif

