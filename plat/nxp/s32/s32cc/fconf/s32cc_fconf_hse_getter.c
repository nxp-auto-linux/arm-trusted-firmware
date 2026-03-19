/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libfdt.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <s32cc_fconf_hse_getter.h>
#include <s32cc_platform_def.h>
#include <utils_def.h>
#include "s32cc_dt.h"

static const char *hse_mu_node_comp = "nxp,s32cc-hse";
static const char *crypto_path = "/soc/crypto";

struct hse_memmap hse_rmem_memmap[HSE_RMEM_SLOTS_NUM] = {0};
struct hse_memmap hse_desc_memmap = {0};
struct hse_memmap hse_regs_memmap = {0};

static int get_crypto_node(const void *fdt)
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

static int get_mu_node_enabled(const void *fdt, int start_off)
{
	static int hse_mu_node_off = -1;

	if (hse_mu_node_off >= 0)
		return hse_mu_node_off;

	hse_mu_node_off = fdt_node_offset_by_compatible(fdt, start_off, hse_mu_node_comp);
	while (hse_mu_node_off != -FDT_ERR_NOTFOUND) {
		if (_fdt_get_status(hse_mu_node_off) == DT_ENABLED)
			break;

		hse_mu_node_off = fdt_node_offset_by_compatible(fdt, hse_mu_node_off,
								hse_mu_node_comp);
	}

	return hse_mu_node_off;
}

static int hse_mu_mem_setter(const void *fdt, const char *prop_name)
{
	struct hse_memmap *memmap = NULL;
	int ret, crypto_node, mu_node;
	uintptr_t base = 0;
	size_t size = 0;

	if (!prop_name)
		return -FDT_ERR_BADVALUE;

	if (!strncmp(prop_name, "hse-regs", strlen(prop_name))) {
		memmap = &hse_regs_memmap;
	} else if (!strncmp(prop_name, "hse-desc", strlen(prop_name))) {
		memmap = &hse_desc_memmap;
	}
	if (!memmap)
		return -FDT_ERR_BADVALUE;

	crypto_node = get_crypto_node(fdt);
	if (crypto_node < 0) {
		ERROR("FCONF: Error while searching for path \"%s\": ret=%d\n",
		      crypto_path, crypto_node);
		return crypto_node;
	}

	mu_node = get_mu_node_enabled(fdt, crypto_node);
	if (mu_node < 0) {
		ERROR("FCONF: Could not find enabled MU node with matching compatible \"%s\"",
		      hse_mu_node_comp);
		return mu_node;
	}

	ret = fdt_get_reg_props_by_name(fdt, mu_node, prop_name, &base, &size);
	if (ret < 0) {
		ERROR("FCONF: Error while getting \"%s\": ret=%d\n", prop_name, ret);
		return ret;
	}

	memmap->paddr = base;
	memmap->vaddr = base;
	memmap->size = size;

	return 0;
}

static int hse_set_mu_regs(const void *fdt)
{
	return hse_mu_mem_setter(fdt, "hse-regs");
}

static int hse_set_mu_desc(const void *fdt)
{
	return hse_mu_mem_setter(fdt, "hse-desc");
}

int hse_set_fdt_rmem(const void *fdt, uint32_t slot)
{
	int ret, crypto_node, len = 0, mem_region_offs;
	const uint32_t *mem_region_phandle;
	uintptr_t base = 0;
	size_t size = 0;

	crypto_node = get_crypto_node(fdt);
	if (crypto_node < 0) {
		ERROR("FCONF: Error while searching for path \"%s\": ret=%d\n",
		      crypto_path, crypto_node);
		return -FDT_ERR_NOTFOUND;
	}

	mem_region_phandle = fdt_getprop(fdt, crypto_node, "memory-region", &len);
	if (!mem_region_phandle || len != sizeof(uint32_t))
		return -FDT_ERR_NOTFOUND;

	mem_region_offs = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*(mem_region_phandle)));
	if (mem_region_offs < 0)
		return mem_region_offs;

	ret = fdt_get_reg_props_by_index(fdt, mem_region_offs, 0, &base, &size);
	if (ret < 0)
		return ret;

	hse_rmem_memmap[slot].paddr = base;
	hse_rmem_memmap[slot].size = size;

	return 0;
}

static int hse_set_mu_space(const void *fdt)
{
	int ret = 0;
	static bool hse_mu_space_populated;

	if (hse_mu_space_populated)
		return 0;

	ret = hse_set_mu_desc(fdt);
	if (ret) {
		ERROR("FCONF: Couldn't populate HSE desc %d\n", ret);
		goto err_out;
	}

	ret = hse_set_mu_regs(fdt);
	if (ret) {
		ERROR("FCONF: Couldn't populate HSE regs %d\n", ret);
		goto err_out;
	}
	hse_mu_space_populated = true;

err_out:
	return ret;
}

int fconf_populate_hse_conf(uintptr_t config)
{
	const void *fdt = (const void *)config;
	int ret = 0;

	ret = hse_set_mu_space(fdt);
	if (ret)
		goto err_out;

	ret = hse_set_rmem_space(fdt);
err_out:
	return ret;
}

FCONF_REGISTER_POPULATOR(HSE_CONFIG, hse_memmap, fconf_populate_hse_conf);
