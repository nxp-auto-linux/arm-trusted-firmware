/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>

#include <fconf_extractor.h>

#include "s32n_fconf_clk_getter.h"
#include "s32n_fconf_gic_getter.h"
#include "s32n_fconf_hse_getter.h"
#include "s32n_fconf_memory_getter.h"
#include "s32n_fconf_scmi_getter.h"
#include "s32n_fconf_smmu_getter.h"
#include "s32n_fconf_tlv_getter.h"
#include "s32n_fconf_ufs_getter.h"
#include "s32n_fconf_xrdc_aida_getter.h"

int plat_populate_fconf_extractor(uintptr_t dtb)
{
	int ret;

	ret = fconf_populate_gic_config(dtb);
	if (ret != 0) {
		printf("Failed to parse GIC configuration (return code: %d)\n", ret);
		return ret;
	}

	ret = fconf_populate_smmu_config(dtb);
	if (ret != 0) {
		printf("Failed to parse SMMU configuration(return code: %d)\n", ret);
		return ret;
	}

	ret = fconf_populate_scmi_config(dtb);
	if (ret != 0) {
		printf("Failed to parse SCMI OSPM configuration (return code: %d)\n", ret);
		return ret;
	}

	ret = fconf_populate_tlv_config(dtb);
	if (ret != 0) {
		printf("Failed to parse TLV configuration (return code: %d)\n", ret);
		return ret;
	}

	ret = fconf_populate_ufs_config(dtb);
	if (ret != 0) {
		printf("Failed to parse UFS configuration (return code: %d)\n", ret);
		return ret;
	}

	ret = fconf_populate_hse_config(dtb);
	if (ret != 0) {
		printf("Failed to parse HSE configuration (return code: %d)\n", ret);
		return ret;
	}

	ret = fconf_populate_clk_config(dtb);
	if (ret != 0) {
		printf("Failed to parse Clock configuration (return code: %d)\n", ret);
		return ret;
	}

	ret = fconf_populate_xrdc_aida_config(dtb);
	if (ret != 0) {
		printf("Failed to parse XRDC_AIDA configuration (return code: %d)\n", ret);
		return ret;
	}

	ret = fconf_populate_memory_regions(dtb);
	if (ret != 0) {
		printf("Failed to parse /memory regions (return code: %d)\n", ret);
		return ret;
	}

	return 0;
}
