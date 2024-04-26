/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32R_PLATFORM_DEF_H
#define S32R_PLATFORM_DEF_H

#include <s32cc_platform_def.h>

#define PLATFORM_CORE_COUNT		4
#define PLATFORM_MAX_CPUS_PER_CLUSTER	2
#define PLATFORM_M7_CORE_COUNT		3
#define PLAT_GICR_BASE			(S32GEN1_GIC_BASE + 0x80000)
#define S32GEN1_GIC_SIZE		(0x100000)

#define MC_CGM2_BASE_ADDR		(0x440C0000ul)
#define MC_CGM2_SIZE			(0x408)

#define S32_MPIDR_CPU_MASK_BITS		0x1

#define MAX_MMAP_REGIONS		42
#ifndef MAX_XLAT_TABLES
#define MAX_XLAT_TABLES			27
#endif

#if defined IMAGE_BL33
#pragma warning "BL33 image is being built; you should configure it out."
#endif

#define S32_SRAM_SIZE		0x00800000

/* MSCM settings */
#define MSCM_IRPC_OFFSET	(0x200u)
#define MSCM_CPN_SIZE		(0x20u)

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
#define SIUL2_1_BASE_ADDR	(0x4403C000UL)
#define SIUL2_1_SIZE		(0x17A4)

#define SIUL2_1_MSCR_START	102
#define SIUL2_1_MSCR_END	133
#define SIUL2_1_IMCR_START	603
#define SIUL2_1_IMCR_END	785

#endif /* S32_PLATFORM_H */
