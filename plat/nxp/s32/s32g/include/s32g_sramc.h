/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_SRAMC_H
#define S32G_SRAMC_H

#include <lib/utils_def.h>

#define S32G_SRAM_BASE          0x34000000
#define S32G_SRAM_SIZE          0x00800000
#define S32G_SRAM_END           (S32G_SRAM_BASE + S32G_SRAM_SIZE)

/* SRAM controller is able to erase 64 bits at once */
#define SRAM_BLOCK              512
#define SRAM_BLOCK_MASK         (SRAM_BLOCK - 1)
#define SRAM_INV_BLOCK_MASK     (~(SRAM_BLOCK_MASK))

#define SRAMC0_BASE_ADDR        0x4019C000
#define SRAMC0_BASE_ADDR_H      (SRAMC0_BASE_ADDR >> 16)
#define SRAMC0_BASE_ADDR_L      ((SRAMC0_BASE_ADDR & 0xFFFF))
#define SRAMC1_BASE_ADDR        0x401A0000
#define SRAMC1_BASE_ADDR_H      (SRAMC1_BASE_ADDR >> 16)
#define SRAMC1_BASE_ADDR_L      ((SRAMC1_BASE_ADDR & 0xFFFF))
#if defined(PLAT_s32g3)
#define SRAMC2_BASE_ADDR        0x4055A000
#define SRAMC2_BASE_ADDR_H      (SRAMC0_BASE_ADDR >> 16)
#define SRAMC2_BASE_ADDR_L      ((SRAMC0_BASE_ADDR & 0xFFFF))
#define SRAMC3_BASE_ADDR        0x4055E000
#define SRAMC3_BASE_ADDR_H      (SRAMC1_BASE_ADDR >> 16)
#define SRAMC3_BASE_ADDR_L      ((SRAMC1_BASE_ADDR & 0xFFFF))
#endif
#define SSRAMC_BASE_ADDR        0x44028000
#define SRAMC_SIZE              0x3000

/* Block ranges */

#if defined(PLAT_s32g2)
#define SRAMC0_MIN_ADDR         (0x0)
#define SRAMC0_MAX_ADDR         (0x7FFF)
#define SRAMC1_MIN_ADDR         (SRAMC0_MAX_ADDR + 1)
#define SRAMC1_MAX_ADDR         (0x10000)

#elif defined(PLAT_s32g3)
#define SRAMC0_MIN_ADDR         (0x0)
#define SRAMC0_MAX_ADDR         (0x7FFF)
#define SRAMC1_MIN_ADDR         (SRAMC0_MAX_ADDR + 1)
#define SRAMC1_MAX_ADDR         (0xFFFF)
#define SRAMC2_MIN_ADDR         (SRAMC1_MAX_ADDR + 1)
#define SRAMC2_MAX_ADDR         (0x17FFF)
#define SRAMC2_MAX_ADDR_H       (SRAMC2_MAX_ADDR >> 16)
#define SRAMC2_MAX_ADDR_L       (SRAMC2_MAX_ADDR & 0xFFFF)
#define SRAMC3_MIN_ADDR         (SRAMC2_MAX_ADDR + 1)
#define SRAMC3_MIN_ADDR_H       (SRAMC3_MIN_ADDR >> 16)
#define SRAMC3_MIN_ADDR_L       (SRAMC3_MIN_ADDR & 0xFFFF)
#define SRAMC3_MAX_ADDR         (0x1FFFF)
#define SRAMC3_MAX_ADDR_H       (SRAMC3_MAX_ADDR >> 16)
#define SRAMC3_MAX_ADDR_L       (SRAMC3_MAX_ADDR & 0xFFFF)
#endif

#define SRAMC_PRAMCR_OFFSET     0x0
#define SRAMC_PRAMCR_INITREQ    1
#define SRAMC_PRAMIAS_OFFSET    0x4
#define SRAMC_PRAMIAE_OFFSET    0x8
#define SRAMC_PRAMSR_OFFSET     0xC
#define SRAMC_PRAMSR_IDONE      1

#define SSRAM_MAX_ADDR          0x7FF

#ifndef __ASSEMBLER__
#include <stdint.h>

int s32g_sram_clear(uintptr_t start, uintptr_t end);
void s32g_ssram_clear(void);
#endif
#endif

