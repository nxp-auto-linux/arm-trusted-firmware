/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <clk/clk.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <errno.h>
#include <libfdt.h>
#include <platform_def.h>
#include <s32_bl_common.h>
#include <s32_dt.h>

static int fdt_checked;

static void *get_fdt(void)
{
	return (void *)get_bl2_dtb_base();
}

int dt_open_and_check(void)
{
	int ret = fdt_check_header(get_fdt());

	if (ret == 0)
		fdt_checked = 1;

	return ret;
}

int fdt_get_address(void **fdt_addr)
{
	if (fdt_checked == 1)
		*fdt_addr = get_fdt();

	return fdt_checked;
}

uint8_t fdt_get_status(int node)
{
	uint8_t status = DT_DISABLED;
	int len;
	const char *cchar;

	cchar = fdt_getprop(get_fdt(), node, "status", &len);
	if ((cchar == NULL) ||
			(strncmp(cchar, "okay", (size_t)len) == 0)) {
		status = DT_ENABLED;
	}

	return status;
}

int dt_enable_clocks(void *fdt_addr, int node)
{
	const fdt32_t *cuint;
	int len, i, index, ret;
	uint32_t clk_id, clk_drv_id;
	struct clk clk;

	cuint = fdt_getprop(fdt_addr, node, "clocks", &len);
	if (!cuint)
		return 0;

	if (len % FDT_CLOCK_CELL_SIZE) {
		ERROR("Invalid clock definition for node: '%s'\n",
		      fdt_get_name(fdt_addr, node, NULL));
		return -EIO;
	}

	for (i = 0; i < len / FDT_CLOCK_CELL_SIZE; i++) {
		index = i * 2;
		clk_drv_id = fdt32_to_cpu(cuint[index]);
		clk_id = fdt32_to_cpu(cuint[index + 1]);

		ret = get_clk(clk_drv_id, clk_id, &clk);
		if (ret) {
			ERROR("Failed to get the clock (drv:%d clk%d) of the node '%s'\n",
			      clk_drv_id, clk_id,
			      fdt_get_name(fdt_addr, node, NULL));
			return ret;
		}

		ret = clk_enable(&clk);
		if (ret) {
			ERROR("Failed to enable the clock (drv:%d clk:%d) of the node '%s'\n",
			      clk_drv_id, clk_id,
			      fdt_get_name(fdt_addr, node, NULL));
			return ret;
		}
	}

	return 0;
}

void dt_fill_device_info(struct dt_node_info *info, int node)
{
	const fdt32_t *cuint;

	(void) fdt_get_reg_props_by_index(get_fdt(), node, 0,
					  &info->base, NULL);

	cuint = fdt_getprop(get_fdt(), node, "resets", NULL);
	if (cuint != NULL) {
		cuint++;
		info->reset = (int)fdt32_to_cpu(*cuint);
	} else
		info->reset = -1;

	info->status = fdt_get_status(node);
}
