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

#include <s32n_fconf_xrdc_aida_getter.h>

struct xrdc_aida_config xrdc_aida_config = {0};

static int fdt_check_iommu_cells(const void *fdt, int iommu_phandle)
{
	uint32_t iommu_cells;
	int iommu_node, ret;

	iommu_node = fdt_node_offset_by_phandle(fdt, iommu_phandle);
	if (iommu_node < 0) {
		ERROR("FCONF: IOMMU node not found for phandle %d\n",
		      iommu_phandle);
		return iommu_node;
	}

	ret = fdt_read_uint32(fdt, iommu_node, "#iommu-cells", &iommu_cells);
	if (ret < 0) {
		ERROR("FCONF: Failed to read #iommu-cells property\n");
		return ret;
	}

	if (iommu_cells != 1) {
		ERROR("FCONF: Expected #iommu-cells=1, got %u\n", iommu_cells);
		return -EINVAL;
	}

	return 0;
}

static int fdt_get_streamid(const void *fdt, int node_off, uint32_t *sid)
{
	uint32_t cells[2];
	int ret;

	ret = fdt_read_uint32_array(fdt, node_off, "iommus", 2, cells);
	if (ret < 0)
		return ret;

	/* cells[0] = phandle, cells[1] = streamid */
	ret = fdt_check_iommu_cells(fdt, cells[0]);
	if (ret < 0)
		return ret;

	*sid = cells[1];
	return 0;
}

static int add_new_xrdc_aida_config_entry(uintptr_t base, uint32_t sid)
{
	struct xrdc_aida_config_entry *new_entries, *new_entry;

	/* Reallocate memory for new entry */
	new_entries = realloc(xrdc_aida_config.entries,
			      (xrdc_aida_config.entries_n_elem + 1) *
			      sizeof(struct xrdc_aida_config_entry));
	if (!new_entries) {
		ERROR("FCONF: Failed to reallocate memory for XRDC AIDA config entries\n");
		return -ENOMEM;
	}

	xrdc_aida_config.entries = new_entries;

	new_entry = &xrdc_aida_config.entries[xrdc_aida_config.entries_n_elem++];
	new_entry->base_addr = base;
	new_entry->stream_id = sid;

	return 0;
}

int fconf_populate_xrdc_aida_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	const char *node_name;
	uintptr_t base;
	int node, ret;
	uint32_t sid;

	xrdc_aida_config.entries = NULL;
	xrdc_aida_config.entries_n_elem = 0;

	for (node = fdt_next_node(hw_conf_dtb, -1, NULL); node >= 0;
	     node = fdt_next_node(hw_conf_dtb, node, NULL)) {

		ret = fdt_get_streamid(hw_conf_dtb, node, &sid);
		if (ret)
			continue;

		node_name = fdt_get_name(hw_conf_dtb, node, NULL);

		ret = fdt_get_reg_props_by_index(hw_conf_dtb, node, 0, &base,
						 NULL);
		if (ret != 0) {
			ERROR("FCONF: %s: Failed to read 'reg' property (%d)\n",
			      node_name, ret);
			return ret;
		}

		base = fdtw_translate_address(hw_conf_dtb, node, base);
		if (base == ILLEGAL_ADDR) {
			ERROR("FCONF: %s: Failed to translate address\n",
			      node_name);
			return -EINVAL;
		}

		ret = add_new_xrdc_aida_config_entry(base, sid);
		if (ret != 0) {
			ERROR("FCONF: Failed to add XRDC AIDA config entry\n");
			return ret;
		}

		INFO("XRDC AIDA master entry: node=%s base=0x%lx sid=0x%x\n",
		     node_name, base, sid);
	}

	INFO("FCONF: Populated %u XRDC AIDA entries\n", xrdc_aida_config.entries_n_elem);
	return 0;
}
