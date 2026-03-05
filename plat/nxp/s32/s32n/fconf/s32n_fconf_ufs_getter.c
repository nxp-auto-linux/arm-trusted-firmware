/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libfdt.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <s32n_fconf_ufs_getter.h>

static const char * const ufs_compatibles[] = {
	"nxp,s32n79-ufshc",
	"nxp,s32n79-ufshc-virt",
};

struct ufs_config_t ufs_config = {0};

int fconf_populate_ufs_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	const struct fdt_property *prop;
	int ufs_node, ret;
	uintptr_t base = 0;
	bool found = false;
	size_t size = 0;
	int len, i;

	for (i = 0; i < ARRAY_SIZE(ufs_compatibles) && !found; i++) {
		fdt_for_each_compatible_node(hw_conf_dtb, ufs_node, ufs_compatibles[i]) {
			if (!fdt_node_is_enabled(hw_conf_dtb, ufs_node)) {
				continue;
			} else {
				found = true;
				break;
			}
		}
	}

	if (!found) {
		WARN("FCONF: Can't find UFS compatible node in dtb\n");
		return 0;
	}

	ret = fdt_get_reg_props_by_name(hw_conf_dtb, ufs_node, "ufshc",
					 &base, &size);
	if (ret)
		return -FDT_ERR_BADVALUE;

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
	}

	return 0;
}
