/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libfdt.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <s32n_fconf_hse_getter.h>

static const char *mu_compatible = "nxp,s32n79-mu-hse";
static const char *rmem_compatible = "nxp,s32n79-hse-rmem";

struct hse_config hse_config = {0};

static void fconf_populate_mu_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int mu_node, rc;
	uintptr_t base = 0;
	size_t size = 0;

	fdt_for_each_compatible_node(hw_conf_dtb, mu_node, mu_compatible) {
		if (!fdt_node_is_enabled_secure(hw_conf_dtb, mu_node)) {
			continue;
		}

		rc = fdt_get_reg_props_by_index(hw_conf_dtb, mu_node, 0,
						&base, &size);
		if (rc)
			continue;

		hse_config.mu_base = base;
		hse_config.mu_size = size;
		return;
	}

	WARN("FCONF: Can't find %s compatible node in dtb\n", mu_compatible);
}

static void fconf_populate_rmem_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int rmem_node, rc;
	uintptr_t base = 0;
	size_t size = 0;

	fdt_for_each_compatible_node(hw_conf_dtb, rmem_node, rmem_compatible) {
		if (!fdt_node_is_enabled_secure(hw_conf_dtb, rmem_node)) {
			continue;
		}

		rc = fdt_get_reg_props_by_index(hw_conf_dtb, rmem_node, 0,
						&base, &size);
		if (rc)
			continue;

		hse_config.rmem_base = base;
		hse_config.rmem_size = size;
		return;
	}

	WARN("FCONF: Can't find %s compatible node in dtb\n", rmem_compatible);
}

int fconf_populate_hse_config(uintptr_t config)
{
	fconf_populate_mu_config(config);

	fconf_populate_rmem_config(config);

	return 0;
}
