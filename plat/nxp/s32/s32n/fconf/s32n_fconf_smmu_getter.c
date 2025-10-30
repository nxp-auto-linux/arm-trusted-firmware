/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <libfdt.h>
#include <fdt_wrappers.h>
#include <s32n_fconf_smmu_getter.h>

static const char *smmu_compatible = "arm,smmu-v3";
struct smmu_config_t smmu_config = {0};

int fconf_populate_smmu_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int node, ret;
	uintptr_t base = 0;
	size_t size = 0;

	node = fdt_node_offset_by_compatible(hw_conf_dtb, -1, smmu_compatible);
	if (node < 0) {
		INFO("FCONF: SMMU node not found.\n");
		return 0;
	}

	ret = fdt_get_reg_props_by_index(hw_conf_dtb, node, 0, &base, &size);
	if (ret < 0) {
		ERROR("FCONF: Failed to read SMMU reg property\n");
		return ret;
	}

	smmu_config.base = base;

	if (fdt_node_is_enabled(hw_conf_dtb, node)) {
		smmu_config.enabled = true;
	}

	return 0;
}
