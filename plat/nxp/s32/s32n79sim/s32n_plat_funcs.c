/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <drivers/arm/gicv3.h>
#include <platform.h>
#include <platform_def.h>
#include "s32n_plat_funcs.h"

static gicv3_redist_ctx_t rdisif_ctxs[PLATFORM_CORE_COUNT];
static gicv3_dist_ctx_t dist_ctx;

void s32n_plat_gic_save(bool system_suspend)
{
	unsigned int pos = plat_my_core_pos();
	unsigned int i;

	assert(pos < PLATFORM_CORE_COUNT);

	if (system_suspend) {
		/* save the redistributors for all the CPUs */
		for (i = 0; i < PLATFORM_CORE_COUNT; i++)
			gicv3_rdistif_save(i, &rdisif_ctxs[i]);

		gicv3_distif_save(&dist_ctx);
	} else
		gicv3_rdistif_save(pos, &rdisif_ctxs[pos]);
}

void s32n_plat_gic_restore(bool system_suspend)
{
	unsigned int pos = plat_my_core_pos();
	unsigned int i;

	assert(pos < PLATFORM_CORE_COUNT);

	if (system_suspend) {
		gicv3_distif_init_restore(&dist_ctx);

		/* restore the redistributors for all the CPUs */
		for (i = 0; i < PLATFORM_CORE_COUNT; i++)
			gicv3_rdistif_init_restore(i, &rdisif_ctxs[i]);
	} else
		gicv3_rdistif_init_restore(pos, &rdisif_ctxs[pos]);
}

