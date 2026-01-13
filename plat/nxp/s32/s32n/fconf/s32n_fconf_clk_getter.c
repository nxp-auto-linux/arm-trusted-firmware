/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <errno.h>
#include <libfdt.h>
#include <fdt_wrappers.h>
#include <string.h>
#include <stdlib.h>
#include <s32n_fconf_clk_getter.h>

struct clk_config clk_config = {0};

static int clk_config_entry_cmp(const void *a, const void *b)
{
	const struct clk_config_entry *entry_a =
		(const struct clk_config_entry *)a;
	const struct clk_config_entry *entry_b =
		(const struct clk_config_entry *)b;

	if (entry_a->reg_base < entry_b->reg_base)
		return -1;
	else if (entry_a->reg_base > entry_b->reg_base)
		return 1;

	return 0;
}

static int fdt_get_clock_cells(const void *fdt, uint32_t phandle)
{
	const fdt32_t *cells_prop;
	int node;

	node = fdt_node_offset_by_phandle(fdt, phandle);
	if (node < 0) {
		return -1;
	}

	cells_prop = fdt_getprop(fdt, node, "#clock-cells", NULL);
	if (!cells_prop) {
		return -1;
	}

	return fdt32_to_cpu(*cells_prop);
}

static uint32_t fdt_get_fixed_clock_frequency(const void *fdt, uint32_t phandle)
{
	const fdt32_t *freq_prop;
	uint64_t freq;
	int node;
	int len;

	node = fdt_node_offset_by_phandle(fdt, phandle);
	if (node < 0)
		return 0;

	freq_prop = fdt_getprop(fdt, node, "clock-frequency", &len);
	if (!freq_prop)
		return 0;

	if (len == sizeof(fdt32_t)) {
		return fdt32_to_cpu(*freq_prop);
	} else if (len == 2 * sizeof(fdt32_t)) {
		freq = fdt64_to_cpu(*(const fdt64_t *)freq_prop);
		return (uint32_t)freq;
	}

	return 0;
}

static int fdt_count_clock_entries(const void *fdt,
				   const fdt32_t *clocks_prop,
				   int prop_len, uint32_t *num_clocks)
{
	uint32_t cells_offset = 0;
	uint32_t count = 0;
	uint32_t phandle;
	int clock_cells;

	while (cells_offset < (prop_len / sizeof(fdt32_t))) {
		phandle = fdt32_to_cpu(clocks_prop[cells_offset]);

		clock_cells = fdt_get_clock_cells(fdt, phandle);
		if (clock_cells < 0) {
			ERROR("FCONF: Failed to get #clock-cells for phandle 0x%x\n",
			      phandle);
			return -FDT_ERR_BADSTRUCTURE;
		}

		if (cells_offset + 1 + clock_cells >
				(prop_len / sizeof(fdt32_t))) {
			ERROR("FCONF: Incomplete clock entry - not enough cells\n");
			return -FDT_ERR_BADSTRUCTURE;
		}

		cells_offset += 1 + clock_cells;
		count++;
	}

	*num_clocks = count;

	return 0;
}

static int fdt_validate_clock_names(const void *fdt, int node_off,
				    uint32_t expected_count)
{
	int names_count;

	names_count = fdt_stringlist_count(fdt, node_off, "clock-names");
	if (names_count < 0)
		return names_count;

	if (expected_count != names_count) {
		ERROR("FCONF: Clock count mismatch: clocks=%u, names=%u\n",
		      expected_count, names_count);
		return -FDT_ERR_BADSTRUCTURE;
	}

	return 0;
}

static int fdt_populate_clock_names(const void *fdt, int node_off,
				    struct fconf_clk_desc *desc_arr, uint32_t count)
{
	uint32_t i;
	int len;

	for (i = 0; i < count; i++) {
		desc_arr[i].name = fdt_stringlist_get(fdt, node_off,
						      "clock-names", i, &len);
		if (!desc_arr[i].name) {
			ERROR("FCONF: Failed to get clock name at index %u\n", i);
			return -FDT_ERR_BADSTRUCTURE;
		}
	}

	return 0;
}

static int fdt_extract_clock_data(const void *fdt,
				  const fdt32_t *clocks_prop,
				  struct fconf_clk_desc *desc_arr, uint32_t count)
{
	uint32_t cells_offset = 0;
	uint32_t i, phandle;
	int clock_cells;

	for (i = 0; i < count; i++) {
		phandle = fdt32_to_cpu(clocks_prop[cells_offset]);

		clock_cells = fdt_get_clock_cells(fdt, phandle);
		if (clock_cells == 0) {
			/* Fixed clock - ID is not meaningful */
			desc_arr[i].type = CLK_TYPE_FIXED;
			desc_arr[i].freq = fdt_get_fixed_clock_frequency(fdt, phandle);
		} else if (clock_cells == 1) {
			/* SCMI based clock with single ID parameter */
			desc_arr[i].type = CLK_TYPE_SCMI;
			desc_arr[i].id = fdt32_to_cpu(clocks_prop[cells_offset + 1]);
			desc_arr[i].freq = 0;
		} else {
			ERROR("FCONF: Unsupported #clock-cells=%d for phandle 0x%x\n",
			      clock_cells, phandle);
			return -FDT_ERR_BADSTRUCTURE;
		}

		cells_offset += 1 + clock_cells;
	}

	return 0;
}

static int fdt_get_clk_descriptors(const void *fdt, int node_off,
				   struct fconf_clk_desc **clk_descs, uint32_t *count)
{
	struct fconf_clk_desc *desc_arr;
	const fdt32_t *clocks_prop;
	uint32_t num_clocks = 0;
	int prop_len = 0;
	int ret;

	clocks_prop = fdt_getprop(fdt, node_off, "clocks", &prop_len);
	if (!clocks_prop)
		return -FDT_ERR_NOTFOUND;

	if (prop_len % sizeof(fdt32_t) != 0) {
		ERROR("FCONF: clocks property length not aligned to cell size\n");
		return -FDT_ERR_BADSTRUCTURE;
	}

	/* Count clock entries */
	ret = fdt_count_clock_entries(fdt, clocks_prop, prop_len, &num_clocks);
	if (ret)
		return ret;

	/* Validate clock-names count matches */
	ret = fdt_validate_clock_names(fdt, node_off, num_clocks);
	if (ret)
		return ret;

	/* Allocate descriptor array */
	desc_arr = malloc(num_clocks * sizeof(struct fconf_clk_desc));
	if (!desc_arr) {
		ERROR("FCONF: Failed to allocate memory for clock descriptors\n");
		return -ENOMEM;
	}

	/* Populate clock names */
	ret = fdt_populate_clock_names(fdt, node_off, desc_arr, num_clocks);
	if (ret) {
		free(desc_arr);
		return ret;
	}

	/* Extract clock data (IDs and frequencies) */
	ret = fdt_extract_clock_data(fdt, clocks_prop, desc_arr, num_clocks);
	if (ret) {
		free(desc_arr);
		return ret;
	}

	*clk_descs = desc_arr;
	*count = num_clocks;

	return 0;
}

static int add_new_clk_config_entry(struct clk_config_entry *entry)
{
	struct clk_config_entry *new_entries;

	/* Reallocate memory for new entry */
	new_entries = realloc(clk_config.entries,
			      (clk_config.entries_n_elem + 1) *
			      sizeof(struct clk_config_entry));
	if (!new_entries) {
		ERROR("FCONF: Failed to reallocate memory for Clock entries\n");
		return -ENOMEM;
	}

	clk_config.entries = new_entries;
	clk_config.entries[clk_config.entries_n_elem] = *entry;
	clk_config.entries_n_elem++;

	return 0;
}

int fconf_populate_clk_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	uint32_t clk_desc_count;
	struct fconf_clk_desc *clk_descs = NULL;
	struct clk_config_entry entry;
	uintptr_t reg_base;
	int node, ret;

	clk_config.entries = NULL;
	clk_config.entries_n_elem = 0;

	for (node = fdt_next_node(hw_conf_dtb, -1, NULL); node >= 0;
	    node = fdt_next_node(hw_conf_dtb, node, NULL)) {

		/* Parse clock descriptors from 'clocks' property */
		ret = fdt_get_clk_descriptors(hw_conf_dtb, node, &clk_descs,
					      &clk_desc_count);
		if (ret)
			continue;

		/* Get base address from 'reg' property */
		ret = fdt_get_reg_props_by_index(hw_conf_dtb, node, 0, &reg_base, NULL);
		if (ret < 0) {
			ERROR("FCONF: Failed to read 'reg' property (ret=%d)\n", ret);
			free(clk_descs);
			continue;
		}

		/* Populate entry with clock descriptors */
		entry.reg_base = reg_base;
		entry.clock_descs = clk_descs;
		entry.clock_descs_n_elem = clk_desc_count;

		ret = add_new_clk_config_entry(&entry);
		if (ret != 0) {
			ERROR("FCONF: Failed to add Clock entry\n");
			free(clk_descs);
			return ret;
		}
	}

	/* Sort clock descriptor entries by their base address */
	qsort(clk_config.entries, clk_config.entries_n_elem,
	      sizeof(struct clk_config_entry), clk_config_entry_cmp);

	INFO("FCONF: Populated %u Clock config entries\n", clk_config.entries_n_elem);

	return 0;
}
