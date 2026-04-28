/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32N_FCONF_MEMORY_GETTER_H
#define S32N_FCONF_MEMORY_GETTER_H

#include <lib/fconf/fconf.h>

#define hw_config__dt_memory_regions_getter(prop) dt_memory_regions.prop

struct memory_region_entry {
	uintptr_t base_addr;
	size_t size;
};

struct dt_memory_regions {
	uint32_t entries_n_elem;
	struct memory_region_entry *entries;
};

extern struct dt_memory_regions dt_memory_regions;

int fconf_populate_memory_regions(uintptr_t config);

#endif /* S32N_FCONF_MEMORY_GETTER_H */
