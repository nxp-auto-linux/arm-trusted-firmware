/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_SMMU_GETTER_H
#define FCONF_SMMU_GETTER_H

#include <lib/fconf/fconf.h>
#include <stdbool.h>


#define hw_config__smmu_config_getter(prop) smmu_config.prop

struct smmu_config_t {
	uintptr_t base;
	bool enabled;
};

extern struct smmu_config_t smmu_config;

int fconf_populate_smmu_config(uintptr_t config);

#endif /* FCONF_SMMU_GETTER_H */
