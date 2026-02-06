/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <common/debug.h>
#include <drivers/clk.h>
#include <fconf/s32n_fconf_clk_getter.h>
#include <string.h>
#include <s32n_clk_utils.h>

static const struct clk_config_entry *bsearch_clk_config_entry(const struct clk_config_entry
							       *entries,
							       uint32_t count, uintptr_t reg_base)
{
	uint32_t mid, left = 0;
	uint32_t right = count;

	while (left < right) {
		mid = left + (right - left) / 2;

		if (entries[mid].reg_base == reg_base)
			return &entries[mid];
		else if (entries[mid].reg_base < reg_base)
			left = mid + 1;
		else
			right = mid;
	}

	return NULL;
}

int get_clock_info(struct clk_info *clk_info,
		   uintptr_t reg_base, const char *clk_name)
{
	const struct clk_config_entry *entries =
		FCONF_GET_PROPERTY(hw_config, clk_config, entries);
	const uint32_t entries_count =
		FCONF_GET_PROPERTY(hw_config, clk_config, entries_n_elem);
	const struct clk_config_entry *entry;
	uint32_t i;

	if (!clk_name || !clk_info) {
		ERROR("Invalid parameters\n");
		return -EINVAL;
	}

	if (!entries || !entries_count) {
		ERROR("No clock configuration entries available\n");
		return -ENOENT;
	}

	/* Binary search for entry with matching base address */
	entry = bsearch_clk_config_entry(entries, entries_count, reg_base);
	if (!entry) {
		WARN("No clock config entry found for base address 0x%lx\n", reg_base);
		return -ENOENT;
	}

	/* Linear search through clock descriptors for matching name */
	for (i = 0; i < entry->clock_descs_n_elem; i++) {
		if (!strcmp(entry->clock_descs[i].name, clk_name)) {
			clk_info->clock_type = entry->clock_descs[i].type;
			clk_info->clk_scmi_id = entry->clock_descs[i].id;
			clk_info->clk_freq = entry->clock_descs[i].freq;

			VERBOSE("Found clock '%s' addr=0x%lx: type=%u, id=0x%x, freq=%u\n",
				clk_name, reg_base, clk_info->clock_type,
				clk_info->clk_scmi_id, clk_info->clk_freq);

			return 0;
		}
	}

	WARN("Clock '%s' not found for base address 0x%lx\n",
	     clk_name, reg_base);

	return -ENOENT;
}

unsigned long s32n_get_clk_freq(uintptr_t base_addr,
				const char *clk_name)
{
	struct clk_info clk_info = { 0 };
	int ret;

	ret = get_clock_info(&clk_info, base_addr, clk_name);
	if (ret) {
		ERROR("Error on get_clock_info: %d\n", ret);
		return 0;
	}

	if (clk_info.clock_type == CLK_TYPE_SCMI)
		return clk_get_rate(clk_info.clk_scmi_id);

	return clk_info.clk_freq;
}
