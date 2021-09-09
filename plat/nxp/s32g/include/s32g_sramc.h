/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_SRAMC_H
#define S32G_SRAMC_H

#include <lib/utils_def.h>

/* SRAM controller is able to erase 64 bits at once */
#define SRAM_BLOCK		512
#define SRAM_BLOCK_MASK		(SRAM_BLOCK - 1)
#define SRAM_INV_BLOCK_MASK	(~(SRAM_BLOCK_MASK))

#define SRAMC0_BASE_ADDR	0x4019C000
#define SRAMC0_BASE_ADDR_H	(SRAMC0_BASE_ADDR >> 16)
#define SRAMC0_BASE_ADDR_L	((SRAMC0_BASE_ADDR & 0xFFFF))
#define SRAMC1_BASE_ADDR	0x401A0000
#define SRAMC1_BASE_ADDR_H	(SRAMC1_BASE_ADDR >> 16)
#define SRAMC1_BASE_ADDR_L	((SRAMC1_BASE_ADDR & 0xFFFF))
#define SSRAMC_BASE_ADDR	0x44028000
#define SRAMC_SIZE		0x3000

#define SRAMC_PRAMCR_OFFSET	0x0
#define SRAMC_PRAMCR_INITREQ	BIT(0)
#define SRAMC_PRAMIAS_OFFSET	0x4
#define SRAMC_PRAMIAE_OFFSET	0x8
#define SRAMC_PRAMSR_OFFSET	0xC
#define SRAMC_PRAMSR_IDONE	BIT(0)

#define SSRAM_MAX_ADDR		0x7FF

#ifndef __ASSEMBLER__
#include <stdint.h>

int s32g_sram_clear(uintptr_t start, uintptr_t end);
void s32g_ssram_clear(void);
#endif

#endif

