/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_UFS_GETTER_H
#define FCONF_UFS_GETTER_H

#include <lib/fconf/fconf.h>

#define hw_config__ufs_config_getter(prop) ufs_config.prop

/**
 * enum mphy_boot_mode - MPHY boot mode options
 * @MPHY_BOOT_NONE: Skip MPHY initialization
 * @MPHY_BOOT_ROM: Running MPHY from internal FW ROM
 * @MPHY_BOOT_COPY_SRAM: Running MPHY from a (modified) FW ROM copy in external SRAM
 */
enum mphy_boot_mode {
	MPHY_BOOT_NONE = 0,
	MPHY_BOOT_ROM = 1,
	MPHY_BOOT_COPY_SRAM = 2,
};

struct ufs_config_t {
	uintptr_t reg_addr;
	size_t reg_size;
	uintptr_t scm_reg_addr;
	size_t scm_reg_size;
	uint8_t mphy_boot_mode;
};

extern struct ufs_config_t ufs_config;

int fconf_populate_ufs_config(uintptr_t config);

#endif /* FCONF_UFS_GETTER_H */
