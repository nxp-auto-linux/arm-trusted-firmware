/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <s32g_platform_def.h>

#define PLATFORM_CORE_COUNT		4
#define PLAT_GICR_BASE			(S32G274A_GIC_BASE + 0x80000)

/* MPIDR_EL1 for the four A53 cores is as follows:
 *	A53_0_cpu0:	0x8000_0000
 *	A53_0_cpu1:	0x8000_0001
 *	A53_1_cpu0:	0x8000_0100
 *	A53_1_cpu1:	0x8000_0101
 */
#define S32G_MPIDR_CPU_MASK		0x1
#define S32G_MPIDR_CPU_MASK_BITS	0x1

#define S32G_SRAM_SIZE			0x00800000

#endif /* PLATFORM_DEF_H */

