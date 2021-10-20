/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <s32g_platform_def.h>

#define PLATFORM_CORE_COUNT		8

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
#define S32G_MPIDR_CPU_MASK		0x3
#define S32G_MPIDR_CPU_MASK_BITS	0x2


#endif /* PLATFORM_DEF_H */

