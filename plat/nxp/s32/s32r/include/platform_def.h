/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32R_PLATFORM_DEF_H
#define S32R_PLATFORM_DEF_H

#include <s32_platform_def.h>

#define PLATFORM_CORE_COUNT		4
#define PLAT_GICR_BASE			(S32GEN1_GIC_BASE + 0x80000)

#define S32_MPIDR_CPU_MASK		0x1
#define S32_MPIDR_CPU_MASK_BITS	0x1

/* FIXME - might be common for G&R */
#define MAX_MMAP_REGIONS		8
#define MAX_XLAT_TABLES			4

#define S32_SRAM_SIZE		0x00800000

#if defined IMAGE_BL1
/* To use in blX_platform_setup() */
#define FIRMWARE_WELCOME_STR_S32R	"This is S32R BL1\n"
#pragma warning "BL1 image is being built; you should configure it out."
#endif

#if defined IMAGE_BL31
#define FIRMWARE_WELCOME_STR_S32R_BL31	"This is S32R BL31\n"
#endif

/* Off-Chasis */
#define SIUL2_1_BASE_ADDR	0x4403C000UL

#endif /* S32_PLATFORM_H */
