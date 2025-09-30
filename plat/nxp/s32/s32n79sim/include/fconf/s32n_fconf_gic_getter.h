/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_GIC_GETTER_H
#define FCONF_GIC_GETTER_H

#include <lib/fconf/fconf.h>

#define hw_config__gic_config_getter(prop) gic_config.prop

struct gic_config_t {
	uint64_t gicd_base;
	uint64_t gicr_base;
	size_t gicd_size;
	size_t gicr_size;
	uint8_t gic_mode;
	uint8_t instance;
};

extern struct gic_config_t gic_config;

int fconf_populate_gic_config(uintptr_t config);

#endif /* FCONF_GIC_GETTER_H */
