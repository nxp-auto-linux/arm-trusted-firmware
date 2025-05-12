/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libfdt.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <s32n_fconf_ufs_getter.h>

static const char *ufs_compatible = "nxp,s32n79-ufshc";
struct ufs_config_t ufs_config = {0};

int fconf_populate_ufs_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int ufs_node, ret;
	uintptr_t base = 0;
	size_t size = 0;

	fdt_for_each_compatible_node(hw_conf_dtb, ufs_node, ufs_compatible) {
		if (!fdt_node_is_enabled(hw_conf_dtb, ufs_node)) {
			continue;
		}

		ret = fdt_get_reg_props_by_index(hw_conf_dtb, ufs_node, 0,
						 &base, &size);
		if (ret)
			continue;

		ufs_config.reg_addr = base;
		ufs_config.reg_size = size;
		return 0;
	}

	WARN("FCONF: Can't find %s compatible node in dtb\n", ufs_compatible);
	return 0;
}

FCONF_REGISTER_POPULATOR(HW_CONFIG, ufs_config, fconf_populate_ufs_config);
