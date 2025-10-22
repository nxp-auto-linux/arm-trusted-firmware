/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libfdt.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <s32n_fconf_sdhc_getter.h>

static const char *usdhc_compatible = "nxp,s32n-usdhc";
struct sdhc_config_t sdhc_config = {0};

int fconf_populate_sdhc_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int sdhc_node, ret;

	sdhc_node = fdt_node_offset_by_compatible(hw_conf_dtb, -1, usdhc_compatible);
	if (sdhc_node < 0) {
		ERROR("FCONF: Missing node with %s compatible.\n", usdhc_compatible);
		return 0;
	}

	/* Get the per clock id. */
	ret = fdt_get_clock_id_by_name(hw_conf_dtb, sdhc_node, "per",
					   &sdhc_config.per_clock_id);
	if (ret) {
		ERROR("Couldn't get uSDHC per clock: %d\n", ret);
		return ret;
	}

	return 0;
}
