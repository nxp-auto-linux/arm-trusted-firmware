/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_SRAMC_H
#define S32G_SRAMC_H

#include <lib/utils_def.h>

#define SRAMC0_BASE_ADDR        0x4019C000
#define SRAMC1_BASE_ADDR        0x401A0000
#define SRAMC_SIZE              0x3000

#define SSRAMC_BASE_ADDR        0x44028000

#ifndef __ASSEMBLER__
#include <stddef.h>
#include <stdint.h>

struct sram_ctrl {
	uintptr_t base_addr;
	uint32_t min_addr;
	uint32_t max_addr;
};

int s32_sram_clear(uintptr_t start, uintptr_t end);
void s32_ssram_clear(void);
void s32_get_sramc(struct sram_ctrl **ctrls, size_t *size);
uintptr_t a53_to_sramc_addr(uintptr_t addr);
#endif
#endif

