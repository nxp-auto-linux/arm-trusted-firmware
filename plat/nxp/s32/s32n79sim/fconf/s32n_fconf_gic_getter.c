/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <libfdt.h>
#include <s32n_fconf_gic_getter.h>

static const char *gicv3_compatible = "arm,gic-v3";
struct gic_config_t gic_config = {0};

int fconf_populate_gic_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int node, ret;
	uintptr_t base = 0;
	size_t size = 0;

	node = fdt_node_offset_by_compatible(hw_conf_dtb, -1, gicv3_compatible);
	if (node < 0) {
		ERROR("FCONF: Missing node with %s compatible.\n", gicv3_compatible);
		return 0;
	}

	ret = fdt_get_reg_props_by_index(hw_conf_dtb, node, 0, &base, &size);
	if (ret < 0) {
		ERROR("FCONF: Failed to read GICD reg property\n");
		return ret;
	}

	gic_config.gicd_base = base;
	gic_config.gicd_size = size;

	ret = fdt_get_reg_props_by_index(hw_conf_dtb, node, 1, &base, &size);
	if (ret < 0) {
		ERROR("FCONF: Failed to read GICR reg property\n");
		return ret;
	}

	gic_config.gicr_base = base;
	gic_config.gicr_size = size;

	return 0;
}

FCONF_REGISTER_POPULATOR(HW_CONFIG, gic_config, fconf_populate_gic_config);
