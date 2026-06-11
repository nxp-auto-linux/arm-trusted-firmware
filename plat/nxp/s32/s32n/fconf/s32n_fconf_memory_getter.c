/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <errno.h>
#include <libfdt.h>
#include <fdt_wrappers.h>
#include <stdlib.h>

#include <s32n_fconf_memory_getter.h>

#define MEMORY_STRING		"memory"

/*
 * Consumers (e.g. the XRDC MIDAC driver) program protection hardware at a
 * 4 KB granule, so /memory entries must be 4 KB-aligned and non-empty. The
 * granule is defined locally because this getter is also built for the host
 * fconf-extractor tool, where platform/arch headers are unavailable.
 */
#define MEMORY_REGION_ALIGN	0x1000U

struct dt_memory_regions dt_memory_regions = {0};

static int add_new_memory_region(uintptr_t base, size_t size)
{
	struct memory_region_entry *new_regions, *region;

	new_regions = realloc(dt_memory_regions.entries,
			      (dt_memory_regions.entries_n_elem + 1) *
			      sizeof(struct memory_region_entry));
	if (!new_regions) {
		ERROR("FCONF: Failed to reallocate memory for /memory entries\n");
		return -ENOMEM;
	}

	dt_memory_regions.entries = new_regions;

	region = &dt_memory_regions.entries[dt_memory_regions.entries_n_elem++];
	region->base_addr = base;
	region->size = size;

	return 0;
}

int fconf_populate_memory_regions(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	const char *node_name;
	uintptr_t base;
	size_t size;
	int node = -1, ret, i;

	dt_memory_regions.entries = NULL;
	dt_memory_regions.entries_n_elem = 0;

	while ((node = fdt_node_offset_by_prop_value(hw_conf_dtb, node,
						     "device_type", MEMORY_STRING,
						     sizeof(MEMORY_STRING))) >= 0) {

		node_name = fdt_get_name(hw_conf_dtb, node, NULL);

		/* A /memory node may carry multiple (base, size) pairs. */
		for (i = 0; ; i++) {
			ret = fdt_get_reg_props_by_index(hw_conf_dtb, node, i,
							 &base, &size);
			if (ret != 0) {
				if (i == 0) {
					ERROR("FCONF: %s: failed to read 'reg' (%d)\n",
					      node_name, ret);
					return ret;
				}
				break;	/* no more reg entries in this node */
			}

			/* base and size must be non-zero and 4 KB-aligned. */
			if (size == 0U ||
			    (base & (MEMORY_REGION_ALIGN - 1U)) != 0U ||
			    (size & (MEMORY_REGION_ALIGN - 1U)) != 0U) {
				ERROR("FCONF: %s: entry [0x%lx+0x%zx] not 4 KB aligned\n",
				      node_name, base, size);
				return -EINVAL;
			}

			ret = add_new_memory_region(base, size);
			if (ret != 0) {
				ERROR("FCONF: failed to add /memory region\n");
				return ret;
			}

			INFO("Memory region: node=%s base=0x%lx size=0x%zx\n",
			     node_name, base, size);
		}
	}

	INFO("FCONF: Populated %u /memory region(s)\n",
	     dt_memory_regions.entries_n_elem);
	return 0;
}
