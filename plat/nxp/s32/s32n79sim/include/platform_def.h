/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <common_def.h>

/* Default APS topology */
#define PLATFORM_SYSTEM_COUNT           U(1)
#define PLATFORM_MAX_CPUS_PER_CLUSTER   U(2)
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER_COUNT * \
					 PLATFORM_MAX_CPUS_PER_CLUSTER)

#define PLATFORM_STACK_SIZE		SZ_4K

/* Power Domains */
#define PLAT_MAX_RET_STATE		U(1)
#define PLAT_MAX_OFF_STATE		U(2)
#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL2

/* Caches */
#define CACHE_WRITEBACK_SHIFT		U(6)
#define CACHE_WRITEBACK_GRANULE		(U(1) << CACHE_WRITEBACK_SHIFT)

/* Start of the reserved memory for all the BL stages.
 * 0xFFF0_0000 by default.
 */
#define S32N_BL_MEM_BASE        (DDR_END - PLAT_BL_MEM_SIZE + 1)

/* BL2 definitions: BL2 is loaded at the beggining of the
 * DDR reserved memory for the BL stages.
 */
#define BL2_BASE    S32N_BL_MEM_BASE
#define BL2_SIZE    SZ_128K
#define BL2_LIMIT   (BL2_BASE + BL2_SIZE - 1)

/* Dummy macros needed for compilation */
#define MAX_IO_DEVICES		U(1)
#define MAX_IO_HANDLES		U(1)

#endif /* PLATFORM_DEF_H */

