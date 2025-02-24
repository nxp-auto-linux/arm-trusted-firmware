/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/nxp/s32/hse/hse_utils.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <common/debug.h>
#include <s32cc_sramc.h>
#include <errno.h>

int map_hse_mu_regs(struct hse_memmap *map)
{
	uintptr_t vaddr, paddr;
	size_t size;
	int ret;

	if (!map)
		return -EINVAL;

	paddr = FCONF_GET_PROPERTY(s32cc, hse_regs, paddr);
	vaddr = FCONF_GET_PROPERTY(s32cc, hse_regs, vaddr);
	size = FCONF_GET_PROPERTY(s32cc, hse_regs, size);

	ret = mmap_add_dynamic_region(paddr, vaddr, size, MT_DEVICE | MT_SECURE | MT_RW);
	if (ret)
		return ret;

	map->paddr = paddr;
	map->vaddr = vaddr;
	map->size = size;

	return 0;
}

int map_hse_mu_desc(struct hse_memmap *map)
{
	uintptr_t vaddr, paddr;
	size_t size;
	int ret;

	if (!map)
		return -EINVAL;

	paddr = FCONF_GET_PROPERTY(s32cc, hse_desc, paddr);
	vaddr = FCONF_GET_PROPERTY(s32cc, hse_desc, vaddr);
	size = FCONF_GET_PROPERTY(s32cc, hse_desc, size);

	ret = mmap_add_dynamic_region(paddr, vaddr, size, MT_DEVICE | MT_SECURE | MT_RW);
	if (ret)
		return ret;

	map->paddr = paddr;
	map->vaddr = vaddr;
	map->size = size;

	return 0;
}

int map_hse_res_mem(struct hse_memmap *map, uint32_t slot, bool do_map)
{
	uintptr_t paddr = 0;
	size_t size = 0;
	int ret;

	if (!map)
		return -EINVAL;

	paddr = FCONF_GET_PROPERTY(s32cc, hse_rmem, slot).paddr;
	size = FCONF_GET_PROPERTY(s32cc, hse_rmem, slot).size;

	if (do_map) {
		ret = mmap_add_dynamic_region(paddr, paddr, size,
						MT_NON_CACHEABLE | MT_RW | MT_SECURE);
		if (ret)
			return ret;
	}

	map->paddr = paddr;
	map->vaddr = paddr;
	map->size = size;

	return 0;
}
