/*
 * Copyright 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G3_PLATFORM_DEF_H
#define S32G3_PLATFORM_DEF_H

#include <s32g_platform_def.h>

#define PLATFORM_CORE_COUNT		8
#define PLATFORM_MAX_CPUS_PER_CLUSTER	4
#define PLATFORM_M7_CORE_COUNT		4
#define PLAT_GICR_BASE			(S32GEN1_GIC_BASE + 0x100000)
#define S32GEN1_GIC_SIZE		(0x200000)

#define MC_CGM2_BASE_ADDR		(0x44018000ul)
#define MC_CGM2_SIZE			(0x580)

#define MC_CGM6_BASE_ADDR		(0x4053c000ul)
#define MC_CGM6_SIZE			(0x400)

#define SRAMC2_BASE_ADDR		(0x4055a000ul)
#define SRAMC3_BASE_ADDR		(0x4055e000ul)

/* MPIDR_EL1 for the eight A53 cores is as follows:
 *	A53_0_cpu0:	0x8000_0000
 *	A53_0_cpu1:	0x8000_0001
 *	A53_0_cpu2:	0x8000_0002
 *	A53_0_cpu3:	0x8000_0003
 *	A53_1_cpu0:	0x8000_0100
 *	A53_1_cpu1:	0x8000_0101
 *	A53_1_cpu2:	0x8000_0102
 *	A53_1_cpu3:	0x8000_0103
 */
#define S32_MPIDR_CPU_MASK_BITS		0x2

#define S32_SRAM_SIZE			0x1400000

/* MSCM settings */
#define MSCM_IRPC_OFFSET	(0xa60u)
#define MSCM_CPN_SIZE		(0x70u)

#if defined IMAGE_BL1
/* To use in blX_platform_setup() */
#define FIRMWARE_WELCOME_STR_S32G	"This is S32G3 BL1\n"
#pragma warning "BL1 image is being built; you should configure it out."
#endif

#if defined IMAGE_BL31
#define FIRMWARE_WELCOME_STR_S32G_BL31	"This is S32G3 BL31\n"
#endif

#endif /* S32G3_PLATFORM_DEF_H */

