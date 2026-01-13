/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <libfdt.h>
#include <fdt_wrappers.h>
#include <s32n_fconf_smmu_getter.h>
#include <s32n_fconf_utils.h>

static const char *smmu_compatible = "arm,smmu-v3";
struct smmu_config_t smmu_config = {0};

int fconf_populate_smmu_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	uintptr_t base = 0;
	size_t size = 0;
	int node;

	node = fconf_get_node_info(hw_conf_dtb, -1, smmu_compatible, &base, &size);
	if (node < 0) {
		ERROR("FCONF: Error retrieving %s node info\n", smmu_compatible);
		return node;
	}

	smmu_config.base = base;

	if (fdt_node_is_enabled(hw_conf_dtb, node)) {
		smmu_config.enabled = true;
	}

	return 0;
}
