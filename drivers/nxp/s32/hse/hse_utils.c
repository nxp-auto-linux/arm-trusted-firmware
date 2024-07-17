/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <drivers/nxp/s32/hse/hse_utils.h>
#include <errno.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include "s32cc_dt.h"

static const char *hse_mu_node_comp = "nxp,s32cc-hse";
static const char *crypto_path = "/soc/crypto";

static int get_fdt(void **fdt)
{
	static void *s32_fdt;

	if (!s32_fdt) {
		if (dt_open_and_check() < 0)
			return -FDT_ERR_BADSTATE;

		if (fdt_get_address(&s32_fdt) == 0) {
			ERROR("No Device Tree found");
			return -FDT_ERR_BADSTATE;
		}
	}

	*fdt = s32_fdt;
	return 0;
}

static int get_crypto_node(void *fdt)
{
	static int crypto_node;

	if (!fdt)
		return -FDT_ERR_BADSTATE;

	if (!crypto_node) {
		crypto_node = fdt_path_offset(fdt, crypto_path);
		if (crypto_node < 0)
			return crypto_node;
	}

	return crypto_node;
}

static int get_mu_node_enabled(void *fdt, int start_off)
{
	static int offs = -1;

	if (offs >= 0)
		return offs;

	offs = fdt_node_offset_by_compatible(fdt, start_off, hse_mu_node_comp);
	while (offs != -FDT_ERR_NOTFOUND) {
		if (fdt_get_status(offs) == DT_ENABLED)
			break;

		offs = fdt_node_offset_by_compatible(fdt, offs, hse_mu_node_comp);
	}

	return offs;
}

static int map_hse_mu_space(const char *prop_name, struct hse_memmap *mapping)
{
	int ret, crypto_node, mu_node;
	uintptr_t base;
	size_t size;
	void *fdt;

	if (!prop_name || !mapping)
		return -EINVAL;

	ret = get_fdt(&fdt);
	if (ret) {
		ERROR("Error during device tree access: ret=%d\n", ret);
		return -ENOENT;
	}

	crypto_node = get_crypto_node(fdt);
	if (crypto_node < 0) {
		ERROR("Error while searching for path \"%s\": ret=%d\n",
		      crypto_path, crypto_node);
		return -ENOENT;
	}

	mu_node = get_mu_node_enabled(fdt, crypto_node);
	if (mu_node < 0) {
		ERROR("Could not find enabled MU node with matching compatible \"%s\"",
		      hse_mu_node_comp);
		return -ENOENT;
	}

	ret = fdt_get_reg_props_by_name(fdt, mu_node, prop_name, &base, &size);
	if (ret < 0)
		return -ENOENT;

	ret = mmap_add_dynamic_region(base, base, size, MT_DEVICE | MT_RW);
	if (ret)
		return ret;

	mapping->paddr = base;
	mapping->vaddr = base;
	mapping->size = size;

	return 0;
}

int map_hse_mu_regs(struct hse_memmap *mapping)
{
	return map_hse_mu_space("hse-regs", mapping);
}

int map_hse_mu_desc(struct hse_memmap *mapping)
{
	return map_hse_mu_space("hse-desc", mapping);
}

int map_hse_res_mem(struct hse_memmap *mapping)
{
	int ret, crypto_node, len, mem_region_offs;
	const uint32_t *mem_region_phandle;
	uintptr_t base;
	size_t size;
	void *fdt;

	if (!mapping)
		return -EINVAL;

	ret = get_fdt(&fdt);
	if (ret) {
		ERROR("Error during device tree access: ret=%d\n", ret);
		return -ENOENT;
	}

	crypto_node = get_crypto_node(fdt);
	if (crypto_node < 0) {
		ERROR("Error while searching for path \"%s\": ret=%d\n",
		      crypto_path, crypto_node);
		return -ENOENT;
	}

	mem_region_phandle = fdt_getprop(fdt, crypto_node, "memory-region",
					 &len);
	if (!mem_region_phandle || len != sizeof(uint32_t))
		return -ENOENT;

	mem_region_offs = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*mem_region_phandle));
	if (mem_region_offs < 0)
		return -ENOENT;

	ret = fdt_get_reg_props_by_index(fdt, mem_region_offs, 0, &base, &size);
	if (ret < 0)
		return -ENOENT;

	ret = mmap_add_dynamic_region(base, base, size,
				      MT_NON_CACHEABLE | MT_RW | MT_SECURE);
	if (ret)
		return ret;

	mapping->paddr = base;
	mapping->vaddr = base;
	mapping->size = size;

	return 0;
}
