/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <drivers/arm/gicv3.h>
#include <drivers/arm/smmu_v3.h>
#include <platform.h>
#include <platform_def.h>
#include "s32n_plat_funcs.h"
#include <fconf/s32n_fconf_smmu_getter.h>

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

int s32n_smmu_config(void)
{
	bool smmu_enabled = FCONF_GET_PROPERTY(hw_config, smmu_config, enabled);
	uintptr_t smmu_base;
	int ret;

	if (S32N_SMMU_GBPA_GFG == 0)
		return 0;

	/* SMMU GBPA default shareability [13:12] SHCFG needs to be set to "Use Incoming" (0b01)
	 * since the HW defaults as 0b00 ("Non-shareable").
	 */
	if (!smmu_enabled)
		return 0;

	smmu_base = FCONF_GET_PROPERTY(hw_config, smmu_config, base);
	ret = smmuv3_ns_set_gbpa_shcfg(smmu_base, SMMU_GBPA_SHCFG_INCOMING);
	if (ret)
		return ret;

	return smmuv3_s_set_gbpa_shcfg(smmu_base, SMMU_S_GBPA_SHCFG_INCOMING);
}
