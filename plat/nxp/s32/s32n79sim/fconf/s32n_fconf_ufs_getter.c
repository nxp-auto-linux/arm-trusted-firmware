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
	const struct fdt_property *prop;
	int ufs_node, ret;
	uintptr_t base = 0;
	size_t size = 0;
	int len;

	fdt_for_each_compatible_node(hw_conf_dtb, ufs_node, ufs_compatible) {
		if (!fdt_node_is_enabled(hw_conf_dtb, ufs_node)) {
			continue;
		}

		ret = fdt_get_reg_props_by_name(hw_conf_dtb, ufs_node, "ufshc",
						 &base, &size);
		if (ret)
			continue;

		ufs_config.reg_addr = base;
		ufs_config.reg_size = size;
		ufs_config.mphy_boot_mode = MPHY_BOOT_NONE;

		/* SCM registers are necessary when the MPHY initialization
		 * should be handled by TF-A.
		 * If MPHY init is skipped, SCM memory area is not necessary.
		 */
		ret = fdt_get_reg_props_by_name(hw_conf_dtb, ufs_node, "scm",
						 &base, &size);
		if (ret)
			return 0;

		ufs_config.scm_reg_addr = base;
		ufs_config.scm_reg_size = size;

		/* Identify MPHY boot mode. */
		prop = fdt_get_property(hw_conf_dtb, ufs_node, "nxp,mphy-boot-mode", &len);
		if (prop == NULL) {
			return 0;
		}

		if (!strcmp(prop->data, "rom")) {
			ufs_config.mphy_boot_mode = MPHY_BOOT_ROM;
		} else if (!strcmp(prop->data, "copy-sram")) {
			ufs_config.mphy_boot_mode = MPHY_BOOT_COPY_SRAM;
		} else {
			ERROR("Unrecognized nxp,mphy-boot-mode property. UFS MPHY init will be skipped.\n");
			return 0;
		}

		/* Get the core clock id. */
		ret = fdt_get_clock_id_by_name(hw_conf_dtb, ufs_node,
					       "core_clk",
					       &ufs_config.core_clock_id);
		if (ret) {
			ERROR("Couldn't get UFS core clock.\n");
			return ret;
		}

		return 0;
	}

	WARN("FCONF: Can't find %s compatible node in dtb\n", ufs_compatible);
	return 0;
}
