/*
 * Copyright 2021-2022 NXP
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

#if defined IMAGE_BL31
/* To limit usage, keep these in sync with sizeof(s32_mmap) */
#define MAX_MMAP_REGIONS		13
#define MAX_XLAT_TABLES			13
#endif

#if defined IMAGE_BL2
#if (ERRATA_S32_050543 == 1)
#define MAX_MMAP_REGIONS		16
#define MAX_XLAT_TABLES			25
#else
#define MAX_MMAP_REGIONS		15
#define MAX_XLAT_TABLES			24
#endif
#endif /* IMAGE_BL2 */

#if defined IMAGE_BL33
#pragma warning "BL33 image is being built; you should configure it out."
#endif

#define S32_SRAM_SIZE		0x00800000

#if defined IMAGE_BL1
/* To use in blX_platform_setup() */
#define FIRMWARE_WELCOME_STR_S32R	"This is S32R BL1\n"
#pragma warning "BL1 image is being built; you should configure it out."
#endif

#if defined IMAGE_BL31
#define FIRMWARE_WELCOME_STR_S32R_BL31	"This is S32R BL31\n"
#endif

#define S32_MAX_I2C_MODULES 3

/* Off-Chasis */
#define SIUL2_1_BASE_ADDR	0x4403C000UL

#endif /* S32_PLATFORM_H */
