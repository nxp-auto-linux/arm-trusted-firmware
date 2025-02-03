/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_UFS_GETTER_H
#define FCONF_UFS_GETTER_H

#include <lib/fconf/fconf.h>

#define hw_config__ufs_config_getter(prop) ufs_config.prop

struct ufs_config_t {
	uintptr_t reg_addr;
	size_t reg_size;
};

extern struct ufs_config_t ufs_config;

#endif /* FCONF_UFS_GETTER_H */
