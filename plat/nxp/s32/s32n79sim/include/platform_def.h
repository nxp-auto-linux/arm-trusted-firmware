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

#endif /* PLATFORM_DEF_H */

