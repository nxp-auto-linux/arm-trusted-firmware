/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G2_PLATFORM_DEF_H
#define S32G2_PLATFORM_DEF_H

#include <s32g_platform_def.h>

#define PLATFORM_CORE_COUNT		4
#define PLAT_GICR_BASE			(S32GEN1_GIC_BASE + 0x80000)

/* MPIDR_EL1 for the four A53 cores is as follows:
 *	A53_0_cpu0:	0x8000_0000
 *	A53_0_cpu1:	0x8000_0001
 *	A53_1_cpu0:	0x8000_0100
 *	A53_1_cpu1:	0x8000_0101
 */
#define S32_MPIDR_CPU_MASK		0x1
#define S32_MPIDR_CPU_MASK_BITS	0x1

#define S32_SRAM_SIZE			0x00800000

#if defined IMAGE_BL1
/* To use in blX_platform_setup() */
#define FIRMWARE_WELCOME_STR_S32G	"This is S32G2 BL1\n"
#pragma warning "BL1 image is being built; you should configure it out."
#endif

#if defined IMAGE_BL31
#define FIRMWARE_WELCOME_STR_S32G_BL31	"This is S32G2 BL31\n"
#endif

#endif /* S32G2_PLATFORM_DEF_H */

